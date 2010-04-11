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
    Area index handles an index over instance of <class T> where T contains a list of
    nodes that build an area that can be abstracted by a bounding box. The index will
    return file offsets of all instances of <class T> that intersect a given area.
    */
  class AreaInAreaIndex
  {
  private:
    struct IndexEntry
    {
      std::vector<FileOffset> dataOffsets;
      FileOffset              children[4];
    };

    typedef std::map<size_t,IndexEntry> IndexLevel;

  private:
    std::string             filepart;
    std::vector<double>     cellWidth;
    std::vector<double>     cellHeight;
    uint32_t                maxLevel;
    std::vector<IndexLevel> index;

  public:
    AreaInAreaIndex(const std::string& filename);

    bool Load(const std::string& path);

    void GetOffsets(const StyleConfig& styleConfig,
                    double minlon, double minlat,
                    double maxlon, double maxlat,
                    size_t maxLevel,
                    size_t maxCount,
                    std::set<FileOffset>& offsets) const;

    void DumpStatistics();
  };


  /**
    Way index handles an index over instance of <class T> where T contains a list of
    nodes that build an way that can be abstracted by a bounding box. The index will
    return file offsets of all instances of <class T> that intersect a given way.
    */
  class WayInAreaIndex
  {
  private:
    struct IndexEntry
    {
      std::map<TypeId,std::vector<FileOffset> > dataOffsets;
      FileOffset                                children[4];
    };

    typedef std::map<size_t,IndexEntry> IndexLevel;

  private:
    std::string             filepart;
    std::vector<double>     cellWidth;
    std::vector<double>     cellHeight;
    uint32_t                maxLevel;
    std::vector<IndexLevel> index;

  public:
    WayInAreaIndex(const std::string& filename);

    bool Load(const std::string& path);

    void GetOffsets(const StyleConfig& styleConfig,
                    double minlon, double minlat,
                    double maxlon, double maxlat,
                    const std::vector<TypeId>& types,
                    size_t maxCount,
                    std::set<FileOffset>& offsets) const;

    void DumpStatistics();
  };
}

#endif
