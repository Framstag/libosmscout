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

#include <osmscout/private/Config.h>

#include <osmscout/util/String.h>

#if defined(HAVE_SYS_TIME_H)
  #include <sys/time.h>
#endif

namespace osmscout {

#if defined(HAVE_SYS_TIME_H)
  #ifndef timersub
      # define timersub(a, b, result) \
            do { \
                  (result)->tv_sec = (a)->tv_sec - (b)->tv_sec; \
                  (result)->tv_usec = (a)->tv_usec - (b)->tv_usec; \
                  if ((result)->tv_usec < 0) { \
                        --(result)->tv_sec; \
                    (result)->tv_usec += 1000000; \
                  } \
            } while (0)
  #endif
#endif

  struct StopClock::StopClockPIMPL
  {
#if defined(HAVE_SYS_TIME_H)
    timeval start;
    timeval stop;
#endif
  };

  StopClock::StopClock()
    : pimpl(new StopClockPIMPL())
  {
#if defined(HAVE_SYS_TIME_H)
    gettimeofday(&pimpl->start,NULL);
#endif
  }

  StopClock::~StopClock()
  {
    delete pimpl;
  }

  void StopClock::Stop()
  {
#if defined(HAVE_SYS_TIME_H)
    gettimeofday(&pimpl->stop,NULL);
#endif
  }

  std::ostream& operator<<(std::ostream& stream, const StopClock& clock)
  {
#if defined(HAVE_SYS_TIME_H)
    timeval diff;

    timersub(&clock.pimpl->stop,&clock.pimpl->start,&diff);

    stream << diff.tv_sec << "." << std::setw(3) << std::setfill('0') << diff.tv_usec/1000;
#else
    stream << "X.XXX";
#endif

    return stream;
  }

  std::string StopClock::ResultString() const
  {
#if defined(HAVE_SYS_TIME_H)
    timeval     diff;
    std::string result;
    std::string seconds;
    std::string millis;

    timersub(&pimpl->stop,&pimpl->start,&diff);

    seconds=NumberToString(diff.tv_sec);
    millis=NumberToString(diff.tv_usec/1000);

    result=seconds;
    result+=".";

    for (size_t i=millis.length()+1; i<=3; i++) {
      result+="0";
    }

    result+=millis;

    return result;
#else
    return "X.XXX";
#endif
  }
}
