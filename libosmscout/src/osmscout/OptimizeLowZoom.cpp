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

#include <osmscout/OptimizeLowZoom.h>

#include <cassert>

#include <osmscout/Way.h>

#include <osmscout/Util.h>

#include <osmscout/util/Projection.h>
#include <osmscout/util/StopClock.h>
#include <osmscout/util/String.h>
#include <osmscout/util/Transformation.h>

#include <osmscout/private/Math.h>

#include <iostream>

namespace osmscout
{
  OptimizeLowZoom::OptimizeLowZoom()
  : datafile("optimized.dat"),
    magnification(0.0)
  {
    // no code
  }

  OptimizeLowZoom::~OptimizeLowZoom()
  {
    if (scanner.IsOpen()) {
      Close();
    }
  }

  bool OptimizeLowZoom::Open(const std::string& path)
  {
    datafilename=AppendFileToDir(path,datafile);

    if (scanner.Open(datafilename)) {
      uint32_t optimizationMaxMag;
      uint32_t typeCount;

      scanner.ReadNumber(optimizationMaxMag);
      scanner.Read(typeCount);

      if (scanner.HasError()) {
        return false;
      }

      magnification=pow(2.0,(int)optimizationMaxMag);

      for (size_t i=1; i<=typeCount; i++) {
        TypeId   typeId;
        DataInfo info;

        scanner.Read(typeId);
        scanner.Read(info.dataCount);
        scanner.Read(info.dataOffset);

        dataInfos[typeId]=info;
      }
    }


    return scanner.IsOpen() && !scanner.HasError();
  }

  bool OptimizeLowZoom::Close()
  {
    bool success=true;

    if (scanner.IsOpen()) {
      if (!scanner.Close()) {
        success=false;
      }
    }

    return success;
  }

  bool OptimizeLowZoom::HasOptimizations(double magnification) const
  {
    return magnification<=this->magnification;
  }

  bool OptimizeLowZoom::GetWays(const StyleConfig& styleConfig,
                                double lonMin, double latMin,
                                double lonMax, double latMax,
                                size_t maxWayCount,
                                std::vector<TypeId>& wayTypes,
                                std::vector<WayRef>& ways) const
  {
    std::vector<TypeId>::iterator type=wayTypes.begin();
    while (type!=wayTypes.end()) {
      std::map<TypeId, DataInfo>::const_iterator dataInfo=dataInfos.find(*type);

      if (dataInfo!=dataInfos.end()) {
        if (!scanner.IsOpen()) {
          if (!scanner.Open(datafilename)) {
            std::cerr << "Error while opening " << datafilename << " for reading!" << std::endl;
            return false;
          }
        }

        if (!scanner.SetPos(dataInfo->second.dataOffset)) {
          std::cerr << "Error while reading data from file " << datafilename  << std::endl;
          type++;
          continue;
        }

        for (size_t i=1; i<=dataInfo->second.dataCount; i++) {
          WayRef way = new Way();

          if (!way->Read(scanner)) {
            std::cerr << "Error while reading data from file " << datafilename  << std::endl;
            continue;
          }

          ways.push_back(way);
        }

        type=wayTypes.erase(type);
      }
      else {
        type++;
      }
    }

    return true;
  }
}

