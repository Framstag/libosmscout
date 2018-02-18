/*
  This source is part of the libosmscout library
  Copyright (C) 2013  Tim Teulings

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

#include <osmscout/import/GenWayAreaDat.h>

#include <algorithm>

#include <osmscout/system/Assert.h>

#include <osmscout/util/Geometry.h>
#include <osmscout/util/StopClock.h>

#include <osmscout/import/RawWay.h>

#include <osmscout/import/GenRelAreaDat.h>
#include <osmscout/import/Preprocess.h>

namespace osmscout {

  const char* WayAreaDataGenerator::WAYAREA_TMP="wayarea.tmp";

  WayAreaDataGenerator::Distribution::Distribution()
  : nodeCount(0),
    wayCount(0),
    areaCount(0)
  {
    // no code
  }

  void WayAreaDataGenerator::GetDescription(const ImportParameter& /*parameter*/,
                                            ImportModuleDescription& description) const
  {
    description.SetName("WayAreaDataGenerator");
    description.SetDescription("Resolves raw ways to areas");

    description.AddRequiredFile(CoordDataFile::COORD_DAT);
    description.AddRequiredFile(Preprocess::RAWWAYS_DAT);
    description.AddRequiredFile(RelAreaDataGenerator::WAYAREABLACK_DAT);

    description.AddProvidedTemporaryFile(WAYAREA_TMP);
  }

  bool WayAreaDataGenerator::ReadWayBlacklist(const ImportParameter& parameter,
                                              Progress& progress,
                                              BlacklistSet& wayBlacklist) const
  {
    FileScanner scanner;

    try {
      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   RelAreaDataGenerator::WAYAREABLACK_DAT),
                   FileScanner::Sequential,
                   true);

      while (!scanner.IsEOF()) {
        OSMId id;

        scanner.ReadNumber(id);

        if (scanner.HasError()) {
          return false;
        }

        wayBlacklist.insert(id);
      }

      scanner.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }

  bool WayAreaDataGenerator::GetAreas(const ImportParameter& parameter,
                                      Progress& progress,
                                      const TypeConfig& typeConfig,
                                      TypeInfoSet& types,
                                      const BlacklistSet& blacklist,
                                      FileScanner& scanner,
                                      std::vector<std::list<RawWayRef> >& areas)
  {
    uint32_t    wayCount=0;
    size_t      collectedWaysCount=0;
    size_t      typesWithWays=0;
    TypeInfoSet currentTypes(types);

    scanner.GotoBegin();

    scanner.Read(wayCount);

    for (uint32_t w=1; w<=wayCount; w++) {
      RawWayRef way=std::make_shared<RawWay>();

      progress.SetProgress(w,wayCount);

      way->Read(typeConfig,
                scanner);

      if (!way->IsArea()) {
        continue;
      }

      if (!currentTypes.IsSet(way->GetType())) {
        continue;
      }

      if (way->GetNodeCount()<3) {
        continue;
      }

      if (blacklist.find(way->GetId())!=blacklist.end()) {
        continue;
      }

      if (areas[way->GetType()->GetIndex()].empty()) {
        typesWithWays++;
      }

      areas[way->GetType()->GetIndex()].push_back(way);

      collectedWaysCount++;

      while (collectedWaysCount>parameter.GetRawWayBlockSize() &&
          typesWithWays>1) {
        TypeInfoRef victimType;

        // Find the type with the smallest amount of ways loaded
        for (auto &type : currentTypes) {
          if (!areas[type->GetIndex()].empty() &&
              (!victimType ||
               (areas[type->GetIndex()].size()<areas[victimType->GetIndex()].size()))) {
            victimType=type;
          }
        }

        if (victimType) {
          collectedWaysCount-=areas[victimType->GetIndex()].size();
          areas[victimType->GetIndex()].clear();

          typesWithWays--;
          currentTypes.Remove(victimType);
        }
      }

      if (typesWithWays==0) {
        break;
      }
    }

    types.Remove(currentTypes);

    progress.SetAction("Collected "+std::to_string(collectedWaysCount)+" ways for "+std::to_string(currentTypes.Size())+" types");

    return true;
  }

  bool WayAreaDataGenerator::WriteArea(const ImportParameter& parameter,
                                       Progress& progress,
                                       const TypeConfig& typeConfig,
                                       FileWriter& writer,
                                       uint32_t& writtenWayCount,
                                       const CoordDataFile::ResultMap& coordsMap,
                                       const RawWay& rawWay)
  {
    Area       area;
    OSMId      wayId=rawWay.GetId();
    Area::Ring ring;

    ring.SetFeatures(rawWay.GetFeatureValueBuffer());

    ring.MarkAsOuterRing();
    ring.nodes.resize(rawWay.GetNodeCount());

    bool success=true;
    for (size_t n=0; n<rawWay.GetNodeCount(); n++) {
      CoordDataFile::ResultMap::const_iterator coord=coordsMap.find(rawWay.GetNodeId(n));

      if (coord==coordsMap.end()) {
        progress.Error("Cannot resolve node with id "+
                       std::to_string(rawWay.GetNodeId(n))+
                       " for area "+
                       std::to_string(wayId));
        success=false;
        break;
      }

      ring.nodes[n].Set(coord->second.GetSerial(),
                        coord->second.GetCoord());
    }

    if (!success) {
      return true;
    }

    if (!IsValidToWrite(ring.nodes)) {
      progress.Error("Area coordinates are not dense enough to be written for area "+
                     std::to_string(wayId));
      return false;
    }

    if (parameter.GetStrictAreas() &&
        !AreaIsSimple(ring.nodes)) {
      progress.Error("Area "+std::to_string(wayId)+" of type '"+ring.GetType()->GetName()+"' is not simple");
      return true;
    }

    area.rings.push_back(ring);

    writer.Write((uint8_t)osmRefWay);
    writer.Write(wayId);
    area.WriteImport(typeConfig,
                     writer);

    writtenWayCount++;

    return true;
  }

  bool WayAreaDataGenerator::Import(const TypeConfigRef& typeConfig,
                                    const ImportParameter& parameter,
                                    Progress& progress)
  {
    progress.SetAction("Generate wayarea.tmp");

    BlacklistSet              wayBlacklist; //! Set of ways that should not be handled

    CoordDataFile             coordDataFile;

    FileScanner               scanner;
    uint32_t                  rawWayCount=0;
    std::vector<RawWayRef>    rawWays;
    std::set<OSMId>           nodeIds;

    FileWriter                areaWriter;
    uint32_t                  writtenWayCount=0;

    rawWays.reserve(parameter.GetRawWayBlockSize());

    //
    // load blacklist of wayId as a result from multipolygon relation parsing
    //

    progress.SetAction("Reading way area blacklist");

    if (!ReadWayBlacklist(parameter,
                          progress,
                          wayBlacklist)) {
      return false;
    }

    if (!coordDataFile.Open(parameter.GetDestinationDirectory(),
                            parameter.GetCoordDataMemoryMaped())) {
      log.Error() << "Cannot open coord data file!";
      return false;
    }

    progress.SetAction("Processing raw ways as areas");

    try {
      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   Preprocess::RAWWAYS_DAT),
                   FileScanner::Sequential,
                   parameter.GetRawWayDataMemoryMaped());

      scanner.Read(rawWayCount);

      areaWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      WAYAREA_TMP));

      areaWriter.Write(writtenWayCount);

      for (uint32_t w=1; w<=rawWayCount; w++) {
        RawWayRef way=std::make_shared<RawWay>();

        progress.SetProgress(w,
                             rawWayCount);

        way->Read(*typeConfig,
                  scanner);

        if (!way->IsArea()) {
          continue;
        }

        if (way->GetType()->GetIgnore()) {
          continue;
        }

        if (way->GetNodeCount()<3) {
          continue;
        }

        if (wayBlacklist.find(way->GetId())!=wayBlacklist.end()) {
          continue;
        }

        for (size_t n=0; n<way->GetNodeCount(); n++) {
          nodeIds.insert(way->GetNodeId(n));
        }

        rawWays.push_back(way);

        if (rawWays.size()>=parameter.GetRawWayBlockSize() ||
            nodeIds.size()>=parameter.GetCoordBlockSize()) {
          CoordDataFile::ResultMap coordsMap;

          if (!coordDataFile.Get(nodeIds,
                                 coordsMap)) {
            log.Error() << "Cannot read coordinates!";
            return false;
          }

          nodeIds.clear();

          for (const auto& rawWay : rawWays) {
            WriteArea(parameter,
                      progress,
                      *typeConfig,
                      areaWriter,
                      writtenWayCount,
                      coordsMap,
                      *rawWay);
          }

          rawWays.clear();
        }
      }

      if (!rawWays.empty()) {
        CoordDataFile::ResultMap coordsMap;

        if (!coordDataFile.Get(nodeIds,
                               coordsMap)) {
          log.Error() << "Cannot read nodes!";
          return false;
        }

        for (const auto& rawWay : rawWays) {
          WriteArea(parameter,
                    progress,
                    *typeConfig,
                    areaWriter,
                    writtenWayCount,
                    coordsMap,
                    *rawWay);
        }
      }

      progress.Info(std::to_string(rawWayCount) + " raw way(s) read, "+
                    std::to_string(writtenWayCount) + " areas(s) written");

      /* -------*/

      scanner.Close();

      areaWriter.SetPos(0);
      areaWriter.Write(writtenWayCount);
      areaWriter.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());

      scanner.CloseFailsafe();
      areaWriter.CloseFailsafe();

      return false;
    }

    // Cleaning up...

    wayBlacklist.clear();

    if (!coordDataFile.Close()) {
      return false;
    }

    progress.Info(std::to_string(rawWayCount) + " raw way(s) read, "+
                  std::to_string(writtenWayCount) + " areas(s) written");

    return true;
  }
}

