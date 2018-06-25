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

#include <osmscout/TypeInfoSet.h>

#include <algorithm>

namespace osmscout {

  TypeInfoSet::TypeInfoSet()
  : count(0)
  {
    // no code
  }

  TypeInfoSet::TypeInfoSet(const TypeConfig& typeConfig)
  : count(0)
  {
    types.resize(typeConfig.GetTypeCount());
  }

  TypeInfoSet::TypeInfoSet(const TypeInfoSet& other)
  : types(other.types),
    count(other.count)
  {
    // no code
  }

  TypeInfoSet::TypeInfoSet(TypeInfoSet&& other) noexcept
  : types(std::move(other.types)),
    count(other.count)
  {
    // no code
  }

  TypeInfoSet::TypeInfoSet(const std::vector<TypeInfoRef>& types)
  {
    for (const auto& type : types) {
      Set(type);
    }
  }

  void TypeInfoSet::Adapt(const TypeConfig& typeConfig)
  {
    types.resize(typeConfig.GetTypeCount());
  }

  void TypeInfoSet::Set(const TypeInfoRef& type)
  {
    assert(type);

    if (type->GetIndex()>=types.size()) {
      types.resize(type->GetIndex()+1);
    }

    if (!types[type->GetIndex()]) {
      types[type->GetIndex()]=type;
      count++;
    }
  }

  void TypeInfoSet::Set(const TypeInfoSet& other)
  {
    types=other.types;
    count=other.count;
  }

  void TypeInfoSet::Set(const std::vector<TypeInfoRef>& types)
  {
    Clear();

    for (const auto& type : types) {
      Set(type);
    }
  }

  void TypeInfoSet::Add(const TypeInfoSet& types)
  {
    for (const auto& type : types) {
      Set(type);
    }
  }

  void TypeInfoSet::Remove(const TypeInfoRef& type)
  {
    assert(type);

    if (type->GetIndex()<types.size() &&
        types[type->GetIndex()]) {
      types[type->GetIndex()]=nullptr;
      count--;
    }
  }

  void TypeInfoSet::Remove(const TypeInfoSet& otherTypes)
  {
    for (const auto &type : otherTypes.types)
    {
      if (type &&
          type->GetIndex()<types.size() &&
          types[type->GetIndex()]) {
        types[type->GetIndex()]=nullptr;
        count--;
      }
    }
  }

  void TypeInfoSet::Intersection(const TypeInfoSet& otherTypes)
  {
    for (size_t i=0; i<types.size(); i++) {
      if (types[i] &&
          (i>=otherTypes.types.size() ||
          !otherTypes.types[i])) {
        types[i]=nullptr;
        count--;
      }
    }
  }

  /**
   * Returns 'true' if at least one type is set in both Sets. Else
   * 'false' is returned.
   */
  bool TypeInfoSet::Intersects(const TypeInfoSet& otherTypes) const
  {
    size_t minSize=std::min(types.size(),otherTypes.types.size());

    for (size_t i=0; i<minSize; i++) {
      if (types[i] && otherTypes.types[i]) {
        return true;
      }
    }

    return false;
  }

  bool TypeInfoSet::operator==(const TypeInfoSet& other) const
  {
    if (this==&other) {
      return true;
    }

    if (count!=other.count) {
      return false;
    }

    for (size_t i=0; i<std::max(types.size(),other.types.size()); i++) {
      if (i<types.size() && i<other.types.size()) {
        if (types[i]!=other.types[i]) {
          return false;
        }
      }
      else if (i<types.size()) {
        if (types[i]) {
          return false;
        }
      }
      else if (i<other.types.size()) {
        if (other.types[i]) {
          return false;
        }
      }
    }

    return true;
  }

  bool TypeInfoSet::operator!=(const TypeInfoSet& other) const
  {
    if (this==&other) {
      return false;
    }

    if (count!=other.count) {
      return true;
    }

    for (size_t i=0; i<std::max(types.size(),other.types.size()); i++) {
      if (i<types.size() && i<other.types.size()) {
        if (types[i]!=other.types[i]) {
          return true;
        }
      }
      else if (i<types.size()) {
        if (types[i]) {
          return true;
        }
      }
      else if (i<other.types.size()) {
        if (other.types[i]) {
          return true;
        }
      }
    }

    return false;
  }
}
