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
    std::vector<bool>                 drawNode;     //! This nodes will be drawn
    std::vector<bool>                 outNode;      //! This nodes is out of the visible area
    std::vector<double>               nodeX;        //! static scratch buffer for calculation
    std::vector<double>               nodeY;        //! static scratch buffer for calculation
    std::vector<double>               lineWidth;    //! line with for this way line style
    std::vector<bool>                 outline;      //! We draw an outline for this way line style
    std::vector<double>               borderWidth;  //! border with for this way (area) border style
    std::map<size_t,cairo_scaled_font_t*> font;     //! Cached scaled font
    std::vector<cairo_surface_t*>     image;        //! List of internal images
    std::vector<bool>                 imageChecked; //! We have tried to load the internal image

    size_t                styleCount;

    std::vector<Node>     nodes;
    std::vector<Way>      ways;
    std::vector<Way>      areas;
    std::vector<Relation> relationWays;
    std::vector<Relation> relationAreas;

    bool                  areaLayers[11];
    bool                  wayLayers[11];
    bool                  relationAreaLayers[11];
    bool                  relationWayLayers[11];

    size_t                nodesDrawnCount;
    size_t                areasDrawnCount;
    size_t                waysDrawnCount;

    cairo_t               *draw;

  public:
    std::list<Way>        poiWays;
    std::list<Node>       poiNodes;

  private:
    cairo_scaled_font_t* GetScaledFont(cairo_t* draw,
                                       size_t fontSize);

    bool IsVisible(const std::vector<Point>& nodes) const;

    void TransformArea(const std::vector<Point>& nodes);
    void TransformWay(const std::vector<Point>& nodes);

    bool GetBoundingBox(const std::vector<Point>& nodes,
                        double& xmin, double& ymin,
                        double& xmax, double& ymax);
    bool GetCenterPixel(const std::vector<Point>& nodes,
                        double& cx,
                        double& cy);

    bool CheckImage(const StyleConfig& styleConfig,
                    IconStyle::Icon icon);

    void DrawLabel(cairo_t* draw,
                   double magnification,
                   const LabelStyle& style,
                   const std::string& text,
                   double x, double y);

    void DrawTiledLabel(cairo_t* draw,
                   double magnification,
                   const LabelStyle& style,
                   const std::string& label,
                   const std::vector<Point>& nodes,
                   std::set<size_t>& tileBlacklist);

    void DrawContourLabel(cairo_t* draw,
                          const LabelStyle& style,
                          const std::string& text,
                          const std::vector<Point>& nodes);

    void DrawSymbol(cairo_t* draw,
                    const SymbolStyle* style,
                    double x, double y);

    void DrawIcon(cairo_t* draw,
                  const IconStyle* style,
                  double x, double y);

    void DrawPath(const std::vector<Point>& nodes,
                  const LineStyle& style,
                  double width);

    void FillRegion(const std::vector<Point>& nodes,
                    const FillStyle& style);

    void DrawWayOutline(const StyleConfig& styleConfig,
                        TypeId type,
                        bool isBridge,
                        bool isTunnel,
                        bool startIsJoint,
                        bool endIsJoint,
                        const std::vector<Point>& nodes);
    void DrawWay(const StyleConfig& styleConfig,
                 TypeId type,
                 bool isBridge,
                 bool isTunnel,
                 const std::vector<Point>& nodes);

    void DrawArea(const StyleConfig& styleConfig,
                  TypeId type,
                  int layer,
                  bool isBuilding,
                  const std::vector<Point>& nodes);

    void DrawAreas(const StyleConfig& styleConfig);
    void DrawWays(const StyleConfig& styleConfig);
    void DrawWayLabels(const StyleConfig& styleConfig);
    void DrawNodes(const StyleConfig& styleConfig);
    void DrawAreaLabels(const StyleConfig& styleConfig);
    void DrawPOIWays(const StyleConfig& styleConfig);
    void DrawPOINodes(const StyleConfig& styleConfig);
    void DrawPOINodeLabels(const StyleConfig& styleConfig);

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

    bool TransformPixelToGeo(double x, double y,
                             double& lon, double& lat);

    bool TransformGeoToPixel(double lon, double lat,
                             double& x, double& y);

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
