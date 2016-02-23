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

#include <osmscout/import/GenLocationIndex.h>

#include <iostream>
#include <fstream>
#include <limits>
#include <list>
#include <map>
#include <set>

#include <osmscout/Pixel.h>

#include <osmscout/TypeFeatures.h>

#include <osmscout/LocationIndex.h>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

#include <osmscout/util/File.h>
#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>
#include <osmscout/util/GeoBox.h>
#include <osmscout/util/Geometry.h>
#include <osmscout/util/String.h>
#include <osmscout/AreaDataFile.h>
#include <osmscout/NodeDataFile.h>
#include <osmscout/WayDataFile.h>
#include <osmscout/import/SortWayDat.h>
#include <osmscout/import/SortNodeDat.h>
#include <osmscout/import/GenAreaAreaIndex.h>

namespace osmscout {

  static const size_t REGION_INDEX_LEVEL=16;

  LocationIndexGenerator::RegionRef LocationIndexGenerator::RegionIndex::GetRegionForNode(RegionRef& rootRegion,
                                                                                          const GeoCoord& coord) const
  {
    uint32_t minX=(uint32_t)((coord.GetLon()+180.0)/cellWidth);
    uint32_t minY=(uint32_t)((coord.GetLat()+90.0)/cellHeight);

    std::map<Pixel,std::list<RegionRef> >::const_iterator indexCell=index.find(Pixel(minX,minY));

    if (indexCell!=index.end()) {
      for (const auto& region : indexCell->second) {
        for (size_t i=0; i<region->areas.size(); i++) {
          if (IsCoordInArea(coord,region->areas[i])) {
            return region;
          }
        }
      }
    }

    return rootRegion;
  }

  void LocationIndexGenerator::Write(FileWriter& writer,
                                     const ObjectFileRef& object)
  {
    writer.Write((uint8_t)object.GetType());

    switch (object.GetType()) {
    case refNone:
      // TODO: Why are we called with refNone?
      break;
    case refNode:
      writer.WriteFileOffset(object.GetFileOffset(),
                             bytesForNodeFileOffset);
    case refArea:
      writer.WriteFileOffset(object.GetFileOffset(),
                             bytesForAreaFileOffset);
    case refWay:
      writer.WriteFileOffset(object.GetFileOffset(),
                             bytesForWayFileOffset);
    }
  }

  void LocationIndexGenerator::AnalyseStringForIgnoreTokens(const std::string& string,
                                                            std::unordered_map<std::string,size_t>& ignoreTokens,
                                                            std::unordered_set<std::string>& blacklist)
  {
    if (string.empty()) {
      return;
    }

    std::list<std::string> tokens;

    SplitStringAtSpace(string,
                       tokens);

    if (tokens.size()>1) {
      for (std::list<std::string>::const_iterator token=tokens.begin();
          token!=tokens.end();
          ++token) {
        if (token->length()<=5) {
          auto entry=ignoreTokens.find(*token);

          if (entry==ignoreTokens.end()) {
            ignoreTokens.insert(std::make_pair(*token,1));
          }
          else {
            entry->second++;
          }
        }

        std::list<std::string>::const_iterator nextToken=token;
        nextToken++;

        if (nextToken!=tokens.end() && nextToken->length()<=5) {
          std::string composition=*token+" "+*nextToken;

          auto entry=ignoreTokens.find(composition);

          if (entry==ignoreTokens.end()) {
            ignoreTokens.insert(std::make_pair(composition,1));
          }
          else {
            entry->second++;
          }
        }
      }
    }
    else if (tokens.size()==1 &&
             tokens.front().length()<=5) {
      blacklist.insert(tokens.front());
    }
    else if (tokens.size()==2) {
      std::list<std::string>::const_iterator token=tokens.begin();
      std::list<std::string>::const_iterator nextToken=token;

      if (token->length()<=5 &&
          nextToken->length()<=5) {
        blacklist.insert(*token+" "+*nextToken);
      }
    }
  }

  void LocationIndexGenerator::CalculateRegionNameIgnoreTokens(const Region& region,
                                                               std::unordered_map<std::string,size_t>& ignoreTokens,
                                                               std::unordered_set<std::string>& blacklist)
  {
    AnalyseStringForIgnoreTokens(region.name,
                                 ignoreTokens,
                                 blacklist);

    for (const auto& alias : region.aliases) {
      AnalyseStringForIgnoreTokens(alias.name,
                                   ignoreTokens,
                                   blacklist);
    }

    /*
    for (const auto& poi : region.pois) {
      AnalyseStringForIgnoreTokens(poi.name,
                                ignoreTokenCounts);
    }

    for (const auto& nodeEntry : region.locations) {
      AnalyseStringForIgnoreTokens(nodeEntry.first,
                                ignoreTokenCounts);
    }*/

    // recursion...

    for (const auto& childRegion : region.regions) {
      CalculateRegionNameIgnoreTokens(*childRegion,
                                      ignoreTokens,
                                      blacklist);
    }
  }

  void LocationIndexGenerator::CalculateLocationNameIgnoreTokens(const Region& region,
                                                                 std::unordered_map<std::string,size_t>& ignoreTokens,
                                                                 std::unordered_set<std::string>& blacklist)
  {
    for (const auto& poi : region.pois) {
      AnalyseStringForIgnoreTokens(poi.name,
                                   ignoreTokens,
                                   blacklist);
    }

    for (const auto& nodeEntry : region.locations) {
      AnalyseStringForIgnoreTokens(nodeEntry.first,
                                   ignoreTokens,
                                   blacklist);
    }

    // recursion...

    for (const auto& childRegion : region.regions) {
      CalculateLocationNameIgnoreTokens(*childRegion,
                                        ignoreTokens,
                                        blacklist);
    }
  }

  bool LocationIndexGenerator::CalculateIgnoreTokens(const Region& rootRegion,
                                                     std::list<std::string>& regionTokens,
                                                     std::list<std::string>& locationTokens)
  {
    std::unordered_map<std::string,size_t> ignoreTokens;
    std::unordered_set<std::string>        blacklist;

    regionTokens.clear();
    locationTokens.clear();

    CalculateRegionNameIgnoreTokens(rootRegion,
                                    ignoreTokens,
                                    blacklist);

    size_t limit=ignoreTokens.size()/100;

    if (limit<5) {
      limit=5;
    }

    for (const auto& tokenEntry : ignoreTokens) {
      if (tokenEntry.second>=limit &&
          blacklist.find(tokenEntry.first)==blacklist.end()) {
        regionTokens.push_back(tokenEntry.first);
      }
    }

    ignoreTokens.clear();
    blacklist.clear();

    CalculateLocationNameIgnoreTokens(rootRegion,
                                      ignoreTokens,
                                      blacklist);

    limit=ignoreTokens.size()/100;

    if (limit<5) {
      limit=5;
    }

    for (const auto& tokenEntry : ignoreTokens) {
      if (tokenEntry.second>=limit &&
          blacklist.find(tokenEntry.first)==blacklist.end()) {
        locationTokens.push_back(tokenEntry.first);
      }
    }

    return true;
  }

  void LocationIndexGenerator::DumpRegion(const Region& parent,
                                          size_t indent,
                                          std::ostream& out)
  {
    for (const auto& childRegion : parent.regions) {
      for (size_t i=0; i<indent; i++) {
        out << " ";
      }
      out << " + " << childRegion->name << " " << childRegion->reference.GetTypeName() << " " << childRegion->reference.GetFileOffset() << std::endl;

      for (const auto& alias : childRegion->aliases) {
        for (size_t i=0; i<indent+2; i++) {
          out << " ";
        }
        out << " = " << alias.name << " Node " << alias.reference << std::endl;
      }

      DumpRegion(*childRegion,
                 indent+2,
                 out);
    }
  }

  bool LocationIndexGenerator::DumpRegionTree(Progress& progress,
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

  void LocationIndexGenerator::DumpRegionAndData(const Region& parent,
                                                 size_t indent,
                                                 std::ostream& out)
  {
    for (const auto& childRegion : parent.regions) {
      for (size_t i=0; i<indent; i++) {
        out << " ";
      }
      out << " + " << childRegion->name << " " << childRegion->reference.GetTypeName() << " " << childRegion->reference.GetFileOffset() << std::endl;

      for (const auto& alias : childRegion->aliases) {
        for (size_t i=0; i<indent+2; i++) {
          out << " ";
        }
        out << " = " << alias.name << " Node " << alias.reference << std::endl;
      }

      for (const auto& poi : childRegion->pois) {
        for (size_t i=0; i<indent+2; i++) {
          out << " ";
        }

        out << " * " << poi.name << " " << poi.object.GetTypeName() << " " << poi.object.GetFileOffset() << std::endl;
      }

      for (const auto& nodeEntry : childRegion->locations) {
        for (size_t i=0; i<indent+2; i++) {
          out << " ";
        }
        out << " - " << nodeEntry.first << std::endl;

        for (const auto& object : nodeEntry.second.objects) {
          for (size_t i=0; i<indent+4; i++) {
            out << " ";
          }

          out << " = " << object.GetTypeName() << " " << object.GetFileOffset() << std::endl;
        }

        for (const auto& address : nodeEntry.second.addresses) {
          for (size_t i=0; i<indent+6; i++) {
            out << " ";
          }

          out << " @ " << address.name << " " << address.object.GetTypeName() << " " << address.object.GetFileOffset() << std::endl;
        }
      }

      DumpRegionAndData(*childRegion,
                        indent+2,
                        out);
    }
  }

  bool LocationIndexGenerator::DumpLocationTree(Progress& progress,
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

    DumpRegionAndData(rootRegion,
                      0,
                      debugStream);
    debugStream.close();

    return true;
  }

  void LocationIndexGenerator::AddRegion(Region& parent,
                                         const RegionRef& region)
  {
    for (const auto& childRegion : parent.regions) {
      if (!(region->maxlon<childRegion->minlon) &&
          !(region->minlon>childRegion->maxlon) &&
          !(region->maxlat<childRegion->minlat) &&
          !(region->minlat>childRegion->maxlat)) {
        for (size_t i=0; i<region->areas.size(); i++) {
          for (size_t j=0; j<childRegion->areas.size(); j++) {
            if (IsAreaSubOfAreaQuorum(region->areas[i],childRegion->areas[j])) {
              // If we already have the same name and are a "minor" reference, we skip...
              if (!(region->name==childRegion->name &&
                    region->reference.type<childRegion->reference.type)) {
                AddRegion(*childRegion,region);
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
  bool LocationIndexGenerator::GetBoundaryAreas(const ImportParameter& parameter,
                                                Progress& progress,
                                                const TypeConfigRef& typeConfig,
                                                const TypeInfoSet& boundaryTypes,
                                                std::list<Boundary>& boundaryAreas)
  {
    FileScanner                  scanner;
    uint32_t                     areaCount;
    NameFeatureValueReader       nameReader(*typeConfig);
    AdminLevelFeatureValueReader adminLevelReader(*typeConfig);

    try {
      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   AreaDataFile::AREAS_DAT),
                   FileScanner::Sequential,
                   true);

      if (!scanner.Read(areaCount)) {
        progress.Error("Error while reading number of data entries in file");
        return false;
      }

      for (uint32_t r=1; r<=areaCount; r++) {
        progress.SetProgress(r,areaCount);

        Area area;

        if (!area.Read(*typeConfig,
                       scanner)) {
          progress.Error(std::string("Error while reading data entry ")+
                         NumberToString(r)+" of "+
                         NumberToString(areaCount)+
                         " in file '"+
                         scanner.GetFilename()+"'");
          return false;
        }

        if (!boundaryTypes.IsSet(area.GetType())) {
          continue;
        }

        NameFeatureValue *nameValue=nameReader.GetValue(area.rings.front().GetFeatureValueBuffer());

        if (nameValue==NULL) {
          progress.Warning(std::string("Boundary area ")+
                           area.GetType()->GetName()+" "+
                           NumberToString(area.GetFileOffset())+" has no name");
          continue;
        }

        AdminLevelFeatureValue *adminLevelValue=adminLevelReader.GetValue(area.rings.front().GetFeatureValueBuffer());

        if (adminLevelValue!=NULL) {
          Boundary boundary;

          boundary.reference.Set(area.GetFileOffset(),refArea);
          boundary.name=nameValue->GetName();
          boundary.level=adminLevelValue->GetAdminLevel();

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
          progress.Info("No tag 'admin_level' for relation "+
                        area.GetType()->GetName()+" "+
                        NumberToString(area.GetFileOffset()));
        }
      }

      scanner.Close();
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      return false;
    }

    return true;
  }

  void LocationIndexGenerator::SortInBoundaries(Progress& progress,
                                                Region& rootRegion,
                                                const std::list<Boundary>& boundaryAreas,
                                                size_t level)
  {
    size_t currentBoundary=0;
    size_t maxBoundary=boundaryAreas.size();

    for (const auto&  boundary : boundaryAreas) {
      currentBoundary++;

      progress.SetProgress(currentBoundary,
                           maxBoundary);

      if (boundary.level!=level) {
        continue;
      }

      RegionRef region(new Region());

      region->reference=boundary.reference;
      region->name=boundary.name;
      region->areas=boundary.areas;

      region->CalculateMinMax();

      AddRegion(rootRegion,
                region);
    }
  }

  /**
    Return the list of nodes ids with the given type.
    */
  bool LocationIndexGenerator::IndexRegionAreas(const TypeConfig& typeConfig,
                                                const ImportParameter& parameter,
                                                Progress& progress,
                                                Region& rootRegion)
  {
    FileScanner            scanner;
    uint32_t               areaCount;
    size_t                 areasFound=0;
    NameFeatureValueReader nameReader(typeConfig);

    try {
      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   AreaDataFile::AREAS_DAT),
                   FileScanner::Sequential,
                   parameter.GetWayDataMemoryMaped());

      if (!scanner.Read(areaCount)) {
        progress.Error("Error while reading number of data entries in file");
        return false;
      }

      for (uint32_t a=1; a<=areaCount; a++) {
        progress.SetProgress(a,areaCount);

        Area area;

        if (!area.Read(typeConfig,
                       scanner)) {
          progress.Error(std::string("Error while reading data entry ")+
                         NumberToString(a)+" of "+
                         NumberToString(areaCount)+
                         " in file '"+
                         scanner.GetFilename()+"'");
          return false;
        }

        if (!area.GetType()->GetIndexAsRegion()) {
          continue;
        }

        NameFeatureValue *nameValue=nameReader.GetValue(area.rings.front().GetFeatureValueBuffer());

        if (nameValue==NULL) {
          progress.Warning(std::string("Region ")+
                           area.GetType()->GetName()+" "+
                           NumberToString(area.GetFileOffset())+" has no name");
          continue;
        }


        RegionRef region=std::make_shared<Region>();

        region->reference.Set(area.GetFileOffset(),refArea);
        region->name=nameValue->GetName();

        for (const auto& ring : area.rings) {
          if (ring.ring==Area::outerRingId) {
            region->areas.push_back(ring.nodes);
          }
        }

        region->CalculateMinMax();

        AddRegion(rootRegion,
                  region);

        areasFound++;
      }

      progress.Info(std::string("Found ")+NumberToString(areasFound)+" cities of type 'area'");

      scanner.Close();
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      return false;
    }

    return true;
  }

  unsigned long LocationIndexGenerator::GetRegionTreeDepth(const Region& rootRegion)
  {
    unsigned long depth=0;

    for (const auto& childRegion : rootRegion.regions) {
      depth=std::max(depth,GetRegionTreeDepth(*childRegion));
    }

    return depth+1;
  }


  void LocationIndexGenerator::SortInRegion(RegionRef& area,
                                            std::vector<std::list<RegionRef> >& regionTree,
                                            unsigned long level)
  {
    regionTree[level].push_back(area);

    for (auto& childRegion : area->regions) {
      SortInRegion(childRegion,
                   regionTree,
                   level+1);
    }
  }

  void LocationIndexGenerator::IndexRegions(const std::vector<std::list<RegionRef> >& regionTree,
                                            RegionIndex& regionIndex)
  {
    for (size_t level=regionTree.size()-1; level>=1; level--) {
      for (const auto& region : regionTree[level]) {
        uint32_t cellMinX=(uint32_t)((region->minlon+180.0)/regionIndex.cellWidth);
        uint32_t cellMaxX=(uint32_t)((region->maxlon+180.0)/regionIndex.cellWidth);
        uint32_t cellMinY=(uint32_t)((region->minlat+90.0)/regionIndex.cellHeight);
        uint32_t cellMaxY=(uint32_t)((region->maxlat+90.0)/regionIndex.cellHeight);

        for (uint32_t y=cellMinY; y<=cellMaxY; y++) {
          for (uint32_t x=cellMinX; x<=cellMaxX; x++) {
            Pixel pixel(x,y);

            regionIndex.index[pixel].push_back(region);
          }
        }
      }
    }
  }

  void LocationIndexGenerator::AddAliasToRegion(Region& region,
                                                const RegionAlias& location,
                                                const GeoCoord& node)
  {
    for (const auto& childRegion : region.regions) {
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
  bool LocationIndexGenerator::IndexRegionNodes(const TypeConfigRef& typeConfig,
                                                const ImportParameter& parameter,
                                                Progress& progress,
                                                RegionRef& rootRegion,
                                                const RegionIndex& regionIndex)
  {
    FileScanner            scanner;
    uint32_t               nodeCount;
    size_t                 citiesFound=0;
    NameFeatureValueReader nameReader(*typeConfig);

    try {
      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   NodeDataFile::NODES_DAT),
                   FileScanner::Sequential,
                   true);

      if (!scanner.Read(nodeCount)) {
        progress.Error("Error while reading number of data entries in file");
        return false;
      }

      for (uint32_t n=1; n<=nodeCount; n++) {
        progress.SetProgress(n,nodeCount);

        Node node;

        if (!node.Read(*typeConfig,
                       scanner)) {
          progress.Error(std::string("Error while reading data entry ")+
                         NumberToString(n)+" of "+
                         NumberToString(nodeCount)+
                         " in file '"+
                         scanner.GetFilename()+"'");
          return false;
        }

        if (node.GetType()->GetIndexAsRegion()) {
          NameFeatureValue *nameValue=nameReader.GetValue(node.GetFeatureValueBuffer());

          if (nameValue==NULL) {
            progress.Warning(std::string("Node ")+NumberToString(node.GetFileOffset())+" has no name, skipping");
            continue;
          }

          RegionAlias alias;

          alias.reference=node.GetFileOffset();
          alias.name=nameValue->GetName();

          RegionRef region=regionIndex.GetRegionForNode(rootRegion,
                                                        node.GetCoords());

          AddAliasToRegion(*region,
                           alias,
                           node.GetCoords());

          citiesFound++;
        }
      }

      progress.Info(std::string("Found ")+NumberToString(citiesFound)+" cities of type 'node'");

      scanner.Close();
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      return false;
    }

    return true;
  }

  bool LocationIndexGenerator::AddLocationAreaToRegion(Region& region,
                                                       const Area& area,
                                                       const std::vector<GeoCoord>& nodes,
                                                       const std::string& name,
                                                       double minlon,
                                                       double minlat,
                                                       double maxlon,
                                                       double maxlat)
  {
    for (const auto& childRegion : region.regions) {
      // Fast check, if the object is in the bounds of the area
      if (!(maxlon<childRegion->minlon) &&
          !(minlon>childRegion->maxlon) &&
          !(maxlat<childRegion->minlat) &&
          !(minlat>childRegion->maxlat)) {
        for (size_t i=0; i<childRegion->areas.size(); i++) {
          // Check if one point is in the area
          bool match=IsCoordInArea(nodes[0],childRegion->areas[i]);

          if (match) {
            bool completeMatch=AddLocationAreaToRegion(*childRegion,area,nodes,name,minlon,minlat,maxlon,maxlat);

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
    Add the given location area to the hierarchical area index.

    The code is designed to minimize the number of "point in area" checks, it assumes that
    if one point of an object is in a area it is very likely that all points of the object
    are in the area.
    */
  void LocationIndexGenerator::AddLocationAreaToRegion(RegionRef& rootRegion,
                                                       const Area& area,
                                                       const Area::Ring& ring,
                                                       const std::string& name,
                                                       const RegionIndex& regionIndex)
  {
    if (ring.ring==Area::masterRingId &&
        ring.nodes.empty()) {
      for (const auto& r : area.rings) {
        if (r.ring==Area::outerRingId) {
          GeoBox boundingBox;

          r.GetBoundingBox(boundingBox);

          RegionRef region=regionIndex.GetRegionForNode(rootRegion,
                                                        boundingBox.GetMinCoord());

          AddLocationAreaToRegion(*region,
                                  area,
                                  r.nodes,
                                  name,
                                  boundingBox.GetMinLon(),
                                  boundingBox.GetMinLat(),
                                  boundingBox.GetMaxLon(),
                                  boundingBox.GetMaxLat());
        }
      }
    }
    else {
      GeoBox boundingBox;

      ring.GetBoundingBox(boundingBox);

      RegionRef region=regionIndex.GetRegionForNode(rootRegion,
                                                    boundingBox.GetMinCoord());

      AddLocationAreaToRegion(*region,
                              area,
                              ring.nodes,
                              name,
                              boundingBox.GetMinLon(),
                              boundingBox.GetMinLat(),
                              boundingBox.GetMaxLon(),
                              boundingBox.GetMaxLat());
    }
  }

  bool LocationIndexGenerator::IndexLocationAreas(const TypeConfig& typeConfig,
                                                  const ImportParameter& parameter,
                                                  Progress& progress,
                                                  RegionRef& rootRegion,
                                                  const RegionIndex& regionIndex)
  {
    FileScanner            scanner;
    uint32_t               areaCount;
    size_t                 areasFound=0;
    NameFeatureValueReader nameReader(typeConfig);

    try {
      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   AreaDataFile::AREAS_DAT),
                   FileScanner::Sequential,
                   parameter.GetWayDataMemoryMaped());

      if (!scanner.Read(areaCount)) {
        progress.Error("Error while reading number of data entries in file");
        return false;
      }

      for (uint32_t w=1; w<=areaCount; w++) {
        progress.SetProgress(w,areaCount);

        Area area;

        if (!area.Read(typeConfig,
                       scanner)) {
          progress.Error(std::string("Error while reading data entry ")+
                         NumberToString(w)+" of "+
                         NumberToString(areaCount)+
                         " in file '"+
                         scanner.GetFilename()+"'");
          return false;
        }

        for (const auto& ring : area.rings) {
          if (!ring.GetType()->GetIgnore() && ring.GetType()->GetIndexAsLocation()) {
            NameFeatureValue *nameValue=nameReader.GetValue(ring.GetFeatureValueBuffer());

            if (nameValue!=NULL) {
              AddLocationAreaToRegion(rootRegion,
                                      area,
                                      ring,
                                      nameValue->GetName(),
                                      regionIndex);

              areasFound++;
            }
          }
        }
      }

      progress.Info(std::string("Found ")+NumberToString(areasFound)+" locations of type 'area'");

      scanner.Close();
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      return false;
    }

    return true;
  }

  /**
    Add the given location way to the hierarchical area index.

    The code is designed to minimize the number of "point in area" checks, it assumes that
    if one point of an object is in a area it is very likely that all points of the object
    are in the area.
    */
  bool LocationIndexGenerator::AddLocationWayToRegion(Region& region,
                                                      const Way& way,
                                                      const std::string& name,
                                                      double minlon,
                                                      double minlat,
                                                      double maxlon,
                                                      double maxlat)
  {
    for (const auto& childRegion : region.regions) {
      // Fast check, if the object is in the bounds of the area
      if (!(maxlon<childRegion->minlon) &&
          !(minlon>childRegion->maxlon) &&
          !(maxlat<childRegion->minlat) &&
          !(minlat>childRegion->maxlat)) {
        // Check if one point is in the area
        for (size_t i=0; i<childRegion->areas.size(); i++) {
          bool match=IsAreaAtLeastPartlyInArea(way.nodes,childRegion->areas[i]);

          if (match) {
            bool completeMatch=AddLocationWayToRegion(*childRegion,way,name,minlon,minlat,maxlon,maxlat);

            if (completeMatch) {
              // We are done, the object is completely enclosed by one of our sub areas
              return true;
            }
          }
        }
      }
    }

    // If we (at least partly) contain it, we add it to the area but continue

    region.locations[name].objects.push_back(ObjectFileRef(way.GetFileOffset(),refWay));

    for (size_t i=0; i<region.areas.size(); i++) {
      if (IsAreaCompletelyInArea(way.nodes,region.areas[i])) {
        return true;
      }
    }

    return false;
  }

  bool LocationIndexGenerator::IndexLocationWays(const TypeConfigRef& typeConfig,
                                                 const ImportParameter& parameter,
                                                 Progress& progress,
                                                 RegionRef& rootRegion,
                                                 const RegionIndex& regionIndex)
  {
    FileScanner            scanner;
    uint32_t               wayCount;
    size_t                 waysFound=0;
    NameFeatureValueReader nameReader(*typeConfig);

    try {
      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   WayDataFile::WAYS_DAT),
                   FileScanner::Sequential,
                   parameter.GetWayDataMemoryMaped());

      if (!scanner.Read(wayCount)) {
        progress.Error("Error while reading number of data entries in file");
        return false;
      }


      for (uint32_t w=1; w<=wayCount; w++) {
        progress.SetProgress(w,wayCount);

        Way way;

        if (!way.Read(*typeConfig,
                      scanner)) {
          progress.Error(std::string("Error while reading data entry ")+
                         NumberToString(w)+" of "+
                         NumberToString(wayCount)+
                         " in file '"+
                         scanner.GetFilename()+"'");
          return false;
        }

        if (!way.GetType()->GetIndexAsLocation()) {
          continue;
        }

        NameFeatureValue *nameValue=nameReader.GetValue(way.GetFeatureValueBuffer());

        if (nameValue==NULL) {
          continue;
        }

        GeoBox boundingBox;

        way.GetBoundingBox(boundingBox);

        RegionRef region=regionIndex.GetRegionForNode(rootRegion,
                                                      boundingBox.GetMinCoord());

        AddLocationWayToRegion(*region,
                               way,
                               nameValue->GetName(),
                               boundingBox.GetMinLon(),
                               boundingBox.GetMinLat(),
                               boundingBox.GetMaxLon(),
                               boundingBox.GetMaxLat());

        waysFound++;
      }

      progress.Info(std::string("Found ")+NumberToString(waysFound)+" locations of type 'way'");

      scanner.Close();
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      return false;
    }

    return true;
  }

  void LocationIndexGenerator::AddAddressAreaToRegion(Progress& progress,
                                                      Region& region,
                                                      const FileOffset& fileOffset,
                                                      const std::string& location,
                                                      const std::string& address,
                                                      const std::vector<GeoCoord>& nodes,
                                                      double minlon,
                                                      double minlat,
                                                      double maxlon,
                                                      double maxlat,
                                                      bool& added)
  {
    for (const auto& childRegion : region.regions) {
      // Fast check, if the object is in the bounds of the area
      if (!(maxlon<childRegion->minlon) &&
          !(minlon>childRegion->maxlon) &&
          !(maxlat<childRegion->minlat) &&
          !(minlat>childRegion->maxlat)) {
        for (size_t i=0; i<childRegion->areas.size(); i++) {
          if (IsAreaCompletelyInArea(nodes,childRegion->areas[i])) {
            AddAddressAreaToRegion(progress,
                                   *childRegion,
                                   fileOffset,
                                   location,
                                   address,
                                   nodes,
                                   minlon,minlat,maxlon,maxlat,
                                   added);
            return;
          }
        }
      }
    }

    std::map<std::string,RegionLocation>::iterator loc=region.locations.find(location);

    if (loc==region.locations.end()) {
      progress.Debug(std::string("Street of address '")+location +"' '"+address+"' of Area "+NumberToString(fileOffset)+" cannot be resolved in region '"+region.name+"'");

      return;
    }

    for (const auto& regionAddress : loc->second.addresses) {
      if (regionAddress.name==address) {
        return;
      }
    }

    RegionAddress regionAddress;

    regionAddress.name=address;
    regionAddress.object.Set(fileOffset,refArea);

    loc->second.addresses.push_back(regionAddress);

    added=true;
  }

  void LocationIndexGenerator::AddPOIAreaToRegion(Progress& progress,
                                                  Region& region,
                                                  const FileOffset& fileOffset,
                                                  const std::string& name,
                                                  const std::vector<GeoCoord>& nodes,
                                                  double minlon,
                                                  double minlat,
                                                  double maxlon,
                                                  double maxlat,
                                                  bool& added)
  {
    for (const auto& childRegion : region.regions) {
      // Fast check, if the object is in the bounds of the area
      if (!(maxlon<childRegion->minlon) &&
          !(minlon>childRegion->maxlon) &&
          !(maxlat<childRegion->minlat) &&
          !(minlat>childRegion->maxlat)) {
        for (size_t i=0; i<childRegion->areas.size(); i++) {
          if (IsAreaCompletelyInArea(nodes,childRegion->areas[i])) {
            AddPOIAreaToRegion(progress,
                               *childRegion,
                               fileOffset,
                               name,
                               nodes,
                               minlon,minlat,maxlon,maxlat,
                               added);
            return;
          }
        }
      }
    }

    RegionPOI poi;

    poi.name=name;
    poi.object.Set(fileOffset,refArea);

    region.pois.push_back(poi);

    added=true;
  }

  bool LocationIndexGenerator::IndexAddressAreas(const TypeConfig& typeConfig,
                                                 const ImportParameter& parameter,
                                                 Progress& progress,
                                                 RegionRef& rootRegion,
                                                 const RegionIndex& regionIndex)
  {
    FileScanner scanner;
    uint32_t    areaCount;
    size_t      addressFound=0;
    size_t      poiFound=0;

    try {
      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   AreaAreaIndexGenerator::AREAADDRESS_DAT),
                   FileScanner::Sequential,
                   parameter.GetWayDataMemoryMaped());

      if (!scanner.Read(areaCount)) {
        progress.Error("Error while reading number of data entries in file");
        return false;
      }

      FileOffset            fileOffset;
      uint32_t              tmpType;
      TypeId                typeId;
      TypeInfoRef           type;
      std::string           name;
      std::string           location;
      std::string           address;
      std::vector<GeoCoord> nodes;

      for (uint32_t a=1; a<=areaCount; a++) {
        progress.SetProgress(a,areaCount);

        if (!scanner.ReadFileOffset(fileOffset) ||
            !scanner.ReadNumber(tmpType) ||
            !scanner.Read(name) ||
            !scanner.Read(location) ||
            !scanner.Read(address) ||
            !scanner.Read(nodes)) {
          progress.Error(std::string("Error while reading data entry ")+
                         NumberToString(a)+" of "+
                         NumberToString(areaCount)+
                         " in file '"+
                         scanner.GetFilename()+"'");
          return false;
        }

        typeId=(TypeId)tmpType;
        type=typeConfig.GetAreaTypeInfo(typeId);

        bool isAddress=!location.empty() &&
                       !address.empty();
        bool isPOI=!name.empty() &&
                   type->GetIndexAsPOI();

        if (!isAddress && !isPOI) {
          continue;
        }

        double minlon;
        double maxlon;
        double minlat;
        double maxlat;

        GetBoundingBox(nodes,
                       minlon,
                       maxlon,
                       minlat,
                       maxlat);

        RegionRef region=regionIndex.GetRegionForNode(rootRegion,
                                                      GeoCoord(minlat,minlon));

        if (isAddress) {
          bool added=false;

          AddAddressAreaToRegion(progress,
                                 *region,
                                 fileOffset,
                                 location,
                                 address,
                                 nodes,
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

          AddPOIAreaToRegion(progress,
                             *region,
                             fileOffset,
                             name,
                             nodes,
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

      progress.Info(NumberToString(areaCount)+" areas analyzed, "+NumberToString(addressFound)+" addresses founds, "+NumberToString(poiFound)+" POIs founds");

      scanner.Close();
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      return false;
    }

    return true;
  }

  bool LocationIndexGenerator::AddAddressWayToRegion(Progress& progress,
                                                     Region& region,
                                                     const FileOffset& fileOffset,
                                                     const std::string& location,
                                                     const std::string& address,
                                                     const std::vector<GeoCoord>& nodes,
                                                     double minlon,
                                                     double minlat,
                                                     double maxlon,
                                                     double maxlat,
                                                     bool& added)
  {
    for (const auto& childRegion : region.regions) {
      // Fast check, if the object is in the bounds of the area
      if (!(maxlon<childRegion->minlon) &&
          !(minlon>childRegion->maxlon) &&
          !(maxlat<childRegion->minlat) &&
          !(minlat>childRegion->maxlat)) {
        // Check if one point is in the area
        for (size_t i=0; i<childRegion->areas.size(); i++) {
          bool match=IsAreaAtLeastPartlyInArea(nodes,childRegion->areas[i]);

          if (match) {
            bool completeMatch=AddAddressWayToRegion(progress,
                                                     *childRegion,
                                                     fileOffset,
                                                     location,
                                                     address,
                                                     nodes,
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

    std::map<std::string,RegionLocation>::iterator loc=region.locations.find(location);

    if (loc==region.locations.end()) {
      progress.Debug(std::string("Street of address '")+location +"' '"+address+"' of Way "+NumberToString(fileOffset)+" cannot be resolved in region '"+region.name+"'");
    }
    else {
      for (const auto& regionAddress : loc->second.addresses) {
        if (regionAddress.name==address) {
          return false;
        }
      }

      RegionAddress regionAddress;

      regionAddress.name=address;
      regionAddress.object.Set(fileOffset,refWay);

      loc->second.addresses.push_back(regionAddress);

      added=true;
    }

    for (size_t i=0; i<region.areas.size(); i++) {
      if (IsAreaCompletelyInArea(nodes,region.areas[i])) {
        return true;
      }
    }

    return false;
  }

  bool LocationIndexGenerator::AddPOIWayToRegion(Progress& progress,
                                                 Region& region,
                                                 const FileOffset& fileOffset,
                                                 const std::string& name,
                                                 const std::vector<GeoCoord>& nodes,
                                                 double minlon,
                                                 double minlat,
                                                 double maxlon,
                                                 double maxlat,
                                                 bool& added)
  {
    for (const auto& childRegion : region.regions) {
      // Fast check, if the object is in the bounds of the area
      if (!(maxlon<childRegion->minlon) &&
          !(minlon>childRegion->maxlon) &&
          !(maxlat<childRegion->minlat) &&
          !(minlat>childRegion->maxlat)) {
        // Check if one point is in the area
        for (size_t i=0; i<childRegion->areas.size(); i++) {
          bool match=IsAreaAtLeastPartlyInArea(nodes,childRegion->areas[i]);

          if (match) {
            bool completeMatch=AddPOIWayToRegion(progress,
                                                 *childRegion,
                                                 fileOffset,
                                                 name,
                                                 nodes,
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

    poi.name=name;
    poi.object.Set(fileOffset,refWay);

    region.pois.push_back(poi);

    added=true;

    for (size_t i=0; i<region.areas.size(); i++) {
      if (IsAreaCompletelyInArea(nodes,region.areas[i])) {
        return true;
      }
    }

    return false;
  }

  bool LocationIndexGenerator::IndexAddressWays(const TypeConfig& typeConfig,
                                                const ImportParameter& parameter,
                                                Progress& progress,
                                                RegionRef& rootRegion,
                                                const RegionIndex& regionIndex)
  {
    FileScanner scanner;
    uint32_t    wayCount;
    //size_t      addressFound=0;
    size_t      poiFound=0;

    try {
      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   SortWayDataGenerator::WAYADDRESS_DAT),
                   FileScanner::Sequential,
                   parameter.GetWayDataMemoryMaped());

      if (!scanner.Read(wayCount)) {
        progress.Error("Error while reading number of data entries in file");
        return false;
      }

      FileOffset            fileOffset;
      uint32_t              tmpType;
      TypeId                typeId;
      TypeInfoRef           type;
      std::string           name;
      std::string           location;
      std::vector<GeoCoord> nodes;

      for (uint32_t w=1; w<=wayCount; w++) {
        progress.SetProgress(w,wayCount);

        if (!scanner.ReadFileOffset(fileOffset) ||
            !scanner.ReadNumber(tmpType) ||
            !scanner.Read(name) ||
            !scanner.Read(location) ||
            !scanner.Read(nodes)) {
          progress.Error(std::string("Error while reading data entry ")+
                         NumberToString(w)+" of "+
                         NumberToString(wayCount)+
                         " in file '"+
                         scanner.GetFilename()+"'");
          return false;
        }

        typeId=(TypeId)tmpType;
        type=typeConfig.GetWayTypeInfo(typeId);

        bool isPOI=!name.empty() &&
                   type->GetIndexAsPOI();

        if (!isPOI) {
          continue;
        }

        double minlon;
        double maxlon;
        double minlat;
        double maxlat;

        if (nodes.size()==0) {
          std::cerr << "Way " << fileOffset << " has no nodes" << std::endl;
        }

        GetBoundingBox(nodes,
                       minlon,
                       maxlon,
                       minlat,
                       maxlat);

        RegionRef region=regionIndex.GetRegionForNode(rootRegion,
                                                      GeoCoord(minlat,minlon));

        /*
        if (isAddress) {
          bool added=false;

          AddAddressWayToRegion(progress,
                                region,
                                fileOffset,
                                location,
                                address,
                                nodes,
                                minlon,
                                minlat,
                                maxlon,
                                maxlat,
                                added);

        if (added) {
          addressFound++;
          }
        }*/

        if (isPOI) {
          bool added=false;

          AddPOIWayToRegion(progress,
                            *region,
                            fileOffset,
                            name,
                            nodes,
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

      progress.Info(NumberToString(wayCount)+" ways analyzed, "/*+NumberToString(addressFound)+" addresses founds, "*/+NumberToString(poiFound)+" POIs founds");

      scanner.Close();
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      return false;
    }

    return true;
  }

  void LocationIndexGenerator::AddAddressNodeToRegion(Progress& progress,
                                                      Region& region,
                                                      const FileOffset& fileOffset,
                                                      const std::string& location,
                                                      const std::string& address,
                                                      bool& added)
  {
    std::map<std::string,RegionLocation>::iterator loc=region.locations.find(location);

    if (loc==region.locations.end()) {
      progress.Debug(std::string("Street of address '")+location +"' '"+address+"' of Node "+NumberToString(fileOffset)+" cannot be resolved in region '"+region.name+"'");
      return;
    }

    for (const auto& regionAddress : loc->second.addresses) {
      if (regionAddress.name==address) {
        return;
      }
    }

    RegionAddress regionAddress;

    regionAddress.name=address;
    regionAddress.object.Set(fileOffset,refNode);

    loc->second.addresses.push_back(regionAddress);

    added=true;
  }

  void LocationIndexGenerator::AddPOINodeToRegion(Region& region,
                                                  const FileOffset& fileOffset,
                                                  const std::string& name,
                                                  bool& added)
  {
    RegionPOI poi;

    poi.name=name;
    poi.object.Set(fileOffset,refNode);

    region.pois.push_back(poi);

    added=true;
  }

  bool LocationIndexGenerator::IndexAddressNodes(const TypeConfig& typeConfig,
                                                 const ImportParameter& parameter,
                                                 Progress& progress,
                                                 RegionRef& rootRegion,
                                                 const RegionIndex& regionIndex)
  {
    FileScanner scanner;
    uint32_t    nodeCount;
    size_t      addressFound=0;
    size_t      poiFound=0;

    try {
      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   SortNodeDataGenerator::NODEADDRESS_DAT),
                   FileScanner::Sequential,
                   true);

      if (!scanner.Read(nodeCount)) {
        progress.Error("Error while reading number of data entries in file");
        return false;
      }

      FileOffset  fileOffset;
      uint32_t    tmpType;
      TypeId      typeId;
      TypeInfoRef type;
      std::string name;
      std::string location;
      std::string address;
      GeoCoord    coord;

      for (uint32_t n=1; n<=nodeCount; n++) {
        progress.SetProgress(n,nodeCount);

        if (!scanner.ReadFileOffset(fileOffset) ||
            !scanner.ReadNumber(tmpType) ||
            !scanner.Read(name) ||
            !scanner.Read(location) ||
            !scanner.Read(address) ||
            !scanner.ReadCoord(coord)) {
          progress.Error(std::string("Error while reading data entry ")+
                         NumberToString(n)+" of "+
                         NumberToString(nodeCount)+
                         " in file '"+
                         scanner.GetFilename()+"'");
          return false;
        }

        typeId=(TypeId)tmpType;
        type=typeConfig.GetNodeTypeInfo(typeId);

        bool isAddress=!location.empty() &&
                       !address.empty();
        bool isPOI=!name.empty() &&
                   type->GetIndexAsPOI();

        if (!isAddress && !isPOI) {
          continue;
        }

        RegionRef region=regionIndex.GetRegionForNode(rootRegion,
                                                      coord);

        if (!region) {
          continue;
        }

        if (isAddress) {
          bool added=false;

          AddAddressNodeToRegion(progress,
                                 *region,
                                 fileOffset,
                                 location,
                                 address,
                                 added);
          if (added) {
            addressFound++;
          }
        }

        if (isPOI) {
          bool added=false;

          AddPOINodeToRegion(*region,
                             fileOffset,
                             name,
                             added);
          if (added) {
            poiFound++;
          }
        }
      }

      progress.Info(NumberToString(nodeCount)+" nodes analyzed, "+NumberToString(addressFound)+" addresses founds, "+NumberToString(poiFound)+" POIs founds");

      scanner.Close();
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      return false;
    }

    return true;
  }

  void LocationIndexGenerator::WriteIgnoreTokens(FileWriter& writer,
                                                 const std::list<std::string>& regionIgnoreTokens,
                                                 const std::list<std::string>& locationIgnoreTokens)
  {
    writer.WriteNumber((uint32_t)regionIgnoreTokens.size());

    for (const auto& token : regionIgnoreTokens) {
      writer.Write(token);
    }

    writer.WriteNumber((uint32_t)locationIgnoreTokens.size());

    for (const auto& token : locationIgnoreTokens) {
      writer.Write(token);
    }
  }

  void LocationIndexGenerator::WriteRegionIndexEntry(FileWriter& writer,
                                                     const Region& parentRegion,
                                                     Region& region)
  {
    region.indexOffset=writer.GetPos();

    writer.WriteFileOffset(region.dataOffset);
    writer.WriteFileOffset(parentRegion.indexOffset);

    writer.Write(region.name);

    Write(writer,
          region.reference);

    writer.WriteNumber((uint32_t)region.aliases.size());
    for (const auto& alias : region.aliases) {
      writer.Write(alias.name);
      writer.WriteFileOffset(alias.reference,
                             bytesForNodeFileOffset);
    }

    writer.WriteNumber((uint32_t)region.regions.size());

    for (const auto& childRegion : region.regions) {
      FileOffset nextChildOffsetOffset;

      nextChildOffsetOffset=writer.GetPos();

      writer.WriteFileOffset(0);

      WriteRegionIndexEntry(writer,
                            region,
                            *childRegion);

      FileOffset nextChildOffset;

      nextChildOffset=writer.GetPos();
      writer.SetPos(nextChildOffsetOffset);
      writer.WriteFileOffset(nextChildOffset);
      writer.SetPos(nextChildOffset);
    }
  }

  void LocationIndexGenerator::WriteRegionIndex(FileWriter& writer,
                                                Region& rootRegion)
  {
    writer.WriteNumber((uint32_t)rootRegion.regions.size());

    for (const auto& childRegion : rootRegion.regions) {
      FileOffset nextChildOffsetOffset;

      nextChildOffsetOffset=writer.GetPos();

      writer.WriteFileOffset(0);

      WriteRegionIndexEntry(writer,
                            rootRegion,
                            *childRegion);

      FileOffset nextChildOffset=0;

      nextChildOffset=writer.GetPos();
      writer.SetPos(nextChildOffsetOffset);
      writer.WriteFileOffset(nextChildOffset);
      writer.SetPos(nextChildOffset);
    }
  }

  void LocationIndexGenerator::WriteRegionDataEntry(FileWriter& writer,
                                                    Region& region)
  {
    region.dataOffset=writer.GetPos();

    writer.SetPos(region.indexOffset);
    writer.WriteFileOffset(region.dataOffset);
    writer.SetPos(region.dataOffset);

    region.pois.sort();

    writer.WriteNumber((uint32_t)region.pois.size());

    ObjectFileRefStreamWriter objectFileRefWriter(writer);

    for (const auto& poi : region.pois) {
      writer.Write(poi.name);

      objectFileRefWriter.Write(poi.object);
    }

    writer.WriteNumber((uint32_t)region.locations.size());
    for (auto& location : region.locations) {
      location.second.objects.sort(ObjectFileRefByFileOffsetComparator());

      writer.Write(location.first);
      writer.WriteNumber((uint32_t)location.second.objects.size()); // Number of objects

      if (!location.second.addresses.empty()) {
        writer.Write(true);
        location.second.addressOffset=writer.GetPos();
        writer.WriteFileOffset(0);
      }
      else {
        writer.Write(false);
      }

      objectFileRefWriter.Reset();

      for (const auto& object : location.second.objects) {
        objectFileRefWriter.Write(object);
      }
    }

    for (const auto& childRegion : region.regions) {
      WriteRegionDataEntry(writer,
                           *childRegion);
    }
  }

  void LocationIndexGenerator::WriteRegionData(FileWriter& writer,
                                                 Region& rootRegion)
  {
    for (const auto& childRegion : rootRegion.regions) {
      WriteRegionDataEntry(writer,
                           *childRegion);
    }
  }

  void LocationIndexGenerator::WriteAddressDataEntry(FileWriter& writer,
                                                     Region& region)
  {
    for (auto& location : region.locations) {
      if (!location.second.addresses.empty()) {
        FileOffset offset;

        offset=writer.GetPos();

        writer.SetPos(location.second.addressOffset);
        writer.WriteFileOffset(offset);
        writer.SetPos(offset);

        location.second.addresses.sort();

        writer.WriteNumber((uint32_t)location.second.addresses.size());

        ObjectFileRefStreamWriter objectFileRefWriter(writer);

        for (const auto& address : location.second.addresses) {
          writer.Write(address.name);

          objectFileRefWriter.Write(address.object);
        }
      }
    }

    for (const auto& childRegion: region.regions) {
      WriteAddressDataEntry(writer,
                            *childRegion);
    }
  }

  void LocationIndexGenerator::WriteAddressData(FileWriter& writer,
                                                Region& rootRegion)
  {
    for (const auto& childRegion : rootRegion.regions) {
      WriteAddressDataEntry(writer,
                            *childRegion);
    }
  }

  void LocationIndexGenerator::GetDescription(const ImportParameter& /*parameter*/,
                                              ImportModuleDescription& description) const
  {
    description.SetName("LocationIndexGenerator");
    description.SetDescription("Create index for lookup of objects based on address data");

    description.AddRequiredFile(NodeDataFile::NODES_DAT);
    description.AddRequiredFile(WayDataFile::WAYS_DAT);
    description.AddRequiredFile(AreaDataFile::AREAS_DAT);

    description.AddRequiredFile(SortNodeDataGenerator::NODEADDRESS_DAT);
    description.AddRequiredFile(SortWayDataGenerator::WAYADDRESS_DAT);
    description.AddRequiredFile(AreaAreaIndexGenerator::AREAADDRESS_DAT);

    description.AddProvidedFile(LocationIndex::FILENAME_LOCATION_IDX);
  }

  bool LocationIndexGenerator::Import(const TypeConfigRef& typeConfig,
                                      const ImportParameter& parameter,
                                      Progress& progress)
  {
    RegionRef                          rootRegion;
    std::vector<std::list<RegionRef> > regionTree;
    RegionIndex                        regionIndex;
    TypeInfoRef                        boundaryType;
    TypeInfoSet                        boundaryTypes(*typeConfig);
    std::list<Boundary>                boundaryAreas;
    std::list<std::string>             regionIgnoreTokens;
    std::list<std::string>             locationIgnoreTokens;

    progress.SetAction("Setup");

    if (!BytesNeededToAddressFileData(AppendFileToDir(parameter.GetDestinationDirectory(),
                                                      "nodes.dat"),
                                      bytesForNodeFileOffset)) {
      progress.Error("Cannot get file size of 'nodes.dat'");
      return false;
    }

    if (!BytesNeededToAddressFileData(AppendFileToDir(parameter.GetDestinationDirectory(),
                                                      "areas.dat"),
                                      bytesForAreaFileOffset)) {
      progress.Error("Cannot get file size of 'areas.dat'");
      return false;
    }

    if (!BytesNeededToAddressFileData(AppendFileToDir(parameter.GetDestinationDirectory(),
                                                      "ways.dat"),
                                      bytesForWayFileOffset)) {
      progress.Error("Cannot get file size of 'ways.dat'");
      return false;
    }

    rootRegion=std::make_shared<Region>();
    rootRegion->name="<root>";
    rootRegion->indexOffset=0;
    rootRegion->dataOffset=0;

    boundaryType=typeConfig->GetTypeInfo("boundary_country");
    assert(boundaryType);
    boundaryTypes.Set(boundaryType);

    boundaryType=typeConfig->GetTypeInfo("boundary_state");
    assert(boundaryType);
    boundaryTypes.Set(boundaryType);

    boundaryType=typeConfig->GetTypeInfo("boundary_county");
    assert(boundaryType);
    boundaryTypes.Set(boundaryType);

    boundaryType=typeConfig->GetTypeInfo("boundary_administrative");
    assert(boundaryType);
    boundaryTypes.Set(boundaryType);


    //
    // Getting all areas of type 'administrative boundary'.
    //

    progress.SetAction("Scanning for administrative boundaries of type 'area'");

    if (!GetBoundaryAreas(parameter,
                          progress,
                          typeConfig,
                          boundaryTypes,
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

    progress.SetAction("Indexing regions of type 'area'");

    if (!IndexRegionAreas(*typeConfig,
                          parameter,
                          progress,
                          *rootRegion)) {
      return false;
    }

    progress.SetAction("Calculating region tree depth");

    regionTree.resize(GetRegionTreeDepth(*rootRegion));

    progress.Info(std::string("Area tree depth: ")+NumberToString(regionTree.size()));

    progress.SetAction("Sorting regions by levels");

    SortInRegion(rootRegion,
                 regionTree,
                 0);

    for (size_t i=0; i<regionTree.size(); i++) {
      progress.Info(std::string("Area tree index ")+NumberToString(i)+" size: "+NumberToString(regionTree[i].size()));
    }

    progress.SetAction("Index regions");

    regionIndex.cellWidth=360.0/pow(2.0,REGION_INDEX_LEVEL);
    regionIndex.cellHeight=180.0/pow(2.0,REGION_INDEX_LEVEL);

    IndexRegions(regionTree,
                 regionIndex);

    //
    // Getting all nodes of type place=*. We later need an area for these cities.
    //

    progress.SetAction("Indexing regions of type 'Node' as area aliases");

    if (!IndexRegionNodes(typeConfig,
                          parameter,
                          progress,
                          rootRegion,
                          regionIndex)) {
      return false;
    }

    progress.SetAction("Index location areas");

    if (!IndexLocationAreas(*typeConfig,
                            parameter,
                            progress,
                            rootRegion,
                            regionIndex)) {
      return false;
    }

    progress.SetAction("Index location ways");

    if (!IndexLocationWays(typeConfig,
                           parameter,
                           progress,
                           rootRegion,
                           regionIndex)) {
      return false;
    }

    for (size_t i=0; i<regionTree.size(); i++) {
      size_t count=0;

      for (const auto& region : regionTree[i]) {
        count+=region->locations.size();
      }

      progress.Info(std::string("Area tree index ")+NumberToString(i)+" object count size: "+NumberToString(count));
    }

    progress.SetAction("Index address areas");

    if (!IndexAddressAreas(*typeConfig,
                           parameter,
                           progress,
                           rootRegion,
                           regionIndex)) {
      return false;
    }

    progress.SetAction("Index address ways");

    if (!IndexAddressWays(*typeConfig,
                          parameter,
                          progress,
                          rootRegion,
                          regionIndex)) {
      return false;
    }

    progress.SetAction("Index address nodes");

    if (!IndexAddressNodes(*typeConfig,
                           parameter,
                           progress,
                           rootRegion,
                           regionIndex)) {
      return false;
    }

    progress.SetAction("Calculate ignore tokens");

    CalculateIgnoreTokens(*rootRegion,
                          regionIgnoreTokens,
                          locationIgnoreTokens);

    progress.Info("Detected "+NumberToString(regionIgnoreTokens.size())+" token(s) to ignore");

    progress.SetAction("Dumping region tree");

    DumpRegionTree(progress,
                   *rootRegion,
                   AppendFileToDir(parameter.GetDestinationDirectory(),
                                   "location_region.txt"));

    progress.SetAction("Dumping location tree");

    DumpLocationTree(progress,
                     *rootRegion,
                     AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "location_full.txt"));

    FileWriter writer;

    //
    // Generate file with all areas, where areas reference parent and children by offset
    //

    progress.SetAction(std::string("Write '")+LocationIndex::FILENAME_LOCATION_IDX+"'");

    try {
      writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                  LocationIndex::FILENAME_LOCATION_IDX));

      writer.Write(bytesForNodeFileOffset);
      writer.Write(bytesForAreaFileOffset);
      writer.Write(bytesForWayFileOffset);

      WriteIgnoreTokens(writer,
                        regionIgnoreTokens,
                        locationIgnoreTokens);

      WriteRegionIndex(writer,
                       *rootRegion);

      WriteRegionData(writer,
                      *rootRegion);

      WriteAddressData(writer,
                       *rootRegion);

      writer.Close();
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();

      writer.CloseFailsafe();

      return false;
    }

    return true;
  }
}
