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

#include <osmscout/Area.h>
#include <osmscout/Way.h>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>
#include <osmscout/util/HashMap.h>

namespace osmscout {

  class OptimizeLowZoomGenerator : public ImportModule
  {
  private:
    typedef OSMSCOUT_HASHMAP<FileOffset,FileOffset> FileOffsetFileOffsetMap;

    struct TypeData
    {
      TypeId     type;            //! The type
      uint32_t   optLevel;        //! The display level this data was optimized for
      uint32_t   indexLevel;      //! Magnification level of index

      uint32_t   cellXStart;
      uint32_t   cellXEnd;
      uint32_t   cellYStart;
      uint32_t   cellYEnd;

      FileOffset bitmapOffset;    //! Position in file where the offset of the bitmap is written
      uint8_t    dataOffsetBytes; //! Number of bytes per entry in bitmap

      uint32_t   cellXCount;
      uint32_t   cellYCount;

      size_t     indexCells;      //! Number of filled cells in index
      size_t     indexEntries;    //! Number of entries over all cells

      TypeData();

      inline bool HasEntries()
      {
        return indexCells>0 &&
               indexEntries>0;
      }
    };

  private:
    void GetWayTypesToOptimize(const TypeConfig& typeConfig,
                               std::set<TypeId>& types);

    void GetAreaTypesToOptimize(const TypeConfig& typeConfig,
                                std::set<TypeId>& types);

    bool WriteTypeData(FileWriter& writer,
                       const TypeData& data);

    bool WriteHeader(FileWriter& writer,
                     const std::list<TypeData>& areaTypesData,
                     const std::set<TypeId>& wayTypes,
                     const std::vector<TypeData>& wayTypesData,
                     uint32_t optimizeMaxMap);

    bool GetAreas(const ImportParameter& parameter,
                            Progress& progress,
                            FileScanner& scanner,
                            std::set<TypeId>& types,
                            std::vector<std::list<AreaRef> >& areas);

    void GetAreaIndexLevel(const ImportParameter& parameter,
                           Progress& progress,
                           const std::list<AreaRef>& areas,
                           TypeData& typeData);

    bool WriteAreas(Progress& progress,
                    FileWriter& writer,
                    const std::list<AreaRef>& areas,
                    FileOffsetFileOffsetMap& offsets);

    bool WriteAreaBitmap(Progress& progress,
                         FileWriter& writer,
                         const TypeInfo& type,
                         const std::list<AreaRef>& areas,
                         const FileOffsetFileOffsetMap& offsets,
                         TypeData& data);

    bool GetWays(const ImportParameter& parameter,
                           Progress& progress,
                           FileScanner& scanner,
                           std::set<TypeId>& types,
                           std::vector<std::list<WayRef> >& ways);

    void MergeWays(Progress& progress,
                   const std::list<WayRef>& ways,
                   std::list<WayRef>& newWays);

    void GetWayIndexLevel(const ImportParameter& parameter,
                          Progress& progress,
                          const std::list<WayRef>& ways,
                          TypeData& typeData);

    void OptimizeAreas(Progress& progress,
                       const TypeConfig& typeConfig,
                       TypeId type,
                       const std::list<AreaRef>& areas,
                       std::list<AreaRef>& optimizedAreas,
                       size_t width,
                       size_t height,
                       const Magnification& magnification,
                       TransPolygon::OptimizeMethod optimizeWayMethod);

    void OptimizeWays(Progress& progress,
                      std::list<WayRef>& ways,
                      size_t width,
                      size_t height,
                      const Magnification& magnification,
                      TransPolygon::OptimizeMethod optimizeWayMethod);

    bool WriteWays(Progress& progress,
                   FileWriter& writer,
                   const std::list<WayRef>& ways,
                   FileOffsetFileOffsetMap& offsets);

    bool WriteWayBitmap(Progress& progress,
                        FileWriter& writer,
                        const TypeInfo& type,
                        const std::list<WayRef>& ways,
                        const FileOffsetFileOffsetMap& offsets,
                        TypeData& data);

    bool HandleAreas(const ImportParameter& parameter,
                     Progress& progress,
                     const TypeConfig& typeConfig,
                     FileWriter& writer,
                     const std::set<TypeId>& types,
                     std::list<TypeData>& typesData);

    bool HandleWays(const ImportParameter& parameter,
                    Progress& progress,
                    const TypeConfig& typeConfig,
                    FileWriter& writer,
                    const std::set<TypeId>& types,
                    std::vector<TypeData>& typesData);

  public:
    std::string GetDescription() const;
    bool Import(const ImportParameter& parameter,
                Progress& progress,
                const TypeConfig& typeConfig);
  };
}

#endif
