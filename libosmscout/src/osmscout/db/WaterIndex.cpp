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

#include <osmscout/db/WaterIndex.h>

#include <osmscout/system/Math.h>

#include <osmscout/util/Logger.h>

#include <osmscout/io/File.h>
#include <osmscout/io/FileScanner.h>

namespace osmscout {

  const char* const WaterIndex::WATER_IDX="water.idx";

  WaterIndex::~WaterIndex()
  {
    Close();
  }

  bool WaterIndex::Open(const std::string& path, bool memoryMappedData)
  {
    datafilename=AppendFileToDir(path,WATER_IDX);

    try {
      scanner.Open(datafilename,FileScanner::FastRandom,memoryMappedData);

      waterIndexMinMag=scanner.ReadUInt32Number();
      waterIndexMaxMag=scanner.ReadUInt32Number();

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
        size_t  idx=level-waterIndexMinMag;

        levels[idx].hasCellData=scanner.ReadBool();
        levels[idx].dataOffsetBytes=scanner.ReadUInt8();

        uint8_t state=scanner.ReadUInt8();

        levels[idx].defaultCellData=(GroundTile::Type)state;

        levels[idx].indexDataOffset=scanner.ReadFileOffset();

        levels[idx].cellXStart=scanner.ReadUInt32Number();
        levels[idx].cellXEnd=scanner.ReadUInt32Number();
        levels[idx].cellYStart=scanner.ReadUInt32Number();
        levels[idx].cellYEnd=scanner.ReadUInt32Number();

        levels[idx].cellXCount=levels[idx].cellXEnd-levels[idx].cellXStart+1;
        levels[idx].cellYCount=levels[idx].cellYEnd-levels[idx].cellYStart+1;

        levels[idx].dataOffset=levels[idx].indexDataOffset+levels[idx].cellXCount*levels[idx].cellYCount*levels[idx].dataOffsetBytes;
      }

      if (scanner.HasError()) {
        log.Error() << "Error while reading from file '" << scanner.GetFilename() << "'";
        return false;
      }

      return true;
    }
    catch (const IOException& e) {
      log.Error() << e.GetDescription();
      return false;
    }
  }

  void WaterIndex::Close()
  {
    levels.clear();
    try  {
      if (scanner.IsOpen()) {
        scanner.Close();
      }
    }
    catch (const IOException& e) {
      log.Error() << e.GetDescription();
      scanner.CloseFailsafe();
    }
  }

  void WaterIndex::GetGroundTileByDefault(const Level& level,
                                          uint32_t cx1,
                                          uint32_t cx2,
                                          uint32_t cy1,
                                          uint32_t cy2,
                                          std::list<GroundTile>& tiles) const
  {
    GroundTile tile;

    tile.cellWidth=level.cellWidth;
    tile.cellHeight=level.cellHeight;

    for (uint32_t y=cy1; y<=cy2; y++) {
      for (uint32_t x=cx1; x<=cx2; x++) {
        tile.xAbs=x;
        tile.yAbs=y;

        if (x>=level.cellXStart &&
            y>=level.cellYStart) {
          tile.xRel=x-level.cellXStart;
          tile.yRel=y-level.cellYStart;
        }
        else {
          tile.xRel=0;
          tile.yRel=0;
        }

        if (x<level.cellXStart ||
            x>level.cellXEnd ||
            y<level.cellYStart ||
            y>level.cellYEnd) {
          tile.type=GroundTile::unknown;
        }
        else {
          tile.type=level.defaultCellData;
        }

        tiles.push_back(tile);
      }
    }
  }

  void WaterIndex::GetGroundTileFromData(const Level& level,
                                          uint32_t cx1,
                                          uint32_t cx2,
                                          uint32_t cy1,
                                          uint32_t cy2,
                                          std::list<GroundTile>& tiles) const
  {
    GroundTile tile;

    tile.coords.reserve(5);
    tile.cellWidth=level.cellWidth;
    tile.cellHeight=level.cellHeight;

    for (uint32_t y=cy1; y<=cy2; y++) {
      for (uint32_t x=cx1; x<=cx2; x++) {
        tile.xAbs=x;
        tile.yAbs=y;

        if (x>=level.cellXStart &&
            y>=level.cellYStart) {
          tile.xRel=x-level.cellXStart;
          tile.yRel=y-level.cellYStart;
        }
        else {
          tile.xRel=0;
          tile.yRel=0;
        }

        if (x<level.cellXStart ||
            x>level.cellXEnd ||
            y<level.cellYStart ||
            y>level.cellYEnd) {
          tile.type=GroundTile::unknown;
          tile.coords.clear();

          tiles.push_back(tile);
        }
        else {
          std::scoped_lock<std::mutex> guard(lookupMutex);
          uint32_t                    cellId=(y-level.cellYStart)*level.cellXCount+x-level.cellXStart;
          uint32_t                    index=cellId*level.dataOffsetBytes;
          FileOffset                  cell;

          scanner.SetPos(level.indexDataOffset+index);

          cell=scanner.ReadFileOffset(level.dataOffsetBytes);

          if (cell==(FileOffset)GroundTile::land ||
              cell==(FileOffset)GroundTile::water ||
              cell==(FileOffset)GroundTile::coast ||
              cell==(FileOffset)GroundTile::unknown) {
            tile.type=(GroundTile::Type)cell;
            tile.coords.clear();

            tiles.push_back(tile);
          }
          else {
            tile.type=GroundTile::coast;
            tile.coords.clear();

            tiles.push_back(tile);

            scanner.SetPos(level.dataOffset+cell);
            uint32_t tileCount=scanner.ReadUInt32Number();

            for (size_t t=0; t<tileCount; t++) {
              uint8_t  tileType=scanner.ReadUInt8();

              tile.type=(GroundTile::Type)tileType;

              uint32_t coordCount=scanner.ReadUInt32Number();

              tile.coords.resize(coordCount);

              for (size_t n=0; n<coordCount; n++) {
                uint16_t x=scanner.ReadUInt16();
                uint16_t y=scanner.ReadUInt16();

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

  bool WaterIndex::GetRegions(const GeoBox& boundingBox,
                              const Magnification& magnification,
                              std::list<GroundTile>& tiles) const
  {
    try {
      uint32_t cx1,cx2,cy1,cy2;
      uint32_t idx=magnification.GetLevel();

      tiles.clear();

      if (levels.empty()) {
        return true;
      }

      idx+=4;

      idx=std::max(waterIndexMinMag,idx);
      idx=std::min(waterIndexMaxMag,idx);

      idx-=waterIndexMinMag;

      cx1=(uint32_t)floor((boundingBox.GetMinLon()+double(GeoCoord::MaxLongitude))/levels[idx].cellWidth);
      cx2=(uint32_t)floor((boundingBox.GetMaxLon()+double(GeoCoord::MaxLongitude))/levels[idx].cellWidth);
      cy1=(uint32_t)floor((boundingBox.GetMinLat()+double(GeoCoord::MaxLatitude))/levels[idx].cellHeight);
      cy2=(uint32_t)floor((boundingBox.GetMaxLat()+double(GeoCoord::MaxLatitude))/levels[idx].cellHeight);

      const Level &level=levels[idx];

      if (level.hasCellData) {
        GetGroundTileFromData(level,
                              cx1,
                              cx2,
                              cy1,
                              cy2,
                              tiles);
      }
      else {
        GetGroundTileByDefault(level,
                               cx1,
                               cx2,
                               cy1,
                               cy2,
                               tiles);
      }
    }
    catch (const IOException& e) {
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
