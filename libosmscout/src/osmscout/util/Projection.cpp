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

#include <algorithm>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

#ifdef OSMSCOUT_HAVE_SSE2
#include <osmscout/system/SSEMath.h>
#endif

#include <osmscout/util/Tiling.h>

#include <iostream>
namespace osmscout {

  /*
   * For the calculations here see:
   * http://en.wikipedia.org/wiki/Mercator_projection
   * http://en.wikipedia.org/wiki/Web_Mercator
   * http://wiki.openstreetmap.org/wiki/Slippy_map_tilenames
   */

  static const double earthRadius=6378137.0; // Meter
  static const double earthExtent=2*M_PI*earthRadius;// Meter
  static const double tileSizeZoom0Aquator=earthExtent;
  static const double tileDPI=96.0;

#ifdef OSMSCOUT_HAVE_SSE2
  static const ALIGN16_BEG double sseGradtorad[] ALIGN16_END = {2*M_PI/360, 2*M_PI/360};
#endif

  static const double gradtorad=2*M_PI/360;

  Projection::~Projection()
  {
    // no code
  }

  MercatorProjection::MercatorProjection()
  : valid(false),
    lon(0),
    lat(0),
    angle(0),
    magnification(0),
    dpi(96),
    width(256),
    height(256),
    lonMin(0.0),
    latMin(0.0),
    lonMax(0.0),
    latMax(0.0),
    latOffset(0.0),
    scale(1),
    scaleGradtorad(0),
    pixelSize(1)
  {
    // no code
  }

  bool MercatorProjection::Set(double lon, double lat,
                                double angle,
                                const Magnification& magnification,
                                double dpi,
                                size_t width, size_t height)
  {
    if (valid &&
        this->lon==lon &&
        this->lat==lat &&
        this->angle==angle &&
        this->magnification==magnification &&
        this->dpi==dpi &&
        this->width==width &&
        this->height==height) {
      return true;
    }

    valid=true;

    // Make a copy of the context information
    this->lon=lon;
    this->lat=lat;
    this->angle=angle;
    this->magnification=magnification;
    this->dpi=dpi;
    this->width=width;
    this->height=height;

    if (angle!=0.0) {
      angleSin=sin(angle);
      angleCos=cos(angle);
      angleNegSin=sin(-angle);
      angleNegCos=cos(-angle); // TODO: Optimize because of symetry
    }
    else {
      angleSin=0;
      angleNegSin=0;
      angleCos=1;
      angleNegCos=-1;
    }

    // Resolution (meter/pixel) of a pixel in a classical 256 pixel tile for the given zoom level
    double resolution=tileSizeZoom0Aquator/256*cos(lat*gradtorad)/magnification.GetMagnification();

    double groundWidthMeter=width*tileDPI/dpi*resolution;

    //
    // Calculation of bounds and scaling factors
    //
    // We have three projections:
    // * Mercator projection of longitude to X-coordinate
    // * Mercator projection of latitude to Y-coordinate
    // * Projection of X and Y coordinate as result of mercator projection to on screen coordinates
    //

    double boxWidth=360.0*groundWidthMeter/earthExtent; // Part of the full earth circle that has to be shown, in degree

    scale=width/(gradtorad*boxWidth);
    scaleGradtorad = scale * gradtorad;

    pixelSize=groundWidthMeter/width;

    // Absolute Y mercator coordinate for latitude
    latOffset=atanh(sin(lat*gradtorad));

    double tlLat;
    double tlLon;

    PixelToGeo(0,height,tlLon,tlLat);

    double trLat;
    double trLon;

    PixelToGeo(width,height,trLon,trLat);

    double blLat;
    double blLon;

    PixelToGeo(0,0,blLon,blLat);

    double brLat;
    double brLon;

    PixelToGeo(width,0,brLon,brLat);

    latMin=std::min(std::min(tlLat,trLat),std::min(blLat,brLat));
    latMax=std::max(std::max(tlLat,trLat),std::max(blLat,brLat));

    lonMin=std::min(std::min(tlLon,trLon),std::min(blLon,brLon));
    lonMax=std::max(std::max(tlLon,trLon),std::max(blLon,brLon));

    /*
    std::cout << "Center: " << lat << "° lat " << lon << "° lon" << std::endl;
    std::cout << "Magnification: " << magnification.GetMagnification() << "/" << magnification.GetLevel() << std::endl;
    std::cout << "Screen dimension: " << width << "x" << height << " " << dpi << " DPI " << std::endl;

    std::cout << "Box: " << latMin << "° - " << latMax << "° lat x " << lonMin << "° -" << lonMax << "° lon, " << groundWidthMeter << " " << std::endl;

    std::cout << "Scale: 1 : " << scale << std::endl;*/

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

    // Transform to center-based coordinate
    x-=width/2;
    y=height/2-y;

    if (angle!=0.0) {
      double xn=x*angleCos-y*angleSin;
      double yn=x*angleSin+y*angleCos;

      x=xn;
      y=yn;
    }

    // Transform to absolute geo coordinate
    lon=this->lon+x/scaleGradtorad;
    lat=atan(sinh(y/scale+latOffset))/gradtorad;

    return true;
  }

  bool MercatorProjection::GeoToPixel(double lon, double lat,
                                      double& x, double& y) const
  {
    assert(valid);

    // Screen coordinate relative to center of image
    x=(lon-this->lon)*scaleGradtorad;
    y=(atanh(sin(lat*gradtorad))-latOffset)*scale;

    if (angle!=0.0) {
      double xn=x*angleNegCos-y*angleNegSin;
      double yn=x*angleNegSin+y*angleNegCos;

      x=xn;
      y=yn;
    }

    // Transform to canvas coordinate
    y=height/2-y;
    x+=width/2;

    return true;
  }

  bool MercatorProjection::GeoToPixel(const GeoCoord& coord,
                                      double& x, double& y) const
  {
    assert(valid);

    // Screen coordinate relative to center of image
    x=(coord.GetLon()-this->lon)*scaleGradtorad;
    y=(atanh(sin(coord.GetLat()*gradtorad))-latOffset)*scale;

    if (angle!=0.0) {
      double xn=x*angleNegCos-y*angleNegSin;
      double yn=x*angleNegSin+y*angleNegCos;

      x=xn;
      y=yn;
    }

    // Transform to canvas coordinate
    y=height/2-y;
    x+=width/2;

    return true;
  }

  bool MercatorProjection::GeoToPixel(const BatchTransformer& /*transformData*/) const
  {
    assert(false); //should not be called
    return false;
  }

  bool MercatorProjection::GetDimensions(GeoBox& boundingBox) const
  {
    assert(valid);

    boundingBox.Set(GeoCoord(latMin,lonMin),
                    GeoCoord(latMax,lonMax));

    return true;
  }

  double MercatorProjection::GetPixelSize() const
  {
    assert(valid);

    return pixelSize;
  }

  bool MercatorProjection::Move(double horizPixel,
                                 double vertPixel)
  {
    double x;
    double y;

    if (!GeoToPixel(GetLon(),GetLat(),
                    x,y)) {
      return false;
    }

    double lat;
    double lon;

    if (!PixelToGeo(x+horizPixel,
                    y-vertPixel,
                    lon,lat)) {
      return false;
    }

    return Set(lon,lat,
               angle,
               magnification,
               dpi,
               width,
               height);
  }

  TileProjection::TileProjection()
  : valid(false),
    tileX(0),
    tileY(0),
    magnification(0),
    dpi(96),
    width(256),
    height(256),
    lonMin(0.0),
    latMin(0.0),
    lonMax(0.0),
    latMax(0.0),
    lonOffset(0.0),
    latOffset(0.0),
    scale(1),
    scaleGradtorad(0),
    pixelSize(1)
  {
    // no code
  }

  bool TileProjection::Set(size_t tileX, size_t tileY,
                           const Magnification& magnification,
                           double dpi,
                           size_t width, size_t height)
  {
    if (valid &&
        this->tileX==tileY &&
        this->tileY==tileY &&
        this->magnification==magnification &&
        this->dpi==dpi &&
        this->width==width &&
        this->height==height) {
      return true;
    }

    valid=true;

    // Make a copy of the context information
    this->tileX=tileX;
    this->tileY=tileY;
    this->magnification=magnification;
    this->dpi=dpi;
    this->width=width;
    this->height=height;

    latMin=TileYToLat(tileY+1,
                      magnification);
    latMax=TileYToLat(tileY,
                      magnification);

    lonMin=TileXToLon(tileX,
                      magnification);
    lonMax=TileXToLon(tileX+1,
                      magnification);

    lat=(latMin+latMax)/2;
    lon=(lonMin+lonMax)/2;

    scale=width/(gradtorad*(lonMax-lonMin));
    scaleGradtorad = scale * gradtorad;

    lonOffset=lonMin*scaleGradtorad;
    latOffset=scale*atanh(sin(latMin*gradtorad));

    pixelSize=earthExtent/magnification.GetMagnification()/256;

#ifdef OSMSCOUT_HAVE_SSE2
    sse2LonOffset      = _mm_set1_pd(lonOffset);
    sse2LatOffset      = _mm_set1_pd(latOffset);
    sse2Scale          = _mm_set1_pd(scale);
    sse2ScaleGradtorad = _mm_set1_pd(scaleGradtorad);
    sse2Height         = _mm_set1_pd(height);
#endif

    return true;
  }

  bool TileProjection::GeoIsIn(double lon, double lat) const
  {
    assert(valid);

    return lon>=lonMin && lon<=lonMax && lat>=latMin && lat<=latMax;
  }

  bool TileProjection::GeoIsIn(double lonMin, double latMin,
                               double lonMax, double latMax) const
  {
    assert(valid);

    return !(lonMin>this->lonMax ||
             lonMax<this->lonMin ||
             latMin>this->latMax ||
             latMax<this->latMin);
  }

  bool TileProjection::PixelToGeo(double x, double y,
                                  double& lon, double& lat) const
  {
    assert(valid);

    lon=(x+lonOffset)/(scale*gradtorad);
    lat=atan(sinh((height-y+latOffset)/scale))/gradtorad;

    return true;
  }

  #ifdef OSMSCOUT_HAVE_SSE2

    bool TileProjection::GeoToPixel(double lon, double lat,
                                    double& x, double& y) const
    {
      assert(valid);

      x=lon*scaleGradtorad-lonOffset;
      y=height-(scale*atanh_sin_pd(lat*gradtorad)-latOffset);

      return true;
    }

    bool TileProjection::GeoToPixel(const GeoCoord& coord,
                                    double& x, double& y) const
    {
      assert(valid);

      x=coord.GetLon()*scaleGradtorad-lonOffset;
      y=height-(scale*atanh_sin_pd(coord.GetLat()*gradtorad)-latOffset);

      return true;
    }

    //this basically transforms 2 coordinates in 1 call
    bool TileProjection::GeoToPixel(const BatchTransformer& transformData) const
    {
      v2df x = _mm_sub_pd(_mm_mul_pd( ARRAY2V2DF(transformData.lon), sse2ScaleGradtorad), sse2LonOffset);
      __m128d test = ARRAY2V2DF(transformData.lat);
      v2df y = _mm_sub_pd(sse2Height, _mm_sub_pd(_mm_mul_pd(sse2Scale, atanh_sin_pd( _mm_mul_pd( test,  ARRAY2V2DF(sseGradtorad)))), sse2LatOffset));

      //store results:
      _mm_storel_pd (transformData.xPointer[0], x);
      _mm_storeh_pd (transformData.xPointer[1], x);
      _mm_storel_pd (transformData.yPointer[0], y);
      _mm_storeh_pd (transformData.yPointer[1], y);

      return true;
    }

  #else

    bool TileProjection::GeoToPixel(double lon, double lat,
                                    double& x, double& y) const
    {
      assert(valid);

      x=lon*scaleGradtorad-lonOffset;
      y=height-(scale*atanh(sin(lat*gradtorad))-latOffset);

      return true;
    }

    bool TileProjection::GeoToPixel(const GeoCoord& coord,
                                    double& x, double& y) const
    {
      assert(valid);

      x=coord.GetLon()*scaleGradtorad-lonOffset;
      y=height-(scale*atanh(sin(coord.GetLat()*gradtorad))-latOffset);

      return true;
    }

    bool TileProjection::GeoToPixel(const BatchTransformer& /*transformData*/) const
    {
      assert(false); //should not be called
      return false;
    }

  #endif

  bool TileProjection::GetDimensions(GeoBox& boundingBox) const
  {
    assert(valid);

    boundingBox.Set(GeoCoord(latMin,lonMin),
                    GeoCoord(latMax,lonMax));

    return true;
  }

  double TileProjection::GetPixelSize() const
  {
    assert(valid);

    return pixelSize;
  }
}
