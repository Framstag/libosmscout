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

#include <osmscout/Pixel.h>

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

  CityStreetIndexGenerator::RegionRef CityStreetIndexGenerator::RegionIndex::GetRegionForNode(RegionRef& rootRegion,
                                                                                              const GeoCoord& coord) const
  {
    size_t minX=(coord.GetLon()+180.0)/cellWidth;
    size_t minY=(coord.GetLat()+90.0)/cellHeight;

    std::map<Pixel,std::list<RegionRef> >::const_iterator indexCell=index.find(Pixel(minX,minY));

    if (indexCell!=index.end()) {
      for (std::list<RegionRef>::const_iterator r=indexCell->second.begin();
          r!=indexCell->second.end();
          ++r) {
        RegionRef region(*r);

        for (size_t i=0; i<region->areas.size(); i++) {
          if (IsCoordInArea(coord,region->areas[i])) {
            return region;
          }
        }
      }
    }

    return rootRegion;
  }

  void CityStreetIndexGenerator::DumpRegion(const Region& parent,
                                            size_t indent,
                                            std::ostream& out)
  {
    for (std::list<RegionRef>::const_iterator r=parent.regions.begin();
         r!=parent.regions.end();
         r++) {
      RegionRef childRegion(*r);

      for (size_t i=0; i<indent; i++) {
        out << " ";
      }
      out << " + " << childRegion->name << " " << childRegion->reference.GetTypeName() << " " << childRegion->reference.GetFileOffset() << std::endl;

      for (std::list<RegionAlias>::const_iterator l=childRegion->aliases.begin();
           l!=childRegion->aliases.end();
           l++) {
        for (size_t i=0; i<indent; i++) {
          out << " ";
        }
        out << " = " << l->name << " Node " << l->reference << std::endl;
      }

      for (std::list<RegionPOI>::const_iterator poi=childRegion->pois.begin();
          poi!=childRegion->pois.end();
          ++poi) {
        for (size_t i=0; i<indent; i++) {
          out << " ";
        }

        out << " * " << poi->name << " " << poi->object.GetTypeName() << " " << poi->object.GetFileOffset() << std::endl;
      }

      for (std::map<std::string,RegionLocation>::const_iterator nodeEntry=childRegion->locations.begin();
          nodeEntry!=childRegion->locations.end();
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

          out << " @ " << address->houseNr << " " << address->object.GetTypeName() << " " << address->object.GetFileOffset() << std::endl;
        }
      }

      DumpRegion(*childRegion,
                 indent+2,
                 out  );
    }
  }

  bool CityStreetIndexGenerator::DumpRegionTree(Progress& progress,
                                                const Region& rootRegion,
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

  void CityStreetIndexGenerator::AddRegion(Region& parent,
                                           const RegionRef& region)
  {
    for (std::list<RegionRef>::iterator r=parent.regions.begin();
         r!=parent.regions.end();
         r++) {
      RegionRef childRegion(*r);

      if (!(region->maxlon<childRegion->minlon) &&
          !(region->minlon>childRegion->maxlon) &&
          !(region->maxlat<childRegion->minlat) &&
          !(region->minlat>childRegion->maxlat)) {
        for (size_t i=0; i<region->areas.size(); i++) {
          for (size_t j=0; j<childRegion->areas.size(); j++) {
            if (IsAreaSubOfArea(region->areas[i],childRegion->areas[j])) {
              // If we already have the same name and are a "minor" reference, we skip...
              if (!(region->name==childRegion->name &&
                    region->reference.type<childRegion->reference.type)) {
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

  /**
    Return the list of ways of type administrative boundary.
    */
  bool CityStreetIndexGenerator::GetBoundaryAreas(const ImportParameter& parameter,
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

  void CityStreetIndexGenerator::SortInBoundaries(Progress& progress,
                                                  Region& rootRegion,
                                                  const std::list<Boundary>& boundaryAreas,
                                                  size_t level)
  {
    size_t currentBoundary=0;
    size_t maxBoundary=boundaryAreas.size();

    for (std::list<Boundary>::const_iterator boundary=boundaryAreas.begin();
         boundary!=boundaryAreas.end();
         ++boundary) {
      currentBoundary++;

      progress.SetProgress(currentBoundary,
                           maxBoundary);

      if (boundary->level!=level) {
        continue;
      }

      RegionRef region(new Region());

      region->reference=boundary->reference;
      region->name=boundary->name;

      region->areas=boundary->areas;

      region->CalculateMinMax();

      AddRegion(rootRegion,
                region);
    }
  }

  /**
    Return the list of nodes ids with the given type.
    */
  bool CityStreetIndexGenerator::IndexRegionAreas(const ImportParameter& parameter,
                                                  Progress& progress,
                                                  const TypeConfig& typeConfig,
                                                  const OSMSCOUT_HASHSET<TypeId>& regionTypes,
                                                  Region& rootRegion,
                                                  const RegionIndex& regionIndex)
  {
    FileScanner scanner;
    uint32_t    areaCount;
    size_t      areasFound=0;

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

      if (regionTypes.find(area.GetType())==regionTypes.end()) {
        continue;
      }

      if (area.rings.front().GetName().empty()) {
        progress.Warning(std::string("City area ")+
                         NumberToString(area.GetType())+" "+
                         NumberToString(area.GetFileOffset())+" has no name");
        continue;
      }

      RegionRef region=new Region();

      region->reference.Set(area.GetFileOffset(),refArea);
      region->name=area.rings.front().GetName();

      for (std::vector<Area::Ring>::const_iterator ring=area.rings.begin();
           ring!=area.rings.end();
           ++ring) {
        if (ring->ring==Area::outerRingId) {
          region->areas.push_back(ring->nodes);
        }
      }

      region->CalculateMinMax();

      AddRegion(rootRegion,
                region);

      areasFound++;
    }

    progress.Info(std::string("Found ")+NumberToString(areasFound)+" cities of type 'area'");

    return scanner.Close();
  }

  unsigned long CityStreetIndexGenerator::GetRegionTreeDepth(const Region& rootRegion)
  {
    unsigned long depth=0;

    for (std::list<RegionRef>::const_iterator a=rootRegion.regions.begin();
         a!=rootRegion.regions.end();
         a++) {
      RegionRef childRegion(*a);

      depth=std::max(depth,GetRegionTreeDepth(*childRegion));
    }

    return depth+1;
  }


  void CityStreetIndexGenerator::SortInRegion(RegionRef& area,
                                              std::vector<std::list<RegionRef> >& regionTree,
                                              unsigned long level)
  {
    regionTree[level].push_back(area);

    for (std::list<RegionRef>::iterator r=area->regions.begin();
         r!=area->regions.end();
         r++) {
      RegionRef childRegion(*r);

      SortInRegion(childRegion,
                   regionTree,
                   level+1);
    }
  }

  void CityStreetIndexGenerator::IndexRegions(const std::vector<std::list<RegionRef> >& regionTree,
                                              RegionIndex& regionIndex)
  {
    for (size_t level=regionTree.size()-1; level>=1; level--) {
      for (std::list<RegionRef>::const_iterator r=regionTree[level].begin();
           r!=regionTree[level].end();
           ++r) {
        RegionRef region(*r);

        size_t cellMinX=(region->minlon+180.0)/regionIndex.cellWidth;
        size_t cellMaxX=(region->maxlon+180.0)/regionIndex.cellWidth;
        size_t cellMinY=(region->minlat+90.0)/regionIndex.cellHeight;
        size_t cellMaxY=(region->maxlat+90.0)/regionIndex.cellHeight;

        for (size_t y=cellMinY; y<=cellMaxY; y++) {
          for (size_t x=cellMinX; x<=cellMaxX; x++) {
            Pixel pixel(x,y);

            regionIndex.index[pixel].push_back(region);
          }
        }
      }
    }
  }

  void CityStreetIndexGenerator::AddAliasToRegion(Region& region,
                                                  const RegionAlias& location,
                                                  const GeoCoord& node)
  {
    for (std::list<RegionRef>::iterator r=region.regions.begin();
         r!=region.regions.end();
         r++) {
      RegionRef childRegion(*r);

      for (size_t i=0; i<childRegion->areas.size(); i++) {
        if (IsCoordInArea(node,childRegion->areas[i])) {
          AddAliasToRegion(*childRegion,
                           location,
                           node);
          return;
        }
      }
    }

    if (region.name==location.name) {
      return;
    }

    region.aliases.push_back(location);
  }

  /**
    Return the list of nodes ids with the given type.
    */
  bool CityStreetIndexGenerator::IndexRegionNodes(const ImportParameter& parameter,
                                                  Progress& progress,
                                                  const TypeConfig& typeConfig,
                                                  const OSMSCOUT_HASHSET<TypeId>& regionTypes,
                                                  RegionRef& rootRegion,
                                                  const RegionIndex& regionIndex)
  {
    FileScanner scanner;
    uint32_t    nodeCount;
    size_t      citiesFound=0;

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

      if (regionTypes.find(node.GetType())!=regionTypes.end()) {
        std::string name=node.GetName();

        if (name.empty()) {
          progress.Warning(std::string("Node ")+NumberToString(node.GetFileOffset())+" has no name, skipping");
          continue;
        }

        RegionAlias alias;

        alias.reference=node.GetFileOffset();
        alias.name=name;

        GeoCoord coord(node.GetLat(),node.GetLon());

        RegionRef region=regionIndex.GetRegionForNode(rootRegion,
                                                      coord);

        AddAliasToRegion(region,
                         alias,
                         coord);

        citiesFound++;
      }
    }

    progress.Info(std::string("Found ")+NumberToString(citiesFound)+" cities of type 'node'");

    return scanner.Close();
  }

  bool CityStreetIndexGenerator::AddLocationAreaToRegion(Region& region,
                                                         const Area& area,
                                                         const std::vector<GeoCoord>& nodes,
                                                         const std::string& name,
                                                         double minlon,
                                                         double minlat,
                                                         double maxlon,
                                                         double maxlat)
  {
    for (std::list<RegionRef>::iterator r=region.regions.begin();
         r!=region.regions.end();
         r++) {
      RegionRef childRegion(*r);

      // Fast check, if the object is in the bounds of the area
      if (!(maxlon<childRegion->minlon) &&
          !(minlon>childRegion->maxlon) &&
          !(maxlat<childRegion->minlat) &&
          !(minlat>childRegion->maxlat)) {
        for (size_t i=0; i<childRegion->areas.size(); i++) {
          // Check if one point is in the area
          bool match=IsCoordInArea(nodes[0],childRegion->areas[i]);

          if (match) {
            bool completeMatch=AddLocationAreaToRegion(*r,area,nodes,name,minlon,minlat,maxlon,maxlat);

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
  void CityStreetIndexGenerator::AddLocationAreaToRegion(RegionRef& rootRegion,
                                                         const Area& area,
                                                         const Area::Ring& ring,
                                                         const RegionIndex& regionIndex)
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

          RegionRef region=regionIndex.GetRegionForNode(rootRegion,
                                                        GeoCoord(minlat,minlon));

          AddLocationAreaToRegion(region,
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

      RegionRef region=regionIndex.GetRegionForNode(rootRegion,
                                                    GeoCoord(minlat,minlon));

      AddLocationAreaToRegion(region,
                              area,
                              ring.nodes,
                              ring.GetName(),
                              minlon,
                              minlat,
                              maxlon,
                              maxlat);
    }
  }

  bool CityStreetIndexGenerator::IndexLocationAreas(const ImportParameter& parameter,
                                                    Progress& progress,
                                                    const OSMSCOUT_HASHSET<TypeId>& indexables,
                                                    RegionRef& rootRegion,
                                                    const RegionIndex& regionIndex)
  {
    FileScanner scanner;
    uint32_t    areaCount;
    size_t      areasFound=0;

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
            AddLocationAreaToRegion(rootRegion,
                                    area,
                                    *ring,
                                    regionIndex);

            areasFound++;
          }
        }

      }
    }

    progress.Info(std::string("Found ")+NumberToString(areasFound)+" locations of type 'area'");

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
  bool CityStreetIndexGenerator::AddLocationWayToRegion(Region& region,
                                                        const Way& way,
                                                        double minlon,
                                                        double minlat,
                                                        double maxlon,
                                                        double maxlat)
  {
    for (std::list<RegionRef>::iterator r=region.regions.begin();
         r!=region.regions.end();
         r++) {
      RegionRef childRegion(*r);

      // Fast check, if the object is in the bounds of the area
      if (!(maxlon<childRegion->minlon) &&
          !(minlon>childRegion->maxlon) &&
          !(maxlat<childRegion->minlat) &&
          !(minlat>childRegion->maxlat)) {
        // Check if one point is in the area
        for (size_t i=0; i<childRegion->areas.size(); i++) {
          bool match=IsAreaAtLeastPartlyInArea(way.nodes,childRegion->areas[i]);

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

  bool CityStreetIndexGenerator::IndexLocationWays(const ImportParameter& parameter,
                                                   Progress& progress,
                                                   const OSMSCOUT_HASHSET<TypeId>& indexables,
                                                   RegionRef& rootRegion,
                                                   const RegionIndex& regionIndex)
  {
    FileScanner scanner;
    uint32_t    wayCount;
    size_t      waysFound=0;

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

      if (indexables.find(way.GetType())==indexables.end()) {
        continue;
      }

      if (way.GetName().empty()) {
        continue;
      }

      double minlon;
      double maxlon;
      double minlat;
      double maxlat;

      way.GetBoundingBox(minlon,maxlon,minlat,maxlat);

      RegionRef region=regionIndex.GetRegionForNode(rootRegion,
                                                    GeoCoord(minlat,minlon));

      AddLocationWayToRegion(region,
                             way,
                             minlon,
                             minlat,
                             maxlon,
                             maxlat);

      waysFound++;
    }

    progress.Info(std::string("Found ")+NumberToString(waysFound)+" locations of type 'way'");

    return scanner.Close();
  }

  void CityStreetIndexGenerator::AddAddressAreaToRegion(Progress& progress,
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
    for (std::list<RegionRef>::iterator r=region.regions.begin();
         r!=region.regions.end();
         r++) {
      RegionRef childRegion(*r);

      // Fast check, if the object is in the bounds of the area
      if (!(maxlon<childRegion->minlon) &&
          !(minlon>childRegion->maxlon) &&
          !(maxlat<childRegion->minlat) &&
          !(minlat>childRegion->maxlat)) {
        for (size_t i=0; i<childRegion->areas.size(); i++) {
          if (IsAreaCompletelyInArea(nodes,childRegion->areas[i])) {
            AddAddressAreaToRegion(progress,
                                   childRegion,
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

  void CityStreetIndexGenerator::AddAddressAreaToRegion(Progress& progress,
                                                        RegionRef& rootRegion,
                                                        const Area& area,
                                                        const Area::Ring& ring,
                                                        const RegionIndex& regionIndex,
                                                        bool& added)
  {
    if (ring.ring==Area::masterRingId &&
        ring.nodes.empty()) {
      for (std::vector<Area::Ring>::const_iterator r=area.rings.begin();
          r!=area.rings.end();
          ++r) {
        if (r->ring==Area::outerRingId) {
          double minlon;
          double maxlon;
          double minlat;
          double maxlat;

          r->GetBoundingBox(minlon,maxlon,minlat,maxlat);

          RegionRef region=regionIndex.GetRegionForNode(rootRegion,
                                                        GeoCoord(minlat,minlon));

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
      double minlon;
      double maxlon;
      double minlat;
      double maxlat;

      ring.GetBoundingBox(minlon,maxlon,minlat,maxlat);

      RegionRef region=regionIndex.GetRegionForNode(rootRegion,
                                                    GeoCoord(minlat,minlon));

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

  void CityStreetIndexGenerator::AddPOIAreaToRegion(Progress& progress,
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
    for (std::list<RegionRef>::iterator r=region.regions.begin();
         r!=region.regions.end();
         r++) {
      RegionRef childRegion(*r);

      // Fast check, if the object is in the bounds of the area
      if (!(maxlon<childRegion->minlon) &&
          !(minlon>childRegion->maxlon) &&
          !(maxlat<childRegion->minlat) &&
          !(minlat>childRegion->maxlat)) {
        for (size_t i=0; i<childRegion->areas.size(); i++) {
          if (IsAreaCompletelyInArea(nodes,childRegion->areas[i])) {
            AddPOIAreaToRegion(progress,
                               childRegion,
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

    /*
    RegionAddress address;

    address.houseNr=ring.GetAttributes().GetHouseNr();
    address.object.Set(area.GetFileOffset(),refArea);

    location->second.addresses.push_back(address);*/

    added=true;
  }

  void CityStreetIndexGenerator::AddPOIAreaToRegion(Progress& progress,
                                                    RegionRef& rootRegion,
                                                    const Area& area,
                                                    const Area::Ring& ring,
                                                    const RegionIndex& regionIndex,
                                                    bool& added)
  {
    if (ring.ring==Area::masterRingId &&
        ring.nodes.empty()) {
      for (std::vector<Area::Ring>::const_iterator r=area.rings.begin();
          r!=area.rings.end();
          ++r) {
        if (r->ring==Area::outerRingId) {
          double minlon;
          double maxlon;
          double minlat;
          double maxlat;

          r->GetBoundingBox(minlon,maxlon,minlat,maxlat);

          RegionRef region=regionIndex.GetRegionForNode(rootRegion,
                                                        GeoCoord(minlat,minlon));

          AddPOIAreaToRegion(progress,
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
      double minlon;
      double maxlon;
      double minlat;
      double maxlat;

      ring.GetBoundingBox(minlon,maxlon,minlat,maxlat);

      RegionRef region=regionIndex.GetRegionForNode(rootRegion,
                                                    GeoCoord(minlat,minlon));

      AddPOIAreaToRegion(progress,
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

  bool CityStreetIndexGenerator::IndexAddressAreas(const ImportParameter& parameter,
                                                   Progress& progress,
                                                   RegionRef& rootRegion,
                                                   const OSMSCOUT_HASHSET<TypeId>& poiTypes,
                                                   const RegionIndex& regionIndex)
  {
    FileScanner scanner;
    uint32_t    areaCount;
    size_t      addressFound=0;
    size_t      poiFound=0;

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
        bool isAddress=ring->GetType()!=typeIgnore &&
                       !ring->GetAttributes().GetStreet().empty() &&
                       !ring->GetAttributes().GetHouseNr().empty();
        bool isPOI=ring->GetType()!=typeIgnore &&
                   !ring->GetAttributes().GetName().empty() &&
                   poiTypes.find(ring->GetType())!=poiTypes.end();

        if (!isAddress && !isPOI) {
          continue;
        }

        if (isAddress) {
          bool added=false;

          AddAddressAreaToRegion(progress,
                                 rootRegion,
                                 area,
                                 *ring,
                                 regionIndex,
                                 added);

          if (added) {
            addressFound++;
          }
        }

        if (isPOI) {
          bool added=false;

          AddPOIAreaToRegion(progress,
                             rootRegion,
                             area,
                             *ring,
                             regionIndex,
                             added);

          if (added) {
            poiFound++;
          }
        }
      }
    }

    progress.Info(NumberToString(areaCount)+" areas analyzed, "+NumberToString(addressFound)+" addresses founds, "+NumberToString(poiFound)+" POIs founds");

    return scanner.Close();
  }

  bool CityStreetIndexGenerator::AddAddressWayToRegion(Progress& progress,
                                                       Region& region,
                                                       const Way& way,
                                                       double minlon,
                                                       double minlat,
                                                       double maxlon,
                                                       double maxlat,
                                                       bool& added)
  {
    for (std::list<RegionRef>::iterator r=region.regions.begin();
         r!=region.regions.end();
         r++) {
      RegionRef childRegion(*r);

      // Fast check, if the object is in the bounds of the area
      if (!(maxlon<childRegion->minlon) &&
          !(minlon>childRegion->maxlon) &&
          !(maxlat<childRegion->minlat) &&
          !(minlat>childRegion->maxlat)) {
        // Check if one point is in the area
        for (size_t i=0; i<childRegion->areas.size(); i++) {
          bool match=IsAreaAtLeastPartlyInArea(way.nodes,childRegion->areas[i]);

          if (match) {
            bool completeMatch=AddAddressWayToRegion(progress,
                                                     *r,
                                                     way,
                                                     minlon,
                                                     minlat,
                                                     maxlon,
                                                     maxlat,
                                                     added);

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

  bool CityStreetIndexGenerator::AddPOIWayToRegion(Progress& progress,
                                                   Region& region,
                                                   const Way& way,
                                                   double minlon,
                                                   double minlat,
                                                   double maxlon,
                                                   double maxlat,
                                                   bool& added)
  {
    for (std::list<RegionRef>::iterator r=region.regions.begin();
         r!=region.regions.end();
         r++) {
      RegionRef childRegion(*r);

      // Fast check, if the object is in the bounds of the area
      if (!(maxlon<childRegion->minlon) &&
          !(minlon>childRegion->maxlon) &&
          !(maxlat<childRegion->minlat) &&
          !(minlat>childRegion->maxlat)) {
        // Check if one point is in the area
        for (size_t i=0; i<childRegion->areas.size(); i++) {
          bool match=IsAreaAtLeastPartlyInArea(way.nodes,childRegion->areas[i]);

          if (match) {
            bool completeMatch=AddAddressWayToRegion(progress,
                                                     *r,
                                                     way,
                                                     minlon,
                                                     minlat,
                                                     maxlon,
                                                     maxlat,
                                                     added);

            if (completeMatch) {
              // We are done, the object is completely enclosed by one of our sub areas
              return true;
            }
          }
        }
      }
    }

    RegionPOI poi;

    poi.name=way.GetName();
    poi.object.Set(way.GetFileOffset(),refWay);

    region.pois.push_back(poi);

    added=true;

    for (size_t i=0; i<region.areas.size(); i++) {
      if (IsAreaCompletelyInArea(way.nodes,region.areas[i])) {
        return true;
      }
    }

    return false;
  }

  bool CityStreetIndexGenerator::IndexAddressWays(const ImportParameter& parameter,
                                                  Progress& progress,
                                                  RegionRef& rootRegion,
                                                  const OSMSCOUT_HASHSET<TypeId>& poiTypes,
                                                  const RegionIndex& regionIndex)
  {
    FileScanner scanner;
    uint32_t    wayCount;
    size_t      addressFound=0;
    size_t      poiFound=0;

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

      bool isAddress=!way.GetStreet().empty() &&
                     !way.GetHouseNr().empty();
      bool isPOI=!way.GetName().empty() &&
                 poiTypes.find(way.GetType())!=poiTypes.end();

      if (!isAddress && !isPOI) {
        continue;
      }

      double minlon;
      double maxlon;
      double minlat;
      double maxlat;

      way.GetBoundingBox(minlon,maxlon,minlat,maxlat);

      RegionRef region=regionIndex.GetRegionForNode(rootRegion,
                                                    GeoCoord(minlat,minlon));

      if (isAddress) {
        bool added=false;

        AddAddressWayToRegion(progress,
                              region,
                              way,
                              minlon,
                              minlat,
                              maxlon,
                              maxlat,
                              added);

      if (added) {
        addressFound++;
        }
      }

      if (isPOI) {
        bool added=false;

        AddPOIWayToRegion(progress,
                          region,
                          way,
                          minlon,
                          minlat,
                          maxlon,
                          maxlat,
                          added);

      if (added) {
          poiFound++;
        }
      }
    }

    progress.Info(NumberToString(wayCount)+" ways analyzed, "+NumberToString(addressFound)+" addresses founds, "+NumberToString(poiFound)+" POIs founds");

    return scanner.Close();
  }

  void CityStreetIndexGenerator::AddAddressNodeToRegion(Progress& progress,
                                                        Region& region,
                                                        const Node& node,
                                                        bool& added)
  {
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

  void CityStreetIndexGenerator::AddPOINodeToRegion(Progress& progress,
                                                    Region& region,
                                                    const Node& node,
                                                    bool& added)
  {
    RegionPOI poi;

    poi.name=node.GetName();
    poi.object.Set(node.GetFileOffset(),refNode);

    region.pois.push_back(poi);

    added=true;
  }

  bool CityStreetIndexGenerator::IndexAddressNodes(const ImportParameter& parameter,
                                                   Progress& progress,
                                                   RegionRef& rootRegion,
                                                   const OSMSCOUT_HASHSET<TypeId>& poiTypes,
                                                   const RegionIndex& regionIndex)
  {
    FileScanner scanner;
    uint32_t    nodeCount;
    size_t      addressFound=0;
    size_t      poiFound=0;

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

      bool isAddress=!node.GetStreet().empty() &&
                     !node.GetHouseNr().empty();
      bool isPOI=!node.GetName().empty() &&
                 poiTypes.find(node.GetType())!=poiTypes.end();

      if (!isAddress && !isPOI) {
        continue;
      }

      RegionRef region=regionIndex.GetRegionForNode(rootRegion,
                                                    GeoCoord(node.GetLat(),
                                                             node.GetLon()));

      if (!region.Valid()) {
        continue;
      }

      if (isAddress) {
        bool added=false;

        AddAddressNodeToRegion(progress,
                               rootRegion,
                               node,
                               added);
        if (added) {
          addressFound++;
        }
      }

      if (isPOI) {
        bool added=false;

        AddPOINodeToRegion(progress,
                           rootRegion,
                           node,
                           added);
        if (added) {
          poiFound++;
        }
      }
    }

    progress.Info(NumberToString(nodeCount)+" nodes analyzed, "+NumberToString(addressFound)+" addresses founds, "+NumberToString(poiFound)+" POIs founds");

    return scanner.Close();
  }

  bool CityStreetIndexGenerator::WriteRegionIndexEntry(FileWriter& writer,
                                                       const Region& parentRegion,
                                                       Region& region)
  {
    if (!writer.GetPos(region.indexOffset)) {
      return false;
    }

    writer.WriteFileOffset(region.dataOffset);
    writer.WriteFileOffset(parentRegion.indexOffset);

    writer.Write(region.name);

    writer.Write((uint8_t)region.reference.GetType());
    writer.WriteFileOffset(region.reference.GetFileOffset());

    writer.WriteNumber((uint32_t)region.aliases.size());
    for (std::list<RegionAlias>::const_iterator alias=region.aliases.begin();
        alias!=region.aliases.end();
        ++alias) {
      writer.Write(alias->name);
      writer.WriteFileOffset(alias->reference);
    }

    writer.WriteNumber((uint32_t)region.regions.size());
    for (std::list<RegionRef>::iterator r=region.regions.begin();
         r!=region.regions.end();
         r++) {
      RegionRef childRegion(*r);

      if (!WriteRegionIndexEntry(writer,
                                 region,
                                 *childRegion)) {
        return false;
      }
    }

    return !writer.HasError();
  }

  bool CityStreetIndexGenerator::WriteRegionIndex(FileWriter& writer,
                                                  Region& rootRegion)
  {
    writer.WriteNumber(rootRegion.regions.size());
    for (std::list<RegionRef>::iterator r=rootRegion.regions.begin();
         r!=rootRegion.regions.end();
         ++r) {
      RegionRef childRegion(*r);

      if (!WriteRegionIndexEntry(writer,
                                 rootRegion,
                                 *childRegion)) {
        return false;
      }
    }

    return true;
  }

  bool CityStreetIndexGenerator::WriteRegionDataEntry(FileWriter& writer,
                                                      const Region& parentRegion,
                                                      Region& region)
  {
    if (!writer.GetPos(region.dataOffset)) {
      return false;
    }

    if (!writer.SetPos(region.indexOffset)) {
      return false;
    }

    if (!writer.WriteFileOffset(region.dataOffset)) {
      return false;
    }

    if (!writer.SetPos(region.dataOffset)) {
      return false;
    }

    writer.WriteNumber((uint32_t)region.locations.size());
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

    for (std::list<RegionRef>::iterator r=region.regions.begin();
         r!=region.regions.end();
         r++) {
      RegionRef childRegion(*r);

      if (!WriteRegionDataEntry(writer,
                                region,
                                *childRegion)) {
        return false;
      }
    }

    return !writer.HasError();
  }

  bool CityStreetIndexGenerator::WriteRegionData(FileWriter& writer,
                                                 Region& rootRegion)
  {
    writer.WriteNumber(rootRegion.regions.size());
    for (std::list<RegionRef>::iterator r=rootRegion.regions.begin();
         r!=rootRegion.regions.end();
         ++r) {
      RegionRef childRegion(*r);

      if (!WriteRegionDataEntry(writer,
                                rootRegion,
                                *childRegion)) {
        return false;
      }
    }

    return true;
  }

  std::string CityStreetIndexGenerator::GetDescription() const
  {
    return "Generate 'region.dat'";
  }

  bool CityStreetIndexGenerator::Import(const ImportParameter& parameter,
                                        Progress& progress,
                                        const TypeConfig& typeConfig)
  {
    OSMSCOUT_HASHSET<TypeId>           regionTypes;
    OSMSCOUT_HASHSET<TypeId>           poiTypes;
    OSMSCOUT_HASHSET<TypeId>           indexables;
    TypeId                             boundaryId;
    RegionRef                          rootRegion;
    std::list<Boundary>                boundaryAreas;
    std::vector<std::list<RegionRef> > regionTree;
    RegionIndex                        regionIndex;


    progress.SetAction("Setup");

    rootRegion=new Region();
    rootRegion->name="<root>";
    rootRegion->indexOffset=0;
    rootRegion->dataOffset=0;

    boundaryId=typeConfig.GetAreaTypeId("boundary_administrative");
    assert(boundaryId!=typeIgnore);

    typeConfig.GetIndexables(indexables);

    typeConfig.GetIndexAsRegionTypes(regionTypes);
    typeConfig.GetIndexAsPOITypes(poiTypes);

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
                       *rootRegion,
                       boundaryAreas,
                       level);
    }

    boundaryAreas.clear();

    //
    // Getting all areas of type place=*.
    //

    progress.SetAction("Indexing cities of type 'area'");

    if (!IndexRegionAreas(parameter,
                          progress,
                          typeConfig,
                          regionTypes,
                          *rootRegion,
                          regionIndex)) {
      return false;
    }

    progress.SetAction("Calculating region tree depth");

    regionTree.resize(GetRegionTreeDepth(rootRegion));

    progress.Info(std::string("Area tree depth: ")+NumberToString(regionTree.size()));

    progress.SetAction("Sorting regions by levels");

    SortInRegion(rootRegion,
                 regionTree,
                 0);

    for (size_t i=0; i<regionTree.size(); i++) {
      progress.Info(std::string("Area tree index ")+NumberToString(i)+" size: "+NumberToString(regionTree[i].size()));
    }

    progress.SetAction("Index regions");

    regionIndex.cellWidth=360.0/pow(2.0,16);
    regionIndex.cellHeight=180.0/pow(2.0,16);

    IndexRegions(regionTree,
                 regionIndex);

    //
    // Getting all nodes of type place=*. We later need an area for these cities.
    //

    progress.SetAction("Indexing cities of type 'node' as area aliases");

    if (!IndexRegionNodes(parameter,
                          progress,
                          typeConfig,
                          regionTypes,
                          rootRegion,
                          regionIndex)) {
      return false;
    }

    progress.SetAction("Index location areas");

    if (!IndexLocationAreas(parameter,
                            progress,
                            indexables,
                            rootRegion,
                            regionIndex)) {
      return false;
    }

    progress.SetAction("Index location ways");

    if (!IndexLocationWays(parameter,
                           progress,
                           indexables,
                           rootRegion,
                           regionIndex)) {
      return false;
    }

    for (size_t i=0; i<regionTree.size(); i++) {
      unsigned long count=0;

      for (std::list<RegionRef>::const_iterator iter=regionTree[i].begin();
           iter!=regionTree[i].end();
           ++iter) {
        count+=(*iter)->locations.size();
      }

      progress.Info(std::string("Area tree index ")+NumberToString(i)+" object count size: "+NumberToString(count));
    }

    progress.SetAction("Index address areas");

    if (!IndexAddressAreas(parameter,
                           progress,
                           rootRegion,
                           poiTypes,
                           regionIndex)) {
      return false;
    }

    progress.SetAction("Index address ways");

    if (!IndexAddressWays(parameter,
                          progress,
                          rootRegion,
                          poiTypes,
                          regionIndex)) {
      return false;
    }

    progress.SetAction("Index address nodes");

    if (!IndexAddressNodes(parameter,
                           progress,
                           rootRegion,
                           poiTypes,
                           regionIndex)) {
      return false;
    }

    progress.SetAction("Dumping areas");

    DumpRegionTree(progress,
                   *rootRegion,
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

    if (!WriteRegionIndex(writer,
                          *rootRegion)) {
      return false;
    }

    if (!WriteRegionData(writer,
                         *rootRegion)) {
      return false;
    }

    if (writer.HasError() || !writer.Close()) {
      return false;
    }

    return true;
  }
}
