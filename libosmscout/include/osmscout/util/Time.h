#ifndef OSMSCOUT_UTIL_TIME_H
#define OSMSCOUT_UTIL_TIME_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2018  Lukas Karas

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

#include <chrono>

namespace osmscout {

  using Timestamp = std::chrono::system_clock::time_point;

  using Duration = Timestamp::duration;

  using HourDuration = std::chrono::duration<double, std::ratio<3600>>;

  inline double DurationAsSeconds(Duration duration)
  {
    return std::chrono::duration_cast<std::chrono::duration<double>>(
        duration).count();
  }

  inline double DurationAsHours(Duration duration)
  {
    return std::chrono::duration_cast<HourDuration>(duration).count();
  }

  inline Duration DurationOfHours(double hours)
  {
    return std::chrono::duration_cast<Duration>(HourDuration(hours));
  }
}

#endif //OSMSCOUT_UTIL_TIME_H
