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
   * Description of a location based on the GeoCoord of that location.
   */
  class OSMSCOUT_API LocationCoordDescription CLASS_FINAL
  {
  private:
    GeoCoord location;

  public:
    explicit LocationCoordDescription(const GeoCoord& location);

    GeoCoord GetLocation() const;
  };

  //! \ingroup Location
  //! Reference counted reference to a LocationCoordDescription instance
  typedef std::shared_ptr<LocationCoordDescription> LocationCoordDescriptionRef;

  class OSMSCOUT_API LocationDescriptionCandicate CLASS_FINAL
  {
  private:
    ObjectFileRef ref;      //!< Reference to the actual object
    std::string   name;     //!< Name of the object
    double        distance; //!< Distance to the object
    double        bearing;  //!< Direction towards the object
    bool          atPlace;  //!< We are at the place or only near the place
    double        size;     //!< The size o the place (size of the geographic bounding box)

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
      // no code
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
  class OSMSCOUT_API LocationAtPlaceDescription CLASS_FINAL
  {
  private:
    Place  place;     //!< Place
    bool   atPlace;   //!< 'true' if at the place itself
    double distance;  //!< distance to the place
    double bearing;   //!< bearing to take from place to reach location

  public:
    explicit LocationAtPlaceDescription(const Place& place);
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
   * Description of a location based on a nearby way
   */
  class OSMSCOUT_API LocationWayDescription CLASS_FINAL
  {
  private:
    Place  way;      //!< the nearest way
    double distance; //!< distance to the way

  public:
    LocationWayDescription(const Place& way);

    LocationWayDescription(const Place& way,
                           double distance);
    /**
     * Return the place this information is refering to
     */
    inline const Place GetWay() const
    {
      return way;
    }

    /**
     * Return the distance to the location in meter
     */
    inline double GetDistance() const
    {
      return distance;
    }
  };
  
  //! \ingroup Location
  //! Reference counted reference to a LocationWayDescription instance
  typedef std::shared_ptr<LocationWayDescription> LocationWayDescriptionRef;

  /**
   * \ingroup Location
   *
   * Description of a location based on a nearby crossing
   */
  class OSMSCOUT_API LocationCrossingDescription CLASS_FINAL
  {
  private:
    GeoCoord         crossing;  //!< The coordinates of the crossing
    bool             atPlace;   //!< 'true' if at the place itself
    std::list<Place> ways;      //!< List of streets
    double           distance;  //!< distance to the place
    double           bearing;   //!< bearing to take from place to reach location

  public:
    LocationCrossingDescription(const GeoCoord& crossing,
                               const std::list<Place>& ways);

    LocationCrossingDescription(const GeoCoord& crossing,
                                const std::list<Place>& ways,
                                double distance,
                                double bearing);
    /**
     * Return the place this information is refering to
     */
    inline const std::list<Place> GetWays() const
    {
      return ways;
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

    inline GeoCoord GetCrossing() const
    {
      return crossing;
    }
  };
  
  //! \ingroup Location
  //! Reference counted reference to a LocationCrossingDescription instance
  typedef std::shared_ptr<LocationCrossingDescription> LocationCrossingDescriptionRef;

  /**
   * \ingroup Location
   *
   * A LocationDescription objects holds various alternative (and optional) descriptions
   * of the given locations.
   */
  class OSMSCOUT_API LocationDescription CLASS_FINAL
  {
  private:
    LocationCoordDescriptionRef    coordDescription;
    LocationAtPlaceDescriptionRef  atNameDescription;
    LocationAtPlaceDescriptionRef  atAddressDescription;
    LocationAtPlaceDescriptionRef  atPOIDescription;
    LocationWayDescriptionRef      wayDescription;
    LocationCrossingDescriptionRef crossingDescription;

  public:
    void SetCoordDescription(const LocationCoordDescriptionRef& description);
    void SetAtNameDescription(const LocationAtPlaceDescriptionRef& description);
    void SetAtAddressDescription(const LocationAtPlaceDescriptionRef& description);
    void SetAtPOIDescription(const LocationAtPlaceDescriptionRef& description);
    void SetWayDescription(const LocationWayDescriptionRef& description);
    void SetCrossingDescription(const LocationCrossingDescriptionRef& description);

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

    /**
     * Return the location in relation to a close way
     * @return
     */
    LocationWayDescriptionRef GetWayDescription() const;

    /**
     * Return the location in relation to a close crossing
     * @return
     */
    LocationCrossingDescriptionRef GetCrossingDescription() const;
  };

  /**
   * \ingroup Location
   *
   * Object holding a search request for to lookup one
   * or more locations based on search patterns for the
   * region, the location and a address.
   */
  class OSMSCOUT_API LocationSearch CLASS_FINAL
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
      std::string postalCodePattern;  //!< name pattern, the admin region must match, empty if no filtering by admin region requested
      std::string locationPattern;    //!< name pattern, the location must match, empty if no filtering by location requested
      std::string addressPattern;     //!< name pattern, the address must match, empty if no filtering by address requested
    };

  public:
    std::list<Entry> searches; //!< List of search entries, the queries are OR'ed
    size_t           limit;    //!< The maximum number of results over all sub searches requested

    LocationSearch();
  };

  class OSMSCOUT_API POIFormSearchParameter CLASS_FINAL
  {
  private:
    std::string             adminRegionSearchString; //!< The search string to match the admin region name against
    std::string             poiSearchString;         //! The search string to match the postal area name against

    bool                    adminRegionOnlyMatch;    //!< Evaluate on direct admin region matches
    bool                    poiOnlyMatch;            //!< Evaluate on direct poi matches

    StringMatcherFactoryRef stringMatcherFactory; //!< String matcher factory to use

    size_t                  limit;                //!< The maximum number of results over all sub searches requested

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

    void SetLimit(size_t limit);
  };

  /**
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
       explicit VisitorMatcher(const std::string& pattern);

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
      AdminRegionMatchVisitor(const std::string& adminRegionPattern,
                              size_t limit);

      Action Visit(const AdminRegion& region) override;
    };

    class PostalAreaMatchVisitor
    {
    public:
      class PostalAreaResult
      {
      public:
        AdminRegionRef adminRegion;
        PostalAreaRef  postalArea;
        bool           isMatch;
      };

    private:
      AdminRegionRef              adminRegion;
      std::string                 postalAreaPattern;
      size_t                      limit;

    public:
      std::list<PostalAreaResult> results;
      bool                        limitReached;

    public:
      PostalAreaMatchVisitor(const AdminRegionRef& adminRegion,
                             const std::string& postalAreaPattern,
                             size_t limit);

      bool Visit(const PostalArea& postalArea);
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
      class Result
      {
      public:
        AdminRegionRef adminRegion;
        PostalAreaRef  postalArea;
        LocationRef    location;
        bool           isMatch;
      };

    private:
      size_t            limit;

    public:
      std::list<Result> results;
      bool              limitReached;

    public:
      LocationMatchVisitor(const std::string& pattern,
                           size_t limit);

      bool Visit(const AdminRegion& adminRegion,
                 const PostalArea& postalArea,
                 const Location &location) override;
    };

    /**
     * \ingroup Location
     *
     * Visitor that gets called for every location found in the given region.
     * It is the task of the visitor to decide if a location matches the given criteria.
     */
    class POIMatchVisitor : public POIVisitor, public VisitorMatcher
    {
    public:
      class Result
      {
      public:
        AdminRegionRef adminRegion;
        POIRef         poi;
        bool           isMatch;
      };

    private:
      size_t            limit;

    public:
      AdminRegionRef    adminRegion;
      std::list<Result> results;
      bool              limitReached;

    public:
      POIMatchVisitor(const AdminRegionRef& adminRegion,
                      const std::string& pattern,
                      size_t limit);

      bool Visit(const AdminRegion& adminRegion,
                 const POI &poi) override;
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
        PostalAreaRef  postalArea;
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
                 const PostalArea& postalArea,
                 const Location& location,
                 const Address& address) override;
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
      PostalAreaRef  postalArea;  //!< Postal area the object is in, if set
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
                                   const PostalAreaMatchVisitor::PostalAreaResult& postalAreaResult,
                                   const LocationMatchVisitor::Result& locationResult,
                                   LocationSearchResult& result) const;

    bool HandleAdminRegionPOI(const LocationSearch& search,
                              const AdminRegionMatchVisitor::AdminRegionResult& adminRegionResult,
                              const POIMatchVisitor::Result& poiResult,
                              LocationSearchResult& result) const;

    bool HandleAdminRegionLocationAddress(const LocationSearch& search,
                                          const AdminRegionMatchVisitor::AdminRegionResult& adminRegionResult,
                                          const PostalAreaMatchVisitor::PostalAreaResult& postalAreaResult,
                                          const LocationMatchVisitor::Result& locationResult,
                                          const AddressMatchVisitor::AddressResult& addressResult,
                                          LocationSearchResult& result) const;

  public:
    explicit LocationService(const DatabaseRef& database);

    bool VisitAdminRegions(AdminRegionVisitor& visitor) const;

    bool VisitAdminRegionLocations(const AdminRegion& region,
                                   const PostalArea& postalArea,
                                   LocationVisitor& visitor) const;

    bool VisitAdminRegionPOIs(const AdminRegion& region,
                              POIVisitor& visitor) const;

    bool VisitLocationAddresses(const AdminRegion& region,
                                const PostalArea& postalArea,
                                const Location& location,
                                AddressVisitor& visitor) const;

    bool ResolveAdminRegionHierachie(const AdminRegionRef& adminRegion,
                                     std::map<FileOffset,AdminRegionRef >& refs) const;

    bool InitializeLocationSearchEntries(const std::string& searchPattern,
                                         LocationSearch& search);

    bool SearchForLocationByString(const LocationStringSearchParameter& searchParameter,
                                   LocationSearchResult& result) const;

    bool SearchForLocationByForm(const LocationFormSearchParameter& searchParameter,
                                 LocationSearchResult& result) const;

    bool SearchForPOIByForm(const POIFormSearchParameter& searchParameter,
                            LocationSearchResult& result) const;

    bool SearchForLocations(const LocationSearch& search,
                            LocationSearchResult& result) const;

    bool ReverseLookupRegion(const GeoCoord &coord,
                             std::list<ReverseLookupResult>& result) const;

    bool ReverseLookupObjects(const std::list<ObjectFileRef>& objects,
                              std::list<ReverseLookupResult>& result) const;
    bool ReverseLookupObject(const ObjectFileRef& object,
                              std::list<ReverseLookupResult>& result) const;

    bool DescribeLocation(const GeoCoord& location,
                          LocationDescription& description,
                          const double lookupDistance=100,
                          const double sizeFilter=1.0);

    /**
     * @see LoadNearAreas
     */
    bool LoadNearNodes(const GeoCoord& location,
                       const TypeInfoSet &types,
                       std::vector<LocationDescriptionCandicate> &candidates,
                       const double maxDistance=100);

    /**
     * @see LoadNearAreas
     */
    bool LoadNearWays(const GeoCoord& location,
                      const TypeInfoSet &types,
                      std::vector<WayRef> &candidates,
                      const double maxDistance=100);

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

    bool DescribeLocationByCrossing(const GeoCoord& location,
                                    LocationDescription& description,
                                    const double lookupDistance=100);

    bool DescribeLocationByWay(const GeoCoord& location,
                               LocationDescription& description,
                               const double lookupDistance=100);
  };

  //! \ingroup Service
  //! Reference counted reference to a location service instance
  typedef std::shared_ptr<LocationService> LocationServiceRef;
}


#endif
