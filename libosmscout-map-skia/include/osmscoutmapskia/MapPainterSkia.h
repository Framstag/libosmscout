#ifndef OSMSCOUT_MAP_MAPPAINTERSKIA_H
#define OSMSCOUT_MAP_MAPPAINTERSKIA_H

/*
  This source is part of the libosmscout-map library
  Copyright (C) 2025  Tim Teulings

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

#include <osmscoutmapskia/MapSkiaFeatures.h>

#include <map>
#include <memory>
#include <mutex>
#include <string>

#include <core/SkCanvas.h>
#include <core/SkFont.h>
#include <core/SkFontMgr.h>
#include <core/SkImage.h>
#include <core/SkPaint.h>
#include <core/SkPath.h>
#include <core/SkShader.h>
#include <core/SkTypeface.h>

#include <osmscoutmapskia/MapSkiaImportExport.h>

#include <osmscoutmap/MapPainter.h>

namespace osmscout {

  class OSMSCOUT_MAP_SKIA_API MapPainterSkia : public MapPainter
  {
  public:
    struct SkiaNativeGlyph {
      std::string character; //!< UTF-8 character string
      double      width;     //!< Glyph advance width in pixels
      double      height;    //!< Glyph height in pixels (full font height)
      double      yMin;      //!< Glyph bounding box top (fAscent), negative above baseline
      double      yMax;      //!< Glyph bounding box bottom, positive below baseline
      double      fontSize;  //!< Font size in pixels for this glyph
    };
    struct SkiaNativeLabel {
      std::string text;           //!< The label text
      double      fontSize;       //!< Font size used
      sk_sp<SkTypeface> typeface; //!< Typeface used for this label
    };
    using SkiaLabel = Label<SkiaNativeGlyph, SkiaNativeLabel>;
    using SkiaGlyph = Glyph<SkiaNativeGlyph>;
    using SkiaLabelLayouter = LabelLayouter<SkiaNativeGlyph, SkiaNativeLabel, MapPainterSkia>;
    friend SkiaLabelLayouter;

  private:
    struct FontDescriptor {
      std::string fontName;
      double      fontSize;

      bool operator<(const FontDescriptor& other) const {
        if (fontName != other.fontName) return fontName < other.fontName;
        return fontSize < other.fontSize;
      }
    };

    struct FollowPathHandle
    {
      bool   closeWay;
      size_t transStart;
      size_t transEnd;
      size_t i;
      size_t nVertex;
      size_t direction;
    };

    SkiaLabelLayouter labelLayouter;

    SkCanvas *draw;            //!< The Skia canvas for drawing
    std::mutex mutex;          //!< Mutex for locking concurrent calls

    std::map<FontDescriptor, sk_sp<SkTypeface>> fontCache;    //!< Cached typefaces by name and size
    sk_sp<SkFontMgr>                             fontMgr;      //!< Font manager (fontconfig-based)
    std::map<std::string, sk_sp<SkShader>>       patternCache; //!< Cached pattern shaders by filename
    std::map<std::string, sk_sp<SkImage>>        iconCache;    //!< Cached icon images by name

  private:
    sk_sp<SkTypeface> GetTypeface(const std::string& fontName);

    void SetLineAttributes(SkPaint& paint,
                           const Color& color,
                           double width,
                           const std::vector<double>& dash,
                           LineStyle::CapStyle startCap,
                           LineStyle::CapStyle endCap);

    void DrawFillStyle(const Projection& projection,
                       const MapParameter& parameter,
                       const FillStyleRef& fill,
                       const BorderStyleRef& border,
                       const SkPath& path);

    bool FollowPath(FollowPathHandle &hnd,
                    const CoordBufferRange& coordRange,
                    double l,
                    Vertex2D &origin);
    void FollowPathInit(FollowPathHandle &hnd,
                        const CoordBufferRange& coordRange,
                        Vertex2D &origin,
                        bool isClosed,
                        bool keepOrientation);

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
                   const ScreenVectorRectangle& labelRectangle,
                   const LabelData& label,
                   const SkiaNativeLabel& layout);

    void DrawGlyphs(const Projection &projection,
                    const MapParameter &parameter,
                    const osmscout::PathTextStyleRef style,
                    const std::vector<SkiaGlyph> &glyphs);

    osmscout::ScreenVectorRectangle GlyphBoundingBox(const SkiaNativeGlyph &glyph) const;

    std::shared_ptr<SkiaLabel> Layout(const Projection& projection,
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
                                      bool basemap,
                                      const ObjectFileRef& ref,
                                      const std::vector<LabelData> &labels,
                                      const Vertex2D &position,
                                      double objectWidth) override;

    /**
     * Register contour label
     */
    virtual void RegisterContourLabel(const Projection &projection,
                                      const MapParameter &parameter,
                                      bool basemap,
                                      const ObjectFileRef& ref,
                                      const PathLabelData &label,
                                      const LabelPath &labelPath) override;

    virtual void DrawLabels(const Projection& projection,
                            const MapParameter& parameter,
                            const std::vector<MapData>& data) override;

    virtual void BeforeDrawingCallback(const Projection& projection,
                                       const MapParameter& parameter,
                                       const std::vector<MapData>& data) override;

    void DrawContourSymbol(const Projection& projection,
                           const MapParameter& parameter,
                           const Symbol& symbol,
                           const ContourSymbolData& data) override;

    void DrawArea(const Projection& projection,
                  const MapParameter& parameter,
                  const AreaData& area) override;

    void StyleSheetChanged(const Projection& projection,
                            const MapParameter& parameter,
                            const std::vector<MapData>& data) override;

  public:
    MapPainterSkia();
    ~MapPainterSkia() override;

    TextMetrics MeasureText(const Projection& projection,
                            const MapParameter& parameter,
                            const std::string& text,
                            double fontSize) override;

    bool DrawMap(const Projection& projection,
                 const MapParameter& parameter,
                 const std::vector<MapData>& data,
                 SkCanvas* canvas,
                 RenderSteps startStep=RenderSteps::FirstStep,
                 RenderSteps endStep=RenderSteps::LastStep);
  };
}

#endif
