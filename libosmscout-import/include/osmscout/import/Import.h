#ifndef OSMSCOUT_IMPORT_IMPORT_H
#define OSMSCOUT_IMPORT_IMPORT_H

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

#include <list>
#include <mutex>
#include <string>

#include <osmscout/import/ImportFeatures.h>

#include <osmscout/import/ImportImportExport.h>

#include <osmscout/TypeConfig.h>

#include <osmscout/import/ImportErrorReporter.h>
#include <osmscout/import/ImportProgress.h>
#include <osmscout/import/ImportParameter.h>
#include <osmscout/import/ImportModule.h>

#include <osmscout/util/Magnification.h>
#include <osmscout/util/Progress.h>
#include <osmscout/util/Transformation.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

  /**
    Does the import based on the given parameters. Feedback about the import progress
    is given by the indivudal import modules calling the Progress instance as appropriate.
    */
  class OSMSCOUT_IMPORT_API Importer
  {
  private:
    ImportParameter                      parameter;
    std::vector<ImportModuleRef>         modules;
    std::vector<ImportModuleDescription> moduleDescriptions;

  private:
    bool ValidateDescription(Progress& progress);
    bool ValidateParameter(Progress& progress);
    void GetModuleList(std::vector<ImportModuleRef>& modules);
    void DumpTypeConfigData(const TypeConfig& typeConfig,
                            Progress& progress);
    bool CleanupTemporaries(size_t currentStep,
                            Progress& progress);

    bool ExecuteModules(const TypeConfigRef& typeConfig,
                        ImportProgress& progress);
  public:
    explicit Importer(const ImportParameter& parameter);
    virtual ~Importer() = default;

    bool Import(ImportProgress& progress);

    std::list<std::string> GetProvidedFiles() const;
    std::list<std::string> GetProvidedOptionalFiles() const;
    std::list<std::string> GetProvidedDebuggingFiles() const;
    std::list<std::string> GetProvidedTemporaryFiles() const;
    std::list<std::string> GetProvidedAnalysisFiles() const;
    std::list<std::string> GetProvidedReportFiles() const;
  };
}

#endif
