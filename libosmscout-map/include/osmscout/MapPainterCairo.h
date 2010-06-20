#ifndef OSMSCOUT_MAP_MAPPAINTERCAIRO_H
#define OSMSCOUT_MAP_MAPPAINTERCAIRO_H

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

#if defined(__WIN32__) || defined(WIN32) || defined(__APPLE__)
  #include <cairo.h>
#else
  #include <cairo/cairo.h>
#endif

#include <osmscout/Private/MapImportExport.h>

#include <osmscout/MapPainter.h>

namespace osmscout {

  class OSMSCOUT_MAP_API MapPainterCairo : public MapPainter
  {
  private:
    // helper struct for drawing
    std::vector<double>               borderWidth;  //! border with for this way (area) border style
    std::map<size_t,cairo_scaled_font_t*> font;     //! Cached scaled font

    // Image handling
    std::vector<cairo_surface_t*>     images;       //! vector of cairo surfaces for images and patterns
    std::vector<cairo_pattern_t*>     patterns;     //! cairo pattern structure for patterns

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

    bool CheckImage(const StyleConfig& styleConfig,
                    IconStyle& style);
    bool CheckImage(const StyleConfig& styleConfig,
                    PatternStyle& style);

    void DrawLabel(cairo_t* draw,
                   const Projection& projection,
                   const LabelStyle& style,
                   const std::string& text,
                   double x, double y);

    void DrawTiledLabel(cairo_t* draw,
                        const Projection& projection,
                        const LabelStyle& style,
                        const std::string& label,
                        const std::vector<Point>& nodes,
                        std::set<size_t>& tileBlacklist);

    void DrawContourLabel(cairo_t* draw,
                          const Projection& projection,
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
                  const Projection& projection,
                  double r, double g, double b, double a,
                  double width,
                  const std::vector<Point>& nodes);

    void FillRegion(const std::vector<Point>& nodes,
                    const Projection& projection,
                    const FillStyle& style);

    void FillRegion(const std::vector<Point>& nodes,
                    const Projection& projection,
                    PatternStyle& style);

    void DrawWayOutline(const StyleConfig& styleConfig,
                        const Projection& projection,
                        TypeId type,
                        double width,
                        bool isBridge,
                        bool isTunnel,
                        bool startIsJoint,
                        bool endIsJoint,
                        const std::vector<Point>& nodes);
    void DrawWay(const StyleConfig& styleConfig,
                 const Projection& projection,
                 TypeId type,
                 double width,
                 bool isBridge,
                 bool isTunnel,
                 const std::vector<Point>& nodes);

    void DrawArea(const StyleConfig& styleConfig,
                  const Projection& projection,
                  TypeId type,
                  int layer,
                  bool isBuilding,
                  const std::vector<Point>& nodes);

    void DrawAreas(const StyleConfig& styleConfig,
                   const Projection& projection,
                   const std::vector<Way>& areas,
                   const std::vector<Relation>& relationAreas);
    void DrawAreaLabels(const StyleConfig& styleConfig,
                        const Projection& projection,
                        const std::vector<Way>& areas,
                        const std::vector<Relation>& relationAreas);
                        
    void DrawWays(const StyleConfig& styleConfig,
                  const Projection& projection,
                  const std::vector<Way>& ways,
                  const std::vector<Relation>& relationWays);
    void DrawWayLabels(const StyleConfig& styleConfig,
                       const Projection& projection,
                       const std::vector<Way>& ways,
                       const std::vector<Relation>& relationWays);
                        
    void DrawNodes(const StyleConfig& styleConfig,
                   const Projection& projection,
                   const std::vector<Node>& areas);
                   
    void DrawPOIWays(const StyleConfig& styleConfig,
                     const Projection& projection,
                     const std::list<Way>& poiWays);
    void DrawPOINodes(const StyleConfig& styleConfig,
                      const Projection& projection,
                      const std::list<Node>& poiNodes);
    void DrawPOINodeLabels(const StyleConfig& styleConfig,
                           const Projection& projection,
                           const std::list<Node>& poiNodes);

  public:
    MapPainterCairo();
    ~MapPainterCairo();


    bool DrawMap(const StyleConfig& styleConfig,
                 const Projection& projection,
                 const MapParameter& parameter,
                 const MapData& data,
                 cairo_surface_t *image,
                 cairo_t *draw);
  };
}

#endif
