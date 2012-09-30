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

#include <osmscout/util/FileScanner.h>

/*
 * Example:
 *   src/DumpData ../TravelJinni/ -n 25293125 -w 4290108 -w 26688152 -r 531985
 */

typedef OSMSCOUT_HASHMAP<osmscout::Id,osmscout::FileOffset> CoordPageOffsetMap;

struct Job
{
  enum Type
  {
    Coord,
    Node,
    Way,
    Relation
  };

  Type         type;
  osmscout::Id id;

  Job(Type type, osmscout::Id id)
  : type(type),
    id(id)
  {
    // no code
  }
};

static void DumpIndent(size_t indent)
{
  for (size_t i=1; i<=indent; i++) {
    std::cout << " ";
  }
}

static void DumpCoord(osmscout::Id id,double lat, double lon)
{
  std::cout << "Coord {" << std::endl;
  std::cout << "  id: " << id << std::endl;
  std::cout << "  lat: " << lat << std::endl;
  std::cout << "  lon: " << lon << std::endl;
  std::cout << "}" << std::endl;
}

static void DumpNode(const osmscout::TypeConfig* typeConfig,
                     const osmscout::NodeRef node)
{
  std::cout << "Node {" << std::endl;
  std::cout << "  id: " << node->GetId() << std::endl;

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

  if (attributes.StartIsJoint()) {
    DumpIndent(indent);
    std::cout << "startIsJoint: true" << std::endl;
  }

  if (attributes.EndIsJoint()) {
    DumpIndent(indent);
    std::cout << "endIsJoint: true" << std::endl;
  }
}

static void DumpWay(const osmscout::TypeConfig* typeConfig,
                    const osmscout::WayRef way)
{
  if (way->IsArea()) {
    std::cout << "Area {" << std::endl;
  }
  else {
    std::cout << "Way {" << std::endl;
  }

  std::cout << "  id: " << way->GetId() << std::endl;

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
      std::cout << "  node[" << n << "] { id: " << way->nodes[n].GetId() << " lat: " << way->nodes[n].GetLat() << " lon: "<< way->nodes[n].GetLon() << " }" << std::endl;
    }
  }

  std::cout << "}" << std::endl;
}

static void DumpRelation(const osmscout::TypeConfig* typeConfig,
                         const osmscout::RelationRef relation)
{
  if (relation->IsArea()) {
    std::cout << "AreaRelation {" << std::endl;
  }
  else {
    std::cout << "WayRelation {" << std::endl;
  }
  std::cout << "  id: " << relation->GetId() << std::endl;

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
  int            arg=1;
  std::string    map;
  std::list<Job> jobs;
  bool           needCoordFile=false;

  if (argc<2) {
    std::cerr << "DumpData <map directory> {-c <Id>|-n <Id>|-w <Id>|-r <Id>}" << std::endl;
    return 1;
  }

  map=argv[arg];

  arg++;

  while (arg<argc) {
    if (strcmp(argv[arg],"-c")==0) {
      unsigned long id;

      arg++;
      if (arg>=argc) {
        std::cerr << "Option -c requires parameter!" << std::endl;
        return 1;
      }

      if (sscanf(argv[arg],"%lu",&id)!=1) {
        std::cerr << "Node id is not numeric!" << std::endl;
        return 1;
      }

      jobs.push_back(Job(Job::Coord,id));

      needCoordFile=true;

      arg++;
    }
    else if (strcmp(argv[arg],"-n")==0) {
      unsigned long id;

      arg++;
      if (arg>=argc) {
        std::cerr << "Option -n requires parameter!" << std::endl;
        return 1;
      }

      if (sscanf(argv[arg],"%lu",&id)!=1) {
        std::cerr << "Node id is not numeric!" << std::endl;
        return 1;
      }

      jobs.push_back(Job(Job::Node,id));

      arg++;
    }
    else if (strcmp(argv[arg],"-w")==0) {
      unsigned long id;

      arg++;
      if (arg>=argc) {
        std::cerr << "Option -w requires parameter!" << std::endl;
        return 1;
      }

      if (sscanf(argv[arg],"%lu",&id)!=1) {
        std::cerr << "Node id is not numeric!" << std::endl;
        return 1;
      }

      jobs.push_back(Job(Job::Way,id));

      arg++;
    }
    else if (strcmp(argv[arg],"-r")==0) {
      unsigned long id;

      arg++;
      if (arg>=argc) {
        std::cerr << "Option -r requires parameter!" << std::endl;
        return 1;
      }

      if (sscanf(argv[arg],"%lu",&id)!=1) {
        std::cerr << "Node id is not numeric!" << std::endl;
        return 1;
      }

      jobs.push_back(Job(Job::Relation,id));

      arg++;
    }
    else {
      std::cerr << "Unknown parameter '" << argv[arg] << "'!" << std::endl;
      return 1;
    }
  }

  osmscout::DatabaseParameter databaseParameter;
  osmscout::Database          database(databaseParameter);

  if (!database.Open(map.c_str())) {
    std::cerr << "Cannot open database" << std::endl;

    return 1;
  }

  osmscout::FileScanner coordDataFile;
  uint32_t              coordPageSize;
  CoordPageOffsetMap    coordPageIdOffsetMap;

  if (needCoordFile) {
    if (coordDataFile.Open(osmscout::AppendFileToDir(map,"coord.dat"),
                           osmscout::FileScanner::LowMemRandom,
                           false)) {
      osmscout::FileOffset mapFileOffset;

      if (!coordDataFile.Read(coordPageSize))
      {
        std::cerr << "Error while reading page size from coord.dat" << std::endl;
        return 1;
      }

      if (!coordDataFile.Read(mapFileOffset)) {
        std::cerr << "Error while reading map offset from coord.dat" << std::endl;
        return 1;
      }

      coordDataFile.SetPos(mapFileOffset);

      uint32_t mapSize;

      if (!coordDataFile.Read(mapSize)) {
        std::cerr << "Error while reading map size from coord.dat" << std::endl;
        return 1;
      }

      for (size_t i=1; i<=mapSize; i++) {
        osmscout::Id         pageId;
        osmscout::FileOffset pageOffset;

        if (!coordDataFile.Read(pageId)) {
          std::cerr << "Error while reading pageId from coord.dat" << std::endl;
          return 1;
        }
        if (!coordDataFile.Read(pageOffset)) {
          std::cerr << "Error while reading pageOffset from coord.dat" << std::endl;
          return 1;
        }

        coordPageIdOffsetMap[pageId]=pageOffset;
      }
    }
  }

  std::cout << std::endl;

  for (std::list<Job>::const_iterator job=jobs.begin();
      job!=jobs.end();
      ++job) {
    if (job!=jobs.begin()) {
      std::cout << std::endl;
    }

    if (job->type==Job::Coord) {
      if (!coordDataFile.IsOpen()) {
        continue;
      }

      double                             lat;
      double                             lon;
      osmscout::Id                       coordPageId=job->id/coordPageSize;
      CoordPageOffsetMap::const_iterator pageOffset=coordPageIdOffsetMap.find(coordPageId);

      if (pageOffset==coordPageIdOffsetMap.end()) {
        continue;
      }

      coordDataFile.SetPos(pageOffset->second+(job->id%coordPageSize)*2*sizeof(uint32_t));
      coordDataFile.ReadCoord(lat,lon);

      if (coordDataFile.HasError()) {
        std::cerr << "Error while reading data from offset " << pageOffset->second << " of file " << coordDataFile.GetFilename() << "!" << std::endl;
        continue;
      }

      DumpCoord(job->id,lat,lon);
    }
    else if (job->type==Job::Node) {
      osmscout::NodeRef node;

      if (!database.GetNode(job->id,node)) {
        std::cerr << "Cannot load node " << job->id << std::endl;
        continue;
      }

      DumpNode(database.GetTypeConfig(),node);
    }
    else if (job->type==Job::Way) {
      osmscout::WayRef way;

      if (!database.GetWay(job->id,way)) {
        std::cerr << "Cannot load way " << job->id << std::endl;
        continue;
      }

      DumpWay(database.GetTypeConfig(),way);
    }
    else if (job->type==Job::Relation) {
      osmscout::RelationRef relation;

      if (!database.GetRelation(job->id,relation)) {
        std::cerr << "Cannot load relation " << job->id << std::endl;
        continue;
      }

      DumpRelation(database.GetTypeConfig(),relation);
    }
  }

  database.Close();

  return 0;
}
