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

#include <osmscout/private/CoreImportExport.h>

#include <osmscout/system/SSEMathPublic.h>

#include <osmscout/GeoCoord.h>

#include <osmscout/util/GeoBox.h>
#include <osmscout/util/Magnification.h>

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

      inline ~BatchTransformer()
      {
        Flush();
      }

      inline void GeoToPixel(double lon,
                             double lat,
                             double& x,
                             double& y)
      {
#ifdef OSMSCOUT_HAVE_SSE2
        if (projection.CanBatch()) {
          this->lon[count]=lon;
          this->lat[count]=lat;
          xPointer[count]=&x;
          yPointer[count]=&y;
          count++;

          if (count==2) {
            count=0;
            return projection.GeoToPixel(*this);
          }

          return true;
        }
        else {
          return projection.GeoToPixel(lon,lat,x,y);
        }
#else
        projection.GeoToPixel(lon,lat,x,y);
#endif
      }

      void Flush()
      {
#ifdef OSMSCOUT_HAVE_SSE2
        if (count!=0) {
          count=0;
          projection.GeoToPixel(lon[0],
                                lat[0],
                                *xPointer[0],
                                *yPointer[0]);
        }
#endif
      }
    };

    virtual ~Projection();

    virtual bool CanBatch() const = 0;

    virtual GeoCoord GetCenter() const = 0;

    /**
     * Returns longitude coordinate of the region center.
     *
     */
    virtual double GetLon() const = 0;

    /**
     * Returns latitude coordinate of the region center.
     *
     */
    virtual double GetLat() const = 0;

    /**
     * Returns the angle ([0..2*PI[) of the display in relation to the north. A degree of 0 means
     * north is to the top, a degree of PI, renders with the south to the top of the display).
     */
    virtual double GetAngle() const = 0;

    /**
     * Returns the width of the screen
     */
    virtual size_t GetWidth() const = 0;
    /**
     * Returns the height of the screen
     */
    virtual size_t GetHeight() const = 0;

    /**
     * Return the magnification as part of the projection.
     */
    virtual Magnification GetMagnification() const = 0;

    /**
     * Return the DPI as part of the projection.
     */
    virtual double GetDPI() const = 0;

    /**
     * Set the "screen" for the projection.
     *
     * @param lon Longitude coordinate of the center of the screen/image
     * @param lat Latitude coordinate of the center of the screen/image
     * @param magnification Magnification to apply
     * @param width Width of screen/image
     * @param height Height of screen/image
     */
    /*
    virtual bool Set(double lon, double lat,
                     const Magnification& magnification,
                     size_t width, size_t height) = 0;*/

    /**
     * Set the "screen" for the projection.
     *
     * @param lon Longitude coordinate of the center of the screen/image
     * @param lat Latitude coordinate of the center of the screen/image
     * @param magnification Magnification to apply
     * @param dpi DPI of the screen/image
     * @param width Width of screen/image
     * @param height Height of screen/image
     */
    /*
    virtual bool Set(double lon, double lat,
                     const Magnification& magnification,
                     double dpi,
                     size_t width, size_t height) = 0;*/

    /**
     * Returns true, if the given geo coordinate is in the bounding box
     */
    virtual bool GeoIsIn(double lon, double lat) const = 0;

    /**
     * Returns true, if the given bounding box is completely within the projection bounding box
     */
    virtual bool GeoIsIn(double lonMin, double latMin,
                         double lonMax, double latMax) const = 0;

    /**
     * Converts a pixel coordinate to a geo coordinate
     */
    virtual bool PixelToGeo(double x, double y,
                            double& lon, double& lat) const = 0;

    /**
     * Converts a geo coordinate to a pixel coordinate
     */
    virtual void GeoToPixel(double lon, double lat,
                            double& x, double& y) const = 0;

    /**
     * Converts a geo coordinate to a pixel coordinate
     */
    virtual void GeoToPixel(const GeoCoord& coord,
                            double& x, double& y) const = 0;

    /**
     * Returns the bounding box of the area covered
     */
    virtual bool GetDimensions(GeoBox& boundingBox) const = 0;

    /**
     * Returns the size of a pixel in meter
     */
    virtual double GetPixelSize() const = 0;

    /**
     * Returns the number of on screen pixel for one meter on the ground
     */
    virtual double GetMeterInPixel() const = 0;

    /**
     * Returns the number of on screen millimeters for one meter on the ground
     */
    virtual double GetMeterInMM() const = 0;

    /**
     * Convert a width in mm into the equivalent pixel size based on the given DPI
     *
     * @param width
     *    Width in mm
     * @return
     *    Width in screen pixel
     */
    inline double ConvertWidthToPixel(double width) const
    {
      return width*GetDPI()/25.4;
    }

    /**
     * Convert a width in pixel into the equivalent mm size based on the given DPI
     *
     * @param width
     *    Width in screen pixel
     * @return
     *    Width in mm
     */
    inline double ConvertPixelToWidth(double pixel) const
    {
      return pixel*25.4/GetDPI();
    }


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
    bool                valid;          //!< projection is valid

    double              lon;            //!< Longitude coordinate of the center of the image
    double              lat;            //!< Latitude coordinate of the center of the image
    double              angle;          //!< Display rotation angle
    Magnification       magnification;  //!< Current magnification
    double              dpi;            //!< Screen DPI
    size_t              width;          //!< Width of image
    size_t              height;         //!< Height of image

    double              lonMin;         //!< Longitude of the upper left corner of the image
    double              latMin;         //!< Latitude of the upper left corner of the image
    double              lonMax;         //!< Longitude of the lower right corner of the image
    double              latMax;         //!< Latitude of the lower right corner of the image

    double              latOffset;      //!< Absolute and untransformed screen position of lat coordinate
    double              angleSin;
    double              angleCos;
    double              angleNegSin;
    double              angleNegCos;

    double              scale;
    double              scaleGradtorad; //!< Precalculated scale*Gradtorad
    double              pixelSize;      //!< Size of a pixel in meter
    double              meterInPixel;   //!< Number of on screen pixel for one meter on the ground
    double              meterInMM;      //!< Number of on screen millimeters for one meter on the ground

  public:
    MercatorProjection();

    inline bool CanBatch() const
    {
      return false;
    }

    inline GeoCoord GetCenter() const
    {
      return GeoCoord(lat,lon);
    }

    inline double GetLon() const
    {
      return lon;
    }

    inline double GetLat() const
    {
      return lat;
    }

    inline double GetAngle() const
    {
      return angle;
    }

    inline size_t GetWidth() const
    {
      return width;
    }

    inline size_t GetHeight() const
    {
      return height;
    }

    inline Magnification GetMagnification() const
    {
      return magnification;
    }

    inline double GetDPI() const
    {
      return dpi;
    }

    inline bool Set(double lon,double lat,
                    const Magnification& magnification,
                    size_t width,size_t height)
    {
      return Set(lon,lat,0,magnification,GetDPI(),width,height);
    }

    inline bool Set(double lon, double lat,
                    double angle,
                    const Magnification& magnification,
                    size_t width, size_t height)
    {
      return Set(lon,lat,angle,magnification,GetDPI(),width,height);
    }

    inline bool Set(double lon, double lat,
                    const Magnification& magnification,
                    double dpi,
                    size_t width, size_t height)
    {
      return Set(lon,lat,0,magnification,dpi,width,height);
    }

    bool Set(double lon, double lat,
             double angle,
             const Magnification& magnification,
             double dpi,
             size_t width, size_t height);

    bool GeoIsIn(double lon, double lat) const;
    bool GeoIsIn(double lonMin, double latMin,
                 double lonMax, double latMax) const;

    bool PixelToGeo(double x, double y,
                    double& lon, double& lat) const;

    void GeoToPixel(double lon, double lat,
                    double& x, double& y) const;

    void GeoToPixel(const GeoCoord& coord,
                    double& x, double& y) const;

    bool GetDimensions(GeoBox& boundingBox) const;

    inline double GetPixelSize() const
    {
      return pixelSize;
    }

    inline double GetMeterInPixel() const
    {
      return meterInPixel;
    }

    inline double GetMeterInMM() const
    {
      return meterInMM;
    }

    bool Move(double horizPixel,
              double vertPixel);

    inline bool MoveUp(double pixel)
    {
      return Move(0,pixel);
    }

    inline bool MoveDown(double pixel)
    {
      return Move(0,-pixel);
    }

    inline bool MoveLeft(double pixel)
    {
      return Move(-pixel,0);
    }

    inline bool MoveRight(double pixel)
    {
      return Move(pixel,0);
    }

  protected:
     void GeoToPixel(const BatchTransformer& transformData) const;
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
    bool                valid;          //!< projection is valid

    double              lon;            //!< Longitude coordinate of the center of the image
    double              lat;            //!< Latitude coordinate of the center of the image
    Magnification       magnification;  //!< Current magnification
    double              dpi;            //!< Screen DPI
    size_t              width;          //!< Width of image
    size_t              height;         //!< Height of image

    double              lonMin;         //!< Longitude of the upper left corner of the image
    double              latMin;         //!< Latitude of the upper left corner of the image
    double              lonMax;         //!< Longitude of the lower right corner of the image
    double              latMax;         //!< Latitude of the lower right corner of the image

    double              lonOffset;
    double              latOffset;
    double              scale;
    double              scaleGradtorad; //!< Precalculated scale*Gradtorad
    double              pixelSize;      //!< Size of a pixel in meter
    double              meterInPixel;   //!< Number of on screen pixel for one meter on the ground
    double              meterInMM;      //!< Number of on screen millimeters for one meter on the ground

#ifdef OSMSCOUT_HAVE_SSE2
      //some extra vars for special sse needs
      v2df              sse2LonOffset;
      v2df              sse2LatOffset;
      v2df              sse2Scale;
      v2df              sse2ScaleGradtorad;
      v2df              sse2Height;
#endif

  private:
    bool SetInternal(double lonMin,double latMin,
                     double lonMax,double latMax,
                     const Magnification& magnification,
                     double dpi,
                     size_t width,size_t height);

  public:
    TileProjection();

    inline bool CanBatch() const
    {
      return true;
    }

    inline size_t GetWidth() const
    {
      return width;
    }

    inline size_t GetHeight() const
    {
      return height;
    }

    inline double GetLon() const
    {
      return lon;
    }

    inline double GetLat() const
    {
      return lat;
    }

    inline GeoCoord GetCenter() const
    {
      return GeoCoord(lat,lon);
    }

    inline double GetAngle() const
    {
      return 0;
    }

    inline Magnification GetMagnification() const
    {
      return magnification;
    }

    inline double GetDPI() const
    {
      return dpi;
    }

    bool Set(size_t tileX, size_t tileY,
             const Magnification& magnification,
             size_t width, size_t height)
    {
      return Set(tileX,tileY,magnification,GetDPI(),width,height);
    }

    bool Set(size_t tileX, size_t tileY,
             const Magnification& magnification,
             double dpi,
             size_t width, size_t height);

    bool Set(size_t tileAX, size_t tileAY,
             size_t tileBX, size_t tileBY,
             const Magnification& magnification,
             double dpi,
             size_t width, size_t height);

    bool GeoIsIn(double lon, double lat) const;
    bool GeoIsIn(double lonMin, double latMin,
                 double lonMax, double latMax) const;

    bool PixelToGeo(double x, double y,
                    double& lon, double& lat) const;

    void GeoToPixel(double lon, double lat,
                    double& x, double& y) const;

    void GeoToPixel(const GeoCoord& coord,
                    double& x, double& y) const;

    bool GetDimensions(GeoBox& boundingBox) const;

    inline double GetPixelSize() const
    {
      return pixelSize;
    }

    inline double GetMeterInPixel() const
    {
      return meterInPixel;
    }

    inline double GetMeterInMM() const
    {
      return meterInMM;
    }

  protected:

     void GeoToPixel(const BatchTransformer& transformData) const;
  };
}

#endif
