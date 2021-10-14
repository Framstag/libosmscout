#ifndef OSMSCOUT_UTIL_PROJECTION_H
#define OSMSCOUT_UTIL_PROJECTION_H

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

#include <osmscout/CoreImportExport.h>

#include <osmscout/GeoCoord.h>
#include <osmscout/Point.h>

#include <osmscout/util/GeoBox.h>
#include <osmscout/util/Magnification.h>
#include <osmscout/util/Tiling.h>

#include <osmscout/system/SSEMathPublic.h>

namespace osmscout {

  /**
   * \ingroup Geometry
   *
   * The Projection class is an abstract base class for multiple projection implementations.
   *
   * The Projection class allows transformation of geo coordinates to screen/image coordinates and
   * screen/image coordinates back to geo coordinates.
   */
  class OSMSCOUT_API Projection
  {
  protected:
    double        lon=0.0;          //!< Longitude coordinate of the center of the image
    double        lat=0.0;          //!< Latitude coordinate of the center of the image
    double        angle=0.0;        //!< Display rotation angle in radians, canvas clockwise
    Magnification magnification;    //!< Current magnification
    double        dpi=0.0;          //!< Screen DPI
    size_t        width=0;          //!< Width of image
    size_t        height=0;         //!< Height of image

    double        lonMin=0.0;       //!< Longitude of the upper left corner of the image
    double        latMin=0.0;       //!< Latitude of the upper left corner of the image
    double        lonMax=0.0;       //!< Longitude of the lower right corner of the image
    double        latMax=0.0;       //!< Latitude of the lower right corner of the image

    double        pixelSize=0.0;    //!< Size of a pixel in meter
    double        meterInPixel=0.0; //!< Number of on screen pixel for one meter on the ground
    double        meterInMM=0.0;    //!< Number of on screen millimeters for one meter on the ground

  public:

    /**
     * This class is used to hide internal complexity concerned with batching GeoToPixel calls
     */
    class BatchTransformer
    {
    public:
      // this should be private, but that would exclude future projection
      // implementors. I don't know a nice way to handle this
      const Projection&  projection;
#ifdef OSMSCOUT_HAVE_SSE2
      int                count;
      ALIGN16_BEG double lon[2] ALIGN16_END;
      ALIGN16_BEG double lat[2] ALIGN16_END;
      double*            xPointer[2];
      double*            yPointer[2];
#endif

    public:
      explicit BatchTransformer(const Projection& projection)
        : projection(projection)
#ifdef OSMSCOUT_HAVE_SSE2
          ,count(0)
#endif
      {
      }

      ~BatchTransformer()
      {
        Flush();
      }

      BatchTransformer(const BatchTransformer& other) = delete;

      void GeoToPixel(const GeoCoord& coord,
                      double& x,
                      double& y)
      {
#ifdef OSMSCOUT_HAVE_SSE2
        if (projection.CanBatch()) {
          this->lon[count]=coord.GetLon();
          this->lat[count]=coord.GetLat();
          xPointer[count]=&x;
          yPointer[count]=&y;
          count++;

          if (count==2) {
            count=0;
            projection.GeoToPixel(*this);
          }
        }
        else {
          projection.GeoToPixel(coord,
                                x,y);
        }
#else
        projection.GeoToPixel(coord,
                              x,y);
#endif
      }

      void GeoToPixel(const Point& coord,
                      double& x,
                      double& y)
      {
#ifdef OSMSCOUT_HAVE_SSE2
        if (projection.CanBatch()) {
          this->lon[count]=coord.GetCoord().GetLon();
          this->lat[count]=coord.GetCoord().GetLat();
          xPointer[count]=&x;
          yPointer[count]=&y;
          count++;

          if (count==2) {
            count=0;
            projection.GeoToPixel(*this);
          }
        }
        else {
          projection.GeoToPixel(coord.GetCoord(),
                                x,y);
        }
#else
        projection.GeoToPixel(coord.GetCoord(),
                              x,y);
#endif
      }

      void Flush()
      {
#ifdef OSMSCOUT_HAVE_SSE2
        if (count!=0) {
          count=0;
          projection.GeoToPixel(GeoCoord(lat[0],
                                         lon[0]),
                                *xPointer[0],
                                *yPointer[0]);
        }
#endif
      }
    };

    Projection() = default;
    virtual ~Projection() = default;

    virtual bool CanBatch() const = 0;
    virtual bool IsValid() const = 0;

    /**
     * Return true if given coordinate is valid for this projection
     */
    virtual bool IsValidFor(const GeoCoord& coord) const = 0;

    GeoCoord GetCenter() const
    {
      return GeoCoord(lat,lon);
    }

    /**
     * Returns the angle in radians ([0..2*PI[) of the display in relation to the north. A degree of 0 means
     * north is to the top, a degree of PI, renders with the south to the top of the display).
     */
    double GetAngle() const
    {
      return angle;
    }

    /**
     * Returns the width of the screen
     */
    size_t GetWidth() const
    {
      return width;
    }

    /**
     * Returns the height of the screen
     */
    size_t GetHeight() const
    {
      return height;
    }

    /**
     * Return the magnification as part of the projection.
     */
    Magnification GetMagnification() const
    {
      return magnification;
    }

    /**
     * Return the DPI as part of the projection.
     */
    double GetDPI() const
    {
      return dpi;
    }

    /**
     * Returns true, if the given geo coordinate is in the bounding box
     */
    bool GeoIsIn(double lon, double lat) const
    {
      return lon>=lonMin && lon<=lonMax && lat>=latMin && lat<=latMax;
    }

    /**
     * Returns true, if the given bounding box is completely within the projection bounding box
     */
    bool GeoIsIn(double lonMin, double latMin,
                 double lonMax, double latMax) const
    {
      return !(lonMin>this->lonMax ||
               lonMax<this->lonMin ||
               latMin>this->latMax ||
               latMax<this->latMin);
    }

    GeoBox GetDimensions() const
    {
      return GeoBox(GeoCoord(latMin,lonMin),
                    GeoCoord(latMax,lonMax));
    }

    /**
     * Returns the bounding box of the area covered
     */
    void GetDimensions(GeoBox& boundingBox) const
    {
      boundingBox.Set(GeoCoord(latMin,lonMin),
                      GeoCoord(latMax,lonMax));
    }

    /**
     * Returns the size of a pixel in meter
     */
    double GetPixelSize() const
    {
      return pixelSize;
    }

    /**
     * Returns the number of on screen pixel for one meter on the ground
     */
    double GetMeterInPixel() const
    {
      return meterInPixel;
    }

    /**
     * Returns the number of on screen millimeters for one meter on the ground
     */
    double GetMeterInMM() const
    {
      return meterInMM;
    }

    /**
     * Convert a width in mm into the equivalent pixel size based on the given DPI
     *
     * @param width
     *    Width in mm
     * @return
     *    Width in screen pixel
     */
    double ConvertWidthToPixel(double width) const
    {
      return width*dpi/25.4;
    }

    /**
     * Convert a width in pixel into the equivalent mm size based on the given DPI
     *
     * @param width
     *    Width in screen pixel
     * @return
     *    Width in mm
     */
    double ConvertPixelToWidth(double pixel) const
    {
      return pixel*25.4/dpi;
    }

    /**
     * Converts a pixel coordinate to a geo coordinate.
     *
     * Return true on success,
     * false if returned coordinate is not valid
     * for this projection.
     */
    virtual bool PixelToGeo(double x, double y,
                            GeoCoord& coord) const = 0;

    /**
     * Converts a geo coordinate to a pixel coordinate.
     *
     * Return true on success,
     * false if given coordinate is not valid for this projection.
     */
    virtual bool GeoToPixel(const GeoCoord& coord,
                            double& x, double& y) const = 0;

    /**
     * Converts a valid GeoBox to its on screen pixel coordinates
     *
     * Return true on success,
     * false if given coordinate is not valid for this projection.
     */
    bool BoundingBoxToPixel(const GeoBox& boundingBox,
                            double& xMin, double& yMin,
                            double& xMax, double& yMax) const;

  protected:
    virtual void GeoToPixel(const BatchTransformer& transformData) const = 0;

    friend class BatchTransformer;
  };

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
  protected:
    bool   valid=false;    //!< projection is valid

    double latOffset=0.0;  //!< Absolute and untransformed screen position of lat coordinate
    double angleSin;
    double angleCos;
    double angleNegSin;
    double angleNegCos;

    double scale=1.0;
    double scaleGradtorad ; //!< Precalculated scale*Gradtorad

    double scaledLatDeriv; //!< precalculated derivation of "latToYPixel" function in projection center scaled by gradtorad * scale
    bool   useLinearInterpolation=false; //!< switch to enable linear interpolation of latitude to pixel computation

  public:
    static const double MaxLat;
    static const double MinLat;
    static const double MaxLon;
    static const double MinLon;

    MercatorProjection() = default;

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
                    double& x, double& y) const override;

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

    bool IsLinearInterpolationEnabled() const
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


  /**
   * Mercator projection as used by the OpenStreetMap tile rendering code.
   *
   * The TileProjection simplifies the general Mercator projection code to
   * make sure that there are no effects based on rounding errors or similar.
   */
  class OSMSCOUT_API TileProjection : public Projection
  {
  protected:
    bool   valid=false;          //!< projection is valid

    double lonOffset=0.0;
    double latOffset=0.0;
    double scale=1.0;
    double scaleGradtorad; //!< Precalculated scale*Gradtorad

    double scaledLatDeriv; //!< precalculated derivation of "latToYPixel" function in projection center scaled by gradtorad * scale
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
    virtual bool SetInternal(double lonMin,double latMin,
                             double lonMax,double latMax,
                             const Magnification& magnification,
                             double dpi,
                             size_t width,size_t height);

  public:
    TileProjection() = default;

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
      return Set(tile,magnification,GetDPI(),width,height);
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
                    double& x, double& y) const override;

    bool IsLinearInterpolationEnabled() const
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
