//
//  SRTM.cpp
//  libosmscout
//
//  Created by Vladimir Vyskocil on 06/01/13.
//  Copyright (c) 2013 Vladimir Vyskocil. All rights reserved.
//

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

#include <cstdlib>
#include <fstream>
#include <ostream>
#include <sstream>

#include <osmscout/elevation/SRTM.h>

#include <osmscout/log/Logger.h>

#include <osmscout/system/Math.h>

namespace osmscout {

#define SRTM1_GRID 3601
#define SRTM3_GRID 1201
#define SRTM1_FILESIZE (SRTM1_GRID*SRTM1_GRID*2)
#define SRTM3_FILESIZE (SRTM3_GRID*SRTM3_GRID*2)

  SRTM::SRTM(const std::string& path)
  : srtmPath(path),
    heights(nullptr)
  {
  }

  SRTM::~SRTM()
  {
    delete heights;
  }

  /**
   * generate SRTM3 filename like N43E006.hgt from integer part of latitude and longitude
   */
  std::string SRTM::CalculateHGTFilename(int patchLat,
                                         int patchLon) const
  {
    std::ostringstream fileName;

    if (patchLat>=0) {
      fileName << "N";
    }
    else {
      fileName << "S";
      patchLat=std::abs(patchLat);
    }

    if (patchLat<10) {
      fileName << "0";
    }

    fileName << patchLat;

    if (patchLon>=0) {
      fileName << "E";
    }
    else {
      fileName << "W";
      patchLon=std::abs(patchLon);
    }

    if (patchLon<10) {
      fileName << "0";
    }

    if (patchLon<100) {
      fileName << "0";
    }

    fileName << patchLon;

    fileName << ".hgt";

    return fileName.str();
  }

  bool SRTM::AssureCorrectFileLoaded(double latitude,
                                     double longitude)
  {
    int         patchLat     =int(floor(latitude));
    int         patchLon     =int(floor(longitude));
    std::string patchFilename=CalculateHGTFilename(patchLat,
                                                   patchLon);

    if (currentFilename==patchFilename) {
      return heights!=nullptr;
    }

    std::istream::pos_type length;

    currentFilename=std::move(patchFilename);

    if (heights!=nullptr) {
      delete heights;
      heights=nullptr;
    }

    std::ifstream currentFile;

    currentFile.open(srtmPath+"/"+currentFilename,
                     std::ios::in | std::ios::binary | std::ios::ate);

    if (!currentFile.std::ios::good()) {
      currentFile.close();
      return false;
    }

    length=currentFile.tellg();
    currentFile.seekg(0,
                      std::ios::beg);

    if (length==(std::istream::pos_type) SRTM1_FILESIZE) {
      rows   =SRTM1_GRID;
      columns=SRTM1_GRID;
      patchSize=SRTM1_FILESIZE;
      log.Debug() << "Open SRTM1 hgt file: " << (srtmPath+"/"+currentFilename).c_str();
    }
    else if (length==(std::istream::pos_type) SRTM3_FILESIZE) {
      rows   =SRTM3_GRID;
      columns=SRTM3_GRID;
      patchSize=SRTM3_FILESIZE;
      log.Debug() << "Open SRTM3 hgt file: " << (srtmPath+"/"+currentFilename).c_str();
    }
    else {
      return false;
    }

    heights=new uint8_t[SRTM::patchSize];

    currentFile.read((char*) heights,
                     SRTM::patchSize);

    if (!currentFile.std::ios::good()) {
      delete[] heights;
      heights=nullptr;
      currentFile.close();

      return false;
    }

    currentFile.close();

    fileBoundingBox=GeoBox(GeoCoord(patchLat,patchLon),GeoCoord(patchLat+1,patchLon+1));

    return true;
  }

  int32_t SRTM::GetHeight(size_t column, size_t row) const
  {
    return (heights[2*row*SRTM::columns+2*column] << 8)+
           (heights[2*row*SRTM::columns+2*column+1]);
  }

  /**
   * return the height at (latitude,longitude) or SRTM::nodata if no data at the location
   */
  int32_t SRTM::GetHeightAtLocation(const GeoCoord& coord)
  {
    if (!AssureCorrectFileLoaded(coord.GetLat(),
                                 coord.GetLon())) {
      return nodata;
    }

    if (heights!=nullptr) {
      [[maybe_unused]] double pixelSize=1.0/double(SRTM::rows-1);
      double                  latitude=coord.GetLat();
      double                  longitude=coord.GetLon();
      double                  fracLat  =latitude>=0.0 ? 1-latitude+floor(latitude) : ceil(latitude)-latitude;
      double                  fracLon  =longitude>=0.0 ? longitude-floor(longitude) : 1-ceil(longitude)+longitude;
      int                     col      =int(floor(fracLon*SRTM::columns));
      int                     row      =int(floor(fracLat*SRTM::rows));

      int32_t h1=GetHeight(col,row);

      log.Info() << "Height at " << coord.GetDisplayText() << " " << fracLat << "x" << fracLon << " with tile coords " << col << "," << row << " = " << h1;

#ifndef SRTM_BILINEAR_INTERPOLATION
      return (int32_t) floor(h1);
#else
      // TODO: Hmmm, is the right interpolation box? Have to look again at the original sources!
      int h2 = GetHeight(col+1,row);
      int h3 = GetHeight(col,row+1);
      int h4 = GetHeight(col+1,row+1);

      // bilinear interpolation
      double fLat = fracLat - fracLat*row/SRTM::rows;
      double fLon = fracLon - fracLon*pixelSize/SRTM::columns;
      log.Info() << "fLat=" << fLat << " fLon=" << fLon;

      double h = h1*(1-fLat)*(1-fLon) + h2*fLat*(1-fLon) + h3*(1-fLat)*fLon + h4*fLat*fLon;
      return (int32_t)floor(h+0.5);
#endif
    }

    return nodata;
  }

  SRTMDataRef SRTM::GetHeightInBoundingBox(const GeoBox& boundingBox)
  {
    if (!AssureCorrectFileLoaded(boundingBox.GetMinCoord().GetLat(),
                                 boundingBox.GetMinCoord().GetLon())) {
      return nullptr;
    }

    SRTMDataRef tile=std::make_shared<SRTMData>();

    tile->boundingBox=boundingBox.Intersection(fileBoundingBox);

    log.Info() << "SRTM boundingBox: " << fileBoundingBox.GetDisplayText();
    log.Info() << "Requested boundingBox: " << boundingBox.GetDisplayText();
    log.Info() << "Clipped boundingBox: " << tile->boundingBox.GetDisplayText();

    // TODO: We simply copy the complete height map for now

    tile->boundingBox=fileBoundingBox;
    tile->columns=columns;
    tile->rows=rows;

    tile->heights.resize(columns*rows);

    for (size_t y=0; y<rows; y++) {
      for (size_t x=0; x<columns; x++) {
        int32_t height=GetHeight(x,y);

        if (height>10000) {
          log.Error() << "Unexpected height: " << x << "," << y << " = " << height;

          height=nodata;
        }

        tile->heights[y*columns+x]=height;
      }
    }

    return tile;
  }
}
