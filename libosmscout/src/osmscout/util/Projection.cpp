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

  //< Radius of the earth in meter
  static const double earthRadiusMeter=6378137.0;
  //< Extent of the earth in meter
  static const double earthExtentMeter=2*M_PI*earthRadiusMeter;
  //< Width of a tile at the equator for zoom level 0 in meter (equal to extent of the earth at the equator
  static const double tileWidthZoom0Aquator=earthExtentMeter;
  //< DPI of a classical OSM tile
  static const double tileDPI=96.0;

#ifdef OSMSCOUT_HAVE_SSE2
  static const ALIGN16_BEG double sseGradtorad[] ALIGN16_END = {2*M_PI/360, 2*M_PI/360};
#endif

  static const double gradtorad=2*M_PI/360;

  Projection::Projection()
  : lon(0),
    lat(0),
    angle(0),
    magnification(0),
    dpi(0),
    width(0),
    height(0),
    lonMin(0.0),
    latMin(0.0),
    lonMax(0.0),
    latMax(0.0),
    pixelSize(0.0),
    meterInPixel(0.0),
    meterInMM(0.0)
  {
    // no code
  }

  Projection::~Projection()
  {
    // no code
  }

  MercatorProjectionOld::MercatorProjectionOld()
    : valid(false),
      latOffset(0.0),
      scale(1),
      scaleGradtorad(0)
  {
    // no code
  }

  bool MercatorProjectionOld::Set(double lon, double lat,
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
    double resolution=tileWidthZoom0Aquator/256.0*cos(lat*gradtorad)/magnification.GetMagnification();

    double groundWidthMeter=width*tileDPI/dpi*resolution;

    //
    // Calculation of bounds and scaling factors
    //
    // We have three projections:
    // * Mercator projection of longitude to X-coordinate
    // * Mercator projection of latitude to Y-coordinate
    // * Projection of X and Y coordinate as result of mercator projection to on screen coordinates
    //

    double boxWidth=360.0*groundWidthMeter/earthExtentMeter; // Part of the full earth circle that has to be shown, in degree

    scale=width/(gradtorad*boxWidth);
    scaleGradtorad = scale * gradtorad;

    pixelSize=groundWidthMeter/width;
    meterInPixel=1.0/pixelSize;
    meterInMM=meterInPixel*25.4/dpi;

    // Absolute Y mercator coordinate for latitude
    latOffset=atanh(sin(lat*gradtorad));

    double tlLat;
    double tlLon;

    PixelToGeo(0.0,(double)height,tlLon,tlLat);

    double trLat;
    double trLon;

    PixelToGeo((double)width,(double)height,trLon,trLat);

    double blLat;
    double blLon;

    PixelToGeo(0.0,0.0,blLon,blLat);

    double brLat;
    double brLon;

    PixelToGeo((double)width,0.0,brLon,brLat);

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

  bool MercatorProjectionOld::PixelToGeo(double x, double y,
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

  void MercatorProjectionOld::GeoToPixel(double lon, double lat,
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
  }

  void MercatorProjectionOld::GeoToPixel(const BatchTransformer& /*transformData*/) const
  {
    assert(false); //should not be called
  }

  bool MercatorProjectionOld::Move(double horizPixel,
                                   double vertPixel)
  {
    double x;
    double y;

    GeoToPixel(lon,lat,
               x,y);

    double lat;
    double lon;

    if (!PixelToGeo(x+horizPixel,
                    y-vertPixel,
                    lon,lat)) {
      return false;
    }

    if (lat <-85.0511 || lat>85.0511) {
      return false;
    }

    if (lon<-180.0 || lon>180.0) {
      return false;
    }

    return Set(lon,lat,
               angle,
               magnification,
               dpi,
               width,
               height);
  }

  MercatorProjection::MercatorProjection()
  : valid(false),
    latOffset(0.0),
    scale(1),
    scaleGradtorad(0),
    useLinearInterpolation(false)
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

    // Width in meter of a tile of the given magnification at the equator
    double equatorTileWidth=tileWidthZoom0Aquator/magnification.GetMagnification();

    // Resolution (meter/pixel) of a pixel in a classical 256 pixel tile for the given zoom level at the equator
    double equatorTileResolution=equatorTileWidth/256.0;

    // Modified resolution (meter/pixel) at the equator based on our actual DPI instead of the standard tile DPI
    double equatorCorrectedEquatorTileResolution=equatorTileResolution*tileDPI/dpi;

    // Width of the visible area at the equator
    double groundWidthEquatorMeter=width*equatorCorrectedEquatorTileResolution;

    // Resulting projection scale factor
    scale=width/(2*M_PI*groundWidthEquatorMeter/earthExtentMeter);
    scaleGradtorad=scale*gradtorad;

    // Width of the visible area in meter
    double groundWidthVisibleMeter=groundWidthEquatorMeter*cos(lat*gradtorad);

    // Size of one pixel in meter
    pixelSize=groundWidthVisibleMeter/width;

    // How many pixel are one meter?
    meterInPixel=1.0/pixelSize;

    // 1 meter on the ground is how many millimeter on display?
    meterInMM=meterInPixel*25.4/dpi;

    // Absolute Y mercator coordinate for latitude
    latOffset=atanh(sin(lat*gradtorad));

    //std::cout << "Pixel size " << pixelSize << " meterInPixel " << meterInPixel << " meterInMM " << meterInMM << std::endl;

    double tlLat;
    double tlLon;

    PixelToGeo(0.0,(double)height,tlLon,tlLat);

    double trLat;
    double trLon;

    PixelToGeo((double)width,(double)height,trLon,trLat);

    double blLat;
    double blLon;

    PixelToGeo(0.0,0.0,blLon,blLat);

    double brLat;
    double brLon;

    PixelToGeo((double)width,0.0,brLon,brLat);

    latMin=std::min(std::min(tlLat,trLat),std::min(blLat,brLat));
    latMax=std::max(std::max(tlLat,trLat),std::max(blLat,brLat));

    lonMin=std::min(std::min(tlLon,trLon),std::min(blLon,brLon));
    lonMax=std::max(std::max(tlLon,trLon),std::max(blLon,brLon));

    // derivation of "latToYPixel" function in projection center
    double latDeriv = 1.0 / sin( (2 * this->lat * gradtorad + M_PI) /  2);
    scaledLatDeriv = latDeriv * gradtorad * scale;

    /*
    std::cout << "Center: " << lat << "° lat " << lon << "° lon" << std::endl;
    std::cout << "Magnification: " << magnification.GetMagnification() << "/" << magnification.GetLevel() << std::endl;
    std::cout << "Screen dimension: " << width << "x" << height << " " << dpi << " DPI " << std::endl;

    std::cout << "Box: " << latMin << "° - " << latMax << "° lat x " << lonMin << "° -" << lonMax << "° lon, " << groundWidthVisibleMeter << " " << std::endl;

    std::cout << "Scale: 1 : " << scale << std::endl;*/

    return true;
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

  void MercatorProjection::GeoToPixel(double lon, double lat,
                                      double& x, double& y) const
  {
    assert(valid);

    // Screen coordinate relative to center of image
    x=(lon-this->lon)*scaleGradtorad;

    if (!useLinearInterpolation){
      y = (atanh(sin(lat*gradtorad))-latOffset)*scale;
    }else{
      y = (lat - this->lat) * scaledLatDeriv;
    }

    if (angle!=0.0) {
      double xn=x*angleNegCos-y*angleNegSin;
      double yn=x*angleNegSin+y*angleNegCos;

      x=xn;
      y=yn;
    }

    // Transform to canvas coordinate
    y=height/2-y;
    x+=width/2;
  }

  void MercatorProjection::GeoToPixel(const BatchTransformer& /*transformData*/) const
  {
    assert(false); //should not be called
  }

  bool MercatorProjection::Move(double horizPixel,
                                double vertPixel)
  {
    double x;
    double y;

    GeoToPixel(lon,lat,
               x,y);

    double lat;
    double lon;

    if (!PixelToGeo(x+horizPixel,
                    y-vertPixel,
                    lon,lat)) {
      return false;
    }

    if (lat <-85.0511 || lat>85.0511) {
      return false;
    }

    if (lon<-180.0 || lon>180.0) {
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
    lonOffset(0.0),
    latOffset(0.0),
    scale(1),
    scaleGradtorad(0),
    useLinearInterpolation(false)
  {
    // no code
  }

  bool TileProjection::SetInternal(double lonMin,double latMin,
                                   double lonMax,double latMax,
                                   const Magnification& magnification,
                                   double dpi,
                                   size_t width,size_t height)
  {
    if (valid &&
        this->latMin==latMin &&
        this->latMax==latMax &&
        this->lonMin==lonMin &&
        this->lonMax==lonMax &&
        this->magnification==magnification &&
        this->dpi==dpi &&
        this->width==width &&
        this->height==height) {
      return true;
    }

    valid=true;

    // Make a copy of the context information
    this->magnification=magnification;
    this->dpi=dpi;
    this->width=width;
    this->height=height;

    this->latMin=latMin;
    this->latMax=latMax;
    this->lonMin=lonMin;
    this->lonMax=lonMax;

    lat=(latMin+latMax)/2;
    lon=(lonMin+lonMax)/2;

    scale=width/(gradtorad*(lonMax-lonMin));
    scaleGradtorad = scale * gradtorad;

    lonOffset=lonMin*scaleGradtorad;
    latOffset=scale*atanh(sin(latMin*gradtorad));

    pixelSize=earthExtentMeter/magnification.GetMagnification()/width;
    meterInPixel=1/pixelSize;
    meterInMM=meterInPixel*25.4/pixelSize;

    // derivation of "latToYPixel" function in projection center
    double latDeriv = 1.0 / sin( (2 * this->lat * gradtorad + M_PI) /  2);
    scaledLatDeriv = latDeriv * gradtorad * scale;

#ifdef OSMSCOUT_HAVE_SSE2
    sse2LonOffset      = _mm_set1_pd(lonOffset);
    sse2LatOffset      = _mm_set1_pd(latOffset);
    sse2Scale          = _mm_set1_pd(scale);
    sse2ScaleGradtorad = _mm_set1_pd(scaleGradtorad);
    sse2Height         = _mm_set1_pd(height);
#endif

    return true;
}

  bool TileProjection::Set(size_t tileX, size_t tileY,
                           const Magnification& magnification,
                           double dpi,
                           size_t width, size_t height)
  {
    double latMin=TileYToLat(tileY+1,
                      magnification);
    double latMax=TileYToLat(tileY,
                      magnification);

    double lonMin=TileXToLon(tileX,
                      magnification);
    double lonMax=TileXToLon(tileX+1,
                      magnification);

    return SetInternal(lonMin,latMin,lonMax,latMax,magnification,dpi,width,height);
  }

  bool TileProjection::Set(size_t tileAX, size_t tileAY,
                           size_t tileBX, size_t tileBY,
                           const Magnification& magnification,
                           double dpi,
                           size_t width,size_t height)
  {
    double latMin=TileYToLat(std::max(tileAY,tileBY)+1,
                             magnification);
    double latMax=TileYToLat(std::min(tileAY,tileBY),
                             magnification);

    double lonMin=TileXToLon(std::min(tileAX,tileBX),
                             magnification);
    double lonMax=TileXToLon(std::max(tileAX,tileBX)+1,
                             magnification);

    return SetInternal(lonMin,latMin,lonMax,latMax,magnification,dpi,width,height);
  }

  bool TileProjection::PixelToGeo(double x, double y,
                                  double& lon, double& lat) const
  {
    lon=(x+lonOffset)/(scale*gradtorad);
    lat=atan(sinh((height-y+latOffset)/scale))/gradtorad;

    return true;
  }

  #ifdef OSMSCOUT_HAVE_SSE2

    void TileProjection::GeoToPixel(double lon, double lat,
                                    double& x, double& y) const
    {
      x=lon*scaleGradtorad-lonOffset;
      y=height-(scale*atanh_sin_pd(lat*gradtorad)-latOffset);
    }

    //this basically transforms 2 coordinates in 1 call
    void TileProjection::GeoToPixel(const BatchTransformer& transformData) const
    {
      v2df x = _mm_sub_pd(_mm_mul_pd( ARRAY2V2DF(transformData.lon), sse2ScaleGradtorad), sse2LonOffset);
      __m128d test = ARRAY2V2DF(transformData.lat);
      v2df y = _mm_sub_pd(sse2Height, _mm_sub_pd(_mm_mul_pd(sse2Scale, atanh_sin_pd( _mm_mul_pd( test,  ARRAY2V2DF(sseGradtorad)))), sse2LatOffset));

      //store results:
      _mm_storel_pd (transformData.xPointer[0], x);
      _mm_storeh_pd (transformData.xPointer[1], x);
      _mm_storel_pd (transformData.yPointer[0], y);
      _mm_storeh_pd (transformData.yPointer[1], y);
    }

  #else

    void TileProjection::GeoToPixel(double lon, double lat,
                                    double& x, double& y) const
    {
      x=lon*scaleGradtorad-lonOffset;

      if (!useLinearInterpolation){
          y=height-(scale*atanh(sin(lat*gradtorad))-latOffset);
      }else{
          y = (height / 2) - ((lat - this->lat) * scaledLatDeriv);
      }
    }

    void TileProjection::GeoToPixel(const BatchTransformer& /*transformData*/) const
    {
      assert(false); //should not be called
    }

  #endif

}
