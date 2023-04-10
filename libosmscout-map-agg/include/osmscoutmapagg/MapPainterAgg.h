#ifndef OSMSCOUT_MAP_MAPPAINTERAGG_H
#define OSMSCOUT_MAP_MAPPAINTERAGG_H

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

#include <mutex>

#include <agg2/agg_conv_curve.h>
#include <agg2/agg_conv_contour.h>
#include <agg2/agg_path_storage.h>
#include <agg2/agg_pixfmt_rgb.h>
#include <agg2/agg_rasterizer_scanline_aa.h>
#include <agg2/agg_renderer_base.h>
#include <agg2/agg_rendering_buffer.h>
#include <agg2/agg_renderer_scanline.h>
#include <agg2/agg_scanline_p.h>

// TODO: This one is likely not available under Windows!
#include <agg2/agg_font_freetype.h>

#include <osmscoutmapagg/MapAggImportExport.h>

#include <osmscoutmap/MapPainter.h>

namespace osmscout {

  class OSMSCOUT_MAP_AGG_API MapPainterAgg : public MapPainter
  {
  public:
    typedef agg::pixfmt_rgb24                                  AggPixelFormat;

    struct NativeGlyph {
      double x;
      double y;
      const agg::glyph_cache* aggGlyph;
    };
    struct NativeLabel {
      std::wstring text;
      std::vector<NativeGlyph> glyphs;
    };
    using AggLabel = Label<NativeGlyph, NativeLabel>;
    using AggGlyph = Glyph<NativeGlyph>;
    using AggLabelLayouter = LabelLayouter<NativeGlyph, NativeLabel, MapPainterAgg>;
    friend AggLabelLayouter;

  private:
    typedef agg::renderer_base<AggPixelFormat>                 AggRenderBase;
    typedef agg::rasterizer_scanline_aa<>                      AggScanlineRasterizer;
    typedef agg::scanline_p8                                   AggScanline;
    typedef agg::renderer_scanline_aa_solid<AggRenderBase>     AggScanlineRendererAA;
    typedef agg::renderer_scanline_bin_solid<AggRenderBase>    AggScanlineRendererBin;

    typedef agg::font_engine_freetype_int32                    AggFontEngine;
    typedef agg::font_cache_manager<AggFontEngine>             AggFontManager;
    typedef agg::conv_curve<AggFontManager::path_adaptor_type> AggTextCurveConverter;
    typedef agg::conv_contour<AggTextCurveConverter>           AggTextContourConverter;

    AggLabelLayouter labelLayouter;

  private:
    AggPixelFormat            *pf;
    AggRenderBase             *renderer_base;
    AggScanlineRasterizer     *rasterizer;
    AggScanline               *scanlineP8;
    AggScanlineRendererAA     *renderer_aa;
    AggScanlineRendererBin    *renderer_bin;
    AggFontEngine             *fontEngine;
    AggFontManager            *fontCacheManager;
    AggTextCurveConverter     *convTextCurves;
    AggTextContourConverter   *convTextContours;

    std::mutex                mutex;              //! Mutex for locking concurrent calls

  private:
    void SetFont(const Projection& projection,
                 const MapParameter& parameter,
                 double size,
                 agg::glyph_rendering ren_type = agg::glyph_ren_native_gray8);

    void GetTextDimension(const std::wstring& text,
                          double& width,
                          double& height);
    void DrawText(double x,
                  double y,
                  const std::wstring& text);

    void DrawGlyph(double x, double y,
                   const agg::glyph_cache* glyph);

    void DrawGlyphVector(double x, double baselineY,
                         const std::vector<MapPainterAgg::NativeGlyph> &glyphs);

    void DrawFill(const Projection& projection,
                  const MapParameter& parameter,
                  const FillStyleRef& fillStyle,
                  const BorderStyleRef& borderStyle,
                  agg::path_storage& path);

  protected:
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

    void DrawIcon(const IconStyle* style,
                  const Vertex2D& centerPos,
                  double width, double height) override;

    void DrawSymbol(const Projection& projection,
                    const MapParameter& parameter,
                    const Symbol& symbol,
                    const Vertex2D& screenPos,
                    double scaleFactor) override;

    void DrawPath(const Projection& projection,
                  const MapParameter& parameter,
                  const Color& color,
                  double width,
                  const std::vector<double>& dash,
                  LineStyle::CapStyle startCap,
                  LineStyle::CapStyle endCap,
                  const CoordBufferRange& coordRange) override;

    void DrawLabel(const Projection& projection,
                   const MapParameter& parameter,
                   const DoubleRectangle& labelRectangle,
                   const LabelData& label,
                   const NativeLabel& layout);

    void DrawGlyphs(const Projection &projection,
                    const MapParameter &parameter,
                    const osmscout::PathTextStyleRef style,
                    const std::vector<AggGlyph> &glyphs);

    osmscout::DoubleRectangle GlyphBoundingBox(const NativeGlyph &glyph) const;

    std::shared_ptr<AggLabel> Layout(const Projection& projection,
                                     const MapParameter& parameter,
                                     const std::string& text,
                                     double fontSize,
                                     double objectWidth,
                                     bool enableWrapping = false,
                                     bool contourLabel = false);

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

    virtual void BeforeDrawing(const StyleConfig& styleConfig,
                               const Projection& projection,
                               const MapParameter& parameter,
                               const MapData& data) override;

    void DrawContourSymbol(const Projection& projection,
                           const MapParameter& parameter,
                           const Symbol& symbol,
                           const ContourSymbolData& data) override;

    void DrawArea(const Projection& projection,
                  const MapParameter& parameter,
                  const AreaData& area) override;

  public:
    explicit MapPainterAgg(const StyleConfigRef& styleConfig);
    ~MapPainterAgg() override;

    bool DrawMap(const Projection& projection,
                 const MapParameter& parameter,
                 const MapData& data,
                 AggPixelFormat* pf,
                 RenderSteps startStep=RenderSteps::FirstStep,
                 RenderSteps endStep=RenderSteps::LastStep);
  };
}

#endif
