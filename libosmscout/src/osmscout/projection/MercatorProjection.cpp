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

#include <osmscout/projection/MercatorProjection.h>

#include <algorithm>

#include <osmscout/projection/Earth.h>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

#ifdef OSMSCOUT_HAVE_SSE2
#include <osmscout/system/SSEMath.h>
#endif

namespace osmscout {

  /*
   * For the calculations here see:
   * http://en.wikipedia.org/wiki/Mercator_projection
   * http://en.wikipedia.org/wiki/Web_Mercator
   * http://wiki.openstreetmap.org/wiki/Slippy_map_tilenames
   */

  //< DPI of a classical OSM tile
  static const double tileDPI=96.0;

  const double MercatorProjection::MaxLat = +85.0511;
  const double MercatorProjection::MinLat = -85.0511;
  const double MercatorProjection::MaxLon = +180.0;
  const double MercatorProjection::MinLon = -180.0;

  bool MercatorProjection::Set(const GeoCoord& coord,
                               double angle,
                               const Magnification& magnification,
                               double dpi,
                               size_t width, size_t height)
  {
    if (valid &&
        this->center==coord &&
        this->angle==angle &&
        this->magnification==magnification &&
        this->dpi==dpi &&
        this->width==width &&
        this->height==height) {
      return true;
    }
    if (!IsValidFor(coord)){
      return false;
    }

    valid=true;

    // Make a copy of the context information
    this->center=coord;
    this->angle=angle;
    this->magnification=magnification;
    this->dpi=dpi;
    this->width=width;
    this->height=height;

    if (angle!=0.0) {
      angleSin=std::sin(angle);
      angleCos=std::cos(angle);
      angleNegSin=-angleSin;
      angleNegCos=angleCos;
    }
    else {
      angleSin=0;
      angleNegSin=0;
      angleCos=1;
      angleNegCos=1;
    }

    // Width in meter of a tile of the given magnification at the equator
    double equatorTileWidth=Earth::extentMeter/magnification.GetMagnification();

    // Resolution (meter/pixel) of a pixel in a classical 256 pixel tile for the given zoom level at the equator
    double equatorTileResolution=equatorTileWidth/256.0;

    // Modified resolution (meter/pixel) at the equator based on our actual DPI instead of the standard tile DPI
    double equatorCorrectedEquatorTileResolution=equatorTileResolution*tileDPI/dpi;

    // Width of the visible area at the equator
    double groundWidthEquatorMeter=width*equatorCorrectedEquatorTileResolution;

    // Width of the visible area in meter
    double groundWidthVisibleMeter=groundWidthEquatorMeter*std::cos(center.GetLat()*gradtorad);

    // Resulting projection scale factor
    scale=width/(2*M_PI*groundWidthEquatorMeter/Earth::extentMeter);
    scaleGradtorad=scale*gradtorad;

    // Size of one pixel in meter
    pixelSize=groundWidthVisibleMeter/width;

    // How many pixel are one meter?
    meterInPixel=1.0/pixelSize;

    // 1 meter on the ground is how many millimeter on display?
    meterInMM=meterInPixel*25.4/dpi;

    // Absolute Y mercator coordinate for latitude
    latOffset=std::atanh(std::sin(coord.GetLat()*gradtorad));

    GeoCoord topLeft;

    PixelToGeo(0.0,0.0,topLeft);

    GeoCoord topRight;

    PixelToGeo((double)width,0.0,topRight);

    GeoCoord bottomLeft;

    PixelToGeo(0.0,(double)height,bottomLeft);

    GeoCoord bottomRight;

    PixelToGeo((double)width,(double)height,bottomRight);

    // evaluate bounding box, crop bounding box to valid Mercator area

    boundingBox=GeoBox(topLeft,bottomRight);
    boundingBox.CropTo(GeoBox(GeoCoord(MinLat,MinLon),
                              GeoCoord(MaxLat,MaxLon)));

    // derivation of "latToYPixel" function in projection center
    double latDeriv = 1.0 / std::sin( (2 * this->center.GetLat() * gradtorad + M_PI) /  2);
    scaledLatDeriv = latDeriv * gradtorad * scale;

    return true;
  }

  bool MercatorProjection::PixelToGeo(double x, double y,
                                      GeoCoord& coord) const
  {
    assert(valid);

    // Transform to center-based coordinate
    x-=width/2.0;
    y=height/2.0-y;

    if (angle!=0.0) {
      double xn=x*angleCos-y*angleSin;
      double yn=x*angleSin+y*angleCos;

      x=xn;
      y=yn;
    }

    // Transform to absolute geo coordinate
    coord.Set(std::atan(std::sinh(y/scale+latOffset))/gradtorad,
              this->center.GetLon()+x/scaleGradtorad);

    return IsValidFor(coord);
  }

  bool MercatorProjection::GeoToPixel(const GeoCoord& coord,
                                      Vertex2D& pixel) const
  {
    assert(valid);

    // Screen coordinate relative to center of image
    double x=(coord.GetLon()-this->center.GetLon())*scaleGradtorad;
    double y;

    if (useLinearInterpolation) {
      y=(coord.GetLat()-this->center.GetLat())*scaledLatDeriv;
    }
    else {
      // Mercator is defined just for latitude +-85.0511
      // For values outside this range is better to result projection border
      // than some invalid coordinate, like INFINITY
      double lat = std::min(std::max(coord.GetLat(), MinLat), MaxLat);
      y=(std::atanh(std::sin(lat*gradtorad))-latOffset)*scale;
    }

    if (angle!=0.0) {
      double xn=x*angleNegCos-y*angleNegSin;
      double yn=x*angleNegSin+y*angleNegCos;

      x=xn;
      y=yn;
    }

    // Transform to canvas coordinate
    y=height/2.0-y;
    x+=width/2.0;

    pixel=Vertex2D(x,y);

    return IsValidFor(coord);
  }

  void MercatorProjection::GeoToPixel(const BatchTransformer& /*transformData*/) const
  {
    assert(false); //should not be called
  }

  bool MercatorProjection::Move(double horizPixel,
                                double vertPixel)
  {
    Vertex2D pixel;

    GeoToPixel(center,
               pixel);

    GeoCoord coord;

    if (!PixelToGeo(pixel.GetX()+horizPixel,
                    pixel.GetY()-vertPixel,
                    coord)) {
      return false;
    }

    if (coord.GetLat()<-85.0511 || coord.GetLat()>85.0511) {
      return false;
    }

    if (coord.GetLon()<-180.0 || coord.GetLon()>180.0) {
      return false;
    }

    return Set(coord,
               angle,
               magnification,
               dpi,
               width,
               height);
  }
}
