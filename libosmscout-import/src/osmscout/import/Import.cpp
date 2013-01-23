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

#include <osmscout/import/Import.h>

#include <iostream>

#include <osmscout/TypeConfigLoader.h>

#include <osmscout/Node.h>
#include <osmscout/Point.h>
#include <osmscout/Relation.h>
#include <osmscout/RouteNode.h>
#include <osmscout/Way.h>

#include <osmscout/import/RawNode.h>
#include <osmscout/import/RawWay.h>
#include <osmscout/import/RawRelation.h>

#include <osmscout/import/GenTypeDat.h>

#include <osmscout/import/Preprocess.h>

#include <osmscout/import/GenTurnRestrictionDat.h>

#include <osmscout/import/GenNodeDat.h>
#include <osmscout/import/GenRelationDat.h>
#include <osmscout/import/GenWayDat.h>
#include <osmscout/import/SortWayDat.h>

#include <osmscout/import/GenNumericIndex.h>

#include <osmscout/import/GenAreaAreaIndex.h>
#include <osmscout/import/GenAreaNodeIndex.h>
#include <osmscout/import/GenAreaWayIndex.h>

#include <osmscout/import/GenCityStreetIndex.h>
#include <osmscout/import/GenWaterIndex.h>
#include <osmscout/import/GenOptimizeLowZoom.h>

// Routing
#include <osmscout/import/GenRouteDat.h>

#include <osmscout/util/Progress.h>
#include <osmscout/util/StopClock.h>

namespace osmscout {

  static const size_t defaultStartStep=1;
  static const size_t defaultEndStep=21;

  ImportParameter::ImportParameter()
   : typefile("map.ost"),
     startStep(defaultStartStep),
     endStep(defaultEndStep),
     strictAreas(false),
     renumberIds(false),
     renumberBlockSize(40000000),
     renumberMag(15),
     numericIndexPageSize(4096),
     coordDataMemoryMaped(false),
     rawNodeIndexMemoryMaped(true),
     rawNodeDataMemoryMaped(false),
     rawNodeDataCacheSize(10000),
     rawNodeIndexCacheSize(25000),
     rawWayIndexMemoryMaped(true),
     rawWayDataMemoryMaped(false),
     rawWayDataCacheSize(5000),
     rawWayIndexCacheSize(10000),
     rawWayBlockSize(500000),
     wayIndexMemoryMaped(true),
     wayDataMemoryMaped(false),
     wayDataCacheSize(0),
     wayIndexCacheSize(10000),
     waysLoadSize(1000000),
     areaAreaIndexMaxMag(17),
     areaWayMinMag(14),
     areaWayIndexMinFillRate(0.1),
     areaWayIndexCellSizeAverage(16),
     areaWayIndexCellSizeMax(256),
     areaNodeMinMag(8),
     areaNodeIndexMinFillRate(0.1),
     areaNodeIndexCellSizeAverage(16),
     areaNodeIndexCellSizeMax(256),
     waterIndexMinMag(6),
     waterIndexMaxMag(14),
     optimizationMaxWayCount(1000000),
     optimizationMaxMag(10),
     optimizationMinMag(6),
     optimizationCellSizeAverage(16),
     optimizationCellSizeMax(256),
     optimizationWayMethod(TransPolygon::fast),
     routeNodeBlockSize(500000),
     assumeLand(true)
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

  bool ImportParameter::GetStrictAreas() const
  {
    return strictAreas;
  }

  bool ImportParameter::GetRenumberIds() const
  {
    return renumberIds;
  }

  size_t ImportParameter::GetRenumberBlockSize() const
  {
    return renumberBlockSize;
  }

  size_t ImportParameter::GetRenumberMag() const
  {
    return renumberMag;
  }

  size_t ImportParameter::GetNumericIndexPageSize() const
  {
    return numericIndexPageSize;
  }

  bool ImportParameter::GetCoordDataMemoryMaped() const
  {
    return coordDataMemoryMaped;
  }

  bool ImportParameter::GetRawNodeIndexMemoryMaped() const
  {
    return rawNodeIndexMemoryMaped;
  }

  bool ImportParameter::GetRawNodeDataMemoryMaped() const
  {
    return rawNodeDataMemoryMaped;
  }

  bool ImportParameter::GetRawWayIndexMemoryMaped() const
  {
    return rawWayIndexMemoryMaped;
  }

  size_t ImportParameter::GetRawWayDataCacheSize() const
  {
    return rawWayDataCacheSize;
  }

  size_t ImportParameter::GetRawWayIndexCacheSize() const
  {
    return rawWayIndexCacheSize;
  }

  bool ImportParameter::GetRawWayDataMemoryMaped() const
  {
    return rawWayDataMemoryMaped;
  }

  size_t ImportParameter::GetRawWayBlockSize() const
  {
    return rawWayBlockSize;
  }

  size_t ImportParameter::GetRawNodeDataCacheSize() const
  {
    return rawNodeDataCacheSize;
  }

  size_t ImportParameter::GetRawNodeIndexCacheSize() const
  {
    return rawNodeIndexCacheSize;
  }

  bool ImportParameter::GetWayIndexMemoryMaped() const
  {
    return wayIndexMemoryMaped;
  }

  size_t ImportParameter::GetWayDataCacheSize() const
  {
    return wayDataCacheSize;
  }

  size_t ImportParameter::GetWayIndexCacheSize() const
  {
    return wayIndexCacheSize;
  }

  bool ImportParameter::GetWayDataMemoryMaped() const
  {
    return wayDataMemoryMaped;
  }

  size_t ImportParameter::GetAreaNodeMinMag() const
  {
    return areaNodeMinMag;
  }

  double ImportParameter::GetAreaNodeIndexMinFillRate() const
  {
    return areaNodeIndexMinFillRate;
  }

  size_t ImportParameter::GetAreaNodeIndexCellSizeAverage() const
  {
    return areaNodeIndexCellSizeAverage;
  }

  size_t ImportParameter::GetAreaNodeIndexCellSizeMax() const
  {
    return areaNodeIndexCellSizeMax;
  }

  size_t ImportParameter::GetAreaWayMinMag() const
  {
    return areaWayMinMag;
  }

  double ImportParameter::GetAreaWayIndexMinFillRate() const
  {
    return areaWayIndexMinFillRate;
  }

  size_t ImportParameter::GetAreaWayIndexCellSizeAverage() const
  {
    return areaWayIndexCellSizeAverage;
  }

  size_t ImportParameter::GetAreaWayIndexCellSizeMax() const
  {
    return areaWayIndexCellSizeMax;
  }

  size_t ImportParameter::GetAreaAreaIndexMaxMag() const
  {
    return areaAreaIndexMaxMag;
  }

  size_t ImportParameter::GetWaterIndexMinMag() const
  {
    return waterIndexMinMag;
  }

  size_t ImportParameter::GetWaterIndexMaxMag() const
  {
    return waterIndexMaxMag;
  }

  size_t ImportParameter::GetOptimizationMaxWayCount() const
  {
    return optimizationMaxWayCount;
  }

  size_t ImportParameter::GetOptimizationMaxMag() const
  {
    return optimizationMaxMag;
  }

  size_t ImportParameter::GetOptimizationMinMag() const
  {
    return optimizationMinMag;
  }

  size_t ImportParameter::GetOptimizationCellSizeAverage() const
  {
    return optimizationCellSizeAverage;
  }

  size_t ImportParameter::GetOptimizationCellSizeMax() const
  {
    return optimizationCellSizeMax;
  }

  TransPolygon::OptimizeMethod ImportParameter::GetOptimizationWayMethod() const
  {
    return optimizationWayMethod;
  }

  size_t ImportParameter::GetRouteNodeBlockSize() const
  {
    return routeNodeBlockSize;
  }

  bool ImportParameter::GetAssumeLand() const
  {
    return assumeLand;
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

  void ImportParameter::SetStrictAreas(bool strictAreas)
  {
    this->strictAreas=strictAreas;
  }

  void ImportParameter::SetRenumberIds(bool renumberIds)
  {
    this->renumberIds=renumberIds;
  }

  void ImportParameter::SetRenumberBlockSize(size_t renumberBlockSize)
  {
    this->renumberBlockSize=renumberBlockSize;
  }

  void ImportParameter::SetRenumberMag(size_t renumberMag)
  {
    this->renumberMag=renumberMag;
  }

  void ImportParameter::SetNumericIndexPageSize(size_t numericIndexPageSize)
  {
    this->numericIndexPageSize=numericIndexPageSize;
  }

  void ImportParameter::SetRawNodeIndexMemoryMaped(bool memoryMaped)
  {
    this->rawNodeIndexMemoryMaped=memoryMaped;
  }

  void ImportParameter::SetCoordDataMemoryMaped(bool memoryMaped)
  {
    this->coordDataMemoryMaped=memoryMaped;
  }

  void ImportParameter::SetRawNodeDataMemoryMaped(bool memoryMaped)
  {
    this->rawNodeDataMemoryMaped=memoryMaped;
  }

  void ImportParameter::SetRawWayIndexMemoryMaped(bool memoryMaped)
  {
    this->rawWayIndexMemoryMaped=memoryMaped;
  }

  void ImportParameter::SetRawWayDataMemoryMaped(bool memoryMaped)
  {
    this->rawWayDataMemoryMaped=memoryMaped;
  }

  void ImportParameter::SetRawWayDataCacheSize(size_t wayDataCacheSize)
  {
    this->rawWayDataCacheSize=wayDataCacheSize;
  }

  void ImportParameter::SetRawWayIndexCacheSize(size_t wayIndexCacheSize)
  {
    this->rawWayIndexCacheSize=wayIndexCacheSize;
  }

  void ImportParameter::SetRawWayBlockSize(size_t blockSize)
  {
    this->rawWayBlockSize=blockSize;
  }

  void ImportParameter::SetRawNodeDataCacheSize(size_t nodeDataCacheSize)
  {
    this->rawNodeDataCacheSize=nodeDataCacheSize;
  }

  void ImportParameter::SetRawNodeIndexCacheSize(size_t nodeIndexCacheSize)
  {
    this->rawNodeIndexCacheSize=nodeIndexCacheSize;
  }

  void ImportParameter::SetWayIndexMemoryMaped(bool memoryMaped)
  {
    this->wayIndexMemoryMaped=memoryMaped;
  }

  void ImportParameter::SetWayDataMemoryMaped(bool memoryMaped)
  {
    this->wayDataMemoryMaped=memoryMaped;
  }

  void ImportParameter::SetWayDataCacheSize(size_t wayDataCacheSize)
  {
    this->wayDataCacheSize=wayDataCacheSize;
  }

  void ImportParameter::SetWayIndexCacheSize(size_t wayIndexCacheSize)
  {
    this->wayIndexCacheSize=wayIndexCacheSize;
  }

  void ImportParameter::SetWaysLoadSize(size_t waysLoadSize)
  {
    this->waysLoadSize=waysLoadSize;
  }

  void ImportParameter::SetAreaAreaIndexMaxMag(size_t areaAreaIndexMaxMag)
  {
    this->areaAreaIndexMaxMag=areaAreaIndexMaxMag;
  }

  void ImportParameter::SetAreaNodeMinMag(size_t areaNodeMinMag)
  {
    this->areaNodeMinMag=areaNodeMinMag;
  }

  void ImportParameter::SetAreaNodeIndexMinFillRate(double areaNodeIndexMinFillRate)
  {
    this->areaNodeIndexMinFillRate=areaNodeIndexMinFillRate;
  }

  void ImportParameter::SetAreaNodeIndexCellSizeAverage(size_t areaNodeIndexCellSizeAverage)
  {
    this->areaNodeIndexCellSizeAverage=areaNodeIndexCellSizeAverage;
  }

  void ImportParameter::SetAreaNodeIndexCellSizeMax(size_t areaNodeIndexCellSizeMax)
  {
    this->areaNodeIndexCellSizeMax=areaNodeIndexCellSizeMax;
  }

  void ImportParameter::SetAreaWayMinMag(size_t areaWayMinMag)
  {
    this->areaWayMinMag=areaWayMinMag;
  }

  void ImportParameter::SetAreaWayIndexMinFillRate(double areaWayIndexMinFillRate)
  {
    this->areaWayIndexMinFillRate=areaWayIndexMinFillRate;
  }

  void ImportParameter::SetAreaWayIndexCellSizeAverage(size_t areaWayIndexCellSizeAverage)
  {
    this->areaWayIndexCellSizeAverage=areaWayIndexCellSizeAverage;
  }

  void ImportParameter::SetAreaWayIndexCellSizeMax(size_t areaWayIndexCellSizeMax)
  {
    this->areaWayIndexCellSizeMax=areaWayIndexCellSizeMax;
  }

  void ImportParameter::SetWaterIndexMinMag(size_t waterIndexMinMag)
  {
    this->waterIndexMinMag=waterIndexMinMag;
  }

  void ImportParameter::SetWaterIndexMaxMag(size_t waterIndexMaxMag)
  {
    this->waterIndexMaxMag=waterIndexMaxMag;
  }

  void ImportParameter::SetOptimizationMaxWayCount(size_t optimizationMaxWayCount)
  {
    this->optimizationMaxWayCount=optimizationMaxWayCount;
  }

  void ImportParameter::SetOptimizationMaxMag(size_t optimizationMaxMag)
  {
    this->optimizationMaxMag=optimizationMaxMag;
  }

  void ImportParameter::SetOptimizationMinMag(size_t optimizationMinMag)
  {
    this->optimizationMinMag=optimizationMinMag;
  }

  void ImportParameter::SetOptimizationCellSizeAverage(size_t optimizationCellSizeAverage)
  {
    this->optimizationCellSizeAverage=optimizationCellSizeAverage;
  }


  void ImportParameter::SetOptimizationCellSizeMax(size_t optimizationCellSizeMax)
  {
    this->optimizationCellSizeMax=optimizationCellSizeMax;
  }


  void ImportParameter::SetOptimizationWayMethod(TransPolygon::OptimizeMethod optimizationWayMethod)
  {
    this->optimizationWayMethod=optimizationWayMethod;
  }

  void ImportParameter::SetRouteNodeBlockSize(size_t blockSize)
  {
    this->routeNodeBlockSize=blockSize;
  }

  void ImportParameter::SetAssumeLand(bool assumeLand)
  {
    this->assumeLand=assumeLand;
  }

  ImportModule::~ImportModule()
  {
    // no code
  }

  static bool ExecuteModules(std::list<ImportModule*>& modules,
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
          progress.Error(std::string("Error while executing step '")+(*module)->GetDescription()+"'!");
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
    modules.push_back(new NumericIndexGenerator<Id,RawRelation>("Generating 'rawrel.idx'",
                                                                AppendFileToDir(parameter.GetDestinationDirectory(),
                                                                                "rawrels.dat"),
                                                                AppendFileToDir(parameter.GetDestinationDirectory(),
                                                                                "rawrel.idx")));
    /* 6 */
    modules.push_back(new RelationDataGenerator());

    /* 7 */
    modules.push_back(new NodeDataGenerator());

    /* 8 */
    modules.push_back(new TurnRestrictionDataGenerator());

    /* 9 */
    modules.push_back(new WayDataGenerator());
    /* 10 */
    modules.push_back(new SortWayDataGenerator());

    /* 11 */
    modules.push_back(new AreaAreaIndexGenerator());
    /* 12 */
    modules.push_back(new AreaWayIndexGenerator());
    /* 13 */
    modules.push_back(new AreaNodeIndexGenerator());

    /* 14 */
    modules.push_back(new CityStreetIndexGenerator());
    /* 15 */
    modules.push_back(new WaterIndexGenerator());
    /* 16 */
    modules.push_back(new OptimizeLowZoomGenerator());

    /* 17 */
    modules.push_back(new RouteDataGenerator());
    /* 18 */
    modules.push_back(new NumericIndexGenerator<Id,RouteNode>("Generating 'route.idx'",
                                                              AppendFileToDir(parameter.GetDestinationDirectory(),
                                                                              "route.dat"),
                                                              AppendFileToDir(parameter.GetDestinationDirectory(),
                                                                              "route.idx")));

    bool result=ExecuteModules(modules,parameter,progress,typeConfig);

    for (std::list<ImportModule*>::iterator module=modules.begin();
         module!=modules.end();
         ++module) {
      delete *module;
    }

    return result;
  }
}

