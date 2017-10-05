#ifndef OSMSCOUT_UTIL_MAGNIFICATION_H
#define OSMSCOUT_UTIL_MAGNIFICATION_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2012  Tim Teulings

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

#include <osmscout/private/CoreImportExport.h>

#include <string>
#include <unordered_map>

#include <osmscout/system/Types.h>

namespace osmscout {

  class OSMSCOUT_API Magnification
  {
  public:
    enum Mag {
      magWorld     =                        1, //  0
      magContinent =                       16, //  4
      magState     =                       32, //  5
      magStateOver =                       64, //  6
      magCounty    =                      128, //  7
      magRegion    =                      256, //  8
      magProximity =                      512, //  9
      magCityOver  =                     1024, // 10
      magCity      =                   2*1024, // 11
      magSuburb    =                 2*2*1024, // 12
      magDetail    =               2*2*2*1024, // 13
      magClose     =             2*2*2*2*1024, // 14
      magCloser    =           2*2*2*2*2*1024, // 15
      magVeryClose =         2*2*2*2*2*2*1024, // 16
      magBlock     =     2*2*2*2*2*2*2*2*1024, // 18
      magStreet    =   2*2*2*2*2*2*2*2*2*1024, // 19
      magHouse     = 2*2*2*2*2*2*2*2*2*2*1024  // 20
    };

  private:
    double   magnification;
    uint32_t level;

  public:
    inline Magnification()
    : magnification(1),
      level(0)
    {
      // no code
    }

    inline Magnification(const Magnification& other)
    : magnification(other.magnification),
      level(other.level)
    {
      // no code
    }

    inline Magnification(double magnification)
    {
      SetMagnification(magnification);
    }

    void SetMagnification(double magnification);
    void SetMagnification(Mag magnification);
    void SetLevel(uint32_t level);

    inline double GetMagnification() const
    {
      return magnification;
    }

    inline uint32_t GetLevel() const
    {
      return level;
    }

    inline Magnification& operator=(const Magnification& other)
    {
      if (this!=&other) {
        this->magnification=other.magnification;
        this->level=other.level;
      }

      return *this;
    }

    inline bool operator==(const Magnification& other) const
    {
      return magnification==other.magnification;
    }

    inline bool operator!=(const Magnification& other) const
    {
      return magnification!=other.magnification;
    }

    inline bool operator<(const Magnification& other) const
    {
      return magnification<other.magnification;
    }

    inline bool operator<=(const Magnification& other) const
    {
      return magnification<=other.magnification;
    }

    inline bool operator>=(const Magnification& other) const
    {
      return magnification>=other.magnification;
    }

    inline bool operator>(const Magnification& other) const
    {
      return magnification>other.magnification;
    }
  };

  class OSMSCOUT_API MagnificationConverter
  {
  private:
    std::unordered_map<std::string,Magnification::Mag> stringToMagMap;
    std::unordered_map<size_t,std::string>             levelToStringMap;

  public:
    MagnificationConverter();

    bool Convert(const std::string& name,
                 Magnification& magnification);

    bool Convert(const size_t level,
                 std::string& name);
  };

}

#endif
