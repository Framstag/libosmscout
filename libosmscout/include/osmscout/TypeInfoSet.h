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
   * All operations on the set are O(1) using the fact, that TypeInfo internally
   * have a continuously running index variable (Set may be slower if the
   * internal array was not preinitialized to it maximum size by passing a
   * TypeConfig or another TypeInfoSet in the constructor.
   */
  class OSMSCOUT_API TypeInfoSet CLASS_FINAL
  {
  private:
    std::vector<TypeInfoRef> types;
    size_t                   count = 0;

  public:
    TypeInfoSet() = default;
    ~TypeInfoSet() = default;

    explicit TypeInfoSet(const TypeConfig& typeConfig);
    explicit TypeInfoSet(const std::vector<TypeInfoRef>& types);

    TypeInfoSet(const TypeInfoSet& other) = default;
    TypeInfoSet(TypeInfoSet&& other) noexcept;

    void Adapt(const TypeConfig& typeConfig);

    inline void Clear()
    {
      if (count>0) {
        types.clear();
      }

      count=0;
    }

    void Set(const TypeInfoRef& type);
    void Set(const std::vector<TypeInfoRef>& types);
    void Set(const TypeInfoSet& other);

    void Add(const TypeInfoSet& types);

    void Remove(const TypeInfoRef& type);
    void Remove(const TypeInfoSet& otherTypes);

    void Intersection(const TypeInfoSet& otherTypes);

    inline bool IsSet(const TypeInfoRef& type) const
    {
      assert(type);

      return type->GetIndex()<types.size() &&
             types[type->GetIndex()];
    }

    inline bool Empty() const
    {
      return count==0;
    }

    inline size_t Size() const
    {
      return count;
    }

    bool Intersects(const TypeInfoSet& otherTypes) const;

    inline TypeInfoSet& operator=(const TypeInfoSet& other)
    {
      if (&other!=this) {
        this->types=other.types;
        this->count=other.count;
      }

      return *this;
    }

    bool operator==(const TypeInfoSet& other) const;
    bool operator!=(const TypeInfoSet& other) const;

    inline TypeInfoSetConstIterator begin() const
    {
      return TypeInfoSetConstIterator(types.begin(),
                                      types.end());
    }

    inline TypeInfoSetConstIterator end() const
    {
      return TypeInfoSetConstIterator(types.end(),
                                      types.end());
    }
  };
}

#endif
