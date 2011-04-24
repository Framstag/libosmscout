#ifndef OSMSCOUT_PREPROCESS_PBF_H
#define OSMSCOUT_PREPROCESS_PBF_H

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
#include <string>
#include <vector>

#include <osmscout/pbf/fileformat.pb.h>
#include <osmscout/pbf/osmformat.pb.h>

#include <osmscout/RawNode.h>
#include <osmscout/RawRelation.h>
#include <osmscout/RawWay.h>

#include <osmscout/Import.h>
#include <osmscout/Types.h>

namespace osmscout {
  class PreprocessPBF : public ImportModule
  {
  private:
    RawNode                     rawNode;
    RawWay                      rawWay;
    RawRelation                 rawRel;
    std::vector<Tag>            tags;
    std::map<TagId,std::string> tagMap;
    std::vector<Id>             nodes;

    uint32_t                    nodeCount;
    uint32_t                    wayCount;
    uint32_t                    areaCount;
    uint32_t                    relationCount;

  private:
    void ReadNodes(const TypeConfig& typeConfig,
                   const PBF::PrimitiveBlock& block,
                   const PBF::PrimitiveGroup &group,
                   FileWriter& nodeWriter);

    void ReadDenseNodes(const TypeConfig& typeConfig,
                        const PBF::PrimitiveBlock& block,
                        const PBF::PrimitiveGroup &group,
                        FileWriter& nodeWriter);

    void ReadWays(const TypeConfig& typeConfig,
                   const PBF::PrimitiveBlock& block,
                   const PBF::PrimitiveGroup &group,
                   FileWriter& wayWriter);

    void ReadRelations(const TypeConfig& typeConfig,
                       const PBF::PrimitiveBlock& block,
                       const PBF::PrimitiveGroup &group,
                       FileWriter& relationWriter);

  public:
    std::string GetDescription() const;
    bool Import(const ImportParameter& parameter,
                Progress& progress,
                const TypeConfig& typeConfig);
  };
}

#endif
