#ifndef OSMSCOUT_TYPESET_H
#define OSMSCOUT_TYPESET_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2012  Tim Teulings

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

#include <vector>

#include <osmscout/Types.h>
#include <osmscout/TypeConfig.h>

namespace osmscout {

  /**
   * TypeSet is an libosmscout specific container class for holding a enumeration of types.
   *
   * Internally is uses the technical knowledge that all types have a low, monotinic increasing id, to
   * not use a set but a vector<bool>.
   *
   * For the vector<bool> to have the correct size without incremental resizing it needs to get passed
   * a TypeConfig instance. If you not pass a TypeConfig, it can be used but performance might be suboptimal
   * because of internal resizing.
   */
  class OSMSCOUT_API TypeSet
  {
  private:
    size_t            typeCount;
    std::vector<bool> types;

  public:
    TypeSet()
    : typeCount(0)
    {
      // no code
    }

    TypeSet(const TypeConfig& typeConfig)
    : typeCount(0)
    {
      types.resize(typeConfig.GetMaxTypeId()+1,false);
    }

    TypeSet(const TypeSet& other)
    {
      this->types=other.types;
      this->typeCount=other.typeCount;
    }

    void Clear()
    {
      for (size_t i=0; i<types.size(); i++) {
        types[i]=false;
      }

      typeCount=0;
    }

    inline bool HasTypes() const
    {
      return typeCount>0;
    }

    inline size_t GetSize() const
    {
      return typeCount;
    }

    void SetType(TypeId type)
    {
      if (type>=types.size()) {
        types.resize(type+1,false);
      }

      if (!types[type]) {
        types[type]=true;

        typeCount++;
      }
    }

    void UnsetType(TypeId type)
    {
      if (type<types.size()) {

        if (types[type]) {
          types[type]=false;

          typeCount--;
        }
      }
    }

    bool IsTypeSet(TypeId type) const
    {
      return type<types.size() && types[type];
    }

    TypeSet& operator=(const TypeSet& other)
    {
      if (&other!=this) {
        this->types=other.types;
        this->typeCount=other.typeCount;
      }

      return *this;
    }
  };
}

#endif
