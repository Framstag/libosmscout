#ifndef OSMSCOUT_AREAROUTEINDEX_H
#define OSMSCOUT_AREAROUTEINDEX_H

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

#include <memory>
#include <mutex>
#include <unordered_set>
#include <vector>

#include <osmscout/AreaIndex.h>
#include <osmscout/TypeConfig.h>
#include <osmscout/TypeInfoSet.h>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/TileId.h>
#include <osmscout/system/Compiler.h>

namespace osmscout {

  /**
    \ingroup Database
    AreaRouteIndex allows you to find routes and way relations in
    a given area.

    Routes can be limited by type and result count.
    */
  class OSMSCOUT_API AreaRouteIndex CLASS_FINAL : public AreaIndex
  {
  public:
    static const char* const AREA_ROUTE_IDX;

  private:
    void ReadTypeData(const TypeConfigRef& typeConfig,
                      TypeData &data) override;

  public:
    AreaRouteIndex();

    // disable copy and move
    AreaRouteIndex(const AreaRouteIndex&) = delete;
    AreaRouteIndex(AreaRouteIndex&&) = delete;
    AreaRouteIndex& operator=(const AreaRouteIndex&) = delete;
    AreaRouteIndex& operator=(AreaRouteIndex&&) = delete;

    ~AreaRouteIndex() override = default;
  };

  using AreaRouteIndexRef = std::shared_ptr<AreaRouteIndex>;
}

#endif
