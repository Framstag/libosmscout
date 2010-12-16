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

#include <osmscout/Cache.h>
#include <osmscout/FileScanner.h>
#include <osmscout/StyleConfig.h>

namespace osmscout {

  class AreaNodeIndex
  {
  private:
    struct Leaf
    {
      FileOffset              children[4];
      std::vector<FileOffset> offsets;


      Leaf();
    };

    typedef Cache<FileOffset,Leaf> LeafCache;

    struct LeafCacheValueSizer : public LeafCache::ValueSizer
    {
      unsigned long GetSize(const Leaf& value) const
      {
        unsigned long memory=0;

        memory+=sizeof(value);

        memory+=value.offsets.size()*sizeof(FileOffset);

        return memory;
      }
    };

  private:
    std::string             filepart;
    std::string             datafilename;
    mutable FileScanner     scanner;

    std::vector<double>     cellWidth;
    std::vector<double>     cellHeight;
    mutable LeafCache       leafCache;
    std::vector<FileOffset> topLevelOffsets;

  private:
    bool GetIndexEntry(FileOffset offset,
                       LeafCache::CacheRef& cacheRef) const;

  public:
    AreaNodeIndex(size_t cacheSize);

    bool LoadAreaNodeIndex(const std::string& path);

    bool GetOffsets(const StyleConfig& styleConfig,
                    double minlon,
                    double minlat,
                    double maxlon,
                    double maxlat,
                    const std::vector<TypeId>& types,
                    size_t maxNodeCount,
                    std::vector<FileOffset>& nodeOffsets) const;

    void DumpStatistics();
  };
}

#endif
