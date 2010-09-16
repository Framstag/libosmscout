#ifndef OSMSCOUT_WATERINDEX_H
#define OSMSCOUT_WATERINDEX_H

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

#include <osmscout/FileScanner.h>
#include <osmscout/GroundTile.h>

namespace osmscout {

  /**
    */
  class WaterIndex
  {
  private:
    std::string                filepart;

    double                     cellWidth;
    double                     cellHeight;
    uint32_t                   cellXStart;
    uint32_t                   cellXEnd;
    uint32_t                   cellYStart;
    uint32_t                   cellYEnd;
    uint32_t                   cellXCount;
    uint32_t                   cellYCount;

    std::vector<unsigned char> area;

  private:
    GroundTile::Type GetType(uint32_t x, uint32_t y) const;

  public:
    WaterIndex();

    bool Load(const std::string& path);

    bool GetRegions(double minlon,
                    double minlat,
                    double maxlon,
                    double maxlat,
                    std::list<GroundTile>& tiles) const;

    void DumpStatistics();
  };
}

#endif
