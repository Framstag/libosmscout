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

#include <osmscout/util/Color.h>

#include <osmscout/system/Math.h>
#include <map>
#include <tuple>

namespace osmscout {

  const Color Color::BLACK(0.0,0.0,0.0);
  const Color Color::WHITE(1.0,1.0,1.0);

  const Color Color::RED(1.0,0.0,0.0);
  const Color Color::GREEN(0.0,1.0,0.0);
  const Color Color::BLUE(0.0,0.0,1.0);

  const Color Color::SILVER(192.0/255,192.0/255,192.0/255);
  const Color Color::GRAY(128.0/255,128.0/255,128.0/255);
  const Color Color::MAROON(128.0/255,0.0/255,0.0/255);
  const Color Color::PURPLE(128.0/255,0/255,128.0/255);
  const Color Color::FUCHSIA(255.0/255,0.0/255,255.0/255);
  const Color Color::LIME(0.0/255,255.0/255,0.0/255);
  const Color Color::OLIVE(128.0/255,128.0/255,0.0/255);
  const Color Color::YELLOW(255.0/255,255.0/255,0.0/255);
  const Color Color::NAVY(0.0/255,0.0/255,128.0/255);
  const Color Color::TEAL(0.0/255,128.0/255,128.0/255);
  const Color Color::AQUA(0.0/255,255.0/255,255.0/255);

  const Color Color::LIGHT_GRAY(211.0/255,211.0/255,211.0/255);
  const Color Color::DARK_GRAY(169.0/255,169.0/255,169.0/255);
  const Color Color::DARK_RED(139.0/255,0.0/255,0.0/255);
  const Color Color::DARK_GREEN(0.0/255,100.0/255,0.0/255);
  const Color Color::DARK_YELLOW(255.0/255,215.0/255,0.0/255);
  const Color Color::DARK_BLUE(0.0/255,0.0/255,139.0/255);
  const Color Color::DARK_FUCHSIA(139.0/255,0.0/255,139.0/255);
  const Color Color::DARK_AQUA(0.0/255,139.0/255,139.0/255);

  const Color Color::LUCENT_WHITE(1.0,1.0,1.0,1.0);


  static char GetHexChar(size_t value)
  {
    if (value<=9) {
      return '0'+(char)value;
    }

    return (char)('a'+(char)value-10);
  }

  static size_t GetHexValue(char digit)
  {
    if (digit>='0' && digit<='9') {
      return digit-'0';
    }

    if (digit>='a' && digit<='f') {
      return 10+(digit-'a');
    }

    assert(false);
    return 0;
  }

  std::string Color::ToHexString() const
  {
    if (a!=1.0) {
      std::string hexString("#        ");

      hexString[1]=GetHexChar((size_t)(r*255.0/16.0));
      hexString[2]=GetHexChar((size_t)fmod(r*255.0,16));
      hexString[3]=GetHexChar((size_t)(g*255.0/16.0));
      hexString[4]=GetHexChar((size_t)fmod(g*255.0,16));
      hexString[5]=GetHexChar((size_t)(b*255.0/16.0));
      hexString[6]=GetHexChar((size_t)fmod(b*255.0,16));
      hexString[7]=GetHexChar((size_t)(a*255.0/16.0));
      hexString[8]=GetHexChar((size_t)fmod(a*255.0,16));

      return hexString;
    }

    std::string hexString("#      ");

    hexString[1]=GetHexChar((size_t)(r*255.0/16.0));
    hexString[2]=GetHexChar((size_t)fmod(r*255.0,16));
    hexString[3]=GetHexChar((size_t)(g*255.0/16.0));
    hexString[4]=GetHexChar((size_t)fmod(g*255.0,16));
    hexString[5]=GetHexChar((size_t)(b*255.0/16.0));
    hexString[6]=GetHexChar((size_t)fmod(b*255.0,16));

    return hexString;
  }

  bool Color::operator<(const Color& other) const
  {
    return std::tie(r, g, b, a) < std::tie(other.r, other.g, other.b, other.a);
  }

  Color Color::FromHexString(const std::string& hexString)
  {
    assert(hexString.size()==7 || hexString.size()==9);
    double r=(16*GetHexValue(hexString[1])+GetHexValue(hexString[2]))/255.0;
    double g=(16*GetHexValue(hexString[3])+GetHexValue(hexString[4]))/255.0;
    double b=(16*GetHexValue(hexString[5])+GetHexValue(hexString[6]))/255.0;
    double a;

    if (hexString.length()==9) {
      a=(16*GetHexValue(hexString[7])+GetHexValue(hexString[8]))/255.0;
    }
    else {
      a=1.0;
    }

    return Color(r,g,b,a);
  }

  bool Color::FromHexString(const std::string& hexString, Color &color)
  {
    if (!IsHexString(hexString)){
      return false;
    }
    color=FromHexString(hexString);
    return true;
  }

  bool Color::IsHexString(const std::string& hexString) {
    if (hexString.length()!=7 && hexString.length()!=9) {
      return false;
    }

    if (hexString[0]!='#') {
      return false;
    }

    for (size_t pos=1; pos<hexString.length(); pos++) {
      if (!(hexString[pos]>='0' && hexString[pos]<='9') &&
          !(hexString[pos]>='a' && hexString[pos]<='f')) {
       return false;
      }
    }

    return true;
  }

  bool Color::FromW3CKeywordString(const std::string& colorKeyword, Color &color)
  {
    static const std::map<std::string, Color> w3cColorKeywords {
        {"black", Color::BLACK}, // #000000
        {"silver", Color::SILVER}, // #C0C0C0
        {"gray", Color::GRAY}, {"grey", Color::GRAY}, // #808080
        {"white", Color::WHITE}, // #FFFFFF
        {"maroon", Color::MAROON}, // #800000
        {"red", Color::RED}, // #FF0000
        {"purple", Color::PURPLE}, // #800080
        {"fuchsia", Color::FUCHSIA}, {"magenta", Color::FUCHSIA}, // #FF00FF
        {"green", Color::GREEN}, // #008000
        {"lime", Color::LIME}, // #00FF00
        {"olive", Color::OLIVE}, // #808000
        {"yellow", Color::YELLOW}, // #FFFF00
        {"navy", Color::NAVY}, // #000080
        {"blue", Color::BLUE}, // #0000FF
        {"teal", Color::TEAL}, // #008080
        {"aqua", Color::AQUA}, {"cyan", Color::AQUA}, // #00FFFF
        {"lightgray", Color::LIGHT_GRAY}, {"lightgrey", Color::LIGHT_GRAY}, // #D3D3D3
        {"darkgray", Color::DARK_GRAY}, {"darkgrey", Color::DARK_GRAY}, // #A9A9A9
        {"darkred", Color::DARK_RED}, // #8B0000
        {"darkgreen", Color::DARK_GREEN}, // #006400
        {"darkyellow", Color::DARK_YELLOW}, // #FFD700
        {"darkblue", Color::DARK_BLUE}, // #00008B
        {"darkfuchsia", Color::DARK_FUCHSIA}, {"darkmagenta", Color::DARK_FUCHSIA},// #8B008B
        {"darkaqua", Color::DARK_AQUA}, {"darkcyan", Color::DARK_AQUA} // #008B8B
    };

    auto it=w3cColorKeywords.find(colorKeyword);
    if (it == w3cColorKeywords.end()) {
      return false;
    }
    color=it->second;
    return true;
  }
}

