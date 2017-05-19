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
#include <sstream>
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

  bool LocationIndexGenerator::Region::CouldContain(const Region& region, bool strict) const
  {
    if (strict) {
      // test if the region can be fit into *this when the boundingBox
      // is taken into account
      double area_region = region.boundingBox.GetSize(); // reference area
      GeoBox bx(this->boundingBox.Intersection(region.boundingBox));

      // 95% of the bounding box has to be covered
      if ( !bx.IsValid() || area_region * 0.95 > bx.GetSize() )
        return false;
    }

    for (const auto& bb : boundingBoxes) {
      for (const auto& rbb : region.boundingBoxes) {
        if (bb.Intersects(rbb)) {
          return true;
        }
      }
    }

    return false;
  }

  void LocationIndexGenerator::Region::CalculateProbePoints()
  {
    probePoints.clear();
    if ( boundingBoxes.size() != areas.size() )
      CalculateMinMax();

    for (size_t i=0; i < areas.size(); ++i)
      CalculateProbePointsForArea(i, 0);
  }

  void LocationIndexGenerator::Region::CalculateProbePointsForArea(size_t areaIndex, size_t refinement)
  {
    const size_t targetprobes = 30; // target number of points
                                    // representing an area in region

    const size_t minprobes = 3;     // minimal number of required
                                    // points. If there are less
                                    // points than this minimum, the
                                    // probe points are supplemented
                                    // with the area border points

    const std::vector<GeoCoord> &area = areas[areaIndex];
    const GeoBox &box = boundingBoxes[areaIndex];

    const double split_base = 10;
    double nsplit = split_base * (1 << refinement);

    double delta_lat = (box.GetMaxCoord().GetLat() - box.GetMinCoord().GetLat()) / nsplit;
    double delta_lon = (box.GetMaxCoord().GetLon() - box.GetMinCoord().GetLon()) / nsplit;

    // checking if the selected step is already too small
    const GeoCoord bbox_max_lat(box.GetMaxCoord().GetLat(), box.GetMinCoord().GetLon());
    const GeoCoord bbox_max_lon(box.GetMinCoord().GetLat(), box.GetMaxCoord().GetLon());

    const double distance_lat =
      bbox_max_lon.GetDistance( box.GetMaxCoord() ) /
      (box.GetMaxCoord().GetLat() - box.GetMinCoord().GetLat()); // distance along latitude per degree
    const double distance_lon =
      bbox_max_lat.GetDistance( box.GetMaxCoord() ) /
      (box.GetMaxCoord().GetLon() - box.GetMinCoord().GetLon()); // distance along longitude per degree

    // 100 meters is taken as a smallest step
    const double min_delta_lat = 0.1 / distance_lat;
    const double min_delta_lon = 0.1 / distance_lon;

    if ( refinement > 0 ) {
      if (delta_lat < min_delta_lat && delta_lon < min_delta_lon )
        return; // the refinement is considered to be too fine

      delta_lat = std::max(min_delta_lat, delta_lat);
      delta_lon = std::max(min_delta_lon, delta_lon);
    }
    else {
      // At refinement = 0, i.e. the first call.
      //
      // For too small regions, 10x10 raster is probably too
      // much. Check if the proposed step is smaller than the one
      // corresponding to about 100 meters (min_delta_xxx) and reduce
      // the mesh up to 3x3 raster
      const double min_raster_steps = 3;
      delta_lat *= std::min(split_base/min_raster_steps, std::max(1.0, min_delta_lat/delta_lat));
      delta_lon *= std::min(split_base/min_raster_steps, std::max(1.0, min_delta_lon/delta_lon));
    }

    // check for probe points on composed raster
    for (double lat = box.GetMinCoord().GetLat() + delta_lat*0.5;
         lat < box.GetMaxCoord().GetLat(); lat += delta_lat ) {
      for (double lon = box.GetMinCoord().GetLon() + delta_lon*0.5;
           lon < box.GetMaxCoord().GetLon(); lon += delta_lon ) {
        GeoCoord p(lat, lon);
        if (osmscout::GetRelationOfPointToArea(p,area) > 0)
          probePoints.push_back(p);
      }
    }

    if ( refinement == 2 ) return; // last level

    if ( probePoints.size() < targetprobes )
      CalculateProbePointsForArea( areaIndex, refinement+1 );

    if ( refinement == 0 && probePoints.size() < minprobes )
      probePoints.insert(std::end(probePoints), std::begin(area), std::end(area));
  }

  bool LocationIndexGenerator::Region::Contains(Region& child) const
  {
    // test whether admin levels allow this to contain child
    if (this->level >= 0 /* check if we have admin level defined for this */ &&
        ( child.level >= 0 && this->level >= child.level ) )
      return false;

    if (child.probePoints.empty()) // calculate probe points if they are missing
      child.CalculateProbePoints();

    size_t in = 0;
    size_t out = 0;
    size_t curr_subarea = 0;
    const size_t nareas = areas.size();

    size_t quorum_isin = child.probePoints.size() * 0.9;
    size_t quorum_isout = child.probePoints.size() - quorum_isin;

    for (const GeoCoord& p: child.probePoints)
      {
        bool isin = false;
        for (size_t i=0; !isin && i < nareas; ++i)
          {
            const auto& area = areas[curr_subarea];
            if ( osmscout::GetRelationOfPointToArea(p,area) >= 0 )
              isin = true;
            else
              curr_subarea = ( curr_subarea + 1 ) % nareas;
          }

        if (isin) in++;
        else out++;

        if (out > quorum_isout) return false;
        if (in > quorum_isin) return true;
      }

    return (in > quorum_isin);
  }

  void LocationIndexGenerator::Region::AddLocationObject(const std::string& locationName,
                                                         const ObjectFileRef& objectRef)
  {
    std::string locationNameLower=UTF8StringToLower(locationName);

    RegionLocation& location=locations[locationNameLower];

    std::unordered_map<std::string,size_t>::iterator nameEntry=location.names.find(locationName);

    if (nameEntry!=location.names.end()) {
      nameEntry->second++;
    }
    else {
      location.names[locationName]=1;
    }

    location.objects.push_back(objectRef);
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

  std::string LocationIndexGenerator::RegionLocation::GetName() const
  {
    std::string name;
    size_t      maxCount=0;

    for (const auto& nameEntry : names) {
      if (nameEntry.second>maxCount) {
        name=nameEntry.first;
        maxCount=nameEntry.second;
      }
    }

    return name;
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
        out << " - " << nodeEntry.second.GetName() << std::endl;

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

          out << " @ " << address.name;

          if (!address.postalCode.empty()) {
            out << " " << address.postalCode;

          }

          out << " " << address.object.GetTypeName() << " " << address.object.GetFileOffset() << std::endl;
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

  bool LocationIndexGenerator::AddRegion(Region& parent,
                                         RegionRef& region,
                                         bool assume_contains)
  {
    bool added = false;
    for (const auto& childRegion : parent.regions) {
      if ( childRegion->CouldContain(*region, true) ) {
        added = AddRegion(*childRegion,region,false);
        if (added) return true;
      }
    }

    if ( !added && (assume_contains || parent.Contains(*region)) ) {
      added = true;

      if (!region->isIn.empty() &&
          parent.name!=region->isIn) {
        errorReporter->ReportLocation(region->reference,"'" + region->name + "' parent should be '"+region->isIn+"' but is '"+parent.name+"'");
      }

      // If we already have the same name and are a "minor" reference, we skip...
      if (!(region->name==parent.name &&
            region->reference.type<parent.reference.type)) {
        parent.regions.push_back(region);
      }
    }

    return added;
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
        region->level=level;

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
                                                std::list<RegionRef>& boundaryAreas)
  {
    size_t currentBoundary=0;
    size_t maxBoundary=boundaryAreas.size();

    for (auto&  region : boundaryAreas) {
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
      regionList.second.sort([](const RegionRef& a, const RegionRef& b) -> bool {
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

    region.AddLocationObject(name,ObjectFileRef(area.GetFileOffset(),refArea));

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

    region.AddLocationObject(name,ObjectFileRef(way.GetFileOffset(),refWay));

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
                                         std::string("Street '")+location +"' of address '"+address+"' cannot be resolved in region '"+region.name+"'");
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
                                         std::string("Street '")+location +"' of address '"+address+"' cannot be resolved in region '"+region.name+"'");
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

  /**
   * Return an iterator the the location with the given name. Besides looking for exact matches
   * the methods also tries to find close matches with difference only in case. It will also
   * fake location in cases where the name matches the region name or one of its aliases.
   *
   * @param progress
   *    Progress
   * @param region
   *    Region to search in
   * @param locationName
   *    Name of the location to search for
   * @return
   *    Iterator to the location or region.locations.end()
   */
  std::map<std::string,LocationIndexGenerator::RegionLocation>::iterator LocationIndexGenerator::FindLocation(Progress& progress,
                                                                                                              Region& region,
                                                                                                              const std::string &locationName)
  {
    std::map<std::string,RegionLocation>           &locations=region.locations;
    std::string                                    locationNameSearch=UTF8StringToLower(locationName);
    std::map<std::string,RegionLocation>::iterator loc=locations.find(locationNameSearch);

    if (loc!=locations.end()) {
      // case insensitive match
      return loc;
    }

    // if locationName is same as region.name (or one of its name aliases) add new location entry
    // it is usual case for addresses without street and defined addr:place
    std::string regionNameLower=UTF8StringToLower(region.name);

    if (regionNameLower==locationNameSearch) {
      region.AddLocationObject(region.name,region.reference);

      progress.Debug(std::string("Create virtual location for region '")+region.name+"'");
      return locations.find(region.name);
    }

    for (auto &alias: region.aliases) {
      std::string regionAliasNameLower=UTF8StringToLower(alias.name);

      if (regionAliasNameLower==locationNameSearch) {
        region.AddLocationObject(alias.name,ObjectFileRef(alias.reference,refNode));

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

      writer.Write(location.second.GetName());
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
      progress.Error(e.GetDescription());

      writer.CloseFailsafe();

      return false;
    }

    return true;
  }
}
