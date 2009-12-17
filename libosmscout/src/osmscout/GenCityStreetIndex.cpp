/*
  TravelJinni - Openstreetmap offline viewer
  Copyright (C) 2009  Tim Teulings

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

#include <osmscout/GenCityStreetIndex.h>

#include <cassert>
#include <cmath>
#include <list>
#include <set>

#include <osmscout/FileWriter.h>
#include <osmscout/RawNode.h>
#include <osmscout/RawRelation.h>
#include <osmscout/RawWay.h>

#include <osmscout/Reference.h>
#include <osmscout/Way.h>

#include <osmscout/Util.h>

struct Node
{
  Id     id;
  double lon;
  double lat;
};

struct City
{
  Id          id;
  RefType     refType;
  Id          urbanId;
  std::string name;
};

struct Urban
{
  Id                     id;
  std::vector<Node>      area;

  std::list<City>        cities;

  std::map<std::string,std::list<Id> > ways;
  std::map<std::string,std::list<Id> > areas;

  double                 minlon;
  double                 minlat;
  double                 maxlon;
  double                 maxlat;
};

static void GetRelationIdsFromRelations(const std::list<RawRelation>& relations, std::set<Id>& ids)
{
  ids.clear();

  for (std::list<RawRelation>::const_iterator iter=relations.begin();
       iter!=relations.end();
       ++iter) {
    for (size_t i=0; i<iter->members.size(); i++) {
      if (iter->members[i].type==RawRelation::memberRelation) {
        ids.insert(iter->members[i].id);
      }
    }
  }
}

static void GetWayIdsFromRelations(const std::list<RawRelation>& relations, std::set<Id>& ids)
{
  ids.clear();

  for (std::list<RawRelation>::const_iterator iter=relations.begin();
       iter!=relations.end();
       ++iter) {
    for (size_t i=0; i<iter->members.size(); i++) {
      if (iter->members[i].type==RawRelation::memberWay) {
        ids.insert(iter->members[i].id);
      }
    }
  }
}

static void GetNodeIdsFromAreas(const std::list<RawWay>& areas,
                                std::set<Id>& ids)
{
  ids.clear();

  for (std::list<RawWay>::const_iterator iter=areas.begin();
       iter!=areas.end();
       ++iter) {
    for (size_t i=0; i<iter->nodes.size(); i++) {
      ids.insert(iter->nodes[i]);
    }
  }
}

static void GetNodeIdsFromAreasAndRelations(const std::list<RawWay>& areas,
                                            const std::list<RawRelation>& relations,
                                            std::set<Id>& ids)
{
  ids.clear();

  for (std::list<RawWay>::const_iterator iter=areas.begin();
       iter!=areas.end();
       ++iter) {
    for (size_t i=0; i<iter->nodes.size(); i++) {
      ids.insert(iter->nodes[i]);
    }
  }

  for (std::list<RawRelation>::const_iterator iter=relations.begin();
       iter!=relations.end();
       ++iter) {
    for (size_t i=0; i<iter->members.size(); i++) {
      if (iter->members[i].type==RawRelation::memberNode) {
        ids.insert(iter->members[i].id);
      }
    }
  }
}

static void RemoveUnresolvedAreas(std::list<RawWay>& areas,
                                  const std::map<Id,RawNode>& nodes,
                                  Progress& progress)
{
  std::list<RawWay>::iterator area=areas.begin();

  while (area!=areas.end()) {
    bool resolved=true;

    for (size_t i=0; i<area->nodes.size(); i++) {
      std::map<Id,RawNode>::const_iterator node=nodes.find(area->nodes[i]);

      if (node==nodes.end()) {
        progress.Warning(std::string("Area ")+NumberToString(area->id)+" has at least one unresolved node, skipping");
        resolved=false;
        break;
      }
    }

    if (resolved) {
      ++area;
    }
    else {
      area=areas.erase(area);
    }
  }
}

static void RemoveUnresolvedRelations(std::list<RawRelation>& relations,
                                      const std::map<Id,RawNode>& nodes,
                                      Progress& progress)
{
  std::list<RawRelation>::iterator rel=relations.begin();

  while (rel!=relations.end()) {
    bool resolved=true;

    for (size_t i=0; i<rel->members.size(); i++) {
      if (rel->members[i].type!=RawRelation::memberNode) {
        progress.Warning(std::string("Relation ")+NumberToString(rel->id)+" has unresolved way "+NumberToString(rel->members[i].id)+" of relation, skipping");
        resolved=false;
        break;
      }
      else {
        std::map<Id,RawNode>::const_iterator node=nodes.find(rel->members[i].id);

        if (node==nodes.end()) {
          progress.Warning(std::string("Relation ")+NumberToString(rel->id)+" has unresolved node, skipping");
          resolved=false;
          break;
        }
      }
    }

    if (resolved) {
      ++rel;
    }
    else {
      rel=relations.erase(rel);
    }
  }
}

/**
  This method replaces all references in a relation with their corresponding
  nodes. The method assures that nodes are inserted in an order that they build up an
  full area boundary. We can however not assure that the ordering is clockwise. It might
  be just the other way round (for the "is point in area" problem this is not a problem!).

  For the transformation to succeed it must be assured that there is no member referencing
  relations or nodes at the start of the replaces. Such relation will be skiped. The replacement
  is of order O(n^2) where n is the number of ways.
 */
static void ResolveWaysInRelations(std::list<RawRelation>& relations,
                                   const std::map<Id,RawWay>& ways,
                                   Progress& progress)
{
  std::list<RawRelation>::iterator rel=relations.begin();

  while (rel!=relations.end()) {
    bool              convertable=true;
    std::list<RawWay> w;

    for (std::vector<RawRelation::Member>::iterator member=rel->members.begin();
         member!=rel->members.end();
         ++member) {
      if (member->type==RawRelation::memberWay) {
        // We currently on check the outer bound and ignore ways in any other role
        // especially role "inner"
        if (member->role.empty() || member->role=="outer") {
          std::map<Id,RawWay>::const_iterator way=ways.find(member->id);

          if (way!=ways.end()) {
            w.push_back(way->second);
          }
          else {
            progress.Warning(std::string("Cannot resolve way ")+NumberToString(member->id)+" in relation "+NumberToString(rel->id));
            convertable=false;
          }
        }
      }
      else {
        progress.Warning(std::string("Member in relation ")+NumberToString(rel->id)+" is not of type way");
        convertable=false;
      }
    }

    if (!convertable || w.size()<=1) {
      progress.Warning(std::string("Skipping relation ")+NumberToString(rel->id)+", because it does not only consist of ways or no ways at all");
      rel=relations.erase(rel);
      continue;
    }

    //std::cout << "Resolving relation " << rel->id << " starting with way " << w.begin()->id << std::endl;
    // Just copy the first way into the relation as list of nodes

    std::list<RawWay>                wb=w;
    std::vector<RawRelation::Member> ms;

    for (size_t i=0; i<w.begin()->nodes.size(); i++) {
      RawRelation::Member m;

      m.type=RawRelation::memberNode;
      m.id=w.begin()->nodes[i];

      ms.push_back(m);
    }

    w.erase(w.begin());

    // Now, while ways available, find the way that either has the last node in relation
    // as start or end
    // Copy all nodes but the first/last one (which is already in the list)
    while (w.size()>1) {
      bool found=false;

      for (std::list<RawWay>::iterator cw=w.begin();
           cw!=w.end();
           ++cw) {
        if (ms.back().id==cw->nodes[0]) {
          //std::cout << "Continuing with way " << cw->id << std::endl;
          for (size_t i=1; i<cw->nodes.size(); i++) {
            RawRelation::Member m;

            m.type=RawRelation::memberNode;
            m.id=cw->nodes[i];

            ms.push_back(m);
          }

          w.erase(cw);
          found=true;
          break;
        }
        else if (ms.back().id==cw->nodes[cw->nodes.size()-1]) {
          //std::cout << "Continuing with way " << cw->id << " (reverse)" << std::endl;
          for (int i=cw->nodes.size()-2; i>=0; i--) {
            RawRelation::Member m;

            m.type=RawRelation::memberNode;
            m.id=cw->nodes[i];

            ms.push_back(m);
          }

          w.erase(cw);
          found=true;
          break;
        }
      }

      if (!found) {
        progress.Warning(std::string("Cannot find way with node ")+NumberToString(ms.back().id)+" as start or end node");
        break;
      }
    }

    if (w.size()>1) {
      progress.Warning(std::string("Cannot resolve ways in Relation ")+NumberToString(rel->id)+" to build an area boundary, skipping");
      /*
      for (std::list<RawWay>::const_iterator iter=wb.begin();
           iter!=wb.end();
           ++iter) {
        std::cout << iter->id << " " << iter->nodes[0] << " " << iter->nodes[iter->nodes.size()-1] << std::endl;
      }*/
      rel=relations.erase(rel);
      continue;
    }

    if (ms.back().id==w.begin()->nodes[0] &&
        ms.front().id==w.begin()->nodes[w.begin()->nodes.size()-1]) {
      //std::cout << "Finishing with way " << w.begin()->id << std::endl;
      for (size_t i=1; i<w.begin()->nodes.size()-1; i++) {
        RawRelation::Member m;

        m.type=RawRelation::memberNode;
        m.id=w.begin()->nodes[i];

        ms.push_back(m);
      }

      w.erase(w.begin());
    }
    else if (ms.back().id==w.begin()->nodes[w.begin()->nodes.size()-1] &&
      ms.front().id==w.begin()->nodes[0]) {
      //std::cout << "Finishing with way " << w.begin()->id << " (reverse)" << std::endl;
      for (int i=w.begin()->nodes.size()-2; i>=1; i--) {
        RawRelation::Member m;

        m.type=RawRelation::memberNode;
        m.id=w.begin()->nodes[i];

        ms.push_back(m);
      }

      w.erase(w.begin());
    }
    else {
      progress.Warning(std::string("Cannot find way with node ")+NumberToString(ms.back().id)+" as start or end node");
      progress.Warning(std::string("Cannot resolve ways in Relation ")+NumberToString(rel->id)+" to build an area boundary, skipping");
      /*
      for (std::list<RawWay>::const_iterator iter=wb.begin();
           iter!=wb.end();
           ++iter) {
        std::cout << iter->id << " " << iter->nodes[0] << " " << iter->nodes[iter->nodes.size()-1] << std::endl;
      }*/
      rel=relations.erase(rel);
      continue;
    }

    // Remove everything but nodes

    rel->members=ms;

    ++rel;
  }
}

static bool GetNodesFromNodeIds(const std::set<Id> & ids,
                                std::map<Id,RawNode>& nodes,
                                Progress& progress)
{
  FileScanner scanner;

  nodes.clear();

  if (ids.size()!=0) {
    progress.Info(std::string("Resolving ")+NumberToString(ids.size())+" node ids");

    if (!scanner.Open("rawnodes.dat")) {
      progress.Error("Cannot open 'rawnodes.dat'");
      return false;
    }

    while (!scanner.HasError()) {
      RawNode node;

      node.Read(scanner);

      if (!scanner.HasError()) {
        if (ids.find(node.id)!=ids.end()) {
          nodes[node.id]=node;
        }
      }
    }

    scanner.Close();

    progress.Info(NumberToString(ids.size())+" ids searched, "+NumberToString(nodes.size())+" nodes found");

    for (std::set<Id>::const_iterator id=ids.begin();
         id!=ids.end();
         ++id) {
      if (nodes.find(*id)==nodes.end()) {
        progress.Warning(std::string("Node Id ")+NumberToString(*id)+" not found!");
      }
    }
  }

  return true;
}

static bool GetWaysFromWayIds(const std::set<Id> & ids,
                              std::map<Id,RawWay>& ways,
                              Progress& progress)
{
  FileScanner scanner;

  ways.clear();

  if (ids.size()!=0) {
    progress.Info(std::string("Resolving ")+NumberToString(ids.size())+" way ids");

    if (!scanner.Open("rawways.dat")) {
      progress.Error("Cannot open 'rawways.dat'");
      return false;
    }

    while (!scanner.HasError()) {
      RawWay way;

      way.Read(scanner);

      if (!scanner.HasError()) {
        if (ids.find(way.id)!=ids.end()) {
          ways[way.id]=way;
        }
      }
    }

    scanner.Close();

    progress.Info(NumberToString(ids.size())+" ids searched, "+NumberToString(ways.size())+" ways found");

    for (std::set<Id>::const_iterator id=ids.begin();
         id!=ids.end();
         ++id) {
      if (ways.find(*id)==ways.end()) {
        progress.Warning(std::string("Ways Id ")+NumberToString(*id)+" not found!");
      }
    }
  }

  return true;
}

bool IsPointInArea(double lon, double lat, const std::vector<RawNode>& nodes)
{
  int  i,j;
  bool c = false;

  for (i = 0, j = nodes.size()-1; i < nodes.size(); j = i++) {
    if ( ((nodes[i].lat>lat) != (nodes[j].lat>lat)) &&
        (lon < (nodes[j].lon-nodes[i].lon) * (lat-nodes[i].lat) / (nodes[j].lat-nodes[i].lat) + nodes[i].lon) )
    c = !c;
  }

  return c;
}

bool IsPointInArea(double lon, double lat, const std::vector<Node>& nodes)
{
  int  i,j;
  bool c = false;

  for (i = 0, j = nodes.size()-1; i < nodes.size(); j = i++) {
    if ( ((nodes[i].lat>lat) != (nodes[j].lat>lat)) &&
        (lon < (nodes[j].lon-nodes[i].lon) * (lat-nodes[i].lat) / (nodes[j].lat-nodes[i].lat) + nodes[i].lon) )
    c = !c;
  }

  return c;
}

void AddOrUpdateUrban(std::map<Id,Urban>& urbans,
                      Id urbanId,
                      Id cityId,
                      RefType refType,
                      const std::string& name,
                      const std::vector<RawNode>& ns)
{
  std::map<Id,Urban>::iterator iter=urbans.find(urbanId);

  City city;

  city.id=cityId;
  city.refType=refType;
  city.urbanId=urbanId;
  city.name=name;

  if (iter!=urbans.end()) {
    iter->second.cities.push_back(city);
  }
  else {
    Urban urban;

    urban.id=urbanId;
    urban.area.resize(ns.size());

    for (size_t i=0; i<ns.size(); i++) {
      urban.area[i].id=ns[i].id;
      urban.area[i].lon=ns[i].lon;
      urban.area[i].lat=ns[i].lat;
    }

    urban.cities.push_back(city);

    urbans[urbanId]=urban;
  }
}

/*
bool FindDataForCityStreet(const std::map<Id,Urban>& urbans,
                           const std::string& cityName,
                           const std::string& streetName,
                           City& c,
                           std::list<Id>& ways,
                           std::list<Id>& areas)
{
  for (std::map<Id,Urban>::const_iterator urban=urbans.begin();
       urban!=urbans.end();
       ++urban) {
    for (std::list<City>::const_iterator city=urban->second.cities.begin();
         city!=urban->second.cities.end();
         ++city) {
      if (city->name==cityName) {
        std::map<std::string,std::list<Id> >::const_iterator street;

        street=urban->second.ways.find(streetName);

        if (street!=urban->second.ways.end()) {
          std::cout << city->name << ", " << street->first << ":" << std::endl;
          for (std::list<Id>::const_iterator id=street->second.begin();
               id!=street->second.end();
               ++id) {
            std::cout << "  Way: " << *id << std::endl;
          }

          c=*city;
          ways=street->second;
          areas.clear(); // TODO

          return true;
        }
      }
    }
  }

  return false;
}*/

bool GenerateCityStreetIndex(const TypeConfig& typeConfig,
                             const ImportParameter& parameter,
                             Progress& progress)
{
  std::set<TypeId>          cityIds;
  TypeId                    boundaryId=typeIgnore;
  TypeId                    typeId;
  FileScanner               scanner;
  std::map<Id,Urban>        urbans;
  std::list<RawNode>        cityNodes;
  std::list<RawWay>         cityAreas;
  std::list<RawWay>         boundaryAreas;
  std::list<RawRelation>    boundaryRelations;
  std::set<Id>              ids;
  std::map<Id,RawNode>      nodes;
  std::map<Id,RawWay>       ways;

  typeId=typeConfig.GetNodeTypeId(tagPlace,"city");
  assert(typeId!=typeIgnore);
  cityIds.insert(typeId);

  typeId=typeConfig.GetNodeTypeId(tagPlace,"town");
  assert(typeId!=typeIgnore);
  cityIds.insert(typeId);

  typeId=typeConfig.GetNodeTypeId(tagPlace,"village");
  assert(typeId!=typeIgnore);
  cityIds.insert(typeId);

  typeId=typeConfig.GetNodeTypeId(tagPlace,"hamlet");
  assert(typeId!=typeIgnore);
  cityIds.insert(typeId);

  boundaryId=typeConfig.GetAreaTypeId(tagBoundary,"administrative");
  assert(boundaryId!=typeIgnore);

  progress.SetAction("Scanning for cities of type 'node'");

  //
  // Getting all nodes of type place=*. We later need an area for these cities.
  //

  if (!scanner.Open("rawnodes.dat")) {
    progress.Error("Cannot open 'rawnodes.dat'");
    return false;
  }

  while (!scanner.HasError()) {
    RawNode node;

    node.Read(scanner);

    if (!scanner.HasError()) {

      if (cityIds.find(node.type)!=cityIds.end()) {
        std::string name;

        for (size_t i=0; i<node.tags.size(); i++) {
          if (node.tags[i].key==tagPlaceName) {
            name=node.tags[i].value;
            break;
          }
          else if (node.tags[i].key==tagName && name.empty()) {
            name=node.tags[i].value;
          }
        }

        if (!name.empty()) {
          //std::cout << "Found node of type city: " << node.id << " " << name << std::endl;
          cityNodes.push_back(node);
        }
        else {
          progress.Warning(std::string("node ")+NumberToString(node.id)+" has no name, skipping");
        }
      }
    }
  }

  scanner.Close();

  progress.Info(std::string("Found ")+NumberToString(cityNodes.size())+" cities of type 'node'");

  //
  // Getting all areas of type place=*.
  //

  progress.SetAction("Scanning for cities of type 'area'");

  if (!scanner.Open("rawways.dat")) {
    progress.Error("Cannot open 'rawways.dat'");
    return false;
  }

  while (!scanner.HasError()) {
    RawWay way;

    way.Read(scanner);

    if (!scanner.HasError()) {

      if (way.IsArea() && cityIds.find(way.type)!=cityIds.end()) {
        std::string name;

        for (size_t i=0; i<way.tags.size(); i++) {
          if (way.tags[i].key==tagPlaceName) {
            name=way.tags[i].value;
            break;
          }
          else if (way.tags[i].key==tagName && name.empty()) {
            name=way.tags[i].value;
          }
        }

        if (!name.empty()) {
          //std::cout << "Found area of type city: " << way.id << " " << name << std::endl;

          cityAreas.push_back(way);
        }
        else {
          progress.Warning(std::string("area ")+NumberToString(way.id)+" has no name, skipping");
        }
      }
    }
  }

  scanner.Close();

  GetNodeIdsFromAreas(cityAreas,ids);

  if (!GetNodesFromNodeIds(ids,nodes,progress)) {
    return false;
  }

  RemoveUnresolvedAreas(cityAreas,nodes,progress);

  progress.Info(std::string("Found ")+NumberToString(cityAreas.size())+" cities of type 'area'");

  //
  // Directly convert all city areas to cities.
  //

  for (std::list<RawWay>::const_iterator area=cityAreas.begin();
       area!=cityAreas.end();
       ++area) {
    std::string name;

    if (name.empty()) {
      for (size_t i=0; i<area->tags.size(); i++) {
        if (area->tags[i].key==tagPlaceName) {
          name=area->tags[i].value;
          break;
        }
        else if (area->tags[i].key==tagName) {
          name=area->tags[i].value;
        }
      }
    }

    if (name.empty()) {
      progress.Warning(std::string("City of type 'area' and id ")+NumberToString(area->id)+" has no name, skipping");
      continue;
    }

    std::vector<RawNode> ns;

    ns.resize(area->nodes.size());
    for (size_t i=0; i<area->nodes.size(); i++) {
      std::map<Id,RawNode>::const_iterator node=nodes.find(area->nodes[i]);

      ns[i].id=node->second.id;
      ns[i].lon=node->second.lon;
      ns[i].lat=node->second.lat;
    }

    AddOrUpdateUrban(urbans,area->id,area->id,refArea,name,ns);
  }

  //
  // Getting all areas of type 'administrative boundary' of level 6 and 8. We use them to later
  // match urbans of type node against them to get an area for a city.
  //

  progress.SetAction("Scanning for city boundaries of type 'area'");

  if (!scanner.Open("rawways.dat")) {
    progress.Error("Cannot open 'rawways.dat'");
    return false;
  }

  while (!scanner.HasError()) {
    RawWay way;

    way.Read(scanner);

    if (!scanner.HasError()) {
      if (way.IsArea() && way.type==boundaryId) {
        size_t level=0;

        for (size_t i=0; i<way.tags.size(); i++) {
          if (way.tags[i].key==tagAdminLevel && way.tags[i].value=="8") {
            level=8;
            break;
          }
          else if (way.tags[i].key==tagAdminLevel && way.tags[i].value=="6") {
            level=6;
            break;
          }
        }

        if (level!=0) {
          std::string name;

          for (size_t i=0; i<way.tags.size(); i++) {
            if (way.tags[i].key==tagName) {
              name=way.tags[i].value;
              break;
            }
          }

          if (level==8 || level==6) {
            boundaryAreas.push_back(way);
          }

          //std::cout << "Found area boundary of admin level 6 or 8: " << area.id << " " << name << std::endl;
        }
      }
    }
  }

  scanner.Close();

  //
  // Getting all relations of type 'administrative boundary' of level 6 and 8. We use them to later
  // match urbans of type node against them to get an area for a city.
  //

  progress.SetAction("Scanning for city boundaries of type 'relation'");

  if (!scanner.Open("rawrels.dat")) {
    progress.Error("Cannot open 'rawrels.dat'");
    return false;
  }

  while (!scanner.HasError()) {
    RawRelation relation;

    relation.Read(scanner);

    if (!scanner.HasError()) {

      if (relation.type==boundaryId) {
        bool match=false;

        for (size_t i=0; i<relation.tags.size(); i++) {
          if ((relation.tags[i].key==tagAdminLevel && relation.tags[i].value=="8") ||
              (relation.tags[i].key==tagAdminLevel && relation.tags[i].value=="6")) {
            match=true;
            break;
          }
        }

        if (match) {
          std::string name;

          for (size_t i=0; i<relation.tags.size(); i++) {
            if (relation.tags[i].key==tagName) {
              name=relation.tags[i].value;
              break;
            }
          }

          boundaryRelations.push_back(relation);

          //std::cout << "Found relation boundary of admin level 6 or 8: " << relation.id << " " << name << std::endl;
        }
      }
    }
  }

  scanner.Close();

  GetRelationIdsFromRelations(boundaryRelations,ids);

  if (ids.size()>0) {
    progress.Warning("(Recursivly) resolving members of type 'relation' in relations is currently not supported!");
  }
  /*
  while (ids.size()!=0) {
    std::cout << "Resolving " << ids.size() << " relation ids from relation members..." << std::endl;
    GetRelationIdsFromRelations(boundaryRelations,ids);
  }*/

  GetWayIdsFromRelations(boundaryRelations,ids);
  GetWaysFromWayIds(ids,ways,progress);
  ResolveWaysInRelations(boundaryRelations,ways,progress);

  GetNodeIdsFromAreasAndRelations(boundaryAreas,boundaryRelations,ids);

  if (!GetNodesFromNodeIds(ids,nodes,progress)) {
    return false;
  }

  RemoveUnresolvedAreas(boundaryAreas,nodes,progress);
  RemoveUnresolvedRelations(boundaryRelations,nodes,progress);

  progress.Info(std::string("Found ")+NumberToString(boundaryAreas.size())+" areas of type 'administrative boundary' and admin level 6 or 8");
  progress.Info(std::string("Found ")+NumberToString(boundaryRelations.size())+" relations of type 'administrative boundary' and admin level 6 or 8");

  // Admin level 8
  for (std::list<RawWay>::const_iterator area=boundaryAreas.begin();
       area!=boundaryAreas.end();
       ++area) {

    size_t level=0;

    for (size_t i=0; i<area->tags.size(); i++) {
      if (area->tags[i].key==tagAdminLevel && area->tags[i].value=="8") {
        level=8;
        break;
      }
      else if (area->tags[i].key==tagAdminLevel && area->tags[i].value=="6") {
        level=6;
        break;
      }
    }

    if (level==8) {
      std::vector<RawNode> ns;

      ns.resize(area->nodes.size());

      for (size_t i=0; i<area->nodes.size(); i++) {
        std::map<Id,RawNode>::const_iterator node;

        node=nodes.find(area->nodes[i]);

        assert(node!=nodes.end());

        ns[i]=node->second;
      }

      std::list<RawNode>::iterator city=cityNodes.begin();
      while (city!=cityNodes.end()) {
        if (IsPointInArea(city->lon,city->lat,ns)) {

          std::string name;

          for (size_t i=0; i<city->tags.size(); i++) {
            if (city->tags[i].key==tagName) {
              name=city->tags[i].value;
              break;
            }
          }

          progress.Info(std::string("Found area of city ")+NumberToString(city->id)+" "+name+" => "+NumberToString(area->id));

          AddOrUpdateUrban(urbans,area->id,city->id,refNode,name,ns);

          city=cityNodes.erase(city);
        }
        else {
          ++city;
        }
      }
    }
  }

  // Admin level 8
  for (std::list<RawRelation>::const_iterator rel=boundaryRelations.begin();
       rel!=boundaryRelations.end();
       ++rel) {

    size_t level=0;

    for (size_t i=0; i<rel->tags.size(); i++) {
      if (rel->tags[i].key==tagAdminLevel && rel->tags[i].value=="8") {
        level=8;
        break;
      }
      else if (rel->tags[i].key==tagAdminLevel && rel->tags[i].value=="6") {
        level=6;
        break;
      }
    }

    if (level==8) {
      std::vector<RawNode> ns;

      ns.resize(rel->members.size());

      for (size_t i=0; i<rel->members.size(); i++) {
        std::map<Id,RawNode>::const_iterator node;

        node=nodes.find(rel->members[i].id);

        assert(node!=nodes.end());

        ns[i]=node->second;
      }

      std::list<RawNode>::iterator city=cityNodes.begin();
      while (city!=cityNodes.end()) {
        if (IsPointInArea(city->lon,city->lat,ns)) {

          std::string name;

          for (size_t i=0; i<city->tags.size(); i++) {
            if (city->tags[i].key==tagName) {
              name=city->tags[i].value;
              break;
            }
          }

          progress.Info(std::string("Found area of city ")+NumberToString(city->id)+" "+name+" => "+NumberToString(rel->id));

          AddOrUpdateUrban(urbans,rel->id,city->id,refNode,name,ns);

          city=cityNodes.erase(city);
        }
        else {
          ++city;
        }
      }
    }
  }

  // Admin level 6
  for (std::list<RawWay>::const_iterator area=boundaryAreas.begin();
       area!=boundaryAreas.end();
       ++area) {

    size_t level=0;

    for (size_t i=0; i<area->tags.size(); i++) {
      if (area->tags[i].key==tagAdminLevel && area->tags[i].value=="8") {
        level=8;
        break;
      }
      else if (area->tags[i].key==tagAdminLevel && area->tags[i].value=="6") {
        level=6;
        break;
      }
    }

    if (level==6) {
      std::vector<RawNode> ns;

      ns.resize(area->nodes.size());

      for (size_t i=0; i<area->nodes.size(); i++) {
        std::map<Id,RawNode>::const_iterator node;

        node=nodes.find(area->nodes[i]);

        assert(node!=nodes.end());

        ns[i]=node->second;
      }

      for (std::list<RawNode>::iterator city=cityNodes.begin();
           city!=cityNodes.end();
           ++city) {
        if (IsPointInArea(city->lon,city->lat,ns)) {
          std::string name;

          for (size_t i=0; i<city->tags.size(); i++) {
            if (city->tags[i].key==tagName) {
              name=city->tags[i].value;
              break;
            }
          }

          progress.Info(std::string("Found area of city ")+NumberToString(city->id)+" "+name+" => "+NumberToString(area->id));

          AddOrUpdateUrban(urbans,area->id,city->id,refNode,name,ns);

          city=cityNodes.erase(city);
        }
      }
    }
  }

  // Admin level 6
  for (std::list<RawRelation>::const_iterator rel=boundaryRelations.begin();
       rel!=boundaryRelations.end();
       ++rel) {

    size_t level=0;

    for (size_t i=0; i<rel->tags.size(); i++) {
      if (rel->tags[i].key==tagAdminLevel && rel->tags[i].value=="8") {
        level=8;
        break;
      }
      else if (rel->tags[i].key==tagAdminLevel && rel->tags[i].value=="6") {
        level=6;
        break;
      }
    }

    if (level==6) {
      std::vector<RawNode> ns;

      ns.resize(rel->members.size());

      for (size_t i=0; i<rel->members.size(); i++) {
        std::map<Id,RawNode>::const_iterator node;

        node=nodes.find(rel->members[i].id);

        assert(node!=nodes.end());

        ns[i]=node->second;
      }

      for (std::list<RawNode>::iterator city=cityNodes.begin();
           city!=cityNodes.end();
           ++city) {
        if (IsPointInArea(city->lon,city->lat,ns)) {

          std::string name;

          for (size_t i=0; i<city->tags.size(); i++) {
            if (city->tags[i].key==tagName) {
              name=city->tags[i].value;
              break;
            }
          }

          progress.Info(std::string("Found area of city ")+NumberToString(city->id)+" "+name+" => "+NumberToString(rel->id));

          AddOrUpdateUrban(urbans,rel->id,city->id,refNode,name,ns);

          city=cityNodes.erase(city);
        }
      }
    }
  }

  //
  // Now start to build up our list of urbans...
  //

  progress.Info(std::string("Found ")+NumberToString(urbans.size())+" city areas");

  progress.SetAction("Delete temporary data");

  cityNodes.clear();
  cityAreas.clear();
  boundaryAreas.clear();
  boundaryRelations.clear();
  ids.clear();
  nodes.clear();
  ways.clear();

  progress.SetAction("Calculating bounds of city areas");

  for (std::map<Id,Urban>::iterator urban=urbans.begin();
       urban!=urbans.end();
       ++urban) {
    urban->second.minlon=urban->second.area[0].lon;
    urban->second.maxlon=urban->second.area[0].lon;

    urban->second.minlat=urban->second.area[0].lat;
    urban->second.maxlat=urban->second.area[0].lat;

    for (size_t i=1; i<urban->second.area.size(); i++) {
      urban->second.minlon=std::min(urban->second.minlon,urban->second.area[i].lon);
      urban->second.maxlon=std::max(urban->second.maxlon,urban->second.area[i].lon);

      urban->second.minlat=std::min(urban->second.minlat,urban->second.area[i].lat);
      urban->second.maxlat=std::max(urban->second.maxlat,urban->second.area[i].lat);
    }
  }

  progress.SetAction("Getting ways with name in found cities");

  std::set<TypeId> wayTypes;

  typeConfig.GetWaysWithKey(tagHighway,wayTypes);

  /*
  for (std::set<TypeId>::const_iterator type=wayTypes.begin();
       type!=wayTypes.end();
       ++type) {
    std::cout << "  Way type: " << *type << std::endl;
  }*/

  // We currently scan Ways instead of RawWays because for Ways we have the nodes already
  // resolved (later we have to rethink this, but why should we index ways that are not
  // part fo the database (...but should we index urbans that are not in the database?)?)

  if (!scanner.Open("ways.dat")) {
    progress.Error("Cannot open 'ways.dat'");
    return false;
  }

  while (!scanner.HasError()) {
    Way way;

    way.Read(scanner);

    if (!scanner.HasError()) {
      if (wayTypes.find(way.type)!=wayTypes.end()) {

        std::string name=way.GetName();

        if (!name.empty()) {
          double minlon=way.nodes[0].lon;
          double maxlon=way.nodes[0].lon;

          double minlat=way.nodes[0].lat;
          double maxlat=way.nodes[0].lat;

          for (size_t n=0; n<way.nodes.size(); n++) {
            minlon=std::min(minlon,way.nodes[n].lon);
            maxlon=std::max(maxlon,way.nodes[n].lon);

            minlat=std::min(minlat,way.nodes[n].lat);
            maxlat=std::max(maxlat,way.nodes[n].lat);
          }

          //std::cout << "Analysing way " << name << std::endl;

          for (std::map<Id,Urban>::iterator urban=urbans.begin();
               urban!=urbans.end();
               ++urban) {
            if (!(maxlon<urban->second.minlon) &&
                !(minlon>urban->second.maxlon) &&
                !(maxlat<urban->second.minlat) &&
                !(minlat>urban->second.maxlat)) {
              bool match=false;

              for (size_t n=0; n<way.nodes.size(); n++) {
                if (IsPointInArea(way.nodes[n].lon,way.nodes[n].lat,urban->second.area)) {
                  match=true;
                  break;
                }
              }

              if (match) {
                //std::cout << "Way " << way.id << " is in city " << city->second.id << std::endl;

                urban->second.ways[name].push_back(way.id);
                break;
              }
            }
          }
        }
      }
    }
  }

  scanner.Close();

  progress.SetAction("Generating 'citysteet.idx'");

  FileWriter writer;

  if (!writer.Open("citystreet.idx")) {
    progress.Error("Cannot open 'citystreet.idx'");
    return false;
  }

  //std::cout << "Storing cities..." << std::endl;

  std::map<std::string,City> cities;

  for (std::map<Id,Urban>::const_iterator urban=urbans.begin();
       urban!=urbans.end();
       ++urban) {
    for (std::list<City>::const_iterator city=urban->second.cities.begin();
         city!=urban->second.cities.end();
         ++city) {
      cities[city->name]=*city;
    }
  }

  size_t cityEntries=cities.size();

  writer.WriteNumber(cityEntries); // Number of cities

  for (std::map<std::string,City>::const_iterator city=cities.begin();
       city!=cities.end();
       ++city) {
    unsigned char refType=city->second.refType;

    writer.Write(city->second.id);      // Id of city
    writer.WriteNumber(refType);        // Type of id
    writer.Write(city->second.urbanId); // Id of the urban area this icty belongs to
    writer.Write(city->first);          // city name
  }

  cities.clear();

  //std::cout << "Storing urbans..." << std::endl;


  writer.WriteNumber(urbans.size()); // Number of urbans

  long urbanEntriesOffset;

  writer.GetPos(urbanEntriesOffset);

  for (std::map<Id,Urban>::const_iterator urban=urbans.begin();
       urban!=urbans.end();
       ++urban) {
    size_t offset=0;

    writer.Write(urban->first); // Id of urban
    writer.Write(offset);       // Offset of urban data
  }

  size_t index=0;

  for (std::map<Id,Urban>::const_iterator urban=urbans.begin();
       urban!=urbans.end();
       ++urban) {
    long offset;

    writer.GetPos(offset);

    writer.SetPos(urbanEntriesOffset+index*(sizeof(Id)+sizeof(unsigned long))+sizeof(Id));
    writer.Write((unsigned long)offset); // Offset to urban data
    writer.SetPos(offset);

    writer.WriteNumber(urban->second.ways.size());  // Number of ways in urban
    writer.WriteNumber(urban->second.areas.size()); // Number of areas in urban

    for (std::map<std::string,std::list<Id> >::const_iterator way=urban->second.ways.begin();
         way!=urban->second.ways.end();
         ++way) {
      writer.Write(way->first);               // Way name
      writer.WriteNumber(way->second.size()); // Number of ids

      for (std::list<Id>::const_iterator id=way->second.begin();
           id!=way->second.end();
           ++id) {
        writer.Write(*id); // Id of way
      }
    }

    for (std::map<std::string,std::list<Id> >::const_iterator area=urban->second.areas.begin();
         area!=urban->second.areas.end();
         ++area) {
      writer.Write(area->first);               // Area name
      writer.WriteNumber(area->second.size()); // Number of ids

      for (std::list<Id>::const_iterator id=area->second.begin();
           id!=area->second.end();
           ++id) {
        writer.Write(*id); // Id of area
      }
    }

    index++;
  }

  return !writer.HasError() && writer.Close();
}

