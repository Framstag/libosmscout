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

#include <osmscout/import/Import.h>

#include <osmscout/system/Compiler.h>

#include <osmscout/util/TileId.h>

namespace osmscout {

  class AreaNodeIndexGenerator CLASS_FINAL : public ImportModule
  {
  private:
    struct TypeData
    {
      TypeInfoRef        type;
      MagnificationLevel level;       //! magnification level of index
      size_t             cellCount;   //! Number of filled cells in index
      size_t             nodeCount;   //! Number of entries over all cells
      TileIdBox          tileBox;     //! Tile box
      FileOffset         indexOffset; //! Position in file where the offset of the bitmap is written

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
