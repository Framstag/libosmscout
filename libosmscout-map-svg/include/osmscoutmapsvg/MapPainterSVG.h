#ifndef OSMSCOUT_MAP_MAPPAINTERSVG_H
#define OSMSCOUT_MAP_MAPPAINTERSVG_H

/*
  This source is part of the libosmscout-map library
  Copyright (C) 2011  Tim Teulings

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

#include <map>
#include <mutex>
#include <optional>
#include <ostream>
#include <set>
#include <unordered_map>

#include <osmscoutmapsvg/MapSVGFeatures.h>

#if defined(OSMSCOUT_MAP_SVG_HAVE_LIB_PANGO)
  #include <pango/pangoft2.h>
#endif

#include <osmscoutmapsvg/MapSVGImportExport.h>

#include <osmscoutmap/MapPainter.h>

namespace osmscout {

  class OSMSCOUT_MAP_SVG_API MapPainterSVG : public MapPainter
  {
#if defined(OSMSCOUT_MAP_SVG_HAVE_LIB_PANGO)
  public:
    using NativeLabel = std::shared_ptr<PangoLayout>;
    struct StandaloneGlyph {
      std::shared_ptr<PangoFont>        font;
      std::shared_ptr<PangoGlyphString> glyphString;
      std::string character;
    };

    using NativeGlyph = StandaloneGlyph;

  private:
    using FontMap = std::unordered_map<size_t,PangoFontDescription*>  ;          //! Map type for mapping  font sizes to font
    PangoFontMap                     *pangoFontMap;
    PangoContext                     *pangoContext;
    FontMap                          fonts;            //! Cached scaled font

#else
  public:
    using NativeLabel = std::wstring;
    struct NativeGlyph {
      std::string character;
      double width;
      double height;
    };
    static constexpr double AverageCharacterWidth = 0.75;
#endif

  public:
    using SvgLabel = Label<NativeGlyph, NativeLabel>;

  private:
    using SvgGlyph = Glyph<NativeGlyph>;
    using SvgLabelInstance = LabelInstance<NativeGlyph, NativeLabel>;
    using SvgLabelLayouter = LabelLayouter<NativeGlyph, NativeLabel, MapPainterSVG>;
    friend SvgLabelLayouter;

    SvgLabelLayouter                  labelLayouter;

    std::map<FillStyle,std::string>   fillStyleNameMap;
    std::map<BorderStyle,std::string> borderStyleNameMap;
    std::map<LineStyle,std::string>   lineStyleNameMap;

    std::ostream                      stream;
    TypeConfigRef                     typeConfig;
    std::mutex                        mutex;         //! Mutex for locking concurrent calls

    std::vector<std::string>          images;

  private:
    std::string GetColorValue(const Color& color);

    osmscout::DoubleRectangle GlyphBoundingBox(const NativeGlyph &glyph) const;

    std::shared_ptr<SvgLabel> Layout(const Projection& projection,
                                     const MapParameter& parameter,
                                     const std::string& text,
                                     double fontSize,
                                     double objectWidth,
                                     bool enableWrapping = false,
                                     bool contourLabel = false);

    void DrawLabel(const Projection& projection,
                   const MapParameter& parameter,
                   const DoubleRectangle& labelRectangle,
                   const LabelData& label,
                   const NativeLabel& layout);

    void DrawGlyphs(const Projection &projection,
                    const MapParameter &parameter,
                    const osmscout::PathTextStyleRef style,
                    const std::vector<SvgGlyph> &glyphs);

#if defined(OSMSCOUT_MAP_SVG_HAVE_LIB_PANGO)
    PangoFontDescription* GetFont(const Projection& projection,
                                  const MapParameter& parameter,
                                  double fontSize);

#endif

    void SetupFillAndStroke(const FillStyleRef &fillStyle,
                                           const BorderStyleRef &borderStyle);

    void WriteHeader(size_t width,size_t height);
    void DumpStyles(const StyleConfig& styleConfig,
                    const MapParameter& parameter,
                    const Projection& projection);
    void WriteFooter();

    void StartMainGroup();
    void FinishMainGroup();

    std::string StrEscape(const std::string &str) const;

    void IconData(const Projection& projection,
                  const MapParameter& parameter);

  protected:
    void AfterPreprocessing(const StyleConfig& styleConfig,
                            const Projection& projection,
                            const MapParameter& parameter,
                            const MapData& data) override;

    void BeforeDrawing(const StyleConfig& styleConfig,
                       const Projection& projection,
                       const MapParameter& parameter,
                       const MapData& data) override;

    void AfterDrawing(const StyleConfig& styleConfig,
                      const Projection& projection,
                      const MapParameter& parameter,
                      const MapData& data) override;

    bool HasIcon(const StyleConfig& styleConfig,
                 const Projection& projection,
                 const MapParameter& parameter,
                 IconStyle& style) override;

    double GetFontHeight(const Projection& projection,
                         const MapParameter& parameter,
                         double fontSize) override;

    void DrawGround(const Projection& projection,
                    const MapParameter& parameter,
                    const FillStyle& style) override;

    /**
      Register regular label with given text at the given pixel coordinate
      in a style defined by the given LabelStyle.
     */
    virtual void RegisterRegularLabel(const Projection &projection,
                                      const MapParameter &parameter,
                                      const std::vector<LabelData> &labels,
                                      const Vertex2D &position,
                                      double objectWidth) override;

    /**
     * Register contour label
     */
    virtual void RegisterContourLabel(const Projection &projection,
                                      const MapParameter &parameter,
                                      const PathLabelData &label,
                                      const LabelPath &labelPath) override;

    virtual void DrawLabels(const Projection& projection,
                            const MapParameter& parameter,
                            const MapData& data) override;

    void DrawSymbol(const Projection& projection,
                    const MapParameter& parameter,
                    const Symbol& style,
                    double x, double y) override;

    void DrawIcon(const IconStyle* style,
                  double centerX, double centerY,
                  double width, double height) override;

    void DrawPath(const Projection& projection,
                  const MapParameter& parameter,
                  const Color& color,
                  double width,
                  const std::vector<double>& dash,
                  LineStyle::CapStyle startCap,
                  LineStyle::CapStyle endCap,
                  size_t transStart, size_t transEnd) override;

    void DrawPath(const Projection& projection,
                  const MapParameter& parameter,
                  const std::string& styleName,
                  const std::optional<Color> &colorOverride,
                  double width,
                  LineStyle::CapStyle startCap,
                  LineStyle::CapStyle endCap,
                  size_t transStart, size_t transEnd);

    void DrawWayOutline(const StyleConfig& styleConfig,
                        const Projection& projection,
                        const MapParameter& parameter,
                        const WayData& data);

    void DrawWay(const StyleConfig& styleConfig,
                 const Projection& projection,
                 const MapParameter& parameter,
                 const WayData& data) override;

    void DrawContourSymbol(const Projection& projection,
                           const MapParameter& parameter,
                           const Symbol& symbol,
                           double space,
                           size_t transStart, size_t transEnd) override;

    void DrawArea(const Projection& projection,
                  const MapParameter& parameter,
                  const AreaData& area) override;

  public:
    explicit MapPainterSVG(const StyleConfigRef& styleConfig);
    ~MapPainterSVG() override;


    bool DrawMap(const Projection& projection,
                 const MapParameter& parameter,
                 const MapData& data,
                 std::ostream& stream);
  };
}

#endif
