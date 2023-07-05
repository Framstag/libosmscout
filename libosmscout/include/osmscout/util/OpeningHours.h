#ifndef OSMSCOUT_OPENINGHOURS_H
#define OSMSCOUT_OPENINGHOURS_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2023 Lukáš Karas

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

#include <cstdint>
#include <vector>
#include <string>
#include <optional>

namespace osmscout {

  /** Utility for parsing simple OpenStreetMap opening hours.
   * see https://wiki.openstreetmap.org/wiki/Key:opening_hours
   */
  class OSMSCOUT_API OpeningHours
  {
  public:
    /** Day of the week in the Gregorian calendar.
     * And special entries for holidays.
     */
    enum class WeekDay
    {
      Monday = 0,
      Tuesday,
      Wednesday,
      Thursday,
      Friday,
      Saturday,
      Sunday,
      PublicHoliday,
      SchoolHoliday
    };

    struct DayTime
    {
      uint8_t hour=0; //!< 24 hour hour format. May be bigger than 24, when opened even after midnight (26 = 2 hours after midnight next day).
      uint8_t minute=0;
    };

    struct TimeInterval
    {
      DayTime from;
      DayTime to;
    };

    struct Rule
    {
      WeekDay day=WeekDay::Monday;
      std::vector<TimeInterval> intervals; //!< Closed when empty
    };

  private:
    std::vector<Rule> rules;

  public:
    explicit OpeningHours(std::vector<Rule> rules);
    OpeningHours(const OpeningHours&) = default;
    OpeningHours(OpeningHours&&) = default;

    ~OpeningHours() = default;

    OpeningHours& operator=(const OpeningHours&) = default;
    OpeningHours& operator=(OpeningHours&&) = default;

    std::vector<Rule> GetRules() const
    {
      return rules;
    }

    /** Parse OSM opening hours string
     *
     * @param str
     * @param explicitClosedDays explicitly add empty rule for week days when is closed
     * @return
     */
    static std::optional<OpeningHours> Parse(const std::string &str, bool explicitClosedDays=false);
  };

}

#endif //OSMSCOUT_OPENINGHOURS_H
