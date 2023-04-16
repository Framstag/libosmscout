/*
  This source is part of the libosmscout library
  Copyright (C) 2023  Tim Teulings

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

#include <osmscout/projection/Earth.h>

#include <osmscout/system/Math.h>

namespace osmscout {

  /*
   * For the calculations here see:
   * http://en.wikipedia.org/wiki/Mercator_projection
   * http://en.wikipedia.org/wiki/Web_Mercator
   * http://wiki.openstreetmap.org/wiki/Slippy_map_tilenames
   */

  //< Radius of the earth in meter
  const double Earth::radiusMeter=6'378'137.0;
  //< Extent of the earth in meter
  const double Earth::extentMeter=2*M_PI*Earth::radiusMeter;
}
