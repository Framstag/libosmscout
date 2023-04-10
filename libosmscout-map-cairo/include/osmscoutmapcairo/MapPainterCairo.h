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

#include <osmscoutmapcairo/MapCairoFeatures.h>

#include <mutex>
#include <unordered_map>

#if defined(__WIN32__) || defined(WIN32)
  #include <cairo.h>
#else
  #include <cairo/cairo.h>
#endif

#if defined(OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO)
  #include <pango/pangocairo.h>
  #include <pango/pango-glyph.h>
#endif

#include <osmscoutmapcairo/MapCairoImportExport.h>

#include <osmscoutmap/MapPainter.h>


namespace osmscout {

  class OSMSCOUT_MAP_CAIRO_API MapPainterCairo : public MapPainter
  {
  public:
#if defined(OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO)
    using CairoFont = PangoFontDescription*;
    using CairoNativeLabel = std::shared_ptr<PangoLayout>;
    struct PangoStandaloneGlyph {
      std::shared_ptr<PangoFont>        font;
      std::shared_ptr<PangoGlyphString> glyphString;
    };

    using CairoNativeGlyph = PangoStandaloneGlyph;
#else
    using CairoFont = cairo_scaled_font_t*;
    struct CairoNativeLabel {
      std::wstring          wstr;
      CairoFont             font;
      cairo_text_extents_t  textExtents;
      cairo_font_extents_t  fontExtents;
    };

    struct CairoNativeGlyph {
      std::string character;
      double width;
      double height;
    };
    //static constexpr double AverageCharacterWidth = 0.75;
#endif

    using CairoLabel = Label<CairoNativeGlyph, CairoNativeLabel>;
    using CairoGlyph = Glyph<CairoNativeGlyph>;
    using CairoLabelInstance = LabelInstance<CairoNativeGlyph, CairoNativeLabel>;
    using CairoLabelLayouter = LabelLayouter<CairoNativeGlyph, CairoNativeLabel, MapPainterCairo>;
    friend CairoLabelLayouter;

  private:
    CairoLabelLayouter labelLayouter;

    using FontMap = std::unordered_map<size_t,CairoFont>;    //! Map type for mapping font sizes to font

    cairo_t                                *draw;            //! The cairo cairo_t for the mask
    std::vector<cairo_surface_t*>          images;           //! vector of cairo surfaces for icons
    std::vector<cairo_surface_t*>          patternImages;    //! vector of cairo surfaces for patterns
    std::vector<cairo_pattern_t*>          patterns;         //! cairo pattern structure for patterns
    FontMap                                fonts;            //! Cached scaled font
    double                                 minimumLineWidth; //! Minimum width a line must have to be visible

    std::mutex                             mutex;            //! Mutex for locking concurrent calls

  private:
    CairoFont GetFont(const Projection& projection,
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
                 const Projection& projection,
                 const MapParameter& parameter,
                 IconStyle& style) override;

    bool HasPattern(const MapParameter& parameter,
                    const FillStyle& style);

    double GetFontHeight(const Projection& projection,
                       const MapParameter& parameter,
                       double fontSize) override;

    void DrawGround(const Projection& projection,
                    const MapParameter& parameter,
                    const FillStyle& style) override;

    std::shared_ptr<CairoLabel> Layout(const Projection& projection,
                                       const MapParameter& parameter,
                                       const std::string& text,
                                       double fontSize,
                                       double objectWidth,
                                       bool enableWrapping = false,
                                       bool contourLabel = false);

    osmscout::DoubleRectangle GlyphBoundingBox(const CairoNativeGlyph &glyph) const;

    void DrawLabel(const Projection& projection,
                   const MapParameter& parameter,
                   const DoubleRectangle& labelRectangle,
                   const LabelData& label,
                   const CairoNativeLabel& layout);

    void DrawGlyphs(const Projection &projection,
                    const MapParameter &parameter,
                    const osmscout::PathTextStyleRef& style,
                    const std::vector<CairoGlyph> &glyphs);

    void BeforeDrawing(const StyleConfig& styleConfig,
                       const Projection& projection,
                       const MapParameter& parameter,
                       const MapData& data) override;

    /**
      Register regular label with given text at the given pixel coordinate
      in a style defined by the given LabelStyle.
     */
    void RegisterRegularLabel(const Projection& projection,
                              const MapParameter& parameter,
                              const std::vector<LabelData>& labels,
                              const Vertex2D& position,
                              double objectWidth) override;

    /**
     * Register contour label
     */
    void RegisterContourLabel(const Projection& projection,
                              const MapParameter& parameter,
                              const PathLabelData& label,
                              const LabelPath& labelPath) override;

    void DrawLabels(const Projection& projection,
                    const MapParameter& parameter,
                    const MapData& data) override;

    void DrawSymbol(const Projection& projection,
                    const MapParameter& parameter,
                    const Symbol& symbol,
                    const Vertex2D& screenPos,
                    double scaleFactor) override;

    void DrawIcon(const IconStyle* style,
                  const Vertex2D& centerPos,
                  double width, double height) override;

    void DrawPath(const Projection& projection,
                  const MapParameter& parameter,
                  const Color& color,
                  double width,
                  const std::vector<double>& dash,
                  LineStyle::CapStyle startCap,
                  LineStyle::CapStyle endCap,
                  const CoordBufferRange& coordRange) override;

    void DrawContourSymbol(const Projection& projection,
                           const MapParameter& parameter,
                           const Symbol& symbol,
                           const ContourSymbolData& data) override;

    void DrawArea(const Projection& projection,
                  const MapParameter& parameter,
                  const AreaData& area) override;

  public:
    explicit MapPainterCairo(const StyleConfigRef& styleConfig);
    ~MapPainterCairo() override;


    bool DrawMap(const Projection& projection,
                 const MapParameter& parameter,
                 const MapData& data,
                 cairo_t *draw,
                 RenderSteps startStep=RenderSteps::FirstStep,
                 RenderSteps endStep=RenderSteps::LastStep);
  };
}

#endif
