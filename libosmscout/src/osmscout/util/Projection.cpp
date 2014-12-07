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

  int long2tilex(double lon, int z)
  {
    return (int)(floor((lon + 180.0) / 360.0 * pow(2.0, z)));
  }

  int lat2tiley(double lat, int z)
  {
    return (int)(floor((1.0 - log( tan(lat * M_PI/180.0) + 1.0 / cos(lat * M_PI/180.0)) / M_PI) / 2.0 * pow(2.0, z)));
  }

  double tilex2long(int x, int z)
  {
    return x / pow(2.0, z) * 360.0 - 180;
  }

  double tiley2lat(int y, int z)
  {
    double n = M_PI - 2.0 * M_PI * y / pow(2.0, z);
    return 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));
  }

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
    // No code
  }

  bool MercatorProjection::Set(double lon, double lat,
                               const Magnification& magnification,
                               double dpi,
                               size_t width, size_t height)
  {
    if (valid &&
        this->lon==lon &&
        this->lat==lat &&
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
    this->magnification=magnification;
    this->dpi=dpi;
    this->width=width;
    this->height=height;

    //
    // Calculation of bounds and scaling factors
    //
    // We have three projections:
    // * Mercator projection of longitude to X-coordinate
    // * Mercator projection of latitude to Y-coordinate
    // * Projection of X and Y coordinate as result of mercator projection to on screen coordinates
    //

    double boxWidth=360/magnification.GetMagnification(); // Part of the full earth circle that has to be shown, in degree

    // longitude does scale linear, so left and right longitude borders is easy to calculate
    lonMin=lon-boxWidth/2;
    lonMax=lon+boxWidth/2;

    scale=(width-1)/(gradtorad*(lonMax-lonMin));
    scaleGradtorad = scale * gradtorad;

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
    std::cout << "Magnification: " << magnification.GetMagnification() << "/" << magnification.GetLevel() << std::endl;
    std::cout << "Scale: " << scale << std::endl;
    std::cout << "Screen dimension: " << width << "x" << height << std::endl;
    std::cout << "d: " << d << " " << d*180*60/M_PI << std::endl;
    std::cout << "The complete screen are " << d*180*60/M_PI*1852.216 << " meters" << std::endl;
    std::cout << "1 pixel are " << pixelSize << " meters" << std::endl;
    std::cout << "20 meters are " << 20/(d*180*60/M_PI*1852.216/width) << " pixels" << std::endl;*/

#ifdef OSMSCOUT_HAVE_SSE2
    sse2LonOffset      = _mm_set1_pd(lonOffset);
    sse2LatOffset      = _mm_set1_pd(latOffset);
    sse2Scale          = _mm_set1_pd(scale);
    sse2ScaleGradtorad = _mm_set1_pd(scaleGradtorad);
    sse2Height         = _mm_set1_pd(height);
#endif

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
    lat=atan(sinh((height-y+latOffset)/scale))/gradtorad;

    return true;
  }

#ifdef OSMSCOUT_HAVE_SSE2

  bool MercatorProjection::GeoToPixel(double lon, double lat,
                                      double& x, double& y) const
  {
    assert(valid);

    x=lon*scaleGradtorad-lonOffset;
    y=height-(scale*atanh_sin_pd(lat*gradtorad)-latOffset);

    return true;
  }

  //this basically transforms 2 coordinates in 1 call
  bool MercatorProjection::GeoToPixel(const BatchTransformer& transformData) const
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

  bool MercatorProjection::GeoToPixel(double lon, double lat,
                                      double& x, double& y) const
  {
    assert(valid);

    x=lon*scaleGradtorad-lonOffset;
    y=height-(scale*atanh(sin(lat*gradtorad))-latOffset);

    return true;
  }

  bool MercatorProjection::GeoToPixel(const BatchTransformer& /*transformData*/) const
  {
    assert(false); //should not be called
    return false;
  }

#endif

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

//
// ReversedYAxisMercatorProjection class
//

  bool ReversedYAxisMercatorProjection::PixelToGeo(double x, double y,
                                                   double& lon, double& lat) const
  {
        assert(valid);

        lon=(x+lonOffset)/(scale*gradtorad);
        lat=atan(sinh((height-y+latOffset)/scale))/gradtorad;

        return true;
    }

#ifdef OSMSCOUT_HAVE_SSE2

    bool ReversedYAxisMercatorProjection::GeoToPixel(double lon, double lat,
                                                     double& x, double& y) const
    {
        assert(valid);

        x=lon*scaleGradtorad-lonOffset;
        y=(scale*atanh_sin_pd(lat*gradtorad)-latOffset);

        return true;
    }

    //this basically transforms 2 coordinates in 1 call
    bool ReversedYAxisMercatorProjection::GeoToPixel(const BatchTransformer& transformData) const
    {
        v2df x = _mm_sub_pd(_mm_mul_pd( ARRAY2V2DF(transformData.lon), sse2ScaleGradtorad), sse2LonOffset);
        __m128d test = ARRAY2V2DF(transformData.lat);
        v2df y = _mm_sub_pd(_mm_mul_pd(sse2Scale, atanh_sin_pd( _mm_mul_pd( test,ARRAY2V2DF(sseGradtorad)))),sse2LatOffset);

        //store results:
        _mm_storel_pd (transformData.xPointer[0], x);
        _mm_storeh_pd (transformData.xPointer[1], x);
        _mm_storel_pd (transformData.yPointer[0], y);
        _mm_storeh_pd (transformData.yPointer[1], y);

        return true;
    }

#else

    bool ReversedYAxisMercatorProjection::GeoToPixel(double lon, double lat,
                                        double& x, double& y) const
    {
        assert(valid);

        x=lon*scaleGradtorad-lonOffset;
        y=(scale*atanh(sin(lat*gradtorad))-latOffset);

        return true;
    }

    bool ReversedYAxisMercatorProjection::GeoToPixel(const BatchTransformer& /*transformData*/) const
    {
        assert(false); //should not be called
        return false;
    }

#endif


    Mercator2Projection::Mercator2Projection()
    : valid(false),
      lon(0),
      lat(0),
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

    bool Mercator2Projection::Set(double lon, double lat,
                                  const Magnification& magnification,
                                  double dpi,
                                  size_t width, size_t height)
    {
      if (valid &&
          this->lon==lon &&
          this->lat==lat &&
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
      this->magnification=magnification;
      this->dpi=dpi;
      this->width=width;
      this->height=height;

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

      // longitude does scale linear, so left and right longitude borders is easy to calculate
      lonMin=lon-boxWidth/2;
      lonMax=lon+boxWidth/2;

      scale=(width-1)/(gradtorad*(lonMax-lonMin));
      scaleGradtorad = scale * gradtorad;

      pixelSize=groundWidthMeter/width;

      // Absolute Y mercator coordinate for latitude
      double y=atanh(sin(lat*gradtorad));

      latMin=atan(sinh(y-(height/2)/scale))/gradtorad;
      latMax=atan(sinh(y+(height/2)/scale))/gradtorad;

      lonOffset=lonMin*scaleGradtorad;
      latOffset=scale*atanh(sin(latMin*gradtorad));
/*
      std::cout << "Center: " << lat << "° lat " << lon << "° lon" << std::endl;
      std::cout << "Magnification: " << magnification.GetMagnification() << "/" << magnification.GetLevel() << std::endl;
      std::cout << "Screen dimension: " << width << "x" << height << " " << dpi << " DPI "<< screenWidthMeter << "m x " << screenHeightMeter << "m" << std::endl;

      std::cout << "Box: " << latMin << "° - " << latMax << "° lat x " << lonMin << "° -" << lonMax << "° lon, " << groundWidthMeter << "m x " << groundHeightMeter << "m" << std::endl;

      std::cout << "Scale: 1 : " << scale << std::endl;*/

  #ifdef OSMSCOUT_HAVE_SSE2
      sse2LonOffset      = _mm_set1_pd(lonOffset);
      sse2LatOffset      = _mm_set1_pd(latOffset);
      sse2Scale          = _mm_set1_pd(scale);
      sse2ScaleGradtorad = _mm_set1_pd(scaleGradtorad);
      sse2Height         = _mm_set1_pd(height);
  #endif

      return true;
    }

    bool Mercator2Projection::GeoIsIn(double lon, double lat) const
    {
      assert(valid);

      return lon>=lonMin && lon<=lonMax && lat>=latMin && lat<=latMax;
    }

    bool Mercator2Projection::GeoIsIn(double lonMin, double latMin,
                                     double lonMax, double latMax) const
    {
      assert(valid);

      return !(lonMin>this->lonMax ||
               lonMax<this->lonMin ||
               latMin>this->latMax ||
               latMax<this->latMin);
    }

    bool Mercator2Projection::PixelToGeo(double x, double y,
                                        double& lon, double& lat) const
    {
      assert(valid);

      lon=(x+lonOffset)/(scale*gradtorad);
      lat=atan(sinh((height-y+latOffset)/scale))/gradtorad;

      return true;
    }

  #ifdef OSMSCOUT_HAVE_SSE2

    bool Mercator2Projection::GeoToPixel(double lon, double lat,
                                        double& x, double& y) const
    {
      assert(valid);

      x=lon*scaleGradtorad-lonOffset;
      y=height-(scale*atanh_sin_pd(lat*gradtorad)-latOffset);

      return true;
    }

    //this basically transforms 2 coordinates in 1 call
    bool Mercator2Projection::GeoToPixel(const BatchTransformer& transformData) const
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

    bool Mercator2Projection::GeoToPixel(double lon, double lat,
                                        double& x, double& y) const
    {
      assert(valid);

      x=lon*scaleGradtorad-lonOffset;
      y=height-(scale*atanh(sin(lat*gradtorad))-latOffset);

      return true;
    }

    bool Mercator2Projection::GeoToPixel(const BatchTransformer& /*transformData*/) const
    {
      assert(false); //should not be called
      return false;
    }

  #endif

    bool Mercator2Projection::GetDimensions(double& lonMin, double& latMin,
                                            double& lonMax, double& latMax) const
    {
      assert(valid);

      lonMin=this->lonMin;
      latMin=this->latMin;
      lonMax=this->lonMax;
      latMax=this->latMax;

      return true;
    }

    double Mercator2Projection::GetPixelSize() const
    {
      assert(valid);

      return pixelSize;
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

    bool TileProjection::GeoToPixel(const BatchTransformer& /*transformData*/) const
    {
      assert(false); //should not be called
      return false;
    }

  #endif

  bool TileProjection::GetDimensions(double& lonMin, double& latMin,
                                     double& lonMax, double& latMax) const
  {
    assert(valid);

    lonMin=this->lonMin;
    latMin=this->latMin;
    lonMax=this->lonMax;
    latMax=this->latMax;

    return true;
  }

  double TileProjection::GetPixelSize() const
  {
    assert(valid);

    return pixelSize;
  }
}
