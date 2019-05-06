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
    double grad=round(AsDegrees());

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
    else if (grad>315 && grad<=360) {
      return "N";
    }
    else if (std::isnan(grad)){
      return "?";
    }

    assert(false);
    return "?";
  }

  std::string Bearing::LongDisplayString() const
  {
    double grad=AsDegrees();

    if (grad>=0 && grad<=12.5) {
      return "N";
    }
    else if (grad>12.5 && grad<=57.5) {
      return "NE";
    }
    else if (grad>57.5 && grad<=102.5) {
      return "E";
    }
    else if (grad>102.5 && grad<=147.5) {
      return "SE";
    }
    else if (grad>147.5 && grad<=192.5) {
      return "S";
    }
    else if (grad>192.5 && grad<=237.5) {
      return "SW";
    }
    else if (grad>237.5 && grad<=282.5) {
      return "W";
    }
    else if (grad>282.5 && grad<327.5) {
      return "NW";
    }
    else if (grad>327.5 && grad<=360) {
      return "N";
    }
    else if (std::isnan(grad)){
      return "?";
    }

    assert(false);
    return "?";
  }

}