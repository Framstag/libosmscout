#ifndef OSMSCOUT_TEST_REGIONLIST_H
#define OSMSCOUT_TEST_REGIONLIST_H

/*
  This source is part of the libosmscout-test library
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
#include <string>

#include <osmscout/private/TestImportExport.h>

namespace osmscout {
  namespace test {

    enum class PlaceType {
      region,
      county,
      city,
      suburb,
      unknown
    };

    class OSMSCOUT_TEST_API Tag
    {
    private:
      std::string key;
      std::string value;

    public:
      Tag(const std::string& key,
          const std::string& value)
        : key(key),
          value(value)
      {

      }
      inline std::string GetKey() const
      {
        return key;
      }
      inline std::string GetValue() const
      {
        return value;
      }
    };

    class OSMSCOUT_TEST_API Address
    {
    private:
      std::string    name;
      std::list<Tag> tags;

    public:
      inline void SetName(const std::string& name)
      {
        this->name=name;
      }

      inline std::string GetName() const
      {
        return name;
      }

      inline void AddTag(const std::string& key,
                         const std::string& value)
      {
        tags.emplace_back(key,value);
      }

      inline const std::list<Tag>& GetTags() const
      {
        return tags;
      }
    };

    typedef std::shared_ptr<Address> AddressRef;

    class OSMSCOUT_TEST_API Location
    {
    private:
      std::string           name;
      std::list<AddressRef> addresses;

    public:
      inline void SetName(const std::string& name)
      {
        this->name=name;
      }

      inline void AddAddress(const AddressRef& address)
      {
        addresses.push_back(address);
      }

      inline std::string GetName() const
      {
        return name;
      }

      inline const std::list<AddressRef>& GetAddresses() const
      {
        return addresses;
      }
    };

    typedef std::shared_ptr<Location> LocationRef;

    class OSMSCOUT_TEST_API PostalArea
    {
    private:
      std::string            name;
      std::list<LocationRef> locations;

    public:
      inline void SetName(const std::string& name)
      {
        this->name=name;
      }

      inline void AddLocation(const LocationRef& location)
      {
        locations.push_back(location);
      }

      inline std::string GetName() const
      {
        return name;
      }

      inline const std::list<LocationRef>& GetLocations() const
      {
        return locations;
      }
    };

    typedef std::shared_ptr<PostalArea> PostalAreaRef;

    class Region;
    typedef std::shared_ptr<Region> RegionRef;

    class OSMSCOUT_TEST_API Region
    {
    private:
      PlaceType                placeType;
      std::string              name;
      bool                     isBoundary;
      bool                     isNode;
      size_t                   adminLevel;
      std::list<PostalAreaRef> postalAreas;
      std::list<RegionRef>     regions;

    public:
      explicit Region()
      : placeType(PlaceType::unknown),
        isBoundary(false),
        isNode(false),
        adminLevel(0)
      {
      }

      inline void SetPlaceType(PlaceType placeType)
      {
        this->placeType=placeType;
      }

      inline void SetAdminLevel(size_t adminLevel)
      {
        isBoundary=true;
        this->adminLevel=adminLevel;
      }

      inline void SetIsNode()
      {
        isNode=true;
      }

      inline void SetName(const std::string& name)
      {
        this->name=name;
      }

      inline void AddPostalArea(const PostalAreaRef& postalArea)
      {
        postalAreas.push_back(postalArea);
      }

      inline void AddRegion(const RegionRef& region)
      {
        regions.push_back(region);
      }

      inline PlaceType GetPlaceType() const
      {
        return placeType;
      }

      inline std::string GetName() const
      {
        return name;
      }

      inline bool IsBoundary() const
      {
        return isBoundary;
      }

      inline bool IsIsNode() const
      {
        return isNode;
      }

      inline size_t GetAdminLevel() const
      {
        return adminLevel;
      }

      inline const std::list<PostalAreaRef>& GetPostalAreas() const
      {
        return postalAreas;
      }

      inline const std::list<RegionRef>& GetRegionList() const
      {
        return regions;
      }
    };

    class OSMSCOUT_TEST_API RegionList
    {
    private:
      std::list<RegionRef> regions;

    public:
      inline void AddRegion(const RegionRef& region)
      {
        regions.push_back(region);
      }

      inline const std::list<RegionRef>& GetRegionList() const
      {
        return regions;
      }
    };
  }
}

#endif
