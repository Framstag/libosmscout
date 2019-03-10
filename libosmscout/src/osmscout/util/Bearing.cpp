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

#include <osmscout/util/Bearing.h>

#include <cassert>

namespace osmscout {

  double Bearing::Normalise(double radians)
  {
    radians = fmod(radians, 2*M_PI);
    if (radians < 0) {
      radians += 2*M_PI;
    }
    return radians;
  }

  std::string Bearing::DisplayString() const
  {
    int grad=(int)round(radians*180/M_PI);

    grad=grad % 360;

    if (grad<0) {
      grad+=360;
    }

    if (grad>=0 && grad<=45) {
      return "N";
    }
    else if (grad>45 && grad<=135) {
      return "E";
    }
    else if (grad>135 && grad<=225) {
      return "S";
    }
    else if (grad>225 && grad<=315) {
      return "W";
    }
    else if (grad>315 && grad<360) {
      return "N";
    }

    assert(false);
    return "?";
  }

}