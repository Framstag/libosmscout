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

#include <osmscout/Location.h>

#include <sstream>

#include <osmscout/util/String.h>

namespace osmscout {

  bool AdminRegion::Match(const ObjectFileRef& object) const
  {
    if (this->object==object) {
      return true;
    }

    if (aliasObject==object) {
      return true;
    }

    if (object.GetType()==refNode) {
      for (std::vector<AdminRegion::RegionAlias>::const_iterator alias=aliases.begin();
          alias!=aliases.end();
          ++alias) {
        if (alias->objectOffset==object.GetFileOffset()) {
          return true;
        }
      }
    }

    return false;
  }

  AdminRegionVisitor::~AdminRegionVisitor()
  {
    // no code
  }

  LocationVisitor::~LocationVisitor()
  {
    // no code
  }

  AddressVisitor::~AddressVisitor()
  {
    // no code
  }

  AddressListVisitor::AddressListVisitor(size_t limit)
  : limit(limit),
    limitReached(false)
  {
    // no code
  }

  bool AddressListVisitor::Visit(const AdminRegion& adminRegion,
                                 const Location& location,
                                 const Address& address)
  {
    AddressResult result;

    result.adminRegion=std::make_shared<AdminRegion>(adminRegion);
    result.location=std::make_shared<Location>(location);
    result.address=std::make_shared<Address>(address);

    results.push_back(result);

    limitReached=results.size()>=limit;

    return !limitReached;
  }

  Place::Place(const ObjectFileRef& object,
               const FeatureValueBufferRef objectFeatures,
               const AdminRegionRef& adminRegion,
               const POIRef& poi,
               const LocationRef& location,
               const AddressRef& address)
  : object(object),
    objectFeatures(objectFeatures),
    adminRegion(adminRegion),
    poi(poi),
    location(location),
    address(address)
  {
    // no oce
  }

  /**
   * Returns a textual representation of the place in
   * @return
   */
  std::string Place::GetDisplayString() const
  {
    std::ostringstream stream;
    bool               empty=true;

    stream.imbue(std::locale());

    if (poi) {
      stream << UTF8StringToLocaleString(poi->name);
      empty=false;
    }

    if (location && address) {
      if (!empty) {
        stream << ", ";
      }

      stream << UTF8StringToLocaleString(location->name) << " " << UTF8StringToLocaleString(address->name);
      empty=false;
    }
    else if (location) {
      if (!empty) {
        stream << ", ";
      }

      stream << UTF8StringToLocaleString(location->name);
      empty=false;
    }
    else if (address) {
      if (!empty) {
        stream << ", ";
      }

      stream << UTF8StringToLocaleString(address->name);
      empty=false;
    }

    if (adminRegion) {
      if (!empty) {
        stream << ", ";
      }

      if (!adminRegion->aliasName.empty()) {
        stream << UTF8StringToLocaleString(adminRegion->aliasName);
      }
      else {
        stream << UTF8StringToLocaleString(adminRegion->name);
      }
    }

    return stream.str();
  }
}

