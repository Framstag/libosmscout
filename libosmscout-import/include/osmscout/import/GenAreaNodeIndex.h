#ifndef OSMSCOUT_IMPORT_GENAREANODEINDEX_H
#define OSMSCOUT_IMPORT_GENAREANODEINDEX_H

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
#include <set>
#include <vector>

#include <osmscout/import/Import.h>

#include <osmscout/system/Compiler.h>

#include <osmscout/util/TileId.h>

namespace osmscout {

  /**
   * Generates an index for querying nodes in a given area and with given node types
   */
  class AreaNodeIndexGenerator CLASS_FINAL : public ImportModule
  {
  public:
    /**
     * The different supported index types for this index
     */
    enum class IndexType : uint8_t
    {
      IndexTypeBitmap  = uint8_t(1),
      IndexTypeList    = uint8_t(2)
    };

    struct DistributionData
    {
      TypeId                  nodeId;        //<! node id of the type
      TypeInfoRef             type;          //<! The node type itself
      bool                    isComplex;     //<! Index is complex
      GeoBox                  boundingBox;   //<! Bounding box of the data
      size_t                  fillCount;     //<! Number of entries of this type over all
      std::map<TileId,size_t> tileFillCount; //<! Number of entries of this type per tile
      std::set<TileId>        listTiles;     //<! Tiles with list index
      std::set<TileId>        bitmapTiles;   //<! Tiles with bitmap index

      inline bool HasNoData() const
      {
        return fillCount==0;
      }

      inline bool IsComplexIndex() const
      {
        return isComplex;
      }
    };

  private:
    bool AnalyseDistribution(const TypeConfigRef& typeConfig,
                             const ImportParameter& parameter,
                             Progress& progress,
                             std::vector<DistributionData>& data);
    void DumpDistribution(Progress& progress,
                          const std::vector<DistributionData>& data);

    std::vector<FileOffset> WriteListIndex(Progress& progress,
                                           const std::vector<DistributionData>& data,
                                           FileWriter& writer);

    std::vector<std::map<TileId,FileOffset>> WriteTileListIndex(Progress& progress,
                                                                const std::vector<DistributionData>& data,
                                                                FileWriter& writer);

    std::vector<std::map<TileId,FileOffset>> WriteBitmapIndex(Progress& progress,
                                                              const std::vector<DistributionData>& data,
                                                              FileWriter& writer);

    void WriteListData(Progress& progress,
                       const std::vector<DistributionData>& data,
                       const std::vector<std::list<std::pair<GeoCoord,FileOffset>>>& listData,
                       const std::vector<FileOffset>& listIndexOffsets,
                       FileWriter& writer);

    void WriteTileListData(const ImportParameter& parameter,
                           const DistributionData& distributionData,
                           const std::list<std::pair<GeoCoord,FileOffset>>& tileData,
                           const FileOffset& tileIndexOffset,
                           FileWriter& writer);

    void WriteBitmapData(const ImportParameter& parameter,
                         const TileId& tileId,
                         const std::list<std::pair<GeoCoord,FileOffset>>& bitmapData,
                         const FileOffset& bitmapIndexOffset,
                         FileWriter& writer);

    bool WriteData(const TypeConfigRef& typeConfig,
                   const ImportParameter& parameter,
                   Progress& progress,
                   const std::vector<DistributionData>& data);

  public:
    void GetDescription(const ImportParameter& parameter,
                        ImportModuleDescription& description) const override;

    bool Import(const TypeConfigRef& typeConfig,
                const ImportParameter& parameter,
                Progress& progress) override;
  };
}

#endif
