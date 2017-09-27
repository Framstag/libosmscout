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

#include <algorithm>

#include <osmscout/LocationService.h>

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

  LocationCrossingDescriptionRef LocationDescription::GetCrossingDescription() const
  {
    return crossingDescription;
  }

  LocationService::VisitorMatcher::VisitorMatcher(const std::string& searchPattern)
  :pattern(searchPattern)
  {
    TolowerUmlaut(pattern);
  }

  /**
   * Internal helper class to unite the parameter and thus the code for string based and form based
   * location search.
   */
  struct SearchParameter CLASS_FINAL
  {
    bool                    searchForLocation;
    bool                    searchForPOI;
    bool                    adminRegionOnlyMatch;
    bool                    poiOnlyMatch;
    bool                    locationOnlyMatch;
    bool                    addressOnlyMatch;
    StringMatcherFactoryRef stringMatcherFactory;
    size_t                  limit;

    SearchParameter() = default;
  };

  LocationFormSearchParameter::LocationFormSearchParameter()
    : adminRegionOnlyMatch(false),
      postalAreaOnlyMatch(false),
      locationOnlyMatch(false),
      addressOnlyMatch(false),
      stringMatcherFactory(std::make_shared<osmscout::StringMatcherCIFactory>()),
      limit(100)
  {
    // no code
  }

  size_t LocationFormSearchParameter::GetLimit() const
  {
    return limit;
  }

  StringMatcherFactoryRef LocationFormSearchParameter::GetStringMatcherFactory() const
  {
    return stringMatcherFactory;
  }

  std::string LocationFormSearchParameter::GetAdminRegionSearchString() const
  {
    return adminRegionSearchString;
  }

  std::string LocationFormSearchParameter::GetPostalAreaSearchString() const
  {
    return postalAreaSearchString;
  }

  std::string LocationFormSearchParameter::GetLocationSearchString() const
  {
    return locationSearchString;
  }

  std::string LocationFormSearchParameter::GetAddressSearchString() const
  {
    return addressSearchString;
  }

  void LocationFormSearchParameter::SetStringMatcherFactory(const StringMatcherFactoryRef& stringMatcherFactory)
  {
    this->stringMatcherFactory=stringMatcherFactory;
  }

  void LocationFormSearchParameter::SetAdminRegionSearchString(const std::string& adminRegionSearchString)
  {
    LocationFormSearchParameter::adminRegionSearchString=adminRegionSearchString;
  }

  void LocationFormSearchParameter::SetPostalAreaSearchString(const std::string& postalAreaSearchString)
  {
    LocationFormSearchParameter::postalAreaSearchString=postalAreaSearchString;
  }

  void LocationFormSearchParameter::SetLocationSearchString(const std::string& locationSearchString)
  {
    LocationFormSearchParameter::locationSearchString=locationSearchString;
  }

  void LocationFormSearchParameter::SetAddressSearchString(const std::string& addressSearchString)
  {
    LocationFormSearchParameter::addressSearchString=addressSearchString;
  }

  void LocationFormSearchParameter::SetAdminRegionOnlyMatch(bool adminRegionOnlyMatch)
  {
    this->adminRegionOnlyMatch=adminRegionOnlyMatch;
  }

  void LocationFormSearchParameter::SetPostalAreaOnlyMatch(bool postalAreaOnlyMatch)
  {
    this->postalAreaOnlyMatch=postalAreaOnlyMatch;
  }

  void LocationFormSearchParameter::SetLocationOnlyMatch(bool locationOnlyMatch)
  {
    this->locationOnlyMatch=locationOnlyMatch;
  }

  void LocationFormSearchParameter::SetAddressOnlyMatch(bool addressOnlyMatch)
  {
    this->addressOnlyMatch=addressOnlyMatch;
  }

  bool LocationFormSearchParameter::GetAdminRegionOnlyMatch() const
  {
    return adminRegionOnlyMatch;
  }

  bool LocationFormSearchParameter::GetPostalAreaOnlyMatch() const
  {
    return postalAreaOnlyMatch;
  }

  bool LocationFormSearchParameter::GetLocationOnlyMatch() const
  {
    return locationOnlyMatch;
  }

  bool LocationFormSearchParameter::GetAddressOnlyMatch() const
  {
    return addressOnlyMatch;
  }

  void LocationFormSearchParameter::SetLimit(size_t limit)
  {
    this->limit=limit;
  }

  POIFormSearchParameter::POIFormSearchParameter()
    : adminRegionOnlyMatch(false),
      poiOnlyMatch(false),
      stringMatcherFactory(std::make_shared<osmscout::StringMatcherCIFactory>()),
      limit(100)
  {
    // no code
  }

  size_t POIFormSearchParameter::GetLimit() const
  {
    return limit;
  }

  StringMatcherFactoryRef POIFormSearchParameter::GetStringMatcherFactory() const
  {
    return stringMatcherFactory;
  }

  std::string POIFormSearchParameter::GetAdminRegionSearchString() const
  {
    return adminRegionSearchString;
  }

  std::string POIFormSearchParameter::GetPOISearchString() const
  {
    return poiSearchString;
  }

  void POIFormSearchParameter::SetStringMatcherFactory(const StringMatcherFactoryRef& stringMatcherFactory)
  {
    this->stringMatcherFactory=stringMatcherFactory;
  }

  void POIFormSearchParameter::SetAdminRegionSearchString(const std::string& adminRegionSearchString)
  {
    POIFormSearchParameter::adminRegionSearchString=adminRegionSearchString;
  }

  void POIFormSearchParameter::SetPOISearchString(const std::string& poiSearchString)
  {
    POIFormSearchParameter::poiSearchString=poiSearchString;
  }

  void POIFormSearchParameter::SetAdminRegionOnlyMatch(bool adminRegionOnlyMatch)
  {
    this->adminRegionOnlyMatch=adminRegionOnlyMatch;
  }

  void POIFormSearchParameter::SetPOIOnlyMatch(bool poiOnlyMatch)
  {
    this->poiOnlyMatch=poiOnlyMatch;
  }

  bool POIFormSearchParameter::GetAdminRegionOnlyMatch() const
  {
    return adminRegionOnlyMatch;
  }

  bool POIFormSearchParameter::GetPOIOnlyMatch() const
  {
    return poiOnlyMatch;
  }

  void POIFormSearchParameter::SetLimit(size_t limit)
  {
    this->limit=limit;
  }

  LocationStringSearchParameter::LocationStringSearchParameter(const std::string& searchString)
    : searchForLocation(true),
      searchForPOI(true),
      adminRegionOnlyMatch(false),
      poiOnlyMatch(false),
      locationOnlyMatch(false),
      addressOnlyMatch(false),
      searchString(searchString),
      stringMatcherFactory(std::make_shared<osmscout::StringMatcherCIFactory>()),
      limit(100)
  {
    // no code
  }

  std::string LocationStringSearchParameter::GetSearchString() const
  {
    return searchString;
  }

  AdminRegionRef LocationStringSearchParameter::GetDefaultAdminRegion() const
  {
    return defaultAdminRegion;
  }

  size_t LocationStringSearchParameter::GetLimit() const
  {
    return limit;
  }

  void LocationStringSearchParameter::SetDefaultAdminRegion(const AdminRegionRef& adminRegion)
  {
    this->defaultAdminRegion=adminRegion;
  }

  void LocationStringSearchParameter::SetLimit(size_t limit)
  {
    this->limit=limit;
  }

  StringMatcherFactoryRef LocationStringSearchParameter::GetStringMatcherFactory() const
  {
    return stringMatcherFactory;
  }

  void LocationStringSearchParameter::SetStringMatcherFactory(const StringMatcherFactoryRef& stringMatcherFactory)
  {
    this->stringMatcherFactory=stringMatcherFactory;
  }

  bool LocationStringSearchParameter::GetSearchForLocation() const
  {
    return searchForLocation;
  }

  bool LocationStringSearchParameter::GetSearchForPOI() const
  {
    return searchForPOI;
  }

  void LocationStringSearchParameter::SetSearchForLocation(bool searchForLocation)
  {
    this->searchForLocation=searchForLocation;
  }

  void LocationStringSearchParameter::SetSearchForPOI(bool searchForPOI)
  {
    this->searchForPOI=searchForPOI;
  }

  void LocationStringSearchParameter::SetAdminRegionOnlyMatch(bool adminRegionOnlyMatch)
  {
    this->adminRegionOnlyMatch=adminRegionOnlyMatch;
  }

  void LocationStringSearchParameter::SetPOIOnlyMatch(bool poiOnlyMatch)
  {
    this->poiOnlyMatch=poiOnlyMatch;
  }

  void LocationStringSearchParameter::SetLocationOnlyMatch(bool locationOnlyMatch)
  {
    this->locationOnlyMatch=locationOnlyMatch;
  }

  void LocationStringSearchParameter::SetAddressOnlyMatch(bool addressOnlyMatch)
  {
    this->addressOnlyMatch=addressOnlyMatch;
  }

  bool LocationStringSearchParameter::GetAdminRegionOnlyMatch() const
  {
    return adminRegionOnlyMatch;
  }

  bool LocationStringSearchParameter::GetPOIOnlyMatch() const
  {
    return poiOnlyMatch;
  }

  bool LocationStringSearchParameter::GetLocationOnlyMatch() const
  {
    return locationOnlyMatch;
  }

  bool LocationStringSearchParameter::GetAddressOnlyMatch() const
  {
    return addressOnlyMatch;
  }

  void LocationService::VisitorMatcher::Match(const std::string& name,
                                              bool& match,
                                              bool& candidate) const
  {
    std::string            tmpname=name;
    std::string::size_type matchPosition;

    TolowerUmlaut(tmpname);

    matchPosition=tmpname.find(pattern);

    match=matchPosition==0 && tmpname.length()==pattern.length();
    candidate=matchPosition!=std::string::npos;
  }

  void LocationService::VisitorMatcher::TolowerUmlaut(std::string& s) const
  {
    for (std::string::iterator it=s.begin();
         it!=s.end();
         ++it)
    {
      /* this filter matches all character from the table
       * http://en.wikipedia.org/wiki/Latin-1_Supplement_%28Unicode_block%29#Compact_table
       * beginning at U+0x00C0 to U+0x00DE
       */
      if((uint8_t)*it == 0xC3)
      {
        ++it;

        if((uint8_t)*it>=0x80 && (uint8_t)*it<=0x9E) {
          // 0x9F is german "sz" which is already small caps.
          *it+=0x20;
        }
      }
      else {
        *it=(char)tolower(*it);
      }
    }
  }

  LocationService::AdminRegionMatchVisitor::AdminRegionMatchVisitor(const std::string& adminRegionPattern,
                                                                    size_t limit)
  : VisitorMatcher(adminRegionPattern),
    limit(limit),
    limitReached(false)
  {
    // no code
  }

  AdminRegionVisitor::Action LocationService::AdminRegionMatchVisitor::Visit(const AdminRegion& region)
  {
    bool atLeastOneAliasMatch=false;

    bool regionMatch;
    bool regionCandidate;

    Match(region.name,
          regionMatch,
          regionCandidate);

    if (!regionMatch) {
      for (const auto& alias : region.aliases) {
        bool match;
        bool candidate;

        Match(alias.name,
              match,
              candidate);

        if (match || candidate) {
          AdminRegionResult result;

          result.adminRegion=std::make_shared<AdminRegion>(region);
          result.isMatch=match;

          result.adminRegion->aliasName=alias.name;
          result.adminRegion->aliasObject.Set(alias.objectOffset,refNode);

          //std::cout << pattern << " => (alias) " << result.adminRegion->aliasName << " " << match << " " << regionCandidate << " " << std::endl;

          results.push_back(result);

          if (match) {
            atLeastOneAliasMatch=true;
          }

          limitReached=results.size()>=limit;
        }
      }
    }

    // If we have a perfect match for an alias, we not not try to regionMatch
    // the region name itself
    if (!atLeastOneAliasMatch) {
      if (regionMatch || regionCandidate) {
        AdminRegionResult result;

        result.adminRegion=std::make_shared<AdminRegion>(region);
        result.isMatch=regionMatch;

        //std::cout << pattern << " => (region) " << result.adminRegion->name << " " << regionMatch << " " << regionCandidate << std::endl;

        results.push_back(result);
      }
    }

    if (limitReached) {
      return stop;
    }

    return visitChildren;
  }

  LocationService::PostalAreaMatchVisitor::PostalAreaMatchVisitor(const AdminRegionRef& adminRegion,
                                                                  const std::string& postalAreaPattern,
                                                                  size_t limit)
    : adminRegion(adminRegion),
      postalAreaPattern(postalAreaPattern),
      limit(limit),
      limitReached(false)
  {
    // no code
  }

  bool LocationService::PostalAreaMatchVisitor::Visit(const PostalArea& postalArea)
  {
    if (postalAreaPattern.empty()) {
      PostalAreaResult result;

      result.adminRegion=adminRegion;
      result.postalArea=std::make_shared<PostalArea>(postalArea);
      result.isMatch=false;

      results.push_back(result);
    }
    else {
      if (postalArea.name==postalAreaPattern ||
          postalArea.name.empty()) {
        PostalAreaResult result;

        result.adminRegion=adminRegion;
        result.postalArea=std::make_shared<PostalArea>(postalArea);
        result.isMatch=!postalArea.name.empty();

        results.push_back(result);
      }
    }

    limitReached=results.size()>=limit;

    return !limitReached;
  }

  LocationService::LocationMatchVisitor::LocationMatchVisitor(const std::string& pattern,
                                                              size_t limit)
  : VisitorMatcher(pattern),
    limit(limit),
    limitReached(false)
  {
    // no code
  }

  bool LocationService::LocationMatchVisitor::Visit(const AdminRegion& adminRegion,
                                                    const PostalArea& postalArea,
                                                    const Location &loc)
  {
    bool match;
    bool candidate;

    Match(loc.name,
          match,
          candidate);

    if (match || candidate) {
      Result result;

      result.adminRegion=std::make_shared<AdminRegion>(adminRegion);
      result.postalArea=std::make_shared<PostalArea>(postalArea);
      result.location=std::make_shared<Location>(loc);
      result.isMatch=match;

      results.push_back(result);

      //std::cout << pattern << " =>  " << result.location->name << " " << adminRegion.name << "/" << adminRegion.aliasName << " " << match << " " << candidate << " " << std::endl;

      limitReached=results.size()>=limit;
    }

    return !limitReached;
  }

  LocationService::POIMatchVisitor::POIMatchVisitor(const AdminRegionRef& adminRegion,
                                                    const std::string& pattern,
                                                    size_t limit)
    : VisitorMatcher(pattern),
      limit(limit),
      adminRegion(adminRegion),
      limitReached(false)
  {
    // no code
  }

  bool LocationService::POIMatchVisitor::Visit(const AdminRegion& adminRegion,
                                               const POI &poi)
  {
    bool match;
    bool candidate;

    Match(poi.name,
          match,
          candidate);

    if (match || candidate) {
      Result result;

      if (adminRegion.object==this->adminRegion->object) {
        result.adminRegion=this->adminRegion;
      }
      else {
        result.adminRegion=std::make_shared<AdminRegion>(adminRegion);
      }

      result.poi=std::make_shared<POI>(poi);
      result.isMatch=match;

      results.push_back(result);

      limitReached=results.size()>=limit;
    }

    return !limitReached;
  }

  LocationService::AddressMatchVisitor::AddressMatchVisitor(const std::string& pattern,
                                                            size_t limit)
  : VisitorMatcher(pattern),
    limit(limit),
    limitReached(false)
  {
    // no code
  }

  bool LocationService::AddressMatchVisitor::Visit(const AdminRegion& adminRegion,
                                                   const PostalArea& postalArea,
                                                   const Location& location,
                                                   const Address& address)
  {
    bool match;
    bool candidate;

    Match(address.name,
          match,
          candidate);

    if (match || candidate) {
      AddressResult result;

      result.adminRegion=std::make_shared<AdminRegion>(adminRegion);
      result.postalArea=std::make_shared<PostalArea>(postalArea);
      result.location=std::make_shared<Location>(location);
      result.address=std::make_shared<Address>(address);
      result.isMatch=match;

      //std::cout << pattern << " =>  " << result.address->name << " " << result.address->postalCode << " " << location.name << " " << adminRegion.name << "/" << adminRegion.aliasName << " " << match << " " << candidate << " " << std::endl;

      results.push_back(result);

      limitReached=results.size()>=limit;
    }

    return !limitReached;
  }

  LocationSearch::LocationSearch()
  : limit(50)
  {
    // no code
  }

  bool LocationSearchResult::Entry::operator<(const Entry& other) const
  {
    if (adminRegionMatchQuality!=other.adminRegionMatchQuality) {
      return adminRegionMatchQuality<other.adminRegionMatchQuality;
    }

    if (postalAreaMatchQuality!=other.postalAreaMatchQuality) {
      return postalAreaMatchQuality<other.postalAreaMatchQuality;
    }

    if (locationMatchQuality!=other.locationMatchQuality) {
      return locationMatchQuality<other.locationMatchQuality;
    }

    if (addressMatchQuality!=other.addressMatchQuality) {
      return addressMatchQuality<other.addressMatchQuality;
    }

    if (poiMatchQuality!=other.poiMatchQuality) {
      return poiMatchQuality<other.poiMatchQuality;
    }

    if (adminRegion && other.adminRegion &&
        adminRegion->name!=other.adminRegion->name) {
      return adminRegion->name<other.adminRegion->name;
    }
    if (postalArea && other.postalArea &&
        postalArea->name!=other.postalArea->name) {
      return postalArea->name<other.postalArea->name;
    }

    if (location && other.location &&
        location->name!=other.location->name) {
      return location->name<other.location->name;
    }

    if (address && other.address &&
        address->name!=other.address->name) {
      return address->name<other.address->name;
    }

    if (poi && other.poi &&
        poi->name!=other.poi->name) {
      return poi->name<other.poi->name;
    }

    return this<&other;
  }

  bool LocationSearchResult::Entry::operator==(const Entry& other) const
  {
    if ((adminRegion && !other.adminRegion) ||
        (!adminRegion && other.adminRegion)) {
      return false;
    }

    if (adminRegion && other.adminRegion) {
      if (adminRegion->aliasObject!=other.adminRegion->aliasObject) {
        return false;
      }

      if (adminRegion->object!=other.adminRegion->object) {
        return false;
      }
    }

    if ((postalArea && !other.postalArea) ||
        (!postalArea && other.postalArea)) {
      return false;
    }

    if (postalArea && other.postalArea) {
      if (postalArea->name!=other.postalArea->name) {
        return false;
      }
    }

    if ((poi && !other.poi) ||
        (!poi && other.poi)) {
      return false;
    }

    if (poi && other.poi) {
      if (poi->object!=other.poi->object) {
        return false;
      }
    }

    if ((location && !other.location) ||
        (!location && other.location)) {
      return false;
    }

    if (location && other.location) {
      if (location->locationOffset!=other.location->locationOffset) {
        return false;
      }
    }

    if ((address && !other.address) ||
        (!address && other.address)) {
      return false;
    }

    if (address && other.address) {
      if (address->addressOffset!=other.address->addressOffset) {
        return false;
      }
    }

    return true;
  }

  /**
   * LocationService constructor
   *
   * @param database
   *    Valid reference to a database instance
   */
  LocationService::LocationService(const DatabaseRef& database)
  : database(database)
  {
    assert(database);
  }

  const FeatureValueBufferRef LocationService::GetObjectFeatureBuffer(const ObjectFileRef &object)
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

  Place LocationService::GetPlace(const std::list<ReverseLookupResult>& lookupResult)
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
  bool LocationService::VisitAdminRegions(AdminRegionVisitor& visitor) const
  {
    LocationIndexRef locationIndex=database->GetLocationIndex();

    if (!locationIndex) {
      return false;
    }

    return locationIndex->VisitAdminRegions(visitor);
  }

  /**
   * Visit the location at the given region and all its sub regions.
   * @param region
   *    Region to start at
   * @param visitor
   *    Visitor
   * @return
   *    True, if there was no error
   */
  bool LocationService::VisitAdminRegionLocations(const AdminRegion& region,
                                                  const PostalArea& postalArea,
                                                  LocationVisitor& visitor) const
  {
    LocationIndexRef locationIndex=database->GetLocationIndex();

    if (!locationIndex) {
      return false;
    }

    return locationIndex->VisitLocations(region,
                                         postalArea,
                                         visitor);
  }

  /**
   * Visit the POIs at the given region and all its sub regions.
   * @param region
   *    Region to start at
   * @param visitor
   *    Visitor
   * @return
   *    True, if there was no error
   */
  bool LocationService::VisitAdminRegionPOIs(const AdminRegion& region,
                                             POIVisitor& visitor) const
  {
    LocationIndexRef locationIndex=database->GetLocationIndex();

    if (!locationIndex) {
      return false;
    }

    return locationIndex->VisitPOIs(region,
                                    visitor);
  }

  /**
   * Visit all addresses at the given location
   * @param region
   *    Region the location belongs to
   * @param location
   *    The location itself
   * @param visitor
   *    The Visitor
   * @return
   *    True, if there was no error
   */
  bool LocationService::VisitLocationAddresses(const AdminRegion& region,
                                               const PostalArea& postalArea,
                                               const Location& location,
                                               AddressVisitor& visitor) const
  {
    LocationIndexRef locationIndex=database->GetLocationIndex();

    if (!locationIndex) {
      return false;
    }

    return locationIndex->VisitAddresses(region,
                                         postalArea,
                                         location,
                                         visitor);
  }

  /**
   * Resolve all parent regions of the given region (walk the region tree up to the root)
   * .
   * Use AdminRegion::regionOffset and AdminRegion::parentRegionOffset to build the tree.
   *
   * @param adminRegion
   *    The region to start at
   * @param refs
   *    A map of all parent regions
   * @return
   *    True, if there was no error
   */
  bool LocationService::ResolveAdminRegionHierachie(const AdminRegionRef& adminRegion,
                                                    std::map<FileOffset,AdminRegionRef >& refs) const
  {
    LocationIndexRef locationIndex=database->GetLocationIndex();

    if (!locationIndex) {
      return false;
    }

    return locationIndex->ResolveAdminRegionHierachie(adminRegion,
                                                      refs);
  }

  bool LocationService::HandleAdminRegion(const LocationSearch& search,
                                          const LocationSearch::Entry& searchEntry,
                                          const AdminRegionMatchVisitor::AdminRegionResult& adminRegionResult,
                                          LocationSearchResult& result) const
  {
    if (searchEntry.locationPattern.empty()) {
      LocationSearchResult::Entry entry;

      entry.adminRegion=adminRegionResult.adminRegion;

      if (adminRegionResult.isMatch) {
        entry.adminRegionMatchQuality=LocationSearchResult::match;
      }
      else {
        entry.adminRegionMatchQuality=LocationSearchResult::candidate;
      }

      entry.postalAreaMatchQuality=LocationSearchResult::none;
      entry.locationMatchQuality=LocationSearchResult::none;
      entry.poiMatchQuality=LocationSearchResult::none;
      entry.addressMatchQuality=LocationSearchResult::none;

      result.results.push_back(entry);

      return true;
    }

    POIMatchVisitor poiVisitor(adminRegionResult.adminRegion,
                               searchEntry.locationPattern,
                               search.limit>=result.results.size() ? search.limit-result.results.size() : 0);


    if (!VisitAdminRegionPOIs(*adminRegionResult.adminRegion,
                              poiVisitor)) {
      log.Error() << "Error during traversal of region location list";
      return false;
    }

    for (const auto& poiResult : poiVisitor.results) {
      if (!HandleAdminRegionPOI(search,
                                adminRegionResult,
                                poiResult,
                                result)) {
        log.Error() << "Error during traversal of region poi list";
        return false;
      }
    }

    PostalAreaMatchVisitor postalAreaVisitor(adminRegionResult.adminRegion,
                                             searchEntry.postalCodePattern,
                                             search.limit>=result.results.size() ? search.limit-result.results.size() : 0);

    for (const auto& postalArea : adminRegionResult.adminRegion->postalAreas) {
      if (!postalAreaVisitor.Visit(postalArea)) {
        log.Error() << "Error during traversal of region postal area list";
        break;
      }
    }

    //std::cout << "  Search for location '" << searchEntry.locationPattern << "'" << " in " << adminRegionResult.adminRegion->name << "/" << adminRegionResult.adminRegion->aliasName << std::endl;

    for (const auto& postalAreaResult : postalAreaVisitor.results) {
      LocationMatchVisitor locationVisitor(searchEntry.locationPattern,
                                           search.limit>=result.results.size() ? search.limit-result.results.size() : 0);

      if (!VisitAdminRegionLocations(*postalAreaResult.adminRegion,
                                     *postalAreaResult.postalArea,
                                     locationVisitor)) {
        log.Error() << "Error during traversal of region location list";
        return false;
      }

      for (const auto& locationResult : locationVisitor.results) {
        //std::cout << "  - '" << locationResult->location->name << "'" << std::endl;
        if (!HandleAdminRegionLocation(search,
                                       searchEntry,
                                       adminRegionResult,
                                       postalAreaResult,
                                       locationResult,
                                       result)) {
          log.Error() << "Error during traversal of region location list";
          return false;
        }
      }
    }

    return true;
  }

  bool LocationService::HandleAdminRegionLocation(const LocationSearch& search,
                                                  const LocationSearch::Entry& searchEntry,
                                                  const AdminRegionMatchVisitor::AdminRegionResult& adminRegionResult,
                                                  const PostalAreaMatchVisitor::PostalAreaResult& postalAreaResult,
                                                  const LocationMatchVisitor::Result& locationResult,
                                                  LocationSearchResult& result) const
  {
    if (searchEntry.addressPattern.empty()) {
      LocationSearchResult::Entry entry;

      entry.adminRegion=locationResult.adminRegion;
      entry.postalArea=locationResult.postalArea;
      entry.location=locationResult.location;

      if (adminRegionResult.isMatch) {
        entry.adminRegionMatchQuality=LocationSearchResult::match;
      }
      else {
        entry.adminRegionMatchQuality=LocationSearchResult::candidate;
      }

      if (postalAreaResult.isMatch) {
        entry.postalAreaMatchQuality=LocationSearchResult::match;
      }
      else {
        entry.postalAreaMatchQuality=LocationSearchResult::candidate;
      }

      if (locationResult.isMatch) {
        entry.locationMatchQuality=LocationSearchResult::match;
      }
      else {
        entry.locationMatchQuality=LocationSearchResult::candidate;
      }

      entry.poiMatchQuality=LocationSearchResult::none;
      entry.addressMatchQuality=LocationSearchResult::none;

      result.results.push_back(entry);

      return true;
    }

    //std::cout << "    Search for address '" << searchEntry.addressPattern << "'" << std::endl;

    AddressMatchVisitor visitor(searchEntry.addressPattern,
                                search.limit>=result.results.size() ? search.limit-result.results.size() : 0);


    if (!VisitLocationAddresses(*locationResult.adminRegion,
                                *locationResult.postalArea,
                                *locationResult.location,
                                visitor)) {
      log.Error() << "Error during traversal of region location address list";
      return false;
    }

    if (visitor.results.empty()) {
      LocationSearchResult::Entry entry;

      entry.adminRegion=locationResult.adminRegion;
      entry.postalArea=locationResult.postalArea;
      entry.location=locationResult.location;

      if (adminRegionResult.isMatch) {
        entry.adminRegionMatchQuality=LocationSearchResult::match;
      }
      else {
        entry.adminRegionMatchQuality=LocationSearchResult::candidate;
      }

      if (postalAreaResult.isMatch) {
        entry.postalAreaMatchQuality=LocationSearchResult::match;
      }
      else {
        entry.postalAreaMatchQuality=LocationSearchResult::candidate;
      }

      if (locationResult.isMatch) {
        entry.locationMatchQuality=LocationSearchResult::match;
      }
      else {
        entry.locationMatchQuality=LocationSearchResult::candidate;
      }

      entry.poiMatchQuality=LocationSearchResult::none;
      entry.addressMatchQuality=LocationSearchResult::none;

      result.results.push_back(entry);

      return true;
    }

    for (const auto& addressResult : visitor.results) {
      //std::cout << "    - '" << addressResult->address->name << "'" << std::endl;
      if (!HandleAdminRegionLocationAddress(search,
                                            adminRegionResult,
                                            postalAreaResult,
                                            locationResult,
                                            addressResult,
                                            result)) {
        return false;
      }
    }

    return true;
  }

  bool LocationService::HandleAdminRegionPOI(const LocationSearch& /*search*/,
                                             const AdminRegionMatchVisitor::AdminRegionResult& adminRegionResult,
                                             const POIMatchVisitor::Result& poiResult,
                                             LocationSearchResult& result) const
  {
    LocationSearchResult::Entry entry;

    entry.adminRegion=adminRegionResult.adminRegion;
    entry.poi=poiResult.poi;

    if (adminRegionResult.isMatch) {
      entry.adminRegionMatchQuality=LocationSearchResult::match;
    }
    else {
      entry.adminRegionMatchQuality=LocationSearchResult::candidate;
    }

    entry.postalAreaMatchQuality=LocationSearchResult::none;

    if (poiResult.isMatch) {
      entry.poiMatchQuality=LocationSearchResult::match;
    }
    else {
      entry.poiMatchQuality=LocationSearchResult::candidate;
    }

    entry.locationMatchQuality=LocationSearchResult::none;
    entry.addressMatchQuality=LocationSearchResult::none;

    result.results.push_back(entry);

    return true;
  }

  bool LocationService::HandleAdminRegionLocationAddress(const LocationSearch& /*search*/,
                                                         const AdminRegionMatchVisitor::AdminRegionResult& adminRegionResult,
                                                         const PostalAreaMatchVisitor::PostalAreaResult& postalAreaResult,
                                                         const LocationMatchVisitor::Result& locationResult,
                                                         const AddressMatchVisitor::AddressResult& addressResult,
                                                         LocationSearchResult& result) const
  {
    LocationSearchResult::Entry entry;

    entry.adminRegion=locationResult.adminRegion;
    entry.postalArea=locationResult.postalArea;
    entry.location=addressResult.location;
    entry.address=addressResult.address;

    if (adminRegionResult.isMatch) {
      entry.adminRegionMatchQuality=LocationSearchResult::match;
    }
    else {
      entry.adminRegionMatchQuality=LocationSearchResult::candidate;
    }

    if (postalAreaResult.isMatch) {
      entry.postalAreaMatchQuality=LocationSearchResult::match;
    }
    else {
      entry.postalAreaMatchQuality=LocationSearchResult::candidate;
    }

    if (locationResult.isMatch) {
      entry.locationMatchQuality=LocationSearchResult::match;
    }
    else {
      entry.locationMatchQuality=LocationSearchResult::candidate;
    }

    entry.poiMatchQuality=LocationSearchResult::none;

    if (addressResult.isMatch) {
      entry.addressMatchQuality=LocationSearchResult::match;
    }
    else {
      entry.addressMatchQuality=LocationSearchResult::candidate;
    }

    result.results.push_back(entry);

    return true;
  }

  /**
   * This takes the given pattern, splits it into tokens,
   * and generates a number of search entries based on the idea
   * that the input follows one of the following patterns:
   * - AdminRegion Location Address
   * - Location Address AdminRegion
   * - AdminRegion Location
   * - Location AdminRegion
   * - AdminRegion
   */
  bool LocationService::InitializeLocationSearchEntries(const std::string& searchPattern,
                                                        LocationSearch& locationSearch)
  {
    LocationIndexRef locationIndex=database->GetLocationIndex();

    if (!locationIndex) {
      return false;
    }

    std::list<std::string> tokens;

    locationSearch.searches.clear();

    if (searchPattern.empty()) {
      return true;
    }

    TokenizeString(searchPattern,
                   tokens);

    if (tokens.empty()) {
      return true;
    }

    SimplifyTokenList(tokens);

    if (tokens.size()>=3) {
      std::list<std::list<std::string> > slices;

      GroupStringListToStrings(tokens.begin(),
                               tokens.size(),
                               3,
                               slices);

      for (const auto& slice : slices) {
        std::list<std::string>::const_iterator text1;
        std::list<std::string>::const_iterator text2;
        std::list<std::string>::const_iterator text3;

        text1=slice.begin();
        text2=text1;
        ++text2;
        text3=text2;
        ++text3;

        osmscout::LocationSearch::Entry search;

        search.locationPattern=*text1;
        search.addressPattern=*text2;
        search.adminRegionPattern=*text3;

        if (!locationIndex->IsLocationIgnoreToken(search.locationPattern) &&
            !locationIndex->IsRegionIgnoreToken(search.adminRegionPattern)) {
          locationSearch.searches.push_back(search);
        }

        search.locationPattern=*text2;
        search.addressPattern=*text3;
        search.adminRegionPattern=*text1;

        if (!locationIndex->IsLocationIgnoreToken(search.locationPattern) &&
            !locationIndex->IsRegionIgnoreToken(search.adminRegionPattern)) {
          locationSearch.searches.push_back(search);
        }
      }
    }

    if (tokens.size()>=2) {
      std::list<std::list<std::string> > slices;

      GroupStringListToStrings(tokens.begin(),
                               tokens.size(),
                               2,
                               slices);

      for (std::list< std::list<std::string> >::const_iterator slice=slices.begin();
          slice!=slices.end();
          ++slice) {
        std::list<std::string>::const_iterator text1;
        std::list<std::string>::const_iterator text2;

        text1=slice->begin();
        text2=text1;
        text2++;

        osmscout::LocationSearch::Entry search;

        search.locationPattern=*text1;
        search.adminRegionPattern=*text2;

        if (!locationIndex->IsLocationIgnoreToken(search.locationPattern) &&
            !locationIndex->IsRegionIgnoreToken(search.adminRegionPattern)) {
          locationSearch.searches.push_back(search);
        }

        search.locationPattern=*text2;
        search.adminRegionPattern=*text1;

        if (!locationIndex->IsLocationIgnoreToken(search.locationPattern) &&
            !locationIndex->IsRegionIgnoreToken(search.adminRegionPattern)) {
          locationSearch.searches.push_back(search);
        }
      }
    }

    if (!tokens.empty()) {
      std::list<std::list<std::string> > slices;

      GroupStringListToStrings(tokens.begin(),
                               tokens.size(),
                               1,
                               slices);

      for (std::list< std::list<std::string> >::const_iterator slice=slices.begin();
          slice!=slices.end();
          ++slice) {
        auto text1=slice->begin();

        osmscout::LocationSearch::Entry search;

        search.adminRegionPattern=*text1;

        if (!locationIndex->IsRegionIgnoreToken(search.adminRegionPattern)) {
          locationSearch.searches.push_back(search);
        }
      }
    }

    return true;
  }

  struct TokenString
  {
    size_t      start;
    size_t      length;
    std::string text;

    TokenString(size_t start,
                size_t length,
                const std::string& text)
    : start(start),
      length(length),
      text(text)
    {
      // no code
    }

    explicit TokenString(const std::string& text)
      : start(0),
        length(text.length()),
        text(text)
    {
      // no code
    }
  };

  typedef std::shared_ptr<TokenString> TokenStringRef;

  struct TokenSearch
  {
    TokenStringRef   tokenString;
    StringMatcherRef matcher;

    TokenSearch(const TokenStringRef& tokenString,
               const StringMatcherRef& matcher)
    : tokenString(tokenString),
      matcher(matcher)
    {
      // no code
    }
  };


  class AdminRegionSearchVisitor : public AdminRegionVisitor
  {
  public:
    struct Result
    {
      TokenStringRef tokenString;
      AdminRegionRef adminRegion;
      std::string    name;

      Result(const TokenStringRef& tokenString,
             const AdminRegionRef& adminRegion,
             const std::string& name)
        : tokenString(tokenString),
          adminRegion(adminRegion),
          name(name)
      {
        // no code
      }
    };

  public:
    std::list<TokenSearch> patterns;
    std::list<Result>      matches;
    std::list<Result>      partialMatches;

  public:
    AdminRegionSearchVisitor(const StringMatcherFactoryRef& matcherFactory,
                             const std::list<TokenStringRef>& patterns)
    {
      for (const auto& pattern : patterns) {
        this->patterns.push_back(TokenSearch(pattern,matcherFactory->CreateMatcher(pattern->text)));
      }
    }

    Action Visit(const AdminRegion& region) override
    {
      for (const auto& pattern : patterns) {
        StringMatcher::Result matchResult=pattern.matcher->Match(region.name);

        if (matchResult==StringMatcher::match) {
          //std::cout << "Match region name '" << region.name << "'" << std::endl;
          matches.push_back(Result(pattern.tokenString,
                                   std::make_shared<AdminRegion>(region),
                                   region.name));

        }
        else if (matchResult==StringMatcher::partialMatch) {
          //std::cout << "Partial match region name '" << region.name << "'" << std::endl;
          partialMatches.push_back(Result(pattern.tokenString,
                                          std::make_shared<AdminRegion>(region),
                                          region.name));
        }

        if (matchResult!=StringMatcher::match) {
          for (const auto& alias : region.aliases) {
            matchResult=pattern.matcher->Match(alias.name);

            if (matchResult==StringMatcher::match) {
              //std::cout << "Match region alias '" << region.name << "' '" << alias.name << "'" << std::endl;
              partialMatches.push_back(Result(pattern.tokenString,
                                              std::make_shared<AdminRegion>(region),
                                             alias.name));
              break;
            }
            else if (matchResult==StringMatcher::partialMatch) {
              //std::cout << "Partial match region alias '" << region.name << "' '" << alias.name << "'" << std::endl;
              partialMatches.push_back(Result(pattern.tokenString,
                                              std::make_shared<AdminRegion>(region),
                                              alias.name));
            }
          }
        }
      }

      return visitChildren;
    }
  };

  class POISearchVisitor : public POIVisitor
  {
  public:
    struct Result
    {
      const TokenStringRef tokenString;
      const AdminRegionRef adminRegion;
      const POIRef         poi;

      Result(const TokenStringRef& tokenString,
             const AdminRegionRef& adminRegion,
             const POIRef& poi)
        : tokenString(tokenString),
          adminRegion(adminRegion),
          poi(poi)
      {
        // no code
      }
    };

  public:
    std::list<TokenSearch> patterns;
    std::list<Result>      matches;
    std::list<Result>      partialMatches;

  public:
    POISearchVisitor(const StringMatcherFactoryRef& matcherFactory,
                     const std::list<TokenStringRef>& patterns)
    {
      for (const auto& pattern : patterns) {
        this->patterns.push_back(TokenSearch(pattern,matcherFactory->CreateMatcher(pattern->text)));
      }
    }

    bool Visit(const AdminRegion& adminRegion,
               const POI& poi) override
    {
      for (const auto& pattern : patterns) {
        StringMatcher::Result matchResult=pattern.matcher->Match(poi.name);

        if (matchResult==StringMatcher::match) {
          matches.push_back(Result(pattern.tokenString,
                                   std::make_shared<AdminRegion>(adminRegion),
                                   std::make_shared<POI>(poi)));
        }
        else if (matchResult==StringMatcher::partialMatch) {
          partialMatches.push_back(Result(pattern.tokenString,
                                          std::make_shared<AdminRegion>(adminRegion),
                                          std::make_shared<POI>(poi)));
        }
      }

      return true;
    }
  };

  class LocationSearchVisitor : public LocationVisitor
  {
  public:
    struct Result
    {
      const TokenStringRef tokenString;
      const AdminRegionRef adminRegion;
      const PostalAreaRef  postalArea;
      const LocationRef    location;

      Result(const TokenStringRef& tokenString,
             const AdminRegionRef& adminRegion,
             const PostalAreaRef& postalArea,
             const LocationRef& location)
        : tokenString(tokenString),
          adminRegion(adminRegion),
          postalArea(postalArea),
          location(location)
      {
        // no code
      }
    };

  public:
    std::list<TokenSearch> patterns;
    std::list<Result>      matches;
    std::list<Result>      partialMatches;

  public:
    LocationSearchVisitor(const StringMatcherFactoryRef& matcherFactory,
                          const std::list<TokenStringRef>& patterns)
    {
      for (const auto& pattern : patterns) {
        this->patterns.push_back(TokenSearch(pattern,matcherFactory->CreateMatcher(pattern->text)));
      }
    }

    bool Visit(const AdminRegion& adminRegion,
               const PostalArea& postalArea,
               const Location& location) override
    {
      for (const auto& pattern : patterns) {
        StringMatcher::Result matchResult=pattern.matcher->Match(location.name);

        if (matchResult==StringMatcher::match) {
          //std::cout << "Match location name '" << location.name << "'" << std::endl;
          matches.push_back(Result(pattern.tokenString,
                                   std::make_shared<AdminRegion>(adminRegion),
                                   std::make_shared<PostalArea>(postalArea),
                                   std::make_shared<Location>(location)));
        }
        else if (matchResult==StringMatcher::partialMatch) {
          //std::cout << "Partial match location name '" << location.name << "'" << std::endl;
          partialMatches.push_back(Result(pattern.tokenString,
                                          std::make_shared<AdminRegion>(adminRegion),
                                          std::make_shared<PostalArea>(postalArea),
                                          std::make_shared<Location>(location)));
        }
      }

      return true;
    }
  };

  class AddressSearchVisitor : public AddressVisitor
  {
  public:
    struct Result
    {
      const TokenStringRef tokenString;
      const AdminRegionRef adminRegion;
      const PostalAreaRef  postalArea;
      const LocationRef    location;
      const AddressRef     address;

      Result(const TokenStringRef& tokenString,
             const AdminRegionRef& adminRegion,
             const PostalAreaRef& postalArea,
             const LocationRef& location,
             const AddressRef& address)
        : tokenString(tokenString),
          adminRegion(adminRegion),
          postalArea(postalArea),
          location(location),
          address(address)
      {
        // no code
      }
    };

  public:
    std::list<TokenSearch> patterns;
    std::list<Result>      matches;
    std::list<Result>      partialMatches;

  public:
    AddressSearchVisitor(const StringMatcherFactoryRef& matcherFactory,
                         const std::list<TokenStringRef>& patterns)
    {
      for (const auto& pattern : patterns) {
        this->patterns.push_back(TokenSearch(pattern,matcherFactory->CreateMatcher(pattern->text)));
      }
    }

    bool Visit(const AdminRegion& adminRegion,
               const PostalArea& postalArea,
               const Location& location,
               const Address& address) override
    {
      for (const auto& pattern : patterns) {
        StringMatcher::Result matchResult=pattern.matcher->Match(address.name);

        if (matchResult==StringMatcher::match) {
          //std::cout << "Match region name '" << region.name << "'" << std::endl;
          matches.push_back(Result(pattern.tokenString,
                                   std::make_shared<AdminRegion>(adminRegion),
                                   std::make_shared<PostalArea>(postalArea),
                                   std::make_shared<Location>(location),
                                   std::make_shared<Address>(address)));
        }
        else if (matchResult==StringMatcher::partialMatch) {
          //std::cout << "Partial match region name '" << region.name << "'" << std::endl;
          partialMatches.push_back(Result(pattern.tokenString,
                                          std::make_shared<AdminRegion>(adminRegion),
                                          std::make_shared<PostalArea>(postalArea),
                                          std::make_shared<Location>(location),
                                          std::make_shared<Address>(address)));
        }
      }

      return true;
    }
  };

  /**
   * Return a list of token by removing tokenString from the given token list (tokens).
   * @param tokenString
   *    Token to remove
   * @param tokens
   *    List to rmeove token parameter from
   * @return
   *    New list
   */
  static std::list<std::string> BuildStringListFromSubToken(const TokenStringRef& tokenString,
                                                            const std::list<std::string>& tokens)
  {
    std::list<std::string> result;

    if (tokenString->start==0) {
      auto tokenStartIter=tokens.begin();

      std::advance(tokenStartIter,tokenString->length);

      result.insert(result.begin(),tokenStartIter,tokens.end());
    }
    else {
      auto tokenEndIter=tokens.begin();

      std::advance(tokenEndIter,tokenString->start);

      result.insert(result.begin(),tokens.begin(),tokenEndIter);
    }

    return result;
  }

  static void CleanupSearchPatterns(std::list<TokenStringRef>& patterns)
  {
    patterns.sort([](const TokenStringRef& a, const TokenStringRef& b) {
      return a->text.length()>b->text.length();
    });

    patterns.unique([](const TokenStringRef& a, const TokenStringRef& b) {
      return a->text==b->text;
    });
  }

  static std::list<TokenStringRef> GenerateSearchPatterns(const std::list<std::string>& tokens,
                                                          const std::unordered_set<std::string>& patternExclusions,
                                                          size_t maxWords)
  {
    std::list<TokenStringRef> patterns;

    for (size_t i=1; i<=std::min(tokens.size(),maxWords); i++) {
      std::string searchExpression=GetTokensFromStart(tokens,i);

      if (patternExclusions.find(UTF8StringToUpper(searchExpression))!=patternExclusions.end()) {
        continue;
      }

      patterns.push_back(std::make_shared<TokenString>(0,i,searchExpression));
    }

    for (size_t i=1; i<=std::min(tokens.size(),(size_t) maxWords); i++) {
      std::string searchExpression=GetTokensFromEnd(tokens,i);

      if (patternExclusions.find(UTF8StringToUpper(searchExpression))!=patternExclusions.end()) {
        continue;
      }

      patterns.push_back(std::make_shared<TokenString>(tokens.size()-i,i,searchExpression));
    }

    return patterns;
  }

  static void AddRegionResult(const SearchParameter& parameter,
                              LocationSearchResult::MatchQuality regionMatchQuality,
                              const AdminRegionSearchVisitor::Result& regionMatch,
                              LocationSearchResult& result)
  {
    if (result.results.size()>parameter.limit) {
      result.limitReached=true;
    }
    else {
      LocationSearchResult::Entry entry;

      entry.adminRegion=regionMatch.adminRegion;
      entry.adminRegionMatchQuality=regionMatchQuality;
      entry.poiMatchQuality=LocationSearchResult::none;
      entry.postalAreaMatchQuality=LocationSearchResult::none;
      entry.locationMatchQuality=LocationSearchResult::none;
      entry.addressMatchQuality=LocationSearchResult::none;

      result.results.push_back(entry);
      result.results.sort();
      result.results.unique();
    }
  }

  static void AddPOIResult(const SearchParameter& parameter,
                           LocationSearchResult::MatchQuality regionMatchQuality,
                           const POISearchVisitor::Result& poiMatch,
                           LocationSearchResult::MatchQuality poiMatchQuality,
                           LocationSearchResult& result)
  {
    if (result.results.size()>parameter.limit) {
      result.limitReached=true;
    }
    else {
      LocationSearchResult::Entry entry;

      entry.adminRegion=poiMatch.adminRegion;
      entry.adminRegionMatchQuality=regionMatchQuality;
      entry.poi=poiMatch.poi;
      entry.poiMatchQuality=poiMatchQuality;
      entry.postalAreaMatchQuality=LocationSearchResult::none;
      entry.locationMatchQuality=LocationSearchResult::none;
      entry.addressMatchQuality=LocationSearchResult::none;

      result.results.push_back(entry);
      result.results.sort();
      result.results.unique();
    }
  }

  static void AddLocationResult(const SearchParameter& parameter,
                                LocationSearchResult::MatchQuality regionMatchQuality,
                                const LocationSearchVisitor::Result& locationMatch,
                                LocationSearchResult::MatchQuality locationMatchQuality,
                                LocationSearchResult& result)
  {
    if (result.results.size()>parameter.limit) {
      result.limitReached=true;
    }
    else {
      LocationSearchResult::Entry entry;

      //std::cout << "Add location: " << locationMatch.location->name << " " << locationMatch.postalArea->name << " " << locationMatch.adminRegion->name << std::endl;

      entry.adminRegion=locationMatch.adminRegion;
      entry.adminRegionMatchQuality=regionMatchQuality;
      entry.poiMatchQuality=LocationSearchResult::none;
      entry.postalArea=locationMatch.postalArea;
      entry.postalAreaMatchQuality=LocationSearchResult::none;
      entry.location=locationMatch.location;
      entry.locationMatchQuality=locationMatchQuality;
      entry.addressMatchQuality=LocationSearchResult::none;

      result.results.push_back(entry);
      result.results.sort();
      result.results.unique();
    }
  }

  static void AddAddressResult(const SearchParameter& parameter,
                               LocationSearchResult::MatchQuality regionMatchQuality,
                               LocationSearchResult::MatchQuality locationMatchQuality,
                               const AddressSearchVisitor::Result& addressMatch,
                               LocationSearchResult::MatchQuality addressMatchQuality,
                               LocationSearchResult& result)
  {
    if (result.results.size()>parameter.limit) {
      result.limitReached=true;
    }
    else {
      LocationSearchResult::Entry entry;

      entry.adminRegion=addressMatch.adminRegion;
      entry.adminRegionMatchQuality=regionMatchQuality;
      entry.poiMatchQuality=LocationSearchResult::none;
      entry.postalArea=addressMatch.postalArea;
      entry.postalAreaMatchQuality=LocationSearchResult::none;
      entry.location=addressMatch.location;
      entry.locationMatchQuality=locationMatchQuality;
      entry.address=addressMatch.address;
      entry.addressMatchQuality=addressMatchQuality;

      result.results.push_back(entry);
      result.results.sort();
      result.results.unique();
    }
  }

  static bool SearchForAddressForLocation(LocationIndexRef& locationIndex,
                                          const SearchParameter& parameter,
                                          const std::list<std::string>& addressTokens,
                                          const LocationSearchVisitor::Result& locationMatch,
                                          LocationSearchResult::MatchQuality regionMatchQuality,
                                          LocationSearchResult::MatchQuality locationMatchQuality,
                                          LocationSearchResult& result)
  {
    // Build address search patterns

    std::unordered_set<std::string> addressPatternExclusions; // Currently none

    std::list<TokenStringRef> addressSearchPatterns=GenerateSearchPatterns(addressTokens,
                                                                           addressPatternExclusions,
                                                                           locationIndex->GetAddressMaxWords());

    CleanupSearchPatterns(addressSearchPatterns);

    AddressSearchVisitor addressVisitor(parameter.stringMatcherFactory,
                                        addressSearchPatterns);

    StopClock addressVisitTime;

    if (!locationIndex->VisitAddresses(*locationMatch.adminRegion,
                                       *locationMatch.postalArea,
                                       *locationMatch.location,
                                       addressVisitor)) {
      return false;
    }

    addressVisitTime.Stop();

    //std::cout << "Address visit time: " << addressVisitTime.ResultString() << std::endl;

    for (const auto& addressMatch : addressVisitor.matches) {
      //std::cout << "Found address match '" << addressMatch.address->name << "' for pattern '" << addressMatch.tokenString->text << "'" << std::endl;
      std::list<std::string> restTokens=BuildStringListFromSubToken(addressMatch.tokenString,
                                                                    addressTokens);

      if (restTokens.empty()) {
        AddAddressResult(parameter,
                         regionMatchQuality,
                         locationMatchQuality,
                         addressMatch,
                         LocationSearchResult::match,
                         result);
      }
    }

    if (!parameter.addressOnlyMatch) {
      for (const auto& addressMatch : addressVisitor.partialMatches) {
        //std::cout << "Found address candidate '" << addressMatch.address->name << "' for pattern '" << addressMatch.tokenString->text << "'" << std::endl;
        std::list<std::string> restTokens=BuildStringListFromSubToken(addressMatch.tokenString,
                                                                      addressTokens);

        if (restTokens.empty()) {
          AddAddressResult(parameter,
                           regionMatchQuality,
                           locationMatchQuality,
                           addressMatch,
                           LocationSearchResult::candidate,
                           result);
        }
      }
    }

    return true;
  }

  static bool SearchForLocationForRegion(LocationIndexRef& locationIndex,
                                         const SearchParameter& parameter,
                                         const std::list<std::string>& locationTokens,
                                         const AdminRegionSearchVisitor::Result& regionMatch,
                                         LocationSearchResult::MatchQuality regionMatchQuality,
                                         LocationSearchResult& result)
  {
    std::unordered_set<std::string> locationIgnoreTokenSet;

    for (const auto& token : locationIndex->GetLocationIgnoreTokens()) {
      locationIgnoreTokenSet.insert(UTF8StringToUpper(token));
    }

    // Build Location search patterns

    std::list<TokenStringRef> locationSearchPatterns=GenerateSearchPatterns(locationTokens,
                                                                            locationIgnoreTokenSet,
                                                                            locationIndex->GetLocationMaxWords());

    CleanupSearchPatterns(locationSearchPatterns);

    // Search for locations

    LocationSearchVisitor locationVisitor(parameter.stringMatcherFactory,
                                          locationSearchPatterns);

    StopClock locationVisitTime;

    if (!locationIndex->VisitLocations(*regionMatch.adminRegion,
                                       locationVisitor)) {
      return false;
    }

    locationVisitTime.Stop();

    //std::cout << "Location (" << regionMatch.adminRegion->name << ") visit time: " << locationVisitTime.ResultString() << std::endl;

    for (const auto& locationMatch : locationVisitor.matches) {
      //std::cout << "Found location match '" << locationMatch.location->name << "' for pattern '" << locationMatch.tokenString->text << "'" << std::endl;
      std::list<std::string> addressTokens=BuildStringListFromSubToken(locationMatch.tokenString,
                                                                       locationTokens);

      if (addressTokens.empty()) {
        AddLocationResult(parameter,
                          regionMatchQuality,
                          locationMatch,
                          LocationSearchResult::match,
                          result);
      }
      else {
        size_t currentResultSize=result.results.size();

        SearchForAddressForLocation(locationIndex,
                                    parameter,
                                    addressTokens,
                                    locationMatch,
                                    regionMatchQuality,
                                    LocationSearchResult::match,
                                    result);

        if (result.results.size()==currentResultSize) {
          // If we have not found any result for the given search entry, we create one for the "upper" object
          // so that partial results are not lost
          AddLocationResult(parameter,
                            regionMatchQuality,
                            locationMatch,
                            LocationSearchResult::match,
                            result);
        }
      }
    }

    if (!parameter.locationOnlyMatch) {
      for (const auto& locationMatch : locationVisitor.partialMatches) {
        //std::cout << "Found location candidate '" << locationMatch.location->name << "' for pattern '" << locationMatch.tokenString->text << "'" << std::endl;
        std::list<std::string> addressTokens=BuildStringListFromSubToken(locationMatch.tokenString,
                                                                         locationTokens);

        if (addressTokens.empty()) {
          AddLocationResult(parameter,
                            regionMatchQuality,
                            locationMatch,
                            LocationSearchResult::candidate,
                            result);
        }
        else {
          size_t currentResultSize=result.results.size();

          SearchForAddressForLocation(locationIndex,
                                      parameter,
                                      addressTokens,
                                      locationMatch,
                                      regionMatchQuality,
                                      LocationSearchResult::candidate,
                                      result);

          if (result.results.size()==currentResultSize) {
            // If we have not found any result for the given search entry, we create one for the "upper" object
            // so that partial results are not lost
            AddLocationResult(parameter,
                              regionMatchQuality,
                              locationMatch,
                              LocationSearchResult::candidate,
                              result);
          }
        }
      }
    }

    return true;
  }

  static bool SearchForLocationForRegion(LocationIndexRef& locationIndex,
                                         const SearchParameter& parameter,
                                         const std::string& locationPattern,
                                         const std::string& addressPattern,
                                         const AdminRegionSearchVisitor::Result& regionMatch,
                                         LocationSearchResult::MatchQuality regionMatchQuality,
                                         LocationSearchResult& result)
  {
    std::unordered_set<std::string> locationIgnoreTokenSet;

    for (const auto& token : locationIndex->GetLocationIgnoreTokens()) {
      locationIgnoreTokenSet.insert(UTF8StringToUpper(token));
    }

    // Build Location search patterns

    std::list<TokenStringRef> locationSearchPatterns;

    locationSearchPatterns.push_back(std::make_shared<TokenString>(locationPattern));

    // Search for locations

    LocationSearchVisitor locationVisitor(parameter.stringMatcherFactory,
                                          locationSearchPatterns);

    for (const auto& postalArea : regionMatch.adminRegion->postalAreas) {
      if (!locationIndex->VisitLocations(*regionMatch.adminRegion,
                                         postalArea,
                                         locationVisitor)) {
        return false;
      }
    }

    for (const auto& locationMatch : locationVisitor.matches) {
      //std::cout << "Found location match '" << locationMatch.location->name << "' for pattern '" << locationMatch.tokenString->text << "'" << std::endl;
      if (addressPattern.empty()) {
        AddLocationResult(parameter,
                          regionMatchQuality,
                          locationMatch,
                          LocationSearchResult::match,
                          result);
      }
      else {
        std::list<std::string> addressTokens;
        size_t                 currentResultSize=result.results.size();

        addressTokens.push_back(addressPattern);

        SearchForAddressForLocation(locationIndex,
                                    parameter,
                                    addressTokens,
                                    locationMatch,
                                    regionMatchQuality,
                                    LocationSearchResult::match,
                                    result);

        if (result.results.size()==currentResultSize) {
          // If we have not found any result for the given search entry, we create one for the "upper" object
          // so that partial results are not lost
          AddLocationResult(parameter,
                            regionMatchQuality,
                            locationMatch,
                            LocationSearchResult::match,
                            result);
        }
      }
    }

    if (!parameter.locationOnlyMatch) {
      for (const auto& locationMatch : locationVisitor.partialMatches) {
        //std::cout << "Found location candidate '" << locationMatch.location->name << "' for pattern '" << locationMatch.tokenString->text << "'" << std::endl;
        if (addressPattern.empty()) {
          AddLocationResult(parameter,
                            regionMatchQuality,
                            locationMatch,
                            LocationSearchResult::candidate,
                            result);
        }
        else {
          std::list<std::string> addressTokens;
          size_t                 currentResultSize=result.results.size();

          addressTokens.push_back(addressPattern);

          SearchForAddressForLocation(locationIndex,
                                      parameter,
                                      addressTokens,
                                      locationMatch,
                                      regionMatchQuality,
                                      LocationSearchResult::candidate,
                                      result);

          if (result.results.size()==currentResultSize) {
            // If we have not found any result for the given search entry, we create one for the "upper" object
            // so that partial results are not lost
            AddLocationResult(parameter,
                              regionMatchQuality,
                              locationMatch,
                              LocationSearchResult::candidate,
                              result);
          }
        }
      }
    }

    return true;
  }

  static bool SearchForPOIForRegion(LocationIndexRef& locationIndex,
                                    const SearchParameter& parameter,
                                    const std::list<std::string>& poiTokens,
                                    const AdminRegionSearchVisitor::Result& regionMatch,
                                    LocationSearchResult::MatchQuality regionMatchQuality,
                                    LocationSearchResult& result)
  {
    std::unordered_set<std::string> poiIgnoreTokenSet;

    for (const auto& token : locationIndex->GetPOIIgnoreTokens()) {
      poiIgnoreTokenSet.insert(UTF8StringToUpper(token));
    }

    // Build Location search patterns

    std::list<TokenStringRef> poiSearchPatterns=GenerateSearchPatterns(poiTokens,
                                                                       poiIgnoreTokenSet,
                                                                       locationIndex->GetPOIMaxWords());

    CleanupSearchPatterns(poiSearchPatterns);

    // Search for locations

    POISearchVisitor poiVisitor(parameter.stringMatcherFactory,
                                poiSearchPatterns);

    if (!locationIndex->VisitPOIs(*regionMatch.adminRegion,
                                  poiVisitor)) {
      return false;
    }

    for (const auto& poiMatch : poiVisitor.matches) {
      //std::cout << "Found poi match '" << poiMatch.poi->name << "' for pattern '" << poiMatch.tokenString->text << "'" << std::endl;
      std::list<std::string> restTokens=BuildStringListFromSubToken(poiMatch.tokenString,
                                                                    poiTokens);

      if (restTokens.empty()) {
        AddPOIResult(parameter,
                     regionMatchQuality,
                     poiMatch,
                     LocationSearchResult::match,
                     result);
      }
    }

    if (!parameter.locationOnlyMatch) {
      for (const auto& poiMatch : poiVisitor.partialMatches) {
        //std::cout << "Found poi candidate '" << poiMatch.poi->name << "' for pattern '" << poiMatch.tokenString->text << "'" << std::endl;
        std::list<std::string> restTokens=BuildStringListFromSubToken(poiMatch.tokenString,
                                                                      poiTokens);

        if (restTokens.empty()) {
          AddPOIResult(parameter,
                       regionMatchQuality,
                       poiMatch,
                       LocationSearchResult::candidate,
                       result);
        }
      }
    }

    return true;
  }

  static bool SearchForPOIForRegion(LocationIndexRef& locationIndex,
                                    const SearchParameter& parameter,
                                    const std::string& poiPattern,
                                    const AdminRegionSearchVisitor::Result& regionMatch,
                                    LocationSearchResult::MatchQuality regionMatchQuality,
                                    LocationSearchResult& result)
  {
    std::unordered_set<std::string> poiIgnoreTokenSet;

    for (const auto& token : locationIndex->GetPOIIgnoreTokens()) {
      poiIgnoreTokenSet.insert(UTF8StringToUpper(token));
    }

    // Build Location search patterns


    std::list<TokenStringRef> poiSearchPatterns;

    poiSearchPatterns.push_back(std::make_shared<TokenString>(poiPattern));

    CleanupSearchPatterns(poiSearchPatterns);

    // Search for locations

    POISearchVisitor poiVisitor(parameter.stringMatcherFactory,
                                poiSearchPatterns);

    if (!locationIndex->VisitPOIs(*regionMatch.adminRegion,
                                  poiVisitor)) {
      return false;
    }

    for (const auto& poiMatch : poiVisitor.matches) {
      AddPOIResult(parameter,
                   regionMatchQuality,
                   poiMatch,
                   LocationSearchResult::match,
                   result);
    }

    if (!parameter.locationOnlyMatch) {
      for (const auto& poiMatch : poiVisitor.partialMatches) {
        AddPOIResult(parameter,
                     regionMatchQuality,
                     poiMatch,
                     LocationSearchResult::candidate,
                     result);
      }
    }

    return true;
  }

  bool LocationService::SearchForLocationByString(const LocationStringSearchParameter& searchParameter,
                                                  LocationSearchResult& result) const
  {
    LocationIndexRef                locationIndex=database->GetLocationIndex();
    std::unordered_set<std::string> regionIgnoreTokenSet;
    AdminRegionRef                  defaultAdminRegion=searchParameter.GetDefaultAdminRegion();
    std::string                     searchPattern=searchParameter.GetSearchString();
    SearchParameter                 parameter;

    parameter.searchForLocation=searchParameter.GetSearchForLocation();
    parameter.searchForPOI=searchParameter.GetSearchForPOI();
    parameter.adminRegionOnlyMatch=searchParameter.GetAdminRegionOnlyMatch();
    parameter.poiOnlyMatch=searchParameter.GetPOIOnlyMatch();
    parameter.locationOnlyMatch=searchParameter.GetLocationOnlyMatch();
    parameter.addressOnlyMatch=searchParameter.GetAddressOnlyMatch();
    parameter.stringMatcherFactory=searchParameter.GetStringMatcherFactory();
    parameter.limit=searchParameter.GetLimit();

    result.limitReached=false;
    result.results.clear();

    if (!locationIndex) {
      return false;
    }

    for (const auto& token : locationIndex->GetRegionIgnoreTokens()) {
      regionIgnoreTokenSet.insert(UTF8StringToUpper(token));
    }

    std::list<std::string> tokens;

    if (searchPattern.empty()) {
      return true;
    }

    TokenizeString(searchPattern,
                   tokens);

    //SimplifyTokenList(tokens);

    if (tokens.empty()) {
      return true;
    }

    if (defaultAdminRegion) {
      std::list<std::string> locationTokens=tokens;
      TokenStringRef         tokenString=std::make_shared<TokenString>(0,defaultAdminRegion->name.length(),defaultAdminRegion->name);

      AdminRegionSearchVisitor::Result regionMatch(tokenString,
                                                   defaultAdminRegion,
                                                   defaultAdminRegion->name);


      if (locationTokens.empty()) {
        AddRegionResult(parameter,
                        LocationSearchResult::match,
                        regionMatch,
                        result);
      }
      else {
        if (parameter.searchForLocation) {
          SearchForLocationForRegion(locationIndex,
                                     parameter,
                                     locationTokens,
                                     regionMatch,
                                     LocationSearchResult::match,
                                     result);
        }

        if (parameter.searchForPOI) {
          SearchForPOIForRegion(locationIndex,
                                parameter,
                                locationTokens,
                                regionMatch,
                                LocationSearchResult::match,
                                result);
        }
      }
    }

    // Build Region search patterns

    std::list<TokenStringRef> regionSearchPatterns=GenerateSearchPatterns(tokens,
                                                                          regionIgnoreTokenSet,
                                                                          locationIndex->GetRegionMaxWords());

    CleanupSearchPatterns(regionSearchPatterns);

    // Search for region name

    AdminRegionSearchVisitor adminRegionVisitor(parameter.stringMatcherFactory,
                                                regionSearchPatterns);

    StopClock adminRegionVisitTime;

    locationIndex->VisitAdminRegions(adminRegionVisitor);

    adminRegionVisitTime.Stop();

    //std::cout << "Admin Region visit: " << adminRegionVisitTime.ResultString() << std::endl;

    for (const auto& regionMatch : adminRegionVisitor.matches) {
      //std::cout << "Found region match '" << regionMatch.adminRegion->name << "' (" << regionMatch.adminRegion->object.GetName() << ") for pattern '" << regionMatch.tokenString->text << "'" << std::endl;
      std::list<std::string> locationTokens=BuildStringListFromSubToken(regionMatch.tokenString,
                                                                        tokens);

      if (locationTokens.empty()) {
        AddRegionResult(parameter,
                        LocationSearchResult::match,
                        regionMatch,
                        result);
      }
      else {
        size_t currentResultSize=result.results.size();

        if (parameter.searchForLocation) {
          SearchForLocationForRegion(locationIndex,
                                     parameter,
                                     locationTokens,
                                     regionMatch,
                                     LocationSearchResult::match,
                                     result);
        }

        if (parameter.searchForPOI) {
          SearchForPOIForRegion(locationIndex,
                                parameter,
                                locationTokens,
                                regionMatch,
                                LocationSearchResult::match,
                                result);
        }

        if (result.results.size()==currentResultSize) {
          // If we have not found any result for the given search entry, we create one for the "upper" object
          // so that partial results are not lost
          AddRegionResult(parameter,
                          LocationSearchResult::match,
                          regionMatch,
                          result);
        }
      }
    }

    if (!parameter.adminRegionOnlyMatch) {
      for (const auto& regionMatch : adminRegionVisitor.partialMatches) {
        //std::cout << "Found region candidate '" << regionMatch.adminRegion->name << "' (" << regionMatch.adminRegion->object.GetName() << ") for pattern '" << regionMatch.tokenString->text << "'" << std::endl;
        std::list<std::string> locationTokens=BuildStringListFromSubToken(regionMatch.tokenString,
                                                                          tokens);

        if (locationTokens.empty()) {
          AddRegionResult(parameter,
                          LocationSearchResult::candidate,
                          regionMatch,
                          result);
        }
        else {
          size_t currentResultSize=result.results.size();

          if (parameter.searchForLocation) {
            SearchForLocationForRegion(locationIndex,
                                       parameter,
                                       locationTokens,
                                       regionMatch,
                                       LocationSearchResult::candidate,
                                       result);
          }

          if (parameter.searchForPOI) {
            SearchForPOIForRegion(locationIndex,
                                  parameter,
                                  locationTokens,
                                  regionMatch,
                                  LocationSearchResult::candidate,
                                  result);
          }

          if (result.results.size()==currentResultSize) {
            // If we have not found any result for the given search entry, we create one for the "upper" object
            // so that partial results are not lost
            AddRegionResult(parameter,
                            LocationSearchResult::candidate,
                            regionMatch,
                            result);
          }
        }
      }
    }

    return true;
  }

  bool LocationService::SearchForLocationByForm(const LocationFormSearchParameter& searchParameter,
                                                LocationSearchResult& result) const
  {
    LocationIndexRef                locationIndex=database->GetLocationIndex();
    std::unordered_set<std::string> regionIgnoreTokenSet;
    SearchParameter                 parameter;

    parameter.searchForLocation=true;
    parameter.searchForPOI=false;
    parameter.adminRegionOnlyMatch=searchParameter.GetAdminRegionOnlyMatch();
    parameter.poiOnlyMatch=false;
    parameter.locationOnlyMatch=searchParameter.GetLocationOnlyMatch();
    parameter.addressOnlyMatch=searchParameter.GetAddressOnlyMatch();
    parameter.stringMatcherFactory=searchParameter.GetStringMatcherFactory();
    parameter.limit=searchParameter.GetLimit();

    result.limitReached=false;
    result.results.clear();

    if (!locationIndex) {
      return false;
    }

    for (const auto& token : locationIndex->GetRegionIgnoreTokens()) {
      regionIgnoreTokenSet.insert(UTF8StringToUpper(token));
    }

    if (searchParameter.GetAdminRegionSearchString().empty()) {
      return true;
    }

    // Build Region search patterns

    std::list<TokenStringRef> regionSearchPatterns;

    regionSearchPatterns.push_back(std::make_shared<TokenString>(searchParameter.GetAdminRegionSearchString()));

    // Search for region name

    AdminRegionSearchVisitor adminRegionVisitor(searchParameter.GetStringMatcherFactory(),
                                                regionSearchPatterns);

    locationIndex->VisitAdminRegions(adminRegionVisitor);

    for (const auto& regionMatch : adminRegionVisitor.matches) {
      //std::cout << "Found region match '" << regionMatch.adminRegion->name << "' for pattern '" << regionMatch.tokenString->text << "'" << std::endl;

      if (searchParameter.GetLocationSearchString().empty()) {
        AddRegionResult(parameter,
                        LocationSearchResult::match,
                        regionMatch,
                        result);
      }
      else {
        size_t currentResultSize=result.results.size();

        SearchForLocationForRegion(locationIndex,
                                   parameter,
                                   searchParameter.GetLocationSearchString(),
                                   searchParameter.GetAddressSearchString(),
                                   regionMatch,
                                   LocationSearchResult::match,
                                   result);

        if (result.results.size()==currentResultSize) {
          // If we have not found any result for the given search entry, we create one for the "upper" object
          // so that partial results are not lost
          AddRegionResult(parameter,
                          LocationSearchResult::match,
                          regionMatch,
                          result);
        }
      }
    }

    for (const auto& regionMatch : adminRegionVisitor.partialMatches) {
      //std::cout << "Found region candidate '" << regionMatch.adminRegion->name << "' for pattern '" << regionMatch.tokenString->text << "'" << std::endl;

      if (searchParameter.GetLocationSearchString().empty()) {
        AddRegionResult(parameter,
                        LocationSearchResult::candidate,
                        regionMatch,
                        result);
      }
      else {
        size_t currentResultSize=result.results.size();

        SearchForLocationForRegion(locationIndex,
                                   parameter,
                                   searchParameter.GetLocationSearchString(),
                                   searchParameter.GetAddressSearchString(),
                                   regionMatch,
                                   LocationSearchResult::candidate,
                                   result);

        if (result.results.size()==currentResultSize) {
          // If we have not found any result for the given search entry, we create one for the "upper" object
          // so that partial results are not lost
          AddRegionResult(parameter,
                          LocationSearchResult::candidate,
                          regionMatch,
                          result);
        }
      }
    }

    result.results.sort();
    result.results.unique();

    return true;
  }

  bool LocationService::SearchForPOIByForm(const POIFormSearchParameter& searchParameter,
                                           LocationSearchResult& result) const
  {
    LocationIndexRef                locationIndex=database->GetLocationIndex();
    std::unordered_set<std::string> regionIgnoreTokenSet;
    SearchParameter                 parameter;

    parameter.searchForLocation=false;
    parameter.searchForPOI=true;
    parameter.adminRegionOnlyMatch=searchParameter.GetAdminRegionOnlyMatch();
    parameter.poiOnlyMatch=searchParameter.GetPOIOnlyMatch();
    parameter.locationOnlyMatch=true;
    parameter.addressOnlyMatch=true;
    parameter.stringMatcherFactory=searchParameter.GetStringMatcherFactory();
    parameter.limit=searchParameter.GetLimit();

    result.limitReached=false;
    result.results.clear();

    if (!locationIndex) {
      return false;
    }

    for (const auto& token : locationIndex->GetRegionIgnoreTokens()) {
      regionIgnoreTokenSet.insert(UTF8StringToUpper(token));
    }

    if (searchParameter.GetAdminRegionSearchString().empty()) {
      return true;
    }

    // Build Region search patterns

    std::list<TokenStringRef> regionSearchPatterns;

    regionSearchPatterns.push_back(std::make_shared<TokenString>(searchParameter.GetAdminRegionSearchString()));

    // Search for region name

    AdminRegionSearchVisitor adminRegionVisitor(searchParameter.GetStringMatcherFactory(),
                                                regionSearchPatterns);

    locationIndex->VisitAdminRegions(adminRegionVisitor);

    for (const auto& regionMatch : adminRegionVisitor.matches) {
      //std::cout << "Found region match '" << regionMatch.adminRegion->name << "' for pattern '" << regionMatch.tokenString->text << "'" << std::endl;

      if (searchParameter.GetPOISearchString().empty()) {
        AddRegionResult(parameter,
                        LocationSearchResult::match,
                        regionMatch,
                        result);
      }
      else {
        size_t currentResultSize=result.results.size();

        SearchForPOIForRegion(locationIndex,
                              parameter,
                              searchParameter.GetPOISearchString(),
                              regionMatch,
                              LocationSearchResult::match,
                              result);

        if (result.results.size()==currentResultSize) {
          // If we have not found any result for the given search entry, we create one for the "upper" object
          // so that partial results are not lost
          AddRegionResult(parameter,
                          LocationSearchResult::match,
                          regionMatch,
                          result);
        }
      }
    }

    for (const auto& regionMatch : adminRegionVisitor.partialMatches) {
      //std::cout << "Found region candidate '" << regionMatch.adminRegion->name << "' for pattern '" << regionMatch.tokenString->text << "'" << std::endl;

      if (searchParameter.GetPOISearchString().empty()) {
        AddRegionResult(parameter,
                        LocationSearchResult::candidate,
                        regionMatch,
                        result);
      }
      else {
        size_t currentResultSize=result.results.size();

        SearchForPOIForRegion(locationIndex,
                              parameter,
                              searchParameter.GetPOISearchString(),
                              regionMatch,
                              LocationSearchResult::candidate,
                              result);

        if (result.results.size()==currentResultSize) {
          // If we have not found any result for the given search entry, we create one for the "upper" object
          // so that partial results are not lost
          AddRegionResult(parameter,
                          LocationSearchResult::candidate,
                          regionMatch,
                          result);
        }
      }
    }

    result.results.sort();
    result.results.unique();

    return true;
  }

  /**
   * Search for the given location patterns
   * @param search
   *    Data structure holding the search requests
   * @param result
   *    Data structure holding the search result
   * @return
   *    True, if there was no error
   */
  bool LocationService::SearchForLocations(const LocationSearch& search,
                                           LocationSearchResult& result) const
  {
    result.limitReached=false;
    result.results.clear();

    for (const auto& searchEntry : search.searches) {
      if (searchEntry.adminRegionPattern.empty()) {
        continue;
      }

      //std::cout << "Search for region '" << searchEntry.adminRegionPattern << "'..." << std::endl;

      AdminRegionMatchVisitor adminRegionVisitor(searchEntry.adminRegionPattern,
                                                 search.limit);

      if (!VisitAdminRegions(adminRegionVisitor)) {
        log.Error() << "Error during search for region '" << searchEntry.adminRegionPattern << "' in region tree";
        return false;
      }

      if (adminRegionVisitor.limitReached) {
        result.limitReached=true;
      }

      adminRegionVisitor.results.sort([](const AdminRegionMatchVisitor::AdminRegionResult& a,
                                         const AdminRegionMatchVisitor::AdminRegionResult& b) {
        return a.adminRegion->regionOffset<b.adminRegion->regionOffset;
      });

      std::set<FileOffset> visitedAdminHierachie;

      for (const auto& regionResult : adminRegionVisitor.results) {
        std::map<FileOffset,AdminRegionRef> adminHierachie;
        bool                                visited=false;

        if (!ResolveAdminRegionHierachie(regionResult.adminRegion,
                                         adminHierachie)) {
          log.Error() << "Error during resolving admin region hierachie";
          return false;
        }

        for (const auto& hierachieEntry : adminHierachie) {
          if (visitedAdminHierachie.find(hierachieEntry.first)!=visitedAdminHierachie.end()) {
            visited=true;
            break;
          }
        }

        visitedAdminHierachie.insert(regionResult.adminRegion->regionOffset);

        if (visited) {
          continue;
        }

        if (!HandleAdminRegion(search,
                               searchEntry,
                               regionResult,
                               result)) {
          return false;
        }
      }
    }

    result.results.sort();
    result.results.unique();

    return true;
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
    std::list<LocationService::ReverseLookupResult>& results;

    std::list<SearchEntry>                           searchEntries;

  public:
    std::map<FileOffset,AdminRegionRef>              adminRegions;

  public:
    AdminRegionReverseLookupVisitor(const Database& database,
                                    std::list<LocationService::ReverseLookupResult>& results);

    void AddSearchEntry(const SearchEntry& searchEntry);

    Action Visit(const AdminRegion& region) override;
  };

  AdminRegionReverseLookupVisitor::AdminRegionReverseLookupVisitor(const Database& database,
                                                                   std::list<LocationService::ReverseLookupResult>& results)
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
        LocationService::ReverseLookupResult result;

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
    std::list<LocationService::ReverseLookupResult>& results;

  public:
    std::list<Result>                         result;

  public:
    explicit POIReverseLookupVisitor(std::list<LocationService::ReverseLookupResult>& results);

    void AddObject(const ObjectFileRef& object);

    bool Visit(const AdminRegion& adminRegion,
               const POI &poi) override;
  };

  POIReverseLookupVisitor::POIReverseLookupVisitor(std::list<LocationService::ReverseLookupResult>& results)
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
      LocationService::ReverseLookupResult result;

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
    std::list<LocationService::ReverseLookupResult>& results;

  public:
    std::list<Result>                          result;

  public:
    explicit LocationReverseLookupVisitor(std::list<LocationService::ReverseLookupResult>& results);

    void AddObject(const ObjectFileRef& object);

    bool Visit(const AdminRegion& adminRegion,
               const PostalArea& postalArea,
               const Location &location) override;
  };

  LocationReverseLookupVisitor::LocationReverseLookupVisitor(std::list<LocationService::ReverseLookupResult>& results)
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
        LocationService::ReverseLookupResult result;

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
    std::list<LocationService::ReverseLookupResult>& results;

    std::set<ObjectFileRef>                          objects;

  public:
    explicit AddressReverseLookupVisitor(std::list<LocationService::ReverseLookupResult>& results);
    void AddObject(const ObjectFileRef& object);

    bool Visit(const AdminRegion& adminRegion,
               const PostalArea& postalArea,
               const Location &location,
               const Address& address) override;
  };

  AddressReverseLookupVisitor::AddressReverseLookupVisitor(std::list<LocationService::ReverseLookupResult>& results)
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
      LocationService::ReverseLookupResult result;

      result.object=address.object;
      result.adminRegion=std::make_shared<AdminRegion>(adminRegion);
      result.postalArea=std::make_shared<PostalArea>(postalArea);
      result.location=std::make_shared<Location>(location);
      result.address=std::make_shared<Address>(address);

      results.push_back(result);
    }

    return true;
  }

  bool LocationService::ReverseLookupRegion(const GeoCoord &coord,
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
  bool LocationService::ReverseLookupObjects(const std::list<ObjectFileRef>& objects,
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

        for (size_t r=0; r<area->rings.size(); r++) {
          if (area->rings[r].IsOuterRing()) {
            AdminRegionReverseLookupVisitor::SearchEntry searchEntry;

            searchEntry.object=object;
            area->rings[r].GetBoundingBox(searchEntry.bbox);

            searchEntry.coords.resize(area->rings[r].nodes.size());

            for (size_t i=0; i<area->rings[r].nodes.size(); i++) {
              searchEntry.coords[i]=area->rings[r].nodes[i].GetCoord();
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
  bool LocationService::ReverseLookupObject(const ObjectFileRef& object,
                                            std::list<ReverseLookupResult>& result) const
  {
    std::list<ObjectFileRef> objects;

    objects.push_back(object);

    return ReverseLookupObjects(objects,
                                result);
  }

  bool LocationService::LoadNearNodes(const GeoCoord& location, const TypeInfoSet &types,
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

        candidates.push_back(LocationDescriptionCandicate(ObjectFileRef(node->GetFileOffset(),refNode),
                                                          nameFeatureLabelLeader.GetLabel(node->GetFeatureValueBuffer()),
                                                          distance,
                                                          bearing,
                                                          false,
                                                          0.0));
      }
    }

    osmscout::log.Debug() << "Found " << candidates.size() << " nodes near " << location.GetDisplayText();

    return true;
  }

  bool LocationService::LoadNearWays(const GeoCoord& location,
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

  bool LocationService::LoadNearAreas(const GeoCoord& location,
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

  bool LocationService::DistanceComparator(const LocationDescriptionCandicate &a,
                                           const LocationDescriptionCandicate &b)
  {
    if (a.IsAtPlace() && b.IsAtPlace()) {
      return a.GetSize()<b.GetSize();
    }

    return a.GetDistance() < b.GetDistance();
  }

  bool LocationService::DescribeLocationByName(const GeoCoord& location,
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

  bool LocationService::DescribeLocationByAddress(const GeoCoord& location,
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

  bool LocationService::DescribeLocationByPOI(const GeoCoord& location,
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
  bool LocationService::DescribeLocationByCrossing(const GeoCoord& location,
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

  bool LocationService::DescribeLocation(const GeoCoord& location,
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

    if (!DescribeLocationByCrossing(location,
                                    description,
                                    lookupDistance)) {
      return false;
    }

    return true;
  }
}
