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

  static const uint32_t coordSortPageSize=100000000;
  static const uint32_t coordDiskPageSize=64;
  static const uint32_t coordDiskSize=8;

  void SerialIdManager::MarkIdAsDuplicate(Id id)
  {
    idToSerialMap[id]=1;
  }

  uint8_t SerialIdManager::GetNextSerialForId(Id id)
  {
    auto entry=idToSerialMap.find(id);

    if (entry!=idToSerialMap.end()) {
      auto currentValue=entry->second;

      entry->second++;

      return currentValue;
    }

    return 1;
  }

  size_t SerialIdManager::Size() const
  {
    return idToSerialMap.size();
  }

  /**
   We currently assume that coordinates are ordered by increasing id
   So if we have to nodes with the same coordinate we can expect them
   to have the same serial, as long as above is true and nodes
   for a coordinate are either all part of the import file - or all are left out.
  */
  static void ProcessIds(const std::vector<Id>& ids,
                         SerialIdManager& serialIdManager)
  {
    Id   lastId                      =std::numeric_limits<Id>::max();
    bool currentIdDetectedAsDuplicate=false;

    for (auto id : ids) {
      if (id==lastId) {
        if (!currentIdDetectedAsDuplicate) {
          serialIdManager.MarkIdAsDuplicate(id);
          currentIdDetectedAsDuplicate=true;
        }
      }
      else {
        currentIdDetectedAsDuplicate=false;
      }

      lastId=id;
    }
  }

  static void SortCoordPage(std::vector<Id>& coordPage)
  {
#if defined(HAVE_STD_EXECUTION)
      std::sort(std::execution::par_unseq,
                    coordPage.begin(),
                    coordPage.end());

#else
      std::sort(coordPage.begin(),
                coordPage.end());
#endif
  }

  static inline bool SortCoordsByOSMId(const RawCoord& a, const RawCoord& b)
  {
    return a.GetOSMId()<b.GetOSMId();
  }

  static void SortCoordPages(std::map<Id,std::vector<RawCoord>>& coordPages)
  {
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
  }

  bool CoordDataGenerator::FindDuplicateCoordinates(const TypeConfig& typeConfig,
                                                    const ImportParameter& parameter,
                                                    Progress& progress,
                                                    SerialIdManager& serialIdManager) const
  {
    progress.SetAction("Searching for duplicate coordinates");

    FileScanner scanner;

    try {
      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   Preprocess::RAWCOORDS_DAT),
                   FileScanner::Sequential,
                   true);

      scanner.GotoBegin();

      uint32_t coordCount=scanner.ReadUInt32();

      PageManager<Id,Id> pageManager(parameter.GetRawCoordBlockSize(),
                                     coordSortPageSize,
                                     coordCount,
                                     [&serialIdManager](const Id& /*id*/, const std::vector<Id>&& page) {
         std::vector<Id> ids(page);

         SortCoordPage(ids);
         ProcessIds(ids,serialIdManager);
       });

      while (!pageManager.AreAllEntriesLoaded()) {
        std::map<Id,std::vector<Id>> coordPages;

        progress.Info("Searching for coordinates with page id >= "+std::to_string(pageManager.GetMinPageId()));

        scanner.GotoBegin();

        coordCount=scanner.ReadUInt32();

        RawCoord coord;

        for (uint32_t i=1; i<=coordCount; i++) {
          progress.SetProgress(i,coordCount);

          coord.Read(typeConfig,scanner);

          Id id=coord.GetCoord().GetId();

          if (!pageManager.IsACurrentlyHandledPage(id)) {
            continue;
          }

          // Since different coords may have the same id, it is possible that a page contains
          // multiple instances of the same id (and a page may container more then
          // 'coordSortPageSize' entries
          pageManager.AddEntry(id,id);

          // Shortcut loading of file in the last iteration
          if (pageManager.AreAllEntriesLoaded()) {
            break;
          }
        }

        pageManager.FileCompletelyScanned();

        progress.Info("Loaded "+std::to_string(pageManager.GetProcessedPagesEntryCount())+
                      " of "+std::to_string(coordCount)+" coords"+
                      "("+std::to_string(pageManager.GetProcessedPagesCount())+" pages)");
      }

      progress.Info("Found "+std::to_string(serialIdManager.Size())+" duplicate coordinates");

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
                                            SerialIdManager& serialIdManager) const
  {
    progress.SetAction("Storing coordinates");

    FileScanner              scanner;
    FileWriter               writer;

    try {
      OSMId                  maxId=std::numeric_limits<OSMId>::max();
      OSMId                  currentLowerLimit=std::numeric_limits<OSMId>::min()/coordSortPageSize;
      OSMId                  currentUpperLimit=maxId/coordSortPageSize;
      size_t                 pageDumps=0;
      PageId                 currentPageId=0;
      std::vector<PageEntry> page(coordDiskPageSize);

      std::unordered_map<OSMId,FileOffset> pageFileOffsetIndex;

      writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                  CoordDataFile::COORD_DAT));

      writer.WriteFileOffset(0);
      writer.Write(coordDiskPageSize);
      // We want a coord page to be page aligned on disk, too
      writer.FlushCurrentBlockWithZeros(coordSortPageSize*coordDiskSize);

      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   Preprocess::RAWCOORDS_DAT),
                   FileScanner::Sequential,
                   true);

      uint32_t coordCount=scanner.ReadUInt32();

      uint32_t loadedCoordCount=0;
      while (loadedCoordCount<coordCount) {
        std::map<Id,std::vector<RawCoord>> coordPages;
        uint32_t                           currentCoordCount=0;

        progress.Info("Search for coordinates with page id >= "+std::to_string(currentLowerLimit));

        scanner.GotoBegin();

        /* ignore */ scanner.ReadUInt32();

        RawCoord coord;

        for (uint32_t i=1; i<=coordCount; i++) {
          progress.SetProgress(i,coordCount);

          coord.Read(typeConfig,scanner);

          OSMId pageId=coord.GetOSMId()/coordSortPageSize;

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

        SortCoordPages(coordPages);

        progress.Info("Write coordinates");

        for (const auto& entry : coordPages) {
          for (const auto& osmCoord : entry.second) {
            uint8_t serial=serialIdManager.GetNextSerialForId(osmCoord.GetCoord().GetId());

            if (serial==255) {
              progress.Error("Coordinate "+std::to_string(osmCoord.GetOSMId())+" "+osmCoord.GetCoord().GetDisplayText()+" has more than 256 nodes");
              continue;
            }

            PageId relatedId=osmCoord.GetOSMId()+std::numeric_limits<OSMId>::min();
            PageId pageId=relatedId/coordDiskPageSize;

            if (currentPageId!=pageId) {
              FileOffset pageOffset=writer.GetPos();

              if (DumpCurrentPage(writer,
                                  page)) {
                pageFileOffsetIndex[currentPageId]=pageOffset;
                pageDumps++;
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
          pageDumps++;
        }

        progress.Info("Loaded "+std::to_string(currentCoordCount)+" coords (" +std::to_string(loadedCoordCount)+"/"+std::to_string(coordCount)+")");

        currentLowerLimit=currentUpperLimit+1;
        currentUpperLimit=maxId/coordSortPageSize;
      }

      FileOffset indexStartOffset=writer.GetPos();

      progress.SetAction("Writing "+std::to_string(pageFileOffsetIndex.size())+" pages to disk, using "+std::to_string(pageDumps)+" page dumps");

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
    SerialIdManager serialIdManager;

    if (!FindDuplicateCoordinates(*typeConfig,
                                  parameter,
                                  progress,
                                  serialIdManager)) {
      return false;
    }

    if (!StoreCoordinates(*typeConfig,
                          parameter,
                          progress,
                          serialIdManager)) {
      return false;
    }

    return true;
  }
}
