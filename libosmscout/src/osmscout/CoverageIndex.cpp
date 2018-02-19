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

#include <osmscout/CoverageIndex.h>

#include <algorithm>

#include <osmscout/util/File.h>
#include <osmscout/util/Geometry.h>
#include <osmscout/util/Logger.h>

namespace osmscout {

  const char* CoverageIndex::COVERAGE_IDX="coverage.idx";

  CoverageIndex::CoverageIndex()
  {
    // no code
  }

  void CoverageIndex::Close()
  {
    try {
      if (scanner.IsOpen()) {
        scanner.Close();
      }
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      scanner.CloseFailsafe();
    }
  }

  bool CoverageIndex::Open(const std::string& path)
  {
    datafilename=AppendFileToDir(path,COVERAGE_IDX);

    try {
      scanner.Open(datafilename,FileScanner::FastRandom,true);

      scanner.Read(cellLevel);

      double cellMagnification=std::pow(2.0,cellLevel);

      cellWidth=360.0/cellMagnification;
      cellHeight=180.0/cellMagnification;
      scanner.Read(minCell.x);
      scanner.Read(minCell.y);
      scanner.Read(maxCell.x);
      scanner.Read(maxCell.y);

      width=maxCell.x-minCell.x+1;
      height=maxCell.y-minCell.y+1;

      bitmap.resize((width*height)/8,0);

      for (auto& b : bitmap) {
        scanner.Read(b);
      }

      return !scanner.HasError();
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      return false;
    }
  }

  Pixel CoverageIndex::GetTile(const GeoCoord& coord) const
  {
    return {(uint32_t)((coord.GetLon()+180.0)/cellWidth),
            (uint32_t)((coord.GetLat()+90.0)/cellHeight)};
  }

  bool CoverageIndex::IsCovered(const Pixel& tile) const
  {
    if (tile.x<minCell.x ||
        tile.x>maxCell.x ||
        tile.y<minCell.y ||
        tile.y>maxCell.y) {
      return false;
    }

    size_t bitInMap=((tile.y-minCell.y)*width+(tile.x-minCell.x));
    size_t byteInMap=bitInMap/8;
    size_t bitInByte=bitInMap%8;

    return (bitmap[byteInMap] & ((uint8_t)1 << bitInByte))!=0;
  }

  bool CoverageIndex::IsCovered(const GeoCoord& coord) const
  {
    return IsCovered(GetTile(coord));
  }

}

