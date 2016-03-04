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

#include <algorithm>

#include <osmscout/system/Math.h>

#include <osmscout/util/File.h>
#include <osmscout/util/FileScanner.h>
#include <osmscout/util/Logger.h>

namespace osmscout {

  const char* WaterIndex::WATER_IDX="water.idx";

  WaterIndex::WaterIndex()
  {
    // no code
  }

  WaterIndex::~WaterIndex()
  {
    Close();
  }

  bool WaterIndex::Open(const std::string& path)
  {
    datafilename=AppendFileToDir(path,WATER_IDX);

    try {
      scanner.Open(datafilename,FileScanner::FastRandom,true);

      scanner.ReadNumber(waterIndexMinMag);
      scanner.ReadNumber(waterIndexMaxMag);

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
      }

      if (scanner.HasError()) {
        log.Error() << "Error while reading from file '" << scanner.GetFilename() << "'";
        return false;
      }

      return true;
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      return false;
    }
  }

  void WaterIndex::Close()
  {
    try  {
      if (scanner.IsOpen()) {
        scanner.Close();
      }
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      scanner.CloseFailsafe();
    }
  }

  bool WaterIndex::GetRegions(const GeoBox& boundingBox,
                              const Magnification& magnification,
                              std::list<GroundTile>& tiles) const
  {
    try {
      uint32_t cx1,cx2,cy1,cy2;
      uint32_t idx=magnification.GetLevel();

      if (levels.empty()) {
        return true;
      }

      idx+=4;

      idx=std::max(waterIndexMinMag,idx);
      idx=std::min(waterIndexMaxMag,idx);

      idx-=waterIndexMinMag;

      tiles.clear();

      cx1=(uint32_t)floor((boundingBox.GetMinLon()+180.0)/levels[idx].cellWidth);
      cx2=(uint32_t)floor((boundingBox.GetMaxLon()+180.0)/levels[idx].cellWidth);
      cy1=(uint32_t)floor((boundingBox.GetMinLat()+90.0)/levels[idx].cellHeight);
      cy2=(uint32_t)floor((boundingBox.GetMaxLat()+90.0)/levels[idx].cellHeight);

      GroundTile tile;

      tile.coords.reserve(5);
      tile.cellWidth=levels[idx].cellWidth;
      tile.cellHeight=levels[idx].cellHeight;

      for (uint32_t y=cy1; y<=cy2; y++) {
        for (uint32_t x=cx1; x<=cx2; x++) {
          tile.xAbs=x;
          tile.yAbs=y;

          if (x>=levels[idx].cellXStart &&
              y>=levels[idx].cellYStart) {
            tile.xRel=x-levels[idx].cellXStart;
            tile.yRel=y-levels[idx].cellYStart;
          }
          else {
            tile.xRel=0;
            tile.yRel=0;
          }

          if (x<levels[idx].cellXStart ||
              x>levels[idx].cellXEnd ||
              y<levels[idx].cellYStart ||
              y>levels[idx].cellYEnd) {
            tile.type=GroundTile::unknown;
            tile.coords.clear();

            tiles.push_back(tile);
          }
          else {
            std::lock_guard<std::mutex> guard(lookupMutex);
            uint32_t                    cellId=(y-levels[idx].cellYStart)*levels[idx].cellXCount+x-levels[idx].cellXStart;
            uint32_t                    index=cellId*8;
            FileOffset                  cell;

            scanner.SetPos(levels[idx].offset+index);

            scanner.Read(cell);

            if (cell==(FileOffset)GroundTile::land ||
                cell==(FileOffset)GroundTile::water ||
                cell==(FileOffset)GroundTile::coast ||
                cell==(FileOffset)GroundTile::unknown) {
              tile.type=(GroundTile::Type)cell;
              tile.coords.clear();

              tiles.push_back(tile);
            }
            else {
              uint32_t tileCount;

              tile.type=GroundTile::coast;
              tile.coords.clear();

              tiles.push_back(tile);

              scanner.SetPos(cell);
              scanner.ReadNumber(tileCount);

              for (size_t t=1; t<=tileCount; t++) {
                uint8_t    tileType;
                uint32_t   coordCount;

                scanner.Read(tileType);

                tile.type=(GroundTile::Type)tileType;

                scanner.ReadNumber(coordCount);

                tile.coords.resize(coordCount);

                for (size_t n=0; n<coordCount; n++) {
                  uint16_t x;
                  uint16_t y;

                  scanner.Read(x);
                  scanner.Read(y);

                  tile.coords[n].Set(x & ~(1 << 15),
                                     y,
                                     (x & (1 << 15))!=0);
                }

                tiles.push_back(tile);
              }
            }
          }
        }
      }
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      return false;
    }

    return true;
  }

  void WaterIndex::DumpStatistics()
  {
    size_t entries=0;
    size_t memory=0;

    log.Info() << "WaterIndex size " << entries << ", memory " << memory;
  }
}
