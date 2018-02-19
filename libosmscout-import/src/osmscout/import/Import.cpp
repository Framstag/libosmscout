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

#include <algorithm>
#include <iostream>
#include <iterator>

#include <osmscout/Types.h>

#include <osmscout/routing/RoutingService.h>
#include <osmscout/routing/RouteNode.h>
#include <osmscout/Intersection.h>

#include <osmscout/import/RawNode.h>
#include <osmscout/import/RawWay.h>
#include <osmscout/import/RawRelation.h>

#include <osmscout/import/GenRawWayIndex.h>
#include <osmscout/import/GenRawRelIndex.h>

#include <osmscout/import/GenTypeDat.h>

#include <osmscout/import/Preprocess.h>

#include <osmscout/import/GenCoordDat.h>

#include <osmscout/import/GenNodeDat.h>
#include <osmscout/import/SortNodeDat.h>

#include <osmscout/import/GenRelAreaDat.h>
#include <osmscout/import/GenWayAreaDat.h>
#include <osmscout/import/MergeAreaData.h>
#include <osmscout/import/GenMergeAreas.h>

#include <osmscout/import/GenWayWayDat.h>
#include <osmscout/import/SortWayDat.h>

#include <osmscout/import/GenNumericIndex.h>

#include <osmscout/import/GenAreaAreaIndex.h>
#include <osmscout/import/GenAreaNodeIndex.h>
#include <osmscout/import/GenAreaWayIndex.h>

#include <osmscout/import/GenCoverageIndex.h>

#include <osmscout/import/GenLocationIndex.h>
#include <osmscout/import/GenOptimizeAreaWayIds.h>
#include <osmscout/import/GenWaterIndex.h>

#include <osmscout/import/GenOptimizeAreasLowZoom.h>
#include <osmscout/import/GenOptimizeWaysLowZoom.h>

// Routing
#include <osmscout/import/GenRouteDat.h>
#include <osmscout/import/GenIntersectionIndex.h>

#if defined(OSMSCOUT_IMPORT_HAVE_LIB_MARISA)
#include <osmscout/import/GenTextIndex.h>
#endif

#include <osmscout/util/MemoryMonitor.h>
#include <osmscout/util/Progress.h>
#include <osmscout/util/StopClock.h>

namespace osmscout {

  static const size_t defaultStartStep=1;
#if defined(OSMSCOUT_IMPORT_HAVE_LIB_MARISA)
  static const size_t defaultEndStep=25;
#else
  static const size_t defaultEndStep=24;
#endif

  ImportParameter::Router::Router(uint8_t vehicleMask,
                                  const std::string& filenamebase)
  : vehicleMask(vehicleMask),
    filenamebase(filenamebase)
  {
     // no code
  }

  ImportParameter::ImportParameter()
   : typefile("map.ost"),
     startStep(defaultStartStep),
     endStep(defaultEndStep),
     eco(false),
     strictAreas(false),
     sortObjects(true),
     sortBlockSize(40000000),
     sortTileMag(14),
     processingQueueSize(std::max((unsigned int)1,std::thread::hardware_concurrency())),
     numericIndexPageSize(1024),
     rawCoordBlockSize(60000000),
     rawNodeDataMemoryMaped(false),
     rawWayIndexMemoryMaped(true),
     rawWayDataMemoryMaped(false),
     rawWayIndexCacheSize(10000),
     rawWayBlockSize(500000),
     coordDataMemoryMaped(false),
     coordIndexCacheSize(1000000),
     coordBlockSize(250000),
     areaDataMemoryMaped(false),
     areaDataCacheSize(0),
     wayDataMemoryMaped(false),
     wayDataCacheSize(0),
     areaAreaIndexMaxMag(17),
     areaNodeMinMag(8),
     areaNodeIndexMinFillRate(0.1),
     areaNodeIndexCellSizeAverage(16),
     areaNodeIndexCellSizeMax(256),
     areaWayMinMag(11), // Should not be >= than optimizationMaxMag
     areaWayIndexMaxLevel(13),
     waterIndexMinMag(6),
     waterIndexMaxMag(14),
     optimizationMaxWayCount(1000000),
     optimizationMaxMag(10),
     optimizationMinMag(0),
     optimizationCellSizeAverage(64),
     optimizationCellSizeMax(255),
     optimizationWayMethod(TransPolygon::quality),
     routeNodeBlockSize(500000),
     assumeLand(AssumeLandStrategy::automatic),
     langOrder({"#"}),
     maxAdminLevel(10),
     firstFreeOSMId(((OSMId)1) << ((sizeof(OSMId)*8)-2)),
     fillWaterArea(20)
  {
    // no code
  }

  ImportParameter::~ImportParameter()
  {

  }

  const std::list<std::string>& ImportParameter::GetMapfiles() const
  {
    return mapfiles;
  }

  std::string ImportParameter::GetTypefile() const
  {
    return typefile;
  }

  std::string ImportParameter::GetDestinationDirectory() const
  {
    return destinationDirectory;
  }

  std::string ImportParameter::GetBoundingPolygonFile() const
  {
    return boundingPolygonFile;
  }

  ImportErrorReporterRef ImportParameter::GetErrorReporter() const
  {
    return errorReporter;
  }

  size_t ImportParameter::GetStartStep() const
  {
    return startStep;
  }

  size_t ImportParameter::GetEndStep() const
  {
    return endStep;
  }

  bool ImportParameter::IsEco() const
  {
    return eco;
  }

  const std::list<ImportParameter::Router>& ImportParameter::GetRouter() const
  {
    return router;
  }

  bool ImportParameter::GetStrictAreas() const
  {
    return strictAreas;
  }

  bool ImportParameter::GetSortObjects() const
  {
    return sortObjects;
  }

  size_t ImportParameter::GetSortBlockSize() const
  {
    return sortBlockSize;
  }

  size_t ImportParameter::GetSortTileMag() const
  {
    return sortTileMag;
  }

  size_t ImportParameter::GetProcessingQueueSize() const
  {
    return processingQueueSize;
  }

  size_t ImportParameter::GetNumericIndexPageSize() const
  {
    return numericIndexPageSize;
  }

  size_t ImportParameter::GetRawCoordBlockSize() const
  {
    return rawCoordBlockSize;
  }

  bool ImportParameter::GetRawNodeDataMemoryMaped() const
  {
    return rawNodeDataMemoryMaped;
  }

  bool ImportParameter::GetRawWayIndexMemoryMaped() const
  {
    return rawWayIndexMemoryMaped;
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

  bool ImportParameter::GetCoordDataMemoryMaped() const
  {
    return coordDataMemoryMaped;
  }

  size_t ImportParameter::GetCoordIndexCacheSize() const
  {
    return coordIndexCacheSize;
  }

  size_t ImportParameter::GetCoordBlockSize() const
  {
    return coordBlockSize;
  }

  size_t ImportParameter::GetAreaDataCacheSize() const
  {
    return areaDataCacheSize;
  }

  bool ImportParameter::GetAreaDataMemoryMaped() const
  {
    return areaDataMemoryMaped;
  }

  size_t ImportParameter::GetWayDataCacheSize() const
  {
    return wayDataCacheSize;
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

  size_t ImportParameter::GetAreaWayIndexMaxLevel() const
  {
    return areaWayIndexMaxLevel;
  }

  size_t ImportParameter::GetAreaAreaIndexMaxMag() const
  {
    return areaAreaIndexMaxMag;
  }

  uint32_t ImportParameter::GetWaterIndexMinMag() const
  {
    return waterIndexMinMag;
  }

  uint32_t ImportParameter::GetWaterIndexMaxMag() const
  {
    return waterIndexMaxMag;
  }

  size_t ImportParameter::GetOptimizationMaxWayCount() const
  {
    return optimizationMaxWayCount;
  }

  uint32_t ImportParameter::GetOptimizationMaxMag() const
  {
    return optimizationMaxMag;
  }

  uint32_t ImportParameter::GetOptimizationMinMag() const
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

  ImportParameter::AssumeLandStrategy ImportParameter::GetAssumeLand() const
  {
    return assumeLand;
  }

  OSMId ImportParameter::GetFirstFreeOSMId() const
  {
    return firstFreeOSMId;
  }

  const std::vector<std::string>& ImportParameter::GetLangOrder() const
  {
    return this->langOrder;
  }

  const std::vector<std::string>& ImportParameter::GetAltLangOrder() const
  {
    return this->altLangOrder;
  }

  size_t ImportParameter::GetMaxAdminLevel() const
  {
    return maxAdminLevel;
  }

  void ImportParameter::SetMapfiles(const std::list<std::string>& mapfiles)
  {
    this->mapfiles=mapfiles;
  }

  void ImportParameter::SetTypefile(const std::string& typefile)
  {
    this->typefile=typefile;
  }

  void ImportParameter::SetDestinationDirectory(const std::string& destinationDirectory)
  {
    this->destinationDirectory=destinationDirectory;
  }

  void ImportParameter::SetBoundingPolygonFile(const std::string& boundingPolygonFile)
  {
    this->boundingPolygonFile=boundingPolygonFile;
  }

  void ImportParameter::SetErrorReporter(const ImportErrorReporterRef& errorReporter)
  {
    this->errorReporter=errorReporter;
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

  void ImportParameter::SetEco(bool eco)
  {
    this->eco=eco;
  }

  void ImportParameter::ClearRouter()
  {
    router.clear();
  }

  void ImportParameter::AddRouter(const Router& router)
  {
    this->router.push_back(router);
  }

  void ImportParameter::SetStrictAreas(bool strictAreas)
  {
    this->strictAreas=strictAreas;
  }

  void ImportParameter::SetSortObjects(bool renumberIds)
  {
    this->sortObjects=renumberIds;
  }

  void ImportParameter::SetSortBlockSize(size_t sortBlockSize)
  {
    this->sortBlockSize=sortBlockSize;
  }

  void ImportParameter::SetSortTileMag(size_t sortTileMag)
  {
    this->sortTileMag=sortTileMag;
  }

  void ImportParameter::SetProcessingQueueSize(size_t processingQueueSize)
  {
    this->processingQueueSize=processingQueueSize;
  }

  void ImportParameter::SetNumericIndexPageSize(size_t numericIndexPageSize)
  {
    this->numericIndexPageSize=numericIndexPageSize;
  }

  void ImportParameter::SetRawCoordBlockSize(size_t blockSize)
  {
    this->rawCoordBlockSize=blockSize;
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

  void ImportParameter::SetRawWayIndexCacheSize(size_t wayIndexCacheSize)
  {
    this->rawWayIndexCacheSize=wayIndexCacheSize;
  }

  void ImportParameter::SetRawWayBlockSize(size_t blockSize)
  {
    this->rawWayBlockSize=blockSize;
  }

  void ImportParameter::SetCoordDataMemoryMaped(bool memoryMaped)
  {
    this->coordDataMemoryMaped=memoryMaped;
  }

  void ImportParameter::SetCoordIndexCacheSize(size_t coordIndexCacheSize)
  {
    this->coordIndexCacheSize=coordIndexCacheSize;
  }

  void ImportParameter::SetCoordBlockSize(size_t coordBlockSize)
  {
    this->coordBlockSize=coordBlockSize;
  }

  void ImportParameter::SetAreaDataMemoryMaped(bool memoryMaped)
  {
    this->areaDataMemoryMaped=memoryMaped;
  }

  void ImportParameter::SetAreaDataCacheSize(size_t areaDataCacheSize)
  {
    this->areaDataCacheSize=areaDataCacheSize;
  }

  void ImportParameter::SetWayDataMemoryMaped(bool memoryMaped)
  {
    this->wayDataMemoryMaped=memoryMaped;
  }

  void ImportParameter::SetWayDataCacheSize(size_t wayDataCacheSize)
  {
    this->wayDataCacheSize=wayDataCacheSize;
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

  void ImportParameter::SetAreaWayIndexMaxMag(size_t areaWayIndexMaxLevel)
  {
    this->areaWayIndexMaxLevel=areaWayIndexMaxLevel;
  }

  void ImportParameter::SetWaterIndexMinMag(uint32_t waterIndexMinMag)
  {
    this->waterIndexMinMag=waterIndexMinMag;
  }

  void ImportParameter::SetWaterIndexMaxMag(uint32_t waterIndexMaxMag)
  {
    this->waterIndexMaxMag=waterIndexMaxMag;
  }

  void ImportParameter::SetOptimizationMaxWayCount(size_t optimizationMaxWayCount)
  {
    this->optimizationMaxWayCount=optimizationMaxWayCount;
  }

  void ImportParameter::SetOptimizationMaxMag(uint32_t optimizationMaxMag)
  {
    this->optimizationMaxMag=optimizationMaxMag;
  }

  void ImportParameter::SetOptimizationMinMag(uint32_t optimizationMinMag)
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

  void ImportParameter::SetAssumeLand(AssumeLandStrategy assumeLand)
  {
    this->assumeLand=assumeLand;
  }

  void ImportParameter::SetLangOrder(const std::vector<std::string>& langOrder)
  {
    this->langOrder = langOrder;
  }

  void ImportParameter::SetAltLangOrder(const std::vector<std::string>& altLangOrder)
  {
    this->altLangOrder = altLangOrder;
  }

  void ImportParameter::SetMaxAdminLevel(size_t maxAdminLevel)
  {
    this->maxAdminLevel=maxAdminLevel;
  }

  void ImportParameter::SetFirstFreeOSMId(OSMId id)
  {
    firstFreeOSMId=id;
  }

  void ImportParameter::SetFillWaterArea(size_t fillWaterArea)
  {
    this->fillWaterArea=fillWaterArea;
  }

  size_t ImportParameter::GetFillWaterArea() const
  {
    return fillWaterArea;
  }

  void ImportParameter::SetPreprocessorFactory(const PreprocessorFactoryRef& factory)
  {
    this->preprocessorFactory=factory;
  }

  std::unique_ptr<Preprocessor> ImportParameter::GetPreprocessor(const std::string& filename,
                                                                 PreprocessorCallback& callback) const
  {
    if (preprocessorFactory) {
      return preprocessorFactory->GetProcessor(filename,
                                              callback);
    }
    return nullptr;
  }

  void ImportModuleDescription::SetName(const std::string& name)
  {
    this->name=name;
  }

  void ImportModuleDescription::SetDescription(const std::string& description)
  {
    this->description=description;
  }

  void ImportModuleDescription::AddProvidedFile(const std::string& providedFile)
  {
    providedFiles.push_back(providedFile);
  }

  void ImportModuleDescription::AddProvidedOptionalFile(const std::string& providedFile)
  {
    providedOptionalFiles.push_back(providedFile);
  }

  void ImportModuleDescription::AddProvidedDebuggingFile(const std::string& providedFile)
  {
    providedDebuggingFiles.push_back(providedFile);
  }

  void ImportModuleDescription::AddProvidedTemporaryFile(const std::string& providedFile)
  {
    providedTemporaryFiles.push_back(providedFile);
  }

  void ImportModuleDescription::AddProvidedAnalysisFile(const std::string& providedFile)
  {
    providedAnalysisFiles.push_back(providedFile);
  }

  void ImportModuleDescription::AddRequiredFile(const std::string& requiredFile)
  {
    requiredFiles.push_back(requiredFile);
  }

  ImportModule::~ImportModule()
  {
    // no code
  }

  void ImportModule::GetDescription(const ImportParameter& /*parameter*/,
                                    ImportModuleDescription& /*description*/) const
  {
    // no code
  }

  Importer::Importer(const ImportParameter& parameter)
  : parameter(parameter)
  {
    GetModuleList(modules);

    for (const auto& module : modules) {
      ImportModuleDescription description;

      module->GetDescription(parameter,
                             description);

      moduleDescriptions.push_back(description);
    }
  }

  Importer::~Importer()
  {
    // no code
  }

  bool Importer::ValidateDescription(Progress& progress)
  {
    std::unordered_set<std::string> temporaryFiles;
    std::unordered_set<std::string> requiredFiles;
    bool                            success=true;

    for (const auto& description : moduleDescriptions) {
      for (const auto& file : description.GetProvidedTemporaryFiles()) {
        temporaryFiles.insert(file);
      }
      for (const auto& file : description.GetRequiredFiles()) {
        requiredFiles.insert(file);
      }
    }

    // Temporary files must be required by some module

    for (const auto& tmpFile : temporaryFiles) {
      if (requiredFiles.find(tmpFile)==requiredFiles.end()) {
        progress.Error("Temporary file '"+tmpFile+"' is not required by any import module");
        success=false;
      }
    }

    return success;
  }

  bool Importer::ValidateParameter(Progress& progress)
  {
    if (parameter.GetAreaWayMinMag()<=parameter.GetOptimizationMaxMag()) {
      progress.Error("Area way index minimum magnification is <= than optimization max magnification");
      return false;
    }

    if (parameter.IsEco() &&
        (parameter.GetStartStep()!=defaultStartStep ||
         parameter.GetEndStep()!=defaultEndStep)) {
      progress.Error("If eco mode is activated you must run all import steps");
    }

    return true;
  }

  void Importer::GetModuleList(std::vector<ImportModuleRef>& modules)
  {
    /* 1 */
    modules.push_back(std::make_shared<TypeDataGenerator>());

    /* 2 */
    modules.push_back(std::make_shared<Preprocess>());

    /* 3 */
    modules.push_back(std::make_shared<CoordDataGenerator>());

    /* 4 */
    modules.push_back(std::make_shared<RawWayIndexGenerator>());
    /* 5 */
    modules.push_back(std::make_shared<RawRelationIndexGenerator>());
    /* 6 */
    modules.push_back(std::make_shared<RelAreaDataGenerator>());

    /* 7 */
    modules.push_back(std::make_shared<WayAreaDataGenerator>());

    /* 8 */
    modules.push_back(std::make_shared<MergeAreaDataGenerator>());

    /* 9 */
    modules.push_back(std::make_shared<MergeAreasGenerator>());

    /* 10 */
    modules.push_back(std::make_shared<WayWayDataGenerator>());

    /* 11 */
    modules.push_back(std::make_shared<OptimizeAreaWayIdsGenerator>());

    /* 12 */
    modules.push_back(std::make_shared<NodeDataGenerator>());

    /* 13 */
    modules.push_back(std::make_shared<SortNodeDataGenerator>());

    /* 14 */
    modules.push_back(std::make_shared<SortWayDataGenerator>());

    /* 15 */
    modules.push_back(std::make_shared<AreaNodeIndexGenerator>());

    /* 16 */
    modules.push_back(std::make_shared<AreaWayIndexGenerator>());

    /* 17 */
    modules.push_back(std::make_shared<AreaAreaIndexGenerator>());

    /* 18 */
    modules.push_back(std::make_shared<CoverageIndexGenerator>());

    /* 19 */
    modules.push_back(std::make_shared<WaterIndexGenerator>());

    /* 20 */
    modules.push_back(std::make_shared<OptimizeAreasLowZoomGenerator>());

    /* 21 */
    modules.push_back(std::make_shared<OptimizeWaysLowZoomGenerator>());

    /* 22 */
    modules.push_back(std::make_shared<LocationIndexGenerator>());

    /* 23 */
    modules.push_back(std::make_shared<RouteDataGenerator>());

    /* 24 */
    modules.push_back(std::make_shared<IntersectionIndexGenerator>());


#if defined(OSMSCOUT_IMPORT_HAVE_LIB_MARISA)
    /* 25 */
    modules.push_back(std::make_shared<TextIndexGenerator>());
#endif
  }

  void Importer::DumpTypeConfigData(const TypeConfig& typeConfig,
                                    Progress& progress)
  {
    progress.Info("Number of types: "+std::to_string(typeConfig.GetTypes().size()));
    progress.Info("Number of node types: "+std::to_string(typeConfig.GetNodeTypes().size())+" "+std::to_string(typeConfig.GetNodeTypeIdBytes())+" byte(s)");
    progress.Info("Number of way types: "+std::to_string(typeConfig.GetWayTypes().size())+" "+std::to_string(typeConfig.GetWayTypeIdBytes())+" byte(s)");
    progress.Info("Number of area types: "+std::to_string(typeConfig.GetAreaTypes().size())+" "+std::to_string(typeConfig.GetAreaTypeIdBytes())+" byte(s)");
  }

  void Importer::DumpModuleDescription(const ImportModuleDescription& description,
                                       Progress& progress)
  {
    for (const auto& filename : description.GetRequiredFiles()) {
      progress.Info("Module requires file '"+filename+"'");
    }
    for (const auto& filename : description.GetProvidedFiles()) {
      progress.Info("Module provides file '"+filename+"'");
    }
    for (const auto& filename : description.GetProvidedOptionalFiles()) {
      progress.Info("Module provides optional file '"+filename+"'");
    }
    for (const auto& filename : description.GetProvidedDebuggingFiles()) {
      progress.Info("Module provides debugging file '"+filename+"'");
    }
    for (const auto& filename : description.GetProvidedTemporaryFiles()) {
      progress.Info("Module provides temporary file '"+filename+"'");
    }
    for (const auto& filename : description.GetProvidedAnalysisFiles()) {
      progress.Info("Module provides analysis file '"+filename+"'");
    }
  }

  bool Importer::CleanupTemporaries(size_t currentStep,
                                    Progress& progress)
  {
    std::set<std::string> allTemporaryFiles;

    for (const auto& description : moduleDescriptions) {
      for (const auto& file : description.GetProvidedTemporaryFiles()) {
        allTemporaryFiles.insert(file);
      }
    }

    std::set<std::string> uptoNowRequiredTemporaryFiles;

    for (const auto& file : moduleDescriptions[currentStep-1].GetRequiredFiles()) {
      if (allTemporaryFiles.find(file)!=allTemporaryFiles.end()) {
        uptoNowRequiredTemporaryFiles.insert(file);
      }
    }

    std::set<std::string> inFutureStillRequiredTemporaryFiles;

    for (size_t step=currentStep; step<moduleDescriptions.size(); step++) {
      for (const auto& file : moduleDescriptions[step].GetRequiredFiles()) {
        if (allTemporaryFiles.find(file)!=allTemporaryFiles.end()) {
          inFutureStillRequiredTemporaryFiles.insert(file);
        }
      }
    }

    std::list<std::string> notAnymoreRequiredFiles;

    std::set_difference(uptoNowRequiredTemporaryFiles.begin(),uptoNowRequiredTemporaryFiles.end(),
                        inFutureStillRequiredTemporaryFiles.begin(),inFutureStillRequiredTemporaryFiles.end(),
                        std::inserter(notAnymoreRequiredFiles,notAnymoreRequiredFiles.begin()));

    for (const auto& file : notAnymoreRequiredFiles) {
      std::string filename=AppendFileToDir(parameter.GetDestinationDirectory(),file);

      progress.Info("Removing temporary file '"+ filename + "'...");

      if (!RemoveFile(filename)) {
        progress.Error("Error while rmeoving file '"+ filename + "'!");
        return false;
      }
    }

    return true;
  }

  bool Importer::ExecuteModules(const TypeConfigRef& typeConfig,
                                Progress& progress)
  {
    StopClock     overAllTimer;
    size_t        currentStep=1;
    MemoryMonitor monitor;
    double        maxVMUsage=0.0;
    double        maxResidentSet=0.0;

    for (const auto& module : modules) {
      if (currentStep>=parameter.GetStartStep() &&
          currentStep<=parameter.GetEndStep()) {
        ImportModuleDescription moduleDescription;
        StopClock               timer;
        bool                    success;
        double                  vmUsage;
        double                  residentSet;

        module->GetDescription(parameter,
                               moduleDescription);

        progress.SetStep("Step #"+
                         std::to_string(currentStep)+
                         " - "+
                         moduleDescription.GetName());
        progress.Info("Module description: "+moduleDescription.GetDescription());

        monitor.Reset();

        DumpModuleDescription(moduleDescription,
                              progress);

        success=module->Import(typeConfig,
                               parameter,
                               progress);

        timer.Stop();

        monitor.GetMaxValue(vmUsage,residentSet);

        maxVMUsage=std::max(maxVMUsage,vmUsage);
        maxResidentSet=std::max(maxResidentSet,residentSet);

        if (vmUsage!=0.0 || residentSet!=0.0) {
          progress.Info(std::string("=> ")+timer.ResultString()+"s, RSS "+ByteSizeToString(residentSet)+", VM "+ByteSizeToString(vmUsage));
        }
        else {
          progress.Info(std::string("=> ")+timer.ResultString()+"s");
        }

        if (!success) {
          progress.Error("Error while executing step '"+moduleDescription.GetName()+"'!");
          return false;
        }

        if (parameter.IsEco()) {
          if (!CleanupTemporaries(currentStep,
                                  progress)) {
            return false;
          }
        }
      }

      currentStep++;
    }

    overAllTimer.Stop();

    if (maxVMUsage!=0.0 || maxResidentSet!=0.0) {
      progress.Info(std::string("Overall ")+overAllTimer.ResultString()+"s, RSS "+ByteSizeToString(maxResidentSet)+", VM "+ByteSizeToString(maxVMUsage));
    }
    else {
      progress.Info(std::string("Overall ")+overAllTimer.ResultString()+"s");
    }

    return true;
  }

  bool Importer::Import(Progress& progress)
  {
    TypeConfigRef typeConfig(std::make_shared<TypeConfig>());

    if (!ValidateDescription(progress)) {
      return false;
    }

    if (!ValidateParameter(progress)) {
      return false;
    }

    progress.SetStep("Loading type config");

    if (!typeConfig->LoadFromOSTFile(parameter.GetTypefile())) {
      progress.Error("Cannot load type configuration!");
      return false;
    }

    DumpTypeConfigData(*typeConfig,
                       progress);

    progress.Info("Parsed language(s) :");
    int langIndex = 0;
    for(const auto& lang : parameter.GetLangOrder()){
      if(lang=="#"){
        progress.Info("  default");
        typeConfig->RegisterNameTag("name", langIndex);
        typeConfig->RegisterNameTag("place_name", langIndex+1);
      } else {
          progress.Info("  " + lang);
          typeConfig->RegisterNameTag("name:"+lang, langIndex);
          typeConfig->RegisterNameTag("place_name:"+lang, langIndex+1);
      }
      langIndex+=2;
    }

    progress.Info("Parsed alt language(s) :");
    langIndex = 0;
    for(const auto& lang : parameter.GetAltLangOrder()){
      if(lang=="#"){
        progress.Info("  default");
        typeConfig->RegisterNameAltTag("name", langIndex);
        typeConfig->RegisterNameAltTag("place_name", langIndex+1);
      } else {
        progress.Info("  " + lang);
        typeConfig->RegisterNameAltTag("name:"+lang, langIndex);
        typeConfig->RegisterNameAltTag("place_name:"+lang, langIndex+1);
      }
      langIndex+=2;
    }

    ImportErrorReporterRef errorReporter=std::make_shared<ImportErrorReporter>(progress,
                                                                               typeConfig,
                                                                               parameter.GetDestinationDirectory());

    parameter.SetErrorReporter(errorReporter);

    bool result=ExecuteModules(typeConfig,
                               progress);

    parameter.GetErrorReporter()->FinishedImport();

    parameter.SetErrorReporter(NULL);

    return result;
  }

  std::list<std::string> Importer::GetProvidedFiles() const
  {
    std::set<std::string> providedFileSet;

    for (const auto& description : moduleDescriptions) {
      for (const auto& file : description.GetProvidedFiles()) {
        providedFileSet.insert(file);
      }
    }

    std::list<std::string> providedFiles(providedFileSet.begin(),providedFileSet.end());

    return providedFiles;
  }

  std::list<std::string> Importer::GetProvidedOptionalFiles() const
  {
    std::set<std::string> providedFileSet;

    for (const auto& description : moduleDescriptions) {
      for (const auto& file : description.GetProvidedOptionalFiles()) {
        providedFileSet.insert(file);
      }
    }

    std::list<std::string> providedFiles(providedFileSet.begin(),providedFileSet.end());

    return providedFiles;
  }

  std::list<std::string> Importer::GetProvidedTemporaryFiles() const
  {
    std::set<std::string> providedFileSet;

    for (const auto& description : moduleDescriptions) {
      for (const auto& file : description.GetProvidedTemporaryFiles()) {
        providedFileSet.insert(file);
      }
    }

    std::list<std::string> providedFiles(providedFileSet.begin(),providedFileSet.end());

    return providedFiles;
  }

  std::list<std::string> Importer::GetProvidedDebuggingFiles() const
  {
    std::set<std::string> providedFileSet;

    for (const auto& description : moduleDescriptions) {
      for (const auto& file : description.GetProvidedDebuggingFiles()) {
        providedFileSet.insert(file);
      }
    }

    std::list<std::string> providedFiles(providedFileSet.begin(),providedFileSet.end());

    return providedFiles;
  }

  std::list<std::string> Importer::GetProvidedAnalysisFiles() const
  {
    std::set<std::string> providedFileSet;

    for (const auto& description : moduleDescriptions) {
      for (const auto& file : description.GetProvidedAnalysisFiles()) {
        providedFileSet.insert(file);
      }
    }

    std::list<std::string> providedFiles(providedFileSet.begin(),providedFileSet.end());

    return providedFiles;
  }

  std::list<std::string> Importer::GetProvidedReportFiles() const
  {
    std::list<std::string> providedFiles={ImportErrorReporter::FILENAME_INDEX_HTML,
                                          ImportErrorReporter::FILENAME_TAG_HTML,
                                          ImportErrorReporter::FILENAME_WAY_HTML,
                                          ImportErrorReporter::FILENAME_RELATION_HTML,
                                          ImportErrorReporter::FILENAME_LOCATION_HTML};

    return providedFiles;
  }

}

