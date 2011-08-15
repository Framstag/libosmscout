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

#include <osmscout/StyleConfig.h>
#include <osmscout/Way.h>

#include <osmscout/util/FileScanner.h>

namespace osmscout {

  class OptimizeLowZoom
  {
  private:
    struct DataInfo
    {
      FileOffset dataOffset;
      uint32_t   dataCount;
    };

  private:
    std::string               datafile;      //! Basename part fo the data file name
    std::string               datafilename;  //! complete filename for data file
    mutable FileScanner       scanner;       //! File stream to the data file

    double                    magnification; //! Vergrößerung, bis zur der Optimization unterstützt wird
    std::map<TypeId,DataInfo> dataInfos;     //! Some info about the data of a type

  public:
    OptimizeLowZoom();
    virtual ~OptimizeLowZoom();

    bool Open(const std::string& path);
    bool Close();

    bool HasOptimizations(double magnification) const;

    bool GetWays(const StyleConfig& styleConfig,
                 double lonMin, double latMin,
                 double lonMax, double latMax,
                 size_t maxWayCount,
                 std::vector<TypeId>& wayTypes,
                 std::vector<WayRef>& ways) const;
  };
}

#endif
