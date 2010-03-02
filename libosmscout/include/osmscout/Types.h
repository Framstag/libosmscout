#ifndef OSMSCOUT_TYPES_H
#define OSMSCOUT_TYPES_H

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

// will be <cstdint> in c++0x
#include <stdint.h>

namespace osmscout {

  typedef long     FileOffset;
  typedef uint32_t Id;
  typedef uint32_t Page;
  typedef uint16_t TypeId;
  typedef uint16_t NodeCount;

  enum Mag {
   magWorld     =              1, //  0
   magState     =             32, //  5
   magStateOver =             64, //  6
   magCounty    =            128, //  7
   magRegion    =            256, //  8
   magProximity =            512, //  9
   magCityOver  =           1024, // 10
   magCity      =         2*1024, // 11
   magSuburb    =       2*2*1014, // 12
   magDetail    =     2*2*2*1024, // 13
   magClose     =   2*2*2*2*1024, // 14
   magVeryClose = 2*2*2*2*2*1024  // 15
  };
}

#endif
