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

#include <osmscout/WaterIndex.h>

#include <cmath>
#include <iostream>

namespace osmscout {

  WaterIndex::WaterIndex()
  : filepart("water.idx")
  {
    // no code
  }

  GroundTile::Type WaterIndex::GetType(uint32_t x, uint32_t y) const
  {
    uint32_t cellId=y*cellXCount+x;
    uint32_t index=cellId/4;
    uint32_t offset=2*(cellId%4);

    return (GroundTile::Type)((area[index] >> offset) & 3);
  }

  bool WaterIndex::Load(const std::string& path)
  {
    FileScanner scanner;
    std::string file=path+"/"+filepart;

    if (!scanner.Open(file)) {
      std::cerr << "Cannot open file '" << file << "'" << std::endl;
      return false;
    }

    uint32_t waterIndexMaxMag;

    scanner.ReadNumber(waterIndexMaxMag);
    scanner.ReadNumber(cellXStart);
    scanner.ReadNumber(cellXEnd);
    scanner.ReadNumber(cellYStart);
    scanner.ReadNumber(cellYEnd);

    if (scanner.HasError()) {
      std::cerr << "Error while reading from file '" << file << "'" << std::endl;
      return false;
    }

    cellXCount=cellXEnd-cellXStart+1;
    cellYCount=cellYEnd-cellYStart+1;

    cellWidth=360.0;
    cellHeight=180.0;

    for (size_t i=2; i<=waterIndexMaxMag; i++) {
      cellWidth=cellWidth/2;
      cellHeight=cellHeight/2;
    }

    std::cout << "Cell dimension: " << cellWidth << "x" << cellHeight << std::endl;
    std::cout << "Cell rectangle: [" << cellXStart << "," << cellYStart << "]x[" << cellXEnd << "," << cellYEnd << "]";
    std::cout <<  " => " << cellXCount << "x" << cellYCount << std::endl;
    std::cout << "Array size: " << cellXCount*cellYCount/4/1024 << "kb" << std::endl;

    // In the beginning everything is undecided
    area.resize(cellXCount*cellYCount/4,0x00);

    for (size_t i=0; i<area.size(); i++) {
      if (!scanner.Read(area[i])) {
        std::cerr << "Error while reading from file '" << file << "'" << std::endl;
      }
    }

    return !scanner.HasError() && scanner.Close();
  }

  bool WaterIndex::GetRegions(double minlon,
                              double minlat,
                              double maxlon,
                              double maxlat,
                              std::list<GroundTile>& tiles) const
  {
    uint32_t cx1,cx2,cy1,cy2;

    tiles.clear();

    cx1=floor((minlon+180.0)/cellWidth);
    cx2=floor((maxlon+180.0)/cellWidth);
    cy1=floor((minlat+90.0)/cellHeight);
    cy2=floor((maxlat+90.0)/cellHeight);

    std::cout << "Cell rectangle: [" << cx1 << "," << cy1 << "]x[" << cx2 << "," << cy2 << "]" << std::endl;

    for (size_t y=cy1; y<=cy2; y++) {
      for (size_t x=cx1; x<=cx2; x++) {
        GroundTile tile;

        tile.minlon=x*cellWidth-180.0;
        tile.maxlon=(x+1)*cellWidth-180.0;
        tile.minlat=y*cellHeight-90.0;
        tile.maxlat=(y+1)*cellHeight-90.0;

        if (x<cellXStart || x>cellXEnd || y<cellYStart || y>cellYEnd) {
          tile.type=GroundTile::unknown;
        }
        else {
          tile.type=GetType(x-cellXStart,y-cellYStart);
        }

        tiles.push_back(tile);
      }
    }

    std::cout << "Returning " << tiles.size() << " ground tile(s)" << std::endl;

    return true;
  }

  void WaterIndex::DumpStatistics()
  {
    // TODO
  }
}

