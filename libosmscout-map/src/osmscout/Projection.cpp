/*
  This source is part of the libosmscout-map library
  Copyright (C) 2010  Tim Teulings

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

#include <osmscout/Projection.h>

#include <cassert>
#include <cmath>

//#include <iostream>

namespace osmscout {

  static const double gradtorad=2*M_PI/360;

  Projection::~Projection()
  {
    // no code
  }

  MercatorProjection::MercatorProjection()
  : valid(false)
  {
    Set(lon,lat,magnification,width,height);
  }

  bool MercatorProjection::Set(double lon, double lat,
                               double magnification,
                               size_t width, size_t height)
  {
    valid=true;
  
    if (this->lon==lon &&
        this->lat==lat &&
        this->magnification==magnification &&
        this->width==width &&
        this->height==height) {
      return true;
    }

    double boxWidth,boxHeight;

    //
    // calculation of bounds and scaling factors
    //

    // Make a copy of the context information
    this->lon=lon;
    this->lat=lat;
    this->width=width;
    this->height=height;
    this->magnification=magnification;


    boxWidth=360/magnification;
    boxHeight=boxWidth*height/width;

    lonMin=lon-boxWidth/2;
    lonMax=lon+boxWidth/2;

    latMin=atan(sinh(atanh(sin(lat*gradtorad))-boxHeight/2*gradtorad))/gradtorad;
    latMax=atan(sinh(atanh(sin(lat*gradtorad))+boxHeight/2*gradtorad))/gradtorad;

    //std::cout << "Dimension: " << lonMin << " " << latMin << " " << lonMax << " " << latMax << std::endl;

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
    
    return true;
  }
  
  bool MercatorProjection::GeoIsIn(double lon, double lat) const
  {
    assert(valid);
    
    return lon>=lonMin && lon<=lonMax && lat>=latMin && lat<=latMax;
  }
  
  bool MercatorProjection::GeoIsIn(double lonMin, double latMin,
                                   double lonMax, double latMax) const
  {
    assert(valid);
    
    return !(lonMin>this->lonMax ||
             lonMax<this->lonMin ||
             latMin>this->latMax ||
             latMax<this->latMin);
  }
  
  bool MercatorProjection::PixelToGeo(double x, double y,
                                      double& lon, double& lat) const
  {
    assert(valid);
  
    lon=lonMin+(lonMax-lonMin)*x/width;
    lat=latMin+(latMax-latMin)*y/height;

    return true;
  }

  bool MercatorProjection::GeoToPixel(double lon, double lat,
                                      double& x, double& y) const
  {
    assert(valid);
    
    x=(lon*gradtorad-hmin)*hscale;
    y=height-(atanh(sin(lat*gradtorad))-vmin)*vscale;

    return true;
  }
  
  bool MercatorProjection::GetDimensions(double& lonMin, double& latMin,
                                         double& lonMax, double& latMax) const
  {
    assert(valid);
    
    lonMin=this->lonMin;
    latMin=this->latMin;
    lonMax=this->lonMax;
    latMax=this->latMax;
    
    return true;
  }
  
  double MercatorProjection::GetPixelSize() const
  {
    assert(valid);
    
    return pixelSize;
  }
  
}

