#ifndef MAPPAINTER_H
#define MAPPAINTER_H

/*
  This source is part of the libosmscout-map library
  Copyright (C) 2009  Tim Teulings

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

#include <set>

#include <cairo/cairo.h>

#include <osmscout/StyleConfig.h>
#include <osmscout/TypeConfig.h>

#include <osmscout/Database.h>

namespace osmscout {

  class MapPainter
  {
  protected:
    const Database& database;

    // current drawing context information
    // These values are valid until the next image gets drawn
    double              lon;           //! Longitude coordinate of the center of the image
    double              lat;           //! Latitude coordinate of the center of the image
    double              lonMin;        //! Longitude of the upper left corner of the image
    double              latMin;        //! Latitude of the upper left corner of the image
    double              lonMax;        //! Longitude of the lower right corner of the image
    double              latMax;        //! Latitude of the lower right corner of the image
    double              magnification; //! Current maginification
    size_t              width;         //! Width fo image
    size_t              height;        //! Height of image
    double              hmin;
    double              hmax;
    double              vmin;
    double              vmax;

    double              hscale;
    double              vscale;

    double              pixelSize;     //! Size of a pixel in meter

  public:
    std::list<Way>        poiWays;
    std::list<Node>       poiNodes;

  protected:
    void RecalculateData(double lon, double lat,
                         double magnification,
                         size_t width, size_t height);

  public:
    MapPainter(const Database& database);
    virtual ~MapPainter();

    bool TransformPixelToGeo(double x, double y,
                             double& lon, double& lat);

    bool TransformGeoToPixel(double lon, double lat,
                             double& x, double& y);

    virtual bool DrawMap(const StyleConfig& styleConfig,
                         double lon, double lat,
                         double magnification,
                         size_t width, size_t height,
                         cairo_surface_t *image,
                         cairo_t *draw) = 0;
                         
    static void GetDimensions(double lon, double lat,
                              double magnification,
                              size_t width, size_t height,
                              double& lonMin, double& latMin,
                              double& lonMax, double& latMax);
                         
    static bool TransformPixelToGeo(int x, int y,
                                    double centerLon, double centerLat,
                                    double magnification,
                                    size_t width, size_t height,
                                    double& outLon, double& outLat);
                                    
    static bool TransformGeoToPixel(double lon, double lat,
                                    double centerLon, double centerLat,
                                    double magnification,
                                    size_t width, size_t height,
                                    double &x, double& y);
  };
}

#endif
