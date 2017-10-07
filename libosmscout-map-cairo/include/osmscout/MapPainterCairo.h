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

#include <osmscout/MapCairoFeatures.h>

#include <mutex>
#include <unordered_map>

#if defined(__WIN32__) || defined(WIN32) || defined(__APPLE__)
  #include <cairo.h>
#else
  #include <cairo/cairo.h>
#endif

#if defined(OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO)
  #include <pango/pangocairo.h>
#endif

#include <osmscout/private/MapCairoImportExport.h>

#include <osmscout/MapPainter.h>

namespace osmscout {

  class OSMSCOUT_MAP_CAIRO_API MapPainterCairo : public MapPainter
  {
#if defined(OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO)
    typedef PangoFontDescription*          Font;
#else
    typedef cairo_scaled_font_t*           Font;
#endif
    typedef std::unordered_map<size_t,Font>  FontMap;          //! Map type for mapping  font sizes to font

    cairo_t                                *draw;            //! The cairo cairo_t for the mask
    std::vector<cairo_surface_t*>          images;           //! vector of cairo surfaces for icons
    std::vector<cairo_surface_t*>          patternImages;    //! vector of cairo surfaces for patterns
    std::vector<cairo_pattern_t*>          patterns;         //! cairo pattern structure for patterns
    FontMap                                fonts;            //! Cached scaled font
    double                                 minimumLineWidth; //! Minimum width a line must have to be visible

    std::mutex                             mutex;            //! Mutex for locking concurrent calls

  private:
    Font GetFont(const Projection& projection,
                 const MapParameter& parameter,
                 double fontSize);

    void SetLineAttributes(const Color& color,
                           double width,
                           const std::vector<double>& dash);

    void DrawFillStyle(const Projection& projection,
                       const MapParameter& parameter,
                       const FillStyleRef& fill,
                       const BorderStyleRef& border);

  protected:
    bool HasIcon(const StyleConfig& styleConfig,
                 const MapParameter& parameter,
                 IconStyle& style) override;

    bool HasPattern(const MapParameter& parameter,
                    const FillStyle& style);

    double GetFontHeight(const Projection& projection,
                       const MapParameter& parameter,
                       double fontSize) override;

    TextDimension GetTextDimension(const Projection& projection,
                                   const MapParameter& parameter,
                                   double objectWidth,
                                   double fontSize,
                                   const std::string& text) override;

    void DrawGround(const Projection& projection,
                    const MapParameter& parameter,
                    const FillStyle& style) override;

    void DrawLabel(const Projection& projection,
                   const MapParameter& parameter,
                   const LabelData& label) override;

    void DrawPrimitivePath(const Projection& projection,
                           const MapParameter& parameter,
                           const DrawPrimitiveRef& primitive,
                           double x, double y,
                           double minX,
                           double minY,
                           double maxX,
                           double maxY);

    void DrawSymbol(const Projection& projection,
                    const MapParameter& parameter,
                    const Symbol& symbol,
                    double x, double y) override;

    void DrawIcon(const IconStyle* style,
                  double x, double y) override;

    void DrawPath(const Projection& projection,
                  const MapParameter& parameter,
                  const Color& color,
                  double width,
                  const std::vector<double>& dash,
                  LineStyle::CapStyle startCap,
                  LineStyle::CapStyle endCap,
                  size_t transStart, size_t transEnd) override;

    void DrawContourLabel(const Projection& projection,
                          const MapParameter& parameter,
                          const PathTextStyle& style,
                          const std::string& text,
                          size_t transStart, size_t transEnd,
                          ContourLabelHelper& helper) override;

    void DrawContourSymbol(const Projection& projection,
                           const MapParameter& parameter,
                           const Symbol& symbol,
                           double space,
                           size_t transStart, size_t transEnd) override;

    void DrawArea(const Projection& projection,
                  const MapParameter& parameter,
                  const AreaData& area) override;

  public:
    explicit MapPainterCairo(const StyleConfigRef& styleConfig);
    ~MapPainterCairo() override;


    bool DrawMap(const Projection& projection,
                 const MapParameter& parameter,
                 const MapData& data,
                 cairo_t *draw);
  };
}

#endif
