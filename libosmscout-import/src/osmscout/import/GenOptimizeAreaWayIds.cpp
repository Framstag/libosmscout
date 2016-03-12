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

#include <osmscout/import/GenOptimizeAreaWayIds.h>

#include <unordered_set>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>

#include <osmscout/DataFile.h>
#include <osmscout/import/GenMergeAreas.h>
#include <osmscout/import/GenWayWayDat.h>

namespace osmscout {

  const char* OptimizeAreaWayIdsGenerator::AREAS3_TMP = "areas3.tmp";
  const char* OptimizeAreaWayIdsGenerator::WAYS_TMP = "ways.tmp";

  void OptimizeAreaWayIdsGenerator::GetDescription(const ImportParameter& /*parameter*/,
                                                   ImportModuleDescription& description) const
  {
    description.SetName("OptimizeAreaWayIdsGenerator");
    description.SetDescription("Optimize ids for areas and ways");

    description.AddRequiredFile(MergeAreasGenerator::AREAS2_TMP);
    description.AddRequiredFile(WayWayDataGenerator::WAYWAY_TMP);

    description.AddProvidedTemporaryFile(AREAS3_TMP);
    description.AddProvidedTemporaryFile(WAYS_TMP);
  }

  bool OptimizeAreaWayIdsGenerator::ScanAreaIds(const ImportParameter& parameter,
                                                Progress& progress,
                                                const TypeConfig& typeConfig,
                                                NodeUseMap& nodeUseMap)
  {
    FileScanner scanner;
    uint32_t    dataCount=0;
    uint32_t    idCount=0;

    progress.SetAction("Scanning ids from 'areas2.tmp'");

    try {
      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   MergeAreasGenerator::AREAS2_TMP),
                   FileScanner::Sequential,
                   parameter.GetAreaDataMemoryMaped());

      scanner.Read(dataCount);

      for (uint32_t current=1; current<=dataCount; current++) {
        uint8_t type;
        OSMId   osmId;
        Area    data;

        progress.SetProgress(current,dataCount);

        scanner.Read(type);
        scanner.Read(osmId);

        data.ReadImport(typeConfig,
                        scanner);

        for (const auto& ring: data.rings) {
          if (!ring.GetType()->CanRoute()) {
            continue;
          }

          std::unordered_set<Id> nodeIds;
          for (const auto node : ring.nodes) {
            Id id=node.GetId();

            if (nodeIds.find(id)==nodeIds.end()) {
              nodeUseMap.SetNodeUsed(id);
              nodeIds.insert(id);
              idCount++;
            }
          }
        }
      }

      progress.Info(NumberToString(dataCount)+" areas, "+NumberToString(idCount)+" ids found");

      scanner.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }

  bool OptimizeAreaWayIdsGenerator::ScanWayIds(const ImportParameter& parameter,
                                               Progress& progress,
                                               const TypeConfig& typeConfig,
                                               NodeUseMap& nodeUseMap)
  {
    FileScanner scanner;
    uint32_t    dataCount=0;
    uint32_t    idCount=0;
    uint32_t    circularWayCount=0;

    progress.SetAction("Scanning ids from 'wayway.tmp'");

    try {
      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   WayWayDataGenerator::WAYWAY_TMP),
                   FileScanner::Sequential,
                   parameter.GetWayDataMemoryMaped());

      scanner.Read(dataCount);

      for (uint32_t current=1; current<=dataCount; current++) {
        uint8_t type;
        OSMId   osmId;
        Way     data;

        progress.SetProgress(current,dataCount);

        scanner.Read(type);
        scanner.Read(osmId);

        data.Read(typeConfig,
                  scanner);

        if (!data.GetType()->CanRoute()) {
          continue;
        }

        std::unordered_set<Id> nodeIds;

        for (const auto& node : data.nodes) {
          Id id=node.GetId();

          if (nodeIds.find(id)==nodeIds.end()) {
            nodeUseMap.SetNodeUsed(id);
            nodeIds.insert(id);
            idCount++;
          }
        }

        // If we have a circular way, we "fake" a double usage,
        // to make sure, that the node id of the first node
        // is not dropped later on, and we cannot detect
        // circular ways anymore
        if (data.IsCircular()) {
          nodeUseMap.SetNodeUsed(data.GetBackId());
          circularWayCount++;
        }
      }

      progress.Info(NumberToString(dataCount)+" ways, "+NumberToString(idCount)+" ids, "+NumberToString(circularWayCount)+" circular ways found");

      scanner.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }

  bool OptimizeAreaWayIdsGenerator::CopyAreas(const ImportParameter& parameter,
                                              Progress& progress,
                                              const TypeConfig& typeConfig,
                                              NodeUseMap& nodeUseMap)
  {
    FileScanner scanner;
    FileWriter  writer;
    uint32_t    areaCount=0;
    uint32_t    idClearedCount=0;

    try {
      writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                  AREAS3_TMP));

      writer.Write(areaCount);

      progress.SetAction("Copy data from 'areas2.tmp' to 'areas3.tmp'");

      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   MergeAreasGenerator::AREAS2_TMP),
                   FileScanner::Sequential,
                   parameter.GetAreaDataMemoryMaped());

      scanner.Read(areaCount);

      for (uint32_t current=1; current<=areaCount; current++) {
        uint8_t type;
        OSMId   osmId;
        Area    data;

        progress.SetProgress(current,areaCount);

        scanner.Read(type);
        scanner.Read(osmId);

        data.ReadImport(typeConfig,
                        scanner);

        for (auto& ring : data.rings) {
          for (auto& node : ring.nodes) {
            if (!nodeUseMap.IsNodeUsedAtLeastTwice(node.GetId())) {
              node.ClearSerial();
              idClearedCount++;
            }
          }
        }

        writer.Write(type);
        writer.Write(osmId);

        data.Write(typeConfig,
                   writer);
      }

      scanner.Close();

      writer.GotoBegin();
      writer.Write(areaCount);

      writer.Close();

      progress.Info(NumberToString(idClearedCount)+" node serials cleared");
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());

      scanner.CloseFailsafe();
      writer.CloseFailsafe();

      return false;
    }

    return true;
  }

  bool OptimizeAreaWayIdsGenerator::CopyWays(const ImportParameter& parameter,
                                             Progress& progress,
                                             const TypeConfig& typeConfig,
                                             NodeUseMap& nodeUseMap)
  {
    FileScanner scanner;
    FileWriter  writer;
    uint32_t    dataCount=0;
    uint32_t    idClearedCount=0;

    progress.SetAction("Copy data from 'wayway.tmp' to 'ways.tmp'");

    try {
      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   WayWayDataGenerator::WAYWAY_TMP),
                   FileScanner::Sequential,
                   parameter.GetWayDataMemoryMaped());

      scanner.Read(dataCount);

      writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                  WAYS_TMP));

      writer.Write(dataCount);

      for (uint32_t current=1; current<=dataCount; current++) {
        uint8_t type;
        OSMId   osmId;
        Way     data;

        progress.SetProgress(current,dataCount);

        scanner.Read(type);
        scanner.Read(osmId);

        data.Read(typeConfig,
                  scanner);

        for (auto& node : data.nodes) {
          if (!nodeUseMap.IsNodeUsedAtLeastTwice(node.GetId())) {
            node.ClearSerial();
            idClearedCount++;
          }
        }

        writer.Write(type);
        writer.Write(osmId);

        data.Write(typeConfig,
                   writer);
      }

      scanner.Close();
      writer.Close();

      progress.Info(NumberToString(idClearedCount)+" node serial cleared");
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());

      scanner.CloseFailsafe();
      writer.CloseFailsafe();

      return false;
    }

    return true;
  }

  bool OptimizeAreaWayIdsGenerator::Import(const TypeConfigRef& typeConfig,
                                           const ImportParameter& parameter,
                                           Progress& progress)
  {
    progress.SetAction("Optimize ids for areas and ways");

    NodeUseMap nodeUseMap;

    if (!ScanAreaIds(parameter,
                        progress,
                        *typeConfig,
                        nodeUseMap)) {
      return false;
    }

    if (!ScanWayIds(parameter,
                       progress,
                       *typeConfig,
                       nodeUseMap)) {
      return false;
    }

    progress.Info("Found "+NumberToString(nodeUseMap.GetNodeUsedCount())+" relevant nodes, "+NumberToString(nodeUseMap.GetDuplicateCount())+" of it at least used twice");

    if (!CopyAreas(parameter,
                   progress,
                   *typeConfig,
                   nodeUseMap)) {
      return false;
    }

    if (!CopyWays(parameter,
                    progress,
                    *typeConfig,
                    nodeUseMap)) {
      return false;
    }

    return true;
  }
}

