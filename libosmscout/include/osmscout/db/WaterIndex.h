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

#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <osmscout/GroundTile.h>
#include <osmscout/OSMScoutTypes.h>

#include <osmscout/util/Magnification.h>

#include <osmscout/io/FileScanner.h>

namespace osmscout {

  /**
   * \ingroup Database
   */
  class OSMSCOUT_API WaterIndex final
  {
  public:
    static const char* const WATER_IDX;

  private:
    struct Level
    {
      // Transient
      double                     cellWidth;       //!< With of an cell
      double                     cellHeight;      //!< Height of an cell
      uint32_t                   cellXCount;      //!< Number of cells in horizontal direction (with of bounding box in cells)
      uint32_t                   cellYCount;      //!< Number of cells in vertical direction (height of bounding box in cells)
      FileOffset                 dataOffset;      //!< FileOffset of the data after the index

      // Persistent

      bool                       hasCellData;      //!< If true, we have cell data
      uint8_t                    dataOffsetBytes;  //!< Number of bytes per entry in bitmap
      GroundTile::Type           defaultCellData;  //!< If hasCellData is false, this is the vaue to be returned for all cells
      FileOffset                 indexDataOffset;  //!< File offset of start cell state data on disk

      uint32_t                   cellXStart;       //!< First x-axis coordinate of cells
      uint32_t                   cellXEnd;         //!< Last x-axis coordinate cells
      uint32_t                   cellYStart;       //!< First y-axis coordinate of cells
      uint32_t                   cellYEnd;         //!< Last x-axis coordinate cells
    };

  private:
    std::string                datafilename;   //!< Full path and name of the data file
    mutable FileScanner        scanner;        //!< Scanner instance for reading this file

    uint32_t                   waterIndexMinMag;
    uint32_t                   waterIndexMaxMag;
    std::vector<Level>         levels;

    mutable std::mutex         lookupMutex;

  private:
    void GetGroundTileByDefault(const Level& level,
                                uint32_t cx1,
                                uint32_t cx2,
                                uint32_t cy1,
                                uint32_t cy2,
                                std::list<GroundTile>& tiles) const;
    void GetGroundTileFromData(const Level& level,
                               uint32_t cx1,
                               uint32_t cx2,
                               uint32_t cy1,
                               uint32_t cy2,
                               std::list<GroundTile>& tiles) const;

  public:
    WaterIndex() = default;
    virtual ~WaterIndex();

    bool Open(const std::string& path, bool memoryMappedData);
    void Close();

    bool GetRegions(const GeoBox& boundingBox,
                    const Magnification& magnification,
                    std::list<GroundTile>& tiles) const;

    void DumpStatistics();
  };

  using WaterIndexRef = std::shared_ptr<WaterIndex>;
}

#endif
