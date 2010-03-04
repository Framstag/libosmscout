#ifndef OSMSCOUT_AREANODEINDEX_H
#define OSMSCOUT_AREANODEINDEX_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2009  Tim Teulings

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

#include <set>

#include <osmscout/Tiles.h>
#include <osmscout/StyleConfig.h>

namespace osmscout {
  class AreaNodeIndex
  {
  private:
  struct IndexEntry
  {
    std::vector<Id> ids;
  };

  private:
    std::map<TypeId,std::map<TileId,IndexEntry> > areaNodeIndex;

  public:
    bool LoadAreaNodeIndex(const std::string& path);

    size_t GetNodes(TypeId drawType,
                    size_t tileMinX, size_t tileMinY,
                    size_t tileMaxX, size_t tileMaxY) const;

    bool GetIds(const StyleConfig& styleConfig,
                double minlon, double minlat,
                double maxlon, double maxlat,
                double magnification,
                size_t maxPriority,
                std::vector<Id>& ids) const;

    void DumpStatistics();
  };
}

#endif
