#ifndef OSMSCOUT_COVERAGEINDEX_H
#define OSMSCOUT_COVERAGEINDEX_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2018  Tim Teulings

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
#include <vector>

#include <osmscout/Pixel.h>

#include <osmscout/util/FileScanner.h>

namespace osmscout {

  /**
    \ingroup Database

    CoverageIndex allows you to check for tiles that hold map data
    (for this database). It allows to identify database regions of different
    databases that overlap.
    */
  class OSMSCOUT_API CoverageIndex
  {
  public:
    static const char* const COVERAGE_IDX;

  private:
    std::string           datafilename;   //!< Full path and name of the data file
    mutable FileScanner   scanner;        //!< Scanner instance for reading this file

    uint32_t              cellLevel;
    double                cellWidth;
    double                cellHeight;
    Pixel                 minCell;
    Pixel                 maxCell;
    uint32_t              width;
    uint32_t              height;
    std::vector<uint8_t>  bitmap;

  public:
    CoverageIndex() = default;

    void Close();
    bool Open(const std::string& path);

    inline bool IsOpen() const
    {
      return scanner.IsOpen();
    }

    Pixel GetTile(const GeoCoord& coord) const;
    bool IsCovered(const Pixel& tile) const;

    bool IsCovered(const GeoCoord& coord) const;
  };

  using CoverageIndexRef = std::shared_ptr<CoverageIndex>;
}

#endif
