#ifndef OSMSCOUT_REFERENCE_H
#define OSMSCOUT_REFERENCE_H

/*
  Import/TravelJinni - Openstreetmap offline viewer
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

#include <osmscout/TypeConfig.h>

enum RefType
{
  refNone     = 0,
  refNode     = 1,
  refWay      = 2,
  refArea     = 3,
  refRelation = 4
};

class Reference
{
public:
  Id      id;
  RefType type;

public:
  inline Reference()
  : id(0),
    type(refNone)
  {
    // no code
  }

  inline Reference(Id id, RefType type)
  : id(id),
    type(type)
  {
    // no code
  }

  inline void Set(const Id& id, const RefType& type)
  {
    this->id=id;
    this->type=type;
  }

  inline const Id& GetId() const
  {
    return id;
  }

  inline const RefType& GetType() const
  {
    return type;
  }

  inline bool operator<(const Reference& reference) const
  {
    return type<reference.type || (type==reference.type &&  id<reference.id);
  }
};

#endif
