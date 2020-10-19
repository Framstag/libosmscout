/*
  This source is part of the libosmscout-map-gdi library
  Copyright (C) 2020 Transporter

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

#include <osmscout/MapPainterGDI.h>

#include <iostream>
#include <iomanip>
#include <limits>
#include <list>
#include <math.h>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>
#include <osmscout/util/String.h>
#include <osmscout/util/File.h>
#include <osmscout/util/Base64.h>

#define min(a,b) (a<b?a:b)
#define max(a,b) (a>b?a:b)
#include <objidl.h>
#include <gdiplus.h>

namespace osmscout {

	DWORD MapPainterGDI::m_gdiplusInstCount = 0;
	ULONG_PTR MapPainterGDI::m_gdiplusToken = 0;

	class GdiRender
	{
	public:
		typedef struct _PENDEF
		{
			BYTE a;
			BYTE r;
			BYTE g;
			BYTE b;
			float width;
			float dashs;
			WORD size;
			BYTE sc;
			BYTE ec;

			bool operator<(const _PENDEF& rhs) const {
				if (*this == rhs) return false;
				if (Gdiplus::Color::MakeARGB(this->a, this->r, this->g, this->b) == Gdiplus::Color::MakeARGB(rhs.a, rhs.r, rhs.g, rhs.b))
				{
					if (this->width == rhs.width)
					{
						if (this->dashs == rhs.dashs)
						{
							if (this->size == rhs.size)
							{
								if (this->sc == rhs.sc)
									return this->ec < rhs.ec;
								else
									return this->sc < rhs.sc;
							}
							else
								return this->size < rhs.size;
						}
						else
							return this->dashs < rhs.dashs;
					}
					else
						return this->width < rhs.width;
				}
				else
					return Gdiplus::Color::MakeARGB(this->a, this->r, this->g, this->b) < Gdiplus::Color::MakeARGB(rhs.a, rhs.r, rhs.g, rhs.b);
			}

			bool operator==(const _PENDEF& rhs) const {
				return this->a == rhs.a && this->r == rhs.r && this->r == rhs.r && this->b == rhs.b && this->width == rhs.width && this->dashs == rhs.dashs && this->size == rhs.size && this->sc == rhs.sc && this->ec == rhs.ec;
			}
		} PENDEF;
		typedef struct _FONTDEF
		{
			std::string name;
			float size;

			bool operator<(const _FONTDEF& rhs) const {
				if (*this == rhs) return false;
				return this->name.compare(rhs.name) < 0 || (this->name.compare(rhs.name) == 0 && this->size < rhs.size);
			}

			bool operator==(const _FONTDEF& rhs) const {
				return this->name == rhs.name && this->size == rhs.size;
			}
		} FONTDEF;

	private:
		Gdiplus::Bitmap* m_pMemBitmap;
		INT m_width;
		INT m_height;

		std::map<Gdiplus::ARGB, Gdiplus::SolidBrush*> m_SolidBrushes;
		std::map<PENDEF, Gdiplus::Pen*> m_Pens;
		std::map<FONTDEF, Gdiplus::Font*> m_Fonts;
		std::map<size_t, Gdiplus::Image*> m_Images;

	public:
		Gdiplus::Graphics* m_pGraphics;

		GdiRender(INT width, INT height)
			: m_pMemBitmap(NULL)
			, m_width(width)
			, m_height(height)
		{
			m_pMemBitmap = new Gdiplus::Bitmap(width, height);
			m_pGraphics = Gdiplus::Graphics::FromImage(m_pMemBitmap);
		}

		~GdiRender()
		{
			Release();
		}

		void Release()
		{
			for (std::map<Gdiplus::ARGB, Gdiplus::SolidBrush*>::iterator iter = m_SolidBrushes.begin(); iter != m_SolidBrushes.end(); iter++) delete iter->second;
			for (std::map<PENDEF, Gdiplus::Pen*>::iterator iter = m_Pens.begin(); iter != m_Pens.end(); iter++) delete iter->second;
			for (std::map<FONTDEF, Gdiplus::Font*>::iterator iter = m_Fonts.begin(); iter != m_Fonts.end(); iter++) delete iter->second;
			for (std::map<size_t, Gdiplus::Image*>::iterator iter = m_Images.begin(); iter != m_Images.end(); iter++) delete iter->second;
			delete m_pGraphics;
			m_pGraphics = NULL;
			delete m_pMemBitmap;
			m_pMemBitmap = NULL;
		}

		void Resize(INT width, INT height)
		{
			if (width == m_width && height == m_height) return;
			if (width == 0 || height == 0) return;
			Release();
			m_pMemBitmap = new Gdiplus::Bitmap(width, height);
			m_pGraphics = Gdiplus::Graphics::FromImage(m_pMemBitmap);
		}

		inline void Paint(Gdiplus::Graphics* pGraphics, INT x = 0, INT y = 0) { pGraphics->DrawImage(m_pMemBitmap, x, y); }

		Gdiplus::SolidBrush* GetSolidBrush(osmscout::Color color)
		{
			Gdiplus::Color colorGdi(
				(BYTE)((int)(color.GetA() * 255.0)),
				(BYTE)((int)(color.GetR() * 255.0)),
				(BYTE)((int)(color.GetG() * 255.0)),
				(BYTE)((int)(color.GetB() * 255.0))
			);
			if (m_SolidBrushes.find(colorGdi.GetValue()) == m_SolidBrushes.end())
				m_SolidBrushes[colorGdi.GetValue()] = new Gdiplus::SolidBrush(colorGdi);
			return m_SolidBrushes[colorGdi.GetValue()];
		}

		inline Gdiplus::SolidBrush* GetSolidBrush(FillStyleRef fillstyle) { return GetSolidBrush(fillstyle->GetFillColor()); }

		Gdiplus::Pen* GetPen(const osmscout::Color color = osmscout::Color::BLACK, double width = 1.0, const std::vector<double>& dash = std::vector<double>(), osmscout::LineStyle::CapStyle startCap = osmscout::LineStyle::CapStyle::capButt, osmscout::LineStyle::CapStyle endCap = osmscout::LineStyle::CapStyle::capButt)
		{
			PENDEF key = {
				(BYTE)((int)(color.GetA() * 255.0)),
				(BYTE)((int)(color.GetR() * 255.0)),
				(BYTE)((int)(color.GetG() * 255.0)),
				(BYTE)((int)(color.GetB() * 255.0)),
				(float)width,
				0.0f,
				(WORD)dash.size(),
				(BYTE)startCap,
				(BYTE)endCap
			};
			for (size_t uj = 0; uj < dash.size(); uj++) key.dashs += (float)dash[uj];
			if (m_Pens.find(key) == m_Pens.end())
			{
				m_Pens[key] = new Gdiplus::Pen(GetSolidBrush(color), (Gdiplus::REAL)width);
				switch (startCap)
				{
				case LineStyle::CapStyle::capRound: m_Pens[key]->SetStartCap(Gdiplus::LineCap::LineCapRound); break;
				case LineStyle::CapStyle::capSquare: m_Pens[key]->SetStartCap(Gdiplus::LineCap::LineCapSquare); break;
				default: break;
				}
				switch (endCap)
				{
				case LineStyle::CapStyle::capRound: m_Pens[key]->SetEndCap(Gdiplus::LineCap::LineCapRound); break;
				case LineStyle::CapStyle::capSquare: m_Pens[key]->SetEndCap(Gdiplus::LineCap::LineCapSquare); break;
				default: break;
				}
				if (dash.size() > 0)
				{
					Gdiplus::REAL* dashArray = new Gdiplus::REAL[dash.size()];
					for (size_t uj = 0; uj < dash.size(); uj++) dashArray[uj] = (Gdiplus::REAL)dash[uj];
					m_Pens[key]->SetDashPattern(dashArray, (INT)dash.size());
					delete dashArray;
				}
			}
			return m_Pens[key];
		}

		inline Gdiplus::Pen* GetPen(BorderStyleRef border) { return GetPen(border->GetColor(), border->GetWidth(), border->GetDash()); }

		Gdiplus::Font* GetFont(std::string fontname, double fontsize)
		{
			FONTDEF key = { fontname, (float)fontsize };
			if (m_Fonts.find(key) == m_Fonts.end())
			{
				std::wstring family = osmscout::UTF8StringToWString(fontname);
				m_Fonts[key] = new Gdiplus::Font(family.c_str(), (Gdiplus::REAL)fontsize, 0, Gdiplus::Unit::UnitPixel);
			}
			return m_Fonts[key];
		}

		Gdiplus::Image* GetIcon(size_t iconID, std::string path = "")
		{
			if (m_Images.find(iconID) == m_Images.end() && path.length() > 0)
			{
				const wchar_t* ext[] = { L".png", L".jpeg", L".jpg", L".gif", L".tiff", L".tif", L".bmp", L".emf", NULL };
				for (int i = 0; ext[i] != NULL; i++)
				{
					std::wstring filepath = osmscout::UTF8StringToWString(path) + ext[i];
					Gdiplus::Image* pImage = Gdiplus::Image::FromFile(filepath.c_str());
					if (pImage != NULL)
					{
						if (pImage->GetHeight() > 0 && pImage->GetWidth() > 0)
						{
							m_Images[iconID] = pImage;
							break;
						}
					}
				}
			}
			if (m_Images.find(iconID) == m_Images.end())
				return NULL;
			else
				return m_Images[iconID];
		}

		inline INT GetWidth() { return m_width; }

		inline INT GetHeight() { return m_height; }
	};

#define RENDEROBJECT(r) if (m_pBuffer == NULL) return; \
	GdiRender* r = (GdiRender*)m_pBuffer
#define RENDEROBJECTRETURN(r,v) if (m_pBuffer == NULL) return v; \
	GdiRender* r = (GdiRender*)m_pBuffer

	class PointFBuffer
	{
	public:
		Gdiplus::PointF* m_Data;
		size_t m_Size;

	public:
		PointFBuffer()
			: m_Data(NULL)
			, m_Size(0)
		{
		}

		~PointFBuffer()
		{
			if (m_Data != NULL)
			{
				delete m_Data;
				m_Data = NULL;
				m_Size = 0;
			}
		}

		void AddPoint(Gdiplus::PointF pt)
		{
			m_Size++;
			if (m_Size == 1)
			{
				m_Data = new Gdiplus::PointF[m_Size];
				m_Data[0] = pt;
			}
			else
			{
				Gdiplus::PointF* tmp = new Gdiplus::PointF[m_Size];
				for (size_t i = 0; i < m_Size - 1; i++) tmp[i] = m_Data[i];
				tmp[m_Size - 1] = pt;
				delete m_Data;
				m_Data = tmp;
			}
		}

		inline void AddPoint(float x, float y) { AddPoint(Gdiplus::PointF((Gdiplus::REAL)x, (Gdiplus::REAL)y)); }
		inline void AddPoint(double x, double y) { AddPoint(Gdiplus::PointF((Gdiplus::REAL)x, (Gdiplus::REAL)y)); }

		inline void Close() { if (m_Size > 0) AddPoint(m_Data[0]); }
	};

	MapPainterGDI::MapPainterGDI(const StyleConfigRef& styleConfig)
		: MapPainter(styleConfig, new CoordBuffer())
		, m_labelLayouter(this)
		, m_pBuffer(NULL)
	{
		if (m_gdiplusInstCount == 0)
		{
			Gdiplus::GdiplusStartupInput gdiplusStartupInput;
			Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);
		}
		m_gdiplusInstCount++;
		m_pBuffer = new GdiRender(100, 100);
	}

	MapPainterGDI::~MapPainterGDI()
	{
		if (m_pBuffer != NULL)
		{
			((GdiRender*)m_pBuffer)->Release();
			delete m_pBuffer;
			m_pBuffer = NULL;
		}
		m_gdiplusInstCount--;
		if (m_gdiplusToken > 0 && m_gdiplusInstCount == 0)
		{
			Gdiplus::GdiplusShutdown(m_gdiplusToken);
			m_gdiplusToken = 0;
		}
	}

	template<>
	std::vector<Glyph<MapPainterGDI::NativeGlyph>> MapPainterGDI::GdiLabel::ToGlyphs() const
	{
		std::vector<Glyph<MapPainterGDI::NativeGlyph>> result;
		double horizontalOffset = 0;
		for (size_t ch = 0; ch < label.wstr.length(); ch++) {
			result.emplace_back();
			result.back().glyph.character = WStringToUTF8String(label.wstr.substr(ch, 1));
			result.back().position.SetX(horizontalOffset);
			result.back().position.SetY(0);
			Gdiplus::RectF bb;
			((GdiRender*)label.render)->m_pGraphics->MeasureString(label.wstr.substr(ch, 1).c_str(), -1, (const Gdiplus::Font*)label.font, Gdiplus::PointF(0, fontSize * 2), &bb);
			horizontalOffset += bb.Width;
		}
		return result;
	}

	DoubleRectangle MapPainterGDI::GlyphBoundingBox(const NativeGlyph &glyph) const
	{
		return DoubleRectangle(0.0,
			(double)(glyph.height * -1),
			(double)glyph.width,
			(double)glyph.height);
	}

	std::shared_ptr<MapPainterGDI::GdiLabel> MapPainterGDI::Layout(const Projection& projection,
		const MapParameter& parameter,
		const std::string& text,
		double fontSize,
		double /*objectWidth*/,
		bool /*enableWrapping*/,
		bool /*contourLabel*/)
	{
		auto label = std::make_shared<MapPainterGDI::GdiLabel>();
		label->label.wstr = UTF8StringToWString(text);
		label->text = text;
		label->fontSize = fontSize;
		label->height = projection.ConvertWidthToPixel(fontSize * parameter.GetFontSize());
		Gdiplus::Font* pFont = ((GdiRender*)m_pBuffer)->GetFont(parameter.GetFontName(), projection.ConvertWidthToPixel(fontSize * parameter.GetFontSize()));
		Gdiplus::RectF bb;
		std::wstring wtext = UTF8StringToWString(text);
		((GdiRender*)m_pBuffer)->m_pGraphics->MeasureString(wtext.c_str(), -1, pFont, Gdiplus::PointF(0, label->height), &bb);
		label->width = bb.Width;
		label->label.font = pFont;
		label->label.render = m_pBuffer;

		return label;
	}

	void MapPainterGDI::DrawLabel(const Projection &projection,
		const MapParameter &parameter,
		const DoubleRectangle &labelRectangle,
		const LabelData &label,
		const NativeLabel &/*layout*/)
	{
		RENDEROBJECT(pRender);
		if (const TextStyle* style = dynamic_cast<const TextStyle*>(label.style.get()); style != nullptr)
		{
			Gdiplus::Font* pFont = pRender->GetFont(parameter.GetFontName(), projection.ConvertWidthToPixel(label.fontSize * parameter.GetFontSize()));
			Gdiplus::SolidBrush* pBrush = pRender->GetSolidBrush(style->GetTextColor());
			std::wstring text = osmscout::UTF8StringToWString(label.text);
			pRender->m_pGraphics->DrawString(text.c_str(), (INT)text.length(), pFont, Gdiplus::PointF((Gdiplus::REAL)labelRectangle.x, (Gdiplus::REAL)labelRectangle.y), pBrush);
		}
		else if (const ShieldStyle* style = dynamic_cast<const ShieldStyle*>(label.style.get()); style != nullptr)
		{
			// Shield background
			Gdiplus::SolidBrush* pBrush = pRender->GetSolidBrush(style->GetBgColor());
			pRender->m_pGraphics->FillRectangle(pBrush, (INT)labelRectangle.x - 2, (INT)labelRectangle.y - 2, (INT)labelRectangle.width + 5, (INT)labelRectangle.height + 6);

			// Shield inner border
			Gdiplus::Pen* pPen = pRender->GetPen(style->GetBorderColor());
			pRender->m_pGraphics->DrawRectangle(pPen, (INT)labelRectangle.x, (INT)labelRectangle.y, (INT)labelRectangle.width, (INT)labelRectangle.height + 1);

			Gdiplus::Font* pFont = pRender->GetFont(parameter.GetFontName(), projection.ConvertWidthToPixel(label.fontSize * parameter.GetFontSize()));
			pBrush = pRender->GetSolidBrush(style->GetTextColor());
			std::wstring text = osmscout::UTF8StringToWString(label.text);
			pRender->m_pGraphics->DrawString(text.c_str(), (INT)text.length(), pFont, Gdiplus::PointF((Gdiplus::REAL)labelRectangle.x, (Gdiplus::REAL)labelRectangle.y), pBrush);
		}
	}

	void MapPainterGDI::DrawGlyphs(const Projection &projection,
		const MapParameter &parameter,
		const osmscout::PathTextStyleRef style,
		const std::vector<GdiGlyph> &glyphs)
	{
		assert(!glyphs.empty());
		RENDEROBJECT(pRender);

		Gdiplus::Font* pFont = pRender->GetFont(parameter.GetFontName(), projection.ConvertWidthToPixel(style->GetSize() * parameter.GetFontSize()));
		Gdiplus::SolidBrush* pBrush = pRender->GetSolidBrush(style->GetTextColor());

		for (auto const &glyph : glyphs) {
			if (glyph.glyph.character.empty() ||
				(glyph.glyph.character.length() == 1 &&
				(glyph.glyph.character == " " || glyph.glyph.character == "\t" || glyph.glyph.character == " "))) {
				continue;
			}
			std::wstring text = osmscout::UTF8StringToWString(glyph.glyph.character);
			Gdiplus::PointF ptf((Gdiplus::REAL)glyph.position.GetX(), (Gdiplus::REAL)glyph.position.GetY());
			Gdiplus::RectF bb;
			pRender->m_pGraphics->MeasureString(text.c_str(), -1, pFont, ptf, &bb);
			Gdiplus::Matrix m;
			m.RotateAt((Gdiplus::REAL)RadToDeg(glyph.angle), ptf);
			pRender->m_pGraphics->SetTransform(&m);
			pRender->m_pGraphics->DrawString(text.c_str(), -1, pFont, ptf - Gdiplus::PointF(0.5f * bb.Width, 0.5f * bb.Height), pBrush);
			pRender->m_pGraphics->ResetTransform();
		}
	}

	void MapPainterGDI::AfterPreprocessing(const StyleConfig& /*styleConfig*/,
		const Projection& /*projection*/,
		const MapParameter& /*parameter*/,
		const MapData& /*data*/)
	{
		// Not implemented
	}

	void MapPainterGDI::BeforeDrawing(const StyleConfig& /*styleConfig*/,
		const Projection& projection,
		const MapParameter& parameter,
		const MapData& /*data*/)
	{
		DoubleRectangle viewport(0.0, 0.0, (double)projection.GetWidth(), (double)projection.GetHeight());
		m_labelLayouter.SetViewport(viewport);
		m_labelLayouter.SetLayoutOverlap(parameter.GetDropNotVisiblePointLabels() ? 0 : 1);
	}

	void MapPainterGDI::AfterDrawing(const StyleConfig& /*styleConfig*/,
		const Projection& /*projection*/,
		const MapParameter& /*parameter*/,
		const MapData& /*data*/)
	{
		// Not implemented
	}

	bool MapPainterGDI::HasIcon(const StyleConfig& /*styleConfig*/,
		const Projection& projection,
		const MapParameter& parameter,
		IconStyle& style)
	{
		if (style.GetIconId() == 0) {
			return false;
		}
		RENDEROBJECTRETURN(pRender, false);

		size_t idx = style.GetIconId() - 1;

		// there is possible that exists multiple IconStyle instances with same iconId (point and area icon with same icon name)
		// setup dimensions for all of them
		double dimension;
		if (parameter.GetIconMode() == MapParameter::IconMode::Scalable) {
			dimension = projection.ConvertWidthToPixel(parameter.GetIconSize());
		}
		else if (parameter.GetIconMode() == MapParameter::IconMode::ScaledPixmap) {
			dimension = std::round(projection.ConvertWidthToPixel(parameter.GetIconSize()));
		}
		else {
			dimension = std::round(parameter.GetIconPixelSize());
		}

		style.SetWidth((unsigned int)dimension);
		style.SetHeight((unsigned int)dimension);

		Gdiplus::Image* pImage = pRender->GetIcon(idx);
		if (pImage != NULL) return true;

		for (const auto& path : parameter.GetIconPaths()) {
			pImage = pRender->GetIcon(idx, AppendFileToDir(path, style.GetIconName()));
			if (pImage != NULL) return true;
		}

		style.SetIconId(0);

		return false;
	}

	double MapPainterGDI::GetFontHeight(const Projection& projection,
		const MapParameter& parameter,
		double fontSize)
	{
		return projection.ConvertWidthToPixel(fontSize * parameter.GetFontSize());
	}

	void MapPainterGDI::DrawGround(const Projection& projection,
		const MapParameter& /*parameter*/,
		const FillStyle& style)
	{
		RENDEROBJECT(pRender);
		Gdiplus::SolidBrush* pBrush = pRender->GetSolidBrush(style.GetFillColor());
		pRender->m_pGraphics->FillRectangle(pBrush, 0, 0, (INT)projection.GetWidth(), (INT)projection.GetHeight());
	}

	void MapPainterGDI::RegisterRegularLabel(const Projection &projection,
		const MapParameter &parameter,
		const std::vector<LabelData> &labels,
		const Vertex2D &position,
		double objectWidth)
	{
		m_labelLayouter.RegisterLabel(projection, parameter, position, labels, objectWidth);
	}

	void MapPainterGDI::RegisterContourLabel(const Projection &projection,
		const MapParameter &parameter,
		const PathLabelData &label,
		const LabelPath &labelPath)
	{
		m_labelLayouter.RegisterContourLabel(projection, parameter, label, labelPath);
	}

	void MapPainterGDI::DrawLabels(const Projection& projection,
		const MapParameter& parameter,
		const MapData& /*data*/)
	{
		m_labelLayouter.Layout(projection, parameter);
		m_labelLayouter.DrawLabels(projection,
			parameter,
			this);
		m_labelLayouter.Reset();
	}

	void MapPainterGDI::DrawSymbol(const Projection& projection,
		const MapParameter& /*parameter*/,
		const Symbol& symbol,
		double x, double y)
	{
		double minX;
		double minY;
		double maxX;
		double maxY;
		double centerX;
		double centerY;
		Gdiplus::Pen* pPen;
		Gdiplus::Brush* pBrush;

		RENDEROBJECT(pRender);
		symbol.GetBoundingBox(projection, minX, minY, maxX, maxY);

		centerX = (minX + maxX) / 2;
		centerY = (minY + maxY) / 2;

		for (const auto& primitive : symbol.GetPrimitives()) {
			const DrawPrimitive *primitivePtr = primitive.get();

			const auto *polygon = dynamic_cast<const PolygonPrimitive*>(primitivePtr);
			const auto *rectangle = dynamic_cast<const RectanglePrimitive*>(primitivePtr);
			const auto *circle = dynamic_cast<const CirclePrimitive*>(primitivePtr);

			if (polygon != nullptr)
			{
				pPen = (polygon->GetBorderStyle()) ? pRender->GetPen(polygon->GetBorderStyle()) : NULL;
				pBrush = (polygon->GetFillStyle()) ? pRender->GetSolidBrush(polygon->GetFillStyle()) : NULL;
				PointFBuffer polypoints;
				for (auto pixel = polygon->GetCoords().begin();
					pixel != polygon->GetCoords().end();
					++pixel) {
					polypoints.AddPoint(x + projection.ConvertWidthToPixel(pixel->GetX()) - centerX, y + projection.ConvertWidthToPixel(pixel->GetY()) - centerY);
				}
				if (pBrush) pRender->m_pGraphics->FillPolygon(pBrush, polypoints.m_Data, (INT)polypoints.m_Size);
				if (pPen) pRender->m_pGraphics->DrawPolygon(pPen, polypoints.m_Data, (INT)polypoints.m_Size);
			}
			else if (rectangle != nullptr) {
				pPen = (rectangle->GetBorderStyle()) ? pRender->GetPen(rectangle->GetBorderStyle()) : NULL;
				pBrush = (rectangle->GetFillStyle()) ? pRender->GetSolidBrush(rectangle->GetFillStyle()) : NULL;
				Gdiplus::RectF rect(
					(Gdiplus::REAL)(x + projection.ConvertWidthToPixel(rectangle->GetTopLeft().GetX()) - centerX),
					(Gdiplus::REAL)(y + projection.ConvertWidthToPixel(rectangle->GetTopLeft().GetY()) - centerY),
					(Gdiplus::REAL)(projection.ConvertWidthToPixel(rectangle->GetWidth())),
					(Gdiplus::REAL)(projection.ConvertWidthToPixel(rectangle->GetHeight()))
				);
				if (pBrush) pRender->m_pGraphics->FillRectangle(pBrush, rect);
				if (pPen) pRender->m_pGraphics->DrawRectangle(pPen, rect);
			}
			else if (circle != nullptr) {
				pPen = (circle->GetBorderStyle()) ? pRender->GetPen(circle->GetBorderStyle()) : NULL;
				pBrush = (circle->GetFillStyle()) ? pRender->GetSolidBrush(circle->GetFillStyle()) : NULL;
				Gdiplus::RectF rect(
					(Gdiplus::REAL)(x + projection.ConvertWidthToPixel(circle->GetCenter().GetX()) - centerX - 2 * projection.ConvertWidthToPixel(circle->GetRadius())),
					(Gdiplus::REAL)(y + projection.ConvertWidthToPixel(circle->GetCenter().GetY()) - centerY - 2 * projection.ConvertWidthToPixel(circle->GetRadius())),
					(Gdiplus::REAL)(2 * projection.ConvertWidthToPixel(circle->GetRadius())),
					(Gdiplus::REAL)(2 * projection.ConvertWidthToPixel(circle->GetRadius()))
				);
				if (pBrush) pRender->m_pGraphics->FillEllipse(pBrush, rect);
				if (pPen) pRender->m_pGraphics->DrawEllipse(pPen, rect);
			}
		}
	}

	void MapPainterGDI::DrawIcon(const IconStyle* style,
		double x, double y,
		double width, double height)
	{
		RENDEROBJECT(pRender);
		Gdiplus::Image* pImage = pRender->GetIcon(style->GetIconId());
		if (pImage != NULL)
			pRender->m_pGraphics->DrawImage(pImage, (INT)(x - width / 2.0), (INT)(y - height / 2.0), (INT)width, (INT)height);
	}

	void MapPainterGDI::DrawPath(const Projection& /*projection*/,
		const MapParameter& /*parameter*/,
		const Color& color,
		double width,
		const std::vector<double>& dash,
		LineStyle::CapStyle startCap,
		LineStyle::CapStyle endCap,
		size_t transStart, size_t transEnd)
	{
		RENDEROBJECT(pRender);
		Gdiplus::Pen* pPen = pRender->GetPen(color, width, dash, startCap, endCap);
		PointFBuffer points;
		for (size_t i = transStart; i <= transEnd; i++)
		{
			points.AddPoint(coordBuffer->buffer[i].GetX(), coordBuffer->buffer[i].GetY());
		}
		pRender->m_pGraphics->DrawLines(pPen, points.m_Data, (INT)points.m_Size);
	}

	void MapPainterGDI::DrawWayOutline(const StyleConfig& /*styleConfig*/,
		const Projection& /*projection*/,
		const MapParameter& /*parameter*/,
		const WayData& /*data*/)
	{
		// Not implemented
	}

	void MapPainterGDI::DrawWay(const StyleConfig& /*styleConfig*/,
		const Projection& projection,
		const MapParameter& parameter,
		const WayData& data)
	{
		if (!data.lineStyle->GetDash().empty() && data.lineStyle->GetGapColor().GetA() > 0.0) {
			DrawPath(projection,
				parameter,
				data.lineStyle->GetGapColor(),
				data.lineWidth,
				emptyDash,
				data.startIsClosed ? data.lineStyle->GetEndCap() : data.lineStyle->GetJoinCap(),
				data.endIsClosed ? data.lineStyle->GetEndCap() : data.lineStyle->GetJoinCap(),
				data.transStart, data.transEnd);
		}
		DrawPath(projection,
			parameter,
			data.lineStyle->GetLineColor(),
			data.lineWidth,
			data.lineStyle->GetDash(),
			data.startIsClosed ? data.lineStyle->GetEndCap() : data.lineStyle->GetJoinCap(),
			data.endIsClosed ? data.lineStyle->GetEndCap() : data.lineStyle->GetJoinCap(),
			data.transStart, data.transEnd);
	}

	void MapPainterGDI::DrawContourSymbol(const Projection& /*projection*/,
		const MapParameter& /*parameter*/,
		const Symbol& /*symbol*/,
		double /*space*/,
		size_t /*transStart*/, size_t /*transEnd*/)
	{
		// Not implemented
	}

	void MapPainterGDI::DrawArea(const Projection& /*projection*/,
		const MapParameter& /*parameter*/,
		const MapPainter::AreaData& area)
	{
		RENDEROBJECT(pRender);
		PointFBuffer areaPoints;
		for (size_t i = area.transStart; i <= area.transEnd; i++)
		{
			areaPoints.AddPoint(coordBuffer->buffer[i].GetX(), coordBuffer->buffer[i].GetY());
		}
		areaPoints.Close();
		struct clippingRegion
		{
			PointFBuffer* points;
			Gdiplus::GraphicsPath* path;
			Gdiplus::Region* region;
		};
		std::vector<clippingRegion> clippingInfo;
		for (std::list<PolyData>::const_iterator c = area.clippings.begin();
			c != area.clippings.end();
			++c) {
			const PolyData &data = *c;
			clippingRegion cr;
			cr.points = new PointFBuffer();
			for (size_t i = data.transStart; i <= data.transEnd; i++) {
				cr.points->AddPoint(coordBuffer->buffer[i].GetX(), coordBuffer->buffer[i].GetY());
			}
			cr.points->Close();
			cr.path = new Gdiplus::GraphicsPath();
			cr.path->AddLines(cr.points->m_Data, (INT)cr.points->m_Size);
			cr.region = new Gdiplus::Region(cr.path);
			pRender->m_pGraphics->ExcludeClip(cr.region);
			clippingInfo.push_back(cr);
		}
		if (area.fillStyle)
		{
			pRender->m_pGraphics->FillPolygon(pRender->GetSolidBrush(area.fillStyle), areaPoints.m_Data, (INT)areaPoints.m_Size);
		}
		pRender->m_pGraphics->ResetClip();
		if (area.borderStyle)
		{
			Gdiplus::Pen* pPen = pRender->GetPen(area.borderStyle);
			for (size_t i = 0; i < clippingInfo.size(); i++)
			{
				pRender->m_pGraphics->DrawLines(pPen, clippingInfo[i].points->m_Data, (INT)clippingInfo[i].points->m_Size);
			}
			pRender->m_pGraphics->DrawLines(pPen, areaPoints.m_Data, (INT)areaPoints.m_Size);
		}
		for (size_t i = 0; i < clippingInfo.size(); i++)
		{
			delete clippingInfo[i].region;
			delete clippingInfo[i].path;
			delete clippingInfo[i].points;
		}
		clippingInfo.clear();
	}

	bool MapPainterGDI::DrawMap(const Projection& projection, const MapParameter& parameter, const MapData& data, HDC hdc, RECT paintRect)
	{
		INT w = paintRect.right - paintRect.left;
		INT h = paintRect.bottom - paintRect.top;
		((GdiRender*)m_pBuffer)->Resize(w, h);
		if (Draw(projection, parameter, data))
		{
			Gdiplus::Graphics g(hdc);
			((GdiRender*)m_pBuffer)->Paint(&g, paintRect.left, paintRect.top);
			return true;
		}
		return false;
	}
}
