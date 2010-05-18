#ifndef OSMSCOUT_AREAINDEX_H
#define OSMSCOUT_AREAINDEX_H

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
#include <vector>
#include <cassert>
#include <cmath>
#include <iostream>

#include <osmscout/FileScanner.h>
#include <osmscout/StyleConfig.h>
#include <osmscout/Util.h>

namespace osmscout {

  /**
    */
  class AreaIndex
  {
  private:
    struct IndexEntry
    {
      FileOffset                                children[4];
      std::map<TypeId,std::vector<FileOffset> > ways;
      std::map<TypeId,std::vector<FileOffset> > relWays;
      std::vector<FileOffset>                   areas;
      std::vector<FileOffset>                   relAreas;
    };

    typedef std::map<size_t,IndexEntry> IndexLevel;

  private:
    std::string             filepart;
    std::vector<double>     cellWidth;
    std::vector<double>     cellHeight;
    uint32_t                maxLevel;
    std::vector<IndexLevel> index;

  public:
    AreaIndex();

    bool Load(const std::string& path);

    bool GetOffsets(const StyleConfig& styleConfig,
                    double minlon,
                    double minlat,
                    double maxlon,
                    double maxlat,
                    size_t maxAreaLevel,
                    size_t maxAreaCount,
                    const std::vector<TypeId>& wayTypes,
                    size_t maxWayCount,
                    std::set<FileOffset>& wayWayOffsets,
                    std::set<FileOffset>& relationWayOffsets,
                    std::set<FileOffset>& wayAreaOffsets,
                    std::set<FileOffset>& relationAreaOffsets) const;

    void DumpStatistics();
  };
}

#endif
