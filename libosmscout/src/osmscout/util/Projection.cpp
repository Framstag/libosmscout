/*
  This source is part of the libosmscout library
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

#include <osmscout/util/Projection.h>

#include <cassert>

#include <osmscout/private/Math.h>
#include <iostream>
namespace osmscout {

  static const double gradtorad=2*M_PI/360;

  Projection::~Projection()
  {
    // no code
  }

  MercatorProjection::MercatorProjection()
  : valid(false),
    lon(0),
    lat(0),
    magnification(1),
    width(0),
    height(0)
  {
    Set(lon,lat,magnification,width,height);
  }

  bool MercatorProjection::Set(double lon, double lat,
                               double magnification,
                               size_t width, size_t height)
  {
    if (valid &&
        this->lon==lon &&
        this->lat==lat &&
        this->magnification==magnification &&
        this->width==width &&
        this->height==height) {
      return true;
    }

    valid=true;

    // Make a copy of the context information
    this->lon=lon;
    this->lat=lat;
    this->width=width;
    this->height=height;
    this->magnification=magnification;

    //
    // Calculation of bounds and scaling factors
    //
    // We have three projections:
    // * Mercator projection of longitudes to X-coordinates
    // * Mercator projections of latitudes to Y-coordinates
    // * Projection of X and Y coordinates as result of mercator projection to on screen cooridnates
    //

    double boxWidth=360/magnification; // Part of the full earth circle that has to be shown, in degree

    // longitude does scale linear, so left and right longitude borders is easy to calculate
    lonMin=lon-boxWidth/2;
    lonMax=lon+boxWidth/2;

    scale=(width-1)/(gradtorad*(lonMax-lonMin));

    // Width of an pixel in meter
    double d=(lonMax-lonMin)*gradtorad;

    pixelSize=d*180*60/M_PI*1852.216/width;

    // Absolute Y mercator coordinate for latitude
    double y=atanh(sin(lat*gradtorad));

    latMin=atan(sinh(y-(height/2)/scale))/gradtorad;
    latMax=atan(sinh(y+(height/2)/scale))/gradtorad;

    lonOffset=lonMin*scale*gradtorad;
    latOffset=scale*atanh(sin(latMin*gradtorad));

    /*
    std::cout << "Box (grad) h: " << lonMin << "-" << lonMax << " v: " << latMin <<"-" << latMax << std::endl;
    std::cout << "Center (grad):" << this->lat << "x" << this->lon << std::endl;
    std::cout << "Magnification: " << magnification << std::endl;
    std::cout << "Scale: " << scale << std::endl;
    std::cout << "Screen dimension: " << width << "x" << height << std::endl;
    std::cout << "d: " << d << " " << d*180*60/M_PI << std::endl;
    std::cout << "The complete screen are " << d*180*60/M_PI*1852.216 << " meters" << std::endl;
    std::cout << "1 pixel are " << pixelSize << " meters" << std::endl;
    std::cout << "20 meters are " << 20/(d*180*60/M_PI*1852.216/width) << " pixels" << std::endl;*/
    
    return true;
  }

  bool MercatorProjection::Set(double lonMin, double latMin,
                               double lonMax, double latMax,
                               double magnification,
                               size_t width)
  {
    if (valid &&
        this->lonMin==lonMin &&
        this->lonMax==lonMax &&
        this->latMin==latMin &&
        this->latMax==latMax &&
        this->magnification==magnification &&
        this->width==width) {
      return true;
    }

    valid=true;

    this->lonMin=lonMin;
    this->lonMax=lonMax;
    this->latMin=latMin;
    this->latMax=latMax;
    this->magnification=magnification;
    this->width=width;

    // Make a copy of the context information
    this->lon=(lonMin+lonMax)/2;
    this->lat=atan(sinh((atanh(sin(latMax*gradtorad))+atanh(sin(latMin*gradtorad)))/2))/gradtorad;

    scale=(width-1)/(gradtorad*(lonMax-lonMin));

    // Width of an pixel in meter
    double d=(lonMax-lonMin)*gradtorad;

    pixelSize=d*180*60/M_PI*1852.216/width;

    this->height=(atanh(sin(latMax*gradtorad))-atanh(sin(latMin*gradtorad)))*scale;

    lonOffset=lonMin*scale*gradtorad;
    latOffset=scale*atanh(sin(latMin*gradtorad));

    /*
    std::cout << "Box (grad) h: " << lonMin << "-" << lonMax << " v: " << latMin <<"-" << latMax << std::endl;
    std::cout << "Center (grad):" << this->lat << "x" << this->lon << std::endl;
    std::cout << "Magnification: " << magnification << std::endl;
    std::cout << "Scale: " << scale << std::endl;
    std::cout << "Screen dimension: " << width << "x" << height << std::endl;
    std::cout << "d: " << d << " " << d*180*60/M_PI << std::endl;
    std::cout << "The complete screen are " << d*180*60/M_PI*1852.216 << " meters" << std::endl;
    std::cout << "1 pixel are " << pixelSize << " meters" << std::endl;
    std::cout << "20 meters are " << 20/(d*180*60/M_PI*1852.216/width) << " pixels" << std::endl;*/

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
  
    lon=(x+lonOffset)/(scale*gradtorad);
    lat=atan(sinh((y-height+latOffset)/scale))/gradtorad;

    return true;
  }

  bool MercatorProjection::GeoToPixel(double lon, double lat,
                                      double& x, double& y) const
  {
    assert(valid);
    
    x=lon*scale*gradtorad-lonOffset;
    y=height-(scale*atanh(sin(lat*gradtorad))-latOffset);

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
