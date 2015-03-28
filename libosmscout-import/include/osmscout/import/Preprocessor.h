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

  class PreprocessorCallback
  {
  public:
    virtual ~PreprocessorCallback();

    virtual void ProcessNode(const TypeConfig& typeConfig,
                             const OSMId& id,
                             const double& lon, const double& lat,
                             const TagMap& tags) = 0;

    virtual void ProcessWay(const TypeConfig& typeConfig,
                            const OSMId& id,
                            std::vector<OSMId>& nodes,
                            const TagMap& tags) = 0;

    virtual void ProcessRelation(const TypeConfig& typeConfig,
                                 const OSMId& id,
                                 const std::vector<RawRelation::Member>& members,
                                 const TagMap& tags) = 0;

  };

  class Preprocessor
  {
  public:
    virtual ~Preprocessor();
  };
}

#endif
