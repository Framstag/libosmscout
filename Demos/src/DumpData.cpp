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

/*
 * Example:
 *   src/DumpData ../TravelJinni/ -n 25293125 -w 4290108 -w 26688152 -r 531985
 */

struct Job
{
  enum Type
  {
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

void DumpNode(const osmscout::TypeConfig* typeConfig,
              const osmscout::NodeRef node)
{
  std::cout << "Node {" << std::endl;
  std::cout << "  id: " << node->GetId() << std::endl;
  std::cout << "  type: " << typeConfig->GetTypeInfo(node->GetType()).GetName() << std::endl;
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

void DumpWay(const osmscout::TypeConfig* typeConfig,
             const osmscout::WayRef way)
{
  if (way->IsArea()) {
    std::cout << "Area {" << std::endl;
  }
  else {
    std::cout << "Way {" << std::endl;
  }

  std::cout << "  id: " << way->GetId() << std::endl;
  std::cout << "  type: " << typeConfig->GetTypeInfo(way->GetType()).GetName() << std::endl;

  if (!way->GetName().empty()) {
    std::cout << "  name: " << way->GetName() << std::endl;
  }

  if (!way->GetRefName().empty()) {
    std::cout << "  ref: " << way->GetRefName() << std::endl;
  }

  if (way->IsArea()) {
    if (!way->GetHouseNr().empty()) {
      std::cout << "  houseNr: " << way->GetHouseNr() << std::endl;
    }
  }
  else {
    if (way->IsBridge()) {
      std::cout << "  bridge: true" << std::endl;
    }

    if (way->IsTunnel()) {
      std::cout << "  tunnel: true" << std::endl;
    }

    if (way->IsOneway()) {
      std::cout << "  oneway: true" << std::endl;
    }

    if (way->IsRoundabout()) {
      std::cout << "  roundabout: true" << std::endl;
    }

    if (way->GetWidth()!=0) {
      std::cout << "  width: " << way->GetWidth() << std::endl;
    }

    if (way->GetLayer()!=0) {
      std::cout << "  layer: " << way->GetLayer() << std::endl;
    }

    if (way->GetMaxSpeed()!=0) {
      std::cout << "  maxSpeed: " << (size_t)way->GetMaxSpeed() << std::endl;
    }

    if (way->StartIsJoint()) {
      std::cout << "  startIsJoint: true" << std::endl;
    }

    if (way->EndIsJoint()) {
      std::cout << "  endIsJoint: true" << std::endl;
    }
  }

  if (!way->HasAccess()) {
    std::cout << "  access: false" << std::endl;
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
      std::cout << "  node[" << n << "] { id: " << way->nodes[n].GetId() << " lat: " << way->nodes[n].GetLat() << " lon: "<< way->nodes[n].GetLon() << "}" << std::endl;
    }
  }

  std::cout << "}" << std::endl;
}

void DumpRelation(const osmscout::TypeConfig* typeConfig,
                  const osmscout::RelationRef relation)
{
  if (relation->IsArea()) {
    std::cout << "AreaRelation {" << std::endl;
  }
  else {
    std::cout << "WayRelation {" << std::endl;
  }
  std::cout << "  id: " << relation->GetId() << std::endl;
  std::cout << "  type: " << typeConfig->GetTypeInfo(relation->GetType()).GetName() << std::endl;

  if (!relation->GetName().empty()) {
    std::cout << "  name: " << relation->GetName() << std::endl;
  }

  if (!relation->GetRefName().empty()) {
    std::cout << "  ref: " << relation->GetRefName() << std::endl;
  }

  if (relation->IsArea()) {
    // no thing
  }
  else {
    if (relation->IsBridge()) {
      std::cout << "  bridge: true" << std::endl;
    }

    if (relation->IsTunnel()) {
      std::cout << "  tunnel: true" << std::endl;
    }

    if (relation->IsOneway()) {
      std::cout << "  oneway: true" << std::endl;
    }

    if (relation->GetLayer()!=0) {
      std::cout << "  layer: " << relation->GetLayer() << std::endl;
    }
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

  if (argc<2) {
    std::cerr << "DumpData <map directory> {-n <Id>|-w <Id>|-r <Id>}" << std::endl;
    return 1;
  }

  map=argv[arg];

  arg++;

  while (arg<argc) {
    if (strcmp(argv[arg],"-n")==0) {
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

  std::cout << std::endl;

  for (std::list<Job>::const_iterator job=jobs.begin();
      job!=jobs.end();
      ++job) {
    if (job!=jobs.begin()) {
      std::cout << std::endl;
    }

    if (job->type==Job::Node) {
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
