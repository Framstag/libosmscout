/*
  DumpData - a demo program for libosmscout
  Copyright (C) 2012  Tim Teulings

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <cstring>
#include <iostream>

#include <osmscout/Database.h>
#include <osmscout/DebugDatabase.h>

#include <list>
#include <map>
#include <string>
#include <vector>

/*
 * Example:
 *   src/DumpData ../TravelJinni/ -n 25293125 -w 4290108 -w 26688152 -r 531985
 */

struct Job
{
  osmscout::ObjectRef      idRef;
  osmscout::ObjectFileRef  fileRef;

  Job()
  {
    // no code
  }

  Job(const osmscout::ObjectRef& idRef)
  : idRef(idRef)
  {
    // no code
  }

  Job(const osmscout::ObjectFileRef& fileRef)
  : fileRef(fileRef)
  {
    // no code
  }
};

static bool ParseArguments(int argc,
                           char* argv[],
                           std::string& map,
                           std::list<osmscout::Id>& coordIds,
                           std::list<Job>& jobs)
{
  if (argc<2) {
    std::cerr << "DumpData <map directory> {-c <Id>|-n <Id>|-w <Id>|-wo <FileOffset>|-r <Id>|-ro <FileOffset>}" << std::endl;
    return false;
  }

  int arg=1;

  map=argv[arg];

  arg++;

  while (arg<argc) {
    if (strcmp(argv[arg],"-c")==0) {
      unsigned long id;

      arg++;
      if (arg>=argc) {
        std::cerr << "Option -c requires parameter!" << std::endl;
        return false;
      }

      if (sscanf(argv[arg],"%lu",&id)!=1) {
        std::cerr << "Node id is not numeric!" << std::endl;
        return false;
      }

      coordIds.push_back(id);

      arg++;
    }
    else if (strcmp(argv[arg],"-n")==0) {
      unsigned long id;

      arg++;
      if (arg>=argc) {
        std::cerr << "Option -n requires parameter!" << std::endl;
        return false;
      }

      if (sscanf(argv[arg],"%lu",&id)!=1) {
        std::cerr << "Node id is not numeric!" << std::endl;
        return false;
      }

      jobs.push_back(Job(osmscout::ObjectRef(id,osmscout::refNode)));

      arg++;
    }
    else if (strcmp(argv[arg],"-w")==0) {
      unsigned long id;

      arg++;
      if (arg>=argc) {
        std::cerr << "Option -w requires parameter!" << std::endl;
        return false;
      }

      if (sscanf(argv[arg],"%lu",&id)!=1) {
        std::cerr << "Node id is not numeric!" << std::endl;
        return false;
      }

      jobs.push_back(Job(osmscout::ObjectRef(id,osmscout::refWay)));

      arg++;
    }
    else if (strcmp(argv[arg],"-wo")==0) {
      unsigned long fileOffset;

      arg++;
      if (arg>=argc) {
        std::cerr << "Option -wo requires parameter!" << std::endl;
        return false;
      }

      if (sscanf(argv[arg],"%lu",&fileOffset)!=1) {
        std::cerr << "Way file offset is not numeric!" << std::endl;
        return false;
      }

      jobs.push_back(Job(osmscout::ObjectFileRef(fileOffset,osmscout::refWay)));

      arg++;
    }
    else if (strcmp(argv[arg],"-r")==0) {
      unsigned long id;

      arg++;
      if (arg>=argc) {
        std::cerr << "Option -r requires parameter!" << std::endl;
        return false;
      }

      if (sscanf(argv[arg],"%lu",&id)!=1) {
        std::cerr << "Relation id is not numeric!" << std::endl;
        return false;
      }

      jobs.push_back(Job(osmscout::ObjectRef(id,osmscout::refRelation)));

      arg++;
    }
    else if (strcmp(argv[arg],"-ro")==0) {
      unsigned long fileOffset;

      arg++;
      if (arg>=argc) {
        std::cerr << "Option -ro requires parameter!" << std::endl;
        return false;
      }

      if (sscanf(argv[arg],"%lu",&fileOffset)!=1) {
        std::cerr << "Relation file offset is not numeric!" << std::endl;
        return false;
      }

      jobs.push_back(Job(osmscout::ObjectFileRef(fileOffset,osmscout::refRelation)));

      arg++;
    }
    else {
      std::cerr << "Unknown parameter '" << argv[arg] << "'!" << std::endl;
      return false;
    }
  }

  return true;
}

static void DumpIndent(size_t indent)
{
  for (size_t i=1; i<=indent; i++) {
    std::cout << " ";
  }
}

static void DumpCoord(const osmscout::Point& coord)
{
  std::cout << "Coord {" << std::endl;
  std::cout << "  id: " << coord.GetId() << std::endl;
  std::cout << "  lat: " << coord.GetLat() << std::endl;
  std::cout << "  lon: " << coord.GetLon() << std::endl;
  std::cout << "}" << std::endl;
}

static void DumpNode(const osmscout::TypeConfig* typeConfig,
                     const osmscout::NodeRef node,
                     osmscout::Id id)
{
  std::cout << "Node {" << std::endl;
  std::cout << "  id: " << id << std::endl;
  std::cout << "  fileOffset: " << node->GetFileOffset() << std::endl;

  if (node->GetType()!=osmscout::typeIgnore) {
    std::cout << "  type: " << typeConfig->GetTypeInfo(node->GetType()).GetName() << std::endl;
  }

  std::cout << "  lat: " << node->GetLat() << std::endl;
  std::cout << "  lon: " << node->GetLon() << std::endl;

  if (node->HasTags()) {
    std::cout << std::endl;

    for (size_t t=0; t<node->GetTagCount(); t++) {
      std::cout << "  " << typeConfig->GetTagInfo(node->GetTagKey(t)).GetName() << ": " << node->GetTagValue(t) << std::endl;
    }
  }

  std::cout << "}" << std::endl;
}

static void DumpGeneralSegmentAttributes(const osmscout::SegmentAttributes& attributes,
                                         const osmscout::TypeConfig* typeConfig,
                                         size_t indent)
{
  if (attributes.GetType()!=osmscout::typeIgnore) {
    DumpIndent(indent);
    std::cout << "type: " << typeConfig->GetTypeInfo(attributes.GetType()).GetName() << std::endl;
  }

  if (!attributes.GetName().empty()) {
    DumpIndent(indent);
    std::cout << "name: " << attributes.GetName() << std::endl;
  }

  if (!attributes.GetRefName().empty()) {
    DumpIndent(indent);
    std::cout << "ref: " << attributes.GetRefName() << std::endl;
  }

  if (!attributes.HasAccess()) {
    DumpIndent(indent);
    std::cout << "access: false" << std::endl;
  }
}

static void DumpAreaSegmentAttributes(const osmscout::SegmentAttributes& attributes,
                                      const osmscout::TypeConfig* typeConfig,
                                      size_t indent)
{
  DumpGeneralSegmentAttributes(attributes,
                               typeConfig,
                               indent);

  if (!attributes.GetHouseNr().empty()) {
    DumpIndent(indent);
    std::cout << "houseNr: " << attributes.GetHouseNr() << std::endl;
  }
}

static void DumpWaySegmentAttributes(const osmscout::SegmentAttributes& attributes,
                                     const osmscout::TypeConfig* typeConfig,
                                     size_t indent)
{
  DumpGeneralSegmentAttributes(attributes,
                               typeConfig,
                               indent);

  if (attributes.IsBridge()) {
    DumpIndent(indent);
    std::cout << "bridge: true" << std::endl;
  }

  if (attributes.IsTunnel()) {
    DumpIndent(indent);
    std::cout << "tunnel: true" << std::endl;
  }

  if (attributes.IsOneway()) {
    DumpIndent(indent);
    std::cout << "oneway: true" << std::endl;
  }

  if (attributes.IsRoundabout()) {
    DumpIndent(indent);
    std::cout << "roundabout: true" << std::endl;
  }

  if (attributes.GetWidth()!=0) {
    DumpIndent(indent);
    std::cout << "width: " << (size_t)attributes.GetWidth() << std::endl;
  }

  if (attributes.GetLayer()!=0) {
    DumpIndent(indent);
    std::cout << "layer: " << (size_t)attributes.GetLayer() << std::endl;
  }

  if (attributes.GetMaxSpeed()!=0) {
    DumpIndent(indent);
    std::cout << "maxSpeed: " << (size_t)attributes.GetMaxSpeed() << std::endl;
  }

  DumpIndent(indent);
  std::cout << "grade: " << (size_t)attributes.GetGrade() << std::endl;
}

static void DumpWay(const osmscout::TypeConfig* typeConfig,
                    const osmscout::WayRef way,
                    osmscout::Id id)
{
  if (way->IsArea()) {
    std::cout << "Area {" << std::endl;
  }
  else {
    std::cout << "Way {" << std::endl;
  }

  std::cout << "  id: " << id << std::endl;
  std::cout << "  fileOffset: " << way->GetFileOffset() << std::endl;

  if (way->IsArea()) {
    DumpAreaSegmentAttributes(way->GetAttributes(),
                              typeConfig,
                              2);
  }
  else {
    DumpWaySegmentAttributes(way->GetAttributes(),
                             typeConfig,
                             2);
  }

  if (way->HasTags()) {
    std::cout << std::endl;

    for (size_t t=0; t<way->GetTagCount(); t++) {
      std::cout << "  " << typeConfig->GetTagInfo(way->GetTagKey(t)).GetName() << ": " << way->GetTagValue(t) << std::endl;
    }
  }

  if (!way->nodes.empty()) {
    std::cout << std::endl;

    for (size_t n=0; n<way->nodes.size(); n++) {
      std::cout << "  node[" << n << "] {";

      if (way->ids[n]!=0) {
        std::cout << " id: " << way->ids[n];
      }

      std::cout << " lat: " << way->nodes[n].GetLat() << " lon: "<< way->nodes[n].GetLon() << " }" << std::endl;
    }
  }

  std::cout << "}" << std::endl;
}

static void DumpRelation(const osmscout::TypeConfig* typeConfig,
                         const osmscout::RelationRef relation,
                         osmscout::Id id)
{
  if (relation->IsArea()) {
    std::cout << "AreaRelation {" << std::endl;
  }
  else {
    std::cout << "WayRelation {" << std::endl;
  }

  std::cout << "  id: " << id << std::endl;
  std::cout << "  fileOffset: " << relation->GetFileOffset() << std::endl;

  if (relation->IsArea()) {
    DumpAreaSegmentAttributes(relation->GetAttributes(),
                              typeConfig,
                              2);
  }
  else {
    DumpWaySegmentAttributes(relation->GetAttributes(),
                             typeConfig,
                             2);
  }

  for (size_t r=0; r<relation->roles.size(); r++) {
    std::cout << std::endl;
    std::cout << "  role[" << r << "] {" << std::endl;

    if (!relation->IsArea() &&
        !relation->roles[r].role.empty()) {
      std::cout << "    role: " << relation->roles[r].role << std::endl;
    }

    if (relation->IsArea()) {
      std::cout << "    ring: " << (size_t)relation->roles[r].ring << std::endl;
    }

    if (relation->IsArea()) {
      DumpAreaSegmentAttributes(relation->roles[r].GetAttributes(),
                                typeConfig,
                                4);
    }
    else {
      DumpWaySegmentAttributes(relation->roles[r].GetAttributes(),
                               typeConfig,
                               4);
    }

    std::cout << "  }" << std::endl;
  }

  if (relation->HasTags()) {
    std::cout << std::endl;

    for (size_t t=0; t<relation->GetTagCount(); t++) {
      std::cout << "  " << typeConfig->GetTagInfo(relation->GetTagKey(t)).GetName() << ": " << relation->GetTagValue(t) << std::endl;
    }
  }

  std::cout << "}" << std::endl;
}

int main(int argc, char* argv[])
{
  std::string                    map;
  std::list<Job>                 jobs;

  std::list<osmscout::Id>        coordIds;

  if (!ParseArguments(argc,
                      argv,
                      map,
                      coordIds,
                      jobs)) {
    return 1;
  }

  osmscout::DatabaseParameter      databaseParameter;
  osmscout::Database               database(databaseParameter);
  osmscout::DebugDatabaseParameter debugDatabaseParameter;
  osmscout::DebugDatabase          debugDatabase(debugDatabaseParameter);

  if (!database.Open(map.c_str())) {
    std::cerr << "Cannot open database" << std::endl;
  }

  if (!debugDatabase.Open(map.c_str())) {
    std::cerr << "Cannot open debug database" << std::endl;
  }

  std::set<osmscout::Id>         nodeIds;
  std::set<osmscout::Id>         wayIds;
  std::set<osmscout::Id>         relIds;
  std::set<osmscout::FileOffset> nodeFileOffsets;
  std::set<osmscout::FileOffset> wayFileOffsets;
  std::set<osmscout::FileOffset> relFileOffsets;

  for (std::list<Job>::const_iterator job=jobs.begin();
       job!=jobs.end();
       ++job) {
    switch (job->idRef.GetType()) {
    case osmscout::refNone:
      break;
    case osmscout::refNode:
      nodeIds.insert(job->idRef.GetId());
      break;
    case osmscout::refWay:
      wayIds.insert(job->idRef.GetId());
      break;
    case osmscout::refRelation:
      relIds.insert(job->idRef.GetId());
      break;
    }

    switch (job->fileRef.GetType()) {
    case osmscout::refNone:
      break;
    case osmscout::refNode:
      nodeFileOffsets.insert(job->fileRef.GetFileOffset());
      break;
    case osmscout::refWay:
      wayFileOffsets.insert(job->fileRef.GetFileOffset());
      break;
    case osmscout::refRelation:
      relFileOffsets.insert(job->fileRef.GetFileOffset());
      break;
    }
  }

  std::map<osmscout::Id,osmscout::FileOffset> nodeIdFileOffsetMap;
  std::map<osmscout::FileOffset,osmscout::Id> nodeFileOffsetIdMap;
  std::map<osmscout::Id,osmscout::FileOffset> wayIdFileOffsetMap;
  std::map<osmscout::FileOffset,osmscout::Id> wayFileOffsetIdMap;
  std::map<osmscout::Id,osmscout::FileOffset> relIdFileOffsetMap;
  std::map<osmscout::FileOffset,osmscout::Id> relFileOffsetIdMap;

  if (!nodeIds.empty() ||
      !nodeFileOffsets.empty()) {
    if (!debugDatabase.ResolveNodeIdsAndOffsets(nodeIds,
                                                nodeIdFileOffsetMap,
                                                nodeFileOffsets,
                                                nodeFileOffsetIdMap)) {
      std::cerr << "Error while resolving node ids and file offsets" << std::endl;
    }
  }

  if (!wayIds.empty() ||
      !wayFileOffsets.empty()) {
    if (!debugDatabase.ResolveWayIdsAndOffsets(wayIds,
                                               wayIdFileOffsetMap,
                                               wayFileOffsets,
                                               wayFileOffsetIdMap)) {
      std::cerr << "Error while resolving way ids and file offsets" << std::endl;
    }
  }

  if (!relIds.empty() ||
      !relFileOffsets.empty()) {
    if (!debugDatabase.ResolveRelationIdsAndOffsets(relIds,
                                                    relIdFileOffsetMap,
                                                    relFileOffsets,
                                                    relFileOffsetIdMap)) {
      std::cerr << "Error while resolving relation ids and file offsets" << std::endl;
    }
  }

  std::vector<osmscout::Point>       coords;
  std::vector<osmscout::NodeRef>     nodes;
  std::vector<osmscout::WayRef>      ways;
  std::vector<osmscout::RelationRef> relations;

  if (!coordIds.empty()) {
    std::vector<osmscout::Id> ids(coordIds.begin(),coordIds.end());

    if (!debugDatabase.GetCoords(ids,
                                 coords)) {
      std::cerr << "Error whole loading coords by id" << std::endl;
    }
  }

  if (!nodeFileOffsetIdMap.empty()) {
    std::list<osmscout::FileOffset> offsets;

    for (std::map<osmscout::FileOffset,osmscout::Id>::const_iterator entry=nodeFileOffsetIdMap.begin();
         entry!=nodeFileOffsetIdMap.end();
         ++entry) {
      offsets.push_back(entry->first);
    }

    if (!database.GetNodesByOffset(offsets,
                                   nodes)) {
      std::cerr << "Error whole loading nodes by offset" << std::endl;
    }
  }

  if (!wayFileOffsetIdMap.empty()) {
    std::list<osmscout::FileOffset> offsets;

    for (std::map<osmscout::FileOffset,osmscout::Id>::const_iterator entry=wayFileOffsetIdMap.begin();
         entry!=wayFileOffsetIdMap.end();
         ++entry) {
      offsets.push_back(entry->first);
    }

    if (!database.GetWaysByOffset(offsets,
                                  ways)) {
      std::cerr << "Error whole loading ways by offset" << std::endl;
    }
  }

  if (!relFileOffsetIdMap.empty()) {
    std::list<osmscout::FileOffset> offsets;

    for (std::map<osmscout::FileOffset,osmscout::Id>::const_iterator entry=relFileOffsetIdMap.begin();
         entry!=relFileOffsetIdMap.end();
         ++entry) {
      offsets.push_back(entry->first);
    }

    if (!database.GetRelationsByOffset(offsets,
                                       relations)) {
      std::cerr << "Error whole loading relations by offset" << std::endl;
    }
  }

  for (std::list<osmscout::Id>::const_iterator id=coordIds.begin();
       id!=coordIds.end();
       ++id) {
    bool found=false;

    for (size_t i=0; i<coords.size(); i++) {
      if (coords[i].GetId()==*id) {
        if (id!=coordIds.begin()) {
          std::cout << std::endl;
        }

        DumpCoord(coords[i]);
        found=true;
        break;
      }
    }

    if (!found) {
        std::cerr << "Cannot find coord with id " << *id << std::endl;
    }
  }

  for (std::list<Job>::const_iterator job=jobs.begin();
       job!=jobs.end();
       ++job) {
    if (job!=jobs.begin() ||
        !coordIds.empty()) {
      std::cout << std::endl;
    }

    if (job->idRef.GetType()==osmscout::refNode) {
      std::map<osmscout::Id,osmscout::FileOffset>::const_iterator offset=nodeIdFileOffsetMap.find(job->idRef.GetId());

      if (offset==nodeIdFileOffsetMap.end()) {
        std::cerr << "Cannot find node with id " << job->idRef.GetId() << std::endl;
      }

      for (size_t i=0; i<nodes.size(); i++) {
        if (nodes[i]->GetFileOffset()==offset->second) {
          DumpNode(debugDatabase.GetTypeConfig(),nodes[i],job->idRef.GetId());
          break;
        }
      }
    }
    else if (job->idRef.GetType()==osmscout::refWay) {
      std::map<osmscout::Id,osmscout::FileOffset>::const_iterator offset=wayIdFileOffsetMap.find(job->idRef.GetId());

      if (offset==wayIdFileOffsetMap.end()) {
        std::cerr << "Cannot find way with id " << job->idRef.GetId() << std::endl;
      }

      for (size_t i=0; i<ways.size(); i++) {
        if (ways[i]->GetFileOffset()==offset->second) {
          DumpWay(debugDatabase.GetTypeConfig(),ways[i],job->idRef.GetId());
          break;
        }
      }
    }
    else if (job->fileRef.GetType()==osmscout::refNode) {
      std::map<osmscout::FileOffset,osmscout::Id>::const_iterator id=nodeFileOffsetIdMap.find(job->fileRef.GetFileOffset());

      if (id==nodeFileOffsetIdMap.end()) {
        std::cerr << "Cannot find node with file offset " << job->fileRef.GetFileOffset() << std::endl;
      }

      for (size_t i=0; i<nodes.size(); i++) {
        if (nodes[i]->GetFileOffset()==job->fileRef.GetFileOffset()) {
          DumpNode(debugDatabase.GetTypeConfig(),nodes[i],id->second);
          break;
        }
      }
    }
    else if (job->fileRef.GetType()==osmscout::refWay) {
      std::map<osmscout::FileOffset,osmscout::Id>::const_iterator id=wayFileOffsetIdMap.find(job->fileRef.GetFileOffset());

      if (id==wayFileOffsetIdMap.end()) {
        std::cerr << "Cannot find way with file offset " << job->fileRef.GetFileOffset() << std::endl;
      }

      for (size_t i=0; i<ways.size(); i++) {
        if (ways[i]->GetFileOffset()==job->fileRef.GetFileOffset()) {
          DumpWay(debugDatabase.GetTypeConfig(),ways[i],id->second);
          break;
        }
      }
    }
    else if (job->idRef.GetType()==osmscout::refRelation) {
      std::map<osmscout::Id,osmscout::FileOffset>::const_iterator offset=relIdFileOffsetMap.find(job->idRef.GetId());

      if (offset==relIdFileOffsetMap.end()) {
        std::cerr << "Cannot find relation with id " << job->idRef.GetId() << std::endl;
      }

      for (size_t i=0; i<relations.size(); i++) {
        if (relations[i]->GetFileOffset()==offset->second) {
          DumpRelation(debugDatabase.GetTypeConfig(),relations[i],job->idRef.GetId());
          break;
        }
      }
    }
    else if (job->fileRef.GetType()==osmscout::refRelation) {
      std::map<osmscout::FileOffset,osmscout::Id>::const_iterator id=relFileOffsetIdMap.find(job->fileRef.GetFileOffset());

      if (id==relFileOffsetIdMap.end()) {
        std::cerr << "Cannot find relation with file offset " << job->fileRef.GetFileOffset() << std::endl;
      }

      for (size_t i=0; i<relations.size(); i++) {
        if (relations[i]->GetFileOffset()==job->fileRef.GetFileOffset()) {
          DumpRelation(debugDatabase.GetTypeConfig(),relations[i],id->second);
          break;
        }
      }
    }
  }

  database.Close();

  debugDatabase.Close();

  return 0;
}
