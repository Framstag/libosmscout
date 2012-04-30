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

namespace osmscout {

  class OSMSCOUT_API Color
  {
  private:
    double r;
    double g;
    double b;
    double a;

  public:
    Color()
    : r(1),
      g(0),
      b(0),
      a(1)
    {
      // no code
    }

    Color(double r,
          double g,
          double b,
          double a)
    : r(r),
      g(g),
      b(b),
      a(a)
    {
      // no code
    }

    Color(double r,
          double g,
          double b)
    : r(r),
      g(g),
      b(b),
      a(1)
    {
      // no code
    }

    Color(const Color& other)
    {
      this->r=other.r;
      this->g=other.g;
      this->b=other.b;
      this->a=other.a;
    }

    Color& operator=(const Color& other)
    {
      if (&other!=this) {
        this->r=other.r;
        this->g=other.g;
        this->b=other.b;
        this->a=other.a;
      }

      return *this;
    }

    double GetR() const
    {
      return r;
    }

    double GetG() const
    {
      return g;
    }

    double GetB() const
    {
      return b;
    }

    double GetA() const
    {
      return a;
    }

    bool IsSolid() const
    {
      return a==1.0;
    }

    Color Lighten(double factor) const
    {
      return Color(r+(1-r)*factor,
                   g+(1-g)*factor,
                   b+(1-b)*factor);
    }
  };
}

#endif
