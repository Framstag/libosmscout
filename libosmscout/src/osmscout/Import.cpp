/*
  This source is part of the libosmscout library
  Copyright (C) 2009  Tim Teulings

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

#include <osmscout/Import.h>

#include <iostream>

#include <osmscout/Progress.h>

#include <osmscout/TypeConfigLoader.h>

#include <osmscout/RawNode.h>
#include <osmscout/RawWay.h>

#include <osmscout/Node.h>
#include <osmscout/Relation.h>
#include <osmscout/Way.h>

#include <osmscout/GenTypeDat.h>

#include <osmscout/Preprocess.h>

#include <osmscout/GenNodeDat.h>
#include <osmscout/GenRelationDat.h>
#include <osmscout/GenWayDat.h>

#include <osmscout/GenNumericIndex.h>

#include <osmscout/GenAreaIndex.h>
#include <osmscout/GenAreaNodeIndex.h>
#include <osmscout/GenNodeUseIndex.h>
#include <osmscout/GenCityStreetIndex.h>
#include <osmscout/GenWaterIndex.h>

#include <osmscout/util/StopClock.h>

namespace osmscout {

  static const size_t defaultStartStep=1;
  static const size_t defaultEndStep=16;

  ImportParameter::ImportParameter()
   : typefile("map.ost"),
     startStep(defaultStartStep),
     endStep(defaultEndStep),
     numericIndexPageSize(4096),
     nodesLoadSize(10000000),
     nodeIndexIntervalSize(50),
     nodeDataCacheSize(100),
     nodeIndexCacheSize(30000),
     waysLoadSize(1000000),
     wayDataCacheSize(100),
     wayIndexCacheSize(5000),
     areaAreaIndexMaxMag(18),
     areaAreaRelIndexMaxMag(18),
     areaWayIndexMaxMag(18),
     areaWayRelIndexMaxMag(18),
     waterIndexMaxMag(14)
  {
    // no code
  }

  std::string ImportParameter::GetMapfile() const
  {
    return mapfile;
  }

  std::string ImportParameter::GetTypefile() const
  {
    return typefile;
  }

  std::string ImportParameter::GetDestinationDirectory() const
  {
    return destinationDirectory;
  }

  size_t ImportParameter::GetStartStep() const
  {
    return startStep;
  }

  size_t ImportParameter::GetEndStep() const
  {
    return endStep;
  }

  size_t ImportParameter::GetNumericIndexPageSize() const
  {
    return numericIndexPageSize;
  }

  size_t ImportParameter::GetNodesLoadSize() const
  {
    return nodesLoadSize;
  }

  size_t ImportParameter::GetNodeIndexIntervalSize() const
  {
    return nodeIndexIntervalSize;
  }

  size_t ImportParameter::GetNodeDataCacheSize() const
  {
    return nodeDataCacheSize;
  }

  size_t ImportParameter::GetNodeIndexCacheSize() const
  {
    return nodeIndexCacheSize;
  }

  size_t ImportParameter::GetWaysLoadSize() const
  {
    return waysLoadSize;
  }

  size_t ImportParameter::GetWayDataCacheSize() const
  {
    return wayDataCacheSize;
  }

  size_t ImportParameter::GetWayIndexCacheSize() const
  {
    return wayIndexCacheSize;
  }

  size_t ImportParameter::GetAreaAreaIndexMaxMag() const
  {
    return areaAreaIndexMaxMag;
  }

  size_t ImportParameter::GetAreaAreaRelIndexMaxMag() const
  {
    return areaAreaRelIndexMaxMag;
  }

  size_t ImportParameter::GetAreaWayIndexMaxMag() const
  {
    return areaWayIndexMaxMag;
  }

  size_t ImportParameter::GetAreaWayRelIndexMaxMag() const
  {
    return areaWayRelIndexMaxMag;
  }

  size_t ImportParameter::GetWaterIndexMaxMag() const
  {
    return waterIndexMaxMag;
  }

  void ImportParameter::SetMapfile(const std::string& mapfile)
  {
    this->mapfile=mapfile;
  }

  void ImportParameter::SetTypefile(const std::string& typefile)
  {
    this->typefile=typefile;
  }

  void ImportParameter::SetDestinationDirectory(const std::string& destinationDirectory)
  {
    this->destinationDirectory=destinationDirectory;
  }

  void ImportParameter::SetStartStep(size_t startStep)
  {
    this->startStep=startStep;
    this->endStep=defaultEndStep;
  }

  void ImportParameter::SetSteps(size_t startStep, size_t endStep)
  {
    this->startStep=startStep;
    this->endStep=endStep;
  }

  void ImportParameter::SetNumericIndexPageSize(size_t numericIndexPageSize)
  {
    this->numericIndexPageSize=numericIndexPageSize;
  }

  void ImportParameter::SetNodesLoadSize(size_t nodesLoadSize)
  {
    this->nodesLoadSize=nodesLoadSize;
  }

  void ImportParameter::SetNodeIndexIntervalSize(size_t nodeIndexIntervalSize)
  {
    this->nodeIndexIntervalSize=nodeIndexIntervalSize;
  }

  void ImportParameter::SetNodeDataCacheSize(size_t nodeDataCacheSize)
  {
    this->nodeDataCacheSize=nodeDataCacheSize;
  }

  void ImportParameter::SetNodeIndexCacheSize(size_t nodeIndexCacheSize)
  {
    this->nodeIndexCacheSize=nodeIndexCacheSize;
  }

  void ImportParameter::SetWaysLoadSize(size_t waysLoadSize)
  {
    this->waysLoadSize=waysLoadSize;
  }

  void ImportParameter::SetWayDataCacheSize(size_t wayDataCacheSize)
  {
    this->wayDataCacheSize=wayDataCacheSize;
  }

  void ImportParameter::SetWayIndexCacheSize(size_t wayIndexCacheSize)
  {
    this->wayIndexCacheSize=wayIndexCacheSize;
  }

  bool ExecuteModules(std::list<ImportModule*>& modules,
                      const ImportParameter& parameter,
                      Progress& progress,
                      const TypeConfig& typeConfig)
  {
    StopClock overAllTimer;
    size_t    currentStep=1;

    for (std::list<ImportModule*>::const_iterator module=modules.begin();
         module!=modules.end();
         ++module) {
      if (currentStep>=parameter.GetStartStep() &&
          currentStep<=parameter.GetEndStep()) {
        StopClock timer;
        bool      success;

        progress.SetStep(std::string("Step #")+
                         NumberToString(currentStep)+
                         " - "+
                         (*module)->GetDescription());

        success=(*module)->Import(parameter,progress,typeConfig);

        timer.Stop();

        progress.Info(std::string("=> ")+timer.ResultString()+" second(s)");

        if (!success) {
          progress.Error(std::string("Error while executing step ")+(*module)->GetDescription()+"!");
          return false;
        }
      }

      currentStep++;
    }

    overAllTimer.Stop();
    progress.Info(std::string("=> ")+overAllTimer.ResultString()+" second(s)");

    return true;
  }

  bool Import(const ImportParameter& parameter,
              Progress& progress)
  {
    // TODO: verify parameter

    TypeConfig               typeConfig;
    std::list<ImportModule*> modules;

    progress.SetStep("Loading type config");

    if (!LoadTypeConfig(parameter.GetTypefile().c_str(),typeConfig)) {
      progress.Error("Cannot load type configuration!");
      return false;
    }

    /* 1 */
    modules.push_back(new TypeDataGenerator());

    /* 2 */
    modules.push_back(new Preprocess());

    /* 3 */
    modules.push_back(new NumericIndexGenerator<Id,RawNode>("Generating 'rawnode.idx'",
                                                            AppendFileToDir(parameter.GetDestinationDirectory(),
                                                                            "rawnodes.dat"),
                                                            AppendFileToDir(parameter.GetDestinationDirectory(),
                                                                            "rawnode.idx")));
    /* 4 */
    modules.push_back(new NumericIndexGenerator<Id,RawWay>("Generating 'rawway.idx'",
                                                           AppendFileToDir(parameter.GetDestinationDirectory(),
                                                                           "rawways.dat"),
                                                           AppendFileToDir(parameter.GetDestinationDirectory(),
                                                                           "rawway.idx")));
    /* 5 */
    modules.push_back(new RelationDataGenerator());
    /* 6 */
    modules.push_back(new NumericIndexGenerator<Id,Relation>("Generating 'relation.idx'",
                                                             AppendFileToDir(parameter.GetDestinationDirectory(),
                                                                             "relations.dat"),
                                                             AppendFileToDir(parameter.GetDestinationDirectory(),
                                                                             "relation.idx")));
    /* 7 */
    modules.push_back(new NodeDataGenerator());
    /* 8 */
    modules.push_back(new NumericIndexGenerator<Id,Node>("Generating 'node.idx'",
                                                         AppendFileToDir(parameter.GetDestinationDirectory(),
                                                                         "nodes.dat"),
                                                         AppendFileToDir(parameter.GetDestinationDirectory(),
                                                                         "node.idx")));
    /* 9 */
    modules.push_back(new WayDataGenerator());
    /* 10 */
    modules.push_back(new NumericIndexGenerator<Id,Way>("Generating 'way.idx'",
                                                        AppendFileToDir(parameter.GetDestinationDirectory(),
                                                                        "ways.dat"),
                                                        AppendFileToDir(parameter.GetDestinationDirectory(),
                                                                        "way.idx")));
    /* 11 */
    modules.push_back(new AreaIndexGenerator());
    /* 12 */
    modules.push_back(new AreaNodeIndexGenerator());
    /* 13 */
    modules.push_back(new CityStreetIndexGenerator());
    /* 14 */
    modules.push_back(new NodeUseIndexGenerator());
    /* 15 */
    modules.push_back(new WaterIndexGenerator());

    bool result=ExecuteModules(modules,parameter,progress,typeConfig);

    for (std::list<ImportModule*>::iterator module=modules.begin();
         module!=modules.end();
         ++module) {
      delete *module;
    }

    return result;
  }
}

