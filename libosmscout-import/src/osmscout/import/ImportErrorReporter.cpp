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

#include <osmscout/DebugDatabase.h>

#include <osmscout/util/File.h>

namespace osmscout {

  const char* const ImportErrorReporter::FILENAME_INDEX_HTML    = "index.html";
  const char* const ImportErrorReporter::FILENAME_TAG_HTML      = "tag.html";
  const char* const ImportErrorReporter::FILENAME_WAY_HTML      = "way.html";
  const char* const ImportErrorReporter::FILENAME_RELATION_HTML = "relation.html";
  const char* const ImportErrorReporter::FILENAME_LOCATION_HTML = "location.html";

  ImportErrorReporter::ImportErrorReporter(Progress& progress,
                                           const TypeConfigRef& typeConfig,
                                           const std::string& destinationDirectory)
    : progress(progress),
      typeConfig(typeConfig),
      destinationDirectory(destinationDirectory),
      tagErrorCount(0),
      wayErrorCount(0),
      relationErrorCount(0),
      locationErrorCount(0)
  {
    nameTagId=typeConfig->GetTagId("name");

    tagReport.Open(AppendFileToDir(destinationDirectory,FILENAME_TAG_HTML));
    tagReport.WriteDocumentStart();
    tagReport.WriteHeader("Tag value errors","Import errors related to tags","","..stylesheet.css");
    tagReport.WriteBodyStart();
    tagReport.WriteListStart();

    wayReport.Open(AppendFileToDir(destinationDirectory,FILENAME_WAY_HTML));
    wayReport.WriteDocumentStart();
    wayReport.WriteHeader("Way format errors","Import errors related to ways","","..stylesheet.css");
    wayReport.WriteBodyStart();
    wayReport.WriteListStart();

    relationReport.Open(AppendFileToDir(destinationDirectory,FILENAME_RELATION_HTML));
    relationReport.WriteDocumentStart();
    relationReport.WriteHeader("Relation format errors","Import errors related to relations","","..stylesheet.css");
    relationReport.WriteBodyStart();
    relationReport.WriteListStart();

    locationReport.Open(AppendFileToDir(destinationDirectory,FILENAME_LOCATION_HTML));
    locationReport.WriteDocumentStart();
    locationReport.WriteHeader("Location format errors","Import errors related to location information","","..stylesheet.css");
    locationReport.WriteBodyStart();
    locationReport.WriteListStart();

    index.Open(AppendFileToDir(destinationDirectory,FILENAME_INDEX_HTML));
    index.WriteDocumentStart();
    index.WriteHeader("Index","Index of all reports","","..stylesheet.css");
    index.WriteBodyStart();
    index.WriteListStart();

    index.WriteListEntryStart();
    index.WriteLink("tag.html","List of tag import errors");
    index.WriteListEntryEnd();

    index.WriteListEntryStart();
    index.WriteLink("way.html","List of way import errors");
    index.WriteListEntryEnd();

    index.WriteListEntryStart();
    index.WriteLink("relation.html","List of relation import errors");
    index.WriteListEntryEnd();

    index.WriteListEntryStart();
    index.WriteLink("location.html","List of location related import errors");
    index.WriteListEntryEnd();

    index.WriteListEnd();
    index.WriteBodyEnd();
    index.WriteDocumentEnd();
    index.CloseFailsafe();
  }

  ImportErrorReporter::~ImportErrorReporter()
  {
    locationReport.WriteListEnd();
    locationReport.WriteText(std::to_string(locationErrorCount)+" errors");
    locationReport.WriteBodyEnd();
    locationReport.WriteDocumentEnd();

    locationReport.CloseFailsafe();

    relationReport.WriteListEnd();
    relationReport.WriteText(std::to_string(relationErrorCount)+" errors");
    relationReport.WriteBodyEnd();
    relationReport.WriteDocumentEnd();

    relationReport.CloseFailsafe();

    wayReport.WriteListEnd();
    wayReport.WriteText(std::to_string(wayErrorCount)+" errors");
    wayReport.WriteBodyEnd();
    wayReport.WriteDocumentEnd();

    wayReport.CloseFailsafe();

    tagReport.WriteListEnd();
    tagReport.WriteText(std::to_string(tagErrorCount)+" errors");
    tagReport.WriteBodyEnd();
    tagReport.WriteDocumentEnd();

    tagReport.CloseFailsafe();
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

  void ImportErrorReporter::ReportTag(const ObjectOSMRef& object,
                                      const TagMap& tags,
                                      const std::string& error)
  {
    std::unique_lock <std::mutex> lock(mutex);

    std::string name(GetName(object,tags));

    progress.Warning(name+" - "+error);

    tagReport.WriteListEntryStart();
    tagReport.WriteOSMObjectLink(object,name);
    tagReport.WriteText(" - ");
    tagReport.WriteText(error);
    tagReport.WriteListEntryEnd();

    tagErrorCount++;
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

    wayErrorCount++;
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

    relationErrorCount++;
  }

  void ImportErrorReporter::ReportRelation(OSMId id,
                                           const TypeInfoRef& type,
                                           const std::string& error)
  {
    std::unique_lock <std::mutex> lock(mutex);

    ObjectOSMRef object(id,osmRefRelation);
    std::string  name;

    if (!type->GetIgnore()) {
      name=object.GetName()+" ("+type->GetName()+")";
    }
    else {
      name=object.GetName();
    }

    progress.Warning(name+" - "+error);

    relationReport.WriteListEntryStart();
    relationReport.WriteOSMObjectLink(object,name);
    relationReport.WriteText(" - ");
    relationReport.WriteText(error);
    relationReport.WriteListEntryEnd();

    relationErrorCount++;
  }

  void ImportErrorReporter::ReportLocationDebug(const ObjectFileRef& object,
                                                const std::string& error)
  {
    progress.Debug(object.GetName()+" - "+error);

    errors.emplace_back(reportLocation,object,error);
  }

  void ImportErrorReporter::ReportLocation(const ObjectFileRef& object,
                                           const std::string& error)
  {
    progress.Warning(object.GetName()+" - "+error);

    errors.emplace_back(reportLocation,object,error);
  }

  void ImportErrorReporter::FinishedImport()
  {
    if (errors.empty()) {
      return;
    }

    progress.SetAction("Resolving OSM objects from error reports");

    DebugDatabaseParameter debugDatabaseParameter;
    DebugDatabase          database(debugDatabaseParameter);

    if (database.Open(destinationDirectory)) {
      std::set<ObjectOSMRef>               ids;
      std::set<ObjectFileRef>              fileOffsets;
      std::map<ObjectOSMRef,ObjectFileRef> idFileOffsetMap;
      std::map<ObjectFileRef,ObjectOSMRef> fileOffsetIdMap;

      for (const auto& error : errors) {
        fileOffsets.insert(error.ref);
      }

      database.ResolveReferences(ids,fileOffsets,idFileOffsetMap,fileOffsetIdMap);

      for (const auto& error : errors) {
        std::map<ObjectFileRef,ObjectOSMRef>::const_iterator entry=fileOffsetIdMap.find(error.ref);

        if (entry!=fileOffsetIdMap.end()) {
          locationReport.WriteListEntryStart();
          locationReport.WriteOSMObjectLink(entry->second,entry->second.GetName());
          locationReport.WriteText(" - ");
          locationReport.WriteText(error.error);
          locationReport.WriteListEntryEnd();
        }
      }

      database.Close();
    }
  }
}

