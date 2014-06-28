#ifndef OSMSCOUT_GEOCOORD_H
#define OSMSCOUT_GEOCOORD_H

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

#include <string>

#include <osmscout/system/Types.h>

namespace osmscout {

  /**
   * \ingroup Geometry
   *
   * Anonymous geographic coordinate.
   */
  struct OSMSCOUT_API GeoCoord
  {
    double lat;
    double lon;

    /**
     * The default constructor creates an uninitialized instance (for performance reasons).
     */
    inline GeoCoord()
    {
      // no code
    }

    inline GeoCoord(double lat,
                    double lon)
     :lat(lat),lon(lon)
    {
      // no code
    }

     inline void Set(double lat,
                    double lon)
    {
      this->lat=lat;
      this->lon=lon;
    }

    inline double GetLat() const
    {
      return lat;
    }

    inline double GetLon() const
    {
      return lon;
    }

    inline bool IsEqual(const GeoCoord& other) const
    {
      return lat==other.lat && lon==other.lon;
    }

    inline bool operator==(const GeoCoord& other) const
    {
      return lat==other.lat && lon==other.lon;
    }

    inline bool operator<(const GeoCoord& other) const
    {
      return lat<other.lat ||
      (lat==other.lat && lon<other.lon);
    }

    inline void operator=(const GeoCoord& other)
    {
      this->lat=other.lat;
      this->lon=other.lon;
    }

    static bool Parse(const std::string& text,
                      GeoCoord& coord);
  };

  /**
   * \ingroup Util
   * Coordinates will be stored as unsigned long values in file.
   * For the conversion the float value is shifted to positive
   * value and afterwards multiplied by conversion factor
   * to get long values without significant values after colon.
   */
  extern OSMSCOUT_API const double lonConversionFactor;

  /**
   * \ingroup Util
   * Coordinates will be stored as unsigned long values in file.
   * For the conversion the float value is shifted to positive
   * value and afterwards multiplied by conversion factor
   * to get long values without significant values after colon.
   */
  extern OSMSCOUT_API const double latConversionFactor;

  /**
   * \ingroup Util
   * Number of bytes needed to store a lat,lon coordinate pair.
   */
  extern OSMSCOUT_API const size_t coordByteSize;
}

#endif
