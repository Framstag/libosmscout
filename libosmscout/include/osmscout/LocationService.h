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

#include <osmscout/Database.h>
#include <osmscout/Location.h>

#include <osmscout/util/Reference.h>

namespace osmscout {

  /**
   * \ingroup Location
   *
   * Object holding a search request for to lookup one
   * or more locations based on search patterns for the
   * region, the location and a address.
   */
  class OSMSCOUT_API LocationSearch
  {
  public:
    /**
     * \ingroup Location
     *
     * One singular name pattern match query
     */
    class OSMSCOUT_API Entry
    {
    public:
      std::string adminRegionPattern; //!< name pattern, the admin region must match, empty if no filtering by admin region requested
      std::string locationPattern;    //!< name pattern, the location must match, empty if no filtering by location requested
      std::string addressPattern;     //!< name pattern, the address must match, empty if no filtering by address requested
    };

  public:
    std::list<Entry> searches; //!< List of search entries, the queries are OR'ed
    size_t           limit;    //!< The maximum number of results over all sub searches requested

    LocationSearch();
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
  class OSMSCOUT_API LocationService : public Referencable
  {
  private:
    /**
     * \ingroup Location
     *
     * Matching algorithm that can compare names regardless of their case.
     * Defined abstract because this function is used in the classes
     * \see AdminRegionVisitor
     * \see LocationMatchVisitor
     * \see AddressMatchVisitor
     */
     class VisitorMatcher
     {
     public:
       VisitorMatcher(const std::string& pattern);

     protected:
       std::string              pattern;

     protected:
       void Match(const std::string& name,
                  bool& match,
                  bool& candidate) const;

     private:
       void TolowerUmlaut(std::string& s) const;
     };

    class AdminRegionMatchVisitor : public AdminRegionVisitor, public VisitorMatcher
    {
    public:
      class AdminRegionResult
      {
      public:
        AdminRegionRef adminRegion;
        bool           isMatch;
      };

    private:
      size_t                       limit;

    public:
      std::list<AdminRegionResult> results;
      bool                         limitReached;

    public:
      AdminRegionMatchVisitor(const std::string& pattern,
                              size_t limit);

      Action Visit(const AdminRegion& region);
    };

    /**
     * \ingroup Location
     *
     * Visitor that gets called for every location found in the given region.
     * It is the task of the visitor to decide if a location matches the given criteria.
     */
    class LocationMatchVisitor : public LocationVisitor, public VisitorMatcher
    {
    public:
      class POIResult
      {
      public:
        AdminRegionRef adminRegion;
        POIRef         poi;
        bool           isMatch;
      };

      class LocationResult
      {
      public:
        AdminRegionRef adminRegion;
        LocationRef    location;
        bool           isMatch;
      };

    private:
      size_t              limit;

    public:
      AdminRegionRef            adminRegion;
      std::list<POIResult>      poiResults;
      std::list<LocationResult> locationResults;
      bool                      limitReached;

    public:
      LocationMatchVisitor(const AdminRegionRef& adminRegion,
                           const std::string& pattern,
                           size_t limit);

      bool Visit(const AdminRegion& adminRegion,
                 const POI &poi);
      bool Visit(const AdminRegion& adminRegion,
                 const Location &location);
    };

    /**
     * \ingroup Location
     *
     */
    class AddressMatchVisitor : public AddressVisitor, public VisitorMatcher
    {
    public:
      class AddressResult
      {
      public:
        AdminRegionRef adminRegion;
        LocationRef    location;
        AddressRef     address;
        bool           isMatch;
      };

    private:
      size_t                   limit;

    public:
      std::list<AddressResult> results;
      bool                     limitReached;

    public:
      AddressMatchVisitor(const std::string& pattern,
                          size_t limit);

      bool Visit(const AdminRegion& adminRegion,
                 const Location& location,
                 const Address& address);
    };

  public:
    /**
     * \ingroup Location
     *
     * Result of a location reverse lookup
     */
    struct OSMSCOUT_API ReverseLookupResult
    {
      ObjectFileRef  object;      //!< object used for lookup
      AdminRegionRef adminRegion; //!< Region the object is in, if set
      POIRef         poi;         //!< POI data, if set
      LocationRef    location;    //!< Location data, if set
      AddressRef     address;     //!< Address data if set
    };

  private:
    DatabaseRef database;

  private:
    bool HandleAdminRegion(const LocationSearch& search,
                           const LocationSearch::Entry& searchEntry,
                           const AdminRegionMatchVisitor::AdminRegionResult& adminRegionResult,
                           LocationSearchResult& result) const;

    bool HandleAdminRegionLocation(const LocationSearch& search,
                                   const LocationSearch::Entry& searchEntry,
                                   const AdminRegionMatchVisitor::AdminRegionResult& adminRegionResult,
                                   const LocationMatchVisitor::LocationResult& locationResult,
                                   LocationSearchResult& result) const;

    bool HandleAdminRegionPOI(const LocationSearch& search,
                              const AdminRegionMatchVisitor::AdminRegionResult& adminRegionResult,
                              const LocationMatchVisitor::POIResult& poiResult,
                              LocationSearchResult& result) const;

    bool HandleAdminRegionLocationAddress(const LocationSearch& search,
                                          const AdminRegionMatchVisitor::AdminRegionResult& adminRegionResult,
                                          const LocationMatchVisitor::LocationResult& locationResult,
                                          const AddressMatchVisitor::AddressResult& addressResult,
                                          LocationSearchResult& result) const;

  public:
    LocationService(const DatabaseRef& database);

    bool VisitAdminRegions(AdminRegionVisitor& visitor) const;

    bool VisitAdminRegionLocations(const AdminRegion& region,
                                   LocationVisitor& visitor) const;

    bool VisitLocationAddresses(const AdminRegion& region,
                                const Location& location,
                                AddressVisitor& visitor) const;

    bool ResolveAdminRegionHierachie(const AdminRegionRef& adminRegion,
                                     std::map<FileOffset,AdminRegionRef >& refs) const;

    bool InitializeLocationSearchEntries(const std::string& searchPattern,
                                         LocationSearch& search);

    bool SearchForLocations(const LocationSearch& search,
                            LocationSearchResult& result) const;

    bool ReverseLookupObjects(const std::list<ObjectFileRef>& objects,
                              std::list<ReverseLookupResult>& result) const;
    bool ReverseLookupObject(const ObjectFileRef& object,
                              std::list<ReverseLookupResult>& result) const;
  };

  //! \ingroup Service
  //! Reference counted reference to a location service instance
  typedef Ref<LocationService> LocationServiceRef;
}


#endif
