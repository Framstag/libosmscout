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
      //this should be private, but that would exclude future projection
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

      inline bool GeoToPixel(double lon,
                             double lat,
                             double& x,
                             double& y)
      {
#ifdef OSMSCOUT_HAVE_SSE2

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

#else
        return projection.GeoToPixel(lon,lat,x,y);
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
     * Returns the width of the screen
     */
    virtual size_t GetWidth() const = 0;
    /**
     * Returns the height of the screen
     */
    virtual size_t GetHeight() const = 0;

    /**
     * Returns minimum longitude value of the area covered by the projection.
     */
    virtual double GetLonMin() const = 0;

    /**
     * Returns minimum latitude value of the area covered by the projection.
     */
    virtual double GetLatMin() const = 0;

    /**
     * Returns maximum longitude value of the area covered by the projection.
     */
    virtual double GetLonMax() const = 0;

    /**
     * Returns maximum latitude value of the area covered by the projection.
     */
    virtual double GetLatMax() const = 0;

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
    virtual bool GeoToPixel(double lon, double lat,
                            double& x, double& y) const = 0;

    /**
     * Returns the bounding box of the area covered
     */
    virtual bool GetDimensions(double& lonMin, double& latMin,
                               double& lonMax, double& latMax) const = 0;

    /**
     * Returns the size of a pixel in meter
     */
    virtual double GetPixelSize() const = 0;

  protected:
    virtual bool GeoToPixel(const BatchTransformer& transformData) const = 0;

    friend class BatchTransformer;
  };

  /**
   * \ingroup Geometry
   *
   * Original mercator projection.
   */
  class OSMSCOUT_API MercatorProjection : public Projection
  {
  protected:
    bool                valid;         //! projects is valid

    double              lon;           //! Longitude coordinate of the center of the image
    double              lat;           //! Latitude coordinate of the center of the image
    Magnification       magnification; //! Current magnification
    double              dpi;           //! Screen DPI
    size_t              width;         //! Width of image
    size_t              height;        //! Height of image

    double              lonMin;        //! Longitude of the upper left corner of the image
    double              latMin;        //! Latitude of the upper left corner of the image
    double              lonMax;        //! Longitude of the lower right corner of the image
    double              latMax;        //! Latitude of the lower right corner of the image

    double              lonOffset;
    double              latOffset;
    double              scale;
    double              scaleGradtorad; //!Precalculated scale*Gradtorad

#ifdef OSMSCOUT_HAVE_SSE2
      //some extra vars for special sse needs
      v2df              sse2LonOffset;
      v2df              sse2LatOffset;
      v2df              sse2Scale;
      v2df              sse2ScaleGradtorad;
      v2df              sse2Height;
#endif

  private:

    double              pixelSize;     //! Size of a pixel in meter

  public:
    MercatorProjection();

    inline double GetLon() const
    {
      return lon;
    }

    inline double GetLat() const
    {
      return lat;
    }

    inline size_t GetWidth() const
    {
      return width;
    }

    inline size_t GetHeight() const
    {
      return height;
    }

    inline double GetLonMin() const
    {
      return lonMin;
    }

    inline double GetLatMin() const
    {
      return latMin;
    }

    inline double GetLonMax() const
    {
      return lonMax;
    }

    inline double GetLatMax() const
    {
      return latMax;
    }

    inline Magnification GetMagnification() const
    {
      return magnification;
    }

    inline double GetDPI() const
    {
      return dpi;
    }

    /**
     * Set the "screen" for the projection.
     *
     * @param lon Longitude coordinate of the center of the screen/image
     * @param lat Latitude coordinate of the center of the screen/image
     * @param magnification Magnification to apply
     * @param width Width of screen/image
     * @param height Height of screen/image
     */
    bool Set(double lon, double lat,
             const Magnification& magnification,
             size_t width, size_t height)
    {
      return Set(lon,lat,magnification,GetDPI(),width,height);
    }

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
    bool Set(double lon, double lat,
             const Magnification& magnification,
             double dpi,
             size_t width, size_t height);

    bool GeoIsIn(double lon, double lat) const;
    bool GeoIsIn(double lonMin, double latMin,
                 double lonMax, double latMax) const;

    bool PixelToGeo(double x, double y,
                    double& lon, double& lat) const;

    bool GeoToPixel(double lon, double lat,
                    double& x, double& y) const;

    bool GetDimensions(double& lonMin, double& latMin,
                       double& lonMax, double& latMax) const;

    double GetPixelSize() const;

  protected:
     bool GeoToPixel(const BatchTransformer& transformData) const;
  };

  /**
   * \ingroup Geometry
   */
  class OSMSCOUT_API ReversedYAxisMercatorProjection : public MercatorProjection
  {
  private:
    bool PixelToGeo(double x, double y, double& lon, double& lat) const;
    bool GeoToPixel(double lon, double lat, double& x, double& y) const;
  protected:
    bool GeoToPixel(const BatchTransformer& transformData) const;
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
  class OSMSCOUT_API Mercator2Projection : public Projection
  {
  protected:
    bool                valid;         //! projection is valid

    double              lon;           //! Longitude coordinate of the center of the image
    double              lat;           //! Latitude coordinate of the center of the image
    Magnification       magnification; //! Current magnification
    double              dpi;           //! Screen DPI
    size_t              width;         //! Width of image
    size_t              height;        //! Height of image

    double              lonMin;        //! Longitude of the upper left corner of the image
    double              latMin;        //! Latitude of the upper left corner of the image
    double              lonMax;        //! Longitude of the lower right corner of the image
    double              latMax;        //! Latitude of the lower right corner of the image

    double              lonOffset;
    double              latOffset;
    double              scale;
    double              scaleGradtorad; //!Precalculated scale*Gradtorad
    double              pixelSize;     //! Size of a pixel in meter

#ifdef OSMSCOUT_HAVE_SSE2
      //some extra vars for special sse needs
      v2df              sse2LonOffset;
      v2df              sse2LatOffset;
      v2df              sse2Scale;
      v2df              sse2ScaleGradtorad;
      v2df              sse2Height;
#endif

  private:


  public:
    Mercator2Projection();

    inline double GetLon() const
    {
      return lon;
    }

    inline double GetLat() const
    {
      return lat;
    }

    inline size_t GetWidth() const
    {
      return width;
    }

    inline size_t GetHeight() const
    {
      return height;
    }

    inline double GetLonMin() const
    {
      return lonMin;
    }

    inline double GetLatMin() const
    {
      return latMin;
    }

    inline double GetLonMax() const
    {
      return lonMax;
    }

    inline double GetLatMax() const
    {
      return latMax;
    }

    inline Magnification GetMagnification() const
    {
      return magnification;
    }

    inline double GetDPI() const
    {
      return dpi;
    }

    bool Set(double lon, double lat,
             const Magnification& magnification,
             size_t width, size_t height)
    {
      return Set(lon,lat,magnification,GetDPI(),width,height);
    }

    bool Set(double lon, double lat,
             const Magnification& magnification,
             double dpi,
             size_t width, size_t height);

    bool GeoIsIn(double lon, double lat) const;
    bool GeoIsIn(double lonMin, double latMin,
                 double lonMax, double latMax) const;

    bool PixelToGeo(double x, double y,
                    double& lon, double& lat) const;

    bool GeoToPixel(double lon, double lat,
                    double& x, double& y) const;

    bool GetDimensions(double& lonMin, double& latMin,
                       double& lonMax, double& latMax) const;

    double GetPixelSize() const;

  protected:
     bool GeoToPixel(const BatchTransformer& transformData) const;
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
    bool                valid;         //! projection is valid

    size_t              tileX;         //! X coordinate of tile
    size_t              tileY;         //! Y coordinate of tile
    double              lon;           //! Longitude coordinate of the center of the image
    double              lat;           //! Latitude coordinate of the center of the image
    Magnification       magnification; //! Current magnification
    double              dpi;           //! Screen DPI
    size_t              width;         //! Width of image
    size_t              height;        //! Height of image

    double              lonMin;        //! Longitude of the upper left corner of the image
    double              latMin;        //! Latitude of the upper left corner of the image
    double              lonMax;        //! Longitude of the lower right corner of the image
    double              latMax;        //! Latitude of the lower right corner of the image

    double              lonOffset;
    double              latOffset;
    double              scale;
    double              scaleGradtorad; //!Precalculated scale*Gradtorad
    double              pixelSize;     //! Size of a pixel in meter

#ifdef OSMSCOUT_HAVE_SSE2
      //some extra vars for special sse needs
      v2df              sse2LonOffset;
      v2df              sse2LatOffset;
      v2df              sse2Scale;
      v2df              sse2ScaleGradtorad;
      v2df              sse2Height;
#endif

  private:


  public:
    TileProjection();

    inline double GetTileX() const
    {
      return tileX;
    }

    inline double GetTileY() const
    {
      return tileY;
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

    inline double GetLonMin() const
    {
      return lonMin;
    }

    inline double GetLatMin() const
    {
      return latMin;
    }

    inline double GetLonMax() const
    {
      return lonMax;
    }

    inline double GetLatMax() const
    {
      return latMax;
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

    bool GeoIsIn(double lon, double lat) const;
    bool GeoIsIn(double lonMin, double latMin,
                 double lonMax, double latMax) const;

    bool PixelToGeo(double x, double y,
                    double& lon, double& lat) const;

    bool GeoToPixel(double lon, double lat,
                    double& x, double& y) const;

    bool GetDimensions(double& lonMin, double& latMin,
                       double& lonMax, double& latMax) const;

  protected:
     bool GeoToPixel(const BatchTransformer& transformData) const;

     double GetPixelSize() const;
  };
}

#endif
