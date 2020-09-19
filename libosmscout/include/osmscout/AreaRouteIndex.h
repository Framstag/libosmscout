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

#include <osmscout/TypeConfig.h>
#include <osmscout/TypeInfoSet.h>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/TileId.h>

namespace osmscout {

  /**
    \ingroup Database
    AreaRouteIndex allows you to find routes and way relations in
    a given area.

    Routes can be limited by type and result count.
    */
  class OSMSCOUT_API AreaRouteIndex
  {
  public:
    static const char* const AREA_ROUTE_IDX;

  private:
    std::string datafilename;   //!< Full path and name of the data file
    bool memoryMappedData;
    TypeConfigRef typeConfig;

  public:
    AreaRouteIndex() = default;
    virtual ~AreaRouteIndex();

    void Close();
    bool Open(const TypeConfigRef& typeConfig,
              const std::string& path,
              bool memoryMappedData);

    inline std::string GetFilename() const
    {
      return datafilename;
    }

    bool GetOffsets(const GeoBox& boundingBox,
                    const TypeInfoSet& types,
                    std::vector<FileOffset>& offsets,
                    TypeInfoSet& loadedTypes) const;
  };

  using AreaRouteIndexRef = std::shared_ptr<AreaRouteIndex>;
}

#endif
