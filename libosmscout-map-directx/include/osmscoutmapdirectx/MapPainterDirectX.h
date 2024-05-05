#ifndef OSMSCOUT_MAP_MAPPAINTERDIRECTX_H
#define OSMSCOUT_MAP_MAPPAINTERDIRECTX_H

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

#include <unordered_map>

#include <osmscoutmapdirectx/MapDirectXFeatures.h>

#include <osmscoutmapdirectx/MapDirectXImportExport.h>

#include <osmscoutmap/MapPainter.h>

#include <osmscoutmapdirectx/PathTextRenderer.h>

#define NOMINMAX 1

#include <d2d1.h>
#include <dwrite.h>
#include <Wincodec.h>

#if defined(UNICODE) || defined(_UNICODE) || defined(_MBCS) || defined(MBCS)
#define MBUC
#endif

namespace osmscout {

  class OSMSCOUT_MAP_DIRECTX_API MapPainterDirectX : public MapPainter
  {
  public:
    struct DirectXNativeGlyph {
#ifdef MBUC
      std::wstring character;
#else
      std::string character;
#endif
      double width;
      double height;
    };

    using DirectXGlyph = Glyph<DirectXNativeGlyph>;

    class DirectXTextLayout
    {
    public:
      IDWriteFactory*     m_pWriteFactory;
      FLOAT               m_fSize;
      IDWriteTextLayout*  m_pDWriteTextLayout;
      DWRITE_TEXT_METRICS m_TextMetrics;

    public:
      DirectXTextLayout(IDWriteFactory* m_pWriteFactory,
                        double fontSize,
                        IDWriteTextFormat* font,
                        const std::string& text);
      ~DirectXTextLayout();
    };

    using DirectXLabel = Label<DirectXNativeGlyph, DirectXTextLayout>;

  private:
    using DirectXLabelLayouter = LabelLayouter<DirectXNativeGlyph, DirectXTextLayout, MapPainterDirectX>;
    friend DirectXLabelLayouter;

  private:
    using FontMap = std::unordered_map<uint32_t, IDWriteTextFormat *>;
    FontMap m_Fonts;
    using BrushMap = std::unordered_map<uint32_t, ID2D1SolidColorBrush *>;
    BrushMap m_Brushs;
    using GeometryMap = std::unordered_map<uint64_t, ID2D1PathGeometry *>;
    GeometryMap m_Geometries;
    GeometryMap m_Polygons;
    using BitmapMap = std::unordered_map<uint64_t, ID2D1Bitmap *>;
    BitmapMap m_Bitmaps;
    using StrokeStyleMap = std::unordered_map<uint64_t, ID2D1StrokeStyle *>;
    StrokeStyleMap m_StrokeStyles;
    ID2D1StrokeStyle* m_dashLessStrokeStyle;

    ID2D1Factory* m_pDirect2dFactory;
    IDWriteFactory* m_pWriteFactory;
    ID2D1RenderTarget* m_pRenderTarget;
    IWICImagingFactory* m_pImagingFactory;
    IDWriteRenderingParams* m_pRenderingParams;
    PathTextRenderer* m_pPathTextRenderer;
    FLOAT dpiX, dpiY;
    std::map<double, double> fontHeightMap;
    TypeConfigRef typeConfig;

    DirectXLabelLayouter            m_LabelLayouter;

  private:
    D2D1_COLOR_F GetColorValue(const Color& color);
    ID2D1SolidColorBrush* GetColorBrush(const Color& color);
    ID2D1SolidColorBrush* GetColorBrush(D2D1_COLOR_F& color);
    ID2D1StrokeStyle* GetStrokeStyle(const std::vector<double>& dash);
    void _DrawText(double x, double y, const Color& color, const DirectXTextLayout& textLayout);
    bool LoadBitmapFromFile(PCWSTR uri, ID2D1Bitmap **ppBitmap);
    IDWriteTextFormat* GetFont(const Projection& projection, const MapParameter& parameter, double fontSize);

  protected:
    void AfterPreprocessing(const StyleConfig &styleConfig,
                            const Projection &projection,
                            const MapParameter &parameter,
                            const MapData &data) override;

    void BeforeDrawing(const StyleConfig &styleConfig,
                       const Projection &projection,
                       const MapParameter &parameter,
                       const MapData &data) override;

    void AfterDrawing(const StyleConfig &styleConfig,
                      const Projection &projection,
                      const MapParameter &parameter,
                      const MapData &data) override;

    bool HasIcon(const StyleConfig &styleConfig,
                 const Projection &projection,
                 const MapParameter &parameter,
                 IconStyle &style) override;


    double GetFontHeight(const Projection &projection,
                         const MapParameter &parameter,
                         double fontSize) override;

	ScreenVectorRectangle GetTextDimension(const Projection& projection,
                                   const MapParameter& parameter,
                                   double objectWidth,
                                   double fontSize,
                                   const std::string& text);

    void DrawGround(const Projection &projection,
                    const MapParameter &parameter,
                    const FillStyle &style) override;

    void DrawLabel(const Projection& projection,
                   const MapParameter& parameter,
                   const ScreenVectorRectangle& labelRectangle,
                   const LabelData& label,
                   const DirectXTextLayout& textLayout);

    void DrawGlyphs(const Projection &projection,
                    const MapParameter &parameter,
                    const osmscout::PathTextStyleRef& style,
                    const std::vector<osmscout::Glyph<DirectXNativeGlyph>> &glyphs);

	/**
	Register regular label with given text at the given pixel coordinate
	in a style defined by the given LabelStyle.
	*/
  void RegisterRegularLabel(const Projection &projection,
                            const MapParameter &parameter,
                            const ObjectFileRef& ref,
                            const std::vector<LabelData> &labels,
                            const Vertex2D &position,
                            double objectWidth) override;

	/**
	* Register contour label
	*/
  void RegisterContourLabel(const Projection &projection,
                            const MapParameter &parameter,
                            const ObjectFileRef& ref,
                            const PathLabelData &label,
                            const LabelPath &labelPath) override;

    void DrawLabels(const Projection &projection,
                    const MapParameter &parameter,
                    const MapData &data) override;

    void DrawIcon(const IconStyle *style,
                  const Vertex2D& centerPos,
                  double width, double height) override;

    void DrawSymbol(const Projection &projection,
                    const MapParameter &parameter,
                    const Symbol &symbol,
                    const Vertex2D& screenPos,
                    double scaleFactor) override;

    void DrawPath(const Projection &projection,
                  const MapParameter &parameter,
                  const Color &color,
                  double width,
                  const std::vector<double> &dash,
                  LineStyle::CapStyle startCap,
                  LineStyle::CapStyle endCap,
                  const CoordBufferRange& coordRange) override;

    std::shared_ptr<DirectXLabel> Layout(const Projection& projection,
                                         const MapParameter& parameter,
                                         const std::string& text,
                                         double fontSize,
                                         double objectWidth,
                                         bool enableWrapping = false,
                                         bool contourLabel = false);

    osmscout::ScreenVectorRectangle GlyphBoundingBox(const DirectXNativeGlyph &glyph) const;

	/*
    virtual void DrawContourLabel(const Projection& projection,
                                  const MapParameter& parameter,
                                  const PathTextStyle& style,
                                  const std::string& text,
                                  size_t transStart, size_t transEnd,
                                  ContourLabelHelper& helper);
	*/

  void DrawContourSymbol(const Projection &projection,
                         const MapParameter &parameter,
                         const Symbol &symbol,
                         const ContourSymbolData& data) override;

    void DrawArea(const Projection &projection,
                  const MapParameter &parameter,
                  const AreaData &area) override;

  public:
    MapPainterDirectX(const StyleConfigRef &styleConfig,
                      ID2D1Factory *pDirect2dFactory,
                      IDWriteFactory *pWriteFactory);

    ~MapPainterDirectX() override;

    void DiscardDeviceResources();

    bool DrawMap(const Projection& projection,
                 const MapParameter& parameter,
                 const MapData& data,
                 ID2D1RenderTarget* renderTarget);
  };
}

#endif
