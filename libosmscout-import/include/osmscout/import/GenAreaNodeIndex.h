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

#include <osmscout/import/Import.h>

#include <osmscout/system/Compiler.h>

#include <osmscout/util/TileId.h>

namespace osmscout {

  /**
   * Generates an index for querying nodes in a given area and with given node types
   */
  class AreaNodeIndexGenerator CLASS_FINAL : public ImportModule
  {
  private:
    /**
     * The different supported index types for this index
     */
    enum class IndexType : uint8_t
    {
      IndexTypeBitmap = uint8_t(0),
      IndexTypeList   = uint8_t(1)
    };

    /**
     * Helper struct to hold information type in the index
     */
    struct TypeData
    {
      TypeInfoRef        type;        //<! Node type
      IndexType          indexType;   //<! Type of the index
      MagnificationLevel level;       //<! magnification level of index
      TileIdBox          tileBox;     //<! Tile box
      FileOffset         indexOffset; //<! Position in file where the offset of the bitmap is written

      size_t             nodeCount;   //<! Number of entries over all cells
      size_t             cellCount;   //<! Number of filled cells in index

      TypeData();

      inline bool HasEntries() const
      {
        return cellCount>0 &&
               nodeCount>0;
      }
    };

  private:
    bool ScanningNodeData(const TypeConfigRef& typeConfig,
                          const ImportParameter& parameter,
                          Progress& progress,
                          std::vector<TypeData>& nodeTypeData);
    void DumpNodeData(Progress& progress,
                      const std::vector<TypeData>& nodeTypeData);
    bool WriteBitmap(Progress& progress,
                     FileWriter& writer,
                     const TypeInfo& type,
                     const TypeData& typeData,
                     const std::map<TileId,std::list<FileOffset>>& bitmapData);
    bool WriteList(Progress& progress,
                   FileWriter& writer,
                   const TypeInfo& type,
                   const TypeData& typeData,
                   const std::list<std::pair<GeoCoord,FileOffset>>& listData);
    bool WriteIndexFile(const TypeConfigRef& typeConfig,
                        const ImportParameter& parameter,
                        Progress& progress,
                        std::vector<TypeData>& nodeTypeData);

  public:
    void GetDescription(const ImportParameter& parameter,
                        ImportModuleDescription& description) const override;

    bool Import(const TypeConfigRef& typeConfig,
                const ImportParameter& parameter,
                Progress& progress) override;
  };
}

#endif
