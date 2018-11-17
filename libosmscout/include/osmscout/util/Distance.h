#ifndef OSMSCOUT_DISTANCE_H
#define OSMSCOUT_DISTANCE_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2018 Lukáš Karas

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

#include <utility>
#include <limits>
#include <algorithm>

namespace osmscout {

  struct OSMSCOUT_API Meter
  {
    static inline double ToMeter(double m)
    {
      return m;
    }

    static inline double FromMeter(double m)
    {
      return m;
    }
  };

  struct OSMSCOUT_API Kilometer
  {
    static inline double ToMeter(double km)
    {
      return km*1000.0;
    }

    static inline double FromMeter(double m)
    {
      return m/1000.0;
    }
  };

  class OSMSCOUT_API Distance CLASS_FINAL
  {
  private:
    double meters;

  private:
    explicit inline Distance(double meters):
      meters(meters)
    { }

  public:
    inline Distance():
      meters(0.0)
    { }

    inline Distance(const Distance &d):
      meters(d.meters)
    { }

    inline Distance& operator=(const Distance &d)
    {
      meters=d.meters;
      return *this;
    }

    inline Distance(Distance &&d)
    {
      std::swap(meters, d.meters);
    }

    inline Distance &operator=(Distance &&d)
    {
      std::swap(meters, d.meters);
      return *this;
    }

    inline double AsMeter() const
    {
      return meters;
    }

    inline ~Distance()
    { }

    inline Distance& operator+=(const Distance &d)
    {
      meters+=d.meters;
      return *this;
    }

    inline Distance operator-(const Distance &d) const
    {
      return Distance(meters-d.meters);
    }

    inline Distance operator+(const Distance &d) const
    {
      return Distance(meters+d.meters);
    }

    inline Distance operator*(double factor) const
    {
      return Distance(meters*factor);
    }

    inline Distance operator/(double factor) const
    {
      return Distance(meters / factor);
    }

    inline bool operator>(const Distance &d) const
    {
      return meters > d.meters;
    }

    inline bool operator<(const Distance &d) const
    {
      return meters < d.meters;
    }

    inline bool operator>=(const Distance &d) const
    {
      return meters >= d.meters;
    }

    inline bool operator<=(const Distance &d) const
    {
      return meters <= d.meters;
    }

    template <typename Unit>
    double As() const
    {
      return Unit::FromMeter(meters);
    }

    static Distance Max();

    static Distance Min();

    static Distance Max(const Distance &a, const Distance &b);

    static Distance Min(const Distance &a, const Distance &b);

    template <typename Unit>
    static Distance Of(double value)
    {
      return Distance(Unit::ToMeter(value));
    }
  };

}

#endif //OSMSCOUT_DISTANCE_H
