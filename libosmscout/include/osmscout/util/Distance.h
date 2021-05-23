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
#include <string>
#include <ostream>
#include <memory>

namespace osmscout {

  enum class DistanceUnitSystem
  {
    Metrics,
    Imperial
  };

  class OSMSCOUT_API Distance CLASS_FINAL
  {
  private:
    double meters=0.0;

  private:
    explicit Distance(double meters):
      meters(meters)
    { }

  public:
    Distance() = default;
    ~Distance() = default;

    Distance(const Distance &d) = default;

    Distance& operator=(const Distance &d) = default;

     Distance(Distance &&d) noexcept
    {
      std::swap(meters, d.meters);
    }

     Distance &operator=(Distance &&d) noexcept
    {
      std::swap(meters, d.meters);
      return *this;
    }

     double AsMeter() const
    {
      return meters;
    }

     Distance& operator+=(const Distance &d)
    {
      meters+=d.meters;
      return *this;
    }

     Distance& operator-=(const Distance &d)
    {
      meters-=d.meters;
      return *this;
    }

     Distance& operator*=(double d)
    {
      meters*=d;
      return *this;
    }

     Distance& operator/=(double d)
    {
      meters/=d;
      return *this;
    }

     Distance operator-(const Distance &d) const
    {
      return Distance(meters-d.meters);
    }

     Distance operator+(const Distance &d) const
    {
      return Distance(meters+d.meters);
    }

     Distance operator*(double factor) const
    {
      return Distance(meters*factor);
    }

     Distance operator/(double factor) const
    {
      return Distance(meters / factor);
    }

     bool operator==(const Distance &d) const
    {
      return meters == d.meters;
    }

     bool operator!=(const Distance &d) const
    {
      return meters != d.meters;
    }

     bool operator>(const Distance &d) const
    {
      return meters > d.meters;
    }

     bool operator<(const Distance &d) const
    {
      return meters < d.meters;
    }

     bool operator>=(const Distance &d) const
    {
      return meters >= d.meters;
    }

     bool operator<=(const Distance &d) const
    {
      return meters <= d.meters;
    }

    template <typename Unit>
    double As() const
    {
      return Unit::FromMeter(meters);
    }

    std::string AsString() const;

    static Distance Zero();

    static Distance Max();

    /**
     * returns the smallest finite value of the given type
     * @return
     */
    static Distance Min();

    /**
     * returns the lowest finite value of the given type
     * @return
     */
    static Distance Lowest();

    static Distance Max(const Distance &a, const Distance &b);

    static Distance Min(const Distance &a, const Distance &b);

    template <typename Unit>
    static Distance Of(double value)
    {
      return Distance(Unit::ToMeter(value));
    }
  };

  inline std::ostream& operator<<(std::ostream& os,
                                  const Distance& distance)
  {
    os << distance.AsString();

    return os;
  }

  struct OSMSCOUT_API DistanceUnit
  {
  public:
    virtual ~DistanceUnit() = default;
    virtual class Distance Distance(double d) const = 0;
    virtual double Value(const class Distance &d) const = 0;
    virtual std::string UnitStr() const = 0;
  };

  using DistanceUnitPtr = std::shared_ptr<DistanceUnit>;

  class OSMSCOUT_API Meter: public DistanceUnit
  {
  public:
    ~Meter() override = default;

    class Distance Distance(double d) const override
    {
      return Distance::Of<Meter>(d);
    }

    double Value(const class Distance &d) const override
    {
      return d.As<Meter>();
    }

    std::string UnitStr() const override
    {
      return "m";
    };

    static  double ToMeter(double m)
    {
      return m;
    }

    static  double FromMeter(double m)
    {
      return m;
    }
  };

  struct OSMSCOUT_API Kilometer: public DistanceUnit
  {
  public:
    ~Kilometer() override = default;

    class Distance Distance(double d) const override
    {
      return Distance::Of<Kilometer>(d);
    }

    double Value(const class Distance &d) const override
    {
      return d.As<Kilometer>();
    }

    std::string UnitStr() const override
    {
      return "km";
    };

    static  double ToMeter(double km)
    {
      return km*1000.0;
    }

    static  double FromMeter(double m)
    {
      return m/1000.0;
    }
  };

  struct OSMSCOUT_API Feet: public DistanceUnit
  {
  public:
    ~Feet() override = default;

    class Distance Distance(double d) const override
    {
      return Distance::Of<Feet>(d);
    }

    double Value(const class Distance &d) const override
    {
      return d.As<Feet>();
    }

    std::string UnitStr() const override
    {
      return "ft";
    };

    static  double ToMeter(double feet)
    {
      return feet * 0.3048;
    }

    static  double FromMeter(double m)
    {
      return m/0.3048;
    }
  };

  struct OSMSCOUT_API Yard: public DistanceUnit
  {
  public:
    ~Yard() override = default;

    class Distance Distance(double d) const override
    {
      return Distance::Of<Yard>(d);
    }

    double Value(const class Distance &d) const override
    {
      return d.As<Yard>();
    }

    std::string UnitStr() const override
    {
      return "yard";
    };

    static  double ToMeter(double yard)
    {
      return yard / 0.9144;
    }

    static  double FromMeter(double m)
    {
      return m * 0.9144;
    }
  };

  struct OSMSCOUT_API Mile: public DistanceUnit
  {
  public:
    ~Mile() override = default;

    class Distance Distance(double d) const override
    {
      return Distance::Of<Mile>(d);
    }

    double Value(const class Distance &d) const override
    {
      return d.As<Mile>();
    }

    std::string UnitStr() const override
    {
      return "mi";
    };

    static  double ToMeter(double mile)
    {
      return mile * 1609.344;
    }

    static  double FromMeter(double m)
    {
      return m / 1609.344;
    }
  };

   inline Distance Meters(double m){
    return Distance::Of<Meter>(m);
  }

   inline Distance Kilometers(double km){
    return Distance::Of<Kilometer>(km);
  }

}

#endif //OSMSCOUT_DISTANCE_H
