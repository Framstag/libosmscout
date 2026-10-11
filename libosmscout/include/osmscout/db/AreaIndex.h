#ifndef LIBOSMSCOUT_AREAYINDEX_H
#define LIBOSMSCOUT_AREAYINDEX_H
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

#include <atomic>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <unordered_set>
#include <vector>

#include <osmscout/TypeConfig.h>
#include <osmscout/TypeInfoSet.h>

#include <osmscout/io/FileScanner.h>

#include <osmscout/util/TileId.h>

namespace osmscout {

  /**
    \ingroup Database
    Generic area index for lookup objects by area
    */
  class OSMSCOUT_API AreaIndex
  {
  protected:
    struct TypeData
    {
      TypeInfoRef         type;
      MagnificationLevel  indexLevel;

      uint8_t             dataOffsetBytes;
      FileOffset          bitmapOffset;

      TileIdBox           tileBox;

      GeoBox              boundingBox;

      TypeData();

      FileOffset GetDataOffset() const;
      FileOffset GetCellOffset(size_t x, size_t y) const;
    };

  private:
    std::string           indexFileName;
    std::string           fullIndexFileName;  //!< Full path and name of the data file

    std::vector<TypeData> typeData;

    /**
     * Position of the entry of a type within `typeData`, addressed by the type's index in
     * the type configuration (`TypeInfo::GetIndex()`), or `kNoEntry` when this index
     * carries no entry for that type. Built by `Open()`, so a lookup does not have to walk
     * the entries to find the ones a request names.
     */
    std::vector<uint32_t>     entryOfType;

    static constexpr uint32_t kNoEntry=std::numeric_limits<uint32_t>::max();

    /**
     * Entries the last lookup examined - that is, the entries of requested types this index
     * carries. A diagnostic for tests; concurrent lookups make it the count of whichever
     * lookup finished last.
     */
    mutable std::atomic<size_t> examinedEntryCount{0};

    mutable std::mutex          lookupMutex;

  protected:
    mutable FileScanner   scanner;            //!< Scanner instance for reading this file

  protected:
    void GetOffsets(const TypeData& typeData,
                    const GeoBox& boundingBox,
                    std::unordered_set<FileOffset>& offsets) const;

    explicit AreaIndex(const std::string &indexFileName);

    virtual void ReadTypeData(const TypeConfigRef& typeConfig,
                              TypeData &data) = 0;

  public:
    AreaIndex() = default;

    // disable copy and move
    AreaIndex(const AreaIndex&) = delete;
    AreaIndex(AreaIndex&&) = delete;
    AreaIndex& operator=(const AreaIndex&) = delete;
    AreaIndex& operator=(AreaIndex&&) = delete;

    virtual ~AreaIndex();

    void Close();
    bool Open(const TypeConfigRef& typeConfig,
              const std::string& path,
              bool memoryMappedData);

    bool IsOpen() const
    {
      return scanner.IsOpen();
    }

    std::string GetFilename() const
    {
      return fullIndexFileName;
    }

    bool GetOffsets(const GeoBox& boundingBox,
                    const TypeInfoSet& types,
                    std::vector<FileOffset>& offsets,
                    TypeInfoSet& loadedTypes) const;

    /**
     * The number of entries this index carries; the index file holds one entry per type it
     * indexed.
     */
    size_t GetEntryCount() const
    {
      return typeData.size();
    }

    /**
     * The number of entries the last lookup examined (diagnostic for tests).
     */
    size_t GetExaminedEntryCount() const
    {
      return examinedEntryCount.load(std::memory_order_relaxed);
    }
  };
}

#endif //LIBOSMSCOUT_AREAYINDEX_H
