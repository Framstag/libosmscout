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

namespace osmscout {

  const Color Color::BLACK(0.0,0.0,0.0);
  const Color Color::WHITE(1.0,1.0,1.0);

  const Color Color::RED(1.0,0.0,0.0);
  const Color Color::GREEN(0.0,1.0,0.0);
  const Color Color::BLUE(0.0,0.0,1.0);

  static char GetHexChar(size_t value)
  {
    if (value<=9) {
      return '0'+(char)value;
    }
    else {
      return (char)('a'+(char)value-10);
    }
  }

  std::string Color::ToHexString() const
  {
    if (a!=1.0) {
      std::string hexString("        ");

      hexString[0]=GetHexChar(((size_t)(r*256.0))/16);
      hexString[1]=GetHexChar(((size_t)(r*256.0))%16);
      hexString[2]=GetHexChar(((size_t)(g*256.0))/16);
      hexString[3]=GetHexChar(((size_t)(g*256.0))%16);
      hexString[4]=GetHexChar(((size_t)(b*256.0))/16);
      hexString[5]=GetHexChar(((size_t)(b*256.0))%16);
      hexString[6]=GetHexChar(((size_t)(a*256.0))/16);
      hexString[7]=GetHexChar(((size_t)(a*256.0))%16);

      return hexString;
    }
    else {
      std::string hexString("      ");

      hexString[0]=GetHexChar(((size_t)(r*256.0))/16);
      hexString[1]=GetHexChar(((size_t)(r*256.0))%16);
      hexString[2]=GetHexChar(((size_t)(g*256.0))/16);
      hexString[3]=GetHexChar(((size_t)(g*256.0))%16);
      hexString[4]=GetHexChar(((size_t)(b*256.0))/16);
      hexString[5]=GetHexChar(((size_t)(b*256.0))%16);

      return hexString;
    }
  }

  bool Color::operator<(const Color& other) const
  {
    if (r!=other.r) {
      return r<other.r;
    }

    if (g!=other.g) {
      return g<other.g;
    }

    if (b!=other.b) {
      return b<other.b;
    }

    return a<other.a;
  }
}

