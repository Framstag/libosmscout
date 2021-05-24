#ifndef LIBOSMSCOUT_BEARING_H
#define LIBOSMSCOUT_BEARING_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2019 Lukáš Karas

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

#include <osmscout/CoreImportExport.h>
#include <osmscout/system/Compiler.h>
#include <osmscout/system/Math.h>

#include <string>
#include <ostream>

namespace osmscout {

  class OSMSCOUT_API Bearing CLASS_FINAL
  {
  private:
    double radians=0.0; //!< bearing in radians, normalised to [0..2*M_PI)

  private:
    explicit Bearing(double radians):
        radians(Normalise(radians))
    { }

  public:
    Bearing() = default;
    ~Bearing() = default;

    Bearing(const Bearing &d) = default;

    Bearing& operator=(const Bearing &d) = default;

    Bearing(Bearing &&d) noexcept
    {
      std::swap(radians, d.radians);
    }

    Bearing &operator=(Bearing &&d) noexcept
    {
      std::swap(radians, d.radians);
      return *this;
    }

    /**
     * Bearing in radians, normalised to [0..2*M_PI)
     */
    double AsRadians() const
    {
      return radians;
    }

    /**
     * Bearing in degrees, normalised to [0..360)
     */
    double AsDegrees() const
    {
      return radians*180.0/M_PI;
    }

    Bearing operator-(const Bearing &d) const
    {
      return Bearing(radians-d.radians);
    }

    Bearing operator+(const Bearing &d) const
    {
      return Bearing(radians+d.radians);
    }

    Bearing operator*(const double &d) const
    {
      return Bearing(radians * d);
    }

    Bearing operator/(const double &d) const
    {
      return Bearing(radians / d);
    }

    /**
     * Convert the bearing to a direction description in relation to the compass (4 points).
     * One from the options: N, E, S, W
     */
    std::string DisplayString() const;

    /**
     * Convert the bearing to a direction description in relation to the compass (8-points).
     * One from the options: N, NE, E, SE, S, SW, W, NW
     */
    std::string LongDisplayString() const;

    bool operator==(const Bearing& o) const
    {
      return radians == o.radians;
    }

    bool operator!=(const Bearing& o) const
    {
      return radians != o.radians;
    }

    inline static Bearing Radians(double radians)
    {
      return Bearing(radians);
    }

    inline static Bearing Degrees(double degrees)
    {
      return Bearing(degrees*M_PI/180.0);
    }

  private:
    static double Normalise(double radians);
  };

}

#endif //LIBOSMSCOUT_BEARING_H
