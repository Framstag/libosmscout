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
#include <utility>

namespace osmscout {

  TypeInfoSet::TypeInfoSet(const TypeConfig& typeConfig)
  {
    words.resize((typeConfig.GetTypeCount()+63)/64);
  }

  TypeInfoSet::TypeInfoSet(TypeInfoSet&& other) noexcept
  : words(std::move(other.words)),
    contained(std::move(other.contained)),
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
    size_t typeCount=typeConfig.GetTypeCount();

    // A type configuration can only grow, but a set that is adapted to a smaller one keeps its
    // invariant: it contains exactly the types below the type count
    while (!contained.empty() &&
           contained.back()->GetIndex()>=typeCount) {
      size_t index=contained.back()->GetIndex();

      words[index/64]&=~(uint64_t(1)<<(index%64));
      contained.pop_back();
      count--;
    }

    words.resize((typeCount+63)/64);
  }

  void TypeInfoSet::Set(const TypeInfoRef& type)
  {
    assert(type);

    size_t index=type->GetIndex();

    if (index/64>=words.size()) {
      words.resize(index/64+1);
    }

    uint64_t mask=uint64_t(1)<<(index%64);

    if ((words[index/64]&mask)!=0) {
      return;
    }

    words[index/64]|=mask;
    count++;

    // The contained types are kept in ascending type index order, so that iteration yields
    // the types of the set in the order of the type indices
    auto pos=std::lower_bound(contained.begin(),
                              contained.end(),
                              index,
                              [](const TypeInfoRef& type,size_t index) {
                                return type->GetIndex()<index;
                              });

    contained.insert(pos,type);
  }

  void TypeInfoSet::Set(const TypeInfoSet& other)
  {
    words=other.words;
    contained=other.contained;
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

    size_t index=type->GetIndex();

    if (!TestBit(index)) {
      return;
    }

    words[index/64]&=~(uint64_t(1)<<(index%64));
    count--;

    auto pos=std::lower_bound(contained.begin(),
                              contained.end(),
                              index,
                              [](const TypeInfoRef& type,size_t index) {
                                return type->GetIndex()<index;
                              });

    if (pos!=contained.end() &&
        (*pos)->GetIndex()==index) {
      contained.erase(pos);
    }
  }

  void TypeInfoSet::Remove(const TypeInfoSet& otherTypes)
  {
    // Removing the types of the set itself would erase the types it iterates
    if (&otherTypes==this) {
      Clear();

      return;
    }

    for (const auto& type : otherTypes.contained) {
      Remove(type);
    }
  }

  void TypeInfoSet::Intersection(const TypeInfoSet& otherTypes)
  {
    size_t write=0;

    for (size_t i=0; i<contained.size(); i++) {
      const TypeInfoRef& type=contained[i];
      size_t             index=type->GetIndex();

      if (otherTypes.TestBit(index)) {
        contained[write]=type;
        write++;
      }
      else {
        words[index/64]&=~(uint64_t(1)<<(index%64));
        count--;
      }
    }

    contained.resize(write);
  }

  /**
   * Returns 'true' if at least one type is set in both Sets. Else
   * 'false' is returned.
   */
  bool TypeInfoSet::Intersects(const TypeInfoSet& otherTypes) const
  {
    size_t minWords=std::min(words.size(),otherTypes.words.size());

    for (size_t i=0; i<minWords; i++) {
      if ((words[i]&otherTypes.words[i])!=0) {
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

    size_t minWords=std::min(words.size(),other.words.size());

    for (size_t i=0; i<minWords; i++) {
      if (words[i]!=other.words[i]) {
        return false;
      }
    }

    // Words beyond the shorter side hold no type and are therefore equal to the missing ones
    for (size_t i=minWords; i<words.size(); i++) {
      if (words[i]!=0) {
        return false;
      }
    }

    for (size_t i=minWords; i<other.words.size(); i++) {
      if (other.words[i]!=0) {
        return false;
      }
    }

    return true;
  }

  bool TypeInfoSet::operator!=(const TypeInfoSet& other) const
  {
    return !(*this==other);
  }
}
