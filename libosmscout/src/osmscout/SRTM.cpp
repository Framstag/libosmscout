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
#include <ostream>
#include <sstream>

#include <osmscout/SRTM.h>

#include <osmscout/util/Logger.h>

#include <osmscout/system/Math.h>
#include <osmscout/system/OSMScoutTypes.h>

namespace osmscout {

  size_t SRTM::rows = SRTM3_GRID;
  size_t SRTM::columns = SRTM3_GRID;
  size_t SRTM::patchSize = 2*rows*columns;

  SRTM::SRTM(const std::string &path){
    srtmPath = path;
    currentFilename = "";
    heights =nullptr;
  }

  SRTM::~SRTM(){
    delete heights;
  }

  /**
   * generate SRTM3 filename like N43E006.hgt from integer part of latitude and longitude
   */
  const std::string& SRTM::srtmFilename(int patchLat, int patchLon){
    std::ostringstream fileName;
    if(patchLat>=0){
      fileName << "N";
    } else {
      fileName << "S";
      patchLat = std::abs(patchLat);
    }
    if(patchLat<10){
      fileName<<"0";
    }
    fileName << patchLat;
    if(patchLon>=0){
      fileName << "E";
    } else {
      fileName << "W";
      patchLon = std::abs(patchLon);
    }
    if(patchLon<10){
      fileName<<"0";
    }
    if(patchLon<100){
      fileName<<"0";
    }
    fileName << patchLon << ".hgt";

    return *(new std::string(fileName.str()));
  }

  /**
   * return the height at (latitude,longitude) or SRTM::nodata if no data at the location
   */
  int SRTM::heightAtLocation(double latitude, double longitude){
    int patchLat = int(floor(latitude));
    int patchLon = int(floor(longitude));
    std::string patchFilename = srtmFilename(patchLat, patchLon);
    if(currentFilename.empty() || currentFilename != patchFilename){
      std::istream::pos_type length;

      currentFilename = patchFilename;
      currentPatchLat = patchLat;
      currentPatchLon = patchLon;

      if(heights){
        delete heights;
        heights =nullptr;
      }

      if(currentFile.is_open()){
        currentFile.close();
      }
      currentFile.open((srtmPath+"/"+currentFilename).c_str(), std::ios::in|std::ios::binary|std::ios::ate);
      if(currentFile.std::ios::good()){
        length = currentFile.tellg();
        currentFile.seekg (0, std::ios::beg);
      } else {
        return SRTM::nodata;
      }
      if(length == (std::istream::pos_type)SRTM1_FILESIZE){
        rows = SRTM1_GRID;
        columns = SRTM1_GRID;
        patchSize = SRTM1_FILESIZE;
        log.Info() << "Open SRTM1 hgt file : "<<(srtmPath+"/"+currentFilename).c_str();
      } else if (length == (std::istream::pos_type)SRTM3_FILESIZE){
        rows = SRTM3_GRID;
        columns = SRTM3_GRID;
        patchSize = SRTM3_FILESIZE;
        log.Info() << "Open SRTM3 hgt file : "<<(srtmPath+"/"+currentFilename).c_str();
      } else {
        return SRTM::nodata;
      }
      if(!heights){
        heights = new unsigned char[SRTM::patchSize];
      }
      currentFile.read((char *)heights,SRTM::patchSize);
    }
    if(currentFile.std::ios::good()){
      double pixelSize = 1.0/(SRTM::rows-1);
      double fracLat = latitude>=0.0 ? 1 - latitude + floor(latitude) : ceil(latitude) - latitude;
      double fracLon = longitude>=0.0 ?  longitude - floor(longitude) : 1 - ceil(longitude) + longitude;
      int col = int(floor(fracLon*SRTM::columns));
      int col2 = col << 1;
      int row = int(floor(fracLat*SRTM::rows));

      int h1 = (heights[2*row*SRTM::columns+col2]<<8)+(heights[2*row*SRTM::columns+col2+1]);

#ifndef SRTM_BILINEAR_INTERPOLATION
      return (int)floor(h1);
#else
      int h2 = (heights[2*row*SRTM::columns+col2+2]<<8)+(heights[2*row*SRTM::columns+col2+3]);
      int h3 = (heights[2*(row+1)*SRTM::columns+col2]<<8)+(heights[2*(row+1)*SRTM::columns+col2+1]);
      int h4 = (heights[2*(row+1)*SRTM::columns+col2+2]<<8)+(heights[2*(row+1)*SRTM::columns+col2+3]);
      // bilinear interpolation
      double fLat = fracLat - fracLat*row/SRTM::rows;
      double fLon = fracLon - fracLon*pixelSize/SRTM::columns;
      log.Info() << "fLat=" << fLat << " fLon=" << fLon;

      double h = h1*(1-fLat)*(1-fLon) + h2*fLat*(1-fLon) + h3*(1-fLat)*fLon + h4*fLat*fLon;
      return (int)floor(h+0.5);
#endif
    } else {
      return SRTM::nodata;
    }
  }

}
