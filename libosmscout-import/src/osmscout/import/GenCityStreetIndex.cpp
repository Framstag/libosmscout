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

#include <iostream>
#include <fstream>
#include <limits>
#include <list>
#include <map>
#include <set>

#include <osmscout/Node.h>
#include <osmscout/Area.h>
#include <osmscout/Way.h>
#include <osmscout/ObjectRef.h>

#include <osmscout/CityStreetIndex.h>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

#include <osmscout/util/File.h>
#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>
#include <osmscout/util/Geometry.h>
#include <osmscout/util/HashSet.h>
#include <osmscout/util/String.h>

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
    std::vector<GeoCoord>                area;      //! the geometric area of this region

    double                               minlon;
    double                               minlat;
    double                               maxlon;
    double                               maxlat;

    std::map<std::string,std::list<ObjectFileRef> > locations; //! list of indexed objects in this region

    std::list<Region>                               regions;   //! A list of sub regions

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
    ObjectFileRef                       reference;
    std::string                         name;
    size_t                              level;
    std::vector<std::vector<GeoCoord> > areas;
  };

  struct CityArea
  {
    ObjectFileRef         reference;
    std::string           name;
    std::vector<GeoCoord> nodes;
  };

  struct CityNode
  {
    ObjectFileRef reference;
    std::string   name;
    GeoCoord      node;
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
        std::string name=node.GetName();

        if (name.empty()) {
          progress.Warning(std::string("Node ")+NumberToString(node.GetFileOffset())+" has no name, skipping");
          continue;
        }

        CityNode cityNode;

        cityNode.reference.Set(node.GetFileOffset(),refNode);
        cityNode.name=name;
        cityNode.node.Set(node.GetLat(),node.GetLon());

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
    uint32_t    areaCount;

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "areas.dat"),
                      FileScanner::Sequential,
                      parameter.GetWayDataMemoryMaped())) {
      progress.Error("Cannot open 'areas.dat'");
      return false;
    }

    if (!scanner.Read(areaCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    for (uint32_t a=1; a<=areaCount; a++) {
      progress.SetProgress(a,areaCount);

      Area area;

      if (!area.Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(a)+" of "+
                       NumberToString(areaCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");
        return false;
      }

      if (cityIds.find(area.GetType())!=cityIds.end()) {
        for (std::vector<Area::Ring>::const_iterator ring=area.rings.begin();
             ring!=area.rings.end();
             ++ring) {
          if (ring->ring!=0) {
            continue;
          }

          std::string name=area.rings.front().GetName();

          CityArea cityArea;

          cityArea.reference.Set(area.GetFileOffset(),refArea);
          cityArea.name=name;
          cityArea.nodes=ring->nodes;

          cityAreas.push_back(cityArea);
        }
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
          if (IsCoordInArea(node->node,area->nodes)) {
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
          progress.Warning(std::string("Could not resolve name of ")+area->reference.GetTypeName()+" "+NumberToString(area->reference.GetFileOffset())+", skipping");
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
    uint32_t    areaCount;

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "areas.dat"),
                      FileScanner::Sequential,
                      true)) {
      progress.Error("Cannot open 'relations.dat'");
      return false;
    }

    if (!scanner.Read(areaCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    for (uint32_t r=1; r<=areaCount; r++) {
      progress.SetProgress(r,areaCount);

      Area area;

      if (!area.Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(r)+" of "+
                       NumberToString(areaCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");
        return false;
      }

      if (area.GetType()==boundaryId) {
        size_t level=0;

        if (area.rings.front().GetName().empty()) {
          progress.Warning(std::string("Area boundary ")+
                           NumberToString(area.GetType())+" "+
                           NumberToString(area.GetFileOffset())+" has no name");
        }

        for (std::vector<Tag>::const_iterator tag=area.rings.front().attributes.GetTags().begin();
            tag!=area.rings.front().attributes.GetTags().end(); ++tag) {
          if (tag->key==typeConfig.tagAdminLevel) {
            if (StringToNumber(tag->value,level)) {
              Boundary boundary;

              boundary.reference.Set(area.GetFileOffset(),refArea);
              boundary.name=area.rings.front().GetName();
              boundary.level=level;

              for (std::vector<Area::Ring>::const_iterator ring=area.rings.begin();
                   ring!=area.rings.end();
                   ++ring) {
                if (ring->ring==Area::outerRingId) {
                  boundary.areas.push_back(ring->nodes);
                }
              }

              boundaryAreas.push_back(boundary);
            }
            else {
              progress.Info("Could not parse admin_level of relation "+
                            NumberToString(area.GetType())+" "+
                            NumberToString(area.GetFileOffset()));
            }

            break;
          }
        }

        if (level==0) {
          progress.Info("No tag 'admin_level' for relation "+
                        NumberToString(area.GetType())+" "+
                        NumberToString(area.GetFileOffset()));
        }
      }
    }

    return scanner.Close();
  }

  static void AddRegion(Region& parent,
                        const Region& region)
  {
    for (std::list<Region>::iterator r=parent.regions.begin();
         r!=parent.regions.end();
         r++) {
      if (!(region.maxlon<r->minlon) &&
          !(region.minlon>r->maxlon) &&
          !(region.maxlat<r->minlat) &&
          !(region.minlat>r->maxlat)) {
        if (IsAreaSubOfArea(region.area,r->area)) {
          // If we already have the same name and are a "minor" reference, we skip...
          if (!(region.name==r->name &&
                region.reference.type<r->reference.type)) {
            AddRegion(*r,region);
          }
          return;
        }
      }
    }

    parent.regions.push_back(region);
  }

  static void AddLocationToRegion(Region& area,
                                  const RegionAlias& location,
                                  const GeoCoord& node)
                                {
    if (area.name==location.name) {
      return;
    }

    for (std::list<Region>::iterator a=area.regions.begin();
         a!=area.regions.end();
         a++) {
      if (IsCoordInArea(node,a->area)) {
        AddLocationToRegion(*a,location,node);
        return;
      }
    }

    area.aliases.push_back(location);
  }

  static void BuildRegionTreeFromAreas(Progress& progress,
                                       const TypeConfig& typeConfig,
                                       const std::list<Boundary>& boundaryAreas,
                                       const std::list<CityArea>& cityAreas,
                                       const std::list<CityNode>& cityNodes,
                                       Region& rootRegion)
  {
    size_t currentCount=1;
    size_t maxCount=boundaryAreas.size()+cityAreas.size()+cityNodes.size();

    for (size_t l=1; l<=10; l++) {
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

  static bool AddAreaToRegion(Region& region,
                              const Area& area,
                              const std::vector<GeoCoord>& nodes,
                              const std::string& name,
                              double minlon,
                              double minlat,
                              double maxlon,
                              double maxlat)
  {
    for (std::list<Region>::iterator r=region.regions.begin();
         r!=region.regions.end();
         r++) {
      // Fast check, if the object is in the bounds of the area
      if (!(maxlon<r->minlon) &&
          !(minlon>r->maxlon) &&
          !(maxlat<r->minlat) &&
          !(minlat>r->maxlat)) {
        // Check if one point is in the area
        bool match=IsCoordInArea(nodes[0],r->area);

        if (match) {
          bool completeMatch=AddAreaToRegion(*r,area,nodes,name,minlon,minlat,maxlon,maxlat);

          if (completeMatch) {
            // We are done, the object is completely enclosed by one of our sub areas
            return true;
          }
        }
      }
    }

    // If we (at least partly) contain it, we add it to the area but continue

    region.locations[name].push_back(ObjectFileRef(area.GetFileOffset(),refArea));

    bool completeMatch=IsAreaCompletelyInArea(nodes,region.area);

    return completeMatch;
  }

  /**
    Add the given object (currently only a way) to
    the hierachical area index.

    If the method returns true, the objects was completely contained
    by the passed area (or one of its sub areas), else it returns false.
    If it returns false, not all points of the object were covered by the area
    and the parent area should add the object, too.

    The code is designed to minimize the number of "point in area" checks, it assumes that
    if one point of an object is in a area it is very likely that all points of the object
    are in the area.
    */
  static void AddAreaToRegion(Region& region,
                              const Area& area,
                              const Area::Ring& ring)
  {
    double minlon;
    double maxlon;
    double minlat;
    double maxlat;

    if (ring.ring==Area::masterRingId &&
        ring.nodes.empty()) {
      for (std::vector<Area::Ring>::const_iterator r=area.rings.begin();
          r!=area.rings.end();
          ++r) {
        if (r->ring==Area::outerRingId) {
          r->GetBoundingBox(minlon,maxlon,minlat,maxlat);

          AddAreaToRegion(region,
                          area,
                          r->nodes,
                          ring.GetName(),
                          minlon,
                          minlat,
                          maxlon,
                          maxlat);
        }
      }
    }
    else {
      ring.GetBoundingBox(minlon,maxlon,minlat,maxlat);

      AddAreaToRegion(region,
                      area,
                      ring.nodes,
                      ring.GetName(),
                      minlon,
                      minlat,
                      maxlon,
                      maxlat);
    }
  }

  /**
    Add the given object (currently only a way) to
    the hierachical area index.

    If the method returns true, the objects was completely contained
    by the passed area (or one of its sub areas), else it returns false.
    If it returns false, not all points of the object were covered by the area
    and the parent area should add the object, too.

    The code is designed to minimize the number of "point in area" checks, it assumes that
    if one point of an object is in a area it is very likely that all points of the object
    are in the area.
    */
  static bool AddWayToRegion(Region& region,
                             const Way& way,
                             double minlon,
                             double minlat,
                             double maxlon,
                             double maxlat)
  {
    for (std::list<Region>::iterator r=region.regions.begin();
         r!=region.regions.end();
         r++) {
      // Fast check, if the object is in the bounds of the area
      if (!(maxlon<r->minlon) &&
          !(minlon>r->maxlon) &&
          !(maxlat<r->minlat) &&
          !(minlat>r->maxlat)) {
        // Check if one point is in the area
        bool match=IsCoordInArea(way.nodes[0],r->area);

        if (match) {
          bool completeMatch=AddWayToRegion(*r,way,minlon,minlat,maxlon,maxlat);

          if (completeMatch) {
            // We are done, the object is completely enclosed by one of our sub areas
            return true;
          }
        }
      }
    }

    // If we (at least partly) contain it, we add it to the area but continue

    region.locations[way.GetName()].push_back(ObjectFileRef(way.GetFileOffset(),refWay));

    bool completeMatch=IsAreaCompletelyInArea(way.nodes,region.area);

    return completeMatch;
  }

  static bool IndexAreas(const ImportParameter& parameter,
                         Progress& progress,
                         const TypeConfig& typeConfig,
                         const OSMSCOUT_HASHSET<TypeId>& indexables,
                         Region& rootArea)
  {
    FileScanner scanner;
    uint32_t    areaCount;

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "areas.dat"),
                      FileScanner::Sequential,
                      parameter.GetWayDataMemoryMaped())) {
      progress.Error("Cannot open 'areas.dat'");
      return false;
    }

    if (!scanner.Read(areaCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    for (uint32_t w=1; w<=areaCount; w++) {
      progress.SetProgress(w,areaCount);

      Area area;

      if (!area.Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(w)+" of "+
                       NumberToString(areaCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");
        return false;
      }

      for (std::vector<Area::Ring>::const_iterator ring=area.rings.begin();
          ring!=area.rings.end();
          ++ring) {
        if (ring->GetType()!=typeIgnore &&
            !ring->GetName().empty()) {
          if (indexables.find(ring->GetType())!=indexables.end()) {
            AddAreaToRegion(rootArea,area,*ring);
          }
        }

      }
    }

    return scanner.Close();
  }

  static bool IndexWays(const ImportParameter& parameter,
                        Progress& progress,
                        const TypeConfig& typeConfig,
                        const OSMSCOUT_HASHSET<TypeId>& indexables,
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

      Way way;

      if (!way.Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(w)+" of "+
                       NumberToString(wayCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");
        return false;
      }

      if (indexables.find(way.GetType())!=indexables.end() &&
          !way.GetName().empty()) {
        double minlon;
        double maxlon;
        double minlat;
        double maxlat;

        way.GetBoundingBox(minlon,maxlon,minlat,maxlat);

        AddWayToRegion(rootArea,way,minlon,minlat,maxlon,maxlat);
      }
    }

    return scanner.Close();
  }

  static void AddNodeToRegion(Region& area,
                              const Node& node,
                              const std::string& name,
                              FileOffset offset)
  {
    for (std::list<Region>::iterator a=area.regions.begin();
         a!=area.regions.end();
         a++) {
      if (IsCoordInArea(node,a->area)) {
        AddNodeToRegion(*a,node,name,offset);
        return;
      }
    }

    area.locations[name].push_back(ObjectFileRef(offset,refNode));
  }

  static bool IndexNodes(const ImportParameter& parameter,
                         Progress& progress,
                         const TypeConfig& typeConfig,
                         const OSMSCOUT_HASHSET<TypeId>& indexables,
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
        std::string name=node.GetName();

        if (!name.empty()) {
          AddNodeToRegion(rootArea,node,name,offset);
        }
      }
    }

    return scanner.Close();
  }

  static void DumpRegion(const Region& parent,
                         size_t indent,
                         std::ostream& out)
  {
    for (std::list<Region>::const_iterator r=parent.regions.begin();
         r!=parent.regions.end();
         r++) {
      for (size_t i=0; i<indent; i++) {
        out << " ";
      }
      out << " + " << r->name << " " << r->reference.GetTypeName() << " " << r->reference.GetFileOffset() << std::endl;

      for (std::list<RegionAlias>::const_iterator l=r->aliases.begin();
           l!=r->aliases.end();
           l++) {
        for (size_t i=0; i<indent; i++) {
          out << " ";
        }
        out << " = " << l->name << " " << l->reference.GetTypeName() << " " << l->reference.GetFileOffset() << std::endl;
      }

      for (std::map<std::string,std::list<ObjectFileRef> >::const_iterator nodeEntry=r->locations.begin();
          nodeEntry!=r->locations.end();
          ++nodeEntry) {
        for (size_t i=0; i<indent; i++) {
          out << " ";
        }
        out << " - " << nodeEntry->first << std::endl;

        for (std::list<ObjectFileRef>::const_iterator object=nodeEntry->second.begin();
            object!=nodeEntry->second.end();
            ++object) {
          for (size_t i=0; i<indent+2; i++) {
            out << " ";
          }

          out << " = " << object->GetTypeName() << " " << object->GetFileOffset() << std::endl;
        }
      }

      DumpRegion(*r,
                 indent+2,
                 out  );
    }
  }

  static unsigned long GetRegionTreeDepth(const Region& area)
  {
    unsigned long depth=0;

    for (std::list<Region>::const_iterator a=area.regions.begin();
         a!=area.regions.end();
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

    for (std::list<Region>::iterator a=area.regions.begin();
         a!=area.regions.end();
         a++) {
      SortInRegion(*a,areaTree,level+1);
    }
  }

  static bool WriteRegion(FileWriter& writer,
                          Region& region, FileOffset parentOffset)
  {
    writer.GetPos(region.offset);

    writer.Write(region.name);
    writer.WriteNumber(parentOffset);

    writer.WriteNumber((uint32_t)region.regions.size());
    for (std::list<Region>::iterator r=region.regions.begin();
         r!=region.regions.end();
         r++) {
      if (!WriteRegion(writer,*r,region.offset)) {
        return false;
      }
    }

    writer.WriteNumber((uint32_t)region.locations.size()); // Number of objects

    for (std::map<std::string,std::list<ObjectFileRef> >::iterator location=region.locations.begin();
         location!=region.locations.end();
         ++location) {
      location->second.sort(ObjectFileRefByFileOffsetComparator());

      writer.Write(location->first);
      writer.WriteNumber((uint32_t)location->second.size()); // Number of objects

      FileOffset lastOffset=0;

      for (std::list<ObjectFileRef>::const_iterator object=location->second.begin();
             object!=location->second.end();
             ++object) {
        writer.Write((uint8_t)object->GetType());
        writer.WriteNumber(object->GetFileOffset()-lastOffset);

        lastOffset=object->GetFileOffset();
      }
    }

    return !writer.HasError();
  }

  static bool WriteRegions(FileWriter& writer,
                           Region& root)
  {
    for (std::list<Region>::iterator r=root.regions.begin();
         r!=root.regions.end();
         ++r) {
      if (!WriteRegion(writer,*r,0)) {
        return false;
      }
    }

    return true;
  }

  static void GetLocationRefs(const Region& regions,
                              std::map<std::string,std::list<RegionRef> >& locationRefs)
  {
    RegionRef locRef;

    locRef.reference=regions.reference;
    locRef.offset=regions.offset;

    locationRefs[regions.name].push_back(locRef);

    for (std::list<RegionAlias>::const_iterator l=regions.aliases.begin();
         l!=regions.aliases.end();
         ++l) {
      locRef.reference=l->reference;
      locRef.offset=regions.offset;

      locationRefs[l->name].push_back(locRef);
    }

    for (std::list<Region>::const_iterator r=regions.regions.begin();
         r!=regions.regions.end();
         r++) {
      GetLocationRefs(*r,locationRefs);
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
    OSMSCOUT_HASHSET<TypeId>         indexables;
    TypeId                           boundaryId;
    TypeId                           typeId;
    Region                           rootRegion;
    std::list<CityNode>              cityNodes;
    std::list<CityArea>              cityAreas;
    std::list<Boundary>              boundaryAreas;
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

    progress.Info(std::string("Found ")+NumberToString(boundaryAreas.size())+" areas of type 'administrative boundary'");

    progress.SetAction("Merging city areas and city nodes");

    MergeCityAreasAndNodes(progress,
                           cityAreas,
                           cityNodes);

    progress.SetAction("Inserting boundaries and cities into area tree");

    BuildRegionTreeFromAreas(progress,
                             typeConfig,
                             boundaryAreas,
                             cityAreas,
                             cityNodes,
                             rootRegion);

    progress.SetAction("Delete temporary data");

    cityNodes.clear();
    cityAreas.clear();
    boundaryAreas.clear();

    progress.SetAction("Calculating bounds of areas");

    regionTree.resize(GetRegionTreeDepth(rootRegion));

    progress.Info(std::string("Area tree depth: ")+NumberToString(regionTree.size()));

    progress.SetAction("Sorting areas in levels");

    SortInRegion(rootRegion,regionTree,0);

    for (size_t i=0; i<regionTree.size(); i++) {
      progress.Info(std::string("Area tree index ")+NumberToString(i)+" size: "+NumberToString(regionTree[i].size()));
    }

    progress.SetAction("Index areas");

    if (!IndexAreas(parameter,
                    progress,
                    typeConfig,
                    indexables,
                    rootRegion)) {
      return false;
    }

    progress.SetAction("Index ways");

    if (!IndexWays(parameter,
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
        count+=(*iter)->locations.size();
      }
      progress.Info(std::string("Area tree index ")+NumberToString(i)+" object count size: "+NumberToString(count));
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

    progress.SetAction(std::string("Write '")+CityStreetIndex::FILENAME_REGION_DAT+"'");

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     CityStreetIndex::FILENAME_REGION_DAT))) {
      progress.Error("Cannot open '"+writer.GetFilename()+"'");
      return false;
    }

    if (!WriteRegions(writer,rootRegion)) {
      return false;
    }

    if (writer.HasError() || !writer.Close()) {
      return false;
    }

    progress.SetAction("Dumping areas");

    std::string   debugFilename=AppendFileToDir(parameter.GetDestinationDirectory(),
                                                "citystreet.txt");
    std::ofstream debugStream;

    debugStream.open(debugFilename.c_str(),
                     std::ios::out|std::ios::trunc);

    if (debugStream.is_open()) {
      DumpRegion(rootRegion,
                 0,
                 debugStream);
      debugStream.close();
    }
    else {
      progress.Error("Cannot open '"+debugFilename+"'");
    }


    //
    // Generate file with all area names, each referencing the areas where it is contained
    //

    std::map<std::string,std::list<RegionRef> > locationRefs;

    progress.SetAction(std::string("Write '")+CityStreetIndex::FILENAME_NAMEREGION_IDX+"'");

    GetLocationRefs(rootRegion,locationRefs);

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     CityStreetIndex::FILENAME_NAMEREGION_IDX))) {
      progress.Error("Cannot open '"+writer.GetFilename()+"'");
      return false;
    }

    WriteLocationRefs(writer,locationRefs);

    return !writer.HasError() && writer.Close();
  }
}
