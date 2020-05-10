#ifndef OSMSCOUT_AREANODEINDEX_H
#define OSMSCOUT_AREANODEINDEX_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2011  Tim Teulings

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

#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include <osmscout/TypeConfig.h>
#include <osmscout/TypeInfoSet.h>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/TileId.h>

namespace osmscout {

  /**
    \ingroup Database

    AreaWayIndex allows you to find ways and way relations in
    a given area.

    Ways can be limited by type and result count.
    */
  class OSMSCOUT_API AreaNodeIndex
  {
  public:
    static const char* const AREA_NODE_IDX;

  private:
    struct ListTile
    {
      FileOffset         fileOffset;
      uint16_t           entryCount;
      bool               storeGeoCoord;
    };

    struct BitmapTile
    {
      FileOffset         fileOffset;
      uint8_t            dataOffsetBytes;
      MagnificationLevel magnification;
    };

    struct TypeData
    {
      bool                        isComplex;
      GeoBox                      boundingBox;
      FileOffset                  indexOffset=0;
      uint16_t                    entryCount=0;

      std::map<TileId,ListTile>   listTiles;
      std::map<TileId,BitmapTile> bitmapTiles;
    };

  private:
    mutable FileScanner   scanner;        //!< Scanner instance for reading this file,
                                          //!< guarded by lookupMutex (Open and Close method are not guarded!)

    MagnificationLevel    gridMag;
    std::vector<TypeData> nodeTypeData;

    mutable std::mutex    lookupMutex;

  private:
    bool GetOffsetsList(const TypeData& typeData,
                        const GeoBox& boundingBox,
                        std::vector<FileOffset>& offsets) const;

    bool GetOffsetsTileList(const TypeData& typeData,
                            const GeoBox& boundingBox,
                            std::vector<FileOffset>& offsets) const;

    bool GetOffsetsBitmap(const TypeData& typeData,
                          const GeoBox& boundingBox,
                          std::vector<FileOffset>& offsets) const;

  public:
    AreaNodeIndex() = default;

    void Close();
    bool Open(const std::string& path,
              bool memoryMappedData);

    inline bool IsOpen() const
    {
      return scanner.IsOpen();
    }

    inline std::string GetFilename() const
    {
      return scanner.GetFilename();
    }

    bool GetOffsets(const GeoBox& boundingBox,
                    const TypeInfoSet& requestedTypes,
                    std::vector<FileOffset>& offsets,
                    TypeInfoSet& loadedTypes) const;
  };

  using AreaNodeIndexRef = std::shared_ptr<AreaNodeIndex>;
}

#endif
