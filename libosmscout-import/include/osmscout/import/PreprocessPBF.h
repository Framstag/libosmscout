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

#include <future>
#include <string>
#include <unordered_map>
#include <vector>

#include <osmscout/Types.h>

#include <osmscout/import/RawRelation.h>

#include <osmscout/import/Preprocessor.h>

#if defined(OSMSCOUT_IMPORT_MESON_BUILD)
  #include <fileformat.pb.h>
  #include <osmformat.pb.h>
#else
  #include <osmscout/import/pbf/fileformat.pb.h>
  #include <osmscout/import/pbf/osmformat.pb.h>
#endif

#include <osmscout/system/Compiler.h>

namespace osmscout {

  class PreprocessPBF CLASS_FINAL : public Preprocessor
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
                         OSMPBF::BlobHeader& blockHeader,
                         bool silent);

    bool ReadHeaderBlock(Progress& progress,
                         FILE* file,
                         const OSMPBF::BlobHeader& blockHeader,
                         OSMPBF::HeaderBlock& headerBlock);

    bool ReadPrimitiveBlock(Progress& progress,
                            FILE* file,
                            const OSMPBF::BlobHeader& blockHeader,
                            OSMPBF::PrimitiveBlock& primitiveBlock);

    void ReadNodes(const TypeConfig& typeConfig,
                   const OSMPBF::PrimitiveBlock& block,
                   const OSMPBF::PrimitiveGroup &group,
                   PreprocessorCallback::RawBlockData& data);

    void ReadDenseNodes(const TypeConfig& typeConfig,
                        const OSMPBF::PrimitiveBlock& block,
                        const OSMPBF::PrimitiveGroup &group,
                        PreprocessorCallback::RawBlockData& data);

    void ReadWays(const TypeConfig& typeConfig,
                  const OSMPBF::PrimitiveBlock& block,
                  const OSMPBF::PrimitiveGroup &group,
                  PreprocessorCallback::RawBlockData& data);

    void ReadRelations(const TypeConfig& typeConfig,
                       const OSMPBF::PrimitiveBlock& block,
                       const OSMPBF::PrimitiveGroup &group,
                       PreprocessorCallback::RawBlockData& data);

    void ProcessBlock(const TypeConfigRef& typeConfig,
                      std::unique_ptr<OSMPBF::PrimitiveBlock>&& block);

  public:
    explicit PreprocessPBF(PreprocessorCallback& callback);
    ~PreprocessPBF() override;

    bool Import(const TypeConfigRef& typeConfig,
                const ImportParameter& parameter,
                Progress& progress,
                const std::string& filename) override;
  };
}

#endif
