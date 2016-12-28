#ifndef OSMSCOUT_UTIL_STOPCLOCK_H
#define OSMSCOUT_UTIL_STOPCLOCK_H

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

#include <chrono>
#include <string>

#include <osmscout/system/Compiler.h>

#include <osmscout/private/CoreImportExport.h>

namespace osmscout {

  /**
   * \ingroup Util
   * Simple stop clock implementation.
   */
  class OSMSCOUT_API StopClock CLASS_FINAL
  {
  private:
    std::chrono::steady_clock::time_point start;
    std::chrono::steady_clock::time_point stop;

  private:
    // We do not want you to make copies of a stop clock
    StopClock(const StopClock& other);

  public:
    StopClock();

    void Stop();

    double GetMilliseconds() const;

    std::string ResultString() const;

    friend OSMSCOUT_API std::ostream& operator<<(std::ostream& stream, const StopClock& clock);
  };

  extern OSMSCOUT_API std::ostream& operator<<(std::ostream& stream, const StopClock& clock);

  /**
   * Copy of the StopClock implementation but using a high_resolution timer
   * and by default return nano seconds.
   */
  class OSMSCOUT_API StopClockNano CLASS_FINAL
  {
  private:
    std::chrono::high_resolution_clock::time_point start;
    std::chrono::high_resolution_clock::time_point stop;

  private:
    // We do not want you to make copies of a stop clock
    StopClockNano(const StopClockNano& other);

  public:
    StopClockNano();

    void Stop();

    double GetNanoseconds() const;

    std::string ResultString() const;

    friend OSMSCOUT_API std::ostream& operator<<(std::ostream& stream, const StopClockNano& clock);
  };

  extern OSMSCOUT_API std::ostream& operator<<(std::ostream& stream, const StopClockNano& clock);

}

#endif
