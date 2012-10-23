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

#include <cstdlib>

#include <osmscout/private/CoreImportExport.h>

#include <osmscout/CoreFeatures.h>

#ifdef OSMSCOUT_HAVE_SSE2
#include <osmscout/system/SSEMathPublic.h>
#endif

namespace osmscout {

  class OSMSCOUT_API Projection
  {
  public:

    /* This class is used to hide internal complexity concerned with batching GeoToPixel calls*/
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

      ~BatchTransformer()
      {
        Flush();
      }

      bool GeoToPixel(double lon,
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

        if (count==2)
        {
          count=0;
          return projection.GeoToPixel(*this);
        }

        return true;

#else

        return projection.GeoToPixel(lon, lat, x, y);

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

    virtual double GetLon() const = 0;
    virtual double GetLat() const = 0;
    virtual size_t GetWidth() const = 0;
    virtual size_t GetHeight() const = 0;
    virtual double GetLonMin() const = 0;
    virtual double GetLatMin() const = 0;
    virtual double GetLonMax() const = 0;
    virtual double GetLatMax() const = 0;
    virtual double GetMagnification() const = 0;

    virtual bool Set(double lon, double lat,
                     double magnification,
                     size_t width, size_t height) = 0;

    virtual bool GeoIsIn(double lon, double lat) const = 0;
    virtual bool GeoIsIn(double lonMin, double latMin,
                         double lonMax, double latMax) const = 0;

    virtual bool PixelToGeo(double x, double y,
                            double& lon, double& lat) const = 0;

    virtual bool GeoToPixel(double lon, double lat,
                            double& x, double& y) const = 0;

    virtual bool GetDimensions(double& lonMin, double& latMin,
                               double& lonMax, double& latMax) const = 0;

    virtual double GetPixelSize() const = 0;

  protected:

    virtual bool GeoToPixel(const BatchTransformer& transformData) const = 0;

    friend class BatchTransformer;
  };

  class OSMSCOUT_API MercatorProjection : public Projection
  {
  protected:
    bool                valid;         //! projects is valid
    double              lon;           //! Longitude coordinate of the center of the image
    double              lat;           //! Latitude coordinate of the center of the image
    double              lonMin;        //! Longitude of the upper left corner of the image
    double              latMin;        //! Latitude of the upper left corner of the image
    double              lonMax;        //! Longitude of the lower right corner of the image
    double              latMax;        //! Latitude of the lower right corner of the image

    size_t              width;         //! Width of image
    size_t              height;        //! Height of image

    double              lonOffset;
    double              latOffset;
    double              scale;
    double              scaleGradtorad; //!Precalculated scale*Gradtorad
      
#ifdef OSMSCOUT_HAVE_SSE2
      //some extra vars for special sse needs
      v2df                sse2LonOffset;
      v2df                sse2LatOffset;
      v2df                sse2Scale;
      v2df                sse2ScaleGradtorad;
      v2df                sse2Height;
#endif

  private:
    double              magnification; //! Current maginification

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

    inline double GetMagnification() const
    {
      return magnification;
    }

    bool Set(double lon, double lat,
             double magnification,
             size_t width, size_t height);

    bool Set(double lonMin, double latMin,
             double lonMax, double latMax,
             double magnification,
             size_t width);

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
    
  class OSMSCOUT_API ReversedYAxisMercatorProjection : public MercatorProjection
  {
    bool PixelToGeo(double x, double y, double& lon, double& lat) const;
    bool GeoToPixel(double lon, double lat, double& x, double& y) const;
  protected:
    bool GeoToPixel(const BatchTransformer& transformData) const;
  };
}

#endif
