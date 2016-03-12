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

namespace osmscout {

  /**
   * Resolved and enhanced OSM geo coordinate
   *
   * Different OSM nodes with the same coordinate will get a serial number. The serial
   * number normally start with 1 (the next node with the same coordinate will get 2,...).
   *
   * This way we can 0 as a special flag for nodes without any identity.
   */
  struct OSMSCOUT_API Coord
  {
  private:
    FileOffset fileOffset; //!< File offset of this coordinate in the data file
    OSMId      osmId;      //!< OSM id
    uint8_t    serial;     //!< Serial id in relation to all coordinates witht he same coord value
    GeoCoord   coord;      //!< The geographic coordinate

  public:
    /**
     * The default constructor
     */
    inline Coord()
    {
      // no code
    }

    inline Coord(OSMId id, uint8_t serial, const GeoCoord& coord)
    : osmId(id),
      serial(serial),
      coord(coord)
    {
      // no code
    }

    inline FileOffset GetFileOffset() const
    {
      return fileOffset;
    }

    inline OSMId GetId() const
    {
      return osmId;
    }

    /**
     * Returns the libosmscout-internal substitution id for the given OSMId
     */
    inline Id GetOSMScoutId() const
    {
      return (fileOffset-sizeof(uint32_t))/coordByteSize;
    }

    inline void SetCoord(const GeoCoord& coord)
    {
      this->coord=coord;
    }

    inline const GeoCoord& GetCoord() const
    {
      return coord;
    }

    inline bool operator==(const Coord& other) const
    {
      return coord==other.coord;
    }

    inline bool operator<(const Coord& other) const
    {
      return coord<other.coord;
    }

    void Read(const TypeConfig& typeConfig,
              FileScanner& scanner);
    void Write(const TypeConfig& typeConfig,
              FileWriter& writer) const;
  };

  typedef std::shared_ptr<Coord> CoordRef;
}

#endif
