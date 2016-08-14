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

#include <osmscout/TypeConfig.h>

#include <osmscout/util/HTMLWriter.h>

namespace osmscout {

  /**
   * Class to report OSM data problems during import against.
   * Based on the reported errors various HTML bases error reporting pages are generated.
   */
  class OSMSCOUT_IMPORT_API ImportErrorReporter
  {
  private:
    Progress&     progress;

    TypeConfigRef typeConfig;
    TagId         nameTagId;

    HTMLWriter    wayReport;
    size_t        wayErrorCount;

    HTMLWriter    relationReport;
    size_t        relationErrorCount;

    HTMLWriter    index;

    std::mutex    mutex;

  private:
    std::string GetName(const ObjectOSMRef& object,
                        const TagMap& tags) const;

  public:
    ImportErrorReporter(Progress& progress,
                        const TypeConfigRef& typeConfig,
                        const std::string& destinationDirectory);
    virtual ~ImportErrorReporter();

    void ReportWay(OSMId id,
                   const TagMap& tags,
                   const std::string& error);

    void ReportRelation(OSMId id,
                        const TagMap& tags,
                        const std::string& error);
    void ReportRelation(OSMId id,
                        const TypeInfoRef& type,
                        const std::string& error);
  };

  typedef std::shared_ptr<ImportErrorReporter> ImportErrorReporterRef;
}

#endif
