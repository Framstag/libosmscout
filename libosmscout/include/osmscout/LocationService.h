#ifndef OSMSCOUT_LOCATIONSERVICE_H
#define OSMSCOUT_LOCATIONSERVICE_H

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

#include <list>
#include <memory>

#include <osmscout/Database.h>
#include <osmscout/Location.h>

#include <osmscout/util/StringMatcher.h>

namespace osmscout {

  /**
   * \ingroup Location
   *
   * Parameter object for form based search of a POI (point of interest)
   */
  class OSMSCOUT_API POIFormSearchParameter CLASS_FINAL
  {
  private:
    std::string             adminRegionSearchString; //!< The search string to match the admin region name against
    std::string             poiSearchString;         //!< The search string to match the postal area name against

    bool                    adminRegionOnlyMatch;    //!< Evaluate on direct admin region matches
    bool                    poiOnlyMatch;            //!< Evaluate on direct poi matches

    StringMatcherFactoryRef stringMatcherFactory;    //!< String matcher factory to use

    size_t                  limit;                   //!< The maximum number of results over all sub searches requested

  public:
    explicit POIFormSearchParameter();

    std::string GetAdminRegionSearchString() const;
    std::string GetPOISearchString() const;

    bool GetAdminRegionOnlyMatch() const;
    bool GetPOIOnlyMatch() const;

    StringMatcherFactoryRef GetStringMatcherFactory() const;

    size_t GetLimit() const;

    void SetStringMatcherFactory(const StringMatcherFactoryRef& stringMatcherFactory);

    void SetAdminRegionSearchString(const std::string& adminRegionSearchString);
    void SetPOISearchString(const std::string& poiSearchString);

    void SetAdminRegionOnlyMatch(bool adminRegionOnlyMatch);
    void SetPOIOnlyMatch(bool poiOnlyMatch);

    void SetLimit(size_t limit);
  };

  /**
   * \ingroup Location
   *
   * Parameter object for form based search of a location
   */
  class OSMSCOUT_API LocationFormSearchParameter CLASS_FINAL
  {
  private:
    std::string             adminRegionSearchString; //!< The search string to match the admin region name against
    std::string             postalAreaSearchString;  //!< The search string to match the postal area name against
    std::string             locationSearchString;    //!< The search string to match the postal location name against
    std::string             addressSearchString;     //!< The search string to match the address name against

    bool                    adminRegionOnlyMatch;    //!< Evaluate on direct admin region matches
    bool                    postalAreaOnlyMatch;     //!< Evaluate on direct postal area matches
    bool                    locationOnlyMatch;       //!< Evaluate on direct location matches
    bool                    addressOnlyMatch;        //!< Evaluate on direct address matches

    bool                    partialMatch;            //!< Add matches to the result, event if they do not match the complete search string

    StringMatcherFactoryRef stringMatcherFactory;    //!< String matcher factory to use
    size_t                  limit;                   //!< The maximum number of results over all sub searches requested

  public:
    explicit LocationFormSearchParameter();

    std::string GetAdminRegionSearchString() const;
    std::string GetPostalAreaSearchString() const;
    std::string GetLocationSearchString() const;
    std::string GetAddressSearchString() const;

    bool GetAdminRegionOnlyMatch() const;
    bool GetPostalAreaOnlyMatch() const;
    bool GetLocationOnlyMatch() const;
    bool GetAddressOnlyMatch() const;

    bool GetPartialMatch() const;

    StringMatcherFactoryRef GetStringMatcherFactory() const;

    size_t GetLimit() const;

    void SetStringMatcherFactory(const StringMatcherFactoryRef& stringMatcherFactory);

    void SetAdminRegionSearchString(const std::string& adminRegionSearchString);
    void SetPostalAreaSearchString(const std::string& postalAreaSearchString);
    void SetLocationSearchString(const std::string& locationSearchString);
    void SetAddressSearchString(const std::string& addressSearchString);

    void SetAdminRegionOnlyMatch(bool adminRegionOnlyMatch);
    void SetPostalAreaOnlyMatch(bool postalAreaOnlyMatch);
    void SetLocationOnlyMatch(bool locationOnlyMatch);
    void SetAddressOnlyMatch(bool addressOnlyMatch);

    void SetPartialMatch(bool partialMatch);

    void SetLimit(size_t limit);
  };

  /**
   * \ingroup Location
   *
   * Parameter object for string pattern based search for a location or a POI
   */
  class OSMSCOUT_API LocationStringSearchParameter CLASS_FINAL
  {
  private:
    AdminRegionRef          defaultAdminRegion;   //!< A default admin region to use, if no admin region was found based on the search string

    bool                    searchForLocation;    //!< Search for a location
    bool                    searchForPOI;         //!< Search for a POI

    bool                    adminRegionOnlyMatch; //!< Evaluate on direct admin region matches
    bool                    poiOnlyMatch;         //!< Evaluate on direct poi matches
    bool                    locationOnlyMatch;    //!< Evaluate on direct location matches
    bool                    addressOnlyMatch;     //!< Evaluate on direct address matches

    bool                    partialMatch;         //!< Add matches to the result, event if they do not match the complete search string

    std::string             searchString;         //!< The search string itself, must bot be empty
    StringMatcherFactoryRef stringMatcherFactory; //!< String matcher factory to use

    size_t                  limit;                //!< The maximum number of results over all sub searches requested

  public:
    explicit LocationStringSearchParameter(const std::string& searchString);

    AdminRegionRef GetDefaultAdminRegion() const;

    bool GetSearchForLocation() const;
    bool GetSearchForPOI() const;

    bool GetAdminRegionOnlyMatch() const;
    bool GetPOIOnlyMatch() const;
    bool GetLocationOnlyMatch() const;
    bool GetAddressOnlyMatch() const;

    bool GetPartialMatch() const;

    std::string GetSearchString() const;

    StringMatcherFactoryRef GetStringMatcherFactory() const;

    size_t GetLimit() const;

    void SetDefaultAdminRegion(const AdminRegionRef& adminRegion);

    void SetSearchForLocation(bool searchForLocation);
    void SetSearchForPOI(bool searchForPOI);

    void SetAdminRegionOnlyMatch(bool adminRegionOnlyMatch);
    void SetPOIOnlyMatch(bool poiOnlyMatch);
    void SetLocationOnlyMatch(bool locationOnlyMatch);
    void SetAddressOnlyMatch(bool addressOnlyMatch);

    void SetPartialMatch(bool partialMatch);

    void SetStringMatcherFactory(const StringMatcherFactoryRef& stringMatcherFactory);

    void SetLimit(size_t limit);
  };

  /**
   * \ingroup Location
   *
   * The result of a location query
   */
  class OSMSCOUT_API LocationSearchResult
  {
  public:

    enum MatchQuality {
      match     = 1,
      candidate = 2,
      none      = 3
    };

    class OSMSCOUT_API Entry
    {
    public:
      AdminRegionRef adminRegion;
      MatchQuality   adminRegionMatchQuality;
      PostalAreaRef  postalArea;
      MatchQuality   postalAreaMatchQuality;
      LocationRef    location;
      MatchQuality   locationMatchQuality;
      POIRef         poi;
      MatchQuality   poiMatchQuality;
      AddressRef     address;
      MatchQuality   addressMatchQuality;

      bool operator<(const Entry& other) const;
      bool operator==(const Entry& other) const;
    };

  public:
    std::list<Entry> results;
    bool             limitReached;
  };

  /**
   * \ingroup Service
   * \ingroup Location
   * The LocationService offers a number of methods for location lookup
   * ( search for a certain location by its name) and location reverse lookup
   * (retrieve the name of a location).
   *
   * The support different type of requests for different interfaces
   * the visitor pattern is used.
   *
   * Currently the following functionalities are supported:
   * - Visit all region (recursivly)
   * - Visit all locations of a region and (optionally) all locations of all
   *   sub regions.
   * - Visit all addresses of a location (non recursive)
   * - Resolve all parent regions for a given region
   * - General interface for location lookup, offering default visitors for the
   *   individual index traversals.
   * - Retrieve the addresses of one or more objects.
   */
  class OSMSCOUT_API LocationService
  {
  private:
    DatabaseRef database;

  public:
    explicit LocationService(const DatabaseRef& database);

    bool VisitAdminRegions(AdminRegionVisitor& visitor) const;

    bool ResolveAdminRegionHierachie(const AdminRegionRef& adminRegion,
                                     std::map<FileOffset,AdminRegionRef >& refs) const;

    bool VisitAdminRegionLocations(const AdminRegion& region,
                                   const PostalArea& postalArea,
                                   LocationVisitor& visitor) const;

    bool VisitAdminRegionPOIs(const AdminRegion& region,
                              POIVisitor& visitor) const;

    bool VisitLocationAddresses(const AdminRegion& region,
                                const PostalArea& postalArea,
                                const Location& location,
                                AddressVisitor& visitor) const;

    bool SearchForLocationByString(const LocationStringSearchParameter& searchParameter,
                                   LocationSearchResult& result) const;

    bool SearchForLocationByForm(const LocationFormSearchParameter& searchParameter,
                                 LocationSearchResult& result) const;

    bool SearchForPOIByForm(const POIFormSearchParameter& searchParameter,
                            LocationSearchResult& result) const;

  };

  //! \ingroup Service
  //! \ingroup Location
  //! Reference counted reference to a location service instance
  typedef std::shared_ptr<LocationService> LocationServiceRef;
}


#endif
