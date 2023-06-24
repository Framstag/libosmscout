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

#include <osmscout/lib/CoreImportExport.h>

#include <array>
#include <cstddef>
#include <string>
#include <tuple>

#include <osmscout/OSMScoutTypes.h>

#include <osmscout/util/Magnification.h>
#include <osmscout/util/Distance.h>
#include <osmscout/util/Bearing.h>

#include <osmscout/system/Math.h>
#include <osmscout/system/Compiler.h>

namespace osmscout {
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

  constexpr uint32_t maxRawCoordValue = 0x7FFFFFF; // 134217727

  /**
   * \ingroup Util
   * Number of bytes needed to store a lat,lon coordinate pair.
   */
  const size_t coordByteSize=7;

  /**
   * \ingroup Geometry
   *
   * Anonymous geographic coordinate.
   */
  class OSMSCOUT_API GeoCoord CLASS_FINAL
  {
  private:
    double lat = 0.0;
    double lon = 0.0;

  public:
    using GeoCoordBuffer = std::array<std::byte,7>;

    static constexpr int MinLatitude = -90;
    static constexpr int MaxLatitude = 90;
    static constexpr int MinLongitude = -180;
    static constexpr int MaxLongitude = 180;

  public:
    /**
     * The default constructor creates an uninitialized instance (for performance reasons).
     */
    GeoCoord() = default;

    GeoCoord(const GeoCoord& other) = default;

    /**
     * Initialize the coordinate with the given latitude and longitude values.
     */
    GeoCoord(double lat,
             double lon)
     :lat(lat),lon(lon)
    {
      // no code
    }

    /**
     * Assign a new latitude and longitude value to the coordinate
     */
    void Set(double lat,
             double lon)
    {
      this->lat=lat;
      this->lon=lon;
    }

    /**
     * Return the latitude value of the coordinate
     */
    double GetLat() const
    {
      return lat;
    }

    /**
     * Return the latitude value of the coordinate
     */
    double GetLon() const
    {
      return lon;
    }

    /**
     * Return a string representation of the coordinate value in a human readable format.
     */
    std::string GetDisplayText() const;

    std::ostream& operator<<(std::ostream& stream) const
    {
      stream << GetDisplayText();
      return stream;
    }

    /**
     * Returns a fast calculable unique id for the coordinate. Coordinates with have
     * the same latitude and longitude value in the supported resolution wil have the same
     * id.
     *
     * The id does not have any semantics regarding sorting. Coordinates with close ids
     * do not need to be close in location.
     */
    Id GetId() const;

    /**
     * Encode the coordinate value into a buffer - as it would be done by FileScanner/FileWriter classes
     *
     * The coord will be encoded with the maximum resolution (of the library),
     * Coords with the same buffer value thus will in effect equal.
     */
    void EncodeToBuffer(GeoCoordBuffer& buffer) const // NOLINT
    {
      auto latValue=(uint32_t)round((lat+90.0)*latConversionFactor);
      auto lonValue=(uint32_t)round((lon+180.0)*lonConversionFactor);

      buffer[0]=std:: byte(latValue >>  0u);
      buffer[1]=std::byte(latValue >>  8u);
      buffer[2]=std::byte(latValue >> 16u);

      buffer[3]=std::byte(lonValue >>  0u);
      buffer[4]=std::byte(lonValue >>  8u);
      buffer[5]=std::byte(lonValue >> 16u);

      buffer[6]=std::byte((latValue >> 24u) & 0x07u) | std::byte((lonValue >> 20u) & 0x70u);
    }

    /**
     * Encode the coordinate value into a number (the number has hash character).
     */
    Id GetHash() const
    {
      auto     latValue=(uint64_t)round((lat+90.0)*latConversionFactor);
      auto     lonValue=(uint64_t)round((lon+180.0)*lonConversionFactor);
      uint64_t number  =0;

      for (size_t i=0; i<27; i++) {
        size_t bit=26-i;

        number=number << 1u;
        number=number+((latValue >> bit) & 0x01u);

        number=number << 1u;
        number=number+((lonValue >> bit) & 0x01u);
      }

      return number;
    }

    /**
     * Return true if both coordinates are equal (using == operator)
     */
    bool IsEqual(const GeoCoord& other) const
    {
      return lat==other.lat && lon==other.lon;
    }

    /**
     * Return true if latitude is in range <-90,+90> and longitude in range <-180,+180>
     */
    bool IsValid() const
    {
      return lat >= MinLatitude && lat <= MaxLatitude &&
             lon >= MinLongitude && lon <= MaxLongitude;
    }

    /**
     * Parse a textual representation of a geo coordinate from a string
     * to an GeoCoord instance.
     *
     * The text should follow the following expression:
     *
     * [+|-|N|S] <coordinate> [N|S] [+|-|W|E] <coordinate> [W|E]
     *
     * coordinate may have one of these formats:
     *  DDD[.DDDDD]
     *  DD°[D[.DDD]'[D[.DDD]"]]
     *  DD°[D[.DDD]
     *
     * The means:
     * * You first define the latitude, then the longitude value
     * * You can define with half you mean by either prefixing or postfixing
     * the actual number with a hint
     * * The hint can either be a sign ('-' or '+') or a direction ('N' and 'S'
     * for latitude, 'E' or 'W' for longitude).
     *
     * Possibly in future more variants will be supported. It is guaranteed
     * that the result of GetDisplayText() is successfully parsed.
     *
     * @param text
     *    Text containing the textual representation
     * @param coord
     *    The resulting coordinate, if the text was correctly parsed
     * @return
     *    true, if the text was correctly parsed, else false
     */
    static bool Parse(const std::string& text,
                      GeoCoord& coord);

    /**
     * Get distance between two coordinates.
     * @param target
     *    Target coordinate to measure distance
     * @return
     *    Point to point distance to target coordinates
     * @note
     *    The difference in height between the two points is neglected.
     */
    Distance GetDistance(const GeoCoord &target) const;

    /**
    * Get coordinate of position + course and distance.
    * @param bearing
    *    Target course in degree
    * @param distance
    *    Target distance
    * @return
    *    Target coordinates
    * @note
    *    The difference in height between the two points is neglected.
    */
    GeoCoord Add(const Bearing &bearing, const Distance &distance) const;

    /**
     * Return true if both coordinates are equals (using == operator)
     */
    bool operator==(const GeoCoord& other) const
    {
      return lat==other.lat && lon==other.lon;
    }

    /**
     * Return true if coordinates are not equal
     */
    bool operator!=(const GeoCoord& other) const
    {
      return lat!=other.lat || lon!=other.lon;
    }

    bool operator<(const GeoCoord& other) const
    {
      return std::tie(lat, lon) < std::tie(other.lat, other.lon);
    }

    /**
     * Assign the value of other
     */
    GeoCoord& operator=(const GeoCoord& other) = default;

    Distance operator-(const GeoCoord& other) const
    {
      return GetDistance(other);
    }
  };
}

#endif
