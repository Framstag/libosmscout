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

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>

#include <osmscout/DataFile.h>

namespace osmscout {

  std::string OptimizeAreaWayIdsGenerator::GetDescription() const
  {
    return "Optimize ids for areas and ways";
  }

  bool OptimizeAreaWayIdsGenerator::ScanWayAreaIds(const ImportParameter& parameter,
                                                   Progress& progress,
                                                   const TypeConfig& typeConfig,
                                                   NodeUseMap& nodeUseMap)
  {
    FileScanner scanner;
    uint32_t    dataCount=0;

    progress.SetAction("Scanning ids from 'wayarea.tmp'");

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "wayarea.tmp"),
                                      FileScanner::Sequential,
                                      parameter.GetAreaDataMemoryMaped())) {
      progress.Error(std::string("Cannot open '")+scanner.GetFilename()+"'");
      return false;
    }

    if (!scanner.Read(dataCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    for (size_t current=1; current<=dataCount; current++) {
      Id   id;
      Area data;

      progress.SetProgress(current,dataCount);

      if (!scanner.Read(id)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(current)+" of "+
                       NumberToString(dataCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");

        return false;
      }

      if (!data.Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(current)+" of "+
                       NumberToString(dataCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");

        return false;
      }


      for (std::vector<Area::Ring>::const_iterator ring=data.rings.begin();
           ring!=data.rings.end();
           ring++) {
        std::set<Id> nodeIds;

        if (!typeConfig.GetTypeInfo(ring->GetType()).CanRoute()) {
          continue;
        }

        for (std::vector<Id>::const_iterator id=ring->ids.begin();
             id!=ring->ids.end();
             id++) {
          if (nodeIds.find(*id)==nodeIds.end()) {
            nodeUseMap.SetNodeUsed(*id);

            nodeIds.insert(*id);
          }
        }
      }
    }

    if (!scanner.Close()) {
      progress.Error(std::string("Error while closing file '")+
                     scanner.GetFilename()+"'");
      return false;
    }

    return true;
  }

  bool OptimizeAreaWayIdsGenerator::ScanRelAreaIds(const ImportParameter& parameter,
                                                   Progress& progress,
                                                   const TypeConfig& typeConfig,
                                                   NodeUseMap& nodeUseMap)
  {
    FileScanner scanner;
    uint32_t    dataCount=0;

    progress.SetAction("Scanning ids from 'relarea.tmp'");

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "relarea.tmp"),
                                      FileScanner::Sequential,
                                      parameter.GetAreaDataMemoryMaped())) {
      progress.Error(std::string("Cannot open '")+scanner.GetFilename()+"'");
      return false;
    }

    if (!scanner.Read(dataCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    for (size_t current=1; current<=dataCount; current++) {
      Id   id;
      Area data;

      progress.SetProgress(current,dataCount);

      if (!scanner.Read(id)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(current)+" of "+
                       NumberToString(dataCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");

        return false;
      }

      if (!data.Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(current)+" of "+
                       NumberToString(dataCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");

        return false;
      }

      for (std::vector<Area::Ring>::const_iterator ring=data.rings.begin();
           ring!=data.rings.end();
           ring++) {
        std::set<Id> nodeIds;

        if (!typeConfig.GetTypeInfo(ring->GetType()).CanRoute()) {
          continue;
        }

        for (std::vector<Id>::const_iterator id=ring->ids.begin();
             id!=ring->ids.end();
             id++) {
          if (nodeIds.find(*id)==nodeIds.end()) {
            nodeUseMap.SetNodeUsed(*id);

            nodeIds.insert(*id);
          }
        }
      }
    }

    if (!scanner.Close()) {
      progress.Error(std::string("Error while closing file '")+
                     scanner.GetFilename()+"'");
      return false;
    }

    return true;
  }

  bool OptimizeAreaWayIdsGenerator::ScanWayWayIds(const ImportParameter& parameter,
                                                  Progress& progress,
                                                  const TypeConfig& typeConfig,
                                                  NodeUseMap& nodeUseMap)
  {
    FileScanner scanner;
    uint32_t    dataCount=0;

    progress.SetAction("Scanning ids from 'wayway.tmp'");

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "wayway.tmp"),
                                      FileScanner::Sequential,
                                      parameter.GetWayDataMemoryMaped())) {
      progress.Error(std::string("Cannot open '")+scanner.GetFilename()+"'");
      return false;
    }

    if (!scanner.Read(dataCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    for (size_t current=1; current<=dataCount; current++) {
      Id  id;
      Way data;

      progress.SetProgress(current,dataCount);

      if (!scanner.Read(id)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(current)+" of "+
                       NumberToString(dataCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");

        return false;
      }

      if (!data.Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(current)+" of "+
                       NumberToString(dataCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");

        return false;
      }

      if (!typeConfig.GetTypeInfo(data.GetType()).CanRoute()) {
        continue;
      }

      std::set<Id> nodeIds;

      for (std::vector<Id>::const_iterator id=data.ids.begin();
          id!=data.ids.end();
          id++) {
        if (nodeIds.find(*id)==nodeIds.end()) {
          nodeUseMap.SetNodeUsed(*id);

          nodeIds.insert(*id);
        }
      }
    }

    if (!scanner.Close()) {
      progress.Error(std::string("Error while closing file '")+
                     scanner.GetFilename()+"'");
      return false;
    }

    return true;
  }

  bool OptimizeAreaWayIdsGenerator::CopyWayArea(const ImportParameter& parameter,
                                                Progress& progress,
                                                NodeUseMap& nodeUseMap)
  {
    FileScanner scanner;
    FileWriter  writer;
    uint32_t    dataCount=0;

    progress.SetAction("Copy data from 'wayarea.tmp' to 'wayarea.dat'");

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "wayarea.tmp"),
                                      FileScanner::Sequential,
                                      parameter.GetAreaDataMemoryMaped())) {
      progress.Error(std::string("Cannot open '")+scanner.GetFilename()+"'");
      return false;
    }

    if (!scanner.Read(dataCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "wayarea.dat"))) {
      progress.Error(std::string("Cannot create '")+writer.GetFilename()+"'");
      return false;
    }

    writer.Write(dataCount);

    for (size_t current=1; current<=dataCount; current++) {
      Id   id;
      Area data;

      progress.SetProgress(current,dataCount);

      if (!scanner.Read(id)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(current)+" of "+
                       NumberToString(dataCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");

        return false;
      }

      if (!data.Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(current)+" of "+
                       NumberToString(dataCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");

        return false;
      }

      for (std::vector<Area::Ring>::iterator ring=data.rings.begin();
           ring!=data.rings.end();
           ring++) {
        std::set<Id> nodeIds;

        for (std::vector<Id>::iterator id=ring->ids.begin();
             id!=ring->ids.end();
             id++) {
          if (!nodeUseMap.IsNodeUsedAtLeastTwice(*id)) {
            *id=0;
          }
        }
      }

      if (!writer.Write(id)) {
        progress.Error(std::string("Error while writing data id to file '")+
                       writer.GetFilename()+"'");

        return false;
      }

      if (!data.Write(writer)) {
        progress.Error(std::string("Error while writing data entry to file '")+
                       writer.GetFilename()+"'");

        return false;
      }
    }

    if (!scanner.Close()) {
      progress.Error(std::string("Error while closing file '")+
                     scanner.GetFilename()+"'");
      return false;
    }

    if (!writer.Close()) {
      progress.Error(std::string("Error while closing file '")+
                     writer.GetFilename()+"'");
      return false;
    }

    return true;
  }

  bool OptimizeAreaWayIdsGenerator::CopyRelArea(const ImportParameter& parameter,
                                                Progress& progress,
                                                NodeUseMap& nodeUseMap)
  {
    FileScanner scanner;
    FileWriter  writer;
    uint32_t    dataCount=0;

    progress.SetAction("Copy data from 'relarea.tmp' to 'relarea.dat'");

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "relarea.tmp"),
                                      FileScanner::Sequential,
                                      parameter.GetAreaDataMemoryMaped())) {
      progress.Error(std::string("Cannot open '")+scanner.GetFilename()+"'");
      return false;
    }

    if (!scanner.Read(dataCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "relarea.dat"))) {
      progress.Error(std::string("Cannot create '")+writer.GetFilename()+"'");
      return false;
    }

    writer.Write(dataCount);

    for (size_t current=1; current<=dataCount; current++) {
      Id   id;
      Area data;

      progress.SetProgress(current,dataCount);

      if (!scanner.Read(id)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(current)+" of "+
                       NumberToString(dataCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");

        return false;
      }

      if (!data.Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(current)+" of "+
                       NumberToString(dataCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");

        return false;
      }

      for (std::vector<Area::Ring>::iterator ring=data.rings.begin();
           ring!=data.rings.end();
           ring++) {
        std::set<Id> nodeIds;

        for (std::vector<Id>::iterator id=ring->ids.begin();
             id!=ring->ids.end();
             id++) {
          if (!nodeUseMap.IsNodeUsedAtLeastTwice(*id)) {
            *id=0;
          }
        }
      }

      if (!writer.Write(id)) {
        progress.Error(std::string("Error while writing data id to file '")+
                       writer.GetFilename()+"'");

        return false;
      }

      if (!data.Write(writer)) {
        progress.Error(std::string("Error while writing data entry to file '")+
                       writer.GetFilename()+"'");

        return false;
      }
    }

    if (!scanner.Close()) {
      progress.Error(std::string("Error while closing file '")+
                     scanner.GetFilename()+"'");
      return false;
    }

    if (!writer.Close()) {
      progress.Error(std::string("Error while closing file '")+
                     writer.GetFilename()+"'");
      return false;
    }

    return true;
  }

  bool OptimizeAreaWayIdsGenerator::CopyWayWay(const ImportParameter& parameter,
                                               Progress& progress,
                                               NodeUseMap& nodeUseMap)
  {
    FileScanner scanner;
    FileWriter  writer;
    uint32_t    dataCount=0;

    progress.SetAction("Copy data from 'wayway.tmp' to 'wayway.dat'");

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "wayway.tmp"),
                                      FileScanner::Sequential,
                                      parameter.GetWayDataMemoryMaped())) {
      progress.Error(std::string("Cannot open '")+scanner.GetFilename()+"'");
      return false;
    }

    if (!scanner.Read(dataCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "wayway.dat"))) {
      progress.Error(std::string("Cannot create '")+writer.GetFilename()+"'");
      return false;
    }

    writer.Write(dataCount);

    for (size_t current=1; current<=dataCount; current++) {
      Id  id;
      Way data;

      progress.SetProgress(current,dataCount);

      if (!scanner.Read(id)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(current)+" of "+
                       NumberToString(dataCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");

        return false;
      }

      if (!data.Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(current)+" of "+
                       NumberToString(dataCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");

        return false;
      }

      for (std::vector<Id>::iterator id=data.ids.begin();
          id!=data.ids.end();
          id++) {
        if (!nodeUseMap.IsNodeUsedAtLeastTwice(*id)) {
          *id=0;
        }
      }

      if (!writer.Write(id)) {
        progress.Error(std::string("Error while writing data id to file '")+
                       writer.GetFilename()+"'");

        return false;
      }

      if (!data.Write(writer)) {
        progress.Error(std::string("Error while writing data entry to file '")+
                       writer.GetFilename()+"'");

        return false;
      }
    }

    if (!scanner.Close()) {
      progress.Error(std::string("Error while closing file '")+
                     scanner.GetFilename()+"'");
      return false;
    }

    if (!writer.Close()) {
      progress.Error(std::string("Error while closing file '")+
                     writer.GetFilename()+"'");
      return false;
    }

    return true;
  }

  bool OptimizeAreaWayIdsGenerator::Import(const ImportParameter& parameter,
                                           Progress& progress,
                                           const TypeConfig& typeConfig)
  {
    progress.SetAction("Optimize ids for areas and ways");

    NodeUseMap nodeUseMap;

    if (!ScanWayAreaIds(parameter,
                        progress,
                        typeConfig,
                        nodeUseMap)) {
      return false;
    }

    if (!ScanRelAreaIds(parameter,
                        progress,
                        typeConfig,
                        nodeUseMap)) {
      return false;
    }

    if (!ScanWayWayIds(parameter,
                       progress,
                       typeConfig,
                       nodeUseMap)) {
      return false;
    }

    if (!CopyWayArea(parameter,
                     progress,
                     nodeUseMap)) {
      return false;
    }

    if (!CopyRelArea(parameter,
                     progress,
                     nodeUseMap)) {
      return false;
    }

    if (!CopyWayWay(parameter,
                    progress,
                    nodeUseMap)) {
      return false;
    }

    return true;
  }
}

