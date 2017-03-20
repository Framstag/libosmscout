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
    // no oce
  }

  LocationAtPlaceDescription::LocationAtPlaceDescription(const Place& place,
                                                         double distance,
                                                         double bearing)
  : place(place),
    atPlace(false),
    distance(distance),
    bearing(bearing)
  {

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

  LocationService::VisitorMatcher::VisitorMatcher(const std::string& searchPattern)
  :pattern(searchPattern)
  {
    TolowerUmlaut(pattern);
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
        *it=tolower(*it);
      }
    }
  }

  LocationService::AdminRegionMatchVisitor::AdminRegionMatchVisitor(const std::string& pattern,
                                                                    size_t limit)
  : VisitorMatcher(pattern),
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
    else {
      return visitChildren;
    }
  }

  LocationService::LocationMatchVisitor::LocationMatchVisitor(const AdminRegionRef& adminRegion,
                                                              const std::string& pattern,
                                                              size_t limit)
  : VisitorMatcher(pattern),
    limit(limit),
    adminRegion(adminRegion),
    limitReached(false)
  {
    // no code
  }

  bool LocationService::LocationMatchVisitor::Visit(const AdminRegion& adminRegion,
                                                    const POI &poi)
  {
    bool match;
    bool candidate;

    Match(poi.name,
          match,
          candidate);

    if (match || candidate) {
      POIResult result;

      if (adminRegion.object==this->adminRegion->object) {
        result.adminRegion=this->adminRegion;
      }
      else {
        result.adminRegion=std::make_shared<AdminRegion>(adminRegion);
      }

      result.poi=std::make_shared<POI>(poi);
      result.isMatch=match;

      poiResults.push_back(result);

      limitReached=poiResults.size()+locationResults.size()>=limit;
    }

    return !limitReached;
  }

  bool LocationService::LocationMatchVisitor::Visit(const AdminRegion& adminRegion,
                                                    const Location &loc)
  {
    bool match;
    bool candidate;

    Match(loc.name,
          match,
          candidate);

    if (match || candidate) {
      LocationResult result;

      if (adminRegion.object==this->adminRegion->object) {
        result.adminRegion=this->adminRegion;
      }
      else {
        result.adminRegion=std::make_shared<AdminRegion>(adminRegion);
      }

      result.location=std::make_shared<Location>(loc);
      result.isMatch=match;

      locationResults.push_back(result);

      //std::cout << pattern << " =>  " << result.location->name << " " << adminRegion.name << "/" << adminRegion.aliasName << " " << match << " " << candidate << " " << std::endl;

      limitReached=poiResults.size()+locationResults.size()>=limit;
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
    else if (locationMatchQuality!=other.locationMatchQuality) {
      return locationMatchQuality<other.locationMatchQuality;
    }
    else if (addressMatchQuality!=other.addressMatchQuality) {
      return addressMatchQuality<other.addressMatchQuality;
    }
    else if (poiMatchQuality!=other.poiMatchQuality) {
      return poiMatchQuality<other.poiMatchQuality;
    }
    else if (adminRegion && other.adminRegion &&
        adminRegion->name!=other.adminRegion->name) {
      return adminRegion->name<other.adminRegion->name;
    }
    else if (location && other.location &&
        location->name!=other.location->name) {
      return location->name<other.location->name;
    }
    else if (address && other.address &&
        address->name!=other.address->name) {
      return address->name<other.address->name;
    }
    else if (poi && other.poi &&
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
    NodeRef node;
    WayRef way;
    AreaRef area;
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
    POIRef         poi;
    LocationRef    location;
    AddressRef     address;

    for (const auto& entry : lookupResult) {
      if (entry.adminRegion && !adminRegion) {
        adminRegion=entry.adminRegion;
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
                                                  LocationVisitor& visitor) const
  {
    LocationIndexRef locationIndex=database->GetLocationIndex();

    if (!locationIndex) {
      return false;
    }

    return locationIndex->VisitAdminRegionLocations(region,
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
                                               const Location& location,
                                               AddressVisitor& visitor) const
  {
    LocationIndexRef locationIndex=database->GetLocationIndex();

    if (!locationIndex) {
      return false;
    }

    return locationIndex->VisitLocationAddresses(region,
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

      entry.locationMatchQuality=LocationSearchResult::none;
      entry.poiMatchQuality=LocationSearchResult::none;
      entry.addressMatchQuality=LocationSearchResult::none;

      result.results.push_back(entry);

      return true;
    }

    //std::cout << "  Search for location '" << searchEntry.locationPattern << "'" << " in " << adminRegionResult.adminRegion->name << "/" << adminRegionResult.adminRegion->aliasName << std::endl;

    LocationMatchVisitor visitor(adminRegionResult.adminRegion,
                                 searchEntry.locationPattern,
                                 search.limit>=result.results.size() ? search.limit-result.results.size() : 0);


    if (!VisitAdminRegionLocations(*adminRegionResult.adminRegion,
                                   visitor)) {
      log.Error() << "Error during traversal of region location list";
      return false;
    }

    if (visitor.poiResults.empty() &&
        visitor.locationResults.empty()) {
      // If we search for a location within an area,
      // we do not return the found area as hit, if we
      // did not find the location in it.
      return true;
    }

    for (const auto& poiResult : visitor.poiResults) {
      if (!HandleAdminRegionPOI(search,
                                adminRegionResult,
                                poiResult,
                                result)) {
        log.Error() << "Error during traversal of region poi list";
        return false;
      }
    }

    for (const auto& locationResult : visitor.locationResults) {
      //std::cout << "  - '" << locationResult->location->name << "'" << std::endl;
      if (!HandleAdminRegionLocation(search,
                                     searchEntry,
                                     adminRegionResult,
                                     locationResult,
                                     result)) {
        log.Error() << "Error during traversal of region location list";
        return false;
      }
    }

    return true;
  }

  bool LocationService::HandleAdminRegionLocation(const LocationSearch& search,
                                                  const LocationSearch::Entry& searchEntry,
                                                  const AdminRegionMatchVisitor::AdminRegionResult& adminRegionResult,
                                                  const LocationMatchVisitor::LocationResult& locationResult,
                                                  LocationSearchResult& result) const
  {
    if (searchEntry.addressPattern.empty()) {
      LocationSearchResult::Entry entry;

      entry.adminRegion=locationResult.adminRegion;
      entry.location=locationResult.location;

      if (adminRegionResult.isMatch) {
        entry.adminRegionMatchQuality=LocationSearchResult::match;
      }
      else {
        entry.adminRegionMatchQuality=LocationSearchResult::candidate;
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
                                *locationResult.location,
                                visitor)) {
      log.Error() << "Error during traversal of region location address list";
      return false;
    }

    if (visitor.results.empty()) {
      LocationSearchResult::Entry entry;

      entry.adminRegion=locationResult.adminRegion;
      entry.location=locationResult.location;

      if (adminRegionResult.isMatch) {
        entry.adminRegionMatchQuality=LocationSearchResult::match;
      }
      else {
        entry.adminRegionMatchQuality=LocationSearchResult::candidate;
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
                                             const LocationMatchVisitor::POIResult& poiResult,
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
                                                         const LocationMatchVisitor::LocationResult& locationResult,
                                                         const AddressMatchVisitor::AddressResult& addressResult,
                                                         LocationSearchResult& result) const
  {
    LocationSearchResult::Entry entry;

    entry.adminRegion=locationResult.adminRegion;
    entry.location=addressResult.location;
    entry.address=addressResult.address;

    if (adminRegionResult.isMatch) {
      entry.adminRegionMatchQuality=LocationSearchResult::match;
    }
    else {
      entry.adminRegionMatchQuality=LocationSearchResult::candidate;
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

      for (std::list< std::list<std::string> >::const_iterator slice=slices.begin();
          slice!=slices.end();
          ++slice) {
        std::list<std::string>::const_iterator text1;
        std::list<std::string>::const_iterator text2;
        std::list<std::string>::const_iterator text3;

        text1=slice->begin();
        text2=text1;
        text2++;
        text3=text2;
        text3++;

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

    if (tokens.size()>=1) {
      std::list<std::list<std::string> > slices;

      GroupStringListToStrings(tokens.begin(),
                               tokens.size(),
                               1,
                               slices);

      for (std::list< std::list<std::string> >::const_iterator slice=slices.begin();
          slice!=slices.end();
          ++slice) {
        std::list<std::string>::const_iterator text1=slice->begin();

        osmscout::LocationSearch::Entry search;

        search.adminRegionPattern=*text1;

        if (!locationIndex->IsRegionIgnoreToken(search.adminRegionPattern)) {
          locationSearch.searches.push_back(search);
        }
      }
    }

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

      //std::cout << "Search for region '" << searchEntry->adminRegionPattern << "'..." << std::endl;

      AdminRegionMatchVisitor adminRegionVisitor(searchEntry.adminRegionPattern,
                                                 search.limit);

      if (!VisitAdminRegions(adminRegionVisitor)) {
        log.Error() << "Error during traversal of region tree";
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
        // std::cout << "- '" << regionResult.adminRegion->name << "', '" << regionResult.adminRegion->aliasName << "'..." << std::endl;

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

    Action Visit(const AdminRegion& region);
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

    for (std::list<SearchEntry>::const_iterator entry=searchEntries.begin();
        entry!=searchEntries.end();
        ++entry) {
      if (region.Match(entry->object)) {
        LocationService::ReverseLookupResult result;

        result.object=entry->object;
        result.adminRegion=std::make_shared<AdminRegion>(region);

        results.push_back(result);
      }

      bool candidate=false;

      for (size_t r=0; r<area->rings.size(); r++) {
        if (!area->rings[r].IsOuterRing()) {
          continue;
        }

        for (const auto& entry : searchEntries) {
          if (entry.coords.size()==1) {
            if (!IsCoordInArea(entry.coords.front(),
                               area->rings[r].nodes)) {
              continue;
            }
          }
          else {
            GeoBox ringBBox;
            area->rings[r].GetBoundingBox(ringBBox);
            if (!IsAreaAtLeastPartlyInArea(entry.coords,
                                           area->rings[r].nodes,
                                           entry.bbox,
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
        atLeastOneCandidate = true;
        adminRegions.insert(std::make_pair(region.regionOffset,
                                           std::make_shared<AdminRegion>(region)));
      }
    }

    if (atLeastOneCandidate) {
      return visitChildren;
    }
    else {
      return skipChildren;
    }
  }

  class LocationReverseLookupVisitor : public LocationVisitor
  {
  public:
    struct Loc
    {
      AdminRegionRef adminRegion;
      LocationRef    location;
    };

  private:
    std::set<ObjectFileRef>                   objects;
    std::list<LocationService::ReverseLookupResult>& results;

  public:
    std::list<Loc>                            locations;

  public:
    LocationReverseLookupVisitor(std::list<LocationService::ReverseLookupResult>& results);

    void AddObject(const ObjectFileRef& object);

    bool Visit(const AdminRegion& adminRegion,
               const POI &poi);
    bool Visit(const AdminRegion& adminRegion,
               const Location &location);
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

  bool LocationReverseLookupVisitor::Visit(const AdminRegion& adminRegion,
                                           const Location &location)
  {
    Loc l;

    l.adminRegion=std::make_shared<AdminRegion>(adminRegion);
    l.location=std::make_shared<Location>(location);

    locations.push_back(l);

    for (const auto& object : location.objects) {
      if (objects.find(object)!=objects.end()) {
        LocationService::ReverseLookupResult result;

        result.object=object;
        result.adminRegion=l.adminRegion;
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
    AddressReverseLookupVisitor(std::list<LocationService::ReverseLookupResult>& results);
    void AddObject(const ObjectFileRef& object);

    bool Visit(const AdminRegion& adminRegion,
               const Location &location,
               const Address& address);
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
                                          const Location &location,
                                          const Address& address)
  {
    if (objects.find(address.object)!=objects.end()) {
      LocationService::ReverseLookupResult result;

      result.object=address.object;
      result.adminRegion=std::make_shared<AdminRegion>(adminRegion);
      result.location=std::make_shared<Location>(location);
      result.address=std::make_shared<Address>(address);

      results.push_back(result);
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

    LocationReverseLookupVisitor locationVisitor(result);

    for (const auto& object : objects) {
      locationVisitor.AddObject(object);
    }

    for (const auto& regionEntry : adminRegionVisitor.adminRegions) {
      if (!locationIndex->VisitAdminRegionLocations(*regionEntry.second,
                                                    locationVisitor,
                                                    false)) {
        return false;
      }
    }

    AddressReverseLookupVisitor addressVisitor(result);

    for (const auto& object : objects) {
      addressVisitor.AddObject(object);
    }

    for (const auto& location : locationVisitor.locations) {
      if (!locationIndex->VisitLocationAddresses(*location.adminRegion,
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
              //distance=GetEllipsoidalDistance(location,intersection);
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

    osmscout::log.Debug() << "Found " << areas.size() << " areas near " << location.GetDisplayText();

    return true;
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
      if (distance*1000 <= maxDistance){
        double  bearing=GetSphericalBearingInitial(node.get()->GetCoords(),location);

        candidates.push_back(LocationDescriptionCandicate(ObjectFileRef(node->GetFileOffset(),refNode),
                                                          nameFeatureLabelLeader.GetLabel(node->GetFeatureValueBuffer()),
                                                          distance,
                                                          bearing,
                                                          false,
                                                          0.0));
      }
    }

    osmscout::log.Debug() << "Found " << nodes.size() << " nodes near " << location.GetDisplayText();

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
    // search all addressable areas and nodes, sort it by distance, get first with name
    TypeConfigRef typeConfig=database->GetTypeConfig();

    if (!typeConfig) {
      return false;
    }

    std::vector<LocationDescriptionCandicate> candidates;

    TypeInfoSet nameTypes;

    // near addressable areas
    for (const auto& type : typeConfig->GetTypes()) {
      if (type->CanBeArea() &&
          type->HasFeature(AddressFeature::NAME)) {
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

    // near addressable nodes
    nameTypes.Clear();
    for (const auto& type : typeConfig->GetTypes()) {
      if (type->CanBeNode() &&
          type->HasFeature(AddressFeature::NAME)) {
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

      if (candidate.GetSize() > sizeFilter || candidate.GetName().empty()) {
        continue;
      }

      if (!ReverseLookupObject(candidate.GetRef(), result)) {
        return false;
      }

      if (!result.empty()) {
        Place place=GetPlace(result);

        if (candidate.IsAtPlace()) {
          description.SetAtNameDescription(std::make_shared<LocationAtPlaceDescription>(place));
        }
        else {
          description.SetAtNameDescription(std::make_shared<LocationAtPlaceDescription>(place,
                                                                                        candidate.GetDistance()*1000, candidate.GetBearing()));
        }

        return true;
      }
      else if (!candidate.GetName().empty()) {
        AdminRegionRef adminRegion;
        POIRef         poi=std::make_shared<POI>();
        LocationRef    location;
        AddressRef     address;

        poi->object=candidate.GetRef();
        poi->name=candidate.GetName();
        poi->regionOffset=0;

        Place place(candidate.GetRef(),
                    GetObjectFeatureBuffer(candidate.GetRef()),
                    adminRegion,
                    poi,
                    location,
                    address);

        if (candidate.IsAtPlace()) {
          description.SetAtNameDescription(std::make_shared<LocationAtPlaceDescription>(place));
        }
        else {
          description.SetAtNameDescription(std::make_shared<LocationAtPlaceDescription>(place,
                                                                                        candidate.GetDistance()*1000, candidate.GetBearing()));
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

  bool LocationService::DescribeLocation(const GeoCoord& location,
                                         LocationDescription& description)
  {
    description.SetCoordDescription(std::make_shared<LocationCoordDescription>(location));

    if (!DescribeLocationByName(location,
                                description)) {
      return false;
    }

    if (!DescribeLocationByAddress(location,
                                   description)) {
      return false;
    }

    if (!DescribeLocationByPOI(location,
                               description)) {
      return false;
    }

    return true;
  }

}
