#ifndef OSMSCOUT_AREAWAYINDEX_H
#define OSMSCOUT_AREAWAYINDEX_H

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

#include <osmscout/StyleConfig.h>

#include <osmscout/Util.h>

#include <osmscout/util/FileScanner.h>

namespace osmscout {

  /**
    AreaWayIndex allows you to find ways and way releations in
    a given area.

    Ways can be limited by type and result count.
    */
  class AreaWayIndex
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
    std::string           datafilename;   //! Fullpath and name of the data file
    mutable FileScanner   scanner;        //! Scanner instance for reading this file

    std::vector<TypeData> wayTypeData;
    std::vector<TypeData> relTypeData;

  private:
  bool GetOffsets(TypeId type,
                  const std::vector<TypeData>& typeData,
                  double minlon,
                  double minlat,
                  double maxlon,
                  double maxlat,
                  size_t maxWayCount,
                  std::vector<FileOffset>& offsets,
                  size_t currentSize,
                  bool& sizeExceeded) const;

  public:
    AreaWayIndex();

    bool Load(const std::string& path);

    bool GetOffsets(double minlon,
                    double minlat,
                    double maxlon,
                    double maxlat,
                    const std::vector<TypeId>& wayTypes,
                    size_t maxWayCount,
                    std::vector<FileOffset>& wayWayOffsets,
                    std::vector<FileOffset>& relationWayOffsets) const;

    void DumpStatistics();
  };
}

#endif
