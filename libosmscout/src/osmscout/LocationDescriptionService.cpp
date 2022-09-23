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
#include <osmscout/FeatureReader.h>

#include <osmscout/system/Math.h>

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
    atPlace(true)
  {
    // no code
  }

  LocationAtPlaceDescription::LocationAtPlaceDescription(const Place &place,
                                                         const Distance &distance,
                                                         const Bearing &bearing)
  : place(place),
    atPlace(false),
    distance(distance),
    bearing(bearing)
  {
    // no code
  }

  LocationWayDescription::LocationWayDescription(const Place &way)
  : way(way)
  {
    // no code
  }

  LocationWayDescription::LocationWayDescription(const Place &way,
                                                 const Distance &distance)
    : way(way),
      distance(distance)
  {
    // no code
  }

  LocationCrossingDescription::LocationCrossingDescription(const GeoCoord& crossing,
                                                           const std::list<Place>& ways)
  : crossing(crossing),
    atPlace(true),
    ways(ways)
  {
    // no code
  }

  LocationCrossingDescription::LocationCrossingDescription(const GeoCoord& crossing,
                                                           const std::list<Place>& ways,
                                                           const Distance &distance,
                                                           const Bearing &bearing)
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

  void LocationDescriptionService::AddToCandidates(std::vector<LocationDescriptionCandicate>& candidates,
                                                   const GeoCoord& location,
                                                   const NodeRegionSearchResult& results,
                                                   bool requireAddress,
                                                   bool requireName)
  {
    NameFeatureLabelReader nameFeatureLabelReader(*database->GetTypeConfig());
    NameFeatureValueReader nameFeatureReader(*database->GetTypeConfig());
    AddressFeatureValueReader addressFeatureReader(*database->GetTypeConfig());

    for (const auto& entry : results.GetNodeResults()) {
      if ((requireAddress && addressFeatureReader.GetValue(entry.GetNode()->GetFeatureValueBuffer()) == nullptr) ||
          (requireName && nameFeatureReader.GetValue(entry.GetNode()->GetFeatureValueBuffer()) == nullptr)) {
        continue;
      }
      GeoBox boundingBox;
      auto bearing=GetSphericalBearingInitial(entry.GetNode()->GetCoords(),location);

      candidates.emplace_back(entry.GetNode()->GetObjectFileRef(),
                              nameFeatureLabelReader.GetLabel(entry.GetNode()->GetFeatureValueBuffer()),
                              entry.GetDistance(),
                              bearing,
                              false,
                              0.0);
    }
  }

  void LocationDescriptionService::AddToCandidates(std::vector<LocationDescriptionCandicate>& candidates,
                                                   const GeoCoord& location,
                                                   const WayRegionSearchResult& results)
  {
    NameFeatureLabelReader nameFeatureLabelReader(*database->GetTypeConfig());

    for (const auto& entry : results.GetWayResults()) {
      GeoBox boundingBox=entry.GetWay()->GetBoundingBox();
      auto bearing=GetSphericalBearingInitial(entry.GetClosestPoint(),
                                              location);

      candidates.emplace_back(entry.GetWay()->GetObjectFileRef(),
                              nameFeatureLabelReader.GetLabel(entry.GetWay()->GetFeatureValueBuffer()),
                              entry.GetDistance(),
                              bearing,
                              false,
                              boundingBox.GetSize());
    }
  }

  void LocationDescriptionService::AddToCandidates(std::vector<LocationDescriptionCandicate>& candidates,
                                                   const GeoCoord& location,
                                                   const AreaRegionSearchResult& results,
                                                   bool requireAddress,
                                                   bool requireName)
  {
    NameFeatureLabelReader nameFeatureLabelReader(*database->GetTypeConfig());
    NameFeatureValueReader nameFeatureReader(*database->GetTypeConfig());
    AddressFeatureValueReader addressFeatureReader(*database->GetTypeConfig());

    for (const auto& entry : results.GetAreaResults()) {
      if ((requireAddress && addressFeatureReader.GetValue(entry.GetArea()->GetFeatureValueBuffer()) == nullptr) ||
          (requireName && nameFeatureReader.GetValue(entry.GetArea()->GetFeatureValueBuffer()) == nullptr)) {
        continue;
      }

      GeoBox boundingBox=entry.GetArea()->GetBoundingBox();
      auto bearing=GetSphericalBearingInitial(entry.GetClosestPoint(),location);

      candidates.emplace_back(entry.GetArea()->GetObjectFileRef(),
                              nameFeatureLabelReader.GetLabel(entry.GetArea()->GetFeatureValueBuffer()),
                              entry.GetDistance(),
                              bearing,
                              entry.IsInArea(),
                              boundingBox.GetSize());
    }
  }

  class POIReverseLookupVisitor : public POIVisitor
  {
  private:
    std::set<ObjectFileRef>                   objects;
    std::list<LocationDescriptionService::ReverseLookupResult>& results;

  public:
    explicit POIReverseLookupVisitor(std::list<LocationDescriptionService::ReverseLookupResult>& results);

    void AddObject(const ObjectFileRef& object);

    bool Visit(const AdminRegion& adminRegion,
               const POI &poi) override;
  };

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

  class LocationReverseLookupVisitor : public LocationVisitor
  {
  private:
    const LocationIndex&                      locationIndex;
    std::set<ObjectFileRef>                   objects;
    std::list<LocationDescriptionService::ReverseLookupResult>& results;
    bool                                      lookupAddress;
    AddressReverseLookupVisitor               addressVisitor;

  public:
    LocationReverseLookupVisitor(const LocationIndex& locationIndex,
                                 std::list<LocationDescriptionService::ReverseLookupResult>& results,
                                 bool lookupAddress);

    void AddObject(const ObjectFileRef& object);

    bool Visit(const AdminRegion& adminRegion,
               const PostalArea& postalArea,
               const Location &location) override;
  };

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
    const LocationIndex&                             locationIndex;
    std::list<LocationDescriptionService::ReverseLookupResult>& results;

    std::list<SearchEntry>                           searchEntries;

    bool                                             lookupPoi;
    POIReverseLookupVisitor                          poiVisitor;
    bool                                             lookupLocation;
    LocationReverseLookupVisitor                     locationVisitor;

  public:
    std::map<FileOffset,AdminRegionRef>              adminRegions;

  public:
    AdminRegionReverseLookupVisitor(const Database& database,
                                    const LocationIndex& locationIndex,
                                    std::list<LocationDescriptionService::ReverseLookupResult>& results,
                                    bool lookupPoi,
                                    bool lookupLocation,
                                    bool lookupAddress);

    void AddSearchEntry(const SearchEntry& searchEntry);

    Action Visit(const AdminRegion& region) override;
  };

  AdminRegionReverseLookupVisitor::AdminRegionReverseLookupVisitor(const Database& database,
                                                                   const LocationIndex& locationIndex,
                                                                   std::list<LocationDescriptionService::ReverseLookupResult>& results,
                                                                   bool lookupPoi,
                                                                   bool lookupLocations,
                                                                   bool lookupAddress)
  : database(database),
    locationIndex(locationIndex),
    results(results),
    lookupPoi(lookupPoi),
    poiVisitor(results),
    lookupLocation(lookupLocations),
    locationVisitor(locationIndex, results, lookupAddress)
  {
  }

  void AdminRegionReverseLookupVisitor::AddSearchEntry(const SearchEntry& searchEntry)
  {
    searchEntries.push_back(searchEntry);
    if (lookupPoi){
      poiVisitor.AddObject(searchEntry.object);
    }
    if (lookupLocation){
      locationVisitor.AddObject(searchEntry.object);
    }
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
      if (!ring.IsTopOuter()) {
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
      adminRegions.emplace(region.regionOffset, std::make_shared<AdminRegion>(region));
      if (lookupPoi){
        if (!locationIndex.VisitPOIs(region,
                                     poiVisitor,
                                     false)) {
          return error;
        }
      }
      if (lookupLocation){
        for (const auto& postalArea : region.postalAreas) {
          if (!locationIndex.VisitLocations(region,
                                            postalArea,
                                            locationVisitor,
                                            false)) {
            return error;
          }
        }
      }
    }

    if (atLeastOneCandidate) {
      return visitChildren;
    }


    return skipChildren;
  }

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

  LocationReverseLookupVisitor::LocationReverseLookupVisitor(const LocationIndex& locationIndex,
                                                             std::list<LocationDescriptionService::ReverseLookupResult>& results,
                                                             bool lookupAddress)
  : locationIndex(locationIndex),
    results(results),
    lookupAddress(lookupAddress),
    addressVisitor(results)
  {
    // no code
  }

  void LocationReverseLookupVisitor::AddObject(const ObjectFileRef& object)
  {
    objects.insert(object);
    if (lookupAddress){
      addressVisitor.AddObject(object);
    }
  }

  bool LocationReverseLookupVisitor::Visit(const AdminRegion& adminRegion,
                                           const PostalArea& postalArea,
                                           const Location &location)
  {
    if (lookupAddress){
      if (!locationIndex.VisitAddresses(adminRegion,
                                        postalArea,
                                        location,
                                        addressVisitor)) {
        return false;
      }
    }

    AdminRegionRef adminRegionRef;
    PostalAreaRef postalAreaRef;
    LocationRef locationRef;

    for (const auto& object : location.objects) {
      if (objects.find(object)!=objects.end()) {
        LocationDescriptionService::ReverseLookupResult result;

        if (!adminRegionRef) {
          adminRegionRef=std::make_shared<AdminRegion>(adminRegion);
        }
        if (!postalAreaRef) {
          postalAreaRef=std::make_shared<PostalArea>(postalArea);
        }
        if (!locationRef){
          locationRef=std::make_shared<Location>(location);
        }

        result.object=object;
        result.adminRegion=adminRegionRef;
        result.postalArea=postalAreaRef;
        result.location=locationRef;

        results.push_back(result);
      }
    }

    return true;
  }

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

    LocationIndexRef locationIndex=database->GetLocationIndex();
    LocationIndex::ScopeCacheCleaner cacheCleaner(database->GetLocationIndex());

    if (!locationIndex) {
      return false;
    }

    AdminRegionReverseLookupVisitor adminRegionVisitor(*database,
                                                       *locationIndex,
                                                       result,
                                                       false,
                                                       false,
                                                       false);
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
                                                       *locationIndex,
                                                       result,
                                                       true,
                                                       true,
                                                       true);

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
          if (ring.IsTopOuter()) {
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
        searchEntry.bbox=way->GetBoundingBox();

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
                                                          const Distance& lookupDistance,
                                                          const double sizeFilter)
  {

    // search all nameable areas and nodes, sort it by distance, get first with name
    TypeConfigRef typeConfig=database->GetTypeConfig();

    if (!typeConfig) {
      return false;
    }

    LocationIndex::ScopeCacheCleaner          cacheCleaner(database->GetLocationIndex());
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
      AreaRegionSearchResult areaSearchResult=database->LoadAreasInRadius(location,
                                                                          nameTypes,
                                                                          lookupDistance);

      AddToCandidates(candidates,
                      location,
                      areaSearchResult,
                      false,
                      true);
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
      NodeRegionSearchResult nodeSearchResult=database->LoadNodesInRadius(location,
                                                                          nameTypes,
                                                                          lookupDistance);
      AddToCandidates(candidates,
                      location,
                      nodeSearchResult,
                      false,
                      true);
    }

    if (candidates.empty()) {
      return true;
    }

    // sort all candidates by its distance from location
    std::sort(candidates.begin(),candidates.end(),DistanceComparator);

    for (const auto &candidate : candidates) {
      std::list<ReverseLookupResult> result;

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
                                                                                        candidate.GetDistance(),
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
                                                                                        candidate.GetDistance(),
                                                                                        candidate.GetBearing()));
        }

        return true;
      }
    }

    return true;
  }

  bool LocationDescriptionService::DescribeLocationByAddress(const GeoCoord& location,
                                                             LocationDescription& description,
                                                             const Distance& lookupDistance,
                                                             const double sizeFilter)
  {
    // search all addressable areas and nodes, sort it by distance, get first with address
    TypeConfigRef typeConfig=database->GetTypeConfig();

    if (!typeConfig) {
      return false;
    }

    LocationIndex::ScopeCacheCleaner cacheCleaner(database->GetLocationIndex());

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
      AreaRegionSearchResult areaSearchResult=database->LoadAreasInRadius(location,
                                                                          addressTypes,
                                                                          lookupDistance);

      AddToCandidates(candidates,
                      location,
                      areaSearchResult,
                      true, // objects without address feature are not in address index
                      false);
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
      NodeRegionSearchResult nodeSearchResult=database->LoadNodesInRadius(location,
                                                                          addressTypes,
                                                                          lookupDistance);
      AddToCandidates(candidates,
                      location,
                      nodeSearchResult,
                      true, // objects without address feature are not in address index
                      false);
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
                            candidate.GetDistance(), candidate.GetBearing()));
        }
        return true;
      }
    }

    return true;
  }

  bool LocationDescriptionService::DescribeLocationByPOI(const GeoCoord& location,
                                                         LocationDescription& description,
                                                         const Distance& lookupDistance,
                                                         const double sizeFilter)
  {
    // search all addressable areas and nodes, sort it by distance, get first with address
    TypeConfigRef typeConfig=database->GetTypeConfig();

    if (!typeConfig) {
      return false;
    }

    LocationIndex::ScopeCacheCleaner          cacheCleaner(database->GetLocationIndex());
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
      AreaRegionSearchResult areaSearchResult=database->LoadAreasInRadius(location,
                                                                          poiTypes,
                                                                          lookupDistance);

      AddToCandidates(candidates,
                      location,
                      areaSearchResult,
                      false,
                      true); // POI have to have name feature to be in location index
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
      NodeRegionSearchResult nodeSearchResult=database->LoadNodesInRadius(location,
                                                                          poiTypes,
                                                                          lookupDistance);
      AddToCandidates(candidates,
                      location,
                      nodeSearchResult,
                      false,
                      true); // POI have to have name feature to be in location index
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
                                                                                       candidate.GetDistance(),
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
                                                              const Distance& lookupDistance)
  {
    TypeConfigRef          typeConfig=database->GetTypeConfig();
    NameFeatureLabelReader nameFeatureLabelReader(*typeConfig);
    if (!typeConfig) {
      return false;
    }

    LocationIndex::ScopeCacheCleaner cacheCleaner(database->GetLocationIndex());
    std::vector<WayRef>              candidates;

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
      WayRegionSearchResult waySearchResult=database->LoadWaysInRadius(location,
                                                                       wayTypes,
                                                                       lookupDistance);

      for (const auto& entry : waySearchResult.GetWayResults()) {
        candidates.push_back(entry.GetWay());
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
    Distance candidateDistance=Distance::Max();

    for (const auto& entry : crossings) {
      Distance distance=GetEllipsoidalDistance(entry.GetCoord(),location);

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
      Distance distance=GetEllipsoidalDistance(location,
                                               candidate.GetCoord());
      auto bearing=GetSphericalBearingInitial(candidate.GetCoord(),
                                              location);

      crossingDescription=std::make_shared<LocationCrossingDescription>(candidate.GetCoord(),
                                                                        places,
                                                                        distance,
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
                                                         const Distance& lookupDistance)
  {
    TypeConfigRef          typeConfig=database->GetTypeConfig();
    NameFeatureLabelReader nameFeatureLabelReader(*typeConfig);
    RefFeatureLabelReader  refFeatureLabelReader(*typeConfig);

    if (!typeConfig) {
      return false;
    }

    std::vector<WayRef>              candidates;
    LocationIndex::ScopeCacheCleaner cacheCleaner(database->GetLocationIndex());

    TypeInfoSet wayTypes;

    // near addressable ways
    for (const auto& type : typeConfig->GetTypes()) {
      if (type->CanBeWay() &&
        (type->HasFeature(NameFeature::NAME) ||
         type->HasFeature(RefFeature::NAME))) {
        wayTypes.Set(type);
      }
    }

    if (!wayTypes.Empty()) {
      WayRegionSearchResult waySearchResult=database->LoadWaysInRadius(location,
                                                                       wayTypes,
                                                                       lookupDistance);

      for (const auto& entry : waySearchResult.GetWayResults()) {
        candidates.push_back(entry.GetWay());
      }

    }

    // Remove candidates if they do no have a name
    candidates.erase(std::remove_if(candidates.begin(),
                                    candidates.end(),
                                    [&nameFeatureLabelReader,&refFeatureLabelReader](const WayRef& candidate) -> bool {
      return nameFeatureLabelReader.GetLabel(candidate->GetFeatureValueBuffer()).empty() &&
             refFeatureLabelReader.GetLabel(candidate->GetFeatureValueBuffer()).empty();
    }),candidates.end());

    if (candidates.empty()) {
      return true;
    }

    WayRef way;
    double minDistanceDeg = std::numeric_limits<double>::max();
    Distance minDistance = Distance::Max();
    for (const auto& candidate : candidates) {
      for (size_t i = 0;  i < candidate->nodes.size() - 1; i++) {
        double r, intersectLon, intersectLat;
        double distanceDeg = DistanceToSegment(location.GetLon(),location.GetLat(),candidate->nodes[i].GetLon(),candidate->nodes[i].GetLat(),
                                               candidate->nodes[i+1].GetLon(),candidate->nodes[i+1].GetLat(), r, intersectLon, intersectLat);
        if (distanceDeg < minDistanceDeg) {
          minDistanceDeg = distanceDeg;
          minDistance = GetSphericalDistance(location, GeoCoord(intersectLat, intersectLon));
          way = candidate;
        }
      }
    }

    std::list<ReverseLookupResult> result;
    if (!ReverseLookupObject(way->GetObjectFileRef(), result)) {
      return false;
    }

    if(result.empty()) {
      return true;
    }

    Place place = GetPlace(result);
    LocationWayDescriptionRef wayDescription=std::make_shared<LocationWayDescription>(place, minDistance);

    description.SetWayDescription(wayDescription);

    return true;
  }

  bool LocationDescriptionService::DescribeLocation(const GeoCoord& location,
                                                    LocationDescription& description,
                                                    const Distance& lookupDistance,
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
