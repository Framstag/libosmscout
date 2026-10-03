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

#include <cstddef>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>

#if defined(__WIN32__) || defined(WIN32)
  #include <cairo.h>
#elif defined(__APPLE__) && __APPLE__
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
      CairoFont   font;
      double width;    //!< ink width of the glyph
      double height;   //!< ink height of the glyph
      double xBearing; //!< x_distance from the glyph base point to the left edge of the ink
      double yBearing; //!< vertical distance from the glyph base point (baseline) to the top edge of the ink
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

    /**
     * Key of a resolved font: the requested font name and the size the font is resolved at.
     * Both are inputs of the resolved font, so a font resolved for one name must not be
     * served for a request with another name.
     */
    struct FontKey
    {
      std::string fontName;
      double      fontSize;

      bool operator==(const FontKey& other) const
      {
        return fontName==other.fontName &&
               fontSize==other.fontSize;
      }
    };

    struct FontKeyHash
    {
      size_t operator()(const FontKey& key) const
      {
        return std::hash<std::string>()(key.fontName) ^
               (std::hash<double>()(key.fontSize) << 1);
      }
    };

    using FontMap = std::unordered_map<FontKey,CairoFont,FontKeyHash>;    //! Map type for mapping a font name and a font size to a font

    cairo_t                                *draw;            //! The cairo cairo_t for the mask
    std::vector<cairo_surface_t*>          images;           //! vector of cairo surfaces for icons
    std::vector<cairo_surface_t*>          patternImages;    //! vector of cairo surfaces for patterns
    std::vector<cairo_pattern_t*>          patterns;         //! cairo pattern structure for patterns
    FontMap                                fonts;            //! Cached scaled font
    size_t                                 resolvedFontCount{0}; //!< Fonts resolved since construction (diagnostic for tests)
    double                                 minimumLineWidth; //! Minimum width a line must have to be visible

    std::mutex                             mutex;            //! Mutex for locking concurrent calls

#if defined(OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO)
    PangoFontMap                           *fontMap{nullptr};       //!< Font map of this painter, holding the configured font file
    PangoContext                           *fontContext{nullptr};   //!< Context of that map, shared by the layouts of this painter
    std::string                            fontMapFile;             //!< Font file added to the font map, empty if none was
#endif

  private:
    CairoFont GetFont(const Projection& projection,
                 const MapParameter& parameter,
                 double fontSize);

#if defined(OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO)
    /**
     * The font map of this painter, created and kept in step with the configured font.
     *
     * The map belongs to the painter and a configured font file is added to it, so that the
     * family the file holds resolves to that file. The addition is confined to that map: the
     * font map API adds a file to the configuration of that map, not to the font configuration
     * of the process (where it is available at all).
     */
    PangoFontMap* GetFontMap(const MapParameter& parameter);

    /**
     * Create a layout on the font map of this painter and bind it to the current drawing
     * target, so that the layout takes its scale and its font options from the target exactly
     * as a layout created with pango_cairo_create_layout() would.
     */
    PangoLayout* CreateLayout(const MapParameter& parameter);

    /**
     * Release the font map and its context, if this painter has them.
     */
    void ReleaseFontMap();
#endif

    /**
     * Return the environment the label measurements of this painter depend on: the state that
     * is not an argument of the label layouter's Layout() call - the resolved font, the
     * resolution of the projection and the font settings of the current drawing target. The
     * label layouter drops its remembered measurements when the environment changes.
     */
    std::string GetMeasurementEnvironment(const Projection& projection,
                                          const MapParameter& parameter) const;

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

    osmscout::ScreenVectorRectangle GlyphBoundingBox(const CairoNativeGlyph &glyph) const;

    void DrawLabel(const Projection& projection,
                   const MapParameter& parameter,
                   const ScreenVectorRectangle& labelRectangle,
                   const LabelData& label,
                   const CairoNativeLabel& layout);

    void DrawGlyphs(const Projection &projection,
                    const MapParameter &parameter,
                    const osmscout::PathTextStyleRef& style,
                    const std::vector<CairoGlyph> &glyphs);

    void StyleSheetChanged(const Projection& projection,
                           const MapParameter& parameter,
                           const std::vector<MapData>& data) override;

    void BeforeDrawingCallback(const Projection& projection,
                               const MapParameter& parameter,
                               const std::vector<MapData>& data) override;

    /**
      Register regular label with given text at the given pixel coordinate
      in a style defined by the given LabelStyle.
     */
    void RegisterRegularLabel(const Projection& projection,
                              const MapParameter& parameter,
                              bool basemap,
                              const ObjectFileRef& ref,
                              const std::vector<LabelData>& labels,
                              const Vertex2D& position,
                              double objectWidth) override;

    /**
     * Register contour label
     */
    void RegisterContourLabel(const Projection& projection,
                              const MapParameter& parameter,
                              bool basemap,
                              const ObjectFileRef& ref,
                              const PathLabelData& label,
                              const LabelPath& labelPath) override;

    void DrawLabels(const Projection& projection,
                    const MapParameter& parameter,
                    const std::vector<MapData>& data) override;

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
    MapPainterCairo();
    ~MapPainterCairo() override;

    TextMetrics MeasureText(const Projection& projection,
                            const MapParameter& parameter,
                            const std::string& text,
                            double fontSize) override;

    bool DrawMap(const Projection& projection,
                 const MapParameter& parameter,
                 const std::vector<MapData>& data,
                 cairo_t *draw,
                 RenderSteps startStep=RenderSteps::FirstStep,
                 RenderSteps endStep=RenderSteps::LastStep);

    /**
     * Number of fonts this painter has resolved since it was created. A diagnostic for tests:
     * a font resolved for one font name must not be reused for a request with another name, so
     * a request with a new name resolves a new font. Carries no rendering behaviour.
     */
    size_t GetResolvedFontCount() const;
  };
}

#endif
