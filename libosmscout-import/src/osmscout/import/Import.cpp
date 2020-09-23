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

#include <osmscout/OSMScoutTypes.h>

#include <osmscout/Intersection.h>

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

#include <osmscout/import/GenRoute2Dat.h>
#include <osmscout/import/GenAreaRouteIndex.h>

// Routing
#include <osmscout/import/GenRouteDat.h>
#include <osmscout/import/GenIntersectionIndex.h>

// Public Transport
#include <osmscout/import/GenPTRouteDat.h>

#if defined(OSMSCOUT_IMPORT_HAVE_LIB_MARISA)
#include <osmscout/import/GenTextIndex.h>
#endif

#include <osmscout/util/MemoryMonitor.h>
#include <osmscout/util/Progress.h>

namespace osmscout {

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
        (parameter.GetStartStep()!=ImportParameter::GetDefaultStartStep() ||
         parameter.GetEndStep()!=ImportParameter::GetDefaultEndStep())) {
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

    /* 25 */
    modules.push_back(std::make_shared<PTRouteDataGenerator>());

    /* 26 */
    modules.push_back(std::make_shared<RouteDataGenerator2>());

    /* 27 */
    modules.push_back(std::make_shared<AreaRouteIndexGenerator>());

#if defined(OSMSCOUT_IMPORT_HAVE_LIB_MARISA)
    /* 28 */
    modules.push_back(std::make_shared<TextIndexGenerator>());
#endif

    assert(modules.size()==ImportParameter::GetDefaultEndStep());
  }

  void Importer::DumpTypeConfigData(const TypeConfig& typeConfig,
                                    Progress& progress)
  {
    progress.Info("Number of types: "+std::to_string(typeConfig.GetTypes().size()));
    progress.Info("Number of node types: "+std::to_string(typeConfig.GetNodeTypes().size())+" "+std::to_string(typeConfig.GetNodeTypeIdBytes())+" byte(s)");
    progress.Info("Number of way types: "+std::to_string(typeConfig.GetWayTypes().size())+" "+std::to_string(typeConfig.GetWayTypeIdBytes())+" byte(s)");
    progress.Info("Number of area types: "+std::to_string(typeConfig.GetAreaTypes().size())+" "+std::to_string(typeConfig.GetAreaTypeIdBytes())+" byte(s)");
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
        progress.Error("Error while removing file '"+ filename + "'!");
        return false;
      }
    }

    return true;
  }

  bool Importer::ExecuteModules(const TypeConfigRef& typeConfig,
                                ImportProgress& progress)
  {
    size_t        currentStep=1;

    for (const auto& module : modules) {
      if (currentStep>=parameter.GetStartStep() &&
          currentStep<=parameter.GetEndStep()) {
        ImportModuleDescription moduleDescription;
        bool                    success;

        module->GetDescription(parameter,
                               moduleDescription);

        progress.StartModule(currentStep, moduleDescription);

        success=module->Import(typeConfig,
                               parameter,
                               progress);

        progress.FinishedModule();

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

    return true;
  }

  bool Importer::Import(ImportProgress& progress)
  {
    TypeConfigRef typeConfig(std::make_shared<TypeConfig>());
    progress.StartImport(parameter);

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
    uint32_t langIndex = 0;
    for(const auto& lang : parameter.GetLangOrder()){
      if(lang=="#"){
        progress.Info("  default");
        typeConfig->GetTagRegistry().RegisterNameTag("name", langIndex);
        typeConfig->GetTagRegistry().RegisterNameTag("place_name", langIndex+1);
        typeConfig->GetTagRegistry().RegisterNameTag("brand", langIndex+2);
      } else {
          progress.Info("  " + lang);
          typeConfig->GetTagRegistry().RegisterNameTag("name:"+lang, langIndex);
          typeConfig->GetTagRegistry().RegisterNameTag("place_name:"+lang, langIndex+1);
          typeConfig->GetTagRegistry().RegisterNameTag("brand:"+lang, langIndex+2);
      }
      langIndex+=3;
    }

    progress.Info("Parsed alt language(s) :");
    langIndex = 0;
    for(const auto& lang : parameter.GetAltLangOrder()){
      if(lang=="#"){
        progress.Info("  default");
        typeConfig->GetTagRegistry().RegisterNameAltTag("name", langIndex);
        typeConfig->GetTagRegistry().RegisterNameAltTag("place_name", langIndex+1);
        typeConfig->GetTagRegistry().RegisterNameAltTag("brand", langIndex+2);
      } else {
        progress.Info("  " + lang);
        typeConfig->GetTagRegistry().RegisterNameAltTag("name:"+lang, langIndex);
        typeConfig->GetTagRegistry().RegisterNameAltTag("place_name:"+lang, langIndex+1);
        typeConfig->GetTagRegistry().RegisterNameAltTag("brand:"+lang, langIndex+2);
      }
      langIndex+=3;
    }

    ImportErrorReporterRef errorReporter=std::make_shared<ImportErrorReporter>(progress,
                                                                               typeConfig,
                                                                               parameter.GetDestinationDirectory());

    parameter.SetErrorReporter(errorReporter);

    bool result=ExecuteModules(typeConfig,
                               progress);

    parameter.GetErrorReporter()->FinishedImport();
    progress.FinishedImport();

    parameter.SetErrorReporter(nullptr);

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

