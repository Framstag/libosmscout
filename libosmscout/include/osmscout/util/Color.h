#ifndef OSMSCOUT_UTIL_COLOR_H
#define OSMSCOUT_UTIL_COLOR_H

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

#include <osmscout/system/Assert.h>

namespace osmscout {

  class OSMSCOUT_API Color
  {
  public:
    static const Color BLACK;
    static const Color WHITE;

    static const Color RED;
    static const Color GREEN;
    static const Color BLUE;

  private:
    double r;
    double g;
    double b;
    double a;

  public:
    inline Color()
    : r(1.0),
      g(0.0),
      b(0.0),
      a(1.0)
    {
      // no code
    }

    inline Color(double r,
                 double g,
                 double b,
                 double a)
    : r(r),
      g(g),
      b(b),
      a(a)
    {
      assert(r>=0.0 && r<=1.0);
      assert(g>=0.0 && g<=1.0);
      assert(b>=0.0 && b<=1.0);
      assert(a>=0.0 && a<=1.0);
    }

    inline Color(double r,
                 double g,
                 double b)
    : r(r),
      g(g),
      b(b),
      a(1)
    {
      assert(r>=0.0 && r<=1.0);
      assert(g>=0.0 && g<=1.0);
      assert(b>=0.0 && b<=1.0);
      assert(a>=0.0 && a<=1.0);
    }

    inline Color(const Color& other)
    {
      this->r=other.r;
      this->g=other.g;
      this->b=other.b;
      this->a=other.a;
    }

    inline Color& operator=(const Color& other)
    {
      if (&other!=this) {
        this->r=other.r;
        this->g=other.g;
        this->b=other.b;
        this->a=other.a;
      }

      return *this;
    }

    inline double GetR() const
    {
      return r;
    }

    inline double GetG() const
    {
      return g;
    }

    inline double GetB() const
    {
      return b;
    }

    inline double GetA() const
    {
      return a;
    }

    inline bool IsSolid() const
    {
      return a==1.0;
    }

    inline bool IsVisible() const
    {
      return a>0.0;
    }

    inline Color Lighten(double factor) const
    {
      return Color(r+(1-r)*factor,
                   g+(1-g)*factor,
                   b+(1-b)*factor);
    }

    inline Color Darken(double factor) const
    {
      return Color(r-r*factor,
                   g-g*factor,
                   b-b*factor);
    }

    std::string ToHexString() const;

    inline bool operator==(const Color& other) const
    {
      return r==other.r && g==other.g && b==other.b && a==other.a;
    }

    inline bool operator!=(const Color& other) const
    {
      return r!=other.r || g!=other.g || b!=other.b || a!=other.a;
    }

    bool operator<(const Color& other) const;

    static Color FromHexString(const std::string& hexString);
  };
}

#endif
