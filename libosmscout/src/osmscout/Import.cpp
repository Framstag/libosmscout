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

#include <osmscout/Preprocess.h>

#include <osmscout/GenNodeDat.h>
#include <osmscout/GenRelationDat.h>
#include <osmscout/GenWayDat.h>

#include <osmscout/NumericIndex.h>

#include <osmscout/GenAreaAreaIndex.h>
#include <osmscout/GenAreaAreaRelIndex.h>
#include <osmscout/GenAreaNodeIndex.h>
#include <osmscout/GenAreaWayIndex.h>
#include <osmscout/GenAreaWayRelIndex.h>
#include <osmscout/GenNodeUseIndex.h>
#include <osmscout/GenCityStreetIndex.h>

namespace osmscout {

  static const size_t defaultStartStep=1;
  static const size_t defaultEndStep=16;

  ImportParameter::ImportParameter()
   : startStep(defaultStartStep),
     endStep(defaultEndStep),
     nodeIndexIntervalSize(50),
     numericIndexLevelSize(1024),
     areaAreaIndexMaxMag(18),
     areaAreaRelIndexMaxMag(18),
     areaWayIndexMaxMag(18),
     areaWayRelIndexMaxMag(18)
  {
    // no code
  }

  std::string ImportParameter::GetMapfile() const
  {
    return mapfile;
  }

  size_t ImportParameter::GetStartStep() const
  {
    return startStep;
  }

  size_t ImportParameter::GetEndStep() const
  {
    return endStep;
  }

  size_t ImportParameter::GetNodeIndexIntervalSize() const
  {
    return nodeIndexIntervalSize;
  }

  size_t ImportParameter::GetNumericIndexLevelSize() const
  {
    return numericIndexLevelSize;
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

  void ImportParameter::SetMapfile(const std::string& mapfile)
  {
    this->mapfile=mapfile;
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

  void ImportParameter::SetNodeIndexIntervalSize(size_t nodeIndexIntervalSize)
  {
    this->nodeIndexIntervalSize=nodeIndexIntervalSize;
  }

  bool ExecuteModules(std::list<ImportModule*>& modules,
                      const ImportParameter& parameter,
                      Progress& progress,
                      const TypeConfig& typeConfig)
  {
    size_t currentStep=1;

    for (std::list<ImportModule*>::const_iterator module=modules.begin();
         module!=modules.end();
         ++module) {
      if (currentStep>=parameter.GetStartStep() &&
          currentStep<=parameter.GetEndStep()) {
        progress.SetStep(NumberToString(currentStep)+" "+(*module)->GetDescription());
        if (!(*module)->Import(parameter,progress,typeConfig)) {
          progress.Error(std::string("Error while executing step ")+(*module)->GetDescription()+"!");
          return false;
        }
      }

      currentStep++;
    }

    return true;
  }

  bool Import(const ImportParameter& parameter,
              Progress& progress)
  {
    // TODO: verify parameter

    TypeConfig               typeConfig;
    std::list<ImportModule*> modules;

    progress.SetStep("Loading type config");

    if (!LoadTypeConfig("map.ost.xml",typeConfig)) {
      progress.Error("Cannot load type configuration!");
      return false;
    }

    modules.push_back(new Preprocess());
    modules.push_back(new NumericIndexGenerator<Id,RawNode>("Generating 'rawnode.idx'",
                                                            "rawnodes.dat",
                                                            "rawnode.idx"));
    modules.push_back(new NumericIndexGenerator<Id,RawWay>("Generating 'rawway.idx'",
                                                           "rawways.dat",
                                                           "rawway.idx"));
    modules.push_back(new RelationDataGenerator());
    modules.push_back(new NumericIndexGenerator<Id,Relation>("Generating 'relation.idx'",
                                                             "relations.dat",
                                                             "relation.idx"));
    modules.push_back(new NodeDataGenerator());
    modules.push_back(new NumericIndexGenerator<Id,Node>("Generating 'node.idx'",
                                                         "nodes.dat",
                                                         "node.idx"));
    modules.push_back(new WayDataGenerator());
    modules.push_back(new NumericIndexGenerator<Id,Way>("Generating 'way.idx'",
                                                        "ways.dat",
                                                        "way.idx"));
    modules.push_back(new AreaNodeIndexGenerator());
    modules.push_back(new AreaAreaIndexGenerator());
    modules.push_back(new AreaWayIndexGenerator());
    modules.push_back(new AreaAreaRelIndexGenerator());
    modules.push_back(new AreaWayRelIndexGenerator());
    modules.push_back(new CityStreetIndexGenerator());
    modules.push_back(new NodeUseIndexGenerator());

    bool result=ExecuteModules(modules,parameter,progress,typeConfig);

    for (std::list<ImportModule*>::iterator module=modules.begin();
         module!=modules.end();
         ++module) {
      delete *module;
    }

    return result;
  }
}

