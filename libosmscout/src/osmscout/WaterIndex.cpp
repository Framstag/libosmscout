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

    if (!scanner.ReadNumber(waterIndexMinMag)) {
      std::cerr << "Error while reading from file '" << datafilename << "'" << std::endl;
      return false;
    }

    if (!scanner.ReadNumber(waterIndexMaxMag)) {
      std::cerr << "Error while reading from file '" << datafilename << "'" << std::endl;
      return false;
    }

    levels.resize(waterIndexMaxMag-waterIndexMinMag+1);

    double cellWidth=360.0;
    double cellHeight=180.0;

    for (size_t level=0; level<=waterIndexMaxMag; level++) {
      if (level>=waterIndexMinMag && level<=waterIndexMaxMag) {
        size_t idx=level-waterIndexMinMag;

        levels[idx].cellWidth=cellWidth;
        levels[idx].cellHeight=cellHeight;
      }

      cellWidth=cellWidth/2;
      cellHeight=cellHeight/2;
    }

    for (size_t level=waterIndexMinMag; level<=waterIndexMaxMag; level++){
      size_t idx=level-waterIndexMinMag;

      scanner.ReadFileOffset(levels[idx].offset);

      scanner.ReadNumber(levels[idx].cellXStart);
      scanner.ReadNumber(levels[idx].cellXEnd);
      scanner.ReadNumber(levels[idx].cellYStart);
      scanner.ReadNumber(levels[idx].cellYEnd);

      levels[idx].cellXCount=levels[idx].cellXEnd-levels[idx].cellXStart+1;
      levels[idx].cellYCount=levels[idx].cellYEnd-levels[idx].cellYStart+1;

      std::cout << idx+waterIndexMinMag << " ";
      std::cout << levels[idx].offset << " ";
      std::cout << levels[idx].cellWidth << "x" << levels[idx].cellHeight << " ";
      std::cout << "[" << levels[idx].cellXStart << "-" << levels[idx].cellXEnd << "]" << "x";
      std::cout << "[" << levels[idx].cellYStart << "-" << levels[idx].cellYEnd << "]" << std::endl;
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
    size_t idx=MagToLevel(magnification);

    if (levels.empty()) {
      return true;
    }

    idx+=4;

    idx=std::max(waterIndexMinMag,idx);
    idx=std::min(waterIndexMaxMag,idx);

    idx-=waterIndexMinMag;

    std::cout << "Level: " << idx+waterIndexMinMag << std::endl;

    tiles.clear();

    if (!scanner.IsOpen()) {
      if (!scanner.Open(datafilename)) {
        std::cerr << "Error while opening " << datafilename << " for reading!" << std::endl;
        return false;
      }
    }

    cx1=(uint32_t)floor((minlon+180.0)/levels[idx].cellWidth);
    cx2=(uint32_t)floor((maxlon+180.0)/levels[idx].cellWidth);
    cy1=(uint32_t)floor((minlat+90.0)/levels[idx].cellHeight);
    cy2=(uint32_t)floor((maxlat+90.0)/levels[idx].cellHeight);

    GroundTile tile;

    tile.points.reserve(5);

    for (size_t y=cy1; y<=cy2; y++) {
      for (size_t x=cx1; x<=cx2; x++) {
        tile.points.clear();

        tile.points.push_back(Point(0,
                                    y*levels[idx].cellHeight-90.0,
                                    x*levels[idx].cellWidth-180.0));

        tile.points.push_back(Point(1,
                                    y*levels[idx].cellHeight-90.0,
                                    (x+1)*levels[idx].cellWidth-180.0));

        tile.points.push_back(Point(2,
                                    (y+1)*levels[idx].cellHeight-90.0,
                                    (x+1)*levels[idx].cellWidth-180.0));

        tile.points.push_back(Point(3,
                                    (y+1)*levels[idx].cellHeight-90.0,
                                    x*levels[idx].cellWidth-180.0));

        tile.points.push_back(tile.points.front());

        if (x<levels[idx].cellXStart ||
            x>levels[idx].cellXEnd ||
            y<levels[idx].cellYStart ||
            y>levels[idx].cellYEnd) {
          tile.type=GroundTile::unknown;
          tile.x=x-levels[idx].cellXStart;
          tile.y=y-levels[idx].cellYStart;

          tiles.push_back(tile);
        }
        else {
          uint32_t   cellId=(y-levels[idx].cellYStart)*levels[idx].cellXCount+x-levels[idx].cellXStart;
          uint32_t   index=cellId*8;
          FileOffset cell;

          scanner.SetPos(levels[idx].offset+index);

          if (!scanner.Read(cell)) {
            std::cerr << "Error while reading from file '" << datafilename << "'" << std::endl;
            return false;
          }

          if (cell==(FileOffset)GroundTile::land ||
              cell==(FileOffset)GroundTile::water ||
              cell==(FileOffset)GroundTile::coast ||
              cell==(FileOffset)GroundTile::unknown) {
            tile.type=(GroundTile::Type)cell;
            tile.x=x-levels[idx].cellXStart;
            tile.y=y-levels[idx].cellYStart;

            tiles.push_back(tile);
          }
          else {
            size_t tileCount;

            tile.type=GroundTile::coast;
            tile.x=x-levels[idx].cellXStart;
            tile.y=y-levels[idx].cellYStart;

            tiles.push_back(tile);

            scanner.SetPos(cell);
            scanner.ReadNumber(tileCount);

            for (size_t t=1; t<=tileCount; t++) {
              uint8_t    tileType;
              size_t     nodeCount;

              scanner.Read(tileType);

              tile.type=(GroundTile::Type)tileType;
              tile.x=x-levels[idx].cellXStart;
              tile.y=y-levels[idx].cellYStart;

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

