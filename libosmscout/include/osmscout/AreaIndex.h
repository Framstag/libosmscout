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
#include <vector>
#include <cassert>
#include <cmath>
#include <iostream>

#include <osmscout/Cache.h>
#include <osmscout/FileScanner.h>
#include <osmscout/StyleConfig.h>
#include <osmscout/Util.h>

namespace osmscout {

  /**
    AreaIndex allows you to find areas, ways, area relations and way releations in
    a given area.

    For area structure result can be limited by the maximum level (which in turn defines
    the mimimum size of the resultng areas since and area in a given level must fit into
    the cell size (but can cross cell borders)) and the maximum number of areas found.

    Way in turn can be limited by type and result count.

    Internal the index is implemented as quadtree. As a result each index entry has 4
    children (besides entries in the lowest level).
    */
  class AreaIndex
  {
  private:
    /**
      Datastructure for every index entry of our index.
      */
    struct IndexEntry
    {
      FileOffset                                children[4]; //! File index of each of the four children, or 0 if there is no child
      std::map<TypeId,std::vector<FileOffset> > ways;
      std::map<TypeId,std::vector<FileOffset> > relWays;
      std::vector<FileOffset>                   areas;
      std::vector<FileOffset>                   relAreas;
    };

    typedef Cache<FileOffset,IndexEntry> IndexCache;

    struct IndexCacheValueSizer : public IndexCache::ValueSizer
    {
      size_t GetSize(const IndexEntry& value) const
      {
        size_t memory=0;

        memory+=sizeof(value);

        // Ways
        memory+=value.ways.size()*(sizeof(TypeId)+sizeof(std::vector<FileOffset>));

        for (std::map<TypeId,std::vector<FileOffset> >::const_iterator iter2=value.ways.begin();
             iter2!=value.ways.end();
             ++iter2) {
          memory+=iter2->second.size()*sizeof(FileOffset);
        }

        // RelWays
        memory+=value.relWays.size()*(sizeof(TypeId)+sizeof(std::vector<FileOffset>));

        for (std::map<TypeId,std::vector<FileOffset> >::const_iterator iter2=value.relWays.begin();
             iter2!=value.relWays.end();
             ++iter2) {
          memory+=iter2->second.size()*sizeof(FileOffset);
        }

        // Areas
        memory+=value.areas.size()*sizeof(FileOffset);

        // RelAreas
        memory+=value.relAreas.size()*sizeof(FileOffset);

        return memory;
      }
    };

  private:
    std::string                     filepart;
    std::string                     datafilename;
    mutable FileScanner             scanner;

    std::vector<double>             cellWidth;
    std::vector<double>             cellHeight;
    uint32_t                        maxLevel;       //! Maximum level in index
    FileOffset                      topLevelOffset; //! Offset o fthe top level index entry

    mutable IndexCache              indexCache;     //! Cached map of all index entries by file offset

  private:
    bool GetIndexEntry(uint32_t level,
                       FileOffset offset,
                       IndexCache::CacheRef& cacheRef) const;

  public:
    AreaIndex(size_t cacheSize);

    bool Load(const std::string& path);

    bool GetOffsets(const StyleConfig& styleConfig,
                    double minlon,
                    double minlat,
                    double maxlon,
                    double maxlat,
                    size_t maxWayLevel,
                    size_t maxAreaLevel,
                    size_t maxAreaCount,
                    const std::vector<TypeId>& wayTypes,
                    size_t maxWayCount,
                    std::vector<FileOffset>& wayWayOffsets,
                    std::vector<FileOffset>& relationWayOffsets,
                    std::vector<FileOffset>& wayAreaOffsets,
                    std::vector<FileOffset>& relationAreaOffsets) const;

    void DumpStatistics();
  };
}

#endif
