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

#include <osmscout/Types.h>

namespace osmscout {

  /**
   * A single ground tile cell. The ground tile defines an area
   * of the given type.
   *
   * If the coords array is empty, the area is the complete cell.
   * If the coords array is not empty it is defining a polygon which
   * is of the given type.
   *
   * A cell can either have no GroundTile, one GroundTile that fills
   * the complete cell area or multiple GroundTiles that only fill
   * parts of the cell area.
   *
   * The polygon can consist (partly) of a coastline (Coord.coast=true) or
   * of cell boundary lines (Coord.cell=false).
   */
  struct OSMSCOUT_API GroundTile
  {
    enum Type {
      unknown = 0,
      land    = 1, // left side of the coast
      water   = 2, // right side of the coast
      coast   = 3
    };

    /**
     * A Coordinate for a point in a ground tile path.
     */
    struct OSMSCOUT_API Coord
    {
      static const uint16_t CELL_MAX;

      uint16_t x;
      uint16_t y;
      bool     coast;

      inline Coord()
      {
        // no code
      }

      inline Coord(uint16_t x,
                   uint16_t y,
                   bool coast)
      {
        this->x=x;
        this->y=y;
        this->coast=coast;
      }

      inline void Set(uint16_t x,
                      uint16_t y,
                      bool coast)
      {
        this->x=x;
        this->y=y;
        this->coast=coast;
      }
    };

    Type               type;
    size_t             xAbs;
    size_t             yAbs;
    size_t             xRel;
    size_t             yRel;
    double             cellWidth;
    double             cellHeight;
    std::vector<Coord> coords;

    inline GroundTile()
    {
      // no code
    }

    inline GroundTile(Type type)
    : type(type)
    {
      // no code
    }
  };
}

#endif
