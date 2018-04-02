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

#include <osmscout/CoreImportExport.h>

#include <osmscout/CoreFeatures.h>

#include <osmscout/system/Types.h>

namespace osmscout {
  /**
   * \ingroup Util
   * Type to be used for OSM ids (signed numbers with 64 bit size).
   */
  typedef int64_t  OSMId;

  /**
   * \ingroup Util
   * Type to be used for libosmscout internal ids (unsigned numbers with 64 bit
   * size).
   */
  typedef uint64_t Id;
  typedef uint64_t PageId;
  /**
   * \ingroup Util
   * Type for describing the position of data within a file.
   */
  typedef uint64_t FileOffset;

  /**
   * \ingroup Util
   * Type for describing a type of an way, area or node.
   */
  typedef uint16_t TypeId;

  enum Vehicle
  {
    vehicleFoot     = 1 << 1,
    vehicleBicycle  = 1 << 2,
    vehicleCar      = 1 << 3
  };

  typedef uint8_t VehicleMask;
}

#endif
