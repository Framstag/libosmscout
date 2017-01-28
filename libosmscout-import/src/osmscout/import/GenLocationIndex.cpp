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

#include <algorithm>
#include <iostream>
#include <fstream>
#include <limits>
#include <locale>
#include <list>
#include <map>
#include <set>

#include <osmscout/Pixel.h>

#include <osmscout/TypeFeatures.h>

#include <osmscout/LocationIndex.h>

#include <osmscout/AreaDataFile.h>
#include <osmscout/NodeDataFile.h>
#include <osmscout/WayDataFile.h>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

#include <osmscout/util/File.h>
#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>
#include <osmscout/util/GeoBox.h>
#include <osmscout/util/Geometry.h>
#include <osmscout/util/String.h>

#include <osmscout/import/SortWayDat.h>
#include <osmscout/import/SortNodeDat.h>
#include <osmscout/import/GenAreaAreaIndex.h>

namespace osmscout {

  static const size_t REGION_INDEX_LEVEL=14;

  const char* const LocationIndexGenerator::FILENAME_LOCATION_REGION_TXT = "location_region.txt";
  const char* const LocationIndexGenerator::FILENAME_LOCATION_FULL_TXT = "location_full.txt";

  void LocationIndexGenerator::Region::CalculateMinMax()
  {
    bool isStart=true;

    boundingBoxes.reserve(areas.size());

    for (const auto& area : areas) {
      GeoBox boundingBox;

      osmscout::GetBoundingBox(area,
                               boundingBox);

      boundingBoxes.push_back(boundingBox);

      if (isStart) {
        this->boundingBox=boundingBox;
        isStart=false;
      }
      else {
        this->boundingBox.Include(boundingBox);
      }
    }
  }

  bool LocationIndexGenerator::Region::CouldContain(const GeoBox& boundingBox) const
  {
    for (const auto& bb : boundingBoxes) {
      if (bb.Intersects(boundingBox)) {
        return true;
      }
    }

    return false;
  }

  bool LocationIndexGenerator::Region::CouldContain(const Region& region) const
  {
    for (const auto& bb : boundingBoxes) {
      for (const auto& rbb : region.boundingBoxes) {
        if (bb.Intersects(rbb)) {
          return true;
        }
      }
    }

    return false;
  }

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
      break;
    case refArea:
      writer.WriteFileOffset(object.GetFileOffset(),
                             bytesForAreaFileOffset);
      break;
    case refWay:
      writer.WriteFileOffset(object.GetFileOffset(),
                             bytesForWayFileOffset);
      break;
    default:
      std::cout << "type: " << (int) object.GetType() << std::endl;
      throw IOException(writer.GetFilename(),"Cannot write ObjectFileRef","Unknown object file type");
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
      out << " + " << childRegion->name << " " << childRegion->reference.GetTypeName() << " " << childRegion->reference.GetFileOffset();

      if (!childRegion->isIn.empty()) {
        out << " (in " << childRegion->isIn << ")";
      }

      if (childRegion->areas.size()>1) {
        out << " " << childRegion->areas.size() << " areas";
      }

      out << std::endl;

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

    debugStream.imbue(std::locale::classic());
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
      out << " + " << childRegion->name << " " << childRegion->reference.GetTypeName() << " " << childRegion->reference.GetFileOffset();

      if (childRegion->areas.size()>1) {
        out << " " << childRegion->areas.size() << " areas";
      }

      out << std::endl;

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

          out << " @ " << address.name << " " << address.postalCode
              << " " << address.object.GetTypeName() << " " << address.object.GetFileOffset() << std::endl;
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

    debugStream.imbue(std::locale::classic());
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
      if (region->CouldContain(*childRegion)) {
        for (const auto& regionArea : region->areas) {
          for (const auto& childRegionArea : childRegion->areas) {
            if (IsAreaSubOfAreaOrSame(regionArea,childRegionArea)) {
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

    if (!region->isIn.empty() &&
      parent.name!=region->isIn) {
      errorReporter->ReportLocation(region->reference,"'" + region->name + "' parent should be '"+region->isIn+"' but is '"+parent.name+"'");
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
                                                std::vector<std::list<RegionRef>>& boundaryAreas)
  {
    FileScanner                  scanner;
    NameFeatureValueReader       nameReader(*typeConfig);
    AdminLevelFeatureValueReader adminLevelReader(*typeConfig);

    try {
      uint32_t areaCount;

      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   AreaDataFile::AREAS_DAT),
                   FileScanner::Sequential,
                   true);

      scanner.Read(areaCount);

      for (uint32_t r=1; r<=areaCount; r++) {
        progress.SetProgress(r,areaCount);

        Area area;

        area.Read(*typeConfig,
                  scanner);

        if (!boundaryTypes.IsSet(area.GetType())) {
          continue;
        }

        NameFeatureValue *nameValue=nameReader.GetValue(area.rings.front().GetFeatureValueBuffer());

        if (nameValue==NULL) {
          errorReporter->ReportLocation(ObjectFileRef(area.GetFileOffset(),refArea),"No name");
          continue;
        }

        AdminLevelFeatureValue *adminLevelValue=adminLevelReader.GetValue(area.rings.front().GetFeatureValueBuffer());

        if (adminLevelValue==NULL) {
          errorReporter->ReportLocation(ObjectFileRef(area.GetFileOffset(),refArea),"No tag 'admin_level'");
          continue;
        }

        RegionRef region=std::make_shared<Region>();
        size_t   level=adminLevelValue->GetAdminLevel();

        region->reference=area.GetObjectFileRef();
        region->name=nameValue->GetName();

        if (!adminLevelValue->GetIsIn().empty()) {
          region->isIn=GetFirstInStringList(adminLevelValue->GetIsIn(),",;");
        }

        for (const auto& ring : area.rings) {
          if (ring.IsOuterRing()) {
            std::vector<GeoCoord> coords;

            for (const auto& node : ring.nodes) {
              coords.push_back(node.GetCoord());
            }

            region->areas.push_back(coords);
          }
        }

        region->CalculateMinMax();

        if (level>=boundaryAreas.size()) {
          boundaryAreas.resize(level+1);
        }

        boundaryAreas[level].push_back(region);
      }

      scanner.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }

  void LocationIndexGenerator::SortInBoundaries(Progress& progress,
                                                Region& rootRegion,
                                                const std::list<RegionRef>& boundaryAreas)
  {
    size_t currentBoundary=0;
    size_t maxBoundary=boundaryAreas.size();

    for (const auto&  region : boundaryAreas) {
      currentBoundary++;

      progress.SetProgress(currentBoundary,
                           maxBoundary);

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
    IsInFeatureValueReader isInReader(typeConfig);

    try {
      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   AreaDataFile::AREAS_DAT),
                   FileScanner::Sequential,
                   parameter.GetWayDataMemoryMaped());

      scanner.Read(areaCount);

      for (uint32_t a=1; a<=areaCount; a++) {
        progress.SetProgress(a,areaCount);

        Area area;

        area.Read(typeConfig,
                  scanner);

        if (!area.GetType()->GetIndexAsRegion()) {
          continue;
        }

        NameFeatureValue *nameValue=nameReader.GetValue(area.rings.front().GetFeatureValueBuffer());

        if (nameValue==NULL) {
          errorReporter->ReportLocation(ObjectFileRef(area.GetFileOffset(),refArea),"No name");
          continue;
        }

        RegionRef region=std::make_shared<Region>();

        region->reference.Set(area.GetFileOffset(),refArea);
        region->name=nameValue->GetName();

        IsInFeatureValue *isInValue=isInReader.GetValue(area.rings.front().GetFeatureValueBuffer());

        if (isInValue!=NULL) {
          region->isIn=GetFirstInStringList(isInValue->GetIsIn(),",;");
        }

        for (const auto& ring : area.rings) {
          if (ring.IsOuterRing()) {
            std::vector<GeoCoord> coords;

            for (const auto& node : ring.nodes) {
              coords.push_back(node.GetCoord());
            }

            region->areas.push_back(coords);
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
      progress.Error(e.GetDescription());
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
        for (const auto& boundingBox : region->GetAreaBoundingBoxes()) {
          uint32_t cellMinX=(uint32_t)((boundingBox.GetMinLon()+180.0)/regionIndex.cellWidth);
          uint32_t cellMaxX=(uint32_t)((boundingBox.GetMaxLon()+180.0)/regionIndex.cellWidth);
          uint32_t cellMinY=(uint32_t)((boundingBox.GetMinLat()+90.0)/regionIndex.cellHeight);
          uint32_t cellMaxY=(uint32_t)((boundingBox.GetMaxLat()+90.0)/regionIndex.cellHeight);

          for (uint32_t y=cellMinY; y<=cellMaxY; y++) {
            for (uint32_t x=cellMinX; x<=cellMaxX; x++) {
              Pixel pixel(x,y);

              regionIndex.index[pixel].push_back(region);
            }
          }
        }
      }
    }

    for (auto& regionList : regionIndex.index) {
      regionList.second.sort([](const RegionRef& a, const RegionRef& b) {
        return a->GetBoundingBox().GetSize()<b->GetBoundingBox().GetSize();
      });
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

      scanner.Read(nodeCount);

      for (uint32_t n=1; n<=nodeCount; n++) {
        progress.SetProgress(n,nodeCount);

        Node node;

        node.Read(*typeConfig,
                  scanner);

        if (node.GetType()->GetIndexAsRegion()) {
          NameFeatureValue *nameValue=nameReader.GetValue(node.GetFeatureValueBuffer());

          if (nameValue==NULL) {
            errorReporter->ReportLocation(ObjectFileRef(node.GetFileOffset(),refNode),"No name");
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
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }

  bool LocationIndexGenerator::AddLocationAreaToRegion(Region& region,
                                                       const Area& area,
                                                       const std::vector<Point>& nodes,
                                                       const std::string& name,
                                                       const GeoBox& boundingBox)
  {
    for (const auto& childRegion : region.regions) {
      // Fast check, if the object is in the bounds of the area
      if (childRegion->CouldContain(boundingBox)) {
        for (size_t i=0; i<childRegion->areas.size(); i++) {
          // Check if one point is in the area
          bool match=IsCoordInArea(nodes[0],childRegion->areas[i]);

          if (match) {
            bool completeMatch=AddLocationAreaToRegion(*childRegion,area,nodes,name,boundingBox);

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
    if (ring.IsMasterRing() &&
        ring.nodes.empty()) {
      for (const auto& r : area.rings) {
        if (r.IsOuterRing()) {
          GeoBox boundingBox;

          r.GetBoundingBox(boundingBox);

          RegionRef region=regionIndex.GetRegionForNode(rootRegion,
                                                        boundingBox.GetCenter());

          AddLocationAreaToRegion(*region,
                                  area,
                                  r.nodes,
                                  name,
                                  boundingBox);
        }
      }
    }
    else {
      GeoBox boundingBox;

      ring.GetBoundingBox(boundingBox);

      RegionRef region=regionIndex.GetRegionForNode(rootRegion,
                                                    boundingBox.GetCenter());

      AddLocationAreaToRegion(*region,
                              area,
                              ring.nodes,
                              name,
                              boundingBox);
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

      scanner.Read(areaCount);

      for (uint32_t w=1; w<=areaCount; w++) {
        progress.SetProgress(w,areaCount);

        Area area;

        area.Read(typeConfig,
                  scanner);

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
      progress.Error(e.GetDescription());
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
                                                      const GeoBox& boundingBox)
  {
    for (const auto& childRegion : region.regions) {
      // Fast check, if the object is in the bounds of the area
      if (childRegion->CouldContain(boundingBox)) {
        // Check if one point is in the area
        for (size_t i=0; i<childRegion->areas.size(); i++) {
          bool match=IsAreaAtLeastPartlyInArea(way.nodes,childRegion->areas[i]);

          if (match) {
            bool completeMatch=AddLocationWayToRegion(*childRegion,way,name,boundingBox);

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

      scanner.Read(wayCount);

      for (uint32_t w=1; w<=wayCount; w++) {
        progress.SetProgress(w,wayCount);

        Way way;

        way.Read(*typeConfig,
                 scanner);

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
                                                      boundingBox.GetCenter());

        AddLocationWayToRegion(*region,
                               way,
                               nameValue->GetName(),
                               boundingBox);

        waysFound++;
      }

      progress.Info(std::string("Found ")+NumberToString(waysFound)+" locations of type 'way'");

      scanner.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }

  void LocationIndexGenerator::AddAddressAreaToRegion(Progress& progress,
                                                      Region& region,
                                                      const FileOffset& fileOffset,
                                                      const std::string& location,
                                                      const std::string& address,
                                                      const std::vector<Point>& nodes,
                                                      const GeoBox& boundingBox,
                                                      bool& added)
  {
    for (const auto& childRegion : region.regions) {
      // Fast check, if the object is in the bounds of the area
      if (childRegion->CouldContain(boundingBox)) {
        for (const auto& area : childRegion->areas) {
          if (IsAreaCompletelyInArea(nodes,area)) {
            AddAddressAreaToRegion(progress,
                                   *childRegion,
                                   fileOffset,
                                   location,
                                   address,
                                   nodes,
                                   boundingBox,
                                   added);
            return;
          }
        }
      }
    }

    std::map<std::string,RegionLocation>::iterator loc=FindLocation(progress,region,location);

    if (loc==region.locations.end()) {
      errorReporter->ReportLocationDebug(ObjectFileRef(fileOffset,refArea),
                                         std::string("Street of address '")+location +"' '"+address+"' cannot be resolved in region '"+region.name+"'");
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
                                                  const std::vector<Point>& nodes,
                                                  const GeoBox& boundingBox,
                                                  bool& added)
  {
    for (const auto& childRegion : region.regions) {
      // Fast check, if the object is in the bounds of the area
      if (childRegion->CouldContain(boundingBox)) {
        for (size_t i=0; i<childRegion->areas.size(); i++) {
          if (IsAreaCompletelyInArea(nodes,childRegion->areas[i])) {
            AddPOIAreaToRegion(progress,
                               *childRegion,
                               fileOffset,
                               name,
                               nodes,
                               boundingBox,
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

      scanner.Read(areaCount);

      FileOffset            fileOffset;
      uint32_t              tmpType;
      TypeId                typeId;
      TypeInfoRef           type;
      std::string           name;
      std::string           location;
      std::string           address;
      std::vector<Point>    nodes;

      for (uint32_t a=1; a<=areaCount; a++) {
        progress.SetProgress(a,areaCount);

        scanner.ReadFileOffset(fileOffset);
        scanner.ReadNumber(tmpType);

        scanner.Read(name);
        scanner.Read(location);
        scanner.Read(address);
        scanner.Read(nodes,false);

        typeId=(TypeId)tmpType;
        type=typeConfig.GetAreaTypeInfo(typeId);

        bool isAddress=!location.empty() &&
                       !address.empty();
        bool isPOI=!name.empty() &&
                   type->GetIndexAsPOI();

        if (!isAddress && !isPOI) {
          continue;
        }

        GeoBox boundingBox;

        GetBoundingBox(nodes,
                       boundingBox);

        RegionRef region=regionIndex.GetRegionForNode(rootRegion,
                                                      boundingBox.GetCenter());

        if (isAddress) {
          bool added=false;

          AddAddressAreaToRegion(progress,
                                 *region,
                                 fileOffset,
                                 location,
                                 address,
                                 nodes,
                                 boundingBox,
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
                             boundingBox,
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
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }

  bool LocationIndexGenerator::AddAddressWayToRegion(Progress& progress,
                                                     Region& region,
                                                     const FileOffset& fileOffset,
                                                     const std::string& location,
                                                     const std::string& address,
                                                     const std::vector<Point>& nodes,
                                                     const GeoBox& boundingBox,
                                                     bool& added)
  {
    for (const auto& childRegion : region.regions) {
      // Fast check, if the object is in the bounds of the area
      if (childRegion->CouldContain(boundingBox)) {
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
                                                     boundingBox,
                                                     added);

            if (completeMatch) {
              // We are done, the object is completely enclosed by one of our sub areas
              return true;
            }
          }
        }
      }
    }

    std::map<std::string,RegionLocation>::iterator loc=FindLocation(progress,region,location);

    if (loc==region.locations.end()) {
      errorReporter->ReportLocationDebug(ObjectFileRef(fileOffset,refWay),
                                         std::string("Street of address '")+location +"' '"+address+"' cannot be resolved in region '"+region.name+"'");
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
                                                 const std::vector<Point>& nodes,
                                                 const GeoBox& boundingBox,
                                                 bool& added)
  {
    for (const auto& childRegion : region.regions) {
      // Fast check, if the object is in the bounds of the area
      if (childRegion->CouldContain(boundingBox)) {
        // Check if one point is in the area
        for (size_t i=0; i<childRegion->areas.size(); i++) {
          bool match=IsAreaAtLeastPartlyInArea(nodes,childRegion->areas[i]);

          if (match) {
            bool completeMatch=AddPOIWayToRegion(progress,
                                                 *childRegion,
                                                 fileOffset,
                                                 name,
                                                 nodes,
                                                 boundingBox,
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

      scanner.Read(wayCount);

      FileOffset            fileOffset;
      uint32_t              tmpType;
      TypeId                typeId;
      TypeInfoRef           type;
      std::string           name;
      std::string           location;
      std::vector<Point>    nodes;

      for (uint32_t w=1; w<=wayCount; w++) {
        progress.SetProgress(w,wayCount);

        scanner.ReadFileOffset(fileOffset);
        scanner.ReadNumber(tmpType);
        scanner.Read(name);
        scanner.Read(location);
        scanner.Read(nodes,false);

        typeId=(TypeId)tmpType;
        type=typeConfig.GetWayTypeInfo(typeId);

        bool isPOI=!name.empty() &&
                   type->GetIndexAsPOI();

        if (!isPOI) {
          continue;
        }

        GeoBox boundingBox;

        GetBoundingBox(nodes,
                       boundingBox);

        RegionRef region=regionIndex.GetRegionForNode(rootRegion,
                                                      boundingBox.GetCenter());

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
                            boundingBox,
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
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }

  std::map<std::string,LocationIndexGenerator::RegionLocation>::iterator LocationIndexGenerator::FindLocation(Progress& progress,
                                                                                                              Region& region,
                                                                                                              const std::string &locationName)
  {
    std::map<std::string,RegionLocation>           &locations=region.locations;
    std::map<std::string,RegionLocation>::iterator loc=locations.find(locationName);

    if (loc!=locations.end()) {
      // exact match
      return loc;
    }

    // Fallback: look if any other location does match case insensitive

    std::wstring wLocation(UTF8StringToWString(locationName));
    std::transform(wLocation.begin(),wLocation.end(),wLocation.begin(),::tolower);

    for (loc=locations.begin(); loc!=locations.end(); loc++) {
      std::wstring wLocation2(UTF8StringToWString(loc->first));
      std::transform(wLocation2.begin(),wLocation2.end(),wLocation2.begin(),::tolower);

      if (wLocation==wLocation2) {
        progress.Debug(std::string("Using address '") + loc->first + "' instead of '" + locationName + "'");

        return loc;
      }
    }

    // if locationName is same as region.name (or its name alias) add new location entry
    // it is usual case for addresses without street and defined addr:place
    std::wstring wRegionName(UTF8StringToWString(region.name));
    std::transform(wRegionName.begin(),wRegionName.end(),wRegionName.begin(),::tolower);

    if (wRegionName==wLocation) {
      RegionLocation newLoc = {0, std::list<ObjectFileRef>(), std::list<RegionAddress>()};
      newLoc.objects.push_back(region.reference);
      locations[region.name]=newLoc;
      progress.Debug(std::string("Create virtual location for region '")+region.name+"'");
      return locations.find(region.name);
    }

    for (auto &alias: region.aliases) {
      std::wstring wRegionName(UTF8StringToWString(alias.name));
      std::transform(wRegionName.begin(),wRegionName.end(),wRegionName.begin(),::tolower);

      if (wRegionName==wLocation) {
        RegionLocation newLoc = {0, std::list<ObjectFileRef>(), std::list<RegionAddress>()};
        newLoc.objects.push_back(ObjectFileRef(alias.reference,refNode));
        locations[alias.name]=newLoc;
        progress.Debug(std::string("Create virtual location for '")+alias.name+"' (alias of region "+region.name+")");
        return locations.find(alias.name);
      }
    }

    return locations.end();
  }

  void LocationIndexGenerator::AddAddressNodeToRegion(Progress& progress,
                                                      Region& region,
                                                      const FileOffset& fileOffset,
                                                      const std::string& location,
                                                      const std::string& address,
                                                      const std::string &postalCode,
                                                      bool& added)
  {
    std::map<std::string,RegionLocation>::iterator loc=FindLocation(progress,region,location);

    if (loc==region.locations.end()) {
      errorReporter->ReportLocationDebug(ObjectFileRef(fileOffset,refNode),
                                         std::string("Street of address '")+location+
                                         "' '"+address+"' cannot be resolved in region '"+region.name+"'");
      return;
    }

    // It is possible that the address is already available at the location
    // We add it anyway. It is possible that multiple nodes share the same address (because their are in the
    // same building) and an area (the building) might hold the address, too.

    RegionAddress regionAddress;

    regionAddress.name=address;
    regionAddress.postalCode=postalCode;
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
    size_t      postalCodeFound=0;

    try {
      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   SortNodeDataGenerator::NODEADDRESS_DAT),
                   FileScanner::Sequential,
                   true);

      scanner.Read(nodeCount);

      FileOffset  fileOffset;
      uint32_t    tmpType;
      TypeId      typeId;
      TypeInfoRef type;
      std::string name;
      std::string location;
      std::string address;
      std::string postalCode;
      GeoCoord    coord;

      for (uint32_t n=1; n<=nodeCount; n++) {
        progress.SetProgress(n,nodeCount);

        scanner.ReadFileOffset(fileOffset);
        scanner.ReadNumber(tmpType);
        scanner.Read(name);
        scanner.Read(postalCode);
        scanner.Read(location);
        scanner.Read(address);

        scanner.ReadCoord(coord);

        typeId=(TypeId)tmpType;
        type=typeConfig.GetNodeTypeInfo(typeId);

        bool isAddress=!location.empty() &&
                       !address.empty();
        bool isPOI=!name.empty() &&
                   type->GetIndexAsPOI();
        bool hasPostalCode=!postalCode.empty();

        if (hasPostalCode)
          postalCodeFound++;

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
                                 postalCode,
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

      progress.Info(NumberToString(nodeCount)+" nodes analyzed, "+
                    NumberToString(addressFound)+" addresses founds, "+
                    NumberToString(poiFound)+" POIs founds, "+
                    NumberToString(postalCodeFound)+" postal codes found");

      scanner.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
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
          writer.Write(address.postalCode);

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

    description.AddProvidedAnalysisFile(FILENAME_LOCATION_REGION_TXT);
    description.AddProvidedAnalysisFile(FILENAME_LOCATION_FULL_TXT);
  }

  bool LocationIndexGenerator::Import(const TypeConfigRef& typeConfig,
                                      const ImportParameter& parameter,
                                      Progress& progress)
  {
    FileWriter                         writer;
    RegionRef                          rootRegion;
    std::vector<std::list<RegionRef>>  regionTree;
    RegionIndex                        regionIndex;
    TypeInfoRef                        boundaryType;
    TypeInfoSet                        boundaryTypes(*typeConfig);
    std::vector<std::list<RegionRef>>  boundaryAreas;
    std::list<std::string>             regionIgnoreTokens;
    std::list<std::string>             locationIgnoreTokens;

    errorReporter=parameter.GetErrorReporter();

    // just from local experience, if it is not enough, GetBoundaryAreas will increase it ;-)
    boundaryAreas.resize(13);

    try {
      bytesForNodeFileOffset=BytesNeededToAddressFileData(AppendFileToDir(parameter.GetDestinationDirectory(),
                                                        "nodes.dat"));
      bytesForAreaFileOffset=BytesNeededToAddressFileData(AppendFileToDir(parameter.GetDestinationDirectory(),
                                                        "areas.dat"));
      bytesForWayFileOffset=BytesNeededToAddressFileData(AppendFileToDir(parameter.GetDestinationDirectory(),
                                                        "ways.dat"));

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

      for (size_t level=0; level<boundaryAreas.size(); level++) {
        progress.SetAction("Sorting in "+NumberToString(boundaryAreas[level].size())+" administrative boundaries of level "+NumberToString(level));

        SortInBoundaries(progress,
                         *rootRegion,
                         boundaryAreas[level]);
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
                                     FILENAME_LOCATION_REGION_TXT));

      progress.SetAction("Dumping location tree");

      DumpLocationTree(progress,
                       *rootRegion,
                       AppendFileToDir(parameter.GetDestinationDirectory(),
                                       FILENAME_LOCATION_FULL_TXT));

      //
      // Generate file with all areas, where areas reference parent and children by offset
      //

      progress.SetAction(std::string("Write '")+LocationIndex::FILENAME_LOCATION_IDX+"'");

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
      progress.Error(e.GetDescription())                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              ;

      writer.CloseFailsafe();

      return false;
    }

    return true;
  }
}
