#ifndef OSMSCOUT_IMPORT_PREPROCESSOR_H
#define OSMSCOUT_IMPORT_PREPROCESSOR_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2015  Tim Teulings

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

#include <unordered_map>

#include <osmscout/Tag.h>
#include <osmscout/TypeConfig.h>
#include <osmscout/OSMScoutTypes.h>
#include <osmscout/util/Progress.h>

#include <osmscoutimport/Import.h>
#include <osmscoutimport/RawRelation.h>

namespace osmscout {

  class OSMSCOUT_IMPORT_API PreprocessorCallback
  {
  public:
    struct RawNodeData
    {
      OSMId    id;
      GeoCoord coord;
      TagMap   tags;

      RawNodeData() = default;

      RawNodeData(OSMId id,
                  const GeoCoord& coord)
      : id(id),
        coord(coord)
      {
        // no code
      }
    };

    struct RawWayData
    {
      OSMId              id;
      TagMap             tags;
      std::vector<OSMId> nodes;
    };

    struct RawRelationData
    {
      OSMId                            id;
      TagMap                           tags;
      std::vector<RawRelation::Member> members;
    };

    struct RawBlockData
    {
      std::vector<RawNodeData>     nodeData;
      std::vector<RawWayData>      wayData;
      std::vector<RawRelationData> relationData;
    };

    // Should be unique_ptr but I get compiler errors if passing it to the WriteWorkerQueue
    using RawBlockDataRef = std::shared_ptr<RawBlockData>;

  public:
    virtual ~PreprocessorCallback() = default;

    virtual void ProcessBlock(RawBlockDataRef data) = 0;
  };

  class OSMSCOUT_IMPORT_API Preprocessor
  {
  public:
    virtual ~Preprocessor() = default;

    virtual bool Import(const TypeConfigRef& typeConfig,
                        const ImportParameter& parameter,
                        Progress& progress,
                        const std::string& filename) = 0;

  };
}

#endif
