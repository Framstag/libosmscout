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

#include <osmscout/lib/CoreImportExport.h>

#include <string>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Compiler.h>

namespace osmscout {

  /**
   * Representation of a color with red, green and blue value
   * (RGB) and a alpha channel.
   *
   * This class follow the "rule of zero" (see https://en.cppreference.com/w/cpp/language/rule_of_three)
   */
  class OSMSCOUT_API Color CLASS_FINAL
  {
  public:
    static const Color BLACK;
    static const Color WHITE;

    static const Color RED;
    static const Color GREEN;
    static const Color BLUE;

    static const Color SILVER;
    static const Color GRAY;
    static const Color MAROON;
    static const Color PURPLE;
    static const Color FUCHSIA;
    static const Color LIME;
    static const Color OLIVE;
    static const Color YELLOW;
    static const Color NAVY;
    static const Color TEAL;
    static const Color AQUA;

    static const Color LIGHT_GRAY;
    static const Color DARK_GRAY;
    static const Color DARK_RED;
    static const Color DARK_GREEN;
    static const Color DARK_YELLOW;
    static const Color DARK_BLUE;
    static const Color DARK_FUCHSIA;
    static const Color DARK_AQUA;

    static const Color LUCENT_WHITE;

  private:
    double r=1.0;
    double g=0.0;
    double b=0.0;
    double a=0.0;

  public:
    Color() = default;
    ~Color() = default;

    Color(double r,
          double g,
          double b,
          double a) noexcept
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

    Color(double r,
          double g,
          double b) noexcept
    : r(r),
      g(g),
      b(b),
      a(1.0)
    {
      assert(r>=0.0 && r<=1.0);
      assert(g>=0.0 && g<=1.0);
      assert(b>=0.0 && b<=1.0);
      assert(a>=0.0 && a<=1.0);
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

    bool IsVisible() const
    {
      return a>0.0;
    }

    Color Lighten(double factor) const
    {
      return Color(r+(1-r)*factor,
                   g+(1-g)*factor,
                   b+(1-b)*factor,
                   a);
    }

    Color Darken(double factor) const
    {
      return Color(r-r*factor,
                   g-g*factor,
                   b-b*factor,
                   a);
    }

    Color Alpha(double newAlpha) const
    {
      return Color(r,
                   g,
                   b,
                   newAlpha);
    }

    Color Decolor() const
    {
      double grey=(r+g+b)/3.0;
      return Color(grey,
                   grey,
                   grey,
                   a);
    }

    std::string ToHexString() const;

    bool operator==(const Color& other) const
    {
      return r==other.r && g==other.g && b==other.b && a==other.a;
    }

    bool operator!=(const Color& other) const
    {
      return r!=other.r || g!=other.g || b!=other.b || a!=other.a;
    }

    bool operator<(const Color& other) const;

    static bool IsHexString(const std::string& hexString);

    /**
     * Convert the given color string to a color value
     *
     * The string must either be of the format
     * - #HHHHHH
     * - #HHHHHHHH
     *
     * where '#' is the symbol itself and 'H' represents a hexadecimal value
     *
     * @param hexString (lowercase)
     * @return
     */
    static Color FromHexString(const std::string& hexString);

    static bool FromHexString(const std::string& hexString, Color &color);

    /**
     * Convert the give color keyword to a color value.
     * Just basic color set is supported. See https://www.w3.org/TR/css-color-3/#html4
     *
     * @param colorKeyword
     * @param color
     * @return true on success, false otherwise
     */
    static bool FromW3CKeywordString(const std::string& colorKeyword, Color &color);
  };
}

#endif
