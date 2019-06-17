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

#include <osmscout/MapDirectXFeatures.h>

#include <osmscout/MapDirectXImportExport.h>

#include <osmscout/MapPainter.h>

#include <osmscout/PathTextRenderer.h>

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
      DirectXTextLayout(IDWriteFactory* m_pWriteFactory, double fontSize, IDWriteTextFormat* font, std::string text);
      ~DirectXTextLayout();
    };

    using DirectXLabel = Label<DirectXNativeGlyph, DirectXTextLayout>;

  private:
    using DirectXLabelLayouter = LabelLayouter<DirectXNativeGlyph, DirectXTextLayout, MapPainterDirectX>;
    friend DirectXLabelLayouter;

  private:
    typedef std::unordered_map<uint32_t, IDWriteTextFormat*> FontMap;
    FontMap m_Fonts;
    typedef std::unordered_map<uint32_t, ID2D1SolidColorBrush*> BrushMap;
    BrushMap m_Brushs;
    typedef std::unordered_map<uint64_t, ID2D1PathGeometry*> GeometryMap;
    GeometryMap m_Geometries;
    GeometryMap m_Polygons;
    typedef std::unordered_map<uint64_t, ID2D1Bitmap*> BitmapMap;
    BitmapMap m_Bitmaps;
    typedef std::unordered_map<uint64_t, ID2D1StrokeStyle*> StrokeStyleMap;
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
    virtual void AfterPreprocessing(const StyleConfig& styleConfig,
                                    const Projection& projection,
                                    const MapParameter& parameter,
                                    const MapData& data);

    virtual void BeforeDrawing(const StyleConfig& styleConfig,
                               const Projection& projection,
                               const MapParameter& parameter,
                               const MapData& data);

    virtual void AfterDrawing(const StyleConfig& styleConfig,
                              const Projection& projection,
                              const MapParameter& parameter,
                              const MapData& data);

    virtual bool HasIcon(const StyleConfig& styleConfig,
                         const Projection& projection,
                         const MapParameter& parameter,
                         IconStyle& style);


	virtual double GetFontHeight(const Projection& projection,
                                 const MapParameter& parameter,
                                 double fontSize);

	DoubleRectangle GetTextDimension(const Projection& projection,
                                   const MapParameter& parameter,
                                   double objectWidth,
                                   double fontSize,
                                   const std::string& text);

    virtual void DrawGround(const Projection& projection,
                            const MapParameter& parameter,
                            const FillStyle& style);

    void DrawLabel(const Projection& projection,
                   const MapParameter& parameter,
                   const DoubleRectangle& labelRectangle,
                   const LabelData& label,
                   const DirectXTextLayout& textLayout);

    void DrawGlyphs(const Projection &projection,
                    const MapParameter &parameter,
                    const osmscout::PathTextStyleRef style,
                    const std::vector<osmscout::Glyph<DirectXNativeGlyph>> &glyphs);

	/**
	Register regular label with given text at the given pixel coordinate
	in a style defined by the given LabelStyle.
	*/
	virtual void RegisterRegularLabel(const Projection &projection,
		const MapParameter &parameter,
		const std::vector<LabelData> &labels,
		const Vertex2D &position,
		double objectWidth);

	/**
	* Register contour label
	*/
	virtual void RegisterContourLabel(const Projection &projection,
		const MapParameter &parameter,
		const PathLabelData &label,
		const LabelPath &labelPath);

	virtual void DrawLabels(const Projection& projection,
		const MapParameter& parameter,
		const MapData& data);

    virtual void DrawIcon(const IconStyle* style,
                          double centerX, double centerY,
                          double width, double height);

    virtual void DrawSymbol(const Projection& projection,
                            const MapParameter& parameter,
                            const Symbol& symbol,
                            double x, double y);

    virtual void DrawPath(const Projection& projection,
                          const MapParameter& parameter,
                          const Color& color,
                          double width,
                          const std::vector<double>& dash,
                          LineStyle::CapStyle startCap,
                          LineStyle::CapStyle endCap,
                          size_t transStart, size_t transEnd);

    std::shared_ptr<DirectXLabel> Layout(const Projection& projection,
                                         const MapParameter& parameter,
                                         const std::string& text,
                                         double fontSize,
                                         double objectWidth,
                                         bool enableWrapping = false,
                                         bool contourLabel = false);

    osmscout::DoubleRectangle GlyphBoundingBox(const DirectXNativeGlyph &glyph) const;

	/*
    virtual void DrawContourLabel(const Projection& projection,
                                  const MapParameter& parameter,
                                  const PathTextStyle& style,
                                  const std::string& text,
                                  size_t transStart, size_t transEnd,
                                  ContourLabelHelper& helper);
	*/

    virtual void DrawContourSymbol(const Projection& projection,
                                   const MapParameter& parameter,
                                   const Symbol& symbol,
                                   double space,
                                   size_t transStart, size_t transEnd);

    virtual void DrawArea(const Projection& projection,
                          const MapParameter& parameter,
                          const AreaData& area);

  public:
    MapPainterDirectX(const StyleConfigRef& styleConfig,
                      ID2D1Factory* pDirect2dFactory,
                      IDWriteFactory* pWriteFactory);
    virtual ~MapPainterDirectX();

    void DiscardDeviceResources();

    bool DrawMap(const Projection& projection,
                 const MapParameter& parameter,
                 const MapData& data,
                 ID2D1RenderTarget* renderTarget);
  };
}

#endif
