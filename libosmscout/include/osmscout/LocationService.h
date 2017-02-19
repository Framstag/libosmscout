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

namespace osmscout {

  /**
   * \ingroup Location
   *
   * Description of a location based on the GeoCoord of that location.
   */
  class OSMSCOUT_API LocationCoordDescription
  {
  private:
    GeoCoord location;

  public:
    LocationCoordDescription(const GeoCoord& location);

    GeoCoord GetLocation() const;
  };

  //! \ingroup Location
  //! Reference counted reference to a LocationCoordDescription instance
  typedef std::shared_ptr<LocationCoordDescription> LocationCoordDescriptionRef;

  class OSMSCOUT_API LocationDescriptionCandicate
  {
  private:
    ObjectFileRef ref;
    std::string   name;
    double        distance;
    double        bearing;
    bool          atPlace;
    double        size;

  public:
    inline LocationDescriptionCandicate(const ObjectFileRef &ref,
                                        const std::string& name,
                                        const double distance,
                                        const double bearing,
                                        const bool atPlace,
                                        const double size)
    : ref(ref),
      name(name),
      distance(distance),
      bearing(bearing),
      atPlace(atPlace),
      size(size)
    {
    }

    inline ObjectFileRef GetRef() const
    {
      return ref;
    }

    inline std::string GetName() const
    {
      return name;
    }

    inline double GetDistance() const
    {
      return distance;
    }

    inline double GetBearing() const
    {
      return bearing;
    }

    inline bool IsAtPlace() const
    {
      return atPlace;
    }

    inline double GetSize() const
    {
      return size;
    }
  };

  /**
   * \ingroup Location
   *
   * Description of a location based on the GeoCoord of that location.
   */
  class OSMSCOUT_API LocationAtPlaceDescription
  {
  private:
    Place  place;     //!< Place
    bool   atPlace;   //!< 'true' if at the place itself
    double distance;  //!< distance to the place
    double bearing;   //!< bearing to take from place to reach location

  public:
    LocationAtPlaceDescription(const Place& place);
    LocationAtPlaceDescription(const Place& place,
                               double distance,
                               double bearing);

    /**
     * Return the place this information is refering to
     */
    inline Place GetPlace() const
    {
      return place;
    }

    /**
     * 'true' if the location is at the place itself (in spite of 'close to...')
     */
    inline bool IsAtPlace() const
    {
      return atPlace;
    }

    /**
     * Return the distance to the location in meter
     */
    inline double GetDistance() const
    {
      return distance;
    }

    /**
     * Return the bearing you have to go to from the place for 'distance' meter to reach the location
     */
    inline double GetBearing() const
    {
      return bearing;
    }
  };

  //! \ingroup Location
  //! Reference counted reference to a LocationAtPlaceDescription instance
  typedef std::shared_ptr<LocationAtPlaceDescription> LocationAtPlaceDescriptionRef;


  /**
   * \ingroup Location
   *
   * A LocationDescription objects holds various alternative (and optional) descriptions
   * of the given locations.
   */
  class OSMSCOUT_API LocationDescription
  {
  private:
    LocationCoordDescriptionRef   coordDescription;
    LocationAtPlaceDescriptionRef atNameDescription;
    LocationAtPlaceDescriptionRef atAddressDescription;
    LocationAtPlaceDescriptionRef atPOIDescription;

  public:
    void SetCoordDescription(const LocationCoordDescriptionRef& description);
    void SetAtNameDescription(const LocationAtPlaceDescriptionRef& description);
    void SetAtAddressDescription(const LocationAtPlaceDescriptionRef& description);
    void SetAtPOIDescription(const LocationAtPlaceDescriptionRef& description);

    /**
     * Return the location is geo coordinates
     * @return
     */
    LocationCoordDescriptionRef GetCoordDescription() const;

    /**
     * Return the location in relation to a named object
     * @return
     */
    LocationAtPlaceDescriptionRef GetAtNameDescription() const;

    /**
     * Return the location in relation to a close address
     * @return
     */
    LocationAtPlaceDescriptionRef GetAtAddressDescription() const;

    /**
     * Return the location in relation to a close POI
     * @return
     */
    LocationAtPlaceDescriptionRef GetAtPOIDescription() const;
  };

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
  class OSMSCOUT_API LocationService
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
    static bool DistanceComparator(const LocationDescriptionCandicate &a,
                                   const LocationDescriptionCandicate &b);

    const FeatureValueBufferRef GetObjectFeatureBuffer(const ObjectFileRef &object);

    Place GetPlace(const std::list<ReverseLookupResult>& lookupResult);

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

    bool DescribeLocation(const GeoCoord& location,
                          LocationDescription& description);

    /**
     * Load areas of given types near to location.
     * 
     * @param location
     * @param types
     * @param candidates - unsorted result buffer
     * @param maxDistance - lookup distance in meters
     * @return true if no error (it don't indicate non-empty result)
     */
    bool LoadNearAreas(const GeoCoord& location, const TypeInfoSet &types,
                       std::vector<LocationDescriptionCandicate> &candidates,
                       const double maxDistance=100);

    /**
     * @see LoadNearAreas
     */
    bool LoadNearNodes(const GeoCoord& location, const TypeInfoSet &types,
                       std::vector<LocationDescriptionCandicate> &candidates,
                       const double maxDistance=100);

    bool DescribeLocationByName(const GeoCoord& location,
                                LocationDescription& description,
                                const double lookupDistance=100,
                                const double sizeFilter=1.0);

    bool DescribeLocationByAddress(const GeoCoord& location,
                                   LocationDescription& description,
                                   const double lookupDistance=100,
                                   const double sizeFilter=1.0);

    bool DescribeLocationByPOI(const GeoCoord& location,
                               LocationDescription& description,
                               const double lookupDistance=100,
                               const double sizeFilter=1.0);
  };

  //! \ingroup Service
  //! Reference counted reference to a location service instance
  typedef std::shared_ptr<LocationService> LocationServiceRef;
}


#endif
