#ifndef OSMSCOUT_TYPES_H
#define OSMSCOUT_TYPES_H

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

#include <osmscout/private/CoreImportExport.h>

#include <vector>

#include <osmscout/system/Types.h>

namespace osmscout {

  typedef int32_t  FileOffset;
  typedef uint32_t Id;
  typedef uint32_t Page;
  typedef uint16_t TypeId;
  typedef uint16_t NodeCount;

  /**
    Named values for magnification values in the range 0..15.
    a value change from x to x+1 means a zoom by the factor of two.
    */
  enum Mag {
   magWorld     =                1, //  0
   magContinent =               16, //  4
   magState     =               32, //  5
   magStateOver =               64, //  6
   magCounty    =              128, //  7
   magRegion    =              256, //  8
   magProximity =              512, //  9
   magCityOver  =             1024, // 10
   magCity      =           2*1024, // 11
   magSuburb    =         2*2*1014, // 12
   magDetail    =       2*2*2*1024, // 13
   magClose     =     2*2*2*2*1024, // 14
   magVeryClose =   2*2*2*2*2*1024, // 15
   magBlock     = 2*2*2*2*2*2*1024  // 16
  };

  class TypeSet
  {
  private:
    std::vector<bool> types;

  public:
    TypeSet()
    {
      // no code
    }

    TypeSet(const TypeSet& other)
    {
      this->types=other.types;
    }

    void Reset(size_t numberOfTypes)
    {
      types.resize(numberOfTypes,false);
    }

    void SetType(TypeId type)
    {
      types[type]=true;
    }

    void UnsetType(TypeId type)
    {
      types[type]=false;
    }

    bool IsTypeSet(TypeId type) const
    {
      return type<types.size() && types[type];
    }

    TypeSet& operator=(const TypeSet& other)
    {
      if (&other!=this) {
        this->types=other.types;
      }

      return *this;
    }
  };


  /**
    Coordinates will be stored as unsigned long values in file.
    For the conversion the float value is shifted to positive
    value sand afterwards multiplied by conversion factor
    to get long values without significant values after colon.
    */
  extern OSMSCOUT_API const double conversionFactor;
  extern OSMSCOUT_API size_t MagToLevel(double mag);
}

#endif
