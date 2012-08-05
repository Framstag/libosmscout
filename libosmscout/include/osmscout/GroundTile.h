#ifndef OSMSCOUT_GROUNDTILE_H
#define OSMSCOUT_GROUNDTILE_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2010  Tim Teulings

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

#include <vector>

#include <osmscout/Point.h>

namespace osmscout {

  struct OSMSCOUT_API TileCoord
  {
    static const uint16_t CELL_MAX;

    uint16_t x;
    uint16_t y;
    bool     coast;

    TileCoord()
    {
      // no code
    }

    TileCoord(uint16_t x,
              uint16_t y,
              bool coast)
    {
      this->x=x;
      this->y=y;
      this->coast=coast;
    }

    void Set(uint16_t x,
             uint16_t y,
             bool coast)
    {
      this->x=x;
      this->y=y;
      this->coast=coast;
    }
  };

  struct OSMSCOUT_API GroundTile
  {
    enum Type {
      unknown = 0,
      land    = 1, // left side of the coast
      water   = 2, // right side of the coast
      coast   = 3
    };

    Type                   type;
    size_t                 xAbs;
    size_t                 yAbs;
    size_t                 xRel;
    size_t                 yRel;
    double                 cellWidth;
    double                 cellHeight;
    std::vector<TileCoord> coords;

    GroundTile()
    {
      // no code
    }

    GroundTile(Type type)
    : type(type)
    {
      // no code
    }
  };
}

#endif
