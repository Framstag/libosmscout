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
}

