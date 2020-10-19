/*
  This source is part of the libosmscout library
  Copyright (C) 2016  Tim Teulings

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#include <osmscout/import/GenCoordDat.h>

#include <osmscout/private/Config.h>

#if defined(HAVE_STD_EXECUTION)
  #include <execution>
#endif
#include <limits>
#include <map>

#include <osmscout/CoordDataFile.h>

#include <osmscout/import/Preprocess.h>
#include <osmscout/import/RawCoord.h>

namespace osmscout {

  static const uint32_t coordSortPageSize=5000000;
  static const uint32_t coordDiskPageSize=64;
  static const uint32_t coordDiskSize=8;

  static inline bool SortCoordsByOSMId(const RawCoord& a, const RawCoord& b)
  {
    return a.GetOSMId()<b.GetOSMId();
  }

  bool CoordDataGenerator::FindDuplicateCoordinates(const TypeConfig& typeConfig,
                                                    const ImportParameter& parameter,
                                                    Progress& progress,
                                                    std::unordered_map<Id,uint8_t>& duplicates) const
  {
    progress.SetAction("Searching for duplicate coordinates");

    Id          maxId=GeoCoord(90.0,180.0).GetId();
    Id          currentLowerLimit=0;
    Id          currentUpperLimit=maxId/coordSortPageSize;
    FileScanner scanner;

    try {
      uint32_t    loadedCoordCount=0;

      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   Preprocess::RAWCOORDS_DAT),
                   FileScanner::Sequential,
                   true);

      scanner.GotoBegin();

      uint32_t coordCount=scanner.ReadUInt32();

      while (loadedCoordCount<coordCount) {
        std::map<Id,std::vector<Id>> coordPages;
        uint32_t                     currentCoordCount=0;

        progress.Info("Searching for coordinates with page id >= "+std::to_string(currentLowerLimit));

        scanner.GotoBegin();

        coordCount=scanner.ReadUInt32();

        RawCoord coord;

        for (uint32_t i=1; i<=coordCount; i++) {
          progress.SetProgress(i,coordCount);

          coord.Read(typeConfig,scanner);

          Id id=coord.GetCoord().GetId();
          Id pageId=id/coordSortPageSize;


          if (pageId<currentLowerLimit || pageId>currentUpperLimit) {
            continue;
          }

          coordPages[pageId].push_back(id);
          currentCoordCount++;
          loadedCoordCount++;

          if (loadedCoordCount==coordCount) {
            break;
          }

          if (currentCoordCount>parameter.GetRawCoordBlockSize()
              && coordPages.size()>1) {
            [[maybe_unused]] Id oldUpperLimit=currentUpperLimit;

            while (currentCoordCount>parameter.GetRawCoordBlockSize()
                   && coordPages.size()>1) {
              auto pageEntry=coordPages.rbegin();
              Id highestPageId=pageEntry->first;

              currentCoordCount-=(uint32_t)pageEntry->second.size();
              loadedCoordCount-=(uint32_t)pageEntry->second.size();
              coordPages.erase(highestPageId);
              currentUpperLimit=highestPageId-1;
            }

            assert(currentUpperLimit<oldUpperLimit);
          }
        }

        progress.Info("Sorting coordinates");

        for (auto& entry : coordPages) {
#if defined(HAVE_STD_EXECUTION)
          std::sort(std::execution::par_unseq,
                    entry.second.begin(),
                    entry.second.end());

#else
          std::sort(entry.second.begin(),
                    entry.second.end());
#endif
        }

        progress.Info("Detect duplicates");

        // We currently assume that coordinates are ordered by increasing id
        // So if we have to nodes with the same coordinate we can expect them
        // to have the same serial, as long as above is true and nodes
        // for a coordinate are either all part of the import file - or all are left out.

        for (auto& entry : coordPages) {
          Id lastId=std::numeric_limits<Id>::max();

          bool flagged=false;
          for (auto& id : entry.second) {
            if (id==lastId) {
              if (!flagged) {
                duplicates[id]=1;
                flagged=true;
              }
            }
            else {
              flagged=false;
            }

            lastId=id;
          }
        }

        progress.Info("Loaded "+std::to_string(currentCoordCount)+" coords (" +std::to_string(loadedCoordCount)+"/"+std::to_string(coordCount)+")");

        currentLowerLimit=currentUpperLimit+1;
        currentUpperLimit=maxId/coordSortPageSize;
      }

      progress.Info("Found "+std::to_string(duplicates.size())+" duplicate cordinates");

      scanner.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      scanner.CloseFailsafe();

      return false;
    }

    return true;
  }

  bool CoordDataGenerator::DumpCurrentPage(FileWriter& writer,
                                           std::vector<PageEntry>& page) const
  {
    bool somethingToStore=std::any_of(page.begin(),
                                      page.end(),
                                      [](PageEntry entry) {
                                        return entry.isSet;
                                      });


    if (!somethingToStore) {
      return false;
    }

    for (const PageEntry entry : page) {
      if (entry.isSet) {
        writer.Write(entry.point.GetSerial());
        writer.WriteCoord(entry.point.GetCoord());
      }
      else {
        writer.Write((uint8_t)0);
        writer.WriteInvalidCoord();
      }
    }

    return true;
  }


  bool CoordDataGenerator::StoreCoordinates(const TypeConfig& typeConfig,
                                            const ImportParameter& parameter,
                                            Progress& progress,
                                            std::unordered_map<Id,uint8_t>& duplicates) const
  {
    progress.SetAction("Storing coordinates");

    OSMId                  maxId=std::numeric_limits<OSMId>::max();
    OSMId                  currentLowerLimit=std::numeric_limits<OSMId>::min()/coordSortPageSize;
    OSMId                  currentUpperLimit=maxId/coordSortPageSize;
    FileScanner            scanner;
    FileWriter             writer;
    PageId                 currentPageId=0;
    std::vector<PageEntry> page(coordDiskPageSize);

    std::unordered_map<OSMId,FileOffset> pageFileOffsetIndex;

    try {
      uint32_t loadedCoordCount=0;

      writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                  CoordDataFile::COORD_DAT));

      writer.WriteFileOffset(0);
      writer.Write(coordDiskPageSize);
      writer.FlushCurrentBlockWithZeros(coordSortPageSize*coordDiskSize);

      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   Preprocess::RAWCOORDS_DAT),
                   FileScanner::Sequential,
                   true);

      scanner.GotoBegin();

      uint32_t coordCount=scanner.ReadUInt32();

      while (loadedCoordCount<coordCount) {
        std::map<Id,std::vector<RawCoord>> coordPages;
        uint32_t                           currentCoordCount=0;

        progress.Info("Search for coordinates with page id >= "+std::to_string(currentLowerLimit));

        scanner.GotoBegin();

        coordCount=scanner.ReadUInt32();

        RawCoord coord;

        for (uint32_t i=1; i<=coordCount; i++) {
          progress.SetProgress(i,coordCount);

          coord.Read(typeConfig,scanner);

          OSMId id=coord.GetOSMId();
          OSMId pageId=id/coordSortPageSize;

          if (pageId<currentLowerLimit || pageId>currentUpperLimit) {
            continue;
          }

          coordPages[pageId].push_back(coord);
          currentCoordCount++;
          loadedCoordCount++;

          if (loadedCoordCount==coordCount) {
            break;
          }

          if (currentCoordCount>parameter.GetRawCoordBlockSize()
              && coordPages.size()>1) {
            [[maybe_unused]] OSMId oldUpperLimit=currentUpperLimit;

            while (currentCoordCount>parameter.GetRawCoordBlockSize()
                   && coordPages.size()>1) {
              auto pageEntry=coordPages.rbegin();
              OSMId highestPageId=pageEntry->first;

              currentCoordCount-=(uint32_t)pageEntry->second.size();
              loadedCoordCount-=(uint32_t)pageEntry->second.size();
              coordPages.erase(highestPageId);
              currentUpperLimit=highestPageId-1;
            }

            assert(currentUpperLimit<oldUpperLimit);
          }
        }

        progress.Info("Sorting coordinates");

        for (auto& entry : coordPages) {
#if defined(HAVE_STD_EXECUTION)
          std::sort(std::execution::par_unseq,
                    entry.second.begin(),
                    entry.second.end(),
                    SortCoordsByOSMId);

#else
          std::sort(entry.second.begin(),
                    entry.second.end(),
                    SortCoordsByOSMId);
#endif
        }

        progress.Info("Write coordinates");

        for (const auto& entry : coordPages) {
          for (const auto& osmCoord : entry.second) {
            uint8_t serial=1;
            auto    duplicateEntry=duplicates.find(osmCoord.GetCoord().GetId());

            if (duplicateEntry!=duplicates.end()) {
              serial=duplicateEntry->second;

              if (serial==255) {
                progress.Error("Coordinate "+std::to_string(osmCoord.GetOSMId())+" "+osmCoord.GetCoord().GetDisplayText()+" has more than 256 nodes");
                continue;
              }

              duplicateEntry->second++;
            }

            PageId relatedId=osmCoord.GetOSMId()+std::numeric_limits<OSMId>::min();
            PageId pageId=relatedId/coordDiskPageSize;

            if (currentPageId!=pageId) {
              FileOffset pageOffset=writer.GetPos();

              if (DumpCurrentPage(writer,
                                  page)) {
                pageFileOffsetIndex[currentPageId]=pageOffset;
              }

              std::for_each(page.begin(),page.end(),[](PageEntry& entry) {
                entry.isSet=false;
              });
              currentPageId=pageId;
            }

            size_t pageIndex=relatedId%coordDiskPageSize;

            page[pageIndex]=PageEntry{true,
                                      Point(serial,
                                                  osmCoord.GetCoord())};
          }
        }

        FileOffset pageOffset=writer.GetPos();

        if (DumpCurrentPage(writer,
                            page)) {
          pageFileOffsetIndex[currentPageId]=pageOffset;
        }

        progress.Info("Loaded "+std::to_string(currentCoordCount)+" coords (" +std::to_string(loadedCoordCount)+"/"+std::to_string(coordCount)+")");

        currentLowerLimit=currentUpperLimit+1;
        currentUpperLimit=maxId/coordSortPageSize;
      }

      FileOffset indexStartOffset=writer.GetPos();

      progress.SetAction("Writing "+std::to_string(pageFileOffsetIndex.size())+" index entries to disk");

      writer.Write((uint32_t)pageFileOffsetIndex.size());

      for (const auto entry : pageFileOffsetIndex) {
        writer.Write(entry.first);
        writer.Write(entry.second);
      }


      scanner.Close();

      writer.GotoBegin();
      writer.WriteFileOffset(indexStartOffset);
      writer.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      scanner.CloseFailsafe();
      writer.CloseFailsafe();

      return false;
    }

    return true;
  }

  void CoordDataGenerator::GetDescription(const ImportParameter& /*parameter*/,
                                          ImportModuleDescription& description) const
  {
    description.SetName("CoordDataGenerator");
    description.SetDescription("Generate coord data file");

    description.AddRequiredFile(Preprocess::RAWCOORDS_DAT);

    description.AddProvidedDebuggingFile(CoordDataFile::COORD_DAT);
  }

  bool CoordDataGenerator::Import(const TypeConfigRef& typeConfig,
                                  const ImportParameter& parameter,
                                  Progress& progress)
  {
    std::unordered_map<Id,uint8_t> duplicates;

    if (!FindDuplicateCoordinates(*typeConfig,
                                  parameter,
                                  progress,
                                  duplicates)) {
      return false;
    }

    if (!StoreCoordinates(*typeConfig,
                          parameter,
                          progress,
                          duplicates)) {
      return false;
    }

    return true;
  }
}
