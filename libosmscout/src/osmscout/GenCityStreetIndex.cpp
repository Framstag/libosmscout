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

#include <osmscout/Node.h>
#include <osmscout/Point.h>
#include <osmscout/Relation.h>
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

  /**
    Return the list of nodes ids with the given type.
    */
  static bool GetCityNodes(const std::set<TypeId>& cityIds,
                           std::list<Node>& cityNodes,
                           Progress& progress)
                           {
    FileScanner scanner;

    if (!scanner.Open("nodes.dat")) {
      progress.Error("Cannot open 'nodes.dat'");
      return false;
    }

    while (!scanner.IsEOF()) {
      Node node;

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

          if (name.empty()) {
            progress.Warning(std::string("node ")+NumberToString(node.id)+" has no name, skipping");
            continue;
          }

          //std::cout << "Found node of type city: " << node.id << " " << name << std::endl;
          cityNodes.push_back(node);
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
                           std::list<Way>& cityAreas,
                           Progress& progress)
  {
      FileScanner scanner;

    if (!scanner.Open("ways.dat")) {
      progress.Error("Cannot open 'ways.dat'");
      return false;
    }

    while (!scanner.IsEOF()) {
      Way way;

      way.Read(scanner);

      if (!scanner.HasError()) {
        if (way.IsArea() && cityIds.find(way.GetType())!=cityIds.end()) {
          std::string name=way.GetName();

          for (size_t i=0; i<way.attributes.tags.size(); i++) {
            if (way.attributes.tags[i].key==tagPlaceName) {
              name=way.attributes.tags[i].value;
              break;
            }
          }

          if (name.empty()) {
            progress.Warning(std::string("area ")+NumberToString(way.id)+" has no name, skipping");
            continue;
          }

          //std::cout << "Found area of type city: " << way.id << " " << name << std::endl;

          cityAreas.push_back(way);
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
          if (IsPointInArea(way.nodes[n],a->area)) {
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
      area.ways[way.GetName()].push_back(way.id);
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

      if (area->area.size()>0) {
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

    writer.WriteNumber((uint32_t)area.areas.size());
    for (std::list<Area>::iterator a=area.areas.begin();
         a!=area.areas.end();
         a++) {
      if (!WriteArea(writer,*a,area.offset)) {
        return false;
      }
    }

    writer.WriteNumber((uint32_t)area.nodes.size());
    for (std::map<std::string,std::list<Id> >::const_iterator node=area.nodes.begin();
         node!=area.nodes.end();
         ++node) {
      Id lastId=0;

      writer.Write(node->first);               // Node name
      writer.WriteNumber((uint32_t)node->second.size()); // Number of ids

      for (std::list<Id>::const_iterator id=node->second.begin();
             id!=node->second.end();
             ++id) {
        writer.WriteNumber(*id-lastId); // Id of node
        lastId=*id;
      }
    }

    writer.WriteNumber((uint32_t)area.ways.size());
    for (std::map<std::string,std::list<Id> >::const_iterator way=area.ways.begin();
         way!=area.ways.end();
         ++way) {
      Id lastId=0;

      writer.Write(way->first);               // Way name
      writer.WriteNumber((uint32_t)way->second.size()); // Number of ids

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
    writer.WriteNumber((uint32_t)locationRefs.size());

    for (std::map<std::string,std::list<LocationRef> >::const_iterator n=locationRefs.begin();
         n!=locationRefs.end();
         ++n) {
      if (!writer.Write(n->first)) {
        return false;
      }

      if (!writer.WriteNumber((uint32_t)n->second.size())) {
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

  std::string CityStreetIndexGenerator::GetDescription() const
  {
    return "Generate 'region.dat' and 'nameregion.idx'";
  }

  bool CityStreetIndexGenerator::Import(const ImportParameter& parameter,
                                        Progress& progress,
                                        const TypeConfig& typeConfig)
  {
    std::set<TypeId>          cityIds;
    TypeId                    boundaryId;
    TypeId                    typeId;
    FileScanner               scanner;
    Area                      rootArea;
    std::list<Node>           cityNodes;
    std::list<Way>            cityAreas;
    std::list<Way>            boundaryAreas;
    std::list<Relation>       boundaryRelations;

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

    // We do not yet know if we handle borders as ways or areas
    boundaryId=typeConfig.GetWayTypeId(tagBoundary,"administrative");
    if (boundaryId==typeIgnore) {
      boundaryId=typeConfig.GetAreaTypeId(tagBoundary,"administrative");
    }
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

    progress.Info(std::string("Found ")+NumberToString(cityAreas.size())+" cities of type 'area'");

    //
    // Getting all areas of type 'administrative boundary'.
    //

    progress.SetAction("Scanning for city boundaries of type 'area'");

    if (!scanner.Open("ways.dat")) {
      progress.Error("Cannot open 'ways.dat'");
      return false;
    }

    while (!scanner.IsEOF()) {
      Way way;

      way.Read(scanner);

      if (!scanner.HasError()) {
        if (way.IsArea() && way.GetType()==boundaryId) {
          size_t level=0;

          for (size_t i=0; i<way.attributes.tags.size(); i++) {
            if (way.attributes.tags[i].key==tagAdminLevel) {
              if (StringToNumber(way.attributes.tags[i].value,level)) {
                boundaryAreas.push_back(way);
              }
              else {
                progress.Info("Could not parse admin_level of way "+
                              NumberToString(way.GetType() )+" "+NumberToString(way.id));
              }

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

    if (!scanner.Open("relations.dat")) {
      progress.Error("Cannot open 'relations.dat'");
      return false;
    }

    while (!scanner.IsEOF()) {
      Relation relation;

      relation.Read(scanner);

      if (!scanner.HasError()) {

        if (relation.type==boundaryId) {
          size_t level=0;

          for (size_t i=0; i<relation.tags.size(); i++) {
            if (relation.tags[i].key==tagAdminLevel) {
              if (StringToNumber(relation.tags[i].value,level)) {
                boundaryRelations.push_back(relation);
              }
              else {
                progress.Info("Could not parse admin_level of relation "+relation.relType+" "+
                              NumberToString(relation.type )+" "+NumberToString(relation.id));
              }

              break;
            }
          }

          if (level==0) {
            progress.Info("No tag 'admin_level' for relation "+relation.relType+" "+
                          NumberToString(relation.type )+" "+NumberToString(relation.id));
          }
        }
      }
    }

    scanner.Close();

    progress.Info(std::string("Found ")+NumberToString(boundaryAreas.size())+" areas of type 'administrative boundary'");
    progress.Info(std::string("Found ")+NumberToString(boundaryRelations.size())+" relations of type 'administrative boundary'");

    progress.SetAction("Inserting boundary relations and areas into area tree");

    for (size_t l=1; l<=10; l++) {
      for (std::list<Relation>::const_iterator rel=boundaryRelations.begin();
           rel!=boundaryRelations.end();
           ++rel) {

        size_t      level=0;
        std::string name;

        for (size_t i=0; i<rel->tags.size() && !(l!=0 && !name.empty()); i++) {
          if (rel->tags[i].key==tagAdminLevel && StringToNumber(rel->tags[i].value,level)) {
          }
          else if (rel->tags[i].key==tagName) {
            name=rel->tags[i].value;
          }
        }

        if (level==l && !name.empty()) {
          std::vector<Point> ns;

          for (size_t i=0; i<rel->roles.size(); i++) {
            if (rel->roles[i].role=="0") {
              Area area;

              area.reference.Set(rel->id,refRelation);
              area.name=name;
              area.area=rel->roles[i].nodes;

              AddArea(rootArea,area);
            }
          }
        }
      }

      for (std::list<Way>::const_iterator a=boundaryAreas.begin();
           a!=boundaryAreas.end();
           ++a) {

        size_t      level=0;
        std::string name;

        for (size_t i=0; i<a->attributes.tags.size() && !(l!=0 && !name.empty()); i++) {
          if (a->attributes.tags[i].key==tagAdminLevel && StringToNumber(a->attributes.tags[i].value,level)) {
          }
          else if (a->attributes.tags[i].key==tagName) {
            name=a->attributes.tags[i].value;
          }
        }

        if (level==l && !name.empty()) {
          Area area;

          area.reference.Set(a->id,refWay);
          area.name=name;
          area.area=a->nodes;

          AddArea(rootArea,area);
        }
      }
    }

    progress.SetAction("Inserting cities of type area into area tree");

    for (std::list<Way>::const_iterator a=cityAreas.begin();
         a!=cityAreas.end();
         ++a) {
      std::string name=a->GetName();

      for (size_t i=0; i<a->attributes.tags.size(); i++) {
        if (a->attributes.tags[i].key==tagPlaceName) {
          name=a->attributes.tags[i].value;
          break;
        }
      }

      if (name.empty()) {
        progress.Warning(std::string("City of type 'area' and id ")+NumberToString(a->id)+" has no name, skipping");
        continue;
      }

      Area area;

      area.reference.Set(a->id,refWay);
      area.name=name;
      area.area=a->nodes;

      AddArea(rootArea,area);
    }

    progress.SetAction("Inserting cities of type node into area tree");

    size_t count=0;
    for (std::list<Node>::iterator city=cityNodes.begin();
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

      /*
      if (count%1000==0) {
        std::cout << count << "/" << cityNodes.size() << std::endl;
      }*/
    }

    progress.SetAction("Dumping areas");

    DumpArea(rootArea,0);

    progress.SetAction("Delete temporary data");

    cityNodes.clear();
    cityAreas.clear();
    boundaryAreas.clear();
    boundaryRelations.clear();

    progress.SetAction("Calculating bounds of areas");

    CalculateAreaBounds(rootArea);

    std::set<TypeId> indexables;

    typeConfig.GetIndexables(indexables);

    progress.SetAction("Resolve ways and areas to areas");

    if (!scanner.Open("ways.dat")) {
      progress.Error("Cannot open 'ways.dat'");
      return false;
    }

    while (!scanner.IsEOF()) {
      Way way;

      way.Read(scanner);

      if (!scanner.HasError()) {
        if (indexables.find(way.GetType())!=indexables.end()) {

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

    while (!scanner.IsEOF()) {
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

    GetLocationRefs(rootArea,locationRefs);

    if (!writer.Open("nameregion.idx")) {
      progress.Error("Cannot open 'nameregion.idx'");
      return false;
    }

    WriteLocationRefs(writer,locationRefs);

    return !writer.HasError() && writer.Close();
  }
}

