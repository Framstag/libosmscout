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

#include <osmscout/import/GenCityStreetIndex.h>

#include <cassert>
#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <set>

#include <osmscout/Node.h>
#include <osmscout/ObjectRef.h>
#include <osmscout/Point.h>
#include <osmscout/Relation.h>
#include <osmscout/Way.h>

#include <osmscout/util/File.h>
#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>
#include <osmscout/util/Geometry.h>
#include <osmscout/util/HashSet.h>
#include <osmscout/util/String.h>

#include <osmscout/private/Math.h>

namespace osmscout {

  /**
   * An area can contain an number of location nodes. Since they do not have
   * their own area we define the nod ename as an alias for the containing
   * area, since this is the best aproximation.
   */
  struct RegionAlias
  {
    ObjectFileRef          reference; //! Reference of the object that
                                      //! is the alias
    std::string            name;      //! The alias itsef
  };

  /**
   * Reference to an area
   */
  struct RegionRef
  {
    ObjectFileRef          reference; //! Reference of the object that
                                      //! is the alias
    FileOffset             offset;    //! Fileoffset of the area
  };

  /**
    An area. An area is a administrative region, a city, a country, ...
    An area can have child areas (suburbs, ...).
    An area has a name and also a number of locations, which are possibly
    within the area but area currently also represented by this area.
    */
  struct Region
  {
    FileOffset                           offset;    //! Offset into the index file

    ObjectFileRef                        reference; //! Reference to the object this area is based on
    std::string                          name;      //! The name of this area

    std::list<RegionAlias>               aliases;   //! Location that are represented by this region
    std::vector<Point>                   area;      //! the geometric area of this region

    double                               minlon;
    double                               minlat;
    double                               maxlon;
    double                               maxlat;

    std::map<std::string,std::list<FileOffset> > nodes;     //! list of indexed nodes in this region
    std::map<std::string,std::list<FileOffset> > ways;      //! list of indexed ways in this region

    std::list<Region>                    areas;     //! A list of sub regions

    void CalculateMinMax()
    {
      if (!area.empty()) {
        minlon=area[0].GetLon();
        maxlon=area[0].GetLon();

        minlat=area[0].GetLat();
        maxlat=area[0].GetLat();

        for (size_t n=1; n<area.size(); n++) {
          minlon=std::min(minlon,area[n].GetLon());
          maxlon=std::max(maxlon,area[n].GetLon());

          minlat=std::min(minlat,area[n].GetLat());
          maxlat=std::max(maxlat,area[n].GetLat());
        }
      }
    }
  };

  struct Boundary
  {
    ObjectFileRef                    reference;
    std::string                      name;
    size_t                           level;
    std::vector<std::vector<Point> > areas;
  };

  struct CityArea
  {
    ObjectFileRef      reference;
    std::string        name;
    std::vector<Point> nodes;
  };

  struct CityNode
  {
    ObjectFileRef reference;
    std::string   name;
    Point         node;
  };


  /**
    Return the list of nodes ids with the given type.
    */
  static bool GetCityNodes(const ImportParameter& parameter,
                           const TypeConfig& typeConfig,
                           const OSMSCOUT_HASHSET<TypeId>& cityIds,
                           std::list<CityNode>& cityNodes,
                           Progress& progress)
  {
    FileScanner scanner;
    uint32_t    nodeCount;

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "nodes.dat"),
                      FileScanner::Sequential,
                      true)) {
      progress.Error("Cannot open 'nodes.dat'");
      return false;
    }

    if (!scanner.Read(nodeCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    for (uint32_t n=1; n<=nodeCount; n++) {
      progress.SetProgress(n,nodeCount);

      Node node;

      if (!node.Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(n)+" of "+
                       NumberToString(nodeCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");
        return false;
      }

      if (cityIds.find(node.GetType())!=cityIds.end()) {
        std::string name;

        for (size_t i=0; i<node.GetTagCount(); i++) {
          if (node.GetTagKey(i)==typeConfig.tagPlaceName) {
            name=node.GetTagValue(i);
            break;
          }
          else if (node.GetTagKey(i)==typeConfig.tagName &&
                   name.empty()) {
            name=node.GetTagValue(i);
          }
        }

        if (name.empty()) {
          progress.Warning(std::string("Node ")+NumberToString(node.GetFileOffset())+" has no name, skipping");
          continue;
        }

        CityNode cityNode;

        cityNode.reference.Set(node.GetFileOffset(),refNode);
        cityNode.name=name;
        cityNode.node.Set(0/*TODO*/,node.GetLat(),node.GetLon());

        cityNodes.push_back(cityNode);
      }
    }

    return scanner.Close();
  }

  /**
    Return the list of nodes ids with the given type.
    */
  static bool GetCityAreas(const ImportParameter& parameter,
                           const TypeConfig& typeConfig,
                           const OSMSCOUT_HASHSET<TypeId>& cityIds,
                           std::list<CityArea>& cityAreas,
                           Progress& progress)
  {
    FileScanner scanner;
    uint32_t    wayCount;

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "ways.dat"),
                      FileScanner::Sequential,
                      parameter.GetWayDataMemoryMaped())) {
      progress.Error("Cannot open 'ways.dat'");
      return false;
    }

    if (!scanner.Read(wayCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    for (uint32_t w=1; w<=wayCount; w++) {
      progress.SetProgress(w,wayCount);

      Way way;

      if (!way.Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(w)+" of "+
                       NumberToString(wayCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");
        return false;
      }

      if (way.IsArea() &&
          cityIds.find(way.GetType())!=cityIds.end()) {
        std::string name=way.GetName();

        for (size_t i=0; i<way.GetTagCount(); i++) {
          if (way.GetTagKey(i)==typeConfig.tagPlaceName) {
            name=way.GetTagValue(i);
            break;
          }
        }

        CityArea cityArea;

        cityArea.reference.Set(way.GetFileOffset(),refWay);
        cityArea.name=name;
        cityArea.nodes=way.nodes;

        cityAreas.push_back(cityArea);
      }
    }

    return scanner.Close();
  }

  static void MergeCityAreasAndNodes(Progress& progress,
                                     std::list<CityArea>& cityAreas,
                                     std::list<CityNode>& cityNodes)
  {
    std::list<CityArea>::iterator area=cityAreas.begin();
    while (area!=cityAreas.end()) {
      if (area->name.empty()) {
        size_t                    hits=0;
        std::list<CityNode>::iterator candidate=cityNodes.end();

        std::list<CityNode>::iterator node=cityNodes.begin();
        while (hits<=1 &&
               node!=cityNodes.end()) {
          if (IsPointInArea(node->node,area->nodes)) {
            hits++;

            if (candidate==cityNodes.end()) {
              candidate=node;
            }
          }

          if (hits<=1) {
            node++;
          }
        }

        if (hits==0) {
          progress.Warning(std::string("Could not resolve name of ")+area->reference.GetTypeName()+NumberToString(area->reference.GetFileOffset())+", skipping");
          area=cityAreas.erase(area);
        }
        else if (hits==1) {
          area->name=candidate->name;

          cityNodes.erase(candidate);

          area++;
        }
        else {
          progress.Warning(area->reference.GetTypeName()+NumberToString(area->reference.GetFileOffset())+" contains multiple city nodes, skipping");
          area=cityAreas.erase(area);
        }
      }
      else {
        area++;
      }
    }
  }

  /**
    Return the list of ways of type administrative boundary.
    */
  static bool GetBoundaryAreas(const ImportParameter& parameter,
                               const TypeConfig& typeConfig,
                               TypeId boundaryId,
                               std::list<Boundary>& boundaryAreas,
                               Progress& progress)
  {
    FileScanner scanner;
    uint32_t    wayCount;

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "ways.dat"),
                      FileScanner::Sequential,
                      parameter.GetWayDataMemoryMaped())) {
      progress.Error("Cannot open 'ways.dat'");
      return false;
    }

    if (!scanner.Read(wayCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    for (uint32_t w=1; w<=wayCount; w++) {
      Way way;

      progress.SetProgress(w,wayCount);

      if (!way.Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(w)+" of "+
                       NumberToString(wayCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");
        return false;
      }

      if (way.IsArea() &&
          way.GetType()==boundaryId) {
        size_t level=0;

        if (way.GetName().empty()) {
          progress.Warning(std::string("Area boundary ")+NumberToString(way.GetFileOffset())+" has no name");
        }

        for (size_t i=0; i<way.GetTagCount(); i++) {
          if (way.GetTagKey(i)==typeConfig.tagAdminLevel) {
            if (StringToNumber(way.GetTagValue(i),level)) {
              Boundary boundary;

              boundary.reference.Set(way.GetFileOffset(),refWay);
              boundary.name=way.GetName();
              boundary.level=level;
              boundary.areas.push_back(way.nodes);

              boundaryAreas.push_back(boundary);
            }
            else {
              progress.Info("Could not parse admin_level of way "+
                            NumberToString(way.GetType() )+" "+NumberToString(way.GetFileOffset()));
            }

            break;
          }
        }
      }
    }

    return scanner.Close();
  }

  /**
    Return the list of ways of type administrative boundary.
    */
  static bool GetBoundaryRelations(const ImportParameter& parameter,
                                   const TypeConfig& typeConfig,
                                   TypeId boundaryId,
                                   std::list<Boundary>& boundaryRelations,
                                   Progress& progress)
  {
    FileScanner scanner;
    uint32_t    relCount;

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "relations.dat"),
                      FileScanner::Sequential,
                      true)) {
      progress.Error("Cannot open 'relations.dat'");
      return false;
    }

    if (!scanner.Read(relCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    for (uint32_t r=1; r<=relCount; r++) {
      progress.SetProgress(r,relCount);

      Relation   relation;
      FileOffset offset;

      if (!scanner.GetPos(offset)) {
        progress.Error(std::string("Cannot get file offset of data entry ")+
                       NumberToString(r)+" of "+
                       NumberToString(relCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");

        return false;
      }

      if (!relation.Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(r)+" of "+
                       NumberToString(relCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");
        return false;
      }

      if (relation.GetType()==boundaryId) {
        size_t level=0;

        if (relation.GetName().empty()) {
          progress.Warning(std::string("Relation boundary ")+
                           NumberToString(relation.GetType())+" "+
                           NumberToString(relation.GetFileOffset())+" has no name");
        }

        for (size_t i=0; i<relation.GetTagCount(); i++) {
          if (relation.GetTagKey(i)==typeConfig.tagAdminLevel) {
            if (StringToNumber(relation.GetTagValue(i),level)) {
              Boundary boundary;

              boundary.reference.Set(offset,refRelation);
              boundary.name=relation.GetName();
              boundary.level=level;

              for (std::vector<Relation::Role>::const_iterator role=relation.roles.begin();
                   role!=relation.roles.end();
                   ++role) {
                if (role->ring==0) {
                  boundary.areas.push_back(role->nodes);
                }
              }

              boundaryRelations.push_back(boundary);
            }
            else {
              progress.Info("Could not parse admin_level of relation "+
                            NumberToString(relation.GetType())+" "+
                            NumberToString(relation.GetFileOffset()));
            }

            break;
          }
        }

        if (level==0) {
          progress.Info("No tag 'admin_level' for relation "+
                        NumberToString(relation.GetType())+" "+
                        NumberToString(relation.GetFileOffset()));
        }
      }
    }

    return scanner.Close();
  }

  static void AddRegion(Region& parent,
                        const Region& area)
  {
    for (std::list<Region>::iterator a=parent.areas.begin();
         a!=parent.areas.end();
         a++) {
      if (!(area.maxlon<a->minlon) &&
          !(area.minlon>a->maxlon) &&
          !(area.maxlat<a->minlat) &&
          !(area.minlat>a->maxlat)) {
        if (IsAreaSubOfArea(area.area,a->area)) {
          // If we already have the same name and are a "minor" reference, we skip...
          if (!(area.name==a->name &&
                area.reference.type<a->reference.type)) {
            AddRegion(*a,area);
          }
          return;
        }
      }
    }

    parent.areas.push_back(area);
  }

  static void AddLocationToRegion(Region& area,
                                  const RegionAlias& location,
                                  const Point& node)
                                {
    if (area.name==location.name) {
      return;
    }

    for (std::list<Region>::iterator a=area.areas.begin();
         a!=area.areas.end();
         a++) {
      if (IsPointInArea(node,a->area)) {
        AddLocationToRegion(*a,location,node);
        return;
      }
    }

    area.aliases.push_back(location);
  }

  static void BuildRegionTreeFromAreas(Progress& progress,
                                       const TypeConfig& typeConfig,
                                       const std::list<Boundary>& boundaryAreas,
                                       const std::list<Boundary>& boundaryRelations,
                                       const std::list<CityArea>& cityAreas,
                                       const std::list<CityNode>& cityNodes,
                                       Region& rootRegion)
  {
    size_t currentCount=1;
    size_t maxCount=boundaryAreas.size()+boundaryRelations.size()+cityAreas.size()+cityNodes.size();

    for (size_t l=1; l<=10; l++) {
      for (std::list<Boundary>::const_iterator rel=boundaryRelations.begin();
           rel!=boundaryRelations.end();
           ++rel) {
        progress.SetProgress(currentCount,maxCount);

        size_t      level=rel->level;
        std::string name=rel->name;

        if (level==l) {
          for (size_t i=0; i<rel->areas.size(); i++) {
            Region region;

            region.reference=rel->reference;
            region.name=name.empty() ? "???" : name;
            region.area=rel->areas[i];

            region.CalculateMinMax();

            AddRegion(rootRegion,region);
          }

          currentCount++;
        }
      }

      for (std::list<Boundary>::const_iterator a=boundaryAreas.begin();
           a!=boundaryAreas.end();
           ++a) {
        progress.SetProgress(currentCount,maxCount);

        size_t      level=a->level;
        std::string name=a->name;

        if (level==l) {
          Region region;

          region.reference=a->reference;
          region.name=name.empty() ? "???" : name;
          region.area=a->areas.front();

          region.CalculateMinMax();

          AddRegion(rootRegion,region);

          currentCount++;
        }
      }
    }

    for (std::list<CityArea>::const_iterator city=cityAreas.begin();
         city!=cityAreas.end();
         ++city) {
      progress.SetProgress(currentCount,maxCount);

      Region region;

      region.reference=city->reference;
      region.name=city->name;
      region.area=city->nodes;

      region.CalculateMinMax();

      AddRegion(rootRegion,region);

      currentCount++;
    }

    for (std::list<CityNode>::const_iterator city=cityNodes.begin();
         city!=cityNodes.end();
         ++city) {
      progress.SetProgress(currentCount,maxCount);

      RegionAlias alias;

      alias.reference=city->reference;
      alias.name=city->name;

      AddLocationToRegion(rootRegion,alias,city->node);

      currentCount++;
    }
  }

  /**
    Add the given object (currently only a way) to
    the hierachical area index.

    If the method returns true, the objects was completely contained
    by the passed area (or one of its sub areas), else it returns false.
    If it returns false, not all points of the object were covered by the area
    and the parent area should add the object, too.

    The code is designed to minimize the number of "point in area" checks, it assume that
    if one point of an object is in a area it is very likely that all points of the object
    are in the area.
    */
  static bool AddWayToRegion(Region& area,
                             const Way& way,
                             FileOffset offset,
                             double minlon,
                             double minlat,
                             double maxlon,
                             double maxlat)
  {
    for (std::list<Region>::iterator a=area.areas.begin();
         a!=area.areas.end();
         a++) {
      // Fast check, if the object is in the bounds of the area
      if (!(maxlon<a->minlon) &&
          !(minlon>a->maxlon) &&
          !(maxlat<a->minlat) &&
          !(minlat>a->maxlat)) {
        // Check if one point is in the area
        bool match=IsPointInArea(way.nodes[0],a->area);

        if (match) {
          bool completeMatch=AddWayToRegion(*a,way,offset,minlon,minlat,maxlon,maxlat);

          if (completeMatch) {
            // We are done, the object is completely enclosed by one of our sub areas
            return true;
          }
        }
      }
    }

    // We (at least partly) contain it, add it to the area but continue

    area.ways[way.GetName()].push_back(offset);

    bool completeMatch=IsAreaInArea(way.nodes,area.area);

    return completeMatch;
  }

  static bool IndexAreasAndWays(const ImportParameter& parameter,
                                Progress& progress,
                                const TypeConfig& typeConfig,
                                const std::set<TypeId>& indexables,
                                Region& rootArea)
  {
    FileScanner scanner;
    uint32_t    wayCount;

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "ways.dat"),
                      FileScanner::Sequential,
                      parameter.GetWayDataMemoryMaped())) {
      progress.Error("Cannot open 'ways.dat'");
      return false;
    }

    if (!scanner.Read(wayCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    for (uint32_t w=1; w<=wayCount; w++) {
      progress.SetProgress(w,wayCount);

      Way        way;
      FileOffset offset;

      if (!scanner.GetPos(offset)) {
        progress.Error(std::string("Cannot get file offset of data entry ")+
                       NumberToString(w)+" of "+
                       NumberToString(wayCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");

        return false;
      }

      if (!way.Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(w)+" of "+
                       NumberToString(wayCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");
        return false;
      }

      if (indexables.find(way.GetType())!=indexables.end()) {
        std::string name=way.GetName();

        if (!name.empty()) {
          double minlon=way.nodes[0].GetLon();
          double maxlon=way.nodes[0].GetLon();

          double minlat=way.nodes[0].GetLat();
          double maxlat=way.nodes[0].GetLat();

          for (size_t n=1; n<way.nodes.size(); n++) {
            minlon=std::min(minlon,way.nodes[n].GetLon());
            maxlon=std::max(maxlon,way.nodes[n].GetLon());

            minlat=std::min(minlat,way.nodes[n].GetLat());
            maxlat=std::max(maxlat,way.nodes[n].GetLat());
          }

          AddWayToRegion(rootArea,way,offset,minlon,minlat,maxlon,maxlat);
        }
      }
    }

    return scanner.Close();
  }

  static void AddNodeToRegion(Region& area,
                              const Node& node,
                              const std::string& name,
                              FileOffset offset)
  {
    for (std::list<Region>::iterator a=area.areas.begin();
         a!=area.areas.end();
         a++) {
      if (IsPointInAreaNoId(node,a->area)) {
        AddNodeToRegion(*a,node,name,offset);
        return;
      }
    }

    area.nodes[name].push_back(offset);
  }

  static bool IndexNodes(const ImportParameter& parameter,
                         Progress& progress,
                         const TypeConfig& typeConfig,
                         const std::set<TypeId>& indexables,
                         Region& rootArea)
  {
    FileScanner scanner;
    uint32_t    nodeCount;

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "nodes.dat"),
                      FileScanner::Sequential,
                      true)) {
      progress.Error("Cannot open 'nodes.dat'");
      return false;
    }

    if (!scanner.Read(nodeCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    for (uint32_t n=1; n<=nodeCount; n++) {
      progress.SetProgress(n,nodeCount);

      Node       node;
      FileOffset offset;

      if (!scanner.GetPos(offset)) {
        progress.Error(std::string("Cannot get file offset of data entry ")+
                       NumberToString(n)+" of "+
                       NumberToString(nodeCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");

        return false;
      }

      if (!node.Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(n)+" of "+
                       NumberToString(nodeCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");
        return false;
      }

      if (indexables.find(node.GetType())!=indexables.end()) {
        std::string name;

        for (size_t i=0; i<node.GetTagCount(); i++) {
          if (node.GetTagKey(i)==typeConfig.tagName) {
            name=node.GetTagValue(i);
            break;
          }
        }

        if (!name.empty()) {
          AddNodeToRegion(rootArea,node,name,offset);
        }
      }
    }

    return scanner.Close();
  }

  static void DumpRegion(const Region& parent, size_t indent)
  {
    for (std::list<Region>::const_iterator a=parent.areas.begin();
         a!=parent.areas.end();
         a++) {
      for (size_t i=0; i<indent; i++) {
        std::cout << " ";
      }
      std::cout << a->name << " " << a->reference.GetTypeName() << " " << a->reference.GetFileOffset() << " " << a->areas.size() << " " << a->nodes.size() << " " << a->ways.size() << " " << a->aliases.size() << std::endl;

      for (std::list<RegionAlias>::const_iterator l=a->aliases.begin();
           l!=a->aliases.end();
           l++) {
        for (size_t i=0; i<indent; i++) {
          std::cout << " ";
        }
        std::cout << " =" << l->name << " " << l->reference.GetTypeName() << " " << l->reference.GetFileOffset() << std::endl;
      }

      DumpRegion(*a,indent+2);
    }
  }

  static unsigned long GetRegionTreeDepth(const Region& area)
  {
    unsigned long depth=0;

    for (std::list<Region>::const_iterator a=area.areas.begin();
         a!=area.areas.end();
         a++) {
      depth=std::max(depth,GetRegionTreeDepth(*a));
    }

    return depth+1;
  }


  static void SortInRegion(Region& area,
                           std::vector<std::list<Region*> >& areaTree,
                           unsigned long level)
  {
    areaTree[level].push_back(&area);

    for (std::list<Region>::iterator a=area.areas.begin();
         a!=area.areas.end();
         a++) {
      SortInRegion(*a,areaTree,level+1);
    }
  }

  static bool WriteRegion(FileWriter& writer,
                          Region& area, FileOffset parentOffset)
  {
    writer.GetPos(area.offset);

    writer.Write(area.name);
    writer.WriteNumber(parentOffset);

    writer.WriteNumber((uint32_t)area.areas.size());
    for (std::list<Region>::iterator a=area.areas.begin();
         a!=area.areas.end();
         a++) {
      if (!WriteRegion(writer,*a,area.offset)) {
        return false;
      }
    }

    writer.WriteNumber((uint32_t)area.nodes.size());
    for (std::map<std::string,std::list<Id> >::const_iterator node=area.nodes.begin();
         node!=area.nodes.end();
         ++node) {
      writer.Write(node->first);                         // Node name
      writer.WriteNumber((uint32_t)node->second.size()); // Number of ids

      for (std::list<FileOffset>::const_iterator offset=node->second.begin();
             offset!=node->second.end();
             ++offset) {
        writer.WriteFileOffset(*offset); // File offset of node
      }
    }

    writer.WriteNumber((uint32_t)area.ways.size());
    for (std::map<std::string,std::list<FileOffset> >::const_iterator way=area.ways.begin();
         way!=area.ways.end();
         ++way) {
      writer.Write(way->first);                         // Way name
      writer.WriteNumber((uint32_t)way->second.size()); // Number of ids

      for (std::list<FileOffset>::const_iterator offset=way->second.begin();
           offset!=way->second.end();
           ++offset) {
        writer.WriteNumber(*offset); // File offset of way
      }
    }

    return !writer.HasError();
  }

  static bool WriteRegions(FileWriter& writer,
                           Region& root)
  {
    for (std::list<Region>::iterator a=root.areas.begin();
         a!=root.areas.end();
         ++a) {
      if (!WriteRegion(writer,*a,0)) {
        return false;
      }
    }

    return true;
  }

  static void GetLocationRefs(const Region& area,
                              std::map<std::string,std::list<RegionRef> >& locationRefs)
  {
    RegionRef locRef;

    locRef.reference=area.reference;
    locRef.offset=area.offset;

    locationRefs[area.name].push_back(locRef);

    for (std::list<RegionAlias>::const_iterator l=area.aliases.begin();
         l!=area.aliases.end();
         ++l) {
      locRef.reference=l->reference;
      locRef.offset=area.offset;

      locationRefs[l->name].push_back(locRef);
    }

    for (std::list<Region>::const_iterator a=area.areas.begin();
         a!=area.areas.end();
         a++) {
      GetLocationRefs(*a,locationRefs);
    }
  }

  static bool WriteLocationRefs(FileWriter& writer,
                                const std::map<std::string,std::list<RegionRef> >& locationRefs)
  {
    writer.WriteNumber((uint32_t)locationRefs.size());

    for (std::map<std::string,std::list<RegionRef> >::const_iterator n=locationRefs.begin();
         n!=locationRefs.end();
         ++n) {
      if (!writer.Write(n->first)) {
        return false;
      }

      if (!writer.WriteNumber((uint32_t)n->second.size())) {
        return false;
      }

      for (std::list<RegionRef>::const_iterator o=n->second.begin();
           o!=n->second.end();
           ++o) {
        if (!writer.WriteNumber((uint32_t)o->reference.GetType())) {
          return false;
        }

        if (!writer.WriteFileOffset(o->reference.GetFileOffset())) {
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
    OSMSCOUT_HASHSET<TypeId>         cityIds;
    std::set<TypeId>                 indexables;
    TypeId                           boundaryId;
    TypeId                           typeId;
    Region                           rootRegion;
    std::list<CityNode>              cityNodes;
    std::list<CityArea>              cityAreas;
    std::list<Boundary>              boundaryAreas;
    std::list<Boundary>              boundaryRelations;
    std::vector<std::list<Region*> > regionTree;

    progress.SetAction("Setup");

    rootRegion.name="<root>";
    rootRegion.offset=0;

    // We ignore (besides strange ones ;-)):
    // continent
    // country
    // county
    // island
    // quarter
    // region
    // square
    // state

    typeId=typeConfig.GetNodeTypeId("place_city");
    assert(typeId!=typeIgnore);
    cityIds.insert(typeId);

    typeId=typeConfig.GetNodeTypeId("place_town");
    assert(typeId!=typeIgnore);
    cityIds.insert(typeId);

    typeId=typeConfig.GetNodeTypeId("place_village");
    assert(typeId!=typeIgnore);
    cityIds.insert(typeId);

    typeId=typeConfig.GetNodeTypeId("place_hamlet");
    assert(typeId!=typeIgnore);
    cityIds.insert(typeId);

    typeId=typeConfig.GetNodeTypeId("place_suburb");
    assert(typeId!=typeIgnore);
    cityIds.insert(typeId);

    // We do not yet know if we handle borders as ways or areas
    boundaryId=typeConfig.GetWayTypeId("boundary_administrative");
    if (boundaryId==typeIgnore) {
      boundaryId=typeConfig.GetAreaTypeId("boundary_administrative");
    }
    assert(boundaryId!=typeIgnore);

    typeConfig.GetIndexables(indexables);

    progress.SetAction("Scanning for cities of type 'node'");

    //
    // Getting all nodes of type place=*. We later need an area for these cities.
    //

    // Get nodes of one of the types in cityIds
    if (!GetCityNodes(parameter,
                      typeConfig,
                      cityIds,
                      cityNodes,
                      progress)) {
      return false;
    }

    progress.Info(std::string("Found ")+NumberToString(cityNodes.size())+" cities of type 'node'");

    //
    // Getting all areas of type place=*.
    //

    progress.SetAction("Scanning for cities of type 'area'");

    // Get areas of one of the types in cityIds
    if (!GetCityAreas(parameter,
                      typeConfig,
                      cityIds,
                      cityAreas,
                      progress)) {
      return false;
    }

    progress.Info(std::string("Found ")+NumberToString(cityAreas.size())+" cities of type 'area'");

    //
    // Getting all areas of type 'administrative boundary'.
    //

    progress.SetAction("Scanning for administrative boundaries of type 'area'");

    if (!GetBoundaryAreas(parameter,
                          typeConfig,
                          boundaryId,
                          boundaryAreas,
                          progress)) {
      return false;
    }

    //
    // Getting all relations of type 'administrative boundary'.
    //

    progress.SetAction("Scanning for administrative boundaries of type 'relation'");

    if (!GetBoundaryRelations(parameter,
                              typeConfig,
                              boundaryId,
                              boundaryRelations,
                              progress)) {
      return false;
    }

    progress.Info(std::string("Found ")+NumberToString(boundaryAreas.size())+" areas of type 'administrative boundary'");
    progress.Info(std::string("Found ")+NumberToString(boundaryRelations.size())+" relations of type 'administrative boundary'");

    progress.SetAction("Merging city areas and city nodes");

    MergeCityAreasAndNodes(progress,
                           cityAreas,
                           cityNodes);

    progress.SetAction("Inserting boundaries and cities into area tree");

    BuildRegionTreeFromAreas(progress,
                             typeConfig,
                             boundaryAreas,
                             boundaryRelations,
                             cityAreas,
                             cityNodes,
                             rootRegion);

    progress.SetAction("Delete temporary data");

    cityNodes.clear();
    cityAreas.clear();
    boundaryAreas.clear();
    boundaryRelations.clear();

    progress.SetAction("Calculating bounds of areas");

    regionTree.resize(GetRegionTreeDepth(rootRegion));

    progress.Info(std::string("Area tree depth: ")+NumberToString(regionTree.size()));

    progress.SetAction("Sorting areas in levels");

    SortInRegion(rootRegion,regionTree,0);

    for (size_t i=0; i<regionTree.size(); i++) {
      progress.Info(std::string("Area tree index ")+NumberToString(i)+" size: "+NumberToString(regionTree[i].size()));
    }

    progress.SetAction("Index ways and areas");

    if (!IndexAreasAndWays(parameter,
                           progress,
                           typeConfig,
                           indexables,
                           rootRegion)) {
      return false;
    }

    for (size_t i=0; i<regionTree.size(); i++) {
      unsigned long count=0;

      for (std::list<Region*>::const_iterator iter=regionTree[i].begin();
           iter!=regionTree[i].end();
           ++iter) {
        count+=(*iter)->ways.size();
      }
      progress.Info(std::string("Area tree index ")+NumberToString(i)+" way count size: "+NumberToString(count));
    }

    progress.SetAction("Index nodes");

    if (!IndexNodes(parameter,
                    progress,
                    typeConfig,
                    indexables,
                    rootRegion)) {
      return false;
    }

    FileWriter writer;

    //
    // Generate file with all areas, where areas reference parent and children by offset
    //

    progress.SetAction("Write 'region.dat'");

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "region.dat"))) {
      progress.Error("Cannot open 'region.dat'");
      return false;
    }

    if (!WriteRegions(writer,rootRegion)) {
      return false;
    }

    if (writer.HasError() || !writer.Close()) {
      return false;
    }

    progress.SetAction("Dumping areas");

    DumpRegion(rootRegion,0);

    //
    // Generate file with all area names, each referencing the areas where it is contained
    //

    std::map<std::string,std::list<RegionRef> > locationRefs;

    progress.SetAction("Write 'nameregion.idx'");

    GetLocationRefs(rootRegion,locationRefs);

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "nameregion.idx"))) {
      progress.Error("Cannot open 'nameregion.idx'");
      return false;
    }

    WriteLocationRefs(writer,locationRefs);

    return !writer.HasError() && writer.Close();
  }
}
