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
#include <osmscout/Types.h>
#include <osmscout/util/Progress.h>

#include <osmscout/import/Import.h>
#include <osmscout/import/RawRelation.h>

namespace osmscout {

  class OSMSCOUT_IMPORT_API PreprocessorCallback
  {
  public:
    struct RawNodeData
    {
      OSMId    id;
      GeoCoord coord;
      TagMap   tags;
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
    typedef std::shared_ptr<RawBlockData> RawBlockDataRef;

  public:
    virtual ~PreprocessorCallback();

    virtual void ProcessBlock(RawBlockDataRef data) = 0;
  };

  class OSMSCOUT_IMPORT_API Preprocessor
  {
  public:
    virtual ~Preprocessor();
  };
}

#endif
