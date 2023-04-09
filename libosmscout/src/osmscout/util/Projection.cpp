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

namespace osmscout {

  /*
   * For the calculations here see:
   * http://en.wikipedia.org/wiki/Mercator_projection
   * http://en.wikipedia.org/wiki/Web_Mercator
   * http://wiki.openstreetmap.org/wiki/Slippy_map_tilenames
   */

  //< Radius of the earth in meter
  static const double earthRadiusMeter=6'378'137.0;
  //< Extent of the earth in meter
  static const double earthExtentMeter=2*M_PI*earthRadiusMeter;
  //< Width of a tile at the equator for zoom level 0 in meter (equal to extent of the earth at the equator
  static const double tileWidthZoom0Aquator=earthExtentMeter;
  //< DPI of a classical OSM tile
  static const double tileDPI=96.0;

  const double MercatorProjection::MaxLat = +85.0511;
  const double MercatorProjection::MinLat = -85.0511;
  const double MercatorProjection::MaxLon = +180.0;
  const double MercatorProjection::MinLon = -180.0;

#ifdef OSMSCOUT_HAVE_SSE2
  static const ALIGN16_BEG double sseGradtorad[] ALIGN16_END = {2*M_PI/360, 2*M_PI/360};
#endif

  static const double gradtorad=2*M_PI/360;

  bool Projection::BoundingBoxToPixel(const GeoBox& boundingBox,
                                      double& xMin,
                                      double& yMin,
                                      double& xMax,
                                      double& yMax) const
  {
    assert(boundingBox.IsValid());

    Vertex2D pixel;

    if (!GeoToPixel(boundingBox.GetMinCoord(),
                    pixel)) {
      return false;
    }

    xMin=pixel.GetX();
    xMax=pixel.GetX();
    yMin=pixel.GetY();
    yMax=pixel.GetY();

    if (!GeoToPixel(boundingBox.GetMaxCoord(),
                    pixel)) {
      return false;
    }

    xMin=std::min(xMin,
                  pixel.GetX());
    xMax=std::max(xMax,
                  pixel.GetX());
    yMin=std::min(yMin,
                  pixel.GetY());
    yMax=std::max(yMax,
                  pixel.GetY());

    if (!GeoToPixel(GeoCoord(boundingBox.GetMinLat(),
                             boundingBox.GetMaxLon()),
                    pixel)) {
      return false;
    }

    xMin=std::min(xMin,
                  pixel.GetX());
    xMax=std::max(xMax,
                  pixel.GetX());
    yMin=std::min(yMin,
                  pixel.GetY());
    yMax=std::max(yMax,
                  pixel.GetY());

    if (!GeoToPixel(GeoCoord(boundingBox.GetMaxLat(),
                             boundingBox.GetMinLon()),
                    pixel)) {
      return false;
    }

    xMin=std::min(xMin,
                  pixel.GetX());
    xMax=std::max(xMax,
                  pixel.GetX());
    yMin=std::min(yMin,
                  pixel.GetY());
    yMax=std::max(yMax,
                  pixel.GetY());

    return true;
  }

  bool Projection::BoundingBoxToPixel(const GeoBox& boundingBox,
                                      ScreenBox& screenBox) const
  {
    assert(boundingBox.IsValid());

    Vertex2D pixel;

    if (!GeoToPixel(boundingBox.GetMinCoord(),
                    pixel)) {
      return false;
    }

    double xMin=pixel.GetX();
    double xMax=pixel.GetX();
    double yMin=pixel.GetY();
    double yMax=pixel.GetY();

    if (!GeoToPixel(boundingBox.GetMaxCoord(),
                    pixel)) {
      return false;
    }

    xMin=std::min(xMin,
                  pixel.GetX());
    xMax=std::max(xMax,
                  pixel.GetX());
    yMin=std::min(yMin,
                  pixel.GetY());
    yMax=std::max(yMax,
                  pixel.GetY());

    if (!GeoToPixel(GeoCoord(boundingBox.GetMinLat(),
                             boundingBox.GetMaxLon()),
                    pixel)) {
      return false;
    }

    xMin=std::min(xMin,
                  pixel.GetX());
    xMax=std::max(xMax,
                  pixel.GetX());
    yMin=std::min(yMin,
                  pixel.GetY());
    yMax=std::max(yMax,
                  pixel.GetY());

    if (!GeoToPixel(GeoCoord(boundingBox.GetMaxLat(),
                             boundingBox.GetMinLon()),
                    pixel)) {
      return false;
    }

    xMin=std::min(xMin,
                  pixel.GetX());
    xMax=std::max(xMax,
                  pixel.GetX());
    yMin=std::min(yMin,
                  pixel.GetY());
    yMax=std::max(yMax,
                  pixel.GetY());

    screenBox=ScreenBox(Vertex2D(xMin,yMin),
                        Vertex2D(xMax,yMax));

    return true;
  }

  bool MercatorProjection::Set(const GeoCoord& coord,
                               double angle,
                               const Magnification& magnification,
                               double dpi,
                               size_t width, size_t height)
  {
    if (valid &&
        this->lon==coord.GetLon() &&
        this->lat==coord.GetLat() &&
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
    this->lon=coord.GetLon();
    this->lat=coord.GetLat();
    this->angle=angle;
    this->magnification=magnification;
    this->dpi=dpi;
    this->width=width;
    this->height=height;

    if (angle!=0.0) {
      angleSin=::sin(angle);
      angleCos=::cos(angle);
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
    double equatorTileWidth=tileWidthZoom0Aquator/magnification.GetMagnification();

    // Resolution (meter/pixel) of a pixel in a classical 256 pixel tile for the given zoom level at the equator
    double equatorTileResolution=equatorTileWidth/256.0;

    // Modified resolution (meter/pixel) at the equator based on our actual DPI instead of the standard tile DPI
    double equatorCorrectedEquatorTileResolution=equatorTileResolution*tileDPI/dpi;

    // Width of the visible area at the equator
    double groundWidthEquatorMeter=width*equatorCorrectedEquatorTileResolution;

    // Width of the visible area in meter
    double groundWidthVisibleMeter=groundWidthEquatorMeter*::cos(lat*gradtorad);

    // Resulting projection scale factor
    scale=width/(2*M_PI*groundWidthEquatorMeter/earthExtentMeter);
    scaleGradtorad=scale*gradtorad;

    // Size of one pixel in meter
    pixelSize=groundWidthVisibleMeter/width;

    // How many pixel are one meter?
    meterInPixel=1.0/pixelSize;

    // 1 meter on the ground is how many millimeter on display?
    meterInMM=meterInPixel*25.4/dpi;

    // Absolute Y mercator coordinate for latitude
    latOffset=::atanh(::sin(coord.GetLat()*gradtorad));

    GeoCoord topLeft;

    PixelToGeo(0.0,0.0,topLeft);

    GeoCoord topRight;

    PixelToGeo((double)width,0.0,topRight);

    GeoCoord bottomLeft;

    PixelToGeo(0.0,(double)height,bottomLeft);

    GeoCoord bottomRight;

    PixelToGeo((double)width,(double)height,bottomRight);

    // evaluate bounding box, crop bounding box to valid Mercator area
    latMin=std::max(MinLat,std::min(std::min(topLeft.GetLat(),topRight.GetLat()),
                                    std::min(bottomLeft.GetLat(),bottomRight.GetLat())));
    latMax=std::min(MaxLat,std::max(std::max(topLeft.GetLat(),topRight.GetLat()),
                                    std::max(bottomLeft.GetLat(),bottomRight.GetLat())));

    lonMin=std::max(MinLon,std::min(std::min(topLeft.GetLon(),topRight.GetLon()),
                                    std::min(bottomLeft.GetLon(),bottomRight.GetLon())));
    lonMax=std::min(MaxLon,std::max(std::max(topLeft.GetLon(),topRight.GetLon()),
                                    std::max(bottomLeft.GetLon(),bottomRight.GetLon())));

    // derivation of "latToYPixel" function in projection center
    double latDeriv = 1.0 / ::sin( (2 * this->lat * gradtorad + M_PI) /  2);
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
    coord.Set(::atan(::sinh(y/scale+latOffset))/gradtorad,
              this->lon+x/scaleGradtorad);

    return IsValidFor(coord);
  }

  bool MercatorProjection::GeoToPixel(const GeoCoord& coord,
                                      Vertex2D& pixel) const
  {
    assert(valid);

    // Screen coordinate relative to center of image
    double x=(coord.GetLon()-this->lon)*scaleGradtorad;
    double y;

    if (useLinearInterpolation) {
      y=(coord.GetLat()-this->lat)*scaledLatDeriv;
    }
    else {
      // Mercator is defined just for latitude +-85.0511
      // For values outside this range is better to result projection border
      // than some invalid coordinate, like INFINITY
      double lat = std::min(std::max(coord.GetLat(), MinLat), MaxLat);
      y=(::atanh(::sin(lat*gradtorad))-latOffset)*scale;
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

    GeoToPixel(GeoCoord(lat,lon),
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
    latOffset=scale*::atanh(::sin(latMin*gradtorad));

    pixelSize=earthExtentMeter/magnification.GetMagnification()/width;
    meterInPixel=1/pixelSize;
    meterInMM=meterInPixel*25.4/pixelSize;

    // derivation of "latToYPixel" function in projection center
    double latDeriv = 1.0 / ::sin( (2 * this->lat * gradtorad + M_PI) /  2);
    scaledLatDeriv = latDeriv * gradtorad * scale;

#ifdef OSMSCOUT_HAVE_SSE2
    sse2LonOffset      = _mm_set1_pd(lonOffset);
    sse2LatOffset      = _mm_set1_pd(latOffset);
    sse2Scale          = _mm_set1_pd(scale);
    sse2ScaleGradtorad = _mm_set1_pd(scaleGradtorad);
    sse2Height         = _mm_set1_pd(double(height));
#endif

    return true;
}

  bool TileProjection::Set(const OSMTileId& tile,
                           const Magnification& magnification,
                           double dpi,
                           size_t width, size_t height)
  {
    GeoBox boundingBox(tile.GetBoundingBox(magnification));

    return SetInternal(boundingBox.GetMinLon(),
                       boundingBox.GetMinLat(),
                       boundingBox.GetMaxLon(),
                       boundingBox.GetMaxLat(),
                       magnification,
                       dpi,
                       width,height);
  }

  bool TileProjection::Set(const OSMTileIdBox& tileBox,
                           const Magnification& magnification,
                           double dpi,
                           size_t width,size_t height)
  {
    GeoBox boundingBox(tileBox.GetBoundingBox(magnification));

    return SetInternal(boundingBox.GetMinLon(),
                       boundingBox.GetMinLat(),
                       boundingBox.GetMaxLon(),
                       boundingBox.GetMaxLat(),
                       magnification,
                       dpi,
                       width,height);
  }

  bool TileProjection::PixelToGeo(double x, double y,
                                  GeoCoord& coord) const
  {
    coord.Set(::atan(::sinh((height-y+latOffset)/scale))/gradtorad,
              (x+lonOffset)/(scale*gradtorad));

    return IsValidFor(coord);
  }

  #ifdef OSMSCOUT_HAVE_SSE2

    bool TileProjection::GeoToPixel(const GeoCoord& coord,
                                    Vertex2D& pixel) const
    {
      double x=coord.GetLon()*scaleGradtorad-lonOffset;
      double y=height-(scale*atanh_sin_pd(coord.GetLat()*gradtorad)-latOffset);
      pixel=Vertex2D(x,y);
      return IsValidFor(coord);
    }

    //this basically transforms 2 coordinates in 1 call
    void TileProjection::GeoToPixel(const BatchTransformer& transformData) const
    {
      v2df x = _mm_sub_pd(_mm_mul_pd( ARRAY2V2DF(transformData.lon), sse2ScaleGradtorad), sse2LonOffset);
      __m128d test = ARRAY2V2DF(transformData.lat);
      v2df y = _mm_sub_pd(sse2Height,
                          _mm_sub_pd(_mm_mul_pd(sse2Scale,
                                                atanh_sin_pd( _mm_mul_pd( test,  ARRAY2V2DF(sseGradtorad)))),
                                     sse2LatOffset));

      //store results:
      _mm_storel_pd (transformData.xPointer[0], x);
      _mm_storeh_pd (transformData.xPointer[1], x);
      _mm_storel_pd (transformData.yPointer[0], y);
      _mm_storeh_pd (transformData.yPointer[1], y);
    }

  #else
  bool TileProjection::GeoToPixel(const GeoCoord& coord,
                                  Vertex2D& pixel) const
  {
    double x=coord.GetLon()*scaleGradtorad-lonOffset;
    double y;

    if (useLinearInterpolation) {
      y=(height/2.0)-((coord.GetLat()-this->lat)*scaledLatDeriv);
    }
    else {
      y=height-(scale*::atanh(::sin(coord.GetLat()*gradtorad))-latOffset);
    }

    pixel=Vertex2D(x,y);

    return IsValidFor(coord);
  }

  void TileProjection::GeoToPixel(const BatchTransformer& /*transformData*/) const
    {
      assert(false); //should not be called
    }

  #endif
}
