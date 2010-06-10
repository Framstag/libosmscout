#ifndef MAPPAINTERCAIRO_H
#define MAPPAINTERCAIRO_H

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

#include <osmscout/MapPainter.h>

namespace osmscout {

  class MapPainterCairo : public MapPainter
  {
  private:
    // helper struct for drawing
    std::vector<bool>                 drawNode;     //! This nodes will be drawn
    std::vector<bool>                 outNode;      //! This nodes is out of the visible area
    std::vector<double>               nodeX;        //! static scratch buffer for calculation
    std::vector<double>               nodeY;        //! static scratch buffer for calculation
    std::vector<double>               borderWidth;  //! border with for this way (area) border style
    std::map<size_t,cairo_scaled_font_t*> font;     //! Cached scaled font

    // Image handling
    std::vector<cairo_surface_t*>     images;       //! vector of cairo surfaces for images and patterns
    std::vector<cairo_pattern_t*>     patterns;     //! cairo pattern structure for patterns

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
                    IconStyle& style);
    bool CheckImage(const StyleConfig& styleConfig,
                    PatternStyle& style);

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

    void DrawPath(LineStyle::Style style,
                  double r, double g, double b, double a,
                  double width,
                  const std::vector<Point>& nodes);

    void FillRegion(const std::vector<Point>& nodes,
                    const FillStyle& style);

    void FillRegion(const std::vector<Point>& nodes,
                    PatternStyle& style);

    void DrawWayOutline(const StyleConfig& styleConfig,
                        TypeId type,
                        double width,
                        bool isBridge,
                        bool isTunnel,
                        bool startIsJoint,
                        bool endIsJoint,
                        const std::vector<Point>& nodes);
    void DrawWay(const StyleConfig& styleConfig,
                 TypeId type,
                 double width,
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
    MapPainterCairo(const Database& database);
    virtual ~MapPainterCairo();


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
  };
}

#endif
