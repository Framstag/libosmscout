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

#include <osmscout/util/StopClock.h>

#include <chrono>
#include <iomanip>
#include <sstream>

#include <osmscout/private/Config.h>

#include <osmscout/util/String.h>

namespace osmscout {

  typedef std::chrono::duration<double,std::milli> MilliDouble;

  struct StopClock::StopClockPIMPL
  {
    std::chrono::steady_clock::time_point start;
    std::chrono::steady_clock::time_point stop;
  };

  StopClock::StopClock()
    : pimpl(new StopClockPIMPL())
  {
    pimpl->start=std::chrono::steady_clock::now();
  }

  StopClock::~StopClock()
  {
    delete pimpl;
  }

  void StopClock::Stop()
  {
    pimpl->stop=std::chrono::steady_clock::now();
  }

  double StopClock::GetMilliseconds() const
  {
    return std::chrono::duration_cast<MilliDouble>(pimpl->stop-pimpl->start).count();
  }

  std::ostream& operator<<(std::ostream& stream, const StopClock& clock)
  {
    double deltaMilli=clock.GetMilliseconds();
    size_t seconds=(size_t)deltaMilli/1000;
    size_t milliseconds=(size_t)deltaMilli-seconds*1000;

    stream << seconds << "." << std::setw(3) << std::setfill('0') << milliseconds;

    return stream;
  }

  std::string StopClock::ResultString() const
  {
    double deltaMilli=GetMilliseconds();
    size_t seconds=(size_t)deltaMilli/1000;
    size_t milliseconds = (size_t)deltaMilli - seconds * 1000;

    std::string result;
    std::string millisString;

    result=NumberToString(seconds);

    result+=".";

    millisString=NumberToString(milliseconds);
    for (size_t i=millisString.length()+1; i<=3; i++) {
      result+="0";
    }

    result+=millisString;

    return result;
  }
}
