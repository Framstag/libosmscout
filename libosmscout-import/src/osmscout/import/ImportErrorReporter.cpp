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

#include <osmscout/import/ImportErrorReporter.h>

#include <sstream>

#include <osmscout/util/File.h>
#include <osmscout/util/String.h>

namespace osmscout {

  ImportErrorReporter::ImportErrorReporter(Progress& progress,
                                           const TypeConfigRef& typeConfig,
                                           const std::string& destinationDirectory)
    : progress(progress),
      typeConfig(typeConfig)
  {
    nameTagId=typeConfig->GetTagId("name");

    wayReport.Open(AppendFileToDir(destinationDirectory,"way.html"));
    wayReport.WriteDocumentStart();
    wayReport.WriteHeader("Way format errors","Import errors related to ways","","..stylesheet.css");
    wayReport.WriteBodyStart();
    wayReport.WriteListStart();

    relationReport.Open(AppendFileToDir(destinationDirectory,"relation.html"));
    relationReport.WriteDocumentStart();
    relationReport.WriteHeader("Relation format errors","Import errors related to relations","","..stylesheet.css");
    relationReport.WriteBodyStart();
    relationReport.WriteListStart();

    index.Open(AppendFileToDir(destinationDirectory,"index.html"));
    index.WriteDocumentStart();
    index.WriteHeader("Index","Index of all reports","","..stylesheet.css");
    index.WriteBodyStart();
    index.WriteListStart();

    index.WriteListEntryStart();
    index.WriteLink("way.html","List of way import errors");
    index.WriteListEntryEnd();

    index.WriteListEntryStart();
    index.WriteLink("relation.html","List of relation import errors");
    index.WriteListEntryEnd();

    index.WriteListEnd();
    index.WriteBodyEnd();
    index.WriteDocumentEnd();
    index.CloseFailsafe();
  }

  ImportErrorReporter::~ImportErrorReporter()
  {
    relationReport.WriteListEnd();
    relationReport.WriteBodyEnd();
    relationReport.WriteDocumentEnd();

    relationReport.CloseFailsafe();

    wayReport.WriteListEnd();
    wayReport.WriteBodyEnd();
    wayReport.WriteDocumentEnd();

    wayReport.CloseFailsafe();
  }

  std::string ImportErrorReporter::GetName(const ObjectOSMRef& object,
                                           const TagMap& tags) const
  {
    std::stringstream buffer;

    buffer << object.GetName();

    if (nameTagId!=tagIgnore) {
      const auto entry=tags.find(nameTagId);

      if (entry!=tags.end()) {
        buffer << " \"" << entry->second << "\"";
      }
    }

    return buffer.str();
  }

  void ImportErrorReporter::ReportWay(OSMId id,
                                      const TagMap& tags,
                                      const std::string& error)
  {
    std::unique_lock <std::mutex> lock(mutex);

    ObjectOSMRef object(id,osmRefWay);
    std::string  name(GetName(object,tags));

    progress.Warning(name+" - "+error);

    wayReport.WriteListEntryStart();
    wayReport.WriteOSMObjectLink(object,name);
    wayReport.WriteText(" - ");
    wayReport.WriteText(error);
    wayReport.WriteListEntryEnd();
  }

  void ImportErrorReporter::ReportRelation(OSMId id,
                                           const TagMap& tags,
                                           const std::string& error)
  {
    std::unique_lock <std::mutex> lock(mutex);

    ObjectOSMRef object(id,osmRefRelation);
    std::string  name(GetName(object,tags));

    progress.Warning(name+" - "+error);

    relationReport.WriteListEntryStart();
    relationReport.WriteOSMObjectLink(object,name);
    relationReport.WriteText(" - ");
    relationReport.WriteText(error);
    relationReport.WriteListEntryEnd();
  }

  void ImportErrorReporter::ReportRelation(OSMId id,
                                           const TypeInfoRef& type,
                                           const std::string& error)
  {
    std::unique_lock <std::mutex> lock(mutex);

    ObjectOSMRef object(id,osmRefRelation);
    std::string  name;

    if (!type->GetIgnore()) {
      name=std::string(object.GetTypeName())+" "+NumberToString(id)+" ("+type->GetName()+")";
    }
    else {
      name=std::string(object.GetTypeName())+" "+NumberToString(id);
    }

    progress.Warning(name+" - "+error);

    relationReport.WriteListEntryStart();
    relationReport.WriteOSMObjectLink(object,name);
    relationReport.WriteText(" - ");
    relationReport.WriteText(error);
    relationReport.WriteListEntryEnd();
  }
}

