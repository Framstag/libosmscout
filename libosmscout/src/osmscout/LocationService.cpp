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

#include <algorithm>

#include <osmscout/util/Geometry.h>
#include <osmscout/util/Logger.h>
#include <osmscout/util/String.h>
#include <osmscout/TypeFeatures.h>
#include <iostream>
namespace osmscout {

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
    bool                    postalAreaOnlyMatch;
    bool                    locationOnlyMatch;
    bool                    addressOnlyMatch;

    bool                    partialMatch;

    StringMatcherFactoryRef stringMatcherFactory;

    size_t                  limit;

    SearchParameter() = default;
  };

  LocationFormSearchParameter::LocationFormSearchParameter()
    : adminRegionOnlyMatch(false),
      postalAreaOnlyMatch(false),
      locationOnlyMatch(false),
      addressOnlyMatch(false),
      partialMatch(false),
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

  bool LocationFormSearchParameter::GetPartialMatch() const
  {
    return partialMatch;
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

  void LocationFormSearchParameter::SetPartialMatch(bool partialMatch)
  {
    this->partialMatch=partialMatch;
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
      partialMatch(true),
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

  void LocationStringSearchParameter::SetPartialMatch(bool partialMatch)
  {
    this->partialMatch=partialMatch;
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

  bool LocationStringSearchParameter::GetPartialMatch() const
  {
    return partialMatch;
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
        this->patterns.emplace_back(pattern,
                                    matcherFactory->CreateMatcher(pattern->text));
      }
    }

    Action Visit(const AdminRegion& region) override
    {
      for (const auto& pattern : patterns) {
        StringMatcher::Result matchResult=pattern.matcher->Match(region.name);

        if (matchResult==StringMatcher::match) {
          //std::cout << "Match of pattern " << pattern.tokenString->text << " against region name '" << region.name << "'" << std::endl;
          matches.emplace_back(pattern.tokenString,
                               std::make_shared<AdminRegion>(region),
                               region.name);

        }
        else if (matchResult==StringMatcher::partialMatch) {
          //std::cout << "Partial match of pattern " << pattern.tokenString->text << " against region name '" << region.name << "'" << std::endl;
          partialMatches.emplace_back(pattern.tokenString,
                                      std::make_shared<AdminRegion>(region),
                                      region.name);
        }

        if (matchResult!=StringMatcher::match) {
          for (const auto& alias : region.aliases) {
            matchResult=pattern.matcher->Match(alias.name);

            if (matchResult==StringMatcher::match) {
              //std::cout << "Match of pattern " << pattern.tokenString->text << " against region alias '" << region.name << "' '" << alias.name << "'" << std::endl;
              partialMatches.emplace_back(pattern.tokenString,
                                          std::make_shared<AdminRegion>(region),
                                          alias.name);
              break;
            }
            else if (matchResult==StringMatcher::partialMatch) {
              //std::cout << "Partial match of pattern " << pattern.tokenString->text << " against region alias '" << region.name << "' '" << alias.name << "'" << std::endl;
              partialMatches.emplace_back(pattern.tokenString,
                                          std::make_shared<AdminRegion>(region),
                                          alias.name);
            }
          }
        }
      }

      return visitChildren;
    }
  };

  class PostalAreaSearchVisitor : public AdminRegionVisitor
  {
  public:
    struct Result
    {
      TokenStringRef tokenString;
      AdminRegionRef adminRegion;
      PostalAreaRef  postalArea;
      std::string    name;

      Result(const TokenStringRef& tokenString,
             const AdminRegionRef& adminRegion,
             const PostalAreaRef& postalArea,
             const std::string& name)
        : tokenString(tokenString),
          adminRegion(adminRegion),
          postalArea(postalArea),
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
    PostalAreaSearchVisitor(const StringMatcherFactoryRef& matcherFactory,
                            const std::list<TokenStringRef>& patterns)
    {
      for (const auto& pattern : patterns) {
        this->patterns.emplace_back(pattern,
                                    matcherFactory->CreateMatcher(pattern->text));
      }
    }

    Action Visit(const AdminRegion& region) override
    {
      //std::cout << "Visiting admin region: " << region.name << std::endl;

      for (const auto& area : region.postalAreas) {
        if (patterns.empty()) {
          //std::cout << "Match postal area name '" << area.name << "'" << std::endl;
          matches.emplace_back(std::make_shared<TokenString>(0,0,""),
                                      std::make_shared<AdminRegion>(region),
                                      std::make_shared<PostalArea>(area),
                                      area.name);
        }
        else {
          for (const auto& pattern : patterns) {
            StringMatcher::Result matchResult;

            if (area.name.empty()) {
              // the empty postal area always matches any pattern
              matchResult=StringMatcher::match;
            }
            else {
              matchResult=pattern.matcher->Match(area.name);
            }

            if (matchResult==StringMatcher::match) {
              //std::cout << "Match postal area name '" << area.name << "'" << std::endl;
              matches.emplace_back(pattern.tokenString,
                                   std::make_shared<AdminRegion>(region),
                                   std::make_shared<PostalArea>(area),
                                   area.name);

            }
            else if (matchResult==StringMatcher::partialMatch) {
              //std::cout << "Partial match postal area name '" << area.name << "'" << std::endl;
              partialMatches.emplace_back(pattern.tokenString,
                                          std::make_shared<AdminRegion>(region),
                                          std::make_shared<PostalArea>(area),
                                          area.name);
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
        this->patterns.emplace_back(pattern,
                                    matcherFactory->CreateMatcher(pattern->text));
      }
    }

    bool Visit(const AdminRegion& adminRegion,
               const POI& poi) override
    {
      for (const auto& pattern : patterns) {
        StringMatcher::Result matchResult=pattern.matcher->Match(poi.name);

        if (matchResult==StringMatcher::match) {
          matches.emplace_back(pattern.tokenString,
                               std::make_shared<AdminRegion>(adminRegion),
                               std::make_shared<POI>(poi));
        }
        else if (matchResult==StringMatcher::partialMatch) {
          partialMatches.emplace_back(pattern.tokenString,
                                      std::make_shared<AdminRegion>(adminRegion),
                                      std::make_shared<POI>(poi));
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
        this->patterns.emplace_back(pattern,
                                    matcherFactory->CreateMatcher(pattern->text));
      }
    }

    bool Visit(const AdminRegion& adminRegion,
               const PostalArea& postalArea,
               const Location& location) override
    {
      //std::cout << "Visiting " << adminRegion.name << " " << postalArea.name << "..." << std::endl;

      for (const auto& pattern : patterns) {
        StringMatcher::Result matchResult=pattern.matcher->Match(location.name);

        if (matchResult==StringMatcher::match) {
          //std::cout << "Match location name '" << location.name << "'" << std::endl;
          matches.emplace_back(pattern.tokenString,
                               std::make_shared<AdminRegion>(adminRegion),
                               std::make_shared<PostalArea>(postalArea),
                               std::make_shared<Location>(location));
        }
        else if (matchResult==StringMatcher::partialMatch) {
          //std::cout << "Partial match location name '" << location.name << "'" << std::endl;
          partialMatches.emplace_back(pattern.tokenString,
                                      std::make_shared<AdminRegion>(adminRegion),
                                      std::make_shared<PostalArea>(postalArea),
                                      std::make_shared<Location>(location));
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
        this->patterns.emplace_back(pattern,
                                    matcherFactory->CreateMatcher(pattern->text));
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
          matches.emplace_back(pattern.tokenString,
                               std::make_shared<AdminRegion>(adminRegion),
                               std::make_shared<PostalArea>(postalArea),
                               std::make_shared<Location>(location),
                               std::make_shared<Address>(address));
        }
        else if (matchResult==StringMatcher::partialMatch) {
          //std::cout << "Partial match region name '" << region.name << "'" << std::endl;
          partialMatches.emplace_back(pattern.tokenString,
                                      std::make_shared<AdminRegion>(adminRegion),
                                      std::make_shared<PostalArea>(postalArea),
                                      std::make_shared<Location>(location),
                                      std::make_shared<Address>(address));
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

  static void AddPostalAreaResult(const SearchParameter& parameter,
                                  LocationSearchResult::MatchQuality regionMatchQuality,
                                  const PostalAreaSearchVisitor::Result& postalAreaMatch,
                                  LocationSearchResult::MatchQuality postalAreaMatchQuality,
                                  LocationSearchResult& result)
  {
    if (result.results.size()>parameter.limit) {
      result.limitReached=true;
    }
    else {
      LocationSearchResult::Entry entry;

      //std::cout << "Add location: " << locationMatch.location->name << " " << locationMatch.postalArea->name << " " << locationMatch.adminRegion->name << std::endl;

      entry.adminRegion=postalAreaMatch.adminRegion;
      entry.adminRegionMatchQuality=regionMatchQuality;
      entry.poiMatchQuality=LocationSearchResult::none;
      entry.postalArea=postalAreaMatch.postalArea;
      entry.postalAreaMatchQuality=postalAreaMatchQuality;
      entry.locationMatchQuality=LocationSearchResult::none;
      entry.addressMatchQuality=LocationSearchResult::none;

      result.results.push_back(entry);
      result.results.sort();
      result.results.unique();
    }
  }

  static void AddLocationResult(const SearchParameter& parameter,
                                LocationSearchResult::MatchQuality regionMatchQuality,
                                LocationSearchResult::MatchQuality postalAreaMatchQuality,
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
      entry.postalAreaMatchQuality=postalAreaMatchQuality;
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
                          LocationSearchResult::candidate,
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

        if (result.results.size()==currentResultSize && parameter.partialMatch) {
          // If we have not found any result for the given search entry, we create one for the "upper" object
          // so that partial results are not lost
          AddLocationResult(parameter,
                            regionMatchQuality,
                            LocationSearchResult::candidate,
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
                            LocationSearchResult::candidate,
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

          if (result.results.size()==currentResultSize && parameter.partialMatch) {
            // If we have not found any result for the given search entry, we create one for the "upper" object
            // so that partial results are not lost
            AddLocationResult(parameter,
                              regionMatchQuality,
                              LocationSearchResult::candidate,
                              locationMatch,
                              LocationSearchResult::candidate,
                              result);
          }
        }
      }
    }

    return true;
  }

  static bool SearchForLocationForPostalArea(LocationIndexRef& locationIndex,
                                             const SearchParameter& parameter,
                                             const std::string& locationPattern,
                                             const std::string& addressPattern,
                                             const PostalAreaSearchVisitor::Result& postalAreaMatch,
                                             LocationSearchResult::MatchQuality regionMatchQuality,
                                             LocationSearchResult::MatchQuality postalAreaMatchQuality,
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

    //std::cout << "Search for location for " << postalAreaMatch.adminRegion->name << " " << postalAreaMatch.postalArea->name << "..." << std::endl;

    if (!locationIndex->VisitLocations(*postalAreaMatch.adminRegion,
                                       *postalAreaMatch.postalArea,
                                       locationVisitor,
                                       false)) {
      return false;
    }

    for (const auto& locationMatch : locationVisitor.matches) {
      //std::cout << "Found location match '" << locationMatch.location->name << "' for pattern '" << locationMatch.tokenString->text << "'" << std::endl;
      if (addressPattern.empty()) {
        AddLocationResult(parameter,
                          regionMatchQuality,
                          postalAreaMatchQuality,
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

        if (result.results.size()==currentResultSize &&
            parameter.partialMatch) {
          // If we have not found any result for the given search entry, we create one for the "upper" object
          // so that partial results are not lost
          AddLocationResult(parameter,
                            regionMatchQuality,
                            postalAreaMatchQuality,
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
                            postalAreaMatchQuality,
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

          if (result.results.size()==currentResultSize &&
              parameter.partialMatch) {
            // If we have not found any result for the given search entry, we create one for the "upper" object
            // so that partial results are not lost
            AddLocationResult(parameter,
                              regionMatchQuality,
                              postalAreaMatchQuality,
                              locationMatch,
                              LocationSearchResult::candidate,
                              result);
          }
        }
      }
    }

    return true;
  }

  static bool SearchForPostalAreaForRegion(LocationIndexRef& locationIndex,
                                           const SearchParameter& parameter,
                                           const std::string& postalAreaPattern,
                                           const std::string& locationPattern,
                                           const std::string& addressPattern,
                                           const AdminRegionSearchVisitor::Result& regionMatch,
                                           LocationSearchResult::MatchQuality regionMatchQuality,
                                           LocationSearchResult& result)
  {
    /*
    std::unordered_set<std::string> postalAreaIgnoreTokenSet;

    for (const auto& token : locationIndex->GetLocationIgnoreTokens()) {
      locationIgnoreTokenSet.insert(UTF8StringToUpper(token));
    }*/

    // Build Location search patterns

    //std::cout << "Search for postal area '" << postalAreaPattern << "'" << std::endl;

    std::list<TokenStringRef> postalAreaSearchPatterns;

    if (!postalAreaPattern.empty()) {
      postalAreaSearchPatterns.push_back(std::make_shared<TokenString>(postalAreaPattern));
    }

    // Search for locations

    PostalAreaSearchVisitor postalAreaVisitor(parameter.stringMatcherFactory,
                                               postalAreaSearchPatterns);

    if (!locationIndex->VisitAdminRegions(*regionMatch.adminRegion,
                                          postalAreaVisitor)) {
      return false;
    }

    for (const auto& postalAreaMatch : postalAreaVisitor.matches) {
      //std::cout << "Found postal area match '" << postalAreaMatch.adminRegion->name << " " << postalAreaMatch.postalArea->name << "' for pattern '" << postalAreaMatch.tokenString->text << "'" << std::endl;

      if (locationPattern.empty() &&
          addressPattern.empty()) {
        AddPostalAreaResult(parameter,
                            regionMatchQuality,
                            postalAreaMatch,
                            LocationSearchResult::match,
                            result);
      }
      else {
        std::list<std::string> locationTokens;
        size_t                 currentResultSize=result.results.size();

        locationTokens.push_back(locationPattern);

        SearchForLocationForPostalArea(locationIndex,
                                       parameter,
                                       locationPattern,
                                       addressPattern,
                                       postalAreaMatch,
                                       regionMatchQuality,
                                       LocationSearchResult::match,
                                       result);

        if (result.results.size()==currentResultSize &&
            parameter.partialMatch) {
          // If we have not found any result for the given search entry, we create one for the "upper" object
          // so that partial results are not lost
          AddPostalAreaResult(parameter,
                              regionMatchQuality,
                              postalAreaMatch,
                              LocationSearchResult::match,
                              result);
        }
      }
    }

    if (!parameter.postalAreaOnlyMatch) {
      for (const auto& postalAreaMatch : postalAreaVisitor.partialMatches) {
        //std::cout << "Found postal area candidate '" << postalAreaMatch.adminRegion->name << " " << postalAreaMatch.postalArea->name << "' for pattern '" << postalAreaMatch.tokenString->text << "'" << std::endl;
        if (locationPattern.empty() &&
            addressPattern.empty()) {
          AddPostalAreaResult(parameter,
                              regionMatchQuality,
                              postalAreaMatch,
                              LocationSearchResult::candidate,
                              result);
        }
        else {
          std::list<std::string> locationTokens;
          size_t                 currentResultSize=result.results.size();

          locationTokens.push_back(locationPattern);

          SearchForLocationForPostalArea(locationIndex,
                                         parameter,
                                         locationPattern,
                                         addressPattern,
                                         postalAreaMatch,
                                         regionMatchQuality,
                                         LocationSearchResult::candidate,
                                         result);

          if (result.results.size()==currentResultSize &&
              parameter.partialMatch) {
            // If we have not found any result for the given search entry, we create one for the "upper" object
            // so that partial results are not lost
            AddPostalAreaResult(parameter,
                                regionMatchQuality,
                                postalAreaMatch,
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
    parameter.postalAreaOnlyMatch=false;
    parameter.locationOnlyMatch=searchParameter.GetLocationOnlyMatch();
    parameter.addressOnlyMatch=searchParameter.GetAddressOnlyMatch();
    parameter.partialMatch=searchParameter.GetPartialMatch();
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

    if (tokens.empty()) {
      return true;
    }

    if (defaultAdminRegion) {
      const std::list<std::string>& locationTokens=tokens;
      TokenStringRef                tokenString=std::make_shared<TokenString>(0,defaultAdminRegion->name.length(),defaultAdminRegion->name);

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

        if (result.results.size()==currentResultSize && parameter.partialMatch) {
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

          if (result.results.size()==currentResultSize && parameter.partialMatch) {
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
    parameter.postalAreaOnlyMatch=searchParameter.GetPostalAreaOnlyMatch();
    parameter.locationOnlyMatch=searchParameter.GetLocationOnlyMatch();
    parameter.addressOnlyMatch=searchParameter.GetAddressOnlyMatch();
    parameter.partialMatch=searchParameter.GetPartialMatch();
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

      if (searchParameter.GetPostalAreaSearchString().empty() &&
          searchParameter.GetLocationSearchString().empty() &&
          searchParameter.GetAddressSearchString().empty()) {
        AddRegionResult(parameter,
                        LocationSearchResult::match,
                        regionMatch,
                        result);
      }
      else {
        size_t currentResultSize=result.results.size();

        SearchForPostalAreaForRegion(locationIndex,
                                     parameter,
                                     searchParameter.GetPostalAreaSearchString(),
                                     searchParameter.GetLocationSearchString(),
                                     searchParameter.GetAddressSearchString(),
                                     regionMatch,
                                     LocationSearchResult::match,
                                     result);

        if (result.results.size()==currentResultSize &&
            searchParameter.GetPartialMatch()) {
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

      if (searchParameter.GetPostalAreaSearchString().empty() &&
          searchParameter.GetLocationSearchString().empty() &&
          searchParameter.GetAddressSearchString().empty()) {
        AddRegionResult(parameter,
                        LocationSearchResult::candidate,
                        regionMatch,
                        result);
      }
      else {
        size_t currentResultSize=result.results.size();

        SearchForPostalAreaForRegion(locationIndex,
                                     parameter,
                                     searchParameter.GetPostalAreaSearchString(),
                                     searchParameter.GetLocationSearchString(),
                                     searchParameter.GetAddressSearchString(),
                                     regionMatch,
                                     LocationSearchResult::candidate,
                                     result);

        if (result.results.size()==currentResultSize &&
            searchParameter.GetPartialMatch()) {
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
    parameter.partialMatch=true;
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
}
