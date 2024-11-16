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

#include <osmscoutimport/GenLocationIndex.h>

#include <algorithm>
#include <sstream>
#include <limits>
#include <list>
#include <map>
#include <set>

#include <osmscout/Pixel.h>

#include <osmscout/FeatureReader.h>

#include <osmscout/db/LocationIndex.h>

#include <osmscout/db/AreaDataFile.h>
#include <osmscout/db/NodeDataFile.h>
#include <osmscout/db/WayDataFile.h>

#include <osmscout/feature/AdminLevelFeature.h>
#include <osmscout/feature/IsInFeature.h>
#include <osmscout/feature/NameFeature.h>
#include <osmscout/feature/NameAltFeature.h>
#include <osmscout/feature/PostalCodeFeature.h>
#include <osmscout/feature/RefFeature.h>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

#include <osmscout/io/File.h>
#include <osmscout/io/FileScanner.h>
#include <osmscout/io/FileWriter.h>

#include <osmscout/util/GeoBox.h>
#include <osmscout/util/Geometry.h>

#include <osmscoutimport/SortWayDat.h>
#include <osmscoutimport/SortNodeDat.h>
#include <osmscoutimport/GenAreaAreaIndex.h>

//#define REGION_DEBUG

#if defined(REGION_DEBUG)
  #include <iostream>
#endif

namespace osmscout {

  static const size_t REGION_INDEX_LEVEL=14;

  const char* const LocationIndexGenerator::FILENAME_LOCATION_REGION_TXT  = "location_region.txt";
  const char* const LocationIndexGenerator::FILENAME_LOCATION_FULL_TXT    = "location_full.txt";
  const char* const LocationIndexGenerator::FILENAME_LOCATION_METRICS_TXT = "location_metrics.txt";

  /**
   * Print the given number of spaces to the output to produce an "indent"
   * @param out output stream
   * @param indent number of spaces
   */
  static void printIndent(std::ostream& out, size_t indent)
  {
    for (size_t i=0; i<indent; i++) {
      out << " ";
    }
  }

  namespace locidx {
    void PostalArea::AddLocationObject(const std::string& name,
                                       const ObjectFileRef& objectRef)
    {
      std::string locationNameNorm=UTF8NormForLookup(name);

      RegionLocation& location=locations[locationNameNorm];

      auto nameEntry=location.names.find(name);

      if (nameEntry!=location.names.end()) {
        nameEntry->second++;
      }
      else {
        location.names[name]=1;
      }

      location.objects.push_back(objectRef);
    }

    Region::Region()
    {
      PostalArea postalArea("");

      // Always add an empty postal area for fallback to the index, to avoid "lookup and insert" on demand coast-
      defaultPostalArea=postalAreas.emplace(postalArea.name, postalArea).first;
    }

    /**
     * Calculates the bounding box of each area and the over all bounding box of all
     * areas.
     */
    void Region::CalculateMinMax()
    {
      boundingBoxes.reserve(areas.size());

      for (const auto& area : areas) {
        boundingBoxes.push_back(osmscout::GetBoundingBox(area));
      }

      GeoBox boundingBox;

      for (const auto& box : boundingBoxes) {
        boundingBox.Include(box);
      }

      this->boundingBox=boundingBox;
    }

    bool Region::CouldContain(const GeoBox& boundingBox) const
    {
      if (!this->boundingBox.Intersects(boundingBox)) {
        return false;
      }

      return std::any_of(boundingBoxes.begin(),
                         boundingBoxes.end(),
                         [&boundingBox] (const auto& box) {
                           return box.Intersects(boundingBox);
                         });
    }

    bool Region::CouldContain(const Region& region, bool strict) const
    {
      if (strict) {
        // test if the region can be fit into *this when the boundingBox
        // is taken into account
        double area_region = region.boundingBox.GetSize(); // reference area
        GeoBox bx(this->boundingBox.Intersection(region.boundingBox));

        // 95% of the bounding box has to be covered
        if ( !bx.IsValid() || area_region * 0.95 > bx.GetSize() ) {
          return false;
        }
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

    void Region::CalculateProbePoints()
    {
      probePoints.clear();
      if ( boundingBoxes.size() != areas.size() ) {
        CalculateMinMax();
      }

      size_t areasCount=areas.size();
      for (size_t i=0; i < areasCount; ++i) {
        CalculateProbePointsForArea(i, 0);
      }
    }

    void Region::CalculateProbePointsForArea(size_t areaIndex, size_t refinement)
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
                     bbox_max_lon.GetDistance( box.GetMaxCoord() ).As<Kilometer>() /
                     (box.GetMaxCoord().GetLat() - box.GetMinCoord().GetLat()); // distance along latitude per degree
      const double distance_lon =
                     bbox_max_lat.GetDistance( box.GetMaxCoord() ).As<Kilometer>() /
                     (box.GetMaxCoord().GetLon() - box.GetMinCoord().GetLon()); // distance along longitude per degree

      // 100 meters is taken as a smallest step
      const double min_delta_lat = 0.1 / distance_lat;
      const double min_delta_lon = 0.1 / distance_lon;

      if ( refinement > 0 ) {
        if (delta_lat < min_delta_lat && delta_lon < min_delta_lon ) {
          return; // the refinement is considered to be too fine
        }

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
          if (osmscout::GetRelationOfPointToArea(p,area) > 0) {
            probePoints.push_back(p);
          }
        }
      }

      if ( refinement == 2 ) { return; // last level
      }

      if ( probePoints.size() < targetprobes ) {
        CalculateProbePointsForArea( areaIndex, refinement+1 );
      }

      if ( refinement == 0 && probePoints.size() < minprobes ) {
        probePoints.insert(std::end(probePoints), std::begin(area), std::end(area));
      }
    }

    /**
     * Returns true, if the passed region is contained in the region. Uses some heuristics for
     * a improved decision.
     *
     * @param child
     *   region to probe for
     * @returns true or false
     */
    bool Region::Contains(Region& child) const
    {
#if defined(REGION_DEBUG)
      std::cout << "CHECK: '" << child.name << "' is child of '" << name << std::endl;
#endif

      // test whether admin levels allow this to contain child
      if (this->level >= 0 /* check if we have admin level defined for this */ &&
          ( child.level >= 0 && this->level >= child.level ) ) {
#if defined(REGION_DEBUG)
        std::cout << "=> NO admin level mismatch" << std::endl;
#endif
        return false;
      }

      if (child.probePoints.empty()) {
        // calculate probe points if they are missing
        child.CalculateProbePoints();
      }

      size_t in = 0;
      size_t out = 0;
      size_t curr_subarea = 0;
      const size_t nareas = areas.size();

      size_t quorum_isin = (size_t)round(child.probePoints.size() * 0.9);
      size_t quorum_isout = child.probePoints.size() - quorum_isin;

      for (const auto& p: child.probePoints) {
        bool        isin=false;

        for (size_t i=0; !isin && i<nareas; ++i) {
          const auto& area=areas[curr_subarea];

          if (osmscout::GetRelationOfPointToArea(p,area)>=0) {
            isin=true;
          }
          else {
            curr_subarea=(curr_subarea+1)%nareas;
          }
        }

        if (isin) {
          in++;
        }
        else {
          out++;
        }

        if (out>quorum_isout) {
#if defined(REGION_DEBUG)
          std::cout << "=> NO quorum decision" << std::endl;
#endif
          return false;
        }

        if (in>quorum_isin) {
#if defined(REGION_DEBUG)
          std::cout << "=> YES quorum decision" << std::endl;
#endif
          return true;
        }
      }

#if defined(REGION_DEBUG)
      if (in > quorum_isin) {
      std::cout << "=> YES secondary quorum decision" << std::endl;
    }
    else {
      std::cout << "=> NO secondary quorum decision" << std::endl;
    }
#endif

      return (in > quorum_isin);
    }

    void Region::AddAlias(const RegionAlias& location,
                          const GeoCoord& node)
    {
      for (const auto& childRegion : regions) {
        for (const auto& area : childRegion->areas) {
          if (IsCoordInArea(node,area)) {
            childRegion->AddAlias(location,
                                  node);
            return;
          }
        }
      }

      if (name==location.name) {
        return;
      }

      aliases.push_back(location);
    }

    void Region::AddPOINode(const FileOffset& fileOffset,
                            const std::string& name,
                            bool& added)
    {
      RegionPOI poi(name,ObjectFileRef(fileOffset,refNode));

      pois.push_back(poi);

      added=true;
    }

    void Region::AddPOIArea(const FileOffset& fileOffset,
                            const std::string& name,
                            const std::vector<Point>& nodes,
                            const GeoBox& boundingBox,
                            bool& added)
    {
      for (const auto& childRegion : regions) {
        // Fast check, if the object is in the bounds of the area
        if (!childRegion->CouldContain(boundingBox)) {
          continue;
        }

        for (const auto& childArea : childRegion->areas) {
          if (IsAreaCompletelyInArea(nodes,childArea)) {
            childRegion->AddPOIArea(fileOffset,
                                    name,
                                    nodes,
                                    boundingBox,
                                    added);
            return;
          }
        }
      }

      locidx::RegionPOI poi(name,ObjectFileRef(fileOffset,refArea));

      pois.push_back(poi);

      added=true;
    }

    bool Region::AddPOIWay(const FileOffset& fileOffset,
                           const std::string& name,
                           const std::vector<Point>& nodes,
                           const GeoBox& boundingBox,
                           bool& added)
    {
      for (const auto& childRegion : regions) {
        // Fast check, if the object is in the bounds of the area
        if (childRegion->CouldContain(boundingBox)) {
          // Check if one point is in the area
          for (const auto& childArea : childRegion->areas) {
            bool match=IsAreaAtLeastPartlyInArea(nodes,childArea);

            if (match) {
              bool completeMatch=childRegion->AddPOIWay(fileOffset,
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

      locidx::RegionPOI poi(name,ObjectFileRef(fileOffset,refWay));

      pois.push_back(poi);

      added=true;

      return std::any_of(areas.begin(),
                         areas.end(),
                         [&nodes] (const auto& area) {
                           return IsAreaCompletelyInArea(nodes,area);
                         });
    }

    /**
     * Add the given location to the region. Creates a new postal area in case an postal area with the given
     * name does not yet exist for the region.
     *
     * @param name
     *    name of the location
     * @param postalCode
     *    optional postal code of the location
     * @param objectRef the object that represents the location
     */
    void Region::AddLocationObject(const std::string& name,
                                   const std::string& postalCode,
                                   const ObjectFileRef& objectRef)
    {
      if (!postalCode.empty()) {
        auto postalAreaEntry=postalAreas.find(postalCode);

        if (postalAreaEntry==postalAreas.end()) {
          PostalArea postalArea(postalCode);

          postalAreaEntry=postalAreas.emplace(postalCode,postalArea).first;
        }

        postalAreaEntry->second.AddLocationObject(name,
                                                  objectRef);
      }

      defaultPostalArea->second.AddLocationObject(name,
                                                  objectRef);
    }

    bool Region::AddLocationArea(const Area& area,
                                 const std::vector<Point>& nodes,
                                 const std::string& name,
                                 const std::string& postalCode,
                                 const GeoBox& boundingBox)
    {
      for (const auto& childRegion : regions) {
        // Fast check, if the object is in the bounds of the area
        if (!childRegion->CouldContain(boundingBox)) {
          continue;
        }

        for (const auto& childArea : childRegion->areas) {
          // Check if one point is in the area
          bool match=IsCoordInArea(nodes[0],childArea);

          if (match) {
            bool completeMatch=childRegion->AddLocationArea(area,
                                                            nodes,
                                                            name,
                                                            postalCode,
                                                            boundingBox);

            if (completeMatch) {
              // We are done, the object is completely enclosed by one of our sub areas
              return true;
            }
          }
        }
      }

      // If we (at least partly) contain it, we add it to the area but continue
      // This means we either do not have any child area or the object is only partially
      // in a child area (and thus should be part of this area, too)

      AddLocationObject(name,
                        postalCode,
                        ObjectFileRef(area.GetFileOffset(),refArea));

      return std::any_of(areas.begin(),
                         areas.end(),
                         [&nodes] (const auto& childArea) {
                           return IsAreaCompletelyInArea(nodes,childArea);
                         });
    }

    /**
      Add the given location way to the hierarchical area index.

      The code is designed to minimize the number of "point in area" checks, it assumes that
      if one point of an object is in a area it is very likely that all points of the object
      are in the area.
      */
    bool Region::AddLocationWay(const Way& way,
                                const std::string& name,
                                const std::string& postalCode,
                                const GeoBox& boundingBox)
    {
      for (const auto& childRegion : regions) {
        // Fast check, if the object is in the bounds of the area
        if (!childRegion->CouldContain(boundingBox)) {
          continue;
        }

        // Check if one point is in the area
        for (const auto& childArea : childRegion->areas) {
          bool match=IsAreaAtLeastPartlyInArea(way.nodes,childArea);

          if (match) {
            bool completeMatch=childRegion->AddLocationWay(way,
                                                           name,
                                                           postalCode,
                                                           boundingBox);

            if (completeMatch) {
              // We are done, the object is completely enclosed by one of our sub areas
              return true;
            }
          }
        }
      }

      // If we (at least partly) contain it, we add it to the area but continue

      AddLocationObject(name,
                        postalCode,
                        ObjectFileRef(way.GetFileOffset(),refWay));

      return std::any_of(areas.begin(),
                         areas.end(),
                         [&way] (const auto& area) {
                           return IsAreaCompletelyInArea(way.nodes,area);
                         });
    }

    bool Region::AddRegion(const RegionRef& region,
                           bool assume_contains)
    {
      bool added = false;
      for (const auto& childRegion : regions) {
        if (childRegion->CouldContain(*region,true)) {
          added=childRegion->AddRegion(region,false);

          if (added) {
            return true;
          }
        }
      }

      if (!added &&
          (assume_contains ||
           Contains(*region))) {
        added=true;

        // If we already have the same name and are a "minor" reference, we skip...
        if (!(region->name==name &&
              region->reference.type<reference.type)) {
#if defined(REGION_DEBUG)
          std::cout << "Added region '"  << region->name << "' " << region->reference.GetName() << " below '" << parent.name << "' " << parent.reference.GetName() << std::endl;
#endif
          regions.push_back(region);
        }
      }

      return added;
    }

    RegionIndex::RegionIndex(double cellWidth,
                             double cellHeight)
    : cellWidth(cellWidth),
      cellHeight(cellHeight)
    {
    }

    void RegionIndex::IndexRegions(const std::vector<std::list<locidx::RegionRef> >& regionTree)
    {
      for (size_t level=regionTree.size()-1; level>=1; level--) {
        for (const auto& region : regionTree[level]) {
          for (const auto& boundingBox : region->GetAreaBoundingBoxes()) {
            uint32_t cellMinX=(uint32_t)((boundingBox.GetMinLon()+180.0)/cellWidth);
            uint32_t cellMaxX=(uint32_t)((boundingBox.GetMaxLon()+180.0)/cellWidth);
            uint32_t cellMinY=(uint32_t)((boundingBox.GetMinLat()+90.0)/cellHeight);
            uint32_t cellMaxY=(uint32_t)((boundingBox.GetMaxLat()+90.0)/cellHeight);

            for (uint32_t y=cellMinY; y<=cellMaxY; y++) {
              for (uint32_t x=cellMinX; x<=cellMaxX; x++) {
                Pixel pixel(x,y);

                index[pixel].push_back(region);
              }
            }
          }
        }
      }

      for (auto& [pixel,regionList] : index) {
        regionList.sort([](const locidx::RegionRef& a, const locidx::RegionRef& b) {
          return a->GetBoundingBox().GetSize()<b->GetBoundingBox().GetSize();
        });
      }
    }

    /**
     * Returns the region that contains the given geo coordinate or the passed root region
     * if no child region was found.
     *
     * @param rootRegion
     *    region to return if no other region was found
     * @param coord
     *    coordinate to search for
     * @return
     *    a region or the passed root region
     */
    RegionRef RegionIndex::GetRegionForNode(const RegionRef& rootRegion,
                                            const GeoCoord& coord) const
    {
      uint32_t minX=(uint32_t)((coord.GetLon()+180.0)/cellWidth);
      uint32_t minY=(uint32_t)((coord.GetLat()+90.0)/cellHeight);

      const auto indexCell=index.find(Pixel(minX,minY));

      if (indexCell!=index.end()) {
        for (const auto& region : indexCell->second) {
          for (const auto& area : region->areas) {
            if (IsCoordInArea(coord,area)) {
              return region;
            }
          }
        }
      }

      return rootRegion;
    }

    std::string RegionLocation::GetName() const
    {
      std::string name;
      size_t      maxCount=0;

      for (const auto& [entryName, entryCount] : names) {
        if (entryCount>maxCount) {
          name=entryName;
          maxCount=entryCount;
        }
      }

      return name;
    }

    /**
 * Ignore 1 or 2 word tokens for strings with more than two tokens, where each token must not have more than 5
 * characters. Blacklist strings that in turn consists of 2 or less tokens, where tokens have 5 or less
 * characters.
 *
 * @param string the string to token- and analyse
 * @param ignoreTokens list of to be ignored tokens and the count of appearance
 * @param blacklist
 */
    static void AnalyseStringForIgnoreTokens(const std::string& string,
                                             std::unordered_map<std::string,size_t>& ignoreTokens,
                                             std::unordered_set<std::string>& blacklist)
    {
      if (string.empty()) {
        return;
      }

      std::list<std::string> tokens=SplitStringAtSpace(string);

      if (tokens.size()>2) {
        for (auto token=tokens.begin();
             token!=tokens.end();
             ++token) {
          if (token->length()<=5) {
            auto entry=ignoreTokens.find(*token);

            if (entry==ignoreTokens.end()) {
              ignoreTokens.emplace(*token,1);
            }
            else {
              entry->second++;
            }
          }

          auto nextToken=token;
          ++nextToken;

          if (nextToken!=tokens.end() &&
              nextToken->length()<=5) {
            std::string composition=*token+" "+*nextToken;

            auto entry=ignoreTokens.find(composition);

            if (entry==ignoreTokens.end()) {
              ignoreTokens.emplace(composition,1);
            }
            else {
              entry->second++;
            }
          }
        }
      }
      else if (tokens.size()==2) {
        std::list<std::string>::const_iterator token=tokens.begin();
        std::list<std::string>::const_iterator nextToken=token;

        if (token->length()<=5 &&
            nextToken->length()<=5) {
          blacklist.insert(*token+" "+*nextToken);
        }
      }
      else if (tokens.size()==1 &&
               tokens.front().length()<=5) {
        blacklist.insert(tokens.front());
      }
    }

    /**
     * @param region
     * @param ignoreTokens
     * @param blacklist
     */
    static void CalculateRegionNameIgnoreTokens(const Region& region,
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

      // recursion...

      for (const auto& childRegion : region.regions) {
        CalculateRegionNameIgnoreTokens(*childRegion,
                                        ignoreTokens,
                                        blacklist);
      }
    }

    static void CalculatePOINameIgnoreTokens(const Region& region,
                                             std::unordered_map<std::string,size_t>& ignoreTokens,
                                             std::unordered_set<std::string>& blacklist)
    {
      for (const auto& poi : region.pois) {
        AnalyseStringForIgnoreTokens(poi.name,
                                     ignoreTokens,
                                     blacklist);
      }

      // recursion...

      for (const auto& childRegion : region.regions) {
        CalculatePOINameIgnoreTokens(*childRegion,
                                     ignoreTokens,
                                     blacklist);
      }
    }

    static void CalculateLocationNameIgnoreTokens(const Region& region,
                                                  std::unordered_map<std::string,size_t>& ignoreTokens,
                                                  std::unordered_set<std::string>& blacklist)
    {
      for (const auto& [id,area] : region.postalAreas) {
        for (const auto& [name,location] : area.locations) {
          AnalyseStringForIgnoreTokens(name,
                                       ignoreTokens,
                                       blacklist);
        }
      }

      // recursion...

      for (const auto& childRegion : region.regions) {
        CalculateLocationNameIgnoreTokens(*childRegion,
                                          ignoreTokens,
                                          blacklist);
      }
    }

    static bool CalculateIgnoreTokens(const Region& rootRegion,
                                      std::list<std::string>& regionTokens,
                                      std::list<std::string>& poiTokens,
                                      std::list<std::string>& locationTokens)
    {
      std::unordered_map<std::string,size_t> regionIgnoreTokens;
      std::unordered_set<std::string>        regionBlacklist;
      std::unordered_map<std::string,size_t> poiIgnoreTokens;
      std::unordered_set<std::string>        poiBlacklist;
      std::unordered_map<std::string,size_t> locationIgnoreTokens;
      std::unordered_set<std::string>        locationBlacklist;

      regionTokens.clear();
      poiTokens.clear();
      locationTokens.clear();

      CalculateRegionNameIgnoreTokens(rootRegion,
                                      regionIgnoreTokens,
                                      regionBlacklist);

      CalculatePOINameIgnoreTokens(rootRegion,
                                   poiIgnoreTokens,
                                   poiBlacklist);

      CalculateLocationNameIgnoreTokens(rootRegion,
                                        locationIgnoreTokens,
                                        locationBlacklist);

      size_t regionLimit=regionIgnoreTokens.size()/100;

      if (regionLimit<5) {
        regionLimit=5;
      }

      size_t poiLimit=poiIgnoreTokens.size()/100;

      if (poiLimit<5) {
        poiLimit=5;
      }

      size_t locationLimit=locationIgnoreTokens.size()/100;

      if (locationLimit<5) {
        locationLimit=5;
      }

      for (const auto& [token,tokenCount] : regionIgnoreTokens) {
        if (tokenCount>=regionLimit) {
          if (regionBlacklist.find(token)==regionBlacklist.end()) {
            regionTokens.push_back(token);
          }

          if (poiBlacklist.find(token)==poiBlacklist.end()) {
            poiTokens.push_back(token);
          }

          if (locationBlacklist.find(token)==locationBlacklist.end()) {
            locationTokens.push_back(token);
          }
        }
      }

      for (const auto& [token,tokenCount] : poiIgnoreTokens) {
        if (tokenCount>=poiLimit) {
          if (regionBlacklist.find(token)==regionBlacklist.end()) {
            regionTokens.push_back(token);
          }

          if (poiBlacklist.find(token)==poiBlacklist.end()) {
            poiTokens.push_back(token);
          }

          if (locationBlacklist.find(token)==locationBlacklist.end()) {
            locationTokens.push_back(token);
          }
        }
      }

      for (const auto& [token,tokenCount] : locationIgnoreTokens) {
        if (tokenCount>=locationLimit) {
          if (regionBlacklist.find(token)==regionBlacklist.end()) {
            regionTokens.push_back(token);
          }

          if (poiBlacklist.find(token)==poiBlacklist.end()) {
            poiTokens.push_back(token);
          }

          if (locationBlacklist.find(token)==locationBlacklist.end()) {
            locationTokens.push_back(token);
          }
        }
      }

      return true;
    }
  }

  void LocationIndexGenerator::Write(FileWriter& writer,
                                     const ObjectFileRef& object) const
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
      //std::cout << "type: " << (int) object.GetType() << std::endl;
      throw IOException(writer.GetFilename(),"Cannot write ObjectFileRef","Unknown object file type");
    }
  }

  void LocationIndexGenerator::CalculateRegionMetrics(const locidx::Region& region,
                                                      locidx::RegionMetrics& metrics) const
  {
    metrics.minRegionChars=std::min(metrics.minRegionChars,(uint32_t)region.name.length());
    metrics.maxRegionChars=std::max(metrics.maxRegionChars,(uint32_t)region.name.length());
    metrics.minRegionWords=std::min(metrics.minRegionWords,(uint32_t)CountWords(region.name));
    metrics.maxRegionWords=std::max(metrics.maxRegionWords,(uint32_t)CountWords(region.name));

    for (const auto& alias : region.aliases) {
      metrics.minRegionChars=std::min(metrics.minRegionChars,(uint32_t)alias.name.length());
      metrics.maxRegionChars=std::max(metrics.maxRegionChars,(uint32_t)alias.name.length());
      metrics.minRegionWords=std::min(metrics.minRegionWords,(uint32_t)CountWords(alias.name));
      metrics.maxRegionWords=std::max(metrics.maxRegionWords,(uint32_t)CountWords(alias.name));
    }

    for (const auto& poi : region.pois) {
      metrics.maxPOIWords=std::max(metrics.maxPOIWords,(uint32_t)CountWords(poi.name));
    }

    for (const auto& [id,area] : region.postalAreas) {
      for (const auto& [name,location] : area.locations) {
        metrics.minLocationChars=std::min(metrics.minLocationChars,(uint32_t)location.GetName().length());
        metrics.maxLocationChars=std::max(metrics.maxLocationChars,(uint32_t)location.GetName().length());
        metrics.minLocationWords=std::min(metrics.minLocationWords,(uint32_t)CountWords(location.GetName()));
        metrics.maxLocationWords=std::max(metrics.maxLocationWords,(uint32_t)CountWords(location.GetName()));

        for (const auto& address : location.addresses) {
          metrics.maxAddressWords=std::max(metrics.maxAddressWords,(uint32_t)CountWords(address.name));
        }
      }
    }

    for (const auto& childRegion : region.regions) {
      CalculateRegionMetrics(*childRegion,
                             metrics);
    }
  }

  void LocationIndexGenerator::DumpRegion(const locidx::Region& parent,
                                          size_t indent,
                                          std::ostream& out) const
  {
    for (const auto& childRegion : parent.regions) {
      printIndent(out,indent);
      out << " + " << childRegion->name
          << " " << childRegion->reference.GetTypeName()
          << " " << childRegion->reference.GetFileOffset();

      if (childRegion->level>=0) {
        out << " (admin level " << size_t(childRegion->level) << ")";
      }

      if (!childRegion->isIn.empty()) {
        out << " (in " << childRegion->isIn << ")";
      }

      if (childRegion->areas.size()>1) {
        out << " " << childRegion->areas.size() << " areas";
      }

      out << std::endl;

      for (const auto& alias : childRegion->aliases) {
        printIndent(out,indent+2);
        out << " = " << alias.name << " Node " << alias.reference << std::endl;
      }

      for (const auto& [id,area] : childRegion->postalAreas) {
        printIndent(out,indent+2);
        out << " # " << area.name << std::endl;
      }

      DumpRegion(*childRegion,
                 indent+2,
                 out);
    }
  }

  bool LocationIndexGenerator::DumpRegionTree(Progress& progress,
                                              const locidx::Region& rootRegion,
                                              const std::string& filename) const
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

  void LocationIndexGenerator::DumpRegionAndData(const locidx::Region& parent,
                                                 size_t indent,
                                                 std::ostream& out) const
  {
    for (const auto& childRegion : parent.regions) {
      printIndent(out,indent);
      out << " + " << childRegion->name
          << " " << childRegion->reference.GetTypeName()
          << " " << childRegion->reference.GetFileOffset();


      if (childRegion->level>=0) {
        out << " (admin level " << size_t(childRegion->level) << ")";
      }

      if (childRegion->areas.size()>1) {
        out << " " << childRegion->areas.size() << " areas";
      }

      out << std::endl;

      for (const auto& alias : childRegion->aliases) {
        printIndent(out,indent+2);
        out << " = " << alias.name << " Node " << alias.reference << std::endl;
      }

      for (const auto& poi : childRegion->pois) {
        printIndent(out,indent+2);
        out << " * " << poi.name << " " << poi.object.GetTypeName() << " " << poi.object.GetFileOffset() << std::endl;
      }

      for (const auto& [id,area] : childRegion->postalAreas) {
        printIndent(out,indent+2);
        out << " # " << area.name << std::endl;

        for (const auto& [name,location] : area.locations) {
          printIndent(out,indent+4);
          out << " - " << location.GetName() << std::endl;

          for (const auto& object : location.objects) {
            printIndent(out,indent+6);
            out << " = " << object.GetTypeName() << " " << object.GetFileOffset() << std::endl;
          }

          for (const auto& address : location.addresses) {
            printIndent(out,indent+8);
            out << " @ " << address.name;
            out << " " << address.object.GetTypeName() << " " << address.object.GetFileOffset() << std::endl;
          }
        }
      }

      DumpRegionAndData(*childRegion,
                        indent+2,
                        out);
    }
  }

  bool LocationIndexGenerator::DumpLocationTree(Progress& progress,
                                                const locidx::Region& rootRegion,
                                                const std::string& filename) const
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

  bool LocationIndexGenerator::DumpLocationMetrics(Progress& progress,
                                                   const std::string& filename,
                                                   const locidx::RegionMetrics& metrics,
                                                   const std::list<std::string>& regionIgnoreTokens,
                                                   const std::list<std::string>& poiIgnoreTokens,
                                                   const std::list<std::string>& locationIgnoreTokens) const
  {
    std::ofstream debugStream;

    debugStream.imbue(std::locale::classic());
    debugStream.open(filename.c_str(),
                     std::ios::out|std::ios::trunc);

    if (!debugStream.is_open()) {
      progress.Error("Cannot open '"+filename+"'");

      return false;
    }

    debugStream << "Region ignore Tokens:" << std::endl;
    for (const auto& token : regionIgnoreTokens) {
      debugStream << "* " << token << std::endl;
    }
    debugStream << std::endl;

    debugStream << "POI ignore Tokens:" << std::endl;
    for (const auto& token : poiIgnoreTokens) {
      debugStream << "* " << token << std::endl;
    }
    debugStream << std::endl;

    debugStream << "Location ignore Tokens:" << std::endl;
    for (const auto& token : locationIgnoreTokens) {
      debugStream << "* " << token << std::endl;
    }
    debugStream << std::endl;

    debugStream << "Metrics:" << std::endl;
    debugStream << "* Region chars: " << metrics.minRegionChars << " - " << metrics.maxRegionChars << std::endl;
    debugStream << "* Region words: " << metrics.minRegionWords << " - " << metrics.maxRegionWords << std::endl;
    debugStream << "* Location chars: " << metrics.minLocationChars << " - " << metrics.maxLocationChars << std::endl;
    debugStream << "* Location words: " << metrics.minLocationWords << " - " << metrics.maxLocationWords << std::endl;
    debugStream << "* Max address words: " << metrics.maxAddressWords << std::endl;
    debugStream << "* Max POI words: " << metrics.maxPOIWords << std::endl;

    debugStream.close();

    return true;
  }

  /**
    Return the list of ways of type administrative boundary.
    */
  bool LocationIndexGenerator::GetBoundaryAreas(const ImportParameter& parameter,
                                                Progress& progress,
                                                const TypeConfigRef& typeConfig,
                                                const TypeInfoSet& boundaryTypes,
                                                std::vector<std::list<locidx::RegionRef>>& boundaryAreas) const
  {
    FileScanner                  scanner;
    NameFeatureValueReader       nameReader(*typeConfig);
    NameAltFeatureValueReader    nameAltReader(*typeConfig);
    AdminLevelFeatureValueReader adminLevelReader(*typeConfig);

    try {
      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   AreaDataFile::AREAS_DAT),
                   FileScanner::Sequential,
                   true);

      uint32_t areaCount=scanner.ReadUInt32();

      for (uint32_t r=1; r<=areaCount; r++) {
        progress.SetProgress(r,areaCount);

        Area area;

        area.Read(*typeConfig,
                  scanner);

        if (!boundaryTypes.IsSet(area.GetType())) {
          continue;
        }

        const NameFeatureValue *nameValue=nameReader.GetValue(area.rings.front().GetFeatureValueBuffer());

        if (nameValue==nullptr) {
          errorReporter->ReportLocation(ObjectFileRef(area.GetFileOffset(),refArea),"No name");
          continue;
        }

        const NameAltFeatureValue *nameAltValue=nameAltReader.GetValue(area.rings.front().GetFeatureValueBuffer());

        const AdminLevelFeatureValue *adminLevelValue=adminLevelReader.GetValue(area.rings.front().GetFeatureValueBuffer());

        if (adminLevelValue==nullptr) {
          errorReporter->ReportLocation(ObjectFileRef(area.GetFileOffset(),refArea),"No tag 'admin_level'");
          continue;
        }

        if (adminLevelValue->GetAdminLevel()>parameter.GetMaxAdminLevel()) {
          errorReporter->ReportLocation(ObjectFileRef(area.GetFileOffset(),refArea),"Admin level greater than limit");
          continue;
        }

        locidx::RegionRef region=std::make_shared<locidx::Region>();
        uint8_t   level=adminLevelValue->GetAdminLevel();

        region->reference=area.GetObjectFileRef();
        region->name=nameValue->GetName();
        if (nameAltValue!=nullptr){
          region->altName=nameAltValue->GetNameAlt();
        }
        region->level=(int8_t)level;

        if (!adminLevelValue->GetIsIn().empty()) {
          region->isIn=GetFirstInStringList(adminLevelValue->GetIsIn(),",;");
        }

        for (const auto& ring : area.rings) {
          if (ring.IsTopOuter()) {
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

#if defined(REGION_DEBUG)
        std::cout << "Loaded administrative boundary " << region->level << " '" << region->name << "' " << region->reference.GetName() << std::endl;

        for (const auto& area : region->areas) {
          std::cout << "- area " << region->GetBoundingBox().GetDisplayText() << std::endl;

          for (const auto& coord : area) {
            std::cout << "  * " << coord.GetDisplayText() << std::endl;
          }
        }
#endif

        boundaryAreas[level].push_back(region);
      }

      scanner.Close();
    }
    catch (const IOException& e) {
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }

  void LocationIndexGenerator::SortInBoundaries(Progress& progress,
                                                locidx::Region& rootRegion,
                                                const std::list<locidx::RegionRef>& boundaryAreas)
  {
    size_t currentBoundary=0;
    size_t maxBoundary=boundaryAreas.size();

    for (const auto& region : boundaryAreas) {
      currentBoundary++;

      progress.SetProgress(currentBoundary,
                           maxBoundary);

      rootRegion.AddRegion(region);
    }
  }

  bool LocationIndexGenerator::GetRegionAreas(const TypeConfig& typeConfig,
                                              const ImportParameter& parameter,
                                              Progress& progress,
                                              std::list<locidx::RegionRef>& regionAreas) const
  {
    FileScanner scanner;

    try {
      size_t                 areasFound=0;
      NameFeatureValueReader nameReader(typeConfig);
      IsInFeatureValueReader isInReader(typeConfig);

      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   AreaDataFile::AREAS_DAT),
                   FileScanner::Sequential,
                   parameter.GetWayDataMemoryMaped());

      uint32_t areaCount=scanner.ReadUInt32();

      for (uint32_t a=1; a<=areaCount; a++) {
        progress.SetProgress(a,areaCount);

        Area area;

        area.Read(typeConfig,
                  scanner);

        if (!area.GetType()->GetIndexAsRegion()) {
          continue;
        }

        const NameFeatureValue *nameValue=nameReader.GetValue(area.rings.front().GetFeatureValueBuffer());

        if (nameValue==nullptr) {
          errorReporter->ReportLocation(ObjectFileRef(area.GetFileOffset(),refArea),"No name");
          continue;
        }

        locidx::RegionRef region=std::make_shared<locidx::Region>();

        region->reference.Set(area.GetFileOffset(),refArea);
        region->name=nameValue->GetName();

        const IsInFeatureValue *isInValue=isInReader.GetValue(area.rings.front().GetFeatureValueBuffer());

        if (isInValue!=nullptr) {
          region->isIn=GetFirstInStringList(isInValue->GetIsIn(),",;");
        }

        for (const auto& ring : area.rings) {
          if (ring.IsTopOuter()) {
            std::vector<GeoCoord> coords;

            std::transform(ring.nodes.cbegin(),
                           ring.nodes.cend(),
                           std::back_inserter(coords),
                           [] (const Point& point) {
                             return point.GetCoord();
            });

            region->areas.push_back(coords);
          }
        }

        region->CalculateMinMax();

#if defined(REGION_DEBUG)
        std::cout << "Loaded region area '" << region->name << "' " << region->reference.GetName() << std::endl;

        for (const auto& area : region->areas) {
          std::cout << "- area " << region->GetBoundingBox().GetDisplayText() << std::endl;

          for (const auto& coord : area) {
            std::cout << "  * " << coord.GetDisplayText() << std::endl;
          }
        }
#endif

        regionAreas.push_back(region);

        areasFound++;
      }

      progress.Info("Found {} place regions of type 'area'",areasFound);

      scanner.Close();
    }
    catch (const IOException& e) {
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }

  /**
    Return the list of nodes ids with the given type.
    */
  bool LocationIndexGenerator::SortInRegionAreas(Progress& progress,
                                                 locidx::Region& rootRegion,
                                                 std::list<locidx::RegionRef>& regionAreas)
  {
    size_t currentRegion=0;
    size_t maxRegion=regionAreas.size();

    // We are sorting regions by size, trying to make sure that we insert region in order of the region tree.
    // TODO: Check if the solution used by the MapPainter for area ordering is better
    regionAreas.sort([](const locidx::RegionRef& a, const locidx::RegionRef&b) {
      return a->GetBoundingBox().GetSize()>b->GetBoundingBox().GetSize();
    });

    for (const auto& region : regionAreas) {
      currentRegion++;

      progress.SetProgress(currentRegion,
                           maxRegion);

      rootRegion.AddRegion(region);
    }

    return true;
  }

  unsigned long LocationIndexGenerator::GetRegionTreeDepth(const locidx::Region& rootRegion) const
  {
    unsigned long depth=0;

    for (const auto& childRegion : rootRegion.regions) {
      depth=std::max(depth,GetRegionTreeDepth(*childRegion));
    }

    return depth+1;
  }


  void LocationIndexGenerator::SortInRegion(const locidx::RegionRef& area,
                                            std::vector<std::list<locidx::RegionRef> >& regionTree,
                                            unsigned long level)
  {
    regionTree[level].push_back(area);

    for (const auto& childRegion : area->regions) {
      SortInRegion(childRegion,
                   regionTree,
                   level+1);
    }
  }

  /**
    Return the list of nodes ids with the given type.
    */
  bool LocationIndexGenerator::IndexRegionNodes(const TypeConfigRef& typeConfig,
                                                const ImportParameter& parameter,
                                                Progress& progress,
                                                const locidx::RegionIndex& regionIndex,
                                                locidx::RegionRef& rootRegion)
  {
    FileScanner scanner;

    try {
      size_t                    citiesFound=0;
      NameFeatureValueReader    nameReader(*typeConfig);
      NameAltFeatureValueReader nameAltReader(*typeConfig);

      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   NodeDataFile::NODES_DAT),
                   FileScanner::Sequential,
                   true);

      uint32_t nodeCount=scanner.ReadUInt32();

      for (uint32_t n=1; n<=nodeCount; n++) {
        progress.SetProgress(n,nodeCount);

        Node node;

        node.Read(*typeConfig,
                  scanner);

        if (node.GetType()->GetIndexAsRegion()) {
          const NameFeatureValue *nameValue=nameReader.GetValue(node.GetFeatureValueBuffer());

          if (nameValue==nullptr) {
            errorReporter->ReportLocation(ObjectFileRef(node.GetFileOffset(),refNode),"No name");
            continue;
          }

          const NameAltFeatureValue *nameAltValue=nameAltReader.GetValue(node.GetFeatureValueBuffer());

          locidx::RegionAlias alias;

          alias.reference=node.GetFileOffset();
          alias.name=nameValue->GetName();
          if (nameAltValue!=nullptr){
            alias.altName=nameAltValue->GetNameAlt();
          }

          // This is an optimization. Instead of propagating the alias down the region tree
          // we make a fast lookup for a matching candidate...
          locidx::RegionRef region=regionIndex.GetRegionForNode(rootRegion,
                                                                node.GetCoords());

          region->AddAlias(alias,
                           node.GetCoords());

          citiesFound++;
        }
      }

      progress.Info(std::string("Found ")+std::to_string(citiesFound)+" cities of type 'node'");

      scanner.Close();
    }
    catch (const IOException& e) {
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }

  bool LocationIndexGenerator::IndexLocationAreas(const TypeConfig& typeConfig,
                                                  const ImportParameter& parameter,
                                                  Progress& progress,
                                                  const locidx::RegionIndex& regionIndex,
                                                  locidx::RegionRef& rootRegion)
  {
    FileScanner scanner;

    try {
      size_t                       areasFound=0;
      NameFeatureValueReader       nameReader(typeConfig);
      PostalCodeFeatureValueReader postalCodeReader(typeConfig);

      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   AreaDataFile::AREAS_DAT),
                   FileScanner::Sequential,
                   parameter.GetWayDataMemoryMaped());

      uint32_t areaCount=scanner.ReadUInt32();

      for (uint32_t w=1; w<=areaCount; w++) {
        progress.SetProgress(w,areaCount);

        Area area;

        area.Read(typeConfig,
                  scanner);

        for (const auto& ring : area.rings) {
          if (!ring.GetType()->GetIgnore() && ring.GetType()->GetIndexAsLocation()) {
            const NameFeatureValue *nameValue=nameReader.GetValue(ring.GetFeatureValueBuffer());

            if (nameValue==nullptr) {
              continue;
            }

            const PostalCodeFeatureValue *postalCodeValue=postalCodeReader.GetValue(ring.GetFeatureValueBuffer());

            AddLocationAreaToRegion(rootRegion,
                                    area,
                                    ring,
                                    nameValue->GetName(),
                                    postalCodeValue!=nullptr ? postalCodeValue->GetPostalCode() : "",
                                    regionIndex);

            areasFound++;
          }
        }
      }

      progress.Info(std::string("Found ")+std::to_string(areasFound)+" locations of type 'area'");

      scanner.Close();
    }
    catch (const IOException& e) {
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }

  bool LocationIndexGenerator::IndexLocationWays(const TypeConfig& typeConfig,
                                                 const ImportParameter& parameter,
                                                 Progress& progress,
                                                 const locidx::RegionIndex& regionIndex,
                                                 locidx::RegionRef& rootRegion)
  {
    FileScanner scanner;

    try {
      size_t                       waysFound=0;
      NameFeatureLabelReader       nameReader(typeConfig);
      RefFeatureLabelReader        refReader(typeConfig);
      PostalCodeFeatureValueReader postalCodeReader(typeConfig);

      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   WayDataFile::WAYS_DAT),
                   FileScanner::Sequential,
                   parameter.GetWayDataMemoryMaped());

      uint32_t wayCount=scanner.ReadUInt32();

      for (uint32_t w=1; w<=wayCount; w++) {
        progress.SetProgress(w,wayCount);

        Way way;

        way.Read(typeConfig,
                 scanner);

        if (!way.GetType()->GetIndexAsLocation()) {
          continue;
        }

        std::string name=nameReader.GetLabel(way.GetFeatureValueBuffer());

        if (name.empty()) {
          name=refReader.GetLabel(way.GetFeatureValueBuffer());
        }

        if (name.empty()) {
          continue;
        }

        const PostalCodeFeatureValue *postalCodeValue=postalCodeReader.GetValue(way.GetFeatureValueBuffer());
        GeoBox                       boundingBox=way.GetBoundingBox();


        // This is an optimization. Instead of propagating the alias down the region tree
        // we make a fast lookup for a matching candidate...
        locidx::RegionRef region=regionIndex.GetRegionForNode(rootRegion,
                                                              boundingBox.GetCenter());
        region->AddLocationWay(way,
                               name,
                               postalCodeValue!=nullptr ? postalCodeValue->GetPostalCode() : "",
                               boundingBox);

        waysFound++;
      }

      progress.Info(std::string("Found ")+std::to_string(waysFound)+" locations of type 'way'");

      scanner.Close();
    }
    catch (const IOException& e) {
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }

  /**
    Add the given location area to the hierarchical area index.

    The code is designed to minimize the number of "point in area" checks, it assumes that
    if one point of an object is in a area it is very likely that all points of the object
    are in the area.
    */
  void LocationIndexGenerator::AddLocationAreaToRegion(locidx::RegionRef& rootRegion,
                                                       const Area& area,
                                                       const Area::Ring& ring,
                                                       const std::string& name,
                                                       const std::string& postalCode,
                                                       const locidx::RegionIndex& regionIndex)
  {
    if (ring.IsMaster() &&
        ring.nodes.empty()) {
      for (const auto& r : area.rings) {
        if (r.IsTopOuter()) {
          GeoBox boundingBox=r.GetBoundingBox();

          locidx::RegionRef region=regionIndex.GetRegionForNode(rootRegion,
                                                                boundingBox.GetCenter());

          region->AddLocationArea(area,
                                  r.nodes,
                                  name,
                                  postalCode,
                                  boundingBox);
        }
      }
    }
    else {
      GeoBox boundingBox=ring.GetBoundingBox();

      locidx::RegionRef region=regionIndex.GetRegionForNode(rootRegion,
                                                            boundingBox.GetCenter());

      region->AddLocationArea(area,
                              ring.nodes,
                              name,
                              postalCode,
                              boundingBox);
    }
  }

  void LocationIndexGenerator::AddAddressToRegion(Progress& progress,
                                                  locidx::Region& region,
                                                  const ObjectFileRef& object,
                                                  const std::string& location,
                                                  const std::string& address,
                                                  const std::string &postalCode,
                                                  bool allowDuplicates,
                                                  bool& added)
  {
    std::map<std::string,locidx::RegionLocation>::iterator loc;
    auto                                           postalAreaEntry=region.postalAreas.end();
    bool                                           foundInDefaultArea=false;
    bool                                           locFound=false;


    // Is postal code available: Search for the postal area for the given postal code
    if (!postalCode.empty()) {
      postalAreaEntry=region.postalAreas.find(postalCode);

      // If the postal areas does not exist yet, create one
      if (postalAreaEntry==region.postalAreas.end()) {
        locidx::PostalArea postalArea(postalCode);

        postalAreaEntry=region.postalAreas.emplace(postalCode,postalArea).first;
      }
    }

    // If we have found a postal area, search there for the location first
    if (postalAreaEntry!=region.postalAreas.end()) {
      loc=FindLocation(progress,
                       region,
                       postalAreaEntry->second,
                       location);

      locFound=loc!=postalAreaEntry->second.locations.end();
    }

    if (!locFound) {
      // Now search in the default postal area
      loc=FindLocation(progress,
                       region,
                       region.defaultPostalArea->second,
                       location);
      locFound=loc!=region.defaultPostalArea->second.locations.end();
      foundInDefaultArea=true;
    }

    if (!locFound) {
      errorReporter->ReportLocationDebug(object,
                                         std::string("Street '")+location +"' of address '"+address+"' cannot be resolved in region '"+region.name+"'");
      return;
    }

    // If there is a non-default postal area but the location is in the default postal area => make a copy
    if (foundInDefaultArea &&
        postalAreaEntry!=region.postalAreas.end()) {
      std::string locationNameNorm=UTF8NormForLookup(loc->second.GetName());

      auto newLoc=postalAreaEntry->second.locations.insert(std::make_pair(locationNameNorm,
                                                                          loc->second)).first;

      newLoc->second.addresses.clear();

      loc=newLoc;
    }

    if (!allowDuplicates) {
      // Check for duplicates
      for (const auto& regionAddress : loc->second.addresses) {
        if (regionAddress.name==address) {
          return;
        }
      }
    }

    locidx::RegionAddress regionAddress(address,
                                        object);

    loc->second.addresses.push_back(regionAddress);

    added=true;
  }

  void LocationIndexGenerator::AddAddressAreaToRegion(Progress& progress,
                                                      locidx::Region& region,
                                                      const FileOffset& fileOffset,
                                                      const std::string& location,
                                                      const std::string& address,
                                                      const std::string &postalCode,
                                                      const std::vector<Point>& nodes,
                                                      const GeoBox& boundingBox,
                                                      bool& added)
  {
    for (const auto& childRegion : region.regions) {
      // Fast check, if the object is in the bounds of the area
      if (childRegion->CouldContain(boundingBox)) {
        for (const auto& childArea : childRegion->areas) {
          if (IsAreaCompletelyInArea(nodes,childArea)) {
            AddAddressAreaToRegion(progress,
                                   *childRegion,
                                   fileOffset,
                                   location,
                                   address,
                                   postalCode,
                                   nodes,
                                   boundingBox,
                                   added);
            return;
          }
        }
      }
    }

    AddAddressToRegion(progress,
                       region,
                       ObjectFileRef(fileOffset,refArea),
                       location,
                       address,
                       postalCode,
                       false,
                       added);
  }

  bool LocationIndexGenerator::IndexAddressAreas(const TypeConfig& typeConfig,
                                                 const ImportParameter& parameter,
                                                 Progress& progress,
                                                 locidx::RegionRef& rootRegion,
                                                 const locidx::RegionIndex& regionIndex)
  {
    FileScanner scanner;

    try {
      size_t             addressFound=0;
      size_t             poiFound=0;
      size_t             postalCodeFound=0;
      FileOffset         fileOffset;
      TypeId             typeId;
      TypeInfoRef        type;
      std::vector<Point> nodes;

      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   AreaAreaIndexGenerator::AREAADDRESS_DAT),
                   FileScanner::Sequential,
                   parameter.GetWayDataMemoryMaped());

      uint32_t areaCount=scanner.ReadUInt32();

      for (uint32_t a=1; a<=areaCount; a++) {
        uint32_t           tmpType;
        std::string        name;
        std::string        postalCode;
        std::string        location;
        std::string        address;

        progress.SetProgress(a,areaCount);

        fileOffset=scanner.ReadFileOffset();
        tmpType=scanner.ReadUInt32Number();

        name=scanner.ReadString();
        postalCode=scanner.ReadString();
        location=scanner.ReadString();
        address=scanner.ReadString();

        GeoBox boundingBox;
        std::vector<SegmentGeoBox> segments;
        scanner.Read(nodes,segments,boundingBox,false);

        typeId=(TypeId)tmpType;
        type=typeConfig.GetAreaTypeInfo(typeId);

        bool isAddress=!location.empty() &&
                       !address.empty();
        bool isPOI=!name.empty() &&
                   type->GetIndexAsPOI();

        if (!postalCode.empty()) {
          postalCodeFound++;
        }

        if (!isAddress && !isPOI) {
          continue;
        }

        locidx::RegionRef region=regionIndex.GetRegionForNode(rootRegion,
                                                              boundingBox.GetCenter());

        if (isAddress) {
          bool added=false;

          AddAddressAreaToRegion(progress,
                                 *region,
                                 fileOffset,
                                 location,
                                 address,
                                 postalCode,
                                 nodes,
                                 boundingBox,
                                 added);

          if (added) {
            addressFound++;
          }
        }

        if (isPOI) {
          bool added=false;

          region->AddPOIArea(fileOffset,
                             name,
                             nodes,
                             boundingBox,
                             added);

          if (added) {
            poiFound++;
          }
        }
      }

      progress.Info(std::to_string(areaCount)+" areas analyzed, "+
                    std::to_string(addressFound)+" addresses founds, "+
                    std::to_string(poiFound)+" POIs founds, "+
                    std::to_string(postalCodeFound)+" postal codes found");

      scanner.Close();
    }
    catch (const IOException& e) {
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }

  bool LocationIndexGenerator::AddAddressWayToRegion(Progress& progress,
                                                     locidx::Region& region,
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
        for (const auto& childArea : childRegion->areas) {
          bool match=IsAreaAtLeastPartlyInArea(nodes,childArea);

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

    AddAddressToRegion(progress,
                       region,
                       ObjectFileRef(fileOffset,refWay),
                       location,
                       address,
                       "",
                       false,
                       added);

    for (const auto& area : region.areas) {
      if (IsAreaCompletelyInArea(nodes,area)) {
        return true;
      }
    }

    return false;
  }

  bool LocationIndexGenerator::IndexAddressWays(const TypeConfig& typeConfig,
                                                const ImportParameter& parameter,
                                                Progress& progress,
                                                const locidx::RegionRef& rootRegion,
                                                const locidx::RegionIndex& regionIndex)
  {
    FileScanner scanner;

    try {
      size_t             poiFound=0;
      size_t             postalCodeFound=0;
      FileOffset         fileOffset;
      TypeId             typeId;
      TypeInfoRef        type;
      std::vector<Point> nodes;

      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   SortWayDataGenerator::WAYADDRESS_DAT),
                   FileScanner::Sequential,
                   parameter.GetWayDataMemoryMaped());

      uint32_t wayCount=scanner.ReadUInt32();

      for (uint32_t w=1; w<=wayCount; w++) {
        uint32_t           tmpType;
        std::string        name;
        std::string        postalCode;

        progress.SetProgress(w,wayCount);

        fileOffset=scanner.ReadFileOffset();
        tmpType=scanner.ReadUInt32Number();

        name=scanner.ReadString();
        postalCode=scanner.ReadString();

        GeoBox boundingBox;
        std::vector<SegmentGeoBox> segments;
        scanner.Read(nodes,segments,boundingBox,false);

        typeId=(TypeId)tmpType;
        type=typeConfig.GetWayTypeInfo(typeId);

        bool isPOI=!name.empty() &&
                   type->GetIndexAsPOI();

        if (!postalCode.empty()) {
          postalCodeFound++;
        }

        if (!isPOI) {
          continue;
        }

        locidx::RegionRef region=regionIndex.GetRegionForNode(rootRegion,
                                                              boundingBox.GetCenter());

        bool added=false;

        region->AddPOIWay(fileOffset,
                          name,
                          nodes,
                          boundingBox,
                          added);

        if (added) {
          poiFound++;
        }
      }

      progress.Info(std::to_string(wayCount)+" ways analyzed, "+std::to_string(poiFound)+" POIs founds");

      progress.Info(std::to_string(wayCount)+" ways analyzed, "+
                    std::to_string(poiFound)+" POIs founds, "+
                    std::to_string(postalCodeFound)+" postal codes found");

      scanner.Close();
    }
    catch (const IOException& e) {
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
  std::map<std::string,locidx::RegionLocation>::iterator LocationIndexGenerator::FindLocation(Progress& progress,
                                                                                              const locidx::Region& region,
                                                                                              locidx::PostalArea& postalArea,
                                                                                              const std::string &locationName) const
  {
    std::map<std::string,locidx::RegionLocation> &locations=postalArea.locations;
    std::string                                  locationNameSearch=UTF8NormForLookup(locationName);

    auto loc=locations.find(locationNameSearch);

    if (loc!=locations.end()) {
      // case insensitive match
      return loc;
    }

    // if locationName is same as region.name (or one of its name aliases) add new location entry
    // it is usual case for addresses without street and defined addr:place
    std::string regionNameNorm=UTF8NormForLookup(region.name);

    if (regionNameNorm==locationNameSearch) {
      postalArea.AddLocationObject(region.name,
                                   region.reference);

      progress.Debug(std::string("Create virtual location for region '")+region.name+"'");
      return locations.find(regionNameNorm);
    }

    for (const auto &alias: region.aliases) {
      std::string regionAliasNameNorm=UTF8NormForLookup(alias.name);

      if (regionAliasNameNorm==locationNameSearch) {
        postalArea.AddLocationObject(alias.name,
                                     ObjectFileRef(alias.reference,refNode));

        progress.Debug(std::string("Create virtual location for '")+alias.name+"' (alias of region "+region.name+")");
        return locations.find(regionAliasNameNorm);
      }
    }

    return locations.end();
  }

  void LocationIndexGenerator::AddAddressNodeToRegion(Progress& progress,
                                                      locidx::Region& region,
                                                      const FileOffset& fileOffset,
                                                      const std::string& location,
                                                      const std::string& address,
                                                      const std::string &postalCode,
                                                      bool& added)
  {
    AddAddressToRegion(progress,
                       region,
                       ObjectFileRef(fileOffset,refNode),
                       location,
                       address,
                       postalCode,
                       true,
                       added);
  }

  bool LocationIndexGenerator::IndexAddressNodes(const TypeConfig& typeConfig,
                                                 const ImportParameter& parameter,
                                                 Progress& progress,
                                                 const locidx::RegionRef& rootRegion,
                                                 const locidx::RegionIndex& regionIndex)
  {
    FileScanner scanner;

    try {
      size_t      addressFound=0;
      size_t      poiFound=0;
      size_t      postalCodeFound=0;
      FileOffset  fileOffset;
      TypeId      typeId;
      TypeInfoRef type;
      GeoCoord    coord;

      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   SortNodeDataGenerator::NODEADDRESS_DAT),
                   FileScanner::Sequential,
                   true);

      uint32_t nodeCount=scanner.ReadUInt32();

      for (uint32_t n=1; n<=nodeCount; n++) {
        uint32_t    tmpType;
        std::string name;
        std::string postalCode;
        std::string location;
        std::string address;

        progress.SetProgress(n,nodeCount);

        fileOffset=scanner.ReadFileOffset();
        tmpType=scanner.ReadUInt32Number();

        name=scanner.ReadString();
        postalCode=scanner.ReadString();
        location=scanner.ReadString();
        address=scanner.ReadString();

        coord=scanner.ReadCoord();

        typeId=(TypeId)tmpType;
        type=typeConfig.GetNodeTypeInfo(typeId);

        bool isAddress=!location.empty() &&
                       !address.empty();
        bool isPOI=!name.empty() &&
                   type->GetIndexAsPOI();

        if (!postalCode.empty()) {
          postalCodeFound++;
        }

        if (!isAddress && !isPOI) {
          continue;
        }

        locidx::RegionRef region=regionIndex.GetRegionForNode(rootRegion,
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

          region->AddPOINode(fileOffset,
                             name,
                             added);
          if (added) {
            poiFound++;
          }
        }
      }

      progress.Info(std::to_string(nodeCount)+" nodes analyzed, "+
                    std::to_string(addressFound)+" addresses founds, "+
                    std::to_string(poiFound)+" POIs founds, "+
                    std::to_string(postalCodeFound)+" postal codes found");

      scanner.Close();
    }
    catch (const IOException& e) {
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }

  /**
   * Remove addresses from locations in the default postal area, that are already indexed by non-default postal areas
   *
   * @param region region to cleanup
   *
   */
  void LocationIndexGenerator::CleanupPostalAreasAdresses(locidx::Region& region)
  {
    std::set<ObjectFileRef> addresses;

    //
    // Build a set of addresses in the non-default postal area
    //

    for (const auto& [id,area]: region.postalAreas) {
      if (area.name.empty()) {
        // Default postal area
        continue;
      }

      // for each location in postal area
      for (const auto& [name,location]: area.locations) {
        // add all objects for each address to set
        std::for_each(location.addresses.begin(),
                      location.addresses.end(),
                      [&addresses](const locidx::RegionAddress& address) {
                        addresses.insert(address.object);
                      });
      }
    }

    //
    // Remove addresses in default postal area, if already in set of non-default postal adrea addresses
    //

    for (auto& [name,location]: region.defaultPostalArea->second.locations) {
      // Location does not have any addresses
      auto addressIter=location.addresses.begin();
      while (addressIter!=location.addresses.end()) {
        // we have an address for this location in the default postal area which is already in another postal area
        if (addresses.find(addressIter->object)!=addresses.end()) {
          addressIter=location.addresses.erase(addressIter);
        }
        else {
          ++addressIter;
        }
      }
    }
  }

  /**
   * Remove empty locations from default postal area, that are indexed by some named postal area
   *
   * @param region region to cleanup
   */
  void LocationIndexGenerator::CleanupPostalAreasLocations(osmscout::locidx::Region& region)
  {
    std::set<ObjectFileRef> locations;

    // Build set of locations
    for (const auto& [id,area] : region.postalAreas) {
      // Skip default postal area
      if (area.name.empty()) {
        continue;
      }
      // for each location in postal area
      for (const auto& [name,location] : area.locations) {
        // add all objects for each address to set
        std::for_each(location.objects.begin(),
                      location.objects.end(),
                      [&locations](const ObjectFileRef& object) {
                        locations.insert(object);
                      });
      }
    }

    auto locationIter=region.defaultPostalArea->second.locations.begin();
    while (locationIter!=region.defaultPostalArea->second.locations.end()) {
      // Skip locations that have addresses
      if (!locationIter->second.addresses.empty()) {
        ++locationIter;
        continue;
      }

      // we have an object for this location in the default postal area that is not in another postal area
      bool foundAll=true;

      for (const auto& object : locationIter->second.objects) {
        if (locations.find(object)==locations.end()) {
          foundAll=false;
          break;
        }
      }

      if (foundAll) {
        locationIter=region.defaultPostalArea->second.locations.erase(locationIter);
      }
      else {
        ++locationIter;
      }
    }
  }

  void LocationIndexGenerator::CleanupPostalAreas(locidx::Region& region)
  {
    CleanupPostalAreasAdresses(region);
    CleanupPostalAreasLocations(region);

    // If default postal area is empty, clear it
    if (region.defaultPostalArea->second.locations.empty()) {
      region.postalAreas.erase(region.defaultPostalArea);
      region.defaultPostalArea=region.postalAreas.end();
    }

    // ... Recursion ...

    for (const auto& childRegion : region.regions) {
      CleanupPostalAreas(*childRegion);
    }
  }

  void LocationIndexGenerator::SortLocationTree(locidx::Region& region)
  {
    region.pois.sort();

    for (auto& [areaName,area] : region.postalAreas) {
      for (auto& [locationName,location] : area.locations) {
        location.objects.sort(ObjectFileRefByFileOffsetComparator());
        location.addresses.sort();
      }
    }

    for (const auto& childRegion : region.regions) {
      SortLocationTree(*childRegion);
    }
  }

  void LocationIndexGenerator::ValidateIsIn(locidx::Region& region)
  {
    for (const auto& child : region.regions) {
      if (!child->isIn.empty() &&
          region.name!=child->isIn) {
        errorReporter->ReportLocation(child->reference,"'" + child->name + "' parent should be '"+child->isIn+"' but is '"+region.name+"'");
      }
    }

    for (const auto& child : region.regions) {
      ValidateIsIn(*child);
    }
  }

  void LocationIndexGenerator::WriteIgnoreTokens(FileWriter& writer,
                                                 const std::list<std::string>& regionIgnoreTokens,
                                                 const std::list<std::string>& poiIgnoreTokens,
                                                 const std::list<std::string>& locationIgnoreTokens) const
  {
    writer.WriteNumber((uint32_t)regionIgnoreTokens.size());

    for (const auto& token : regionIgnoreTokens) {
      writer.Write(token);
    }

    writer.WriteNumber((uint32_t)poiIgnoreTokens.size());

    for (const auto& token : poiIgnoreTokens) {
      writer.Write(token);
    }

    writer.WriteNumber((uint32_t)locationIgnoreTokens.size());

    for (const auto& token : locationIgnoreTokens) {
      writer.Write(token);
    }
  }

  void LocationIndexGenerator::WriteRegionMetrics(FileWriter& writer,
                                                  const locidx::RegionMetrics& metrics) const
  {
    writer.WriteNumber(metrics.minRegionChars);
    writer.WriteNumber(metrics.maxRegionChars);
    writer.WriteNumber(metrics.minRegionWords);
    writer.WriteNumber(metrics.maxRegionWords);
    writer.WriteNumber(metrics.maxPOIWords);
    writer.WriteNumber(metrics.minLocationChars);
    writer.WriteNumber(metrics.maxLocationChars);
    writer.WriteNumber(metrics.minLocationWords);
    writer.WriteNumber(metrics.maxLocationWords);
    writer.WriteNumber(metrics.maxAddressWords);
  }

  void LocationIndexGenerator::WriteRegionIndex(FileWriter& writer,
                                                locidx::Region& region)
  {
    std::list<FileOffset> childrenOffsetOffsets;

    writer.WriteNumber((uint32_t)region.regions.size());

    for (size_t i=0; i<region.regions.size(); i++) {
      childrenOffsetOffsets.push_back(writer.GetPos());
      writer.WriteFileOffset(0);
    }

    for (const auto& childRegion : region.regions) {
      FileOffset currentOffset=writer.GetPos();

      writer.SetPos(childrenOffsetOffsets.front());
      writer.WriteFileOffset(currentOffset);
      childrenOffsetOffsets.pop_front();

      writer.SetPos(currentOffset);

      WriteRegionIndexEntry(writer,
                            region,
                            *childRegion);
    }
  }

  void LocationIndexGenerator::WriteRegionIndexEntry(FileWriter& writer,
                                                     const locidx::Region& parentRegion,
                                                     locidx::Region& region)
  {
    region.indexOffset=writer.GetPos();

    writer.WriteFileOffset(region.dataOffset);
    writer.WriteFileOffset(parentRegion.indexOffset);

    writer.Write(region.name);
    writer.Write(region.altName);

    Write(writer,
          region.reference);

    writer.WriteNumber((uint32_t)region.aliases.size());
    for (const auto& alias : region.aliases) {
      writer.Write(alias.name);
      writer.Write(alias.altName);
      writer.WriteFileOffset(alias.reference,
                             bytesForNodeFileOffset);
    }

    writer.WriteNumber((uint32_t)region.postalAreas.size());
    for (auto& [id,area] : region.postalAreas) {
      writer.Write(area.name);
      area.dataOffsetOffset=writer.GetPos();
      writer.WriteFileOffset(0);
    }

    writer.WriteNumber((uint32_t)region.regions.size());

    std::list<FileOffset> childrenOffsetOffsets;

    for (size_t i=0; i<region.regions.size(); i++) {
      childrenOffsetOffsets.push_back(writer.GetPos());
      writer.WriteFileOffset(0);
    }

    for (const auto& childRegion : region.regions) {
      FileOffset currentOffset=writer.GetPos();

      writer.SetPos(childrenOffsetOffsets.front());
      writer.WriteFileOffset(currentOffset);
      childrenOffsetOffsets.pop_front();

      writer.SetPos(currentOffset);

      WriteRegionIndexEntry(writer,
                            region,
                            *childRegion);
    }
  }

  void LocationIndexGenerator::WriteRegionData(FileWriter& writer,
                                               locidx::Region& region)
  {
    for (const auto& childRegion : region.regions) {
      WriteRegionDataEntry(writer,
                           *childRegion);
    }
  }

  void LocationIndexGenerator::WriteRegionDataEntry(FileWriter& writer,
                                                    locidx::Region& region)
  {
    region.dataOffset=writer.GetPos();

    writer.SetPos(region.indexOffset);
    writer.WriteFileOffset(region.dataOffset);
    writer.SetPos(region.dataOffset);

    writer.WriteNumber((uint32_t)region.pois.size());

    ObjectFileRefStreamWriter objectFileRefWriter(writer);

    for (const auto& poi : region.pois) {
      writer.Write(poi.name);

      objectFileRefWriter.Write(poi.object);
    }

    writer.WriteNumber((uint32_t)region.postalAreas.size());

    for (auto& [id,area] : region.postalAreas) {
      WritePostalArea(writer,
                     area);
    }

    for (const auto& childRegion : region.regions) {
      WriteRegionDataEntry(writer,
                           *childRegion);
    }
  }

  void LocationIndexGenerator::WritePostalArea(FileWriter& writer,
                                               locidx::PostalArea& postalArea) const
  {
    ObjectFileRefStreamWriter objectFileRefWriter(writer);

    FileOffset currentPos=writer.GetPos();

    writer.SetPos(postalArea.dataOffsetOffset);
    writer.WriteFileOffset(currentPos);
    writer.SetPos(currentPos);

    writer.WriteNumber((uint32_t)postalArea.locations.size());
    for (auto& [name,location] : postalArea.locations) {
      writer.Write(location.GetName());
      writer.WriteNumber((uint32_t)location.objects.size()); // Number of objects

      if (!location.addresses.empty()) {
        writer.Write(true);
        location.dataOffsetOffset=writer.GetPos();
        writer.WriteFileOffset(0);
      }
      else {
        writer.Write(false);
      }

      objectFileRefWriter.Reset();

      for (const auto& object : location.objects) {
        objectFileRefWriter.Write(object);
      }
    }
  }

  void LocationIndexGenerator::WriteAddressDataEntry(FileWriter& writer,
                                                     const locidx::Region& region)
  {
    for (const auto& [id,area] : region.postalAreas) {
      for (const auto& [name,location] : area.locations) {
        if (location.addresses.empty()) {
          continue;
        }

        FileOffset currentOffset=writer.GetPos();

        writer.SetPos(location.dataOffsetOffset);
        writer.WriteFileOffset(currentOffset);
        writer.SetPos(currentOffset);

        writer.WriteNumber((uint32_t)location.addresses.size());

        ObjectFileRefStreamWriter objectFileRefWriter(writer);

        for (const auto& address : location.addresses) {
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
                                                const locidx::Region& region)
  {
    for (const auto& childRegion : region.regions) {
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
    FileWriter                                 writer;
    locidx::RegionRef                          rootRegion;
    TypeInfoRef                                boundaryType;
    std::vector<std::list<locidx::RegionRef>>  boundaryAreas;

    errorReporter=parameter.GetErrorReporter();

    // just from local experience, if it is not enough, GetBoundaryAreas will increase it ;-)
    boundaryAreas.resize(13);

    try {
      std::vector<std::list<locidx::RegionRef>> regionTree;
      std::list<locidx::RegionRef>              regionAreas;
      std::list<std::string>                    regionIgnoreTokens;
      std::list<std::string>                    poiIgnoreTokens;
      std::list<std::string>                    locationIgnoreTokens;
      TypeInfoSet                               boundaryTypes(*typeConfig);

      bytesForNodeFileOffset=BytesNeededToAddressFileData(AppendFileToDir(parameter.GetDestinationDirectory(),
                                                        "nodes.dat"));
      bytesForAreaFileOffset=BytesNeededToAddressFileData(AppendFileToDir(parameter.GetDestinationDirectory(),
                                                        "areas.dat"));
      bytesForWayFileOffset=BytesNeededToAddressFileData(AppendFileToDir(parameter.GetDestinationDirectory(),
                                                        "ways.dat"));

      rootRegion=std::make_shared<locidx::Region>();
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

      size_t level=0;
      for (const auto& regionList : boundaryAreas) {
        progress.SetAction("Sorting in {} administrative boundaries of level {}",
          regionList.size(),
          level);

        SortInBoundaries(progress,
                         *rootRegion,
                         regionList);

        level++;
      }

      boundaryAreas.clear();

      //
      // Getting all areas of type place=*.
      //

      progress.SetAction("Scanning for place regions of type 'area'");

      if (!GetRegionAreas(*typeConfig,
                          parameter,
                          progress,
                          regionAreas)) {
        return false;
      }

      progress.SetAction("Sorting in place regions of type 'area'");

      if (!SortInRegionAreas(progress,
                             *rootRegion,
                             regionAreas)) {
        return false;
      }

      regionAreas.clear();

      progress.SetAction("Calculating region tree depth");

      regionTree.resize(GetRegionTreeDepth(*rootRegion));

      progress.Info(std::string("Area tree depth: ")+std::to_string(regionTree.size()));

      progress.SetAction("Sorting regions by levels");

      SortInRegion(rootRegion,
                   regionTree,
                   0);

      for (size_t i=0; i<regionTree.size(); i++) {
        progress.Info(std::string("Area tree index ")+std::to_string(i)+" size: "+std::to_string(regionTree[i].size()));
      }

      progress.SetAction("Index regions");

      locidx::RegionIndex regionIndex(360.0/pow(2.0,REGION_INDEX_LEVEL),
                                      180.0/pow(2.0,REGION_INDEX_LEVEL));

      regionIndex.IndexRegions(regionTree);

      //
      // Getting all nodes of type place=*. We later need an area for these cities.
      //

      progress.SetAction("Indexing regions of type 'Node' as area aliases");

      if (!IndexRegionNodes(typeConfig,
                            parameter,
                            progress,
                            regionIndex,
                            rootRegion)) {
        return false;
      }

      progress.SetAction("Index location areas");

      if (!IndexLocationAreas(*typeConfig,
                              parameter,
                              progress,
                              regionIndex,
                              rootRegion)) {
        return false;
      }

      progress.SetAction("Index location ways");

      if (!IndexLocationWays(*typeConfig,
                             parameter,
                             progress,
                             regionIndex,
                             rootRegion)) {
        return false;
      }

      size_t index=0;
      for (const auto& regionList : regionTree) {
        size_t count=0;

        for (const auto& region : regionList) {
          for (const auto& [id,area] : region->postalAreas) {
            count+=area.locations.size();
          }
        }

        progress.Info(std::string("Area tree index ")+std::to_string(index)+" object count size: "+std::to_string(count));
        index++;
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

      progress.SetAction("Cleanup location tree");

      CleanupPostalAreas(*rootRegion);

      SortLocationTree(*rootRegion);

      progress.SetAction("Calculate ignore tokens");

      CalculateIgnoreTokens(*rootRegion,
                            regionIgnoreTokens,
                            poiIgnoreTokens,
                            locationIgnoreTokens);

      progress.Info("Detected "+std::to_string(regionIgnoreTokens.size())+" token(s) to ignore");

      progress.SetAction("Calculation region metrics");

      locidx::RegionMetrics metrics;

      CalculateRegionMetrics(*rootRegion,
                             metrics);

      progress.Info("Region words: "+std::to_string(metrics.minRegionWords)+" - "+std::to_string(metrics.maxRegionWords));
      progress.Info("Max POI words: "+std::to_string(metrics.maxPOIWords));
      progress.Info("Location words: "+std::to_string(metrics.minLocationWords)+" - "+std::to_string(metrics.maxLocationWords));
      progress.Info("Max address words: "+std::to_string(metrics.maxAddressWords));

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


      DumpLocationMetrics(progress,
                          AppendFileToDir(parameter.GetDestinationDirectory(),
                                          FILENAME_LOCATION_METRICS_TXT),
                          metrics,
                          regionIgnoreTokens,
                          poiIgnoreTokens,
                          locationIgnoreTokens);

      progress.SetAction("Validations");

      ValidateIsIn(*rootRegion);

      //
      // Generate file with all areas, where areas reference parent and children by offset
      //

      progress.SetAction("Write '{}'",LocationIndex::FILENAME_LOCATION_IDX);

      writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                  LocationIndex::FILENAME_LOCATION_IDX));

      writer.Write(bytesForNodeFileOffset);
      writer.Write(bytesForAreaFileOffset);
      writer.Write(bytesForWayFileOffset);

      WriteIgnoreTokens(writer,
                        regionIgnoreTokens,
                        poiIgnoreTokens,
                        locationIgnoreTokens);

      WriteRegionMetrics(writer,
                         metrics);

      WriteRegionIndex(writer,
                       *rootRegion);

      WriteRegionData(writer,
                      *rootRegion);

      WriteAddressData(writer,
                       *rootRegion);

      writer.Close();
    }
    catch (const IOException& e) {
      progress.Error(e.GetDescription());

      writer.CloseFailsafe();

      return false;
    }

    return true;
  }
}
