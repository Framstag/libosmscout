#ifndef OSMSCOUT_POINT_H
#define OSMSCOUT_POINT_H

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

#include <osmscout/GeoCoord.h>
#include <osmscout/OSMScoutTypes.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

  /**
   * \ingroup Geometry
   * A point is a identifiable (has an id) geo-coordinate.
   */
  class OSMSCOUT_API Point CLASS_FINAL
  {
  private:
    uint8_t  serial=0;
    GeoCoord coord;

  public:
    Point() = default;

    inline Point(uint8_t serial,
                 const GeoCoord& coords)
     : serial(serial),
       coord(coords)
     {
       // no code
     }

    inline void Set(uint8_t serial,
                    const GeoCoord& coords)
    {
      this->serial=serial;
      this->coord=coords;
    }

    inline void SetSerial(uint8_t serial)
    {
      this->serial=serial;
    }

    inline void ClearSerial()
    {
      serial=0;
    }

    inline void SetCoord(const GeoCoord& coords)
    {
      this->coord=coords;
    }

    inline uint8_t GetSerial() const
    {
      return serial;
    }

    inline bool IsRelevant() const
    {
      return serial!=0;
    }

    /**
     * Returns a fast calculable unique id for the coordinate under consideration
     * that different OSM nodes with the same coordinate will have different ids if the
     * identity of the node is important - else the serial id will be 0.
     *
     * The id does not have any semantics regarding sorting. Coordinates with close ids
     * do not need to be close in location.
     */
    Id GetId() const;

    inline const GeoCoord& GetCoord() const
    {
      return coord;
    }

    inline double GetLat() const
    {
      return coord.GetLat();
    }

    inline double GetLon() const
    {
      return coord.GetLon();
    }

    /**
     * Compare this and the other point for identity. Identity is defined
     * as have the same coordinates and the same serial id per coordinate.
     *
     * @param other
     *    Other point to compare against
     * @return
     *    true if identical, else false
     */
    inline bool IsIdentical(const Point& other) const
    {
      return serial==other.serial && coord==other.coord;
    }

    /**
     * Compare this and the other point for "sameness". Sameness is defined as having the
     * same coordinate but not necessarily the same serial id. This means, that both points
     * have the same location but are not necessarily identical.
     *
     * @param other
     *    Other point to compare against
     * @return
     *    true if same location, else false
     */
    inline bool IsSame(const Point& other) const
    {
      return coord==other.coord;
    }

    /**
     * Same semantics as IsSame(), implement for template compability with GeoCoord.
     *
     * @param other
     *    Other point to compare against
     * @return
     *    true if same location, else false
     */
    inline bool IsEqual(const Point& other) const
    {
      return coord==other.coord;
    }

    /**
     * While we do not want to compare using operator== we at least want to
     * manage points in containers. So we need to implement operator<.
     *
     * @param other
     *    Other point to compare to
     * @return
     *    true or false
     */
    inline bool operator<(const Point& other) const
    {
      return coord<other.GetCoord() ||
        (coord==other.GetCoord() && serial < other.serial);
    }

    static GeoCoord GetCoordFromId(Id id);
  };
}

#endif
