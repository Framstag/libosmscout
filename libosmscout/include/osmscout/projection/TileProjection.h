#ifndef OSMSCOUT_UTIL_TILEPROJECTION_H
#define OSMSCOUT_UTIL_TILEPROJECTION_H

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

#include <osmscout/lib/CoreImportExport.h>

#include <osmscout/projection/Projection.h>

#include <osmscout/util/Tiling.h>

namespace osmscout {

  /**
   * Mercator projection as used by the OpenStreetMap tile rendering code.
   *
   * The TileProjection simplifies the general Mercator projection code to
   * make sure that there are no effects based on rounding errors or similar.
   */
  class OSMSCOUT_API TileProjection : public Projection
  {
  private:
    bool   valid=false;          //!< projection is valid

    double lonOffset=0.0;
    double latOffset=0.0;
    double scale=1.0;
    double scaleGradtorad=0.0; //!< Precalculated scale*Gradtorad

    double scaledLatDeriv=0.0; //!< precalculated derivation of "latToYPixel" function in projection
                           //!< center scaled by gradtorad * scale
    bool   useLinearInterpolation=false; //!< switch to enable linear interpolation of latitude to pixel computation

#ifdef OSMSCOUT_HAVE_SSE2
    //some extra vars for special sse needs
      v2df              sse2LonOffset;
      v2df              sse2LatOffset;
      v2df              sse2Scale;
      v2df              sse2ScaleGradtorad;
      v2df              sse2Height;
#endif

  protected:
    virtual bool SetInternal(const GeoBox& boundingBox,
                             const Magnification& magnification,
                             double dpi,
                             size_t width,size_t height);

  public:
    TileProjection() = default;
    TileProjection(const TileProjection&) = default;
    TileProjection(TileProjection&&) = default;
    TileProjection& operator=(const TileProjection&) = default;
    TileProjection& operator=(TileProjection&&) = default;
    ~TileProjection() override = default;

    bool CanBatch() const override
    {
      return true;
    }

    bool IsValid() const override
    {
      return valid;
    }

    bool IsValidFor(const GeoCoord& coord) const override
    {
      return coord.GetLat() >= -85.0511 && coord.GetLat() <= +85.0511 &&
             coord.GetLon() >= -180.0   && coord.GetLon() <= +180.0;
    }

    bool Set(const OSMTileId& tile,
             const Magnification& magnification,
             size_t width, size_t height)
    {
      return Set(tile,
                 magnification,
                 GetDPI(),
                 width,
                 height);
    }

    bool Set(const OSMTileId& tile,
             const Magnification& magnification,
             double dpi,
             size_t width, size_t height);

    bool Set(const OSMTileIdBox& tileBox,
             const Magnification& magnification,
             double dpi,
             size_t width, size_t height);

    bool PixelToGeo(double x, double y,
                    GeoCoord& coord) const override;

    bool GeoToPixel(const GeoCoord& coord,
                    Vertex2D& pixel) const override;

    [[nodiscard]] bool IsLinearInterpolationEnabled() const
    {
      return useLinearInterpolation;
    }

    /**
     * Switch to enable/disable linear interpolation of latitude to pixel computation.
     * It speedup GeoToPixel calculation with fractional error on small render area.
     */
    void SetLinearInterpolationUsage(bool b)
    {
      useLinearInterpolation = b;
    }

  protected:

    void GeoToPixel(const BatchTransformer& transformData) const override;
  };
}

#endif
