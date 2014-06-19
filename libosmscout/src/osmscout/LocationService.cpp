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

#include <osmscout/LocationService.h>

#include <osmscout/util/Geometry.h>
#include <osmscout/util/String.h>

namespace osmscout {

  LocationService::AdminRegionMatchVisitor::AdminRegionMatchVisitor(const std::string& pattern,
                                                                    size_t limit)
  : pattern(pattern),
    limit(limit),
    limitReached(false)
  {
    // no code
  }

  void LocationService::AdminRegionMatchVisitor::Match(const std::string& name,
                                                       bool& match,
                                                       bool& candidate) const
  {
    std::string::size_type matchPosition;

    matchPosition=name.find(pattern);

    match=matchPosition==0 && name.length()==pattern.length();
    candidate=matchPosition!=std::string::npos;
  }

  AdminRegionVisitor::Action LocationService::AdminRegionMatchVisitor::Visit(const AdminRegion& region)
  {
    bool atLeastOneMatch=false;

    bool match;
    bool candidate;

    for (size_t i=0; i<region.aliases.size(); i++) {
      Match(region.aliases[i].name,
            match,
            candidate);

      if (match || candidate) {
        AdminRegionResult result;

        result.adminRegion=new AdminRegion(region);
        result.isMatch=match;

        result.adminRegion->aliasName=region.aliases[i].name;
        result.adminRegion->aliasObject.Set(region.aliases[i].objectOffset,refNode);

        results.push_back(result);

        if (match) {
          atLeastOneMatch=true;
        }

        limitReached=results.size()>=limit;
      }
    }

    // If we have a perfect match for an alias, we not not try to match
    // the region name itself
    if (!atLeastOneMatch) {
      Match(region.name,
            match,
            candidate);

      if (match || candidate) {
        AdminRegionResult result;

        result.adminRegion=new AdminRegion(region);
        result.isMatch=match;

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

  LocationService::LocationMatchVisitor::LocationMatchVisitor(const std::string& pattern,
                                                              size_t limit)
  : pattern(pattern),
    limit(limit),
    limitReached(false)
  {
    // no code
  }

  void LocationService::LocationMatchVisitor::Match(const std::string& name,
                                                    bool& match,
                                                    bool& candidate) const
  {
    std::string::size_type matchPosition;

    matchPosition=name.find(pattern);

    match=matchPosition==0 && name.length()==pattern.length();
    candidate=matchPosition!=std::string::npos;
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

      result.adminRegion=new AdminRegion(adminRegion);
      result.poi=new POI(poi);
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

      result.adminRegion=new AdminRegion(adminRegion);
      result.location=new Location(loc);
      result.isMatch=match;

      locationResults.push_back(result);

      limitReached=poiResults.size()+locationResults.size()>=limit;
    }

    return !limitReached;
  }

  LocationService::AddressMatchVisitor::AddressMatchVisitor(const std::string& pattern,
                                                            size_t limit)
  : pattern(pattern),
    limit(limit),
    limitReached(false)
  {
    // no code
  }

  void LocationService::AddressMatchVisitor::Match(const std::string& name,
                                                   bool& match,
                                                   bool& candidate) const
  {
    std::string::size_type matchPosition;

    matchPosition=name.find(pattern);

    match=matchPosition==0 && name.length()==pattern.length();
    candidate=matchPosition!=std::string::npos;
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

      result.adminRegion=new AdminRegion(adminRegion);
      result.location=new Location(location);
      result.address=new Address(address);
      result.isMatch=match;

      results.push_back(result);

      limitReached=results.size()>=limit;
    }

    return !limitReached;
  }

  void LocationSearch::InitializeSearchEntries(const std::string& searchPattern)
  {
    std::list<std::string> tokens;

    if (searchPattern.empty()) {
      return;
    }

    TokenizeString(searchPattern,
                   tokens);

    if (tokens.empty()) {
      return;
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

        searches.push_back(search);

        search.locationPattern=*text2;
        search.addressPattern=*text3;
        search.adminRegionPattern=*text1;

        searches.push_back(search);
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

        searches.push_back(search);

        search.locationPattern=*text2;
        search.adminRegionPattern=*text1;

        searches.push_back(search);
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

        searches.push_back(search);
      }
    }
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
    else if (adminRegion.Valid() && other.adminRegion.Valid() &&
        adminRegion->name!=other.adminRegion->name) {
      return adminRegion->name<other.adminRegion->name;
    }
    else if (location.Valid() && other.location.Valid() &&
        location->name!=other.location->name) {
      return location->name<other.location->name;
    }
    else if (address.Valid() && other.address.Valid() &&
        address->name!=other.address->name) {
      return address->name<other.address->name;
    }
    else if (poi.Valid() && other.poi.Valid() &&
        poi->name!=other.poi->name) {
      return poi->name<other.poi->name;
    }

    return this<&other;
  }

  bool LocationSearchResult::Entry::operator==(const Entry& other) const
  {
    if ((adminRegion.Valid() && !other.adminRegion.Valid()) ||
        (!adminRegion.Valid() && other.adminRegion.Valid())) {
      return false;
    }

    if (adminRegion.Valid() && other.adminRegion.Valid()) {
      if (adminRegion->aliasObject!=other.adminRegion->aliasObject) {
        return false;
      }

      if (adminRegion->object!=other.adminRegion->object) {
        return false;
      }
    }

    if ((poi.Valid() && !other.poi.Valid()) ||
        (!poi.Valid() && other.poi.Valid())) {
      return false;
    }

    if (poi.Valid() && other.poi.Valid()) {
      if (poi->object!=other.poi->object) {
        return false;
      }
    }

    if ((location.Valid() && !other.location.Valid()) ||
        (!location.Valid() && other.location.Valid())) {
      return false;
    }

    if (location.Valid() && other.location.Valid()) {
      if (location->locationOffset!=other.location->locationOffset) {
        return false;
      }
    }

    if ((address.Valid() && !other.address.Valid()) ||
        (!address.Valid() && other.address.Valid())) {
      return false;
    }

    if (address.Valid() && other.address.Valid()) {
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
    // no code
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

    if (locationIndex.Invalid()) {
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

    if (locationIndex.Invalid()) {
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

    if (locationIndex.Invalid()) {
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

    if (locationIndex.Invalid()) {
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

    //std::cout << "  Search for location '" << searchEntry.locationPattern << "'" << std::endl;

    LocationMatchVisitor visitor(searchEntry.locationPattern,
                                 search.limit>=result.results.size() ? search.limit-result.results.size() : 0);


    if (!VisitAdminRegionLocations(adminRegionResult.adminRegion,
                                   visitor)) {
      std::cerr << "Error during traversal of region location list" << std::endl;
      return false;
    }

    if (visitor.poiResults.empty() &&
        visitor.locationResults.empty()) {
      // If we search for a location within an area,
      // we do not return the found area as hit, if we
      // did not find the location in it.
      return true;
    }

    for (std::list<LocationMatchVisitor::POIResult>::const_iterator poiResult=visitor.poiResults.begin();
        poiResult!=visitor.poiResults.end();
        ++poiResult) {
      if (!HandleAdminRegionPOI(search,
                                adminRegionResult,
                                *poiResult,
                                result)) {
        std::cerr << "Error during traversal of region poi list" << std::endl;
        return false;
      }
    }

    for (std::list<LocationMatchVisitor::LocationResult>::const_iterator locationResult=visitor.locationResults.begin();
        locationResult!=visitor.locationResults.end();
        ++locationResult) {
      //std::cout << "  - '" << locationResult->location->name << "'" << std::endl;
      if (!HandleAdminRegionLocation(search,
                                     searchEntry,
                                     adminRegionResult,
                                     *locationResult,
                                     result)) {
        std::cerr << "Error during traversal of region location list" << std::endl;
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


    if (!VisitLocationAddresses(locationResult.adminRegion,
                                locationResult.location,
                                visitor)) {
      std::cerr << "Error during traversal of region location address list" << std::endl;
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

    for (std::list<AddressMatchVisitor::AddressResult>::const_iterator addressResult=visitor.results.begin();
        addressResult!=visitor.results.end();
        ++addressResult) {
      //std::cout << "    - '" << addressResult->address->name << "'" << std::endl;
      if (!HandleAdminRegionLocationAddress(search,
                                            adminRegionResult,
                                            locationResult,
                                            *addressResult,
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

    for (std::list<LocationSearch::Entry>::const_iterator searchEntry=search.searches.begin();
        searchEntry!=search.searches.end();
        ++searchEntry) {
      if (searchEntry->adminRegionPattern.empty()) {
        continue;
      }

      //std::cout << "Search for region '" << searchEntry->adminRegionPattern << "'..." << std::endl;

      AdminRegionMatchVisitor adminRegionVisitor(searchEntry->adminRegionPattern,
                                                 search.limit);

      if (!VisitAdminRegions(adminRegionVisitor)) {
        std::cerr << "Error during traversal of region tree" << std::endl;
        return false;
      }

      if (adminRegionVisitor.limitReached) {
        result.limitReached=true;
      }

      for (std::list<AdminRegionMatchVisitor::AdminRegionResult>::const_iterator regionResult=adminRegionVisitor.results.begin();
          regionResult!=adminRegionVisitor.results.end();
          ++regionResult) {
        //std::cout << "- '" << regionResult->adminRegion->name << "', '" << regionResult->adminRegion->aliasName << "'..." << std::endl;

        if (!HandleAdminRegion(search,
                               *searchEntry,
                               *regionResult,
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
        result.adminRegion=new AdminRegion(region);

        results.push_back(result);
      }

      bool candidate=false;

      for (size_t r=0; r<area->rings.size(); r++) {
        if (area->rings[r].ring!=Area::outerRingId) {
          continue;
        }

        for (std::list<SearchEntry>::const_iterator entry=searchEntries.begin();
            entry!=searchEntries.end();
            ++entry) {
          if (entry->coords.size()==1) {
            if (!IsCoordInArea(entry->coords.front(),
                               area->rings[r].nodes)) {
              continue;
            }
          }
          else {
            if (!IsAreaAtLeastPartlyInArea(entry->coords,
                                           area->rings[r].nodes)) {
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
                                           new AdminRegion(region)));
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
      result.adminRegion=new AdminRegion(adminRegion);
      result.poi=new POI(poi);

      results.push_back(result);
    }

    return true;
  }

  bool LocationReverseLookupVisitor::Visit(const AdminRegion& adminRegion,
                                           const Location &location)
  {
    Loc l;

    l.adminRegion=new AdminRegion(adminRegion);
    l.location=new Location(location);

    locations.push_back(l);

    for (std::vector<ObjectFileRef>::const_iterator object=location.objects.begin();
        object!=location.objects.end();
        ++object) {
      if (objects.find(*object)!=objects.end()) {
        LocationService::ReverseLookupResult result;

        result.object=*object;
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
      result.adminRegion=new AdminRegion(adminRegion);
      result.location=new Location(location);
      result.address=new Address(address);

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

    if (locationIndex.Invalid()) {
      return false;
    }

    AdminRegionReverseLookupVisitor adminRegionVisitor(*database,
                                                       result);

    for (std::list<ObjectFileRef>::const_iterator object=objects.begin();
        object!=objects.end();
        ++object) {
      std::vector<GeoCoord> coords;

      if (object->GetType()==refNode) {
        NodeRef node;

        if (!database->GetNodeByOffset(object->GetFileOffset(),
                                       node)) {
          return false;
        }

        AdminRegionReverseLookupVisitor::SearchEntry searchEntry;

        searchEntry.object=*object;
        searchEntry.coords.push_back(node->GetCoords());

        adminRegionVisitor.AddSearchEntry(searchEntry);
      }
      else if (object->GetType()==refArea) {
        AreaRef area;

        if (!database->GetAreaByOffset(object->GetFileOffset(),
                                       area)) {
          return false;
        }

        for (size_t r=0; r<area->rings.size(); r++) {
          if (area->rings[r].ring==Area::outerRingId) {
            AdminRegionReverseLookupVisitor::SearchEntry searchEntry;

            searchEntry.object=*object;
            searchEntry.coords=area->rings[r].nodes;

            adminRegionVisitor.AddSearchEntry(searchEntry);
          }
        }
      }
      else if (object->GetType()==refWay) {
        WayRef way;

        if (!database->GetWayByOffset(object->GetFileOffset(),
                                      way)) {
          return false;
        }

        AdminRegionReverseLookupVisitor::SearchEntry searchEntry;

        searchEntry.object=*object;
        searchEntry.coords=way->nodes;

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

    for (std::list<ObjectFileRef>::const_iterator object=objects.begin();
        object!=objects.end();
        ++object) {
      locationVisitor.AddObject(*object);
    }

    for (std::map<FileOffset,AdminRegionRef>::const_iterator regionEntry=adminRegionVisitor.adminRegions.begin();
        regionEntry!=adminRegionVisitor.adminRegions.end();
        ++regionEntry) {
      if (!locationIndex->VisitAdminRegionLocations(*regionEntry->second,
                                                    locationVisitor,
                                                    false)) {
        return false;
      }
    }

    AddressReverseLookupVisitor addressVisitor(result);

    for (std::list<ObjectFileRef>::const_iterator object=objects.begin();
        object!=objects.end();
        ++object) {
      addressVisitor.AddObject(*object);
    }

    for (std::list<LocationReverseLookupVisitor::Loc>::const_iterator location=locationVisitor.locations.begin();
        location!=locationVisitor.locations.end();
        ++location) {

      if (!locationIndex->VisitLocationAddresses(location->adminRegion,
                                                 location->location,
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
}
