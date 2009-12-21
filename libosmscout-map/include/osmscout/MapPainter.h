#ifndef MAPPAINTER_H
#define MAPPAINTER_H

/*
  TravelJinni - Openstreetmap offline viewer
  Copyright (C) 2009  Tim Teulings

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <set>

#include <cairo/cairo.h>

#include <osmscout/StyleConfig.h>
#include <osmscout/TypeConfig.h>

#include <osmscout/Database.h>

class MapPainter
{
private:
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

  // helper struct for drawing
  std::vector<bool>   drawNode;      //! This nodes will be drawn
  std::vector<bool>   outNode;       //! This nodes is out of the visible area
  std::vector<double> nodeX;
  std::vector<double> nodeY;
  std::vector<double> lineWidth;     //! line with for this way line style
  std::vector<bool>   outline;       //! We draw an outline for this way line style
  std::vector<double> borderWidth;   //! border with for this way (area) border style

  void DrawLabel(cairo_t* draw,
                 double magnification,
                 const LabelStyle& style,
                 const std::string& text,
                 double x, double y);

  void DrawContourLabel(cairo_t* draw,
                        const LabelStyle& style,
                        const std::string& text,
                        const std::vector<Point>& nodes);

  void DrawSymbol(cairo_t* draw,
                  const SymbolStyle* style,
                  double x, double y);

  void SetLineStyle(cairo_t* draw,
                    double lineWidth,
                    const LineStyle& style);
  void SetLinePatternStyle(cairo_t* draw,
                           double lineWidth,
                           const LineStyle& style);

public:
  std::list<Way> poiWays;

public:
  MapPainter(const Database& database);
  virtual ~MapPainter();


  bool DrawMap(const StyleConfig& styleConfig,
               double lon, double lat,
               double magnification,
               size_t width, size_t height,
               cairo_surface_t *image,
               cairo_t *draw);

  bool PrintMap(const StyleConfig& styleConfig,
                double lon, double lat,
                double magnification,
                size_t width, size_t height);

  bool transformPixelToGeo(int x, int y,
                           double& lon, double& lat);

  bool transformGeoToPixel(double lon, double lat,
                           int& x, int& y);

  static void GetDimensions(double lon, double lat,
                            double magnification,
                            size_t width, size_t height,
                            double& lonMin, double& latMin,
                            double& lonMax, double& latMax);

  static bool transformPixelToGeo(int x, int y,
                                  double centerLon, double centerLat,
                                  double magnification,
                                  size_t width, size_t height,
                                  double& outLon, double& outLat);
  static bool transformGeoToPixel(double lon, double lat,
                                  double centerLon, double centerLat,
                                  double magnification,
                                  size_t width, size_t height,
                                  double &x, double& y);
};

#endif
