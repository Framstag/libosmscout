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

#include <osmscout/projection/TileProjection.h>

#include <osmscout/projection/Earth.h>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

#ifdef OSMSCOUT_HAVE_SSE2
#include <osmscout/system/SSEMath.h>
#endif

#include <osmscout/util/Tiling.h>

namespace osmscout {

#ifdef OSMSCOUT_HAVE_SSE2
  static const ALIGN16_BEG double sseGradtorad[] ALIGN16_END = {2*M_PI/360, 2*M_PI/360};
#endif

  bool TileProjection::SetInternal(const GeoBox& boundingBox,
                                   const Magnification& magnification,
                                   double dpi,
                                   size_t width,size_t height)
  {
    if (valid &&
        this->boundingBox==boundingBox &&
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

    this->boundingBox=boundingBox;
    this->center=this->boundingBox.GetCenter();

    scale=width/(gradtorad*(boundingBox.GetMaxLon()-boundingBox.GetMinLon()));
    scaleGradtorad = scale * gradtorad;

    lonOffset=boundingBox.GetMinLon()*scaleGradtorad;
    latOffset=scale*std::atanh(std::sin(boundingBox.GetMinLat()*gradtorad));

    pixelSize=Earth::extentMeter/magnification.GetMagnification()/width;
    meterInPixel=1/pixelSize;
    meterInMM=meterInPixel*25.4/pixelSize;

    // derivation of "latToYPixel" function in projection center
    double latDeriv = 1.0 / std::sin( (2 * this->center.GetLat() * gradtorad + M_PI) /  2);
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

    return SetInternal(boundingBox,
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

    return SetInternal(boundingBox,
                       magnification,
                       dpi,
                       width,height);
  }

  bool TileProjection::PixelToGeo(double x, double y,
                                  GeoCoord& coord) const
  {
    coord.Set(std::atan(std::sinh((height-y+latOffset)/scale))/gradtorad,
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
      y=(height/2.0)-((coord.GetLat()-this->center.GetLat())*scaledLatDeriv);
    }
    else {
      y=height-(scale*std::atanh(std::sin(coord.GetLat()*gradtorad))-latOffset);
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
