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

#include <osmscout/LocationDescriptionService.h>

#include <algorithm>

#include <osmscout/util/Geometry.h>
#include <osmscout/util/Logger.h>
#include <osmscout/util/String.h>
#include <osmscout/TypeFeatures.h>
#include <iostream>
namespace osmscout {

  LocationCoordDescription::LocationCoordDescription(const GeoCoord& location)
    : location(location)
  {
    // no code
  }

  GeoCoord LocationCoordDescription::GetLocation() const
  {
    return location;
  }

  LocationAtPlaceDescription::LocationAtPlaceDescription(const Place& place)
  : place(place),
    atPlace(true),
    distance(0.0),
    bearing(0.0)
  {
    // no code
  }

  LocationAtPlaceDescription::LocationAtPlaceDescription(const Place& place,
                                                         double distance,
                                                         double bearing)
  : place(place),
    atPlace(false),
    distance(distance),
    bearing(bearing)
  {
    // no code
  }

  LocationWayDescription::LocationWayDescription(const Place& way)
  : way(way),
    distance(0.0)
  {
    // no code
  }

  LocationWayDescription::LocationWayDescription(const Place& way,
                                                 double distance)
    : way(way),
      distance(distance)
  {
    // no code
  }

  LocationCrossingDescription::LocationCrossingDescription(const GeoCoord& crossing,
                                                           const std::list<Place>& ways)
  : crossing(crossing),
    atPlace(true),
    ways(ways),
    distance(0.0),
    bearing(0.0)
  {
    // no code
  }

  LocationCrossingDescription::LocationCrossingDescription(const GeoCoord& crossing,
                                                           const std::list<Place>& ways,
                                                           double distance,
                                                           double bearing)
    : crossing(crossing),
      atPlace(false),
      ways(ways),
      distance(distance),
      bearing(bearing)
  {
    // no code
  }

  void LocationDescription::SetCoordDescription(const LocationCoordDescriptionRef& description)
  {
    this->coordDescription=description;
  }

  void LocationDescription::SetAtNameDescription(const LocationAtPlaceDescriptionRef& description)
  {
    this->atNameDescription=description;
  }

  void LocationDescription::SetAtAddressDescription(const LocationAtPlaceDescriptionRef& description)
  {
    this->atAddressDescription=description;
  }

  void LocationDescription::SetAtPOIDescription(const LocationAtPlaceDescriptionRef& description)
  {
    this->atPOIDescription=description;
  }

  void LocationDescription::SetWayDescription(const LocationWayDescriptionRef& description)
  {
    this->wayDescription=description;
  }

  void LocationDescription::SetCrossingDescription(const LocationCrossingDescriptionRef& description)
  {
    this->crossingDescription=description;
  }

  LocationCoordDescriptionRef LocationDescription::GetCoordDescription() const
  {
    return coordDescription;
  }

  LocationAtPlaceDescriptionRef LocationDescription::GetAtNameDescription() const
  {
    return atNameDescription;
  }

  LocationAtPlaceDescriptionRef LocationDescription::GetAtAddressDescription() const
  {
    return atAddressDescription;
  }

  LocationAtPlaceDescriptionRef LocationDescription::GetAtPOIDescription() const
  {
    return atPOIDescription;
  }

  LocationWayDescriptionRef LocationDescription::GetWayDescription() const
  {
    return wayDescription;
  }

  LocationCrossingDescriptionRef LocationDescription::GetCrossingDescription() const
  {
    return crossingDescription;
  }

  /**
   * LocationService constructor
   *
   * @param database
   *    Valid reference to a database instance
   */
  LocationDescriptionService::LocationDescriptionService(const DatabaseRef& database)
  : database(database)
  {
    assert(database);
  }

  const FeatureValueBufferRef LocationDescriptionService::GetObjectFeatureBuffer(const ObjectFileRef &object)
  {
    FeatureValueBufferRef objectFeatureBuff;
    NodeRef               node;
    WayRef                way;
    AreaRef               area;

    switch (object.GetType()){
      case refNode:
        if (database->GetNodeByOffset(object.GetFileOffset(), node)) {
          objectFeatureBuff = std::make_shared<FeatureValueBuffer>();
          objectFeatureBuff->Set(node->GetFeatureValueBuffer());
        }
        break;
      case refWay:
        if (database->GetWayByOffset(object.GetFileOffset(), way)) {
          objectFeatureBuff = std::make_shared<FeatureValueBuffer>();
          objectFeatureBuff->Set(way->GetFeatureValueBuffer());
        }
        break;
      case refArea:
        if (database->GetAreaByOffset(object.GetFileOffset(), area)) {
          objectFeatureBuff = std::make_shared<FeatureValueBuffer>();
          objectFeatureBuff->Set(area->GetFeatureValueBuffer());
        }
        break;
      case refNone:
      default:
        /* do nothing */
        break;
    }

    return objectFeatureBuff;
  }

  Place LocationDescriptionService::GetPlace(const std::list<ReverseLookupResult>& lookupResult)
  {
    ObjectFileRef  object=lookupResult.front().object;
    AdminRegionRef adminRegion;
    PostalAreaRef  postalArea;
    POIRef         poi;
    LocationRef    location;
    AddressRef     address;

    for (const auto& entry : lookupResult) {
      if (entry.adminRegion && !adminRegion) {
        adminRegion=entry.adminRegion;
      }

      if (entry.postalArea && !postalArea) {
        postalArea=entry.postalArea;
      }

      if (entry.poi && !poi) {
        poi=entry.poi;
      }

      if (entry.location && !location) {
        location=entry.location;
      }

      if (entry.address && !address) {
        address=entry.address;
      }
    }

    return Place(object,
                 GetObjectFeatureBuffer(object),
                 adminRegion,
                 postalArea,
                 poi,
                 location,
                 address);
  }

  /**
   * Call the given visitor for each region in the index (deep first)
   *
   * @param visitor
   *    The visitor
   * @return
   *    True if the traversal finished and the visitor did not signal an error, else false
   */
  bool LocationDescriptionService::VisitAdminRegions(AdminRegionVisitor& visitor) const
  {
    LocationIndexRef locationIndex=database->GetLocationIndex();

    if (!locationIndex) {
      return false;
    }

    return locationIndex->VisitAdminRegions(visitor);
  }

  class AdminRegionReverseLookupVisitor : public AdminRegionVisitor
  {
  public:
    struct SearchEntry
    {
      ObjectFileRef         object;
      std::vector<GeoCoord> coords;
      GeoBox                bbox;
    };

  private:
    const Database&                                  database;
    std::list<LocationDescriptionService::ReverseLookupResult>& results;

    std::list<SearchEntry>                           searchEntries;

  public:
    std::map<FileOffset,AdminRegionRef>              adminRegions;

  public:
    AdminRegionReverseLookupVisitor(const Database& database,
                                    std::list<LocationDescriptionService::ReverseLookupResult>& results);

    void AddSearchEntry(const SearchEntry& searchEntry);

    Action Visit(const AdminRegion& region) override;
  };

  AdminRegionReverseLookupVisitor::AdminRegionReverseLookupVisitor(const Database& database,
                                                                   std::list<LocationDescriptionService::ReverseLookupResult>& results)
  : database(database),
    results(results)
  {
    // no code
  }

  void AdminRegionReverseLookupVisitor::AddSearchEntry(const SearchEntry& searchEntry)
  {
    searchEntries.push_back(searchEntry);
  }

  AdminRegionVisitor::Action AdminRegionReverseLookupVisitor::Visit(const AdminRegion& region)
  {
    AreaRef area;
    bool    atLeastOneCandidate=false;

    if (!database.GetAreaByOffset(region.object.GetFileOffset(),
                                  area)) {
      return error;
    }

    // Test for direct match
    for (const auto& searchEntry : searchEntries) {
      if (region.Match(searchEntry.object)) {
        LocationDescriptionService::ReverseLookupResult result;

        result.object=searchEntry.object;
        result.adminRegion=std::make_shared<AdminRegion>(region);

        results.push_back(result);
      }
    }

    // Test for inclusion
    bool candidate=false;
    for (const auto& ring : area->rings) {
      if (!ring.IsOuterRing()) {
        continue;
      }

      for (const auto& searchEntry : searchEntries) {
        if (searchEntry.coords.size()==1) {
          if (!IsCoordInArea(searchEntry.coords.front(),
                             ring.nodes)) {
            continue;
          }
        }
        else {
          GeoBox ringBBox;
          ring.GetBoundingBox(ringBBox);
          if (!IsAreaAtLeastPartlyInArea(searchEntry.coords,
                                         ring.nodes,
                                         searchEntry.bbox,
                                         ringBBox)) {
            continue;
          }
        }

        // candidate
        candidate=true;
        break;
      }

      if (candidate) {
        break;
      }
    }

    if (candidate) {
      atLeastOneCandidate=true;
      adminRegions.insert(std::make_pair(region.regionOffset,
                                         std::make_shared<AdminRegion>(region)));
    }

    if (atLeastOneCandidate) {
      return visitChildren;
    }


    return skipChildren;
  }

  class POIReverseLookupVisitor : public POIVisitor
  {
  public:
    struct Result
    {
      AdminRegionRef adminRegion;
      LocationRef    location;
    };

  private:
    std::set<ObjectFileRef>                   objects;
    std::list<LocationDescriptionService::ReverseLookupResult>& results;

  public:
    std::list<Result>                         result;

  public:
    explicit POIReverseLookupVisitor(std::list<LocationDescriptionService::ReverseLookupResult>& results);

    void AddObject(const ObjectFileRef& object);

    bool Visit(const AdminRegion& adminRegion,
               const POI &poi) override;
  };

  POIReverseLookupVisitor::POIReverseLookupVisitor(std::list<LocationDescriptionService::ReverseLookupResult>& results)
    : results(results)
  {
    // no code
  }

  void POIReverseLookupVisitor::AddObject(const ObjectFileRef& object)
  {
    objects.insert(object);
  }

  bool POIReverseLookupVisitor::Visit(const AdminRegion& adminRegion,
                                      const POI &poi)
  {
    if (objects.find(poi.object)!=objects.end()) {
      LocationDescriptionService::ReverseLookupResult result;

      result.object=poi.object;
      result.adminRegion=std::make_shared<AdminRegion>(adminRegion);
      result.poi=std::make_shared<POI>(poi);

      results.push_back(result);
    }

    return true;
  }

  class LocationReverseLookupVisitor : public LocationVisitor
  {
  public:
    struct Result
    {
      AdminRegionRef adminRegion;
      PostalAreaRef  postalArea;
      LocationRef    location;
    };

  private:
    std::set<ObjectFileRef>                   objects;
    std::list<LocationDescriptionService::ReverseLookupResult>& results;

  public:
    std::list<Result>                          result;

  public:
    explicit LocationReverseLookupVisitor(std::list<LocationDescriptionService::ReverseLookupResult>& results);

    void AddObject(const ObjectFileRef& object);

    bool Visit(const AdminRegion& adminRegion,
               const PostalArea& postalArea,
               const Location &location) override;
  };

  LocationReverseLookupVisitor::LocationReverseLookupVisitor(std::list<LocationDescriptionService::ReverseLookupResult>& results)
  : results(results)
  {
    // no code
  }

  void LocationReverseLookupVisitor::AddObject(const ObjectFileRef& object)
  {
    objects.insert(object);
  }

  bool LocationReverseLookupVisitor::Visit(const AdminRegion& adminRegion,
                                           const PostalArea& postalArea,
                                           const Location &location)
  {
    Result l;

    l.adminRegion=std::make_shared<AdminRegion>(adminRegion);
    l.postalArea=std::make_shared<PostalArea>(postalArea);
    l.location=std::make_shared<Location>(location);

    result.push_back(l);

    for (const auto& object : location.objects) {
      if (objects.find(object)!=objects.end()) {
        LocationDescriptionService::ReverseLookupResult result;

        result.object=object;
        result.adminRegion=l.adminRegion;
        result.postalArea=l.postalArea;
        result.location=l.location;

        results.push_back(result);
      }
    }

    return true;
  }

  class AddressReverseLookupVisitor : public AddressVisitor
  {
  private:
    std::list<LocationDescriptionService::ReverseLookupResult>& results;

    std::set<ObjectFileRef>                          objects;

  public:
    explicit AddressReverseLookupVisitor(std::list<LocationDescriptionService::ReverseLookupResult>& results);
    void AddObject(const ObjectFileRef& object);

    bool Visit(const AdminRegion& adminRegion,
               const PostalArea& postalArea,
               const Location &location,
               const Address& address) override;
  };

  AddressReverseLookupVisitor::AddressReverseLookupVisitor(std::list<LocationDescriptionService::ReverseLookupResult>& results)
  : results(results)
  {
    // no code
  }

  void AddressReverseLookupVisitor::AddObject(const ObjectFileRef& object)
  {
    objects.insert(object);
  }

  bool AddressReverseLookupVisitor::Visit(const AdminRegion& adminRegion,
                                          const PostalArea& postalArea,
                                          const Location &location,
                                          const Address& address)
  {
    if (objects.find(address.object)!=objects.end()) {
      LocationDescriptionService::ReverseLookupResult result;

      result.object=address.object;
      result.adminRegion=std::make_shared<AdminRegion>(adminRegion);
      result.postalArea=std::make_shared<PostalArea>(postalArea);
      result.location=std::make_shared<Location>(location);
      result.address=std::make_shared<Address>(address);

      results.push_back(result);
    }

    return true;
  }

  bool LocationDescriptionService::ReverseLookupRegion(const GeoCoord &coord,
                                                       std::list<ReverseLookupResult>& result) const
  {
    result.clear();
    AdminRegionReverseLookupVisitor adminRegionVisitor(*database,
                                                       result);
    AdminRegionReverseLookupVisitor::SearchEntry searchEntry;

    searchEntry.coords.push_back(coord);
    adminRegionVisitor.AddSearchEntry(searchEntry);

    if (!VisitAdminRegions(adminRegionVisitor)) {
      return false;
    }

    for (const auto &region:adminRegionVisitor.adminRegions){
      ReverseLookupResult regionResult;
      regionResult.adminRegion=region.second;
      result.push_back(regionResult);
    }

    return true;
  }

  /**
   * Lookups location descriptions for the given objects.
   * @param objects
   *    List of objects
   * @param result
   *    List of results. The list may hold none, one or more entries for each
   *    object.
   * @return
   *    True, if there was no error
   */
  bool LocationDescriptionService::ReverseLookupObjects(const std::list<ObjectFileRef>& objects,
                                                        std::list<ReverseLookupResult>& result) const
  {
    result.clear();

    LocationIndexRef locationIndex=database->GetLocationIndex();

    if (!locationIndex) {
      return false;
    }

    AdminRegionReverseLookupVisitor adminRegionVisitor(*database,
                                                       result);

    for (const auto& object : objects) {
      if (object.GetType()==refNode) {
        NodeRef node;

        if (!database->GetNodeByOffset(object.GetFileOffset(),
                                       node)) {
          return false;
        }

        AdminRegionReverseLookupVisitor::SearchEntry searchEntry;

        searchEntry.object=object;
        searchEntry.coords.push_back(node->GetCoords());

        adminRegionVisitor.AddSearchEntry(searchEntry);
      }
      else if (object.GetType()==refArea) {
        AreaRef area;

        if (!database->GetAreaByOffset(object.GetFileOffset(),
                                       area)) {
          return false;
        }

        for (auto& ring : area->rings) {
          if (ring.IsOuterRing()) {
            AdminRegionReverseLookupVisitor::SearchEntry searchEntry;

            searchEntry.object=object;
            ring.GetBoundingBox(searchEntry.bbox);

            searchEntry.coords.resize(ring.nodes.size());

            for (size_t i=0; i<ring.nodes.size(); i++) {
              searchEntry.coords[i]=ring.nodes[i].GetCoord();
            }

            adminRegionVisitor.AddSearchEntry(searchEntry);
          }
        }
      }
      else if (object.GetType()==refWay) {
        WayRef way;

        if (!database->GetWayByOffset(object.GetFileOffset(),
                                      way)) {
          return false;
        }

        AdminRegionReverseLookupVisitor::SearchEntry searchEntry;

        searchEntry.object=object;
        way->GetBoundingBox(searchEntry.bbox);

        searchEntry.coords.resize(way->nodes.size());

        for (size_t i=0; i<way->nodes.size(); i++) {
          searchEntry.coords[i]=way->nodes[i].GetCoord();
        }

        adminRegionVisitor.AddSearchEntry(searchEntry);
      }
      else {
        return false;
      }
    }

    if (!VisitAdminRegions(adminRegionVisitor)) {
      return false;
    }

    if (adminRegionVisitor.adminRegions.empty()) {
      return true;
    }

    POIReverseLookupVisitor poiVisitor(result);

    for (const auto& object : objects) {
      poiVisitor.AddObject(object);
    }

    for (const auto& regionEntry : adminRegionVisitor.adminRegions) {
      if (!locationIndex->VisitPOIs(*regionEntry.second,
                                    poiVisitor,
                                    false)) {
        return false;
      }
    }

    LocationReverseLookupVisitor locationVisitor(result);

    for (const auto& object : objects) {
      locationVisitor.AddObject(object);
    }

    for (const auto& regionEntry : adminRegionVisitor.adminRegions) {
      for (const auto& postalArea : regionEntry.second->postalAreas) {
        if (!locationIndex->VisitLocations(*regionEntry.second,
                                           postalArea,
                                           locationVisitor,
                                           false)) {
          return false;
        }
      }
    }

    AddressReverseLookupVisitor addressVisitor(result);

    for (const auto& object : objects) {
      addressVisitor.AddObject(object);
    }

    for (const auto& location : locationVisitor.result) {
      if (!locationIndex->VisitAddresses(*location.adminRegion,
                                         *location.postalArea,
                                         *location.location,
                                         addressVisitor)) {
        return false;
      }
    }

    return true;
  }

  /**
   * Lookup one object
   * @param object
   *    The object to lookup
   * @param result
   *    List of results. The list may hold none, one or more entries for the object
   * @return
   *    True, if there was no error
   */
  bool LocationDescriptionService::ReverseLookupObject(const ObjectFileRef& object,
                                                       std::list<ReverseLookupResult>& result) const
  {
    std::list<ObjectFileRef> objects;

    objects.push_back(object);

    return ReverseLookupObjects(objects,
                                result);
  }

  bool LocationDescriptionService::LoadNearNodes(const GeoCoord& location, const TypeInfoSet &types,
                                                 std::vector<LocationDescriptionCandicate> &candidates,
                                                 const double maxDistance)
  {
    TypeConfigRef           typeConfig=database->GetTypeConfig();
    AreaNodeIndexRef        areaNodeIndex=database->GetAreaNodeIndex();
    GeoBox                  box=GeoBox::BoxByCenterAndRadius(location,maxDistance);
    NameFeatureLabelReader  nameFeatureLabelLeader(*typeConfig);

    if (!typeConfig ||
        !areaNodeIndex) {
      return false;
    }
    std::vector<FileOffset>  offsets;
    TypeInfoSet              loadedAddressTypes;

    if (!areaNodeIndex->GetOffsets(box, types, offsets, loadedAddressTypes)) {
      return false;
    }

    if (offsets.empty()) {
      return true;
    }

    std::vector<NodeRef> nodes;

    if (!database->GetNodesByOffset(offsets, nodes)) {
      return false;
    }

    if (nodes.empty()) {
      return true;
    }

    for (const auto &node: nodes) {
      double  distance=GetEllipsoidalDistance(location,node.get()->GetCoords()); // In Km
      if (distance*1000 <= maxDistance) {
        double  bearing=GetSphericalBearingInitial(node.get()->GetCoords(),location);

        candidates.emplace_back(ObjectFileRef(node->GetFileOffset(),refNode),
                                nameFeatureLabelLeader.GetLabel(node->GetFeatureValueBuffer()),
                                distance,
                                bearing,
                                false,
                                0.0);
      }
    }

    osmscout::log.Debug() << "Found " << candidates.size() << " nodes near " << location.GetDisplayText();

    return true;
  }

  bool LocationDescriptionService::LoadNearWays(const GeoCoord& location,
                                                const TypeInfoSet &types,
                                                std::vector<WayRef> &candidates,
                                                const double maxDistance)
  {
    TypeConfigRef          typeConfig=database->GetTypeConfig();
    AreaWayIndexRef        areaWayIndex=database->GetAreaWayIndex();
    GeoBox                 box=GeoBox::BoxByCenterAndRadius(location,maxDistance);

    if (!typeConfig ||
        !areaWayIndex) {
      return false;
    }
    std::vector<FileOffset> wayFileOffsets;
    TypeInfoSet             loadedTypes;

    if (!areaWayIndex->GetOffsets(box,
                                  types,
                                  wayFileOffsets,
                                  loadedTypes)) {
      return false;
    }

    std::vector<WayRef> ways;

    if (wayFileOffsets.empty()) {
      return true;
    }

    if (!database->GetWaysByOffset(wayFileOffsets,
                                   ways)) {
      return false;
    }

    for (const auto& way : ways) {
      double  distance=std::numeric_limits<double>::max(); // In Km

      for (size_t i=0; i<way->nodes.size(); i++) {
        double   currentDistance;
        GeoCoord a;
        GeoCoord b;
        GeoCoord intersection;

        if (i>0) {
          a=way->nodes[i-1].GetCoord();
          b=way->nodes[i].GetCoord();
        }
        else {
          a=way->nodes[way->nodes.size()-1].GetCoord();
          b=way->nodes[i].GetCoord();
        }

        currentDistance=CalculateDistancePointToLineSegment(location,
                                                            a,
                                                            b,
                                                            intersection);

        currentDistance=GetEllipsoidalDistance(location,intersection);

        if (currentDistance<distance) {
          distance=currentDistance;
        }
      }

      if (distance*1000 <= maxDistance) {
        candidates.push_back(way);
      }
    }

    osmscout::log.Debug() << "Found " << candidates.size() << " ways near " << location.GetDisplayText();

    return true;
  }

  bool LocationDescriptionService::LoadNearAreas(const GeoCoord& location,
                                                 const TypeInfoSet &types,
                                                 std::vector<LocationDescriptionCandicate> &candidates,
                                                 const double maxDistance)
  {
    TypeConfigRef           typeConfig=database->GetTypeConfig();
    AreaAreaIndexRef        areaAreaIndex=database->GetAreaAreaIndex();
    GeoBox                  box=GeoBox::BoxByCenterAndRadius(location,maxDistance);
    NameFeatureLabelReader  nameFeatureLabelLeader(*typeConfig);

    if (!typeConfig ||
        !areaAreaIndex) {
      return false;
    }
    std::vector<DataBlockSpan> areaSpans;
    TypeInfoSet                loadedTypes;

    if (!areaAreaIndex->GetAreasInArea(*typeConfig,
                                       box,
                                       std::numeric_limits<size_t>::max(),
                                       types,
                                       areaSpans,
                                       loadedTypes)) {
      return false;
    }

    std::vector<AreaRef> areas;

    if (areaSpans.empty()) {
      return true;
    }

    if (!database->GetAreasByBlockSpans(areaSpans,
                                        areas)) {
      return false;
    }

    for (const auto& area : areas) {
      bool    atPlace=false;
      double  distance=std::numeric_limits<double>::max(); // In Km
      double  bearing=0;
      GeoBox  boundingBox;

      area->GetBoundingBox(boundingBox);

      for (const auto& ring : area->rings) {
        if (ring.IsOuterRing()) {
          if (!atPlace && IsCoordInArea(location,
                                        ring.nodes)) {
            atPlace=true;
            //placeArea=area;
            distance=0.0;
            bearing=0;
          }

          for (size_t i=0; i<ring.nodes.size(); i++) {
            double   currentDistance;
            GeoCoord a;
            GeoCoord b;
            GeoCoord intersection;

            if (i>0) {
              a=ring.nodes[i-1].GetCoord();
              b=ring.nodes[i].GetCoord();
            }
            else {
              a=ring.nodes[ring.nodes.size()-1].GetCoord();
              b=ring.nodes[i].GetCoord();
            }

            currentDistance=CalculateDistancePointToLineSegment(location,
                                                                a,
                                                                b,
                                                                intersection);

            currentDistance=GetEllipsoidalDistance(location,intersection);

            if (!atPlace &&
                currentDistance<distance) {
              distance=currentDistance;
              bearing=GetSphericalBearingInitial(intersection,location);
            }
          }
        }
      }

      if (distance*1000 <= maxDistance){
        candidates.push_back(LocationDescriptionCandicate(ObjectFileRef(area->GetFileOffset(),refArea),
                                                          nameFeatureLabelLeader.GetLabel(area->GetFeatureValueBuffer()),
                                                          distance,
                                                          bearing,
                                                          atPlace,
                                                          boundingBox.GetSize()));
      }
    }

    osmscout::log.Debug() << "Found " << candidates.size() << " areas near " << location.GetDisplayText();

    return true;
  }

  bool LocationDescriptionService::DistanceComparator(const LocationDescriptionCandicate &a,
                                                      const LocationDescriptionCandicate &b)
  {
    if (a.IsAtPlace() && b.IsAtPlace()) {
      return a.GetSize()<b.GetSize();
    }

    return a.GetDistance() < b.GetDistance();
  }

  bool LocationDescriptionService::DescribeLocationByName(const GeoCoord& location,
                                                          LocationDescription& description,
                                                          const double lookupDistance,
                                                          const double sizeFilter)
  {

    // search all nameable areas and nodes, sort it by distance, get first with name
    TypeConfigRef typeConfig=database->GetTypeConfig();

    if (!typeConfig) {
      return false;
    }

    std::vector<LocationDescriptionCandicate> candidates;

    TypeInfoSet nameTypes;

    // near nameable areas, but no regions
    for (const auto& type : typeConfig->GetTypes()) {
      if (type->CanBeArea() &&
          type->HasFeature(NameFeature::NAME) &&
          !type->GetIndexAsRegion() &&
          !type->HasFeature(AdminLevelFeature::NAME)) {
        nameTypes.Set(type);
      }
    }

    if (!nameTypes.Empty()) {
      if (!LoadNearAreas(location,
                         nameTypes,
                         candidates,
                         lookupDistance)) {
        return false;
      }
    }

    // near nameable nodes, but no regions
    nameTypes.Clear();
    for (const auto& type : typeConfig->GetTypes()) {
      if (type->CanBeNode() &&
          type->HasFeature(NameFeature::NAME) &&
          !type->GetIndexAsRegion() &&
          !type->HasFeature(AdminLevelFeature::NAME)) {
        nameTypes.Set(type);
      }
    }

    if (!nameTypes.Empty()) {
      if (!LoadNearNodes(location,
                         nameTypes,
                         candidates,
                         lookupDistance)) {
        return false;
      }
    }

    if (candidates.empty()) {
      return true;
    }

    // sort all candidates by its distance from location
    std::sort(candidates.begin(),candidates.end(),DistanceComparator);

    for (const auto &candidate : candidates) {
      std::list<ReverseLookupResult> result;

      if (candidate.GetName().empty()) {
        continue;
      }

      if (candidate.GetSize() > sizeFilter) {
        continue;
      }

      if (!ReverseLookupObject(candidate.GetRef(),
                               result)) {
        return false;
      }

      if (!result.empty()) {
        Place place=GetPlace(result);

        if (candidate.IsAtPlace()) {
          description.SetAtNameDescription(std::make_shared<LocationAtPlaceDescription>(place));
        }
        else {
          description.SetAtNameDescription(std::make_shared<LocationAtPlaceDescription>(place,
                                                                                        candidate.GetDistance()*1000,
                                                                                        candidate.GetBearing()));
        }

        return true;
      }
      else if (!candidate.GetName().empty()) {
        AdminRegionRef adminRegion;
        PostalAreaRef  postalArea;
        POIRef         poi=std::make_shared<POI>();
        LocationRef    location;
        AddressRef     address;

        poi->object=candidate.GetRef();
        poi->name=candidate.GetName();
        poi->regionOffset=0;

        Place place(candidate.GetRef(),
                    GetObjectFeatureBuffer(candidate.GetRef()),
                    adminRegion,
                    postalArea,
                    poi,
                    location,
                    address);

        if (candidate.IsAtPlace()) {
          description.SetAtNameDescription(std::make_shared<LocationAtPlaceDescription>(place));
        }
        else {
          description.SetAtNameDescription(std::make_shared<LocationAtPlaceDescription>(place,
                                                                                        candidate.GetDistance()*1000,
                                                                                        candidate.GetBearing()));
        }

        return true;
      }
    }

    return true;
  }

  bool LocationDescriptionService::DescribeLocationByAddress(const GeoCoord& location,
                                                             LocationDescription& description,
                                                             const double lookupDistance,
                                                             const double sizeFilter)
  {
    // search all addressable areas and nodes, sort it by distance, get first with address
    TypeConfigRef typeConfig=database->GetTypeConfig();

    if (!typeConfig) {
      return false;
    }

    std::vector<LocationDescriptionCandicate> candidates;

    TypeInfoSet addressTypes;

    // near addressable areas
    for (const auto& type : typeConfig->GetTypes()) {
      if (type->CanBeArea() &&
          type->GetIndexAsAddress()) {
        addressTypes.Set(type);
      }
    }

    if (!addressTypes.Empty()) {
      if (!LoadNearAreas(location,
                         addressTypes,
                         candidates,
                         lookupDistance)){
        return false;
      }
    }

    // near addressable nodes
    addressTypes.Clear();
    for (const auto& type : typeConfig->GetTypes()) {
      if (type->CanBeNode() &&
          type->GetIndexAsAddress()) {
        addressTypes.Set(type);
      }
    }

    if (!addressTypes.Empty()) {
      if (!LoadNearNodes(location,
                         addressTypes,
                         candidates,
                         lookupDistance)) {
        return false;
      }
    }

    if (candidates.empty()) {
      return true;
    }

    // sort all candidates by its distance from location
    std::sort(candidates.begin(),candidates.end(),DistanceComparator);

    for (const auto &candidate : candidates) {
      if (candidate.GetSize()>sizeFilter) {
        continue;
      }

      std::list<ReverseLookupResult> result;
      if (!ReverseLookupObject(candidate.GetRef(), result)) {
        return false;
      }

      if (!result.empty()) {
        Place place=GetPlace(result);

        if (!place.GetAddress()) {
          continue;
        }

        if (candidate.IsAtPlace()) {
          description.SetAtAddressDescription(std::make_shared<LocationAtPlaceDescription>(place));
        }
        else {
          description.SetAtAddressDescription(std::make_shared<LocationAtPlaceDescription>(place,
                            candidate.GetDistance()*1000, candidate.GetBearing()));
        }
        return true;
      }
    }

    return true;
  }

  bool LocationDescriptionService::DescribeLocationByPOI(const GeoCoord& location,
                                                         LocationDescription& description,
                                                         const double lookupDistance,
                                                         const double sizeFilter)
  {
    // search all addressable areas and nodes, sort it by distance, get first with address
    TypeConfigRef typeConfig=database->GetTypeConfig();

    if (!typeConfig) {
      return false;
    }

    std::vector<LocationDescriptionCandicate> candidates;

    TypeInfoSet poiTypes;

    // near addressable areas
    for (const auto& type : typeConfig->GetTypes()) {
      if (type->CanBeArea() &&
          type->GetIndexAsPOI()) {
        poiTypes.Set(type);
      }
    }

    if (!poiTypes.Empty()) {
      if (!LoadNearAreas(location,
                         poiTypes,
                         candidates,
                         lookupDistance)){
        return false;
      }
    }

    // near addressable nodes
    poiTypes.Clear();
    for (const auto& type : typeConfig->GetTypes()) {
      if (type->CanBeNode() &&
          type->GetIndexAsPOI()) {
        poiTypes.Set(type);
      }
    }
    if (!poiTypes.Empty()) {
      if (!LoadNearNodes(location,
                         poiTypes,
                         candidates,
                         lookupDistance)){
        return false;
      }
    }

    if (candidates.empty()) {
      return true;
    }

    // sort all candidates by its distance from location
    std::sort(candidates.begin(),candidates.end(),DistanceComparator);

    for (const auto &candidate : candidates) {
      if (candidate.GetSize() > sizeFilter){
        continue;
      }

      std::list<ReverseLookupResult> result;
      if (!ReverseLookupObject(candidate.GetRef(), result)) {
        return false;
      }

      if (!result.empty()) {
        Place place=GetPlace(result);

        if (candidate.IsAtPlace()) {
          description.SetAtPOIDescription(std::make_shared<LocationAtPlaceDescription>(place));
        }
        else {
          description.SetAtPOIDescription(std::make_shared<LocationAtPlaceDescription>(place,
                                                                                       candidate.GetDistance()*1000,
                                                                                       candidate.GetBearing()));
        }
        return true;
      }
    }

    return true;
  }

  /**
   * Returns crossings (of roads that can be driven by cars and which have a name feature)
   *
   * @param location
   *    Location to search for the closest crossing
   * @param description
   *    The description returned
   * @param lookupDistance
   *    The range to look in
   * @return
   */
  bool LocationDescriptionService::DescribeLocationByCrossing(const GeoCoord& location,
                                                              LocationDescription& description,
                                                              const double lookupDistance)
  {
    TypeConfigRef          typeConfig=database->GetTypeConfig();
    NameFeatureLabelReader nameFeatureLabelReader(*typeConfig);

    if (!typeConfig) {
      return false;
    }

    std::vector<WayRef> candidates;

    TypeInfoSet wayTypes;

    // near addressable ways
    for (const auto& type : typeConfig->GetTypes()) {
      if (type->CanBeWay() &&
          type->CanRouteCar() &&
          type->HasFeature(NameFeature::NAME)) {
        wayTypes.Set(type);
      }
    }

    if (!wayTypes.Empty()) {
      if (!LoadNearWays(location,
                        wayTypes,
                        candidates,
                        lookupDistance)) {
        return false;
      }
    }

    // Remove candidates if they do no have a name
    candidates.erase(std::remove_if(candidates.begin(),candidates.end(),[&nameFeatureLabelReader](const WayRef& candidate) -> bool {
      return nameFeatureLabelReader.GetLabel(candidate->GetFeatureValueBuffer()).empty();
    }),candidates.end());

    if (candidates.empty()) {
      return true;
    }

    std::map<Point,std::set<std::string>> routeNodeUseCount;

    for (const auto& candidate : candidates) {
      for (const auto& point : candidate->nodes) {
        if (point.IsRelevant()) {
          routeNodeUseCount[point].insert(nameFeatureLabelReader.GetLabel(candidate->GetFeatureValueBuffer()));
        }
      }
    }

    std::vector<Point> crossings;

    crossings.reserve(routeNodeUseCount.size());

    for (const auto& entry : routeNodeUseCount) {
      if (entry.second.size()>=2) {
        crossings.push_back(entry.first);
      }
    }

    if (crossings.empty()) {
      return true;
    }

    Point  candidate=Point(0,GeoCoord(0.0,0.0));
    double candidateDistance=std::numeric_limits<double>::max();

    for (const auto& entry : crossings) {
      double distance=GetEllipsoidalDistance(entry.GetCoord(),location);

      if (distance<candidateDistance) {
        candidate=entry;
        candidateDistance=distance;
      }
    }

    std::list<WayRef> crossingWays;

    for (const auto& way : candidates) {
      for (const auto& point : way->nodes) {
        if (candidate.IsIdentical(point)) {
          crossingWays.push_back(way);
        }
      }
    }

    std::list<Place> places;

    for (const auto& way : crossingWays) {
      std::list<ReverseLookupResult> result;

      if (!ReverseLookupObject(way->GetObjectFileRef(),
                               result)) {
        return false;
      }

      places.push_back(GetPlace(result));
    }

    LocationCrossingDescriptionRef crossingDescription;

    if (candidate.GetCoord()==location) {
      crossingDescription=std::make_shared<LocationCrossingDescription>(candidate.GetCoord(),
                                                                        places);
    }
    else {
      double distance=GetEllipsoidalDistance(location,
                                             candidate.GetCoord());
      double bearing=GetSphericalBearingInitial(candidate.GetCoord(),
                                                location);

      crossingDescription=std::make_shared<LocationCrossingDescription>(candidate.GetCoord(),
                                                                        places,
                                                                        distance*1000,
                                                                        bearing);
    }

    description.SetCrossingDescription(crossingDescription);

    return true;
  }

  /**
   * Returns ways (roads that can be driven by cars and which have a name feature)
   *
   * @param location
   *    Location to search for the closest ways
   * @param description
   *    The description returned
   * @param lookupDistance
   *    The range to look in
   * @return
   */
  bool LocationDescriptionService::DescribeLocationByWay(const GeoCoord& location,
                                                         LocationDescription& description,
                                                         const double lookupDistance)
  {
    TypeConfigRef          typeConfig=database->GetTypeConfig();
    NameFeatureLabelReader nameFeatureLabelReader(*typeConfig);

    if (!typeConfig) {
      return false;
    }

    std::vector<WayRef> candidates;

    TypeInfoSet wayTypes;

    // near addressable ways
    for (const auto& type : typeConfig->GetTypes()) {
      if (type->CanBeWay() &&
          type->HasFeature(NameFeature::NAME)) {
        wayTypes.Set(type);
      }
    }

    if (!wayTypes.Empty()) {
      if (!LoadNearWays(location,
                        wayTypes,
                        candidates,
                        lookupDistance)) {
        return false;
      }
    }

    // Remove candidates if they do no have a name
    candidates.erase(std::remove_if(candidates.begin(),candidates.end(),[&nameFeatureLabelReader](const WayRef& candidate) -> bool {
      return nameFeatureLabelReader.GetLabel(candidate->GetFeatureValueBuffer()).empty();
    }),candidates.end());

    if (candidates.empty()) {
      return true;
    }

    WayRef way;
    double minDistance = std::numeric_limits<double>::max();
    for (const auto& candidate : candidates) {
      for (size_t i = 0;  i < candidate->nodes.size() - 1; i++) {
        double r, intersectLon, intersectLat;
        double distance = DistanceToSegment(location.GetLon(),location.GetLat(),candidate->nodes[i].GetLon(),candidate->nodes[i].GetLat(),
                        candidate->nodes[i+1].GetLon(),candidate->nodes[i+1].GetLat(), r, intersectLon, intersectLat);
        if (distance < minDistance) {
          minDistance = distance;
          way = candidate;
        }
      }
    }

    std::list<ReverseLookupResult> result;
    if (!ReverseLookupObject(way->GetObjectFileRef(), result)) {
      return false;
    }

    Place place = GetPlace(result);
    LocationWayDescriptionRef wayDescription;
    wayDescription=std::make_shared<LocationWayDescription>(place, minDistance*1000);

    description.SetWayDescription(wayDescription);

    return true;
  }

  bool LocationDescriptionService::DescribeLocation(const GeoCoord& location,
                                                    LocationDescription& description,
                                                    const double lookupDistance,
                                                    const double sizeFilter)
  {
    description.SetCoordDescription(std::make_shared<LocationCoordDescription>(location));

    if (!DescribeLocationByName(location,
                                description,
                                lookupDistance,
                                sizeFilter)) {
      return false;
    }

    if (!DescribeLocationByAddress(location,
                                   description,
                                   lookupDistance,
                                   sizeFilter)) {
      return false;
    }

    if (!DescribeLocationByPOI(location,
                               description,
                               lookupDistance,
                               sizeFilter)) {
      return false;
    }

    if (!DescribeLocationByWay(location,
                               description,
                               lookupDistance)) {
      return false;
    }

    return DescribeLocationByCrossing(location,
                                      description,
                                      lookupDistance);

  }
}
