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

#include <osmscout/Types.h>

namespace osmscout {

  class OSMSCOUT_API Point
  {
  private:
    Id     id;
    double lat;
    double lon;

  public:
    inline Point()
    {
      // no code
    }

    inline Point(Id id,
                 double lat,
                 double lon)
    : id(id),
      lat(lat),
      lon(lon)
    {
      // no code
    }

    inline void Set(Id id,
                    double lat,
                    double lon)
    {
      this->id=id;
      this->lat=lat;
      this->lon=lon;
    }

    inline Id GetId() const
    {
      return id;
    }

    inline double GetLat() const
    {
      return lat;
    }

    inline double GetLon() const
    {
      return lon;
    }

    inline bool IsIdentical(const Point& other) const
    {
      return id==other.id;
    }

    inline bool IsSame(const Point& other) const
    {
      return lat==other.lat && lon==other.lon;
    }

    inline bool IsEqual(const Point& other) const
    {
      return id==other.id || (lat==other.lat && lon==other.lon);
    }
  };
}

#endif
