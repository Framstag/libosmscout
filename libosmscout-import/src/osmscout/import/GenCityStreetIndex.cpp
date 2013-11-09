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
    FileOffset             reference; //! Reference to the node that is the alias
    std::string            name;      //! The alias itself
  };

  struct RegionAddress
  {
    ObjectFileRef          object;
    std::string            houseNr;
  };

  struct RegionLocation
  {
    std::list<ObjectFileRef> objects;   //! Objects that represent this location
    std::list<RegionAddress> addresses; //! Addresses at this location
  };

  /**
   * Reference to an area
   */
  struct RegionRef
  {
    ObjectFileRef          reference; //! Reference of the object that
                                      //! is the alias
    FileOffset             offset;    //! Fileoffset of the area

    inline bool operator<(const RegionRef& other) const
    {
      return reference<other.reference;
    }
  };

  /**
    An area. An area is a administrative region, a city, a country, ...
    An area can have child areas (suburbs, ...).
    An area has a name and also a number of locations, which are possibly
    within the area but area currently also represented by this area.
    */
  struct Region
  {
    FileOffset                          offset;    //! Offset into the index file

    ObjectFileRef                       reference; //! Reference to the object this area is based on
    std::string                         name;      //! The name of this area

    std::list<RegionAlias>              aliases;   //! Location that are represented by this region
    std::vector<std::vector<GeoCoord> > areas;     //! the geometric area of this region

    double                              minlon;
    double                              minlat;
    double                              maxlon;
    double                              maxlat;

    std::map<std::string,RegionLocation> locations; //! list of indexed objects in this region

    std::list<Region>                   regions;   //! A list of sub regions

    void CalculateMinMax()
    {
      bool isStart=true;

      for (size_t i=0; i<areas.size(); i++) {
        for (size_t j=0; j<areas[i].size(); j++) {
          if (isStart) {
            minlon=areas[i][j].GetLon();
            maxlon=areas[i][j].GetLon();

            minlat=areas[i][j].GetLat();
            maxlat=areas[i][j].GetLat();

            isStart=false;
          }
          else {
            minlon=std::min(minlon,areas[i][j].GetLon());
            maxlon=std::max(maxlon,areas[i][j].GetLon());

            minlat=std::min(minlat,areas[i][j].GetLat());
            maxlat=std::max(maxlat,areas[i][j].GetLat());
          }
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
    FileOffset                          reference;
    std::string                         name;
    std::vector<std::vector<GeoCoord> > areas;
  };

  struct CityNode
  {
    FileOffset  reference;
    std::string name;
    GeoCoord    node;
  };

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
        for (size_t i=0; i<region.areas.size(); i++) {
          for (size_t j=0; j<r->areas.size(); j++) {
            if (IsAreaSubOfArea(region.areas[i],r->areas[j])) {
              // If we already have the same name and are a "minor" reference, we skip...
              if (!(region.name==r->name &&
                    region.reference.type<r->reference.type)) {
                AddRegion(*r,region);
              }
              return;
            }
          }
        }
      }
    }

    parent.regions.push_back(region);
  }

  static void AddAliasToRegion(Region& area,
                               const RegionAlias& location,
                               const GeoCoord& node)
  {
    for (std::list<Region>::iterator a=area.regions.begin();
         a!=area.regions.end();
         a++) {
      for (size_t i=0; i<a->areas.size(); i++) {
        if (IsCoordInArea(node,a->areas[i])) {
          AddAliasToRegion(*a,location,node);
          return;
        }
      }
    }

    if (area.name==location.name) {
      return;
    }

    area.aliases.push_back(location);
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
        out << " = " << l->name << " Node " << l->reference << std::endl;
      }

      for (std::map<std::string,RegionLocation>::const_iterator nodeEntry=r->locations.begin();
          nodeEntry!=r->locations.end();
          ++nodeEntry) {
        for (size_t i=0; i<indent; i++) {
          out << " ";
        }
        out << " - " << nodeEntry->first << std::endl;

        for (std::list<ObjectFileRef>::const_iterator object=nodeEntry->second.objects.begin();
            object!=nodeEntry->second.objects.end();
            ++object) {
          for (size_t i=0; i<indent+2; i++) {
            out << " ";
          }

          out << " = " << object->GetTypeName() << " " << object->GetFileOffset() << std::endl;
        }

        for (std::list<RegionAddress>::const_iterator address=nodeEntry->second.addresses.begin();
            address!=nodeEntry->second.addresses.end();
            ++address) {
          for (size_t i=0; i<indent+4; i++) {
            out << " ";
          }

          out << " * " << address->houseNr << " " << address->object.GetTypeName() << " " << address->object.GetFileOffset() << std::endl;
        }
      }

      DumpRegion(*r,
                 indent+2,
                 out  );
    }
  }

  static bool DumpRegionTree(Progress& progress,
                             Region& rootRegion,
                             const std::string& filename)
  {
    std::ofstream debugStream;

    debugStream.open(filename.c_str(),
                     std::ios::out|std::ios::trunc);

    if (!debugStream.is_open()) {
      progress.Error("Cannot open '"+filename+"'");

      return false;
    }

    DumpRegion(rootRegion,
               0,
               debugStream);
    debugStream.close();

    return true;
  }

  /**
    Return the list of ways of type administrative boundary.
    */
  static bool GetBoundaryAreas(const ImportParameter& parameter,
                               Progress& progress,
                               const TypeConfig& typeConfig,
                               TypeId boundaryId,
                               std::list<Boundary>& boundaryAreas)
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

      if (area.GetType()!=boundaryId) {
        continue;
      }

      size_t level=0;

      if (area.rings.front().GetName().empty()) {
        progress.Warning(std::string("Boundary area ")+
                         NumberToString(area.GetType())+" "+
                         NumberToString(area.GetFileOffset())+" has no name");
        continue;
      }

      for (std::vector<Tag>::const_iterator tag=area.rings.front().attributes.GetTags().begin();
          tag!=area.rings.front().attributes.GetTags().end();
          ++tag) {
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

    return scanner.Close();
  }

  static void SortInBoundaries(Progress& progress,
                               Region& rootRegion,
                               const std::list<Boundary>& boundaryAreas,
                               size_t level)
  {
    size_t currentCount=0;

    for (std::list<Boundary>::const_iterator boundary=boundaryAreas.begin();
         boundary!=boundaryAreas.end();
         ++boundary) {
      progress.SetProgress(currentCount,boundaryAreas.size());

      if (boundary->level==level) {
        Region region;

        region.reference=boundary->reference;
        region.name=boundary->name;

        region.areas=boundary->areas;

        region.CalculateMinMax();

        AddRegion(rootRegion,region);

        currentCount++;
      }
    }
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

      if (cityIds.find(area.GetType())==cityIds.end()) {
        continue;
      }

      if (area.rings.front().GetName().empty()) {
        progress.Warning(std::string("City area ")+
                         NumberToString(area.GetType())+" "+
                         NumberToString(area.GetFileOffset())+" has no name");
        continue;
      }

      CityArea cityArea;

      cityArea.reference=area.GetFileOffset();
      cityArea.name=area.rings.front().GetName();

      for (std::vector<Area::Ring>::const_iterator ring=area.rings.begin();
           ring!=area.rings.end();
           ++ring) {
        if (ring->ring==Area::outerRingId) {
          cityArea.areas.push_back(ring->nodes);
        }
      }

      cityAreas.push_back(cityArea);
    }

    return scanner.Close();
  }

  static void SortInCityAreas(Progress& progress,
                              Region& rootRegion,
                              const std::list<CityArea>& cityAreas)
  {
    size_t currentCount=0;

    for (std::list<CityArea>::const_iterator area=cityAreas.begin();
         area!=cityAreas.end();
         ++area) {
      progress.SetProgress(currentCount,cityAreas.size());

      Region region;

      region.reference.Set(area->reference,refArea);
      region.name=area->name;

      region.areas=area->areas;

      region.CalculateMinMax();

      AddRegion(rootRegion,region);
    }
  }

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

        cityNode.reference=node.GetFileOffset();
        cityNode.name=name;
        cityNode.node.Set(node.GetLat(),node.GetLon());

        cityNodes.push_back(cityNode);
      }
    }

    return scanner.Close();
  }

  static void SortInCityNodes(Progress& progress,
                              Region& rootRegion,
                              std::list<CityNode>& cityNodes)
  {
    size_t currentCount=0;

    for (std::list<CityNode>::const_iterator city=cityNodes.begin();
         city!=cityNodes.end();
         ++city) {
      progress.SetProgress(currentCount,cityNodes.size());

      RegionAlias alias;

      alias.reference=city->reference;
      alias.name=city->name;

      AddAliasToRegion(rootRegion,alias,city->node);

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
        for (size_t i=0; i<r->areas.size(); i++) {
          // Check if one point is in the area
          bool match=IsCoordInArea(nodes[0],r->areas[i]);

          if (match) {
            bool completeMatch=AddAreaToRegion(*r,area,nodes,name,minlon,minlat,maxlon,maxlat);

            if (completeMatch) {
              // We are done, the object is completely enclosed by one of our sub areas
              return true;
            }
          }
        }
      }
    }

    // If we (at least partly) contain it, we add it to the area but continue

    region.locations[name].objects.push_back(ObjectFileRef(area.GetFileOffset(),refArea));

    for (size_t i=0; i<region.areas.size(); i++) {
      if (IsAreaCompletelyInArea(nodes,region.areas[i])) {
        return true;
      }
    }

    return false;
  }

  /**
    Add the given object (currently only a way) to
    the hierarchical area index.

    If the method returns true, the objects was completely contained
    by the passed area (or one of its sub areas), else it returns false.
    If it returns false, not all points of the object were covered by the area
    and the parent area should add the object, too.

    The code is designed to minimize the number of "point in area" checks, it assumes that
    if one point of an object is in a area it is very likely that all points of the object
    are in the area.
    */
  static void AddLocationAreaToRegion(Region& region,
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

  static bool IndexLocationAreas(const ImportParameter& parameter,
                                 Progress& progress,
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
            AddLocationAreaToRegion(rootArea,area,*ring);
          }
        }

      }
    }

    return scanner.Close();
  }

  /**
    Add the given object (currently only a way) to
    the hierarchical area index.

    If the method returns true, the objects was completely contained
    by the passed area (or one of its sub areas), else it returns false.
    If it returns false, not all points of the object were covered by the area
    and the parent area should add the object, too.

    The code is designed to minimize the number of "point in area" checks, it assumes that
    if one point of an object is in a area it is very likely that all points of the object
    are in the area.
    */
  static bool AddLocationWayToRegion(Region& region,
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
        for (size_t i=0; i<r->areas.size(); i++) {
          bool match=IsAreaAtLeastPartlyInArea(way.nodes,r->areas[i]);

          if (match) {
            bool completeMatch=AddLocationWayToRegion(*r,way,minlon,minlat,maxlon,maxlat);

            if (completeMatch) {
              // We are done, the object is completely enclosed by one of our sub areas
              return true;
            }
          }
        }
      }
    }

    // If we (at least partly) contain it, we add it to the area but continue

    region.locations[way.GetName()].objects.push_back(ObjectFileRef(way.GetFileOffset(),refWay));

    for (size_t i=0; i<region.areas.size(); i++) {
      if (IsAreaCompletelyInArea(way.nodes,region.areas[i])) {
        return true;
      }
    }

    return false;
  }

  static bool IndexLocationWays(const ImportParameter& parameter,
                                Progress& progress,
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

        AddLocationWayToRegion(rootArea,way,minlon,minlat,maxlon,maxlat);
      }
    }

    return scanner.Close();
  }

  static void AddLocationNodeToRegion(Region& region,
                                      const Node& node,
                                      const std::string& name,
                                      FileOffset offset)
  {
    for (std::list<Region>::iterator r=region.regions.begin();
         r!=region.regions.end();
         r++) {
      for (size_t i=0; i<r->areas.size(); i++) {
        if (IsCoordInArea(node,r->areas[i])) {
          AddLocationNodeToRegion(*r,node,name,offset);
          return;
        }
      }
    }

    region.locations[name].objects.push_back(ObjectFileRef(offset,refNode));
  }

  static bool IndexLocationNodes(const ImportParameter& parameter,
                                 Progress& progress,
                                 const OSMSCOUT_HASHSET<TypeId>& indexables,
                                 Region& rootRegion)
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
          AddLocationNodeToRegion(rootRegion,
                                  node,
                                  name,
                                  offset);
        }
      }
    }

    return scanner.Close();
  }


  static void AddAddressAreaToRegion(Progress& progress,
                                     Region& region,
                                     const Area& area,
                                     const std::vector<GeoCoord>& nodes,
                                     const Area::Ring& ring,
                                     double minlon,
                                     double minlat,
                                     double maxlon,
                                     double maxlat,
                                     bool& added)
  {
    for (std::list<Region>::iterator r=region.regions.begin();
         r!=region.regions.end();
         r++) {
      // Fast check, if the object is in the bounds of the area
      if (!(maxlon<r->minlon) &&
          !(minlon>r->maxlon) &&
          !(maxlat<r->minlat) &&
          !(minlat>r->maxlat)) {
        for (size_t i=0; i<r->areas.size(); i++) {
          if (IsAreaCompletelyInArea(nodes,r->areas[i])) {
            AddAddressAreaToRegion(progress,
                                   *r,
                                   area,
                                   nodes,
                                   ring,
                                   minlon,minlat,maxlon,maxlat,
                                   added);
            return;
          }
        }
      }
    }

    std::map<std::string,RegionLocation>::iterator location=region.locations.find(ring.GetAttributes().GetStreet());

    if (location==region.locations.end()) {
      progress.Debug(std::string("Street of address '")+ring.GetAttributes().GetStreet() +"' '"+ring.GetAttributes().GetHouseNr()+"' of Area "+NumberToString(area.GetFileOffset())+" cannot be resolved in region '"+region.name+"'");

      return;
    }

    for (std::list<RegionAddress>::const_iterator address=location->second.addresses.begin();
        address!=location->second.addresses.end();
        ++address) {
      if (address->houseNr==ring.GetAttributes().GetHouseNr()) {
        return;
      }
    }

    RegionAddress address;

    address.houseNr=ring.GetAttributes().GetHouseNr();
    address.object.Set(area.GetFileOffset(),refArea);

    location->second.addresses.push_back(address);

    added=true;
  }

  static void AddAddressAreaToRegion(Progress& progress,
                                     Region& region,
                                     const Area& area,
                                     const Area::Ring& ring,
                                     bool& added)
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

          AddAddressAreaToRegion(progress,
                                 region,
                                 area,
                                 r->nodes,
                                 ring,
                                 minlon,
                                 minlat,
                                 maxlon,
                                 maxlat,
                                 added);
        }
      }
    }
    else {
      ring.GetBoundingBox(minlon,maxlon,minlat,maxlat);

      AddAddressAreaToRegion(progress,
                             region,
                             area,
                             ring.nodes,
                             ring,
                             minlon,
                             minlat,
                             maxlon,
                             maxlat,
                             added);
    }
  }

  static bool IndexAddressAreas(const ImportParameter& parameter,
                                Progress& progress,
                                Region& rootRegion)
  {
    FileScanner scanner;
    uint32_t    areaCount;
    size_t      areasFound=0;
    size_t      areasInserted=0;

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
            !ring->GetAttributes().GetStreet().empty() &&
            !ring->GetAttributes().GetHouseNr().empty()) {
          bool added=false;

          areasFound++;

          AddAddressAreaToRegion(progress,
                                 rootRegion,
                                 area,
                                 *ring,
                                 added);

          if (added) {
            areasInserted++;
          }
        }
      }
    }

    progress.Info(NumberToString(areasFound)+" address areas found, "+NumberToString(areasInserted)+" inserted");

    return scanner.Close();
  }

  static bool AddAddressWayToRegion(Progress& progress,
                                    Region& region,
                                    const Way& way,
                                    double minlon,
                                    double minlat,
                                    double maxlon,
                                    double maxlat,
                                    bool& added)
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
        for (size_t i=0; i<r->areas.size(); i++) {
          bool match=IsAreaAtLeastPartlyInArea(way.nodes,r->areas[i]);

          if (match) {
            bool completeMatch=AddLocationWayToRegion(*r,way,minlon,minlat,maxlon,maxlat);

            if (completeMatch) {
              // We are done, the object is completely enclosed by one of our sub areas
              return true;
            }
          }
        }
      }
    }

    std::map<std::string,RegionLocation>::iterator location=region.locations.find(way.GetStreet());

    if (location==region.locations.end()) {
      progress.Debug(std::string("Street of address '")+way.GetStreet() +"' '"+way.GetHouseNr()+"' of Way "+NumberToString(way.GetFileOffset())+" cannot be resolved in region '"+region.name+"'");
    }
    else {
      for (std::list<RegionAddress>::const_iterator address=location->second.addresses.begin();
          address!=location->second.addresses.end();
          ++address) {
        if (address->houseNr==way.GetHouseNr()) {
          return false;
        }
      }

      RegionAddress address;

      address.houseNr=way.GetHouseNr();
      address.object.Set(way.GetFileOffset(),refWay);

      location->second.addresses.push_back(address);

      added=true;
    }

    for (size_t i=0; i<region.areas.size(); i++) {
      if (IsAreaCompletelyInArea(way.nodes,region.areas[i])) {
        return true;
      }
    }

    return false;
  }

  static bool IndexAddressWays(const ImportParameter& parameter,
                               Progress& progress,
                               Region& rootRegion)
  {
    FileScanner scanner;
    uint32_t    wayCount;
    size_t      waysFound=0;
    size_t      waysInserted=0;

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

      if (!way.GetStreet().empty() &&
          !way.GetHouseNr().empty()) {
        double minlon;
        double maxlon;
        double minlat;
        double maxlat;
        bool   added=false;

        waysFound++;

        way.GetBoundingBox(minlon,maxlon,minlat,maxlat);

        AddAddressWayToRegion(progress,
                              rootRegion,
                              way,
                              minlon,
                              minlat,
                              maxlon,
                              maxlat,
                              added);

      if (added) {
          waysInserted++;
        }
      }
    }

    progress.Info(NumberToString(waysFound)+" address ways found, "+NumberToString(waysInserted)+" inserted");

    return scanner.Close();
  }

  static void AddAddressNodeToRegion(Progress& progress,
                                     Region& region,
                                     const Node& node,
                                     bool& added)
  {
    for (std::list<Region>::iterator r=region.regions.begin();
         r!=region.regions.end();
         r++) {
      for (size_t i=0; i<r->areas.size(); i++) {
        if (IsCoordInArea(node,r->areas[i])) {
          AddAddressNodeToRegion(progress,
                                 *r,
                                 node,
                                 added);

          return;
        }
      }
    }

    std::map<std::string,RegionLocation>::iterator location=region.locations.find(node.GetStreet());

    if (location==region.locations.end()) {
      progress.Debug(std::string("Street of address '")+node.GetStreet() +"' '"+node.GetHouseNr()+"' of Node "+NumberToString(node.GetFileOffset())+" cannot be resolved in region '"+region.name+"'");
      return;
    }

    for (std::list<RegionAddress>::const_iterator address=location->second.addresses.begin();
        address!=location->second.addresses.end();
        ++address) {
      if (address->houseNr==node.GetHouseNr()) {
        return;
      }
    }

    RegionAddress address;

    address.houseNr=node.GetHouseNr();
    address.object.Set(node.GetFileOffset(),refNode);

    location->second.addresses.push_back(address);

    added=true;
  }

  static bool IndexAddressNodes(const ImportParameter& parameter,
                                Progress& progress,
                                Region& rootRegion)
  {
    FileScanner scanner;
    size_t      nodesFound=0;
    size_t      nodesInserted=0;
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

      if (!node.GetStreet().empty() &&
          !node.GetHouseNr().empty()) {
        bool added=false;

        nodesFound++;

        AddAddressNodeToRegion(progress,
                               rootRegion,
                               node,
                               added);
        if (added) {
          nodesInserted++;
        }
      }
    }

    progress.Info(NumberToString(nodesFound)+" address nodes found, "+NumberToString(nodesInserted)+" inserted");

    return scanner.Close();
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
                          Region& region,
                          FileOffset parentOffset)
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

    for (std::map<std::string,RegionLocation>::iterator location=region.locations.begin();
         location!=region.locations.end();
         ++location) {
      location->second.objects.sort(ObjectFileRefByFileOffsetComparator());

      writer.Write(location->first);
      writer.WriteNumber((uint32_t)location->second.objects.size()); // Number of objects

      FileOffset lastOffset=0;

      for (std::list<ObjectFileRef>::const_iterator object=location->second.objects.begin();
             object!=location->second.objects.end();
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

  static void GetRegionRefs(const Region& region,
                            std::map<std::string,std::list<RegionRef> >& locationRefs)
  {
    RegionRef regionRef;

    regionRef.reference=region.reference;
    regionRef.offset=region.offset;

    locationRefs[region.name].push_back(regionRef);

    for (std::list<RegionAlias>::const_iterator alias=region.aliases.begin();
         alias!=region.aliases.end();
         ++alias) {
      regionRef.reference.Set(alias->reference,refNode);
      regionRef.offset=region.offset;

      locationRefs[alias->name].push_back(regionRef);
    }

    for (std::list<Region>::const_iterator r=region.regions.begin();
         r!=region.regions.end();
         r++) {
      GetRegionRefs(*r,locationRefs);
    }
  }

  static bool WriteRegionRefs(FileWriter& writer,
                              std::map<std::string,std::list<RegionRef> >& locationRefs)
  {
    writer.WriteNumber((uint32_t)locationRefs.size());

    for (std::map<std::string,std::list<RegionRef> >::iterator n=locationRefs.begin();
         n!=locationRefs.end();
         ++n) {
      if (!writer.Write(n->first)) {
        return false;
      }

      if (!writer.WriteNumber((uint32_t)n->second.size())) {
        return false;
      }

      n->second.sort();

      FileOffset lastOffset=0;

      for (std::list<RegionRef>::const_iterator o=n->second.begin();
           o!=n->second.end();
           ++o) {
        if (!writer.Write((uint8_t)o->reference.GetType())) {
          return false;
        }

        if (!writer.WriteNumber(o->reference.GetFileOffset()-lastOffset)) {
          return false;
        }

        if (!writer.WriteFileOffset(o->offset)) {
          return false;
        }

        lastOffset=o->reference.GetFileOffset();
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

    boundaryId=typeConfig.GetAreaTypeId("boundary_administrative");
    assert(boundaryId!=typeIgnore);

    typeConfig.GetIndexables(indexables);

    //
    // Getting all areas of type 'administrative boundary'.
    //

    progress.SetAction("Scanning for administrative boundaries of type 'area'");

    if (!GetBoundaryAreas(parameter,
                          progress,
                          typeConfig,
                          boundaryId,
                          boundaryAreas)) {
      return false;
    }

    progress.Info(std::string("Found ")+NumberToString(boundaryAreas.size())+" areas of type 'administrative boundary'");

    for (size_t level=1; level<=10; level++) {
      progress.SetAction("Sorting in administrative boundaries of level "+NumberToString(level));

      SortInBoundaries(progress,
                       rootRegion,
                       boundaryAreas,
                       level);
    }

    boundaryAreas.clear();

    //
    // Getting all areas of type place=*.
    //

    progress.SetAction("Scanning for cities of type 'area'");

    if (!GetCityAreas(parameter,
                      typeConfig,
                      cityIds,
                      cityAreas,
                      progress)) {
      return false;
    }

    progress.Info(std::string("Found ")+NumberToString(cityAreas.size())+" cities of type 'area'");

    progress.SetAction("Sorting in city areas");

    SortInCityAreas(progress,
                    rootRegion,
                    cityAreas);

    cityAreas.clear();

    //
    // Getting all nodes of type place=*. We later need an area for these cities.
    //

    progress.SetAction("Scanning for cities of type 'node'");

    if (!GetCityNodes(parameter,
                      typeConfig,
                      cityIds,
                      cityNodes,
                      progress)) {
      return false;
    }

    progress.Info(std::string("Found ")+NumberToString(cityNodes.size())+" cities of type 'node'");

    progress.SetAction("Sort in city nodes");

    SortInCityNodes(progress,
                     rootRegion,
                     cityNodes);

    cityNodes.clear();

    progress.SetAction("Calculating bounds of areas");

    regionTree.resize(GetRegionTreeDepth(rootRegion));

    progress.Info(std::string("Area tree depth: ")+NumberToString(regionTree.size()));

    progress.SetAction("Sorting areas in levels");

    SortInRegion(rootRegion,regionTree,0);

    for (size_t i=0; i<regionTree.size(); i++) {
      progress.Info(std::string("Area tree index ")+NumberToString(i)+" size: "+NumberToString(regionTree[i].size()));
    }

    progress.SetAction("Index location areas");

    if (!IndexLocationAreas(parameter,
                            progress,
                            indexables,
                            rootRegion)) {
      return false;
    }

    progress.SetAction("Index location ways");

    if (!IndexLocationWays(parameter,
                           progress,
                           indexables,
                           rootRegion)) {
      return false;
    }

    progress.SetAction("Index location nodes");

    if (!IndexLocationNodes(parameter,
                            progress,
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

    progress.SetAction("Index address areas");

    if (!IndexAddressAreas(parameter,
                           progress,
                           rootRegion)) {
      return false;
    }

    progress.SetAction("Index address ways");

    if (!IndexAddressWays(parameter,
                          progress,
                          rootRegion)) {
      return false;
    }

    progress.SetAction("Index address nodes");

    if (!IndexAddressNodes(parameter,
                           progress,
                           rootRegion)) {
      return false;
    }

    progress.SetAction("Dumping areas");

    DumpRegionTree(progress,
                   rootRegion,
                   AppendFileToDir(parameter.GetDestinationDirectory(),
                                   "citystreet.txt"));

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

    //
    // Generate file with all area names, each referencing the areas where it is contained
    //

    std::map<std::string,std::list<RegionRef> > locationRefs;

    progress.SetAction(std::string("Write '")+CityStreetIndex::FILENAME_NAMEREGION_IDX+"'");

    GetRegionRefs(rootRegion,locationRefs);

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     CityStreetIndex::FILENAME_NAMEREGION_IDX))) {
      progress.Error("Cannot open '"+writer.GetFilename()+"'");
      return false;
    }

    WriteRegionRefs(writer,locationRefs);

    return !writer.HasError() && writer.Close();
  }
}
