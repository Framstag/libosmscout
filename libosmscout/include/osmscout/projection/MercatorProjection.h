#ifndef OSMSCOUT_UTIL_MERCATORPROJECTION_H
#define OSMSCOUT_UTIL_MERCATORPROJECTION_H

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

namespace osmscout {

  /**
   * Mercator projection that tries to render the resulting map in the same
   * physical size on all devices. If the physical DPI of the device is
   * correctly given, objects on any device has the same size. Bigger devices
   * will show "more" map thus.
   *
   * Scale is calculated based on the assumption that the original OpenStreetMap
   * tiles were designed for 96 DPI displays.
   *
   */
  class OSMSCOUT_API MercatorProjection : public Projection
  {
  protected: // Because the OpenGLProjection inherits from this projection
    bool   valid=false;    //!< projection is valid

    double latOffset=0.0;  //!< Absolute and untransformed screen position of lat coordinate
    double angleSin=0.0;
    double angleCos=0.0;
    double angleNegSin=0.0;
    double angleNegCos=0.0;

    double scale=1.0;
    double scaleGradtorad=0.0 ; //!< Precalculated scale*Gradtorad

    double scaledLatDeriv=0.0; //!< precalculated derivation of "latToYPixel" function in projection
                           //!< center scaled by gradtorad * scale
    bool   useLinearInterpolation=false; //!< switch to enable linear interpolation of latitude to pixel computation

  public:
    static const double MaxLat;
    static const double MinLat;
    static const double MaxLon;
    static const double MinLon;

    MercatorProjection() = default;
    MercatorProjection(const MercatorProjection&) = default;
    MercatorProjection(MercatorProjection&&) = default;
    MercatorProjection& operator=(const MercatorProjection&) = default;
    MercatorProjection& operator=(MercatorProjection&&) = default;
    ~MercatorProjection() override = default;

    bool CanBatch() const override
    {
      return false;
    }

    bool IsValid() const override
    {
      return valid;
    }

    bool IsValidFor(const GeoCoord& coord) const override
    {
      return coord.GetLat() >= MinLat && coord.GetLat() <= MaxLat &&
             coord.GetLon() >= MinLon && coord.GetLon() <= MaxLon;
    }

    bool Set(const GeoCoord& coord,
                    const Magnification& magnification,
                    size_t width,size_t height)
    {
      return Set(coord,0.0,magnification,GetDPI(),width,height);
    }

    bool Set(const GeoCoord& coord,
                    double angle,
                    const Magnification& magnification,
                    size_t width, size_t height)
    {
      return Set(coord,angle,magnification,GetDPI(),width,height);
    }

    bool Set(const GeoCoord& coord,
                    const Magnification& magnification,
                    double dpi,
                    size_t width, size_t height)
    {
      return Set(coord,0.0,magnification,dpi,width,height);
    }

    /**
     * Setup projection parameters.
     *
     * Return true on success,
     * false if arguments are not valid for Mercator projection,
     * projection parameters are unchanged in such case.
     *
     * Angle is in radians ([0..2*PI[)
     *
     * Note that coord (center) have to be valid coordinate
     * in Mercator projection. But it is possible setup dimensions
     * (width and height) that projection will cover area bigger
     * than the one valid for Mercator projection. Bounding box
     * is adjusted then to be valid for projection.
     *
     * In code:
     *
     *   projection.GetDimensions(bbox);
     *   projection.GeoToPixel(bbox.GetMinCoord(),x,y)
     *
     * may be x >= 0
     */
    bool Set(const GeoCoord& coord,
             double angle,
             const Magnification& magnification,
             double dpi,
             size_t width, size_t height);

    bool PixelToGeo(double x, double y,
                    GeoCoord& coord) const override;

    bool GeoToPixel(const GeoCoord& coord,
                    Vertex2D& pixel) const override;

    bool Move(double horizPixel,
              double vertPixel);

    bool MoveUp(double pixel)
    {
      return Move(0,pixel);
    }

    bool MoveDown(double pixel)
    {
      return Move(0,-pixel);
    }

    bool MoveLeft(double pixel)
    {
      return Move(-pixel,0);
    }

    bool MoveRight(double pixel)
    {
      return Move(pixel,0);
    }

    [[nodiscard]] bool IsLinearInterpolationEnabled() const
    {
      return useLinearInterpolation;
    }

    /**
     * Switch to enable/disable linear interpolation of latitude to pixel computation.
     * It speedup GeoToPixel calculation with fractional error on small render area.
     */
    void SetLinearInterpolationUsage(bool useLinearInterpolation)
    {
      this->useLinearInterpolation=useLinearInterpolation;
    }

  protected:
    void GeoToPixel(const BatchTransformer& transformData) const override;
  };
}

#endif
