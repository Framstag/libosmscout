#ifndef OSMSCOUT_AREAAREAINDEX_H
#define OSMSCOUT_AREAAREAINDEX_H

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

#include <vector>

#include <osmscout/TypeSet.h>

#include <osmscout/util/Cache.h>
#include <osmscout/util/FileScanner.h>

namespace osmscout {

  /**
    AreaAreaIndex allows you to find areas and area relations in
    a given area.

    For areas result can be limited by the maximum level (which in turn
    defines the minimum size of the resulting areas since an area in a given
    level must fit into the cell size (but can cross cell borders)) and the
    maximum number of areas found.

    Way in turn can be limited by type and result count.

    Internally the index is implemented as quadtree. As a result each index entry
    has 4 children (besides entries in the lowest level).
    */
  class OSMSCOUT_API AreaAreaIndex
  {
  private:
    /**
      An individual index entry in a index cell
      */
    struct IndexEntry
    {
      TypeId     type;
      FileOffset offset;
    };

    /**
      Datastructure for every index cell of our index.
      */
    struct IndexCell
    {
      FileOffset              children[4]; //! File index of each of the four children, or 0 if there is no child
      std::vector<IndexEntry> areas;
    };

    typedef Cache<FileOffset,IndexCell> IndexCache;

    struct IndexCacheValueSizer : public IndexCache::ValueSizer
    {
      unsigned long GetSize(const IndexCell& value) const
      {
        unsigned long memory=0;

        memory+=sizeof(value);

        // Areas
        memory+=value.areas.size()*sizeof(IndexEntry);

        return memory;
      }
    };

  private:
    std::string                     filepart;       //! name of the data file
    std::string                     datafilename;   //! Fullpath and name of the data file
    mutable FileScanner             scanner;        //! Scanner instance for reading this file

    std::vector<double>             cellWidth;      //! Precalculated cellWidth for each level of the quadtree
    std::vector<double>             cellHeight;     //! Precalculated cellHeight for each level of the quadtree
    uint32_t                        maxLevel;       //! Maximum level in index
    FileOffset                      topLevelOffset; //! Offset o fthe top level index entry

    mutable IndexCache              indexCache;     //! Cached map of all index entries by file offset

  private:
    bool GetIndexCell(uint32_t level,
                       FileOffset offset,
                       IndexCache::CacheRef& cacheRef) const;

  public:
    AreaAreaIndex(size_t cacheSize);

    bool Load(const std::string& path);

    bool GetOffsets(double minlon,
                    double minlat,
                    double maxlon,
                    double maxlat,
                    size_t maxAreaLevel,
                    const TypeSet& types,
                    size_t maxAreaCount,
                    std::vector<FileOffset>& wayAreaOffsets) const;

    void DumpStatistics();
  };
}

#endif
