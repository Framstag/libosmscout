#ifndef OSMSCOUT_IMPORT_PREPROCESS_PBF_H
#define OSMSCOUT_IMPORT_PREPROCESS_PBF_H

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

#include <string>
#include <unordered_map>
#include <vector>

#include <osmscout/Types.h>

#include <osmscout/import/RawRelation.h>

#include <osmscout/import/Preprocessor.h>

#include <osmscout/import/pbf/fileformat.pb.h>
#include <osmscout/import/pbf/osmformat.pb.h>

namespace osmscout {

  class PreprocessPBF : public Preprocessor
  {
  private:
    char                             *buffer;
    google::protobuf::int32          bufferSize;
    PreprocessorCallback&            callback;
    TagMap                           tagMap;
    std::vector<OSMId>               nodes;
    std::vector<RawRelation::Member> members;

  private:
    bool GetPos(FILE* file,
                FileOffset& pos) const;

    void AssureBlockSize(google::protobuf::int32 length);

    bool ReadBlockHeader(Progress& progress,
                         FILE* file,
                         PBF::BlockHeader& blockHeader,
                         bool silent);

    bool ReadHeaderBlock(Progress& progress,
                         FILE* file,
                         const PBF::BlockHeader& blockHeader,
                         PBF::HeaderBlock& headerBlock);

    bool ReadPrimitiveBlock(Progress& progress,
                            FILE* file,
                            const PBF::BlockHeader& blockHeader,
                            PBF::PrimitiveBlock& primitiveBlock);

    void ReadNodes(const TypeConfig& typeConfig,
                   const PBF::PrimitiveBlock& block,
                   const PBF::PrimitiveGroup &group);

    void ReadDenseNodes(const TypeConfig& typeConfig,
                        const PBF::PrimitiveBlock& block,
                        const PBF::PrimitiveGroup &group);

    void ReadWays(const TypeConfig& typeConfig,
                  const PBF::PrimitiveBlock& block,
                  const PBF::PrimitiveGroup &group);

    void ReadRelations(const TypeConfig& typeConfig,
                       const PBF::PrimitiveBlock& block,
                       const PBF::PrimitiveGroup &group);

  public:
    PreprocessPBF(PreprocessorCallback& callback);
    ~PreprocessPBF();
    bool Import(const TypeConfigRef& typeConfig,
                const ImportParameter& parameter,
                Progress& progress,
                const std::string& filename);
  };
}

#endif
