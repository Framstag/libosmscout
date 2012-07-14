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

#include <iostream>

#include <osmscout/util/FileScanner.h>

#include <osmscout/private/Math.h>

namespace osmscout {

  WaterIndex::WaterIndex()
  : filepart("water.idx")
  {
    // no code
  }

  bool WaterIndex::Load(const std::string& path)
  {
    datafilename=path+"/"+filepart;

    if (!scanner.Open(datafilename)) {
      std::cerr << "Cannot open file '" << datafilename << "'" << std::endl;
      return false;
    }

    uint32_t waterIndexMaxMag;

    if (!scanner.ReadNumber(waterIndexMaxMag)) {
      std::cerr << "Error while reading from file '" << datafilename << "'" << std::endl;
      return false;
    }

    levels.resize(waterIndexMaxMag+1);

    levels[0].cellWidth=360.0;
    levels[0].cellHeight=180.0;

    for (size_t level=1; level<levels.size(); level++) {
      levels[level].cellWidth=levels[level-1].cellWidth/2;
      levels[level].cellHeight=levels[level-1].cellHeight/2;
    }

    for (size_t level=0; level<levels.size(); level++){
      scanner.ReadFileOffset(levels[level].offset);

      scanner.ReadNumber(levels[level].cellXStart);
      scanner.ReadNumber(levels[level].cellXEnd);
      scanner.ReadNumber(levels[level].cellYStart);
      scanner.ReadNumber(levels[level].cellYEnd);

      levels[level].cellXCount=levels[level].cellXEnd-levels[level].cellXStart+1;
      levels[level].cellYCount=levels[level].cellYEnd-levels[level].cellYStart+1;
    }

    if (scanner.HasError()) {
      std::cerr << "Error while reading from file '" << datafilename << "'" << std::endl;
      return false;
    }

    return scanner.Close();
  }

  bool WaterIndex::GetRegions(double minlon,
                              double minlat,
                              double maxlon,
                              double maxlat,
                              double magnification,
                              std::list<GroundTile>& tiles) const
  {
    uint32_t cx1,cx2,cy1,cy2;
    size_t level=MagToLevel(magnification);

    if (levels.empty()) {
      return true;
    }

    level+=4;

    if (level>=levels.size()) {
      level=levels.size()-1;
    }

    tiles.clear();

    if (!scanner.IsOpen()) {
      if (!scanner.Open(datafilename)) {
        std::cerr << "Error while opening " << datafilename << " for reading!" << std::endl;
        return false;
      }
    }

    cx1=(uint32_t)floor((minlon+180.0)/levels[level].cellWidth);
    cx2=(uint32_t)floor((maxlon+180.0)/levels[level].cellWidth);
    cy1=(uint32_t)floor((minlat+90.0)/levels[level].cellHeight);
    cy2=(uint32_t)floor((maxlat+90.0)/levels[level].cellHeight);

    GroundTile tile;

    tile.points.reserve(5);

    for (size_t y=cy1; y<=cy2; y++) {
      for (size_t x=cx1; x<=cx2; x++) {
        tile.points.clear();

        tile.points.push_back(Point(0,
                                    y*levels[level].cellHeight-90.0,
                                    x*levels[level].cellWidth-180.0));

        tile.points.push_back(Point(1,
                                    y*levels[level].cellHeight-90.0,
                                    (x+1)*levels[level].cellWidth-180.0));

        tile.points.push_back(Point(2,
                                    (y+1)*levels[level].cellHeight-90.0,
                                    (x+1)*levels[level].cellWidth-180.0));

        tile.points.push_back(Point(3,
                                    (y+1)*levels[level].cellHeight-90.0,
                                    x*levels[level].cellWidth-180.0));

        tile.points.push_back(tile.points.front());

        if (x<levels[level].cellXStart ||
            x>levels[level].cellXEnd ||
            y<levels[level].cellYStart ||
            y>levels[level].cellYEnd) {
          tile.type=GroundTile::unknown;
          tile.x=x-levels[level].cellXStart;
          tile.y=y-levels[level].cellYStart;

          tiles.push_back(tile);
        }
        else {
          uint32_t   cellId=(y-levels[level].cellYStart)*levels[level].cellXCount+x-levels[level].cellXStart;

          uint32_t   index=cellId*8;
          FileOffset cell;

          scanner.SetPos(levels[level].offset+index);

          if (!scanner.Read(cell)) {
            std::cerr << "Error while reading from file '" << datafilename << "'" << std::endl;
            return false;
          }

          if (cell==(FileOffset)GroundTile::land ||
              cell==(FileOffset)GroundTile::water ||
              cell==(FileOffset)GroundTile::coast ||
              cell==(FileOffset)GroundTile::unknown) {
            tile.type=(GroundTile::Type)cell;
            tile.x=x-levels[level].cellXStart;
            tile.y=y-levels[level].cellYStart;

            tiles.push_back(tile);
          }
          else {
            size_t tileCount;

            tile.type=GroundTile::coast;
            tile.x=x-levels[level].cellXStart;
            tile.y=y-levels[level].cellYStart;

            tiles.push_back(tile);

            scanner.SetPos(cell);
            scanner.ReadNumber(tileCount);

            for (size_t t=1; t<=tileCount; t++) {
              uint8_t    tileType;
              size_t     nodeCount;

              scanner.Read(tileType);

              tile.type=(GroundTile::Type)tileType;
              tile.x=x-levels[level].cellXStart;
              tile.y=y-levels[level].cellYStart;

              scanner.ReadNumber(nodeCount);

              tile.points.resize(nodeCount);

              for (size_t n=0;n<nodeCount; n++) {
                Id       id;
                uint32_t latDat;
                uint32_t lonDat;

                scanner.ReadNumber(id);
                scanner.ReadNumber(latDat);
                scanner.ReadNumber(lonDat);

                tile.points[n].Set(id,
                                   latDat/conversionFactor-90.0,
                                   lonDat/conversionFactor-180.0);
              }

              tiles.push_back(tile);
            }
          }
        }
      }
    }

    return true;
  }

  void WaterIndex::DumpStatistics()
  {
    size_t entries=0;
    size_t memeory=0;

    std::cout << "WaterIndex size " << entries << ", memory " << memeory << std::endl;
  }
}

