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

#include <iomanip>
#include <sstream>

namespace osmscout {

  typedef std::chrono::duration<double,std::milli> MilliDouble;
  typedef std::chrono::duration<double,std::nano>  NanoDouble;

  StopClock::StopClock()
   : start(std::chrono::steady_clock::now()),
     stop(start)
  {
    // no code
  }

  void StopClock::Stop()
  {
    stop=std::chrono::steady_clock::now();
  }

  double StopClock::GetMilliseconds() const
  {
    return std::chrono::duration_cast<MilliDouble>(stop-start).count();
  }

  std::ostream& operator<<(std::ostream& stream, const StopClock& clock)
  {
    double   deltaMilli=clock.GetMilliseconds();
    uint64_t seconds=(uint64_t)deltaMilli/1000;
    uint64_t milliseconds=(uint64_t)deltaMilli-seconds*1000;

    stream << seconds << "." << std::setw(3) << std::setfill('0') << milliseconds;

    return stream;
  }

  std::string StopClock::ResultString() const
  {
    double   deltaMilli=GetMilliseconds();
    uint64_t seconds=(uint64_t)deltaMilli/1000;
    uint64_t milliseconds = (uint64_t)deltaMilli - seconds * 1000;

    std::string result;
    std::string millisString;

    result=std::to_string(seconds);

    result+=".";

    millisString=std::to_string(milliseconds);
    for (size_t i=millisString.length()+1; i<=3; i++) {
      result+="0";
    }

    result+=millisString;

    return result;
  }

  /**
   * Return true, if the measured time is siginificant, which means, that it has an value of at least
   * one millisecond.
   *
   * @return true if significant, else false
   */
  bool StopClock::IsSignificant() const
  {
    return GetMilliseconds()>=1.0;
  }

  StopClockNano::StopClockNano()
    : start(std::chrono::high_resolution_clock::now()),
      stop(start)
  {
    // no code
  }

  void StopClockNano::Stop()
  {
    stop=std::chrono::high_resolution_clock::now();
  }

  double StopClockNano::GetNanoseconds() const
  {
    return std::chrono::duration_cast<NanoDouble>(stop-start).count();
  }

  std::ostream& operator<<(std::ostream& stream, const StopClockNano& clock)
  {
    double deltaNano=clock.GetNanoseconds();

    stream <<  deltaNano;

    return stream;
  }

  std::string StopClockNano::ResultString() const
  {
    double deltaNano=GetNanoseconds();

    return std::to_string((uint64_t)deltaNano);
  }

}
