/*
  This source is part of the libosmscout-map library
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

#include <osmscout/MapPainter.h>

namespace osmscout {

  static const double gradtorad=2*M_PI/360;

  MapPainter::MapPainter(const Database& database)
   : database(database)
  {
    // no code
  }

  MapPainter::~MapPainter()
  {
    // no code
  }
  
  void MapPainter::RecalculateData(double lon, double lat,
                                   double magnification,
                                   size_t width, size_t height)
  {
    double gradtorad=2*M_PI/360;

    //
    // calculation of bounds and scaling factors
    //

    // Make a copy of the context information
    this->lon=lon;
    this->lat=lat;
    this->width=width;
    this->height=height;
    this->magnification=magnification;

    // Get bounding dimensions and copy them to the context information, too
    GetDimensions(lon,lat,magnification,width,height,lonMin,latMin,lonMax,latMax);

    std::cout << "Dimension: " << lonMin << " " << latMin << " " << lonMax << " " << latMax << std::endl;

    hmin=lonMin*gradtorad;
    hmax=lonMax*gradtorad;
    vmin=atanh(sin(latMin*gradtorad));
    vmax=atanh(sin(latMax*gradtorad));

    hscale=(width-1)/(hmax-hmin);
    vscale=(height-1)/(vmax-vmin);

    // Width of an pixel in meter
    double d=(lonMax-lonMin)*gradtorad;

    pixelSize=d*180*60/M_PI*1852.216/width;

    /*
    std::cout << "Box (grad) h: " << lonMin << "-" << lonMax << " v: " << latMin <<"-" << latMax << std::endl;
    std::cout << "Box (merc) h: " << hmin << "-" << hmax << " v: " << vmin <<"-" << vmax << std::endl;
    std::cout << "hscale: " << hscale << " vscale: " << vscale << std::endl;
    std::cout << "d: " << d << " " << d*180*60/M_PI << std::endl;
    std::cout << "The complete screen are " << d*180*60/M_PI*1852.216 << " meters" << std::endl;
    std::cout << "1 pixel are " << pixelSize << " meters" << std::endl;
    std::cout << "20 meters are " << 20/(d*180*60/M_PI*1852.216/width) << " pixels" << std::endl;
    */
  }
  
  bool MapPainter::TransformPixelToGeo(double x, double y,
                                       double& lon, double& lat)
  {
    lon=lonMin+(lonMax-lonMin)*x/width;
    lat=latMin+(latMax-latMin)*y/height;

    return true;
  }

  bool MapPainter::TransformGeoToPixel(double lon, double lat,
                                       double& x, double& y)
  {
    x=(lon*gradtorad-hmin)*hscale;
    y=height-(atanh(sin(lat*gradtorad))-vmin)*vscale;

    return true;
  }
  
  void MapPainter::GetDimensions(double lon, double lat,
                                 double magnification,
                                 size_t width, size_t height,
                                 double& lonMin, double& latMin,
                                 double& lonMax, double& latMax)
  {
    double boxWidth,boxHeight;

    boxWidth=360/magnification;
    boxHeight=boxWidth*height/width;

    lonMin=lon-boxWidth/2;
    lonMax=lon+boxWidth/2;

    latMin=atan(sinh(atanh(sin(lat*gradtorad))-boxHeight/2*gradtorad))/gradtorad;
    latMax=atan(sinh(atanh(sin(lat*gradtorad))+boxHeight/2*gradtorad))/gradtorad;
  }

  bool MapPainter::TransformPixelToGeo(int x, int y,
                                       double centerLon, double centerLat,
                                       double magnification,
                                       size_t width, size_t height,
                                       double& outLon, double& outLat)
  {
    double lonMin,latMin,lonMax,latMax;

    GetDimensions(centerLon,centerLat,
                  magnification,
                  width,height,
                  lonMin,latMin,lonMax,latMax);

    outLon=lonMin+(lonMax-lonMin)*x/width;

    // This transformation is currently only valid for big magnifications
    // since it does not take the mercator (back-)transformation into account!
    outLat=latMin+(latMax-latMin)*y/height;

    return true;
  }

  bool MapPainter::TransformGeoToPixel(double lon, double lat,
                                       double centerLon, double centerLat,
                                       double magnification,
                                       size_t width, size_t height,
                                       double &x, double& y)
  {
    double lonMin,latMin,lonMax,latMax,hmin,hmax,vmin,vmax,hscale,vscale;

    GetDimensions(centerLon,centerLat,
                  magnification,
                  width,height,
                  lonMin,latMin,lonMax,latMax);

    hmin=lonMin*gradtorad;
    hmax=lonMax*gradtorad;
    vmin=atanh(sin(latMin*gradtorad));
    vmax=atanh(sin(latMax*gradtorad));

    hscale=(width-1)/(hmax-hmin);
    vscale=(height-1)/(vmax-vmin);

    x=(lon*gradtorad-hmin)*hscale;
    y=height-(atanh(sin(lat*gradtorad))-vmin)*vscale;

    return true;
  }
}

