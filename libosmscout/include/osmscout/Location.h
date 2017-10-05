#ifndef OSMSCOUT_LOCATION_H
#define OSMSCOUT_LOCATION_H

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
#include <vector>

#include <osmscout/ObjectRef.h>
#include <osmscout/TypeConfig.h>

namespace osmscout {
  /**
   * \defgroup Location Location related data structures and services
   *
   * Classes and methods for handling location aspects of object
   * in the libosmscout database.
   */

  class OSMSCOUT_API PostalArea
  {
  public:
    std::string name;         //!< Name of the postal area
    FileOffset  objectOffset; //!< Offset of the postal area data
  };

  typedef std::shared_ptr<PostalArea> PostalAreaRef;

  /**
   \ingroup Location
   A named administrative region. It is used to build up hierarchical,
   structured containment information like "Streets in City". Most of
   the time an administrative region is just the area of a city, but
   depending on the data quality it may also be parts of the city
   or bigger administrative regions like states.

   AdminRegions are currently returned by
   Database.GetMatchingAdminRegions() using the CityStreetIndex.
   */
  class OSMSCOUT_API AdminRegion
  {
  public:
    class OSMSCOUT_API RegionAlias
    {
    public:
      std::string name;         //!< Alias
      FileOffset  objectOffset; //!< Node data offset of the alias
    };

    FileOffset               regionOffset;       //!< Offset of this entry in the index
    FileOffset               dataOffset;         //!< Offset of the data part of this entry
    FileOffset               parentRegionOffset; //!< Offset of the parent region index entry
    std::string              name;               //!< name of the region
    ObjectFileRef            object;             //!< The object that represents this region
    std::string              aliasName;          //!< Additional optional alias name
    ObjectFileRef            aliasObject;        //!< Additional optional alias reference
    std::vector<RegionAlias> aliases;            //!< The list of alias for this region
    std::vector<PostalArea>  postalAreas;        //<! The list of postal areas
    std::vector<FileOffset>  childrenOffsets;    //!< The list of child region offset

  public:
    bool Match(const ObjectFileRef& object) const;
  };

  typedef std::shared_ptr<AdminRegion> AdminRegionRef;

  /**
   * \ingroup Location
   * Visitor that gets called for every region found.
   * It is the task of the visitor to decide if a region matches the given criteria.
   */
  class OSMSCOUT_API AdminRegionVisitor
  {
  public:
    enum Action {
      //! Do not visit child regions, but continue with traversal
      skipChildren,
      //! Visit child regions
      visitChildren,
      //! Stop
      stop,
      //! Signal an error
      error
    };

  public:
    virtual ~AdminRegionVisitor() = default;

    virtual Action Visit(const AdminRegion& region) = 0;
  };

  /**
   * \ingroup Location
   * A POI is an object within an area, which has been indexed by
   * its name.
   */
  class OSMSCOUT_API POI
  {
  public:
    FileOffset    regionOffset; //!< Offset of the region this location is in
    std::string   name;         //!< name of the POI
    ObjectFileRef object;       //!< Reference to the object
  };

  typedef std::shared_ptr<POI> POIRef;

  /**
   * \ingroup Location
   * Visitor that gets called for every POI found in the given area.
   * It is the task of the visitor to decide if a locations matches the given criteria.
   */
  class OSMSCOUT_API POIVisitor
  {
  public:
    virtual ~POIVisitor() = default;

    virtual bool Visit(const AdminRegion& adminRegion,
                       const POI &poi) = 0;
  };

  /**
    \ingroup Location
    A location is a named point, way, area or relation on the map.
    Something you can search for. Location are currently returned
    by Database.GetMatchingLocations() which uses CityStreetIndex
    internally.
   */
  class OSMSCOUT_API Location
  {
  public:
    FileOffset                 locationOffset;  //!< Offset to location
    FileOffset                 regionOffset;    //!< Offset of the admin region this location is in
    FileOffset                 addressesOffset; //!< Offset to the list of addresses
    std::string                name;            //!< name of the location
    std::vector<ObjectFileRef> objects;         //!< List of objects that build up this location
  };

  typedef std::shared_ptr<Location> LocationRef;

  /**
   * \ingroup Location
   * Visitor that gets called for every location found in the given area.
   * It is the task of the visitor to decide if a locations matches the given criteria.
   */
  class OSMSCOUT_API LocationVisitor
  {
  public:
    virtual ~LocationVisitor() = default;

    virtual bool Visit(const AdminRegion& adminRegion,
                       const PostalArea& postalArea,
                       const Location &location) = 0;
  };

  /**
    \ingroup Location
    An address is a unique place at a given location, normally a building that
    is address by its house number.
   */
  class OSMSCOUT_API Address
  {
  public:
    FileOffset    addressOffset;  //!< Offset of the address entry
    FileOffset    locationOffset; //!< Offset to location
    FileOffset    regionOffset;   //!< Offset of the admin region this location is in
    std::string   name;           //!< name of the address
    ObjectFileRef object;         //!< Object that represents the address
  };

  typedef std::shared_ptr<Address> AddressRef;

  /**
   * \ingroup Location
   * Visitor that gets called for every address found at a given location.
   * It is the task of the visitor to decide if a address matches the given criteria.
   */
  class OSMSCOUT_API AddressVisitor
  {
  public:
    virtual ~AddressVisitor() = default;

    virtual bool Visit(const AdminRegion& adminRegion,
                       const PostalArea& postalArea,
                       const Location& location,
                       const Address& address) = 0;
  };

  /**
   * \ingroup Location
   */
  class OSMSCOUT_API AddressListVisitor : public AddressVisitor
  {
  public:
    class OSMSCOUT_API AddressResult
    {
    public:
      AdminRegionRef adminRegion; //!< The admin region the address is contained by
      PostalAreaRef  postalArea;  //!< The postal area
      LocationRef    location;    //!< The location the address belongs to
      AddressRef     address;     //!< The address itself
    };

  private:
    size_t                   limit;

  public:
    std::list<AddressResult> results;
    bool                     limitReached;

  public:
    AddressListVisitor(size_t limit);

    bool Visit(const AdminRegion& adminRegion,
               const PostalArea& postalArea,
               const Location& location,
               const Address& address);
  };

  /**
   * \ingroup Location
   *
   * A Place description a certain place in respect to the location index.
   * A place consists on certain optional attributes, that in turn describe
   * certain aspects.
   *
   * * object: A reference to the object that is described by the place
   * * adminRegion: The hierarchical administrative region the object is in (normally the City)
   * * poi: Additional information, if the object is an POI
   * * location: The location the place is at (normally the street)
   * * address: The address of the object in respect to the location (normally the house number)
   */
  class OSMSCOUT_API Place
  {
  private:
    ObjectFileRef         object;         //!< Object the location is in
    FeatureValueBufferRef objectFeatures; //!< Features of the object
    AdminRegionRef        adminRegion;    //!< Region the object is in, if set
    PostalAreaRef         postalArea;     //!< Postal area the object is in, if set
    POIRef                poi;            //!< POI data, if set
    LocationRef           location;       //!< Location data, if set
    AddressRef            address;        //!< Address data if set

  public:
    Place(const ObjectFileRef& object,
          const FeatureValueBufferRef objectFeatureBuff,
          const AdminRegionRef& adminRegion,
          const PostalAreaRef& postalArea,
          const POIRef& poi,
          const LocationRef& location,
          const AddressRef& address);

    inline ObjectFileRef GetObject() const
    {
      return object;
    }

    inline FeatureValueBufferRef GetObjectFeatures() const
    {
      return objectFeatures;
    }

    inline AdminRegionRef GetAdminRegion() const
    {
      return adminRegion;
    }

    inline PostalAreaRef GetPostalArea() const
    {
      return postalArea;
    }

    inline POIRef GetPOI() const
    {
      return poi;
    }

    inline LocationRef GetLocation() const
    {
      return location;
    }


    inline AddressRef GetAddress() const
    {
      return address;
    }

    std::string GetDisplayString() const;
  };
}

#endif
