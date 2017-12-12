#ifndef OSMSCOUT_IMPORT_IMPORTERRORREPORTER_H
#define OSMSCOUT_IMPORT_IMPORTERRORREPORTER_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2016  Tim Teulings

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

#include <mutex>
#include <string>

#include <osmscout/private/ImportImportExport.h>

#include <osmscout/ObjectRef.h>
#include <osmscout/TypeConfig.h>

#include <osmscout/util/HTMLWriter.h>
#include <osmscout/util/TagErrorReporter.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

  /**
   * Class to report OSM data problems during import against.
   * Based on the reported errors various HTML bases error reporting pages are generated.
   */
  class OSMSCOUT_IMPORT_API ImportErrorReporter CLASS_FINAL: public TagErrorReporter
  {
  public:
    static const char* const FILENAME_INDEX_HTML;
    static const char* const FILENAME_TAG_HTML;
    static const char* const FILENAME_WAY_HTML;
    static const char* const FILENAME_RELATION_HTML;
    static const char* const FILENAME_LOCATION_HTML;


  private:
    enum Report {
      reportLocation
    };

  private:
    struct ReportError
    {
      Report        report;
      ObjectFileRef ref;
      std::string   error;

      inline ReportError(Report report,
                         const ObjectFileRef& ref,
                         const std::string& error)
      : report(report),
        ref(ref),
        error(error)
      {
        // no code
      }
    };

  private:
    Progress&              progress;

    TypeConfigRef          typeConfig;
    TagId                  nameTagId;

    std::string            destinationDirectory;

    std::list<ReportError> errors;

    HTMLWriter             tagReport;
    size_t                 tagErrorCount;

    HTMLWriter             wayReport;
    size_t                 wayErrorCount;

    HTMLWriter             relationReport;
    size_t                 relationErrorCount;

    HTMLWriter             locationReport;
    size_t                 locationErrorCount;

    HTMLWriter             index;

    std::mutex             mutex;

  private:
    std::string GetName(const ObjectOSMRef& object,
                        const TagMap& tags) const;

  public:
    ImportErrorReporter(Progress& progress,
                        const TypeConfigRef& typeConfig,
                        const std::string& destinationDirectory);

    ~ImportErrorReporter() override;

    void ReportTag(const ObjectOSMRef &object,
                   const TagMap& tags,
                   const std::string& error) override;

    void ReportWay(OSMId id,
                   const TagMap& tags,
                   const std::string& error);

    void ReportRelation(OSMId id,
                        const TagMap& tags,
                        const std::string& error);
    void ReportRelation(OSMId id,
                        const TypeInfoRef& type,
                        const std::string& error);

    void ReportLocationDebug(const ObjectFileRef& object,
                             const std::string& error);

    void ReportLocation(const ObjectFileRef& object,
                        const std::string& error);

    void FinishedImport();
  };

  typedef std::shared_ptr<ImportErrorReporter> ImportErrorReporterRef;
}

#endif
