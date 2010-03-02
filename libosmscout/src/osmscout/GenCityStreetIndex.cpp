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

#include <osmscout/GenCityStreetIndex.h>

#include <cassert>
#include <cmath>
#include <limits>
#include <list>
#include <map>
#include <set>

#include <osmscout/FileWriter.h>
#include <osmscout/RawNode.h>
#include <osmscout/RawRelation.h>
#include <osmscout/RawWay.h>

#include <osmscout/Node.h>
#include <osmscout/Point.h>
#include <osmscout/Reference.h>
#include <osmscout/Way.h>

#include <osmscout/Util.h>
#include <iostream>

namespace osmscout {

  /**
    A location within an area
    */
  struct Location
  {
    Reference              reference;
    std::string            name;
  };

  struct LocationRef
  {
    FileOffset             offset;
    Reference              reference;
  };

  /**
    An area
    */
  struct Area
  {
    FileOffset                           offset;    //! Offset into the index file
    Reference                            reference; //! The id for this area
    std::string                          name;      //! The name of this area
    std::list<Location>                  locations; //! Location that are represented by this area
    std::vector<Point>                   area;      //! the geometric area of this area

    double                               minlon;
    double                               minlat;
    double                               maxlon;
    double                               maxlat;

    std::map<std::string,std::list<Id> > nodes;     //! list of indexed nodes in this area
    std::map<std::string,std::list<Id> > ways;      //! list of indexed ways in this area

    std::list<Area>                      areas;     //! A list of sub areas
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

  /**
    Return the consolidated and unique list of node ids from the given list of
    ways/areas.
    */
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

  /**
    Remove all areas from the given list of areas, where we do not have
    a RawNode for one (or multiple) of the node ids that will up the area.
    */
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
      std::string       name;

      // Get name for debugging
      for (std::vector<Tag>::iterator tag=rel->tags.begin();
           tag!=rel->tags.end();
           ++tag) {
        if (tag->key==tagName) {
          name=tag->value;
          break;
        }
      }

      // Find out, if we just take every memebr rin account or only member
      // of the role outer...

      std::string role;

      for (std::vector<RawRelation::Member>::iterator member=rel->members.begin();
           member!=rel->members.end();
           ++member) {
        if (member->type==RawRelation::memberWay) {
          if (member->role=="inner" || member->role=="outer") {
            role="outer";
            break;
          }
        }
      }

      for (std::vector<RawRelation::Member>::iterator member=rel->members.begin();
           member!=rel->members.end();
           ++member) {
        if (member->type==RawRelation::memberWay) {
          if (member->role==role) {
            std::map<Id,RawWay>::const_iterator way=ways.find(member->id);

            if (way!=ways.end()) {
              w.push_back(way->second);
            }
            else {
              progress.Warning(std::string("Cannot resolve way ")+NumberToString(member->id)+" in relation "+NumberToString(rel->id)+" "+name);
              convertable=false;
            }
          }
        }
        /*
        else {
          progress.Warning(std::string("Member in relation ")+NumberToString(rel->id)+" "+name+" is not of type way");
          convertable=false;
        }*/
      }

      if (!convertable || w.size()==0) {
        progress.Warning(std::string("Skipping relation ")+NumberToString(rel->id)+" "+name+", because it does not only consist of ways or no ways at all");
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
        progress.Warning(std::string("Cannot resolve ways in Relation ")+NumberToString(rel->id)+" "+name+" to build an area boundary, skipping");
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
        progress.Warning(std::string("Cannot resolve ways in Relation ")+NumberToString(rel->id)+" "+name+" to build an area boundary, skipping");
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

  /**
    Load all nodes for the given list of node ids. Ignore ids we cannot load a RawNode
    for.

    Returns false, if the wass a file io error.
    */
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

  /**
    Load all ways for the given list of way ids. Ignore ids we cannot load a RawWay
    for.

    Returns false, if the was a file io error.
    */
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

  template<typename N, typename M>
  double isLeft(const N& p0, const N& p1, const M& p2)
  {
    if (p2.id==p0.id || p2.id==p1.id) {
      return 0;
    }

    return ((p1.lon-p0.lon)*(p2.lat-p0.lat)-(p2.lon-p0.lon)*(p1.lat-p0.lat));
  }

  template<typename N, typename M>
  bool IsPointInArea(const N& node,
                     const std::vector<M>& nodes)
  {
    for (int i=0; i<nodes.size()-1; i++) {
      if (node.id==nodes[i].id) {
        return true;
      }
    }

    int wn=0;    // the winding number counter

    // loop through all edges of the polygon
    for (int i=0; i<nodes.size()-1; i++) {   // edge from V[i] to V[i+1]
      if (nodes[i].lat <= node.lat) {         // start y <= P.y
        if (nodes[i+1].lat > node.lat) {     // an upward crossing
          if (isLeft( nodes[i], nodes[i+1], node) > 0) { // P left of edge
            ++wn;            // have a valid up intersect
          }
        }
      }
      else {                       // start y > P.y (no test needed)
        if (nodes[i+1].lat <= node.lat) {    // a downward crossing
          if (isLeft( nodes[i], nodes[i+1], node) < 0) { // P right of edge
            --wn;            // have a valid down intersect
          }
        }
      }
    }

    return wn!=0;
  }

  /**
    Returns true, if point in area.
    */
  template<typename N>
  bool IsPointInArea(double lon, double lat,
                     const std::vector<N>& nodes)
  {
    int  i,j;
    bool c=false;

    for (i=0, j=nodes.size()-1; i<nodes.size(); j=i++) {
      if (((nodes[i].lat>lat) != (nodes[j].lat>lat)) &&
          (lon<(nodes[j].lon-nodes[i].lon)*(lat-nodes[i].lat) /
           (nodes[j].lat-nodes[i].lat)+nodes[i].lon))  {
        c=!c;
      }
    }

    return c;
  }

  template<typename N,typename M>
  bool IsAreaInArea(const std::vector<N>& a,
                    const std::vector<M>& b)
  {
    for (typename std::vector<N>::const_iterator i=a.begin(); i!=a.end(); i++) {
      if (!IsPointInArea(*i,b)) {
        return false;
      }
    }

    return true;
  }

  /**
    Return the list of nodes ids with the given type.
    */
  static bool GetCityNodes(const std::set<TypeId>& cityIds,
                           std::list<RawNode>& cityNodes,
                           Progress& progress)
                           {
    FileScanner scanner;

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
            else if (node.tags[i].key==tagName &&
                     name.empty()) {
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

    return true;
  }

  /**
    Return the list of nodes ids with the given type.
    */
  static bool GetCityAreas(const std::set<TypeId>& cityIds,
                           std::list<RawWay>& cityAreas,
                           Progress& progress)
  {
      FileScanner scanner;

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

    return true;
  }

  static void AddArea(Area& parent, const Area& area)
  {
    for (std::list<Area>::iterator a=parent.areas.begin();
         a!=parent.areas.end();
         a++) {
      if (IsAreaInArea(area.area,a->area)) {
        // If we already have the same name and are a "minor" reference, we skip...
        if (!(area.name==a->name &&
              area.reference.type<a->reference.type)) {
          AddArea(*a,area);
        }
        return;
      }
    }

    parent.areas.push_back(area);
  }

  static void AddLocationToArea(Area& area,
                                const Location& location,
                                const Point& node)
                                {
    if (area.name==location.name) {
      return;
    }

    for (std::list<Area>::iterator a=area.areas.begin();
         a!=area.areas.end();
         a++) {
      if (IsPointInArea(node,a->area)) {
        AddLocationToArea(*a,location,node);
        return;
      }
    }

    area.locations.push_back(location);
  }

  static void AddWayToArea(Area& area,
                           const Way& way,
                           double minlon,
                           double minlat,
                           double maxlon,
                           double maxlat)
  {
    bool inserted=false;

    for (std::list<Area>::iterator a=area.areas.begin();
         a!=area.areas.end();
         a++) {
      if (!(maxlon<a->minlon) &&
          !(minlon>a->maxlon) &&
          !(maxlat<a->minlat) &&
          !(minlat>a->maxlat)) {
        bool match=false;

        for (size_t n=0; n<way.nodes.size(); n++) {
          if (IsPointInArea(way.nodes[n].lon,way.nodes[n].lat,a->area)) {
            match=true;
            break;
          }
        }

        if (match) {
          AddWayToArea(*a,way,minlon,minlat,maxlon,maxlat);
          inserted=true;
        }
      }
    }

    if (!inserted) {
      area.ways[way.name].push_back(way.id);
    }
  }

  static void AddNodeToArea(Area& area,
                            const Node& node,
                            const std::string& name)
  {
    for (std::list<Area>::iterator a=area.areas.begin();
         a!=area.areas.end();
         a++) {
      if (IsPointInArea(node,a->area)) {
        AddNodeToArea(*a,node,name);
        return;
      }
    }

    area.nodes[name].push_back(node.id);
  }

  static void DumpArea(const Area& parent, size_t indent)
  {
    for (std::list<Area>::const_iterator a=parent.areas.begin();
         a!=parent.areas.end();
         a++) {
      for (size_t i=0; i<indent; i++) {
        std::cout << " ";
      }
      std::cout << a->name << std::endl;

      for (std::list<Location>::const_iterator l=a->locations.begin();
           l!=a->locations.end();
           l++) {
        for (size_t i=0; i<indent; i++) {
          std::cout << " ";
        }
        std::cout << " =" << l->name << std::endl;
      }

      DumpArea(*a,indent+2);
    }
  }

  static void CalculateAreaBounds(Area& parent)
  {
    for (std::list<Area>::iterator area=parent.areas.begin();
         area!=parent.areas.end();
         ++area) {

      area->minlon=area->area[0].lon;
      area->maxlon=area->area[0].lon;
      area->minlat=area->area[0].lat;
      area->maxlat=area->area[0].lat;

      for (size_t i=1; i<area->area.size(); i++) {
        area->minlon=std::min(area->minlon,area->area[i].lon);
        area->maxlon=std::max(area->maxlon,area->area[i].lon);
        area->minlat=std::min(area->minlat,area->area[i].lat);
        area->maxlat=std::max(area->maxlat,area->area[i].lat);
      }

      CalculateAreaBounds(*area);
    }
  }

  static bool WriteArea(FileWriter& writer,
                        Area& area, FileOffset parentOffset)
  {
    writer.GetPos(area.offset);

    writer.Write(area.name);
    writer.WriteNumber(parentOffset);

    writer.WriteNumber(area.areas.size());
    for (std::list<Area>::iterator a=area.areas.begin();
         a!=area.areas.end();
         a++) {
      if (!WriteArea(writer,*a,area.offset)) {
        return false;
      }
    }

    writer.WriteNumber(area.nodes.size());
    for (std::map<std::string,std::list<Id> >::const_iterator node=area.nodes.begin();
         node!=area.nodes.end();
         ++node) {
      Id lastId=0;

      writer.Write(node->first);               // Node name
      writer.WriteNumber(node->second.size()); // Number of ids

      for (std::list<Id>::const_iterator id=node->second.begin();
             id!=node->second.end();
             ++id) {
        writer.WriteNumber(*id-lastId); // Id of node
        lastId=*id;
      }
    }

    writer.WriteNumber(area.ways.size());
    for (std::map<std::string,std::list<Id> >::const_iterator way=area.ways.begin();
         way!=area.ways.end();
         ++way) {
      Id lastId=0;

      writer.Write(way->first);               // Way name
      writer.WriteNumber(way->second.size()); // Number of ids

      for (std::list<Id>::const_iterator id=way->second.begin();
           id!=way->second.end();
           ++id) {
        writer.WriteNumber(*id-lastId); // Id of way
        lastId=*id;
      }
    }

    return !writer.HasError();
  }

  static bool WriteAreas(FileWriter& writer,
                         Area& root)
  {
    for (std::list<Area>::iterator a=root.areas.begin();
         a!=root.areas.end();
         ++a) {
      if (!WriteArea(writer,*a,0)) {
        return false;
      }
    }

    return true;
  }

  static void GetLocationRefs(const Area& area,
                              std::map<std::string,std::list<LocationRef> >& locationRefs)
  {
    LocationRef locRef;

    locRef.offset=area.offset;
    locRef.reference=area.reference;

    locationRefs[area.name].push_back(locRef);

    for (std::list<Location>::const_iterator l=area.locations.begin();
         l!=area.locations.end();
         ++l) {
      locRef.offset=area.offset;
      locRef.reference=l->reference;

      locationRefs[l->name].push_back(locRef);
    }

    for (std::list<Area>::const_iterator a=area.areas.begin();
         a!=area.areas.end();
         a++) {
      GetLocationRefs(*a,locationRefs);
    }
  }

  static bool WriteLocationRefs(FileWriter& writer,
                                const std::map<std::string,std::list<LocationRef> >& locationRefs)
  {
    writer.WriteNumber(locationRefs.size());

    for (std::map<std::string,std::list<LocationRef> >::const_iterator n=locationRefs.begin();
         n!=locationRefs.end();
         ++n) {
      if (!writer.Write(n->first)) {
        return false;
      }

      if (!writer.WriteNumber(n->second.size())) {
        return false;
      }

      for (std::list<LocationRef>::const_iterator o=n->second.begin();
           o!=n->second.end();
           ++o) {
        if (!writer.WriteNumber(o->reference.type)) {
          return false;
        }

        if (!writer.WriteNumber(o->reference.id)) {
          return false;
        }

        if (!writer.WriteNumber(o->offset)) {
          return false;
        }
      }
    }

    return true;
  }

  bool GenerateCityStreetIndex(const TypeConfig& typeConfig,
                               const ImportParameter& parameter,
                               Progress& progress)
  {
    std::set<TypeId>          cityIds;
    TypeId                    boundaryId;
    TypeId                    typeId;
    FileScanner               scanner;
    Area                      rootArea;
    std::list<RawNode>        cityNodes;
    std::list<RawWay>         cityAreas;
    std::list<RawWay>         boundaryAreas;
    std::list<RawRelation>    boundaryRelations;
    std::set<Id>              ids;
    std::map<Id,RawNode>      nodes;
    std::map<Id,RawWay>       ways;

    rootArea.name="<root>";
    rootArea.offset=0;

    // We ignore (besides strange ones ;-)):
    // continent
    // country
    // county
    // island
    // quarter
    // region
    // square
    // state

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

    typeId=typeConfig.GetNodeTypeId(tagPlace,"suburb");
    assert(typeId!=typeIgnore);
    cityIds.insert(typeId);

    boundaryId=typeConfig.GetAreaTypeId(tagBoundary,"administrative");
    assert(boundaryId!=typeIgnore);

    progress.SetAction("Scanning for cities of type 'node'");

    //
    // Getting all nodes of type place=*. We later need an area for these cities.
    //

    // Get nodes of one of the types in cityIds
    if (!GetCityNodes(cityIds,cityNodes,progress)) {
      return false;
    }

    progress.Info(std::string("Found ")+NumberToString(cityNodes.size())+" cities of type 'node'");

    //
    // Getting all areas of type place=*.
    //

    progress.SetAction("Scanning for cities of type 'area'");

    // Get areas of one of the types in cityIds
    if (!GetCityAreas(cityIds,cityAreas,progress)) {
      return false;
    }

    // Get all node ids for all areas in list
    GetNodeIdsFromAreas(cityAreas,ids);

    // No load all RawNodes based on the list of node ids
    if (!GetNodesFromNodeIds(ids,nodes,progress)) {
      return false;
    }

    // Now remove all areas where we do not have all nodes to build up their area
    RemoveUnresolvedAreas(cityAreas,nodes,progress);

    progress.Info(std::string("Found ")+NumberToString(cityAreas.size())+" cities of type 'area'");

    //
    // Getting all areas of type 'administrative boundary'.
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
            if (way.tags[i].key==tagAdminLevel &&
              sscanf(way.tags[i].value.c_str(),"%d",&level)==1) {
              boundaryAreas.push_back(way);
              break;
            }
          }

        }
      }
    }

    scanner.Close();

    //
    // Getting all relations of type 'administrative boundary'.
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
          size_t level=0;

          for (size_t i=0; i<relation.tags.size(); i++) {
            if (relation.tags[i].key==tagAdminLevel &&
              sscanf(relation.tags[i].value.c_str(),"%d",&level)==1) {
              boundaryRelations.push_back(relation);
              break;
            }
          }
        }
      }
    }

    scanner.Close();

    progress.Info(std::string("Found ")+NumberToString(boundaryAreas.size())+" areas of type 'administrative boundary'");
    progress.Info(std::string("Found ")+NumberToString(boundaryRelations.size())+" relations of type 'administrative boundary'");

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

    progress.Info(std::string("Resolved ")+NumberToString(boundaryAreas.size())+" areas of type 'administrative boundary'");
    progress.Info(std::string("Resolved ")+NumberToString(boundaryRelations.size())+" relations of type 'administrative boundary'");

    progress.SetAction("Inserting boundary relations and areas into area tree");

    for (size_t l=1; l<=10; l++) {
      for (std::list<RawRelation>::const_iterator rel=boundaryRelations.begin();
           rel!=boundaryRelations.end();
           ++rel) {

        size_t      level=0;
        std::string name;

        for (size_t i=0; i<rel->tags.size() && !(l!=0 && !name.empty()); i++) {
          if (rel->tags[i].key==tagAdminLevel &&
            sscanf(rel->tags[i].value.c_str(),"%d",&level)==1) {
          }
          else if (rel->tags[i].key==tagName) {
            name=rel->tags[i].value;
          }
        }

        if (level==l && !name.empty()) {
          std::vector<Point> ns;

          ns.resize(rel->members.size());
          for (size_t i=0; i<rel->members.size(); i++) {
            assert(rel->members[i].type==RawRelation::memberNode);
            std::map<Id,RawNode>::const_iterator node=nodes.find(rel->members[i].id);

            ns[i].id=node->second.id;
            ns[i].lon=node->second.lon;
            ns[i].lat=node->second.lat;
          }

          Area area;

          area.reference.Set(rel->id,refRelation);
          area.name=name;
          area.area=ns;

          AddArea(rootArea,area);
        }
      }

      for (std::list<RawWay>::const_iterator a=boundaryAreas.begin();
           a!=boundaryAreas.end();
           ++a) {

        size_t      level=0;
        std::string name;

        for (size_t i=0; i<a->tags.size() && !(l!=0 && !name.empty()); i++) {
          if (a->tags[i].key==tagAdminLevel &&
            sscanf(a->tags[i].value.c_str(),"%d",&level)==1) {
          }
          else if (a->tags[i].key==tagName) {
            name=a->tags[i].value;
          }
        }

        if (level==l && !name.empty()) {
          std::vector<Point> ns;

          ns.resize(a->nodes.size());
          for (size_t i=0; i<a->nodes.size(); i++) {
            std::map<Id,RawNode>::const_iterator node=nodes.find(a->nodes[i]);

            ns[i].id=node->second.id;
            ns[i].lon=node->second.lon;
            ns[i].lat=node->second.lat;
          }

          Area area;

          area.reference.Set(a->id,refWay);
          area.name=name;
          area.area=ns;

          AddArea(rootArea,area);
        }
      }
    }

    progress.SetAction("Inserting cities of type area into area tree");

    for (std::list<RawWay>::const_iterator a=cityAreas.begin();
         a!=cityAreas.end();
         ++a) {
      std::string name;

      for (size_t i=0; i<a->tags.size(); i++) {
        if (a->tags[i].key==tagPlaceName) {
          name=a->tags[i].value;
          break;
        }
        else if (a->tags[i].key==tagName) {
          name=a->tags[i].value;
        }
      }

      if (name.empty()) {
        progress.Warning(std::string("City of type 'area' and id ")+NumberToString(a->id)+" has no name, skipping");
        continue;
      }

      std::vector<Point> ns;

      ns.resize(a->nodes.size());
      for (size_t i=0; i<a->nodes.size(); i++) {
        std::map<Id,RawNode>::const_iterator node=nodes.find(a->nodes[i]);

        ns[i].id=node->second.id;
        ns[i].lon=node->second.lon;
        ns[i].lat=node->second.lat;
      }

      Area area;

      area.reference.Set(a->id,refWay);
      area.name=name;
      area.area=ns;

      AddArea(rootArea,area);
    }

    progress.SetAction("Inserting cities of type node into area tree");

    size_t count=0;
    for (std::list<RawNode>::iterator city=cityNodes.begin();
         city!=cityNodes.end();
         ++city) {
      count++;
      std::string name;

      for (size_t i=0; i<city->tags.size(); i++) {
        if (city->tags[i].key==tagName) {
          name=city->tags[i].value;
          break;
        }
      }

      if (name.empty()) {
        progress.Warning(std::string("City of type 'node' and id ")+NumberToString(city->id)+" has no name, skipping");
        continue;
      }

      Location location;

      location.reference.Set(city->id,refNode);
      location.name=name;

      Point node;

      node.id=city->id;
      node.lon=city->lon;
      node.lat=city->lat;

      AddLocationToArea(rootArea,location,node);

      if (count%1000==0) {
        std::cout << count << "/" << cityNodes.size() << std::endl;
      }
    }

    /*
    progress.SetAction("Dumping areas");

    DumpArea(rootArea,0);*/

    progress.SetAction("Delete temporary data");

    cityNodes.clear();
    cityAreas.clear();
    boundaryAreas.clear();
    boundaryRelations.clear();
    ids.clear();
    nodes.clear();
    ways.clear();

    progress.SetAction("Calculating bounds of areas");

    CalculateAreaBounds(rootArea);

    std::set<TypeId> indexables;

    typeConfig.GetIndexables(indexables);

    progress.SetAction("Resolve ways to areas");

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
        if (indexables.find(way.type)!=indexables.end()) {

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

            AddWayToArea(rootArea,way,minlon,minlat,maxlon,maxlat);
          }
        }
      }
    }

    scanner.Close();

    progress.SetAction("Resolve nodes to areas");

    if (!scanner.Open("nodes.dat")) {
      progress.Error("Cannot open 'nodes.dat'");
      return false;
    }

    while (!scanner.HasError()) {
      Node node;

      node.Read(scanner);

      if (!scanner.HasError()) {
        if (indexables.find(node.type)!=indexables.end()) {
          std::string name;

          for (std::vector<Tag>::iterator tag=node.tags.begin();
               tag!=node.tags.end();
               ++tag) {
            if (tag->key==tagName) {
              name=tag->value;
              break;
            }
          }

          if (!name.empty()) {
            AddNodeToArea(rootArea,node,name);
          }
        }
      }
    }

    if (!scanner.Close()) {
      return false;
    }

    FileWriter writer;

    //
    // Generate file with all areas, where areas reference parent and children by offset
    //

    progress.SetAction("Write 'region.dat'");

    if (!writer.Open("region.dat")) {
      progress.Error("Cannot open 'region.dat'");
      return false;
    }

    WriteAreas(writer,rootArea);

    if (writer.HasError() || !writer.Close()) {
      return false;
    }

    //
    // Generate file with all area names, each referencing the areas where it is contained
    //

    std::map<std::string,std::list<LocationRef> > locationRefs;

    progress.SetAction("Write 'nameregion.idx'");

    for (std::list<Area>::const_iterator a=rootArea.areas.begin();
         a!=rootArea.areas.end();
         a++) {
      GetLocationRefs(*a,locationRefs);
    }

    if (!writer.Open("nameregion.idx")) {
      progress.Error("Cannot open 'nameregion.idx'");
      return false;
    }

    WriteLocationRefs(writer,locationRefs);

    return !writer.HasError() && writer.Close();
  }
}

