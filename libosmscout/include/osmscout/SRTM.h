//
//  SRTM.h
//  libosmscout
//
//  Created by Vladimir Vyskocil on 06/01/13.
//  Copyright (c) 2013 Vladimir Vyskocil. All rights reserved.
//

#ifndef libosmscout_SRTM_h
#define libosmscout_SRTM_h

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

#include <string>

#include <osmscout/GeoCoord.h>

#include <osmscout/OSMScoutTypes.h>

#include <osmscout/system/SystemTypes.h>

namespace osmscout {

  /**
   * Read elevation data in hgt format
   */
  class OSMSCOUT_API SRTM
  {
  public:
    static const int32_t nodata=-32768;

  private:
    std::string srtmPath;
    std::string currentFilename;
    size_t      rows;
    size_t      columns;
    size_t      patchSize;
    uint8_t     *heights;

  private:
    bool AssureCorrectFileLoaded(double latitude,
                                 double longitude);

    std::string CalculateHGTFilename(int patchLat,
                                     int patchLon) const;
  public:
    explicit SRTM(const std::string& path);

    virtual ~SRTM();

    int32_t GetHeightAtLocation(double latitude,
                                double longitude);

    int32_t GetHeightAtLocation(const GeoCoord& coord);
  };
}

#endif
