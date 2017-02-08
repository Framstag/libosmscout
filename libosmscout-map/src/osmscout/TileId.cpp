/*
  This source is part of the libosmscout library
  Copyright (C) 2016  Tim Teulings

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

#include <osmscout/TileId.h>

#include <osmscout/util/String.h>

namespace osmscout {

  /**
   * Ceate a new tile by passing magnification and tile coordinates
   */
  TileId::TileId(const Magnification& magnification,
                 size_t x,
                 size_t y)
  : level(magnification.GetLevel()),
    x(x),
    y(y),
    boundingBox(GeoCoord(y*cellDimension[magnification.GetLevel()].height-90.0,
                         x*cellDimension[magnification.GetLevel()].width-180.0),
                GeoCoord((y+1)*cellDimension[magnification.GetLevel()].height-90.0,
                         (x+1)*cellDimension[magnification.GetLevel()].width-180.0))
  {
    // no code
  }

  /**
   * Return a short human readable description of the tile id
   */
  std::string TileId::DisplayText() const
  {
    return NumberToString(level)+ "." + NumberToString(y) + "." + NumberToString(x);
  }

  /**
   * Return the parent tile.
   *
   * Note that the parent tile will cover a 4 times bigger region than the current tile.
   *
   * Note that for tiles with level 0 no parent tile will exist. The method will assert in this case!
   */
  TileId TileId::GetParent() const
  {
    Magnification zoomedOutMagnification;

    assert(level>0);

    zoomedOutMagnification.SetLevel(level-1);

    return TileId(zoomedOutMagnification,x/2,y/2);
  }

  /**
   * Compare tile ids for equality
   */
  bool TileId::operator==(const TileId& other) const
  {
    return level==other.level &&
           y==other.y &&
           x==other.x;
  }

  /**
   * Compare tile ids for inequality
   */
  bool TileId::operator!=(const TileId& other) const
  {
    return level!=other.level ||
           y!=other.y ||
           x!=other.x;
  }

  /**
   * Compare tile ids by their order. Needed for sorting tile ids and placing them into (some)
   * containers.
   */
  bool TileId::operator<(const TileId& other) const
  {
    if (level!=other.level) {
      return level<other.level;
    }

    if (y!=other.y) {
      return y<other.y;
    }

    return x<other.x;
  }
}
