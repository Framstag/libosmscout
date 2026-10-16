#ifndef OSMSCOUT_TYPEINFOSET_H
#define OSMSCOUT_TYPEINFOSET_H

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

#include <cstdint>
#include <iterator>
#include <vector>

#include <osmscout/TypeConfig.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

  class OSMSCOUT_API TypeInfoSetConstIterator CLASS_FINAL
  {
  public:
    using self_type         = TypeInfoSetConstIterator;
    using value_type        = TypeInfoRef;
    using reference         = const TypeInfoRef&;
    using pointer           = TypeInfoRef;
    using iterator_category = std::input_iterator_tag;

  private:
    std::vector<TypeInfoRef>::const_iterator iterCurrent;
    std::vector<TypeInfoRef>::const_iterator iterEnd;

  public:
    TypeInfoSetConstIterator(const std::vector<value_type>::const_iterator& iterCurrent,
                             const std::vector<value_type>::const_iterator& iterEnd)
    : iterCurrent(iterCurrent),
      iterEnd(iterEnd)
    {
      while (this->iterCurrent!=this->iterEnd &&
            !*this->iterCurrent) {
        ++this->iterCurrent;
      }
    }

    TypeInfoSetConstIterator(const TypeInfoSetConstIterator& other) = default;

    TypeInfoSetConstIterator& operator++()
    {
      ++iterCurrent;

      while (iterCurrent!=iterEnd &&
            !*iterCurrent) {
        ++iterCurrent;
      }

      return *this;
    }

    TypeInfoSetConstIterator operator++(int)
    {
      TypeInfoSetConstIterator tmp(*this);

      operator++();

      return tmp;
    }

    bool operator==(const TypeInfoSetConstIterator& other) const
    {
      return iterCurrent==other.iterCurrent;
    }

    bool operator!=(const TypeInfoSetConstIterator& other) const
    {
      return iterCurrent!=other.iterCurrent;
    }

    const TypeInfoRef& operator*() const
    {
      return *iterCurrent;
    }

    TypeInfoRef operator->() const
    {
      return *iterCurrent;
    }
  };

  /**
   * Custom data structure to efficiently handle a set of TypeInfoRef.
   *
   * The set holds one bit per type index and the contained types in ascending index order,
   * using the fact, that TypeInfo internally have a continuously running index variable. Its
   * operations are independent of the number of types the type configuration defines; the
   * bytes the set allocates follow the contained types and not the number of types the type
   * configuration defines.
   */
  class OSMSCOUT_API TypeInfoSet CLASS_FINAL
  {
  private:
    //! One bit per type index, set if the type is contained
    std::vector<uint64_t>    words;
    //! The contained types, in ascending type index order
    std::vector<TypeInfoRef> contained;
    size_t                   count = 0;

    bool TestBit(size_t index) const
    {
      return index/64<words.size() &&
             (words[index/64]&(uint64_t(1)<<(index%64)))!=0;
    }

  public:
    TypeInfoSet() = default;
    ~TypeInfoSet() = default;

    explicit TypeInfoSet(const TypeConfig& typeConfig);
    explicit TypeInfoSet(const std::vector<TypeInfoRef>& types);

    TypeInfoSet(const TypeInfoSet& other) = default;
    TypeInfoSet(TypeInfoSet&& other) noexcept;

    TypeInfoSet& operator=(const TypeInfoSet& other) = default;
    TypeInfoSet& operator=(TypeInfoSet&& other) noexcept = default;

    bool operator==(const TypeInfoSet& other) const;
    bool operator!=(const TypeInfoSet& other) const;

    void Adapt(const TypeConfig& typeConfig);

    void Clear()
    {
      words.clear();
      contained.clear();

      count=0;
    }

    void Set(const TypeInfoRef& type);
    void Set(const std::vector<TypeInfoRef>& types);
    void Set(const TypeInfoSet& other);

    void Add(const TypeInfoSet& types);

    void Remove(const TypeInfoRef& type);
    void Remove(const TypeInfoSet& otherTypes);

    void Intersection(const TypeInfoSet& otherTypes);

    bool IsSet(const TypeInfoRef& type) const
    {
      assert(type);

      return TestBit(type->GetIndex());
    }

    bool Empty() const
    {
      return count==0;
    }

    size_t Size() const
    {
      return count;
    }

    bool Intersects(const TypeInfoSet& otherTypes) const;

    TypeInfoSetConstIterator begin() const
    {
      return TypeInfoSetConstIterator(contained.begin(),
                                      contained.end());
    }

    TypeInfoSetConstIterator end() const
    {
      return TypeInfoSetConstIterator(contained.end(),
                                      contained.end());
    }
  };
}

#endif
