#ifndef OSMSCOUT_IMPORTMODULE_H
#define OSMSCOUT_IMPORTMODULE_H

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

#include <osmscout/import/ImportErrorReporter.h>
#include <osmscout/import/ImportParameter.h>
#include <osmscout/import/ImportImportExport.h>

namespace osmscout {

class OSMSCOUT_IMPORT_API ImportModuleDescription CLASS_FINAL
{
private:
  std::string            name;
  std::string            description;
  std::list<std::string> providedFiles;
  std::list<std::string> providedOptionalFiles;
  std::list<std::string> providedDebuggingFiles;
  std::list<std::string> providedTemporaryFiles;
  std::list<std::string> providedAnalysisFiles;
  std::list<std::string> requiredFiles;

public:
  void SetName(const std::string& name);
  void SetDescription(const std::string& description);

  void AddProvidedFile(const std::string& providedFile);
  void AddProvidedOptionalFile(const std::string& providedFile);
  void AddProvidedDebuggingFile(const std::string& providedFile);
  void AddProvidedTemporaryFile(const std::string& providedFile);
  void AddProvidedAnalysisFile(const std::string& providedFile);
  void AddRequiredFile(const std::string& requiredFile);

  inline std::string GetName() const
  {
    return name;
  }

  inline std::string GetDescription() const
  {
    return description;
  }

  inline std::list<std::string> GetProvidedFiles() const
  {
    return providedFiles;
  }

  inline std::list<std::string> GetProvidedOptionalFiles() const
  {
    return providedOptionalFiles;
  }

  inline std::list<std::string> GetProvidedDebuggingFiles() const
  {
    return providedDebuggingFiles;
  }

  inline std::list<std::string> GetProvidedTemporaryFiles() const
  {
    return providedTemporaryFiles;
  }

  inline std::list<std::string> GetProvidedAnalysisFiles() const
  {
    return providedAnalysisFiles;
  }

  inline std::list<std::string> GetRequiredFiles() const
  {
    return requiredFiles;
  }
};

/**
  A single import module representing a single import step.

  An import consists of a number of sequentially executed steps. A step normally
  works on one object type and generates one output file (though this is just
  an suggestion). Such a step is realized by a ImportModule.
  */
class OSMSCOUT_IMPORT_API ImportModule
{
public:
  virtual ~ImportModule();

  virtual void GetDescription(const ImportParameter& parameter,
                              ImportModuleDescription& description) const;

  /**
   * Do the import
   *
   * @param typeConfig
   *   Type configuration
   * @param parameter
   *   Import parameter
   * @param progress
   *   Progress object, for tracking import progress
   */
  virtual bool Import(const TypeConfigRef& typeConfig,
                      const ImportParameter& parameter,
                      Progress& progress) = 0;
};

using ImportModuleRef = std::shared_ptr<ImportModule>;

}
#endif //OSMSCOUT_IMPORTMODULE_H
