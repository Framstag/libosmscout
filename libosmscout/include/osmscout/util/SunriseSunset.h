#ifndef LIBOSMSCOUT_SUNRISESUNSET_H
#define LIBOSMSCOUT_SUNRISESUNSET_H

/*
  Sunrise Sunset Calculator.
  inspired by Java implementation https://github.com/caarmen/SunriseSunset (LGPL-2.1 License)

  This source is part of the libosmscout library

  Copyright (C) 2013-2017 Carmen Alvarez
  Copyright (C) 2021 Lukáš Karas

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
#include <osmscout/GeoCoord.h>
#include <osmscout/system/Compiler.h>
#include <osmscout/system/Math.h>
#include <osmscout/util/Time.h>
#include <osmscout/util/String.h>
#include <osmscout/util/SunriseSunset.h>

#include <optional>
#include <string>

namespace osmscout {

  using SunriseSunsetRes=std::optional<std::tuple<Timestamp, Timestamp>>;

  /**
   * Calculate the sunrise and sunset times for the given date, given
   * location, and sun altitude.
   * This is based on the Wikipedia article on the Sunrise equation.
   *
   * @param day         The day for which to calculate sunrise and sunset
   * @param location    the location
   * @param sunAltitude <a href="http://en.wikipedia.org/wiki/Solar_zenith_angle#Solar_elevation_angle">the angle between the horizon and the center of the sun's disc.</a>
   * @return a two-element Gregorian Calendar array. The first element is the
   * sunrise, the second element is the sunset. This will return nullopt if there is no sunrise or sunset. (Ex: no sunrise in Antarctica in June)
   * @see <a href="http://en.wikipedia.org/wiki/Sunrise_equation">Sunrise equation on Wikipedia</a>
   */
  extern OSMSCOUT_API SunriseSunsetRes GetSunriseSunset(const GeoCoord &location, const Timestamp &day, double sunAltitude);

  /**
   * Calculate the civil twilight time for the given date and given location.
   *
   * @param day The day for which to calculate civil twilight
   * @param location the location
   * @return a two-element Gregorian Calendar array. The first element is the
   * civil twilight dawn, the second element is the civil twilight dusk.
   * This will return nullopt if there is no civil twilight. (Ex: no twilight in Antarctica in December)
   */
  extern OSMSCOUT_API SunriseSunsetRes GetCivilTwilight(const GeoCoord &location, const Timestamp &day = Timestamp::clock::now());

  /**
   * Calculate the nautical twilight time for the given date and given location.
   *
   * @param day       The day for which to calculate nautical twilight
   * @param location the location
   * @return a two-element Gregorian Calendar array. The first element is the
   * nautical twilight dawn, the second element is the nautical twilight dusk.
   * This will return nullopt if there is no nautical twilight. (Ex: no twilight in Antarctica in December)
   */
  extern OSMSCOUT_API SunriseSunsetRes GetNauticalTwilight(const GeoCoord &location, const Timestamp &day = Timestamp::clock::now());

  /**
   * Calculate the astronomical twilight time for the given date and given location.
   *
   * @param day       The day for which to calculate astronomical twilight
   * @param location the location
   * @return a two-element Gregorian Calendar array. The first element is the
   * astronomical twilight dawn, the second element is the  astronomical twilight dusk.
   * This will return nullopt if there is no astronomical twilight. (Ex: no twilight in Antarctica in December)
   */
  extern OSMSCOUT_API SunriseSunsetRes GetAstronomicalTwilight(const GeoCoord &location, const Timestamp &day = Timestamp::clock::now());


  /**
   * Calculate the sunrise and sunset times for the given date and given
   * location. This is based on the Wikipedia article on the Sunrise equation.
   *
   * @param day       The day for which to calculate sunrise and sunset
   * @param location the location
   * @return a two-element Gregorian Calendar array. The first element is the
   * sunrise, the second element is the sunset. This will return nullopt if there is no sunrise or sunset. (Ex: no sunrise in Antarctica in June)
   * @see <a href="http://en.wikipedia.org/wiki/Sunrise_equation">Sunrise equation on Wikipedia</a>
   */
  extern OSMSCOUT_API SunriseSunsetRes GetSunriseSunset(const GeoCoord &location, const Timestamp &day = Timestamp::clock::now());

}

#endif //LIBOSMSCOUT_SUNRISESUNSET_H
