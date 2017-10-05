#ifndef OSMSCOUT_COORD_H
#define OSMSCOUT_COORD_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2013  Tim Teulings

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

#include <osmscout/system/Types.h>

#include <osmscout/GeoCoord.h>
#include <osmscout/TypeConfig.h>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

  /**
   * Resolved and enhanced OSM geo coordinate
   *
   * Different OSM nodes with the same coordinate will get a serial number. The serial
   * number normally start with 1 (the next node with the same coordinate will get 2,...).
   *
   * This way we can 0 as a special flag for nodes without any identity.
   */
  struct OSMSCOUT_API Coord CLASS_FINAL
  {
  private:
    uint8_t    serial;     //!< Serial id in relation to all coordinates with the same coord value
    GeoCoord   coord;      //!< The geographic coordinate

  public:
    /**
     * The default constructor
     */
    inline Coord()
    {
      // no code
    }

    inline Coord(uint8_t serial,const GeoCoord& coord)
    : serial(serial),
      coord(coord)
    {
      // no code
    }

    /**
     * Returns a fast calculable unique id for the coordinate. Coordinates with have
     * the same latitude and longitude value in the supported resolution wil have the same
     * id.
     *
     * The id does not have any semantics regarding sorting. Coordinates with close ids
     * do not need to be close in location.
     */
    Id GetOSMScoutId() const;

    /**
     * Encode the coordinate value into a number (the number has hash character).
     */
    Id GetHash() const;

    inline uint8_t GetSerial() const
    {
      return serial;
    }

    inline const GeoCoord& GetCoord() const
    {
      return coord;
    }

    inline void SetCoord(const GeoCoord& coord)
    {
      this->coord=coord;
    }

    inline bool operator==(const Coord& other) const
    {
      return coord==other.coord;
    }

    inline bool operator<(const Coord& other) const
    {
      return coord<other.coord;
    }
  };
}

#endif
