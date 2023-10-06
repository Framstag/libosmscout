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

#include <osmscout/util/SunriseSunset.h>

namespace osmscout {
namespace {

  /**
   * The altitude of the sun (solar elevation angle) at the moment of sunrise or sunset: -0.833
   */
  const double SUN_ALTITUDE_SUNRISE_SUNSET = -0.833;

  /**
   * The altitude of the sun (solar elevation angle) at the moment of civil twilight: -6.0
   */
  const double SUN_ALTITUDE_CIVIL_TWILIGHT = -6.0;

  /**
   * The altitude of the sun (solar elevation angle) at the moment of nautical twilight: -12.0
   */
  const double SUN_ALTITUDE_NAUTICAL_TWILIGHT = -12.0;

  /**
   * The altitude of the sun (solar elevation angle) at the moment of astronomical twilight: -18.0
   */
  const double SUN_ALTITUDE_ASTRONOMICAL_TWILIGHT = -18.0;

  const int JULIAN_DATE_2000_01_01 = 2451545;
  const double CONST_0009 = 0.0009;
  const double CONST_360 = 360;

  /**
   * Intermediate variables used in the sunrise equation
   * @see <a href="http://en.wikipedia.org/wiki/Sunrise_equation">Sunrise equation on Wikipedia</a>
   */
  struct SolarEquationVariables
  {
    double n;// Julian cycle (number of days since 2000-01-01).
    double m; // solar mean anomaly
    double lambda; // ecliptic longitude
    double jtransit; // Solar transit (hour angle for solar noon)
    double delta; // Declination of the sun
  };

  /**
   * returns the utc timezone offset
   * (e.g. -8 hours for PST)
   */
  int GetUtcOffset()
  {

    time_t zero = 24 * 60 * 60L;
    struct tm *timeptr;
    int gmtimeHours;

    // get the local time for Jan 2, 1900 00:00 UTC
    timeptr = localtime(&zero);
    gmtimeHours = timeptr->tm_hour;

    // if the local time is the "day before" the UTC, subtract 24 hours
    // from the hours to get the UTC offset
    if (timeptr->tm_mday < 2) {
      gmtimeHours -= 24;
    }

    return gmtimeHours;
  }

  /**
   * https://stackoverflow.com/a/9088549/1632737
   *
   * the utc analogue of mktime,
   * (much like timegm on some systems)
   */
  time_t MkTimeUTC(struct tm *timeptr)
  {
    // gets the epoch time relative to the local time zone,
    // and then adds the appropriate number of seconds to make it UTC
    return mktime(timeptr) + GetUtcOffset() * 3600;
  }

  /**
     * Convert a Julian date to a Gregorian date. Accuracy is to the second.
     * <br>
     * This is based on the Wikipedia article for Julian day.
     *
     * @param julianDate The date to convert
     * @return a Gregorian date in the local time zone.
     * @see <a href="http://en.wikipedia.org/wiki/Julian_day#Gregorian_calendar_from_Julian_day_number">Converting from Julian day to Gregorian date, on Wikipedia</a>
     */
  Timestamp GetGregorianDate(double julianDate)
  {

    int DAYS_PER_4000_YEARS = 146097;
    int DAYS_PER_CENTURY = 36524;
    int DAYS_PER_4_YEARS = 1461;
    int DAYS_PER_5_MONTHS = 153;

    // Let J = JD + 0.5: (note: this shifts the epoch back by one half day,
    // to start it at 00:00UTC, instead of 12:00 UTC);
    int J = (int) (julianDate + 0.5);

    // let j = J + 32044; (note: this shifts the epoch back to astronomical
    // year -4800 instead of the start of the Christian era in year AD 1 of
    // the proleptic Gregorian calendar).
    int j = J + 32044;

    // let g = j div 146097; let dg = j mod 146097;
    int g = j / DAYS_PER_4000_YEARS;
    int dg = j % DAYS_PER_4000_YEARS;

    // let c = (dg div 36524 + 1) * 3 div 4; let dc = dg - c * 36524;
    int c = ((dg / DAYS_PER_CENTURY + 1) * 3) / 4;
    int dc = dg - c * DAYS_PER_CENTURY;

    // let b = dc div 1461; let db = dc mod 1461;
    int b = dc / DAYS_PER_4_YEARS;
    int db = dc % DAYS_PER_4_YEARS;

    // let a = (db div 365 + 1) * 3 div 4; let da = db - a * 365;
    int a = ((db / 365 + 1) * 3) / 4;
    int da = db - a * 365;

    // let y = g * 400 + c * 100 + b * 4 + a; (note: this is the integer
    // number of full years elapsed since March 1, 4801 BC at 00:00 UTC);
    int y = g * 400 + c * 100 + b * 4 + a;

    // let m = (da * 5 + 308) div 153 - 2; (note: this is the integer number
    // of full months elapsed since the last March 1 at 00:00 UTC);
    int m = (da * 5 + 308) / DAYS_PER_5_MONTHS - 2;

    // let d = da -(m + 4) * 153 div 5 + 122; (note: this is the number of
    // days elapsed since day 1 of the month at 00:00 UTC, including
    // fractions of one day);
    int d = da - ((m + 4) * DAYS_PER_5_MONTHS) / 5 + 122;

    // let Y = y - 4800 + (m + 2) div 12;
    int year = y - 4800 + (m + 2) / 12;

    // let M = (m + 2) mod 12 + 1;
    int month = (m + 2) % 12;

    // let D = d + 1;
    int day = d + 1;

    // Apply the fraction of the day in the Julian date to the Gregorian
    // date.
    // Example: dayFraction = 0.717
    double dayFraction = (julianDate + 0.5) - J;

    // Ex: 0.717*24 = 17.208 hours. We truncate to 17 hours.
    int hours = (int) (dayFraction * 24);
    // Ex: 17.208 - 17 = 0.208 days. 0.208*60 = 12.48 minutes. We truncate
    // to 12 minutes.
    int minutes = (int) ((dayFraction * 24 - hours) * 60);
    // Ex: 17.208*60 - (17*60 + 12) = 1032.48 - 1032 = 0.48 minutes. 0.48*60
    // = 28.8 seconds.
    // We round to 29 seconds.
    int seconds = (int) ((dayFraction * 24 * 3600 - (hours * 3600 + minutes * 60)) + .5);

    // Create the gregorian date.
    using namespace std::chrono;
    std::tm time{};

    time.tm_year = year - 1900; // Year since 1900
    time.tm_mon = month;     // 0-11
    time.tm_mday = day;      // 1-31
    time.tm_hour = hours;    // 0-23
    time.tm_min = minutes;   // 0-59
    time.tm_sec = seconds;   // 0-60

    std::time_t tt = MkTimeUTC(&time);

    time_point <system_clock, nanoseconds> timePoint = system_clock::from_time_t(tt);
    return time_point_cast<milliseconds, system_clock, nanoseconds>(timePoint);
  }

  /**
   * Convert a Gregorian calendar date to a Julian date. Accuracy is to the
   * second.
   * <br>
   * This is based on the Wikipedia article for Julian day.
   *
   * @param gregorianDate Gregorian date in any time zone.
   * @return the Julian date for the given Gregorian date.
   * @see <a href="http://en.wikipedia.org/wiki/Julian_day#Converting_Julian_or_Gregorian_calendar_date_to_Julian_Day_Number">Converting to Julian day number on Wikipedia</a>
   */
  double GetJulianDate(const Timestamp &gregorianDate)
  {
    std::time_t tt = std::chrono::system_clock::to_time_t(gregorianDate);
    std::tm tm = *std::gmtime(&tt);

    // For the year (Y) astronomical year numbering is used, thus 1 BC is 0,
    // 2 BC is -1, and 4713 BC is -4712.
    int year = tm.tm_year + 1900;
    // The months (M) January to December are 1 to 12
    int month = tm.tm_mon + 1;
    // D is the day of the month.
    int day = tm.tm_mday;
    int a = (14 - month) / 12;
    int y = year + 4800 - a;
    int m = month + 12 * a - 3;

    int julianDay = day + (153 * m + 2) / 5 + 365 * y + (y / 4) - (y / 100)
                    + (y / 400) - 32045;
    int hour = tm.tm_hour;
    int minute = tm.tm_min;
    int second = tm.tm_sec;

    return julianDay + ((double) hour - 12) / 24 + ((double) minute) / 1440 + ((double) second) / 86400;
  }

  double ToRadians(double degrees)
  {
    return degrees * M_PI / 180.0;
  }

  double ToDegrees(double radians)
  {
    return radians * 180.0 / M_PI;
  }

  /**
   * Return intermediate variables used for calculating sunrise, sunset, and solar noon.
   *
   * @param day         The day for which to calculate the ecliptic longitude and jtransit
   * @param longitude   the longitude of the location in degrees (West is negative)
   * @return a 2-element array with the ecliptic longitude (lambda) as the first element, and solar transit (jtransit) as the second element
   * @see <a href="http://en.wikipedia.org/wiki/Sunrise_equation">Sunrise equation on Wikipedia</a>
   */
  SolarEquationVariables GetSolarEquationVariables(const Timestamp &day, double longitude)
  {
    longitude = -longitude;

    // Get the given date as a Julian date.
    double julianDate = GetJulianDate(day);

    // Calculate current Julian cycle (number of days since 2000-01-01).
    double nstar = julianDate - JULIAN_DATE_2000_01_01 - CONST_0009 - longitude / CONST_360;
    double n = std::round(nstar);

    // Approximate solar noon
    double jstar = JULIAN_DATE_2000_01_01 + CONST_0009 + longitude / CONST_360 + n;
    // Solar mean anomaly
    double m = ToRadians(std::fmod((357.5291 + 0.98560028 * (jstar - JULIAN_DATE_2000_01_01)), CONST_360));

    // Equation of center
    double c = 1.9148 * std::sin(m) + 0.0200 * std::sin(2 * m) + 0.0003 * std::sin(3 * m);

    // Ecliptic longitude
    double lambda = ToRadians(std::fmod((ToDegrees(m) + 102.9372 + c + 180), CONST_360));

    // Solar transit (hour angle for solar noon)
    double jtransit = jstar + 0.0053 * std::sin(m) - 0.0069 * std::sin(2 * lambda);

    // Declination of the sun.
    double delta = std::asin(std::sin(lambda) * std::sin(ToRadians(23.439)));

    return SolarEquationVariables{n, m, lambda, jtransit, delta};
  }
} // anonymous namespace

  SunriseSunsetRes GetSunriseSunset(const GeoCoord &location, const Timestamp &day, double sunAltitude)
  {
    double latitude = location.GetLat();
    double longitude = location.GetLon();
    SolarEquationVariables solarEquationVariables = GetSolarEquationVariables(day, longitude);

    longitude = -longitude;
    double latitudeRad = ToRadians(latitude);

    // Hour angle
    double omega = std::acos(
        (std::sin(ToRadians(sunAltitude)) - std::sin(latitudeRad) * std::sin(solarEquationVariables.delta))
        / (std::cos(latitudeRad) * std::cos(solarEquationVariables.delta)));

    if (std::isnan(omega)) {
      return std::nullopt;
    }

    // Sunset
    double jset = JULIAN_DATE_2000_01_01
                  + CONST_0009
                  + ((ToDegrees(omega) + longitude) / CONST_360 + solarEquationVariables.n + 0.0053
                                                                                             * std::sin(
        solarEquationVariables.m) - 0.0069 * std::sin(2 * solarEquationVariables.lambda));

    // Sunrise
    double jrise = solarEquationVariables.jtransit - (jset - solarEquationVariables.jtransit);

    // Convert sunset and sunrise to Gregorian dates, in UTC
    Timestamp gregRiseUTC = GetGregorianDate(jrise);
    Timestamp gregSetUTC = GetGregorianDate(jset);

    return std::make_optional(std::make_tuple(gregRiseUTC, gregSetUTC));
  }

  SunriseSunsetRes GetCivilTwilight(const GeoCoord &location, const Timestamp &day)
  {
    return GetSunriseSunset(location, day, SUN_ALTITUDE_CIVIL_TWILIGHT);
  }

  SunriseSunsetRes GetNauticalTwilight(const GeoCoord &location, const Timestamp &day)
  {
    return GetSunriseSunset(location, day, SUN_ALTITUDE_NAUTICAL_TWILIGHT);
  }

  SunriseSunsetRes GetAstronomicalTwilight(const GeoCoord &location, const Timestamp &day)
  {
    return GetSunriseSunset(location, day, SUN_ALTITUDE_ASTRONOMICAL_TWILIGHT);
  }

  SunriseSunsetRes GetSunriseSunset(const GeoCoord &location, const Timestamp &day)
  {
    return GetSunriseSunset(location, day, SUN_ALTITUDE_SUNRISE_SUNSET);
  }

}
