#ifndef OSMSCOUT_LOCATIONDESCRIPTIONSERVICE_H
#define OSMSCOUT_LOCATIONDESCRIPTIONSERVICE_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2017  Tim Teulings

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

#include <osmscout/TypeInfoSet.h>

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
  using LocationCoordDescriptionRef = std::shared_ptr<LocationCoordDescription>;

  class OSMSCOUT_API LocationDescriptionCandicate CLASS_FINAL
  {
  private:
    ObjectFileRef ref;      //!< Reference to the actual object
    std::string   name;     //!< Name of the object
    Distance      distance; //!< Distance to the object
    Bearing       bearing;  //!< Direction towards the object
    bool          atPlace;  //!< We are at the place or only near the place
    double        size;     //!< The size o the place (size of the geographic bounding box)

  public:
    inline LocationDescriptionCandicate(const ObjectFileRef &ref,
                                        const std::string& name,
                                        const Distance &distance,
                                        const Bearing &bearing,
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

    inline Distance GetDistance() const
    {
      return distance;
    }

    inline Bearing GetBearing() const
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
    Place    place;     //!< Place
    bool     atPlace;   //!< 'true' if at the place itself
    Distance distance;  //!< distance to the place
    Bearing  bearing;   //!< bearing to take from place to reach location

  public:
    explicit LocationAtPlaceDescription(const Place& place);
    LocationAtPlaceDescription(const Place& place,
                               const Distance &distance,
                               const Bearing &bearing);

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
     * Return the distance to the location
     */
    inline Distance GetDistance() const
    {
      return distance;
    }

    /**
     * Return the bearing you have to go to from the place for 'distance' to reach the location
     */
    inline Bearing GetBearing() const
    {
      return bearing;
    }
  };

  //! \ingroup Location
  //! Reference counted reference to a LocationAtPlaceDescription instance
  using LocationAtPlaceDescriptionRef = std::shared_ptr<LocationAtPlaceDescription>;

  /**
   * \ingroup Location
   *
   * Description of a location based on a nearby way
   */
  class OSMSCOUT_API LocationWayDescription CLASS_FINAL
  {
  private:
    Place    way;      //!< the nearest way
    Distance distance; //!< distance to the way

  public:
    explicit LocationWayDescription(const Place& way);

    LocationWayDescription(const Place& way,
                           const Distance &distance);
    /**
     * Return the place this information is refering to
     */
    inline Place GetWay() const
    {
      return way;
    }

    /**
     * Return the distance to the location
     */
    inline Distance GetDistance() const
    {
      return distance;
    }
  };

  //! \ingroup Location
  //! Reference counted reference to a LocationWayDescription instance
  using LocationWayDescriptionRef = std::shared_ptr<LocationWayDescription>;

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
    Distance         distance;  //!< distance to the place
    Bearing          bearing;   //!< bearing to take from place to reach location

  public:
    LocationCrossingDescription(const GeoCoord& crossing,
                                const std::list<Place>& ways);

    LocationCrossingDescription(const GeoCoord& crossing,
                                const std::list<Place>& ways,
                                const Distance &distance,
                                const Bearing &bearing);
    /**
     * Return the place this information is refering to
     */
    inline std::list<Place> GetWays() const
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
     * Return the distance to the location
     */
    inline Distance GetDistance() const
    {
      return distance;
    }

    /**
     * Return the bearing you have to go to from the place for 'distance' to reach the location
     */
    inline Bearing GetBearing() const
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
  using LocationCrossingDescriptionRef = std::shared_ptr<LocationCrossingDescription>;

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
  class OSMSCOUT_API LocationDescriptionService
  {
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

    using ReverseLookupRef = std::shared_ptr<ReverseLookupResult>;

  private:
    DatabaseRef database;

  private:
    static bool DistanceComparator(const LocationDescriptionCandicate &a,
                                   const LocationDescriptionCandicate &b);

    const FeatureValueBufferRef GetObjectFeatureBuffer(const ObjectFileRef &object);

    Place GetPlace(const std::list<ReverseLookupResult>& lookupResult);

    bool VisitAdminRegions(AdminRegionVisitor& visitor) const;

    void AddToCandidates(std::vector<LocationDescriptionCandicate>& candidates,
                         const GeoCoord& location,
                         const NodeRegionSearchResult& results,
                         bool requireAddress,
                         bool requireName);
    void AddToCandidates(std::vector<LocationDescriptionCandicate>& candidates,
                         const GeoCoord& location,
                         const WayRegionSearchResult& results);
    void AddToCandidates(std::vector<LocationDescriptionCandicate>& candidates,
                         const GeoCoord& location,
                         const AreaRegionSearchResult& results,
                         bool requireAddress,
                         bool requireName);

  public:
    explicit LocationDescriptionService(const DatabaseRef& database);

    bool ReverseLookupRegion(const GeoCoord &coord,
                             std::list<ReverseLookupResult>& result) const;

    bool ReverseLookupObjects(const std::list<ObjectFileRef>& objects,
                              std::list<ReverseLookupResult>& result) const;
    bool ReverseLookupObject(const ObjectFileRef& object,
                              std::list<ReverseLookupResult>& result) const;

    bool DescribeLocation(const GeoCoord& location,
                          LocationDescription& description,
                          const Distance& lookupDistance=Distance::Of<Meter>(100),
                          double sizeFilter=1.0);

    bool DescribeLocationByName(const GeoCoord& location,
                                LocationDescription& description,
                                const Distance& lookupDistance=Distance::Of<Meter>(100),
                                double sizeFilter=1.0);

    bool DescribeLocationByAddress(const GeoCoord& location,
                                   LocationDescription& description,
                                   const Distance& lookupDistance=Distance::Of<Meter>(100),
                                   double sizeFilter=1.0);

    bool DescribeLocationByPOI(const GeoCoord& location,
                               LocationDescription& description,
                               const Distance& lookupDistance=Distance::Of<Meter>(100),
                               double sizeFilter=1.0);

    bool DescribeLocationByCrossing(const GeoCoord& location,
                                    LocationDescription& description,
                                    const Distance& lookupDistance=Distance::Of<Meter>(100));

    bool DescribeLocationByWay(const GeoCoord& location,
                               LocationDescription& description,
                               const Distance& lookupDistance=Distance::Of<Meter>(100));
  };

  //! \ingroup Service
  //! Reference counted reference to a location service instance
  using LocationDescriptionServiceRef = std::shared_ptr<LocationDescriptionService>;
}


#endif
