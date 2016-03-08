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

#include <limits>

#include <osmscout/Coord.h>
#include <osmscout/CoordDataFile.h>

#include <osmscout/import/Preprocess.h>
#include <osmscout/import/RawCoord.h>

#include <osmscout/util/String.h>

namespace osmscout {

  static inline bool SortCoordsByCoordId(const RawCoord& a, const RawCoord& b)
  {
    return a.GetCoord().ToNumber()<b.GetCoord().ToNumber();
  }

  static inline bool SortCoordsByOSMId(const RawCoord& a, const RawCoord& b)
  {
    return a.GetId()<b.GetId();
  }

  CoordDataGenerator::CoordDataGenerator()
  {
    // no code
  }

  bool CoordDataGenerator::FindDuplicateCoordinates(const TypeConfig& typeConfig,
                                                    const ImportParameter& parameter,
                                                    Progress& progress,
                                                    std::unordered_map<Id,uint8_t>& duplicates) const
  {
    progress.SetAction("Searching for duplicate coordinates");

    uint32_t    coordPageSize=100000;
    uint32_t    coordBlockSize=30000000;
    Id          maxId=GeoCoord(90.0,180.0).ToNumber();
    Id          currentLowerLimit=0;
    Id          currentUpperLimit=maxId/coordPageSize;
    FileScanner scanner;
    uint32_t    loadedCoordCount=0;

    try {
      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   Preprocess::RAWCOORDS_DAT),
                   FileScanner::Sequential,
                   true);

      scanner.GotoBegin();

      uint32_t coordCount;

      scanner.Read(coordCount);

      while (loadedCoordCount<coordCount) {
        std::map<Id,std::list<RawCoord>> coordPages;
        uint32_t                         currentCoordCount=0;

        progress.Info("Searching for coordinates with page id >= "+NumberToString(currentLowerLimit));

        scanner.GotoBegin();

        scanner.Read(coordCount);

        for (uint32_t i=1; i<=coordCount; i++) {
          progress.SetProgress(i,coordCount);

          RawCoord coord;

          coord.Read(typeConfig,scanner);

          Id id=coord.GetCoord().ToNumber();
          Id pageId=id/coordPageSize;

          if (pageId<currentLowerLimit || pageId>currentUpperLimit) {
            continue;
          }

          coordPages[pageId].push_back(coord);
          currentCoordCount++;
          loadedCoordCount++;

          if (loadedCoordCount==coordCount) {
            break;
          }

          if (currentCoordCount>coordBlockSize
              && coordPages.size()>1) {
            Id oldUpperLimit=currentUpperLimit;

            while (currentCoordCount>coordBlockSize
                   && coordPages.size()>1) {
              auto pageEntry=coordPages.rbegin();
              Id highestPageId=pageEntry->first;

              currentCoordCount-=pageEntry->second.size();
              loadedCoordCount-=pageEntry->second.size();
              coordPages.erase(highestPageId);
              currentUpperLimit=highestPageId-1;
            }

            assert(currentUpperLimit<oldUpperLimit);
          }
        }

        for (auto& entry : coordPages) {
          entry.second.sort(SortCoordsByCoordId);

          Id lastId=std::numeric_limits<Id>::max();

          bool flaged=false;
          for (auto& coord : entry.second) {
            Id id=coord.GetCoord().ToNumber();

            if (id==lastId) {
              if (!flaged) {
                duplicates[id]=0;
                flaged=true;
              }
            }
            else {
              flaged=false;
            }

            lastId=id;
          }
        }

        progress.Info("Loaded "+NumberToString(currentCoordCount)+" coords (" +NumberToString(loadedCoordCount)+"/"+NumberToString(coordCount)+")");

        currentLowerLimit=currentUpperLimit+1;
        currentUpperLimit=maxId/coordPageSize;
      }

      progress.Info("Found "+NumberToString(duplicates.size())+" duplicate cordinates");

      scanner.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      scanner.CloseFailsafe();

      return false;
    }

    return true;
  }


  bool CoordDataGenerator::StoreCoordinates(const TypeConfig& typeConfig,
                                            const ImportParameter& parameter,
                                            Progress& progress,
                                            std::unordered_map<Id,uint8_t>& duplicates) const
  {
    progress.SetAction("Storing coordinates");

    uint32_t    coordPageSize=100000;
    uint32_t    coordBlockSize=30000000;
    OSMId       maxId=std::numeric_limits<OSMId>::max();
    OSMId       currentLowerLimit=std::numeric_limits<OSMId>::min()/coordPageSize;
    OSMId       currentUpperLimit=maxId/coordPageSize;
    FileScanner scanner;
    FileWriter  writer;
    uint32_t    loadedCoordCount=0;

    try {
      writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                  CoordDataFile::COORD_DAT));

      writer.Write(loadedCoordCount);

      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   Preprocess::RAWCOORDS_DAT),
                   FileScanner::Sequential,
                   true);

      scanner.GotoBegin();

      uint32_t coordCount;

      scanner.Read(coordCount);

      while (loadedCoordCount<coordCount) {
        std::map<Id,std::list<RawCoord>> coordPages;
        uint32_t                         currentCoordCount=0;

        progress.Info("Search for coordinates with page id >= "+NumberToString(currentLowerLimit));

        scanner.GotoBegin();

        scanner.Read(coordCount);

        for (uint32_t i=1; i<=coordCount; i++) {
          progress.SetProgress(i,coordCount);

          RawCoord coord;

          coord.Read(typeConfig,scanner);

          OSMId id=coord.GetId();
          OSMId pageId=id/coordPageSize;

          if (pageId<currentLowerLimit || pageId>currentUpperLimit) {
            continue;
          }

          coordPages[pageId].push_back(coord);
          currentCoordCount++;
          loadedCoordCount++;

          if (loadedCoordCount==coordCount) {
            break;
          }

          if (currentCoordCount>coordBlockSize
              && coordPages.size()>1) {
            OSMId oldUpperLimit=currentUpperLimit;

            while (currentCoordCount>coordBlockSize
                   && coordPages.size()>1) {
              auto pageEntry=coordPages.rbegin();
              OSMId highestPageId=pageEntry->first;

              currentCoordCount-=pageEntry->second.size();
              loadedCoordCount-=pageEntry->second.size();
              coordPages.erase(highestPageId);
              currentUpperLimit=highestPageId-1;
            }

            assert(currentUpperLimit<oldUpperLimit);
          }
        }

        for (auto& entry : coordPages) {
          entry.second.sort(SortCoordsByOSMId);

          for (auto& osmCoord : entry.second) {
            Coord coord;

            coord.SetId(osmCoord.GetId());
            coord.SetCoord(osmCoord.GetCoord());

            coord.Write(typeConfig,writer);
          }
        }

        progress.Info("Loaded "+NumberToString(currentCoordCount)+" coords (" +NumberToString(loadedCoordCount)+"/"+NumberToString(coordCount)+")");

        currentLowerLimit=currentUpperLimit+1;
        currentUpperLimit=maxId/coordPageSize;
      }

      scanner.Close();

      writer.GotoBegin();
      writer.Write(loadedCoordCount);
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

                                   ;
