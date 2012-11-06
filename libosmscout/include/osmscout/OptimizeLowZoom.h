#ifndef OSMSCOUT_OPTIMIZELOWZOOM_H
#define OSMSCOUT_OPTIMIZELOWZOOM_H

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

#include <set>
#include <string>

#include <osmscout/TypeSet.h>

#include <osmscout/Way.h>

#include <osmscout/util/FileScanner.h>

namespace osmscout {

  class OptimizeLowZoom
  {
  private:
    struct TypeData
    {
      uint32_t   indexLevel;   //! magnification level of index
      FileOffset bitmapOffset; //! Position in file where the offset of the bitmap is written
      uint8_t    dataOffsetBytes;

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
    };

  private:
    std::string               datafile;      //! Basename part fo the data file name
    std::string               datafilename;  //! complete filename for data file
    mutable FileScanner       scanner;       //! File stream to the data file

    double                    magnification; //! Vergrößerung, bis zur der Optimization unterstützt wird
    std::map<TypeId,TypeData> typesData;     //! Index information for all types

  public:
    OptimizeLowZoom();
    virtual ~OptimizeLowZoom();

    bool Open(const std::string& path);
    bool Close();

    bool HasOptimizations(double magnification) const;

    bool GetOffsets(const TypeData& typeData,
                    double minlon,
                    double minlat,
                    double maxlon,
                    double maxlat,
                    std::vector<FileOffset>& offsets) const;

    bool GetWays(double lonMin, double latMin,
                 double lonMax, double latMax,
                 size_t maxWayCount,
                 std::vector<TypeSet>& wayTypes,
                 std::vector<WayRef>& ways) const;
  };
}

#endif
