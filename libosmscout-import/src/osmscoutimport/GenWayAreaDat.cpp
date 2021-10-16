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

#include <osmscoutimport/GenWayAreaDat.h>

#include <algorithm>

#include <osmscout/util/Geometry.h>
#include <osmscout/util/StopClock.h>

#include <osmscoutimport/RawWay.h>

#include <osmscoutimport/GenRelAreaDat.h>
#include <osmscoutimport/Preprocess.h>

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

  std::tuple<WayAreaDataGenerator::BlacklistSet,bool>
  WayAreaDataGenerator::ReadWayBlacklist(const ImportParameter& parameter,
                                         Progress& progress) const
  {
    BlacklistSet wayBlacklist;
    FileScanner scanner;

    try {
      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   RelAreaDataGenerator::WAYAREABLACK_DAT),
                   FileScanner::Sequential,
                   true);

      while (!scanner.IsEOF()) {
        OSMId id=scanner.ReadInt64Number();

        wayBlacklist.insert(id);
      }

      scanner.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());

      wayBlacklist.clear();
      return {wayBlacklist,false};
    }

    return {wayBlacklist,true};
  }

  void WayAreaDataGenerator::WriteArea(const ImportParameter& parameter,
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

    size_t index=0;
    for (const auto& nodeId  : rawWay.GetNodes()) {
      auto coord=coordsMap.find(nodeId);

      if (coord==coordsMap.end()) {
        progress.Error("Cannot resolve node with id "+
                       std::to_string(nodeId)+
                       " for area "+
                       std::to_string(wayId));
        return;
      }

      ring.nodes[index].Set(coord->second.GetSerial(),
                            coord->second.GetCoord());
      index++;
    }

    if (!IsValidToWrite(ring.nodes)) {
      progress.Error("Area coordinates are not dense enough to be written for area "+
                     std::to_string(wayId));
      return;
    }

    if (parameter.GetStrictAreas() &&
        !AreaIsSimple(ring.nodes)) {
      progress.Error("Area "+std::to_string(wayId)+" of type '"+ring.GetType()->GetName()+"' is not simple");
      return;
    }

    area.rings.push_back(ring);

    writer.Write((uint8_t)osmRefWay);
    writer.Write(wayId);
    area.WriteImport(typeConfig,
                     writer);

    writtenWayCount++;
  }

  bool WayAreaDataGenerator::Import(const TypeConfigRef& typeConfig,
                                    const ImportParameter& parameter,
                                    Progress& progress)
  {
    progress.SetAction("Generate wayarea.tmp");

    CoordDataFile             coordDataFile;

    FileScanner               scanner;
    std::vector<RawWayRef>    rawWays;
    std::set<OSMId>           nodeIds;

    FileWriter                areaWriter;

    rawWays.reserve(parameter.GetRawWayBlockSize());

    //
    // load blacklist of wayId as a result from multipolygon relation parsing
    //

    progress.SetAction("Reading way area blacklist");

    auto [wayBlacklist, success]=ReadWayBlacklist(parameter,
                                                  progress);
    if (!success) {
      return false;
    }

    if (!coordDataFile.Open(parameter.GetDestinationDirectory(),
                            parameter.GetCoordDataMemoryMaped())) {
      log.Error() << "Cannot open coord data file!";
      return false;
    }

    progress.SetAction("Processing raw ways as areas");

    try {
      uint32_t writtenWayCount=0;

      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   Preprocess::RAWWAYS_DAT),
                   FileScanner::Sequential,
                   parameter.GetRawWayDataMemoryMaped());

      uint32_t rawWayCount=scanner.ReadUInt32();

      areaWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      WAYAREA_TMP));

      areaWriter.Write(writtenWayCount);

      for (uint32_t w=1; w<=rawWayCount; w++) {
        RawWayRef way=std::make_shared<RawWay>();

        progress.SetProgress(w,
                             rawWayCount,
                             Preprocess::RAWWAYS_DAT);

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

        nodeIds.insert(way->GetNodes().begin(),way->GetNodes().end());

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

      /* -------*/

      scanner.Close();

      areaWriter.SetPos(0);
      areaWriter.Write(writtenWayCount);
      areaWriter.Close();

      progress.Info(std::to_string(rawWayCount) + " raw way(s) read, "+
                    std::to_string(writtenWayCount) + " areas(s) written");
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());

      scanner.CloseFailsafe();
      areaWriter.CloseFailsafe();

      return false;
    }

    // Cleaning up...

    wayBlacklist.clear();

    return coordDataFile.Close();
  }
}

