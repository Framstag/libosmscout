#ifndef OSMSCOUT_AREANODEINDEX_H
#define OSMSCOUT_AREANODEINDEX_H

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

#include <vector>

#include <osmscout/TypeSet.h>

#include <osmscout/util/FileScanner.h>

namespace osmscout {

  /**
    AreaWayIndex allows you to find ways and way relations in
    a given area.

    Ways can be limited by type and result count.
    */
  class AreaNodeIndex
  {
  private:
    struct TypeData
    {
      uint32_t   indexLevel;

      FileOffset indexOffset;

      uint32_t   cellXStart;
      uint32_t   cellXEnd;
      uint32_t   cellYStart;
      uint32_t   cellYEnd;
      uint32_t   cellXCount;
      uint32_t   cellYCount;

      double     cellWidth;
      double     cellHeight;

      double     minLon;
      double     maxLon;
      double     minLat;
      double     maxLat;

      TypeData();
    };

  private:
    std::string           filepart;       //! name of the data file
    std::string           datafilename;   //! Full path and name of the data file
    mutable FileScanner   scanner;        //! Scanner instance for reading this file

    std::vector<TypeData> nodeTypeData;

  private:
    bool GetOffsets(const TypeData& typeData,
                    double minlon,
                    double minlat,
                    double maxlon,
                    double maxlat,
                    size_t maxNodeCount,
                    std::vector<FileOffset>& offsets,
                    size_t currentSize,
                    bool& sizeExceeded) const;

  public:
    AreaNodeIndex();

    bool Load(const std::string& path);

    bool GetOffsets(double minlon,
                    double minlat,
                    double maxlon,
                    double maxlat,
                    const TypeSet& nodeTypes,
                    size_t maxNodeCount,
                    std::vector<FileOffset>& nodeOffsets) const;

    void DumpStatistics();
  };
}

#endif
