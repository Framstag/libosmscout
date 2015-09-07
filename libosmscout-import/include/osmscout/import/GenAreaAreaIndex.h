#ifndef OSMSCOUT_IMPORT_GENAREAAREAINDEX_H
#define OSMSCOUT_IMPORT_GENAREAAREAINDEX_H

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

#include <map>
#include <set>

#include <osmscout/Area.h>
#include <osmscout/Pixel.h>

#include <osmscout/util/FileWriter.h>
#include <osmscout/util/Geometry.h>

#include <osmscout/import/Import.h>
#include <osmscout/import/SortDat.h>

namespace osmscout {

  class AreaAreaIndexGenerator : public ImportModule
  {
  private:
    struct Entry
    {
      FileOffset offset;
      TypeId     type;
    };

    struct AreaLeaf
    {
      std::list<Entry> areas;
    };

    typedef std::map<Pixel,AreaLeaf> Level;

  private:

    std::list<SortDataGenerator<Area>::ProcessingFilterRef> filters;

  private:
    size_t CalculateLevel(const ImportParameter& parameter,
                          const GeoBox& boundingBox) const;

    void EnrichLevels(std::vector<Level>& levels);

    bool CopyData(const TypeConfig& typeConfig,
                  Progress& progress,
                  FileScanner& scanner,
                  FileWriter& dataWriter,
                  FileWriter& mapWriter,
                  const std::list<FileOffset>& srcOffsets,
                  FileOffset& dataStartOffset,
                  uint32_t& dataWrittenCount);

    bool WriteCell(const TypeConfig& typeConfig,
                   Progress& progress,
                   const ImportParameter& parameter,
                   FileScanner& scanner,
                   FileWriter& indexWriter,
                   FileWriter& dataWriter,
                   FileWriter& mapWriter,
                   const std::vector<Level>& levels,
                   size_t level,
                   const Pixel& pixel,
                   const AreaLeaf& leaf,
                   FileOffset& offset,
                   uint32_t& dataWrittenCount);

  public:
    AreaAreaIndexGenerator();
    std::string GetDescription() const;
    bool Import(const TypeConfigRef& typeConfig,
                const ImportParameter& parameter,
                Progress& progress);
  };
}

#endif
