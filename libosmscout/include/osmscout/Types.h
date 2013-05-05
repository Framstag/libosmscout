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

#include <osmscout/CoreFeatures.h>

#include <osmscout/system/Types.h>

namespace osmscout {

#if defined(OSMSCOUT_HAVE_UINT64_T)
  typedef int64_t  OSMId;
  typedef uint64_t Id;
  typedef uint64_t PageId;
  typedef uint64_t FileOffset;
#else
  typedef int32_t  OSMId;
  typedef uint32_t Id;
  typedef uint32_t PageId;
  typedef uint64_t FileOffset;
#endif

  typedef uint16_t TypeId;

  /**
    Coordinates will be stored as unsigned long values in file.
    For the conversion the float value is shifted to positive
    value sand afterwards multiplied by conversion factor
    to get long values without significant values after colon.
    */
  extern OSMSCOUT_API const double conversionFactor;
}

#endif
