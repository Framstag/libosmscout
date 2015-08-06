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

#include <osmscout/private/Config.h>

#include <osmscout/util/String.h>

#if defined(OSMSCOUT_HAVE_CHRONO_H)
#include <chrono>
#elif defined(HAVE_SYS_TIME_H)
  #include <sys/time.h>
#endif

namespace osmscout {

#if defined(OSMSCOUT_HAVE_CHRONO_H)
  typedef std::chrono::duration<double,std::milli> MilliDouble;
#elif defined(HAVE_SYS_TIME_H)
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
#if defined(OSMSCOUT_HAVE_CHRONO_H)
    std::chrono::steady_clock::time_point start;
    std::chrono::steady_clock::time_point stop;
#elif defined(HAVE_SYS_TIME_H)
    timeval start;
    timeval stop;
#endif
  };

  StopClock::StopClock()
    : pimpl(new StopClockPIMPL())
  {
#if defined(OSMSCOUT_HAVE_CHRONO_H)
    pimpl->start=std::chrono::steady_clock::now();
#elif defined(HAVE_SYS_TIME_H)
    gettimeofday(&pimpl->start,NULL);
#endif
  }

  StopClock::~StopClock()
  {
    delete pimpl;
  }

  void StopClock::Stop()
  {
#if defined(OSMSCOUT_HAVE_CHRONO_H)
    pimpl->stop=std::chrono::steady_clock::now();
#elif defined(HAVE_SYS_TIME_H)
    gettimeofday(&pimpl->stop,NULL);
#endif
  }

  double StopClock::GetMilliseconds() const
  {
#if defined(OSMSCOUT_HAVE_CHRONO_H)
    return std::chrono::duration_cast<MilliDouble>(pimpl->stop-pimpl->start).count();
#elif defined(HAVE_SYS_TIME_H)
    timeval diff;
    size_t  result;

    timersub(&pimpl->stop,&pimpl->start,&diff);

    result=diff.tv_sec*1000.0+diff.tv_usec/1000;

    return result;
#else
    return 0.0;
#endif
  }

  std::ostream& operator<<(std::ostream& stream, const StopClock& clock)
  {
#if defined(OSMSCOUT_HAVE_CHRONO_H)
    double deltaMilli=clock.GetMilliseconds();
    size_t seconds=(size_t)deltaMilli/1000;
    size_t milliseconds=(size_t)deltaMilli-seconds*1000;

    stream << seconds << "." << std::setw(3) << std::setfill('0') << milliseconds;
#elif defined(HAVE_SYS_TIME_H)
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
#if defined(OSMSCOUT_HAVE_CHRONO_H)
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
#elif defined(HAVE_SYS_TIME_H)
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
