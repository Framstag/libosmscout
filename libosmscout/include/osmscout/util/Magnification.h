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

#include <osmscout/lib/CoreImportExport.h>

#include <functional>
#include <ostream>
#include <string>
#include <unordered_map>

#include <osmscout/system/Compiler.h>
#include <osmscout/system/SystemTypes.h>

namespace osmscout {

  class OSMSCOUT_API MagnificationLevel CLASS_FINAL
  {
  private:
    uint32_t level = 0;

  public:
    MagnificationLevel() = default;

    explicit MagnificationLevel(uint32_t level) noexcept
      : level(level)
    {
    }

    MagnificationLevel(const MagnificationLevel& level) = default;

    uint32_t Get() const
    {
      return level;
    }

    MagnificationLevel& operator=(const MagnificationLevel& other) = default;

    MagnificationLevel& operator++()
    {
      ++level;

      return *this;
    }

    MagnificationLevel operator++(int)
    {
      ++level;

      return *this;
    }

    MagnificationLevel& operator+=(uint32_t increment)
    {
      level+=increment;

      return *this;
    }

    bool operator==(const MagnificationLevel& other) const
    {
      return level==other.level;
    }

    bool operator!=(const MagnificationLevel& other) const
    {
      return level!=other.level;
    }

    bool operator<(const MagnificationLevel& other) const
    {
      return level<other.level;
    }

    bool operator<=(const MagnificationLevel& other) const
    {
      return level<=other.level;
    }

    bool operator>=(const MagnificationLevel& other) const
    {
      return level>=other.level;
    }

    bool operator>(const MagnificationLevel& other) const
    {
      return level>other.level;
    }
  };

  inline std::ostream& operator<<(std::ostream& os,
                                  const MagnificationLevel& level)
  {
    os << level.Get();

    return os;
  }

  inline std::string operator+(const char* text,
                               const MagnificationLevel& level)
  {
    return std::string(text)+std::to_string(level.Get());
  }

  inline std::string operator+(const std::string& text,
                               const MagnificationLevel& level)
  {
    return text+std::to_string(level.Get());
  }
}

namespace std {
  template <>
  struct hash<osmscout::MagnificationLevel>
  {
    size_t operator()(const osmscout::MagnificationLevel& level) const
    {
      return hash<uint32_t>{}(level.Get());
    }
  };
}

namespace osmscout {
  class OSMSCOUT_API Magnification CLASS_FINAL
  {
  public:
    static MagnificationLevel magWorld;     //  0
    static MagnificationLevel magContinent; //  4
    static MagnificationLevel magState;     //  5
    static MagnificationLevel magStateOver; //  6
    static MagnificationLevel magCounty;    //  7
    static MagnificationLevel magRegion;    //  8
    static MagnificationLevel magProximity; //  9
    static MagnificationLevel magCityOver;  // 10
    static MagnificationLevel magCity;      // 11
    static MagnificationLevel magSuburb;    // 12
    static MagnificationLevel magDetail;    // 13
    static MagnificationLevel magClose;     // 14
    static MagnificationLevel magCloser;    // 15
    static MagnificationLevel magVeryClose; // 16
    static MagnificationLevel magBlock;     // 18
    static MagnificationLevel magStreet;    // 19
    static MagnificationLevel magHouse;     // 20

  private:
    double   magnification=1;
    uint32_t level=0;

  public:
    Magnification() = default;

    /**
     * Create specific magnification.
     * @param magnification value, have to be valid - greater or equals to 1 (magnification level >= 0)
     */
    explicit Magnification(double magnification) noexcept
    {
      SetMagnification(magnification);
    }

    explicit Magnification(const MagnificationLevel& level) noexcept
    {
      SetLevel(level);
    }

    /**
     * Set magnification.
     * @param magnification value, have to be valid - greater or equals to 1 (magnification level >= 0)
     */
    void SetMagnification(double magnification);

    void SetLevel(const MagnificationLevel& level);

    double GetMagnification() const
    {
      return magnification;
    }

    uint32_t GetLevel() const
    {
      return level;
    }

    bool operator==(const Magnification& other) const
    {
      return magnification==other.magnification;
    }

    bool operator!=(const Magnification& other) const
    {
      return magnification!=other.magnification;
    }

    bool operator<(const Magnification& other) const
    {
      return magnification<other.magnification;
    }

    bool operator<=(const Magnification& other) const
    {
      return magnification<=other.magnification;
    }

    bool operator>=(const Magnification& other) const
    {
      return magnification>=other.magnification;
    }

    bool operator>(const Magnification& other) const
    {
      return magnification>other.magnification;
    }

    Magnification& operator++()
    {
      magnification*=2.0;
      level+=1;

      return *this;
    }

    Magnification operator++(int)
    {
      magnification*=2.0;
      level+=1;

      return *this;
    }
  };

  class OSMSCOUT_API MagnificationConverter
  {
  private:
    std::unordered_map<std::string,MagnificationLevel> stringToMagMap;
    std::unordered_map<MagnificationLevel,std::string> levelToStringMap;

  public:
    MagnificationConverter();

    bool Convert(const std::string& name,
                 Magnification& magnification);

    bool Convert(const MagnificationLevel& level,
                 std::string& name);
  };
}

#endif
