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
#include <osmscout/Types.h>

namespace osmscout {

  class OSMSCOUT_API Point
  {
  private:
    Id       id;
    GeoCoord coords;

  public:
    inline Point()
    : id(0)
    {
      // no code
    }

    inline Point(Id id,
                 const GeoCoord& coords)
     : id(id),
       coords(coords)
     {
       // no code
     }

    inline Point(Id id,
                 double lat,
                 double lon)
    : id(id),
      coords(lat,lon)
    {
      // no code
    }

    inline void Set(Id id,
                    const GeoCoord& coords)
    {
      this->id=id;
      this->coords=coords;
    }

    inline void Set(const GeoCoord& coords)
    {
      this->coords=coords;
    }

    inline Id GetId() const
    {
      return id;
    }

    inline const GeoCoord& GetCoords() const
    {
      return coords;
    }

    inline double GetLat() const
    {
      return coords.GetLat();
    }

    inline double GetLon() const
    {
      return coords.GetLon();
    }

    inline bool IsIdentical(const Point& other) const
    {
      return id==other.id;
    }

    inline bool IsSame(const Point& other) const
    {
      return coords==other.coords;
    }

    inline bool IsEqual(const Point& other) const
    {
      return id==other.id || (coords==other.coords);
    }
  };
}

#endif
