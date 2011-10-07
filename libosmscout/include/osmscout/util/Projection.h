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

namespace osmscout {

  class OSMSCOUT_API Projection
  {
  public:
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
  };

  class OSMSCOUT_API MercatorProjection : public Projection
  {
    bool                valid;         //! projects is valid
    double              lon;           //! Longitude coordinate of the center of the image
    double              lat;           //! Latitude coordinate of the center of the image
    double              lonMin;        //! Longitude of the upper left corner of the image
    double              latMin;        //! Latitude of the upper left corner of the image
    double              lonMax;        //! Longitude of the lower right corner of the image
    double              latMax;        //! Latitude of the lower right corner of the image
    double              magnification; //! Current maginification
    size_t              width;         //! Width of image
    size_t              height;        //! Height of image

    double              lonOffset;
    double              latOffset;
    double              scale;

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
  };
}

#endif
