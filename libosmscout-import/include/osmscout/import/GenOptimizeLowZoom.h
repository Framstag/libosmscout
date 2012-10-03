#ifndef OSMSCOUT_IMPORT_GENOPTIMIZELOWZOOM_H
#define OSMSCOUT_IMPORT_GENOPTIMIZELOWZOOM_H

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

#include <osmscout/ImportFeatures.h>

#include <map>

#include <osmscout/import/Import.h>

#include <osmscout/Way.h>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>
#include <osmscout/util/HashMap.h>

namespace osmscout {

  class OptimizeLowZoomGenerator : public ImportModule
  {
  private:
    typedef OSMSCOUT_HASHMAP<Id,FileOffset> IdFileOffsetMap;

    struct TypeData
    {
      uint32_t   indexLevel;   //! magnification level of index
      size_t     indexCells;   //! Number of filled cells in index
      size_t     indexEntries; //! Number of entries over all cells

      uint32_t   cellXStart;
      uint32_t   cellXEnd;
      uint32_t   cellYStart;
      uint32_t   cellYEnd;
      uint32_t   cellXCount;
      uint32_t   cellYCount;

      FileOffset bitmapOffset; //! Position in file where the offset of the bitmap is written

      TypeData();

      inline bool HasEntries()
      {
        return indexCells>0 &&
               indexEntries>0;
      }
    };

  private:
    void GetTypesToOptimize(const TypeConfig& typeConfig,
                            std::set<TypeId>& types);

    void WriteHeader(FileWriter& writer,
                     const std::set<TypeId>& types,
                     const std::vector<TypeData>& typesData,
                     size_t optimizeMaxMap,
                     std::map<TypeId,FileOffset>& typeOffsetMap);

    bool GetWaysToOptimize(const ImportParameter& parameter,
                           Progress& progress,
                           FileScanner& scanner,
                           std::set<TypeId>& types,
                           std::vector<std::list<WayRef> >& ways);

    void MergeWays(const std::list<WayRef>& ways,
                   std::list<WayRef>& newWays);

    void GetIndexLevel(const ImportParameter& parameter,
                       Progress& progress,
                       const std::list<WayRef>& newWays,
                       TypeData& typeData);

    bool WriteOptimizedWays(Progress& progress,
                            FileWriter& writer,
                            const std::list<WayRef>& ways,
                            IdFileOffsetMap& offsets,
                            size_t width,
                            size_t height,
                            double magnification,
                            TransPolygon::OptimizeMethod optimizeWayMethod);

    bool WriteBitmap(Progress& progress,
                     FileWriter& writer,
                     const TypeInfo& type,
                     const std::list<WayRef>& ways,
                     const IdFileOffsetMap& offsets,
                     TypeData& data);

  public:
    std::string GetDescription() const;
    bool Import(const ImportParameter& parameter,
                Progress& progress,
                const TypeConfig& typeConfig);
  };
}

#endif
