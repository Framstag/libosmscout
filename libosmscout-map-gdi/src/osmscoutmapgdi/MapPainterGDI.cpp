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

// Including Windows and Gdiplus is a pain
// We get compiler errors in gdi, if someone includes
// Windows headers before us using other settings...
#ifndef UNICODE
#define UNICODE
#endif
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>

#undef min
#undef max

#include <osmscoutmapgdi/MapPainterGDI.h>

#include <osmscout/util/String.h>
#include <osmscout/util/File.h>

namespace osmscout {

  DWORD MapPainterGDI::m_gdiplusInstCount = 0;
  ULONG_PTR MapPainterGDI::m_gdiplusToken = 0;

  class PointFBuffer {
  public:
    std::vector<Gdiplus::PointF> m_Data;

  public:
    PointFBuffer() = default;

    void ResetAndReserve(size_t bufferSize) {
      m_Data.clear();
      m_Data.reserve(bufferSize);
    }

    void AddPoint(const Gdiplus::PointF &pt) {
      m_Data.push_back(pt);
    }

    inline void AddPoint(double x, double y) {
      AddPoint(Gdiplus::PointF((Gdiplus::REAL) x, (Gdiplus::REAL) y));
    }

    double GetLength() const {
      double result = 0;

      for (size_t i = 1; i < m_Data.size(); i++) {
        result += sqrt((m_Data[i].X - m_Data[i - 1].X) * (m_Data[i].X - m_Data[i - 1].X) +
                       (m_Data[i].Y - m_Data[i - 1].Y) * (m_Data[i].Y - m_Data[i - 1].Y));
      }

      return result;
    }
  };

  struct PENDEF {
    BYTE a;
    BYTE r;
    BYTE g;
    BYTE b;
    float width;
    float dashs;
    WORD size;
    BYTE sc;
    BYTE ec;

    bool operator<(const PENDEF &rhs) const {
      if (this == &rhs) {
        return false;
      }

      if (Gdiplus::Color::MakeARGB(this->a, this->r, this->g, this->b) ==
          Gdiplus::Color::MakeARGB(rhs.a, rhs.r, rhs.g, rhs.b)) {
        if (this->width == rhs.width) {
          if (this->dashs == rhs.dashs) {
            if (this->size == rhs.size) {
              if (this->sc == rhs.sc) {
                return this->ec < rhs.ec;
              }

              return this->sc < rhs.sc;
            }

            return this->size < rhs.size;
          }

          return this->dashs < rhs.dashs;
        }

        return this->width < rhs.width;
      }

      return Gdiplus::Color::MakeARGB(this->a, this->r, this->g, this->b) <
             Gdiplus::Color::MakeARGB(rhs.a, rhs.r, rhs.g, rhs.b);
    }

    bool operator==(const PENDEF &rhs) const {
      return this->a == rhs.a &&
             this->r == rhs.r &&
             this->r == rhs.r &&
             this->b == rhs.b &&
             this->width == rhs.width &&
             this->dashs == rhs.dashs &&
             this->size == rhs.size &&
             this->sc == rhs.sc &&
             this->ec == rhs.ec;
    }
  };

  struct FONTDEF {
    std::string name;
    float size;

    bool operator<(const FONTDEF &rhs) const {
      if (this == &rhs) {
        return false;
      }

      return name < rhs.name || (this->name == rhs.name && this->size < rhs.size);
    }

    bool operator==(const FONTDEF &rhs) const {
      return this->name == rhs.name && this->size == rhs.size;
    }
  };

  class GdiRender {
  private:
    std::map<PENDEF, Gdiplus::Pen *> m_Pens;
    std::map<FONTDEF, Gdiplus::Font *> m_Fonts;
    std::map<size_t, Gdiplus::Image *> m_Images;

  public:
    Gdiplus::Graphics *m_pGraphics;
    PointFBuffer pointBuffer;

    GdiRender(HDC hdc) {
      m_pGraphics = Gdiplus::Graphics::FromHDC(hdc);
      m_pGraphics->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
      m_pGraphics->SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
      m_pGraphics->SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
    }

    ~GdiRender() {
      Release();
    }

    void Release() {
      for (auto &pen: m_Pens) {
        delete pen.second;
      }
      m_Pens.clear();

      for (auto &font: m_Fonts) {
        delete font.second;
      }
      m_Fonts.clear();

      for (auto &img: m_Images) {
        delete img.second;
      }
      m_Images.clear();

      delete m_pGraphics;
      m_pGraphics = nullptr;
    }

    Gdiplus::Color ColorToGdiColor(const osmscout::Color &color) {
      return {
        (BYTE) ((int) (color.GetA() * 255.0)),
        (BYTE) ((int) (color.GetR() * 255.0)),
        (BYTE) ((int) (color.GetG() * 255.0)),
        (BYTE) ((int) (color.GetB() * 255.0))
      };
    }

    Gdiplus::Pen *GetPen(const osmscout::Color &color = osmscout::Color::BLACK,
                         double width = 1.0,
                         const std::vector<double> &dash = std::vector<double>(),
                         osmscout::LineStyle::CapStyle startCap = osmscout::LineStyle::CapStyle::capButt,
                         osmscout::LineStyle::CapStyle endCap = osmscout::LineStyle::CapStyle::capButt) {
      PENDEF key = {
        (BYTE) ((int) (color.GetA() * 255.0)),
        (BYTE) ((int) (color.GetR() * 255.0)),
        (BYTE) ((int) (color.GetG() * 255.0)),
        (BYTE) ((int) (color.GetB() * 255.0)),
        (float) width,
        0.0f,
        (WORD) dash.size(),
        (BYTE) startCap,
        (BYTE) endCap
      };

      for (double uj: dash) {
        key.dashs += (float) uj;
      }

      auto existingPen = m_Pens.find(key);

      if (existingPen != m_Pens.end()) {
        return existingPen->second;
      }

      Gdiplus::SolidBrush brush(ColorToGdiColor(color));

      Gdiplus::Pen *pen = new Gdiplus::Pen(&brush,
                                           (Gdiplus::REAL) width);

      switch (startCap) {
        case LineStyle::CapStyle::capRound:
          pen->SetStartCap(Gdiplus::LineCap::LineCapRound);
          break;
        case LineStyle::CapStyle::capSquare:
          pen->SetStartCap(Gdiplus::LineCap::LineCapSquare);
          break;
        default:
          break;
      }

      switch (endCap) {
        case LineStyle::CapStyle::capRound:
          pen->SetEndCap(Gdiplus::LineCap::LineCapRound);
          break;
        case LineStyle::CapStyle::capSquare:
          pen->SetEndCap(Gdiplus::LineCap::LineCapSquare);
          break;
        default:
          break;
      }

      if (!dash.empty()) {
        Gdiplus::REAL *dashArray = new Gdiplus::REAL[dash.size()];
        for (size_t uj = 0; uj < dash.size(); uj++) {
          dashArray[uj] = (Gdiplus::REAL) dash[uj];
        }

        pen->SetDashPattern(dashArray, (INT) dash.size());
        delete[] dashArray;
      }

      m_Pens[key] = pen;

      return pen;
    }

    inline Gdiplus::Pen *GetPen(const BorderStyleRef &border) {
      return GetPen(border->GetColor(), border->GetWidth(), border->GetDash());
    }

    Gdiplus::Font *GetFont(const std::string &fontname,
                           double fontsize) {
      FONTDEF key = {fontname, (float) fontsize};
      if (m_Fonts.find(key) == m_Fonts.end()) {
        std::wstring family = osmscout::UTF8StringToWString(fontname);
        m_Fonts[key] = new Gdiplus::Font(family.c_str(), (Gdiplus::REAL) fontsize, 0, Gdiplus::Unit::UnitPixel);
      }
      return m_Fonts[key];
    }

    Gdiplus::Image *GetIcon(size_t iconID,
                            const std::string &path = "") {
      if (m_Images.find(iconID) == m_Images.end() && path.length() > 0) {
        std::array<std::wstring, 8> extensions = {L".png", L".jpeg", L".jpg", L".gif", L".tiff", L".tif", L".bmp",
                                                  L".emf"};
        for (const auto &ext: extensions) {
          std::wstring filepath = osmscout::UTF8StringToWString(path) + ext;
          Gdiplus::Image *pImage = Gdiplus::Image::FromFile(filepath.c_str());
          if (pImage != nullptr) {
            if (pImage->GetHeight() > 0 && pImage->GetWidth() > 0) {
              m_Images[iconID] = pImage;
              break;
            }
          }
        }
      }

      if (m_Images.find(iconID) == m_Images.end()) {
        return nullptr;
      }

      return m_Images[iconID];
    }
  };

#define RENDEROBJECT(r) if (m_pBuffer == NULL) return; \
  GdiRender* r = (GdiRender*)m_pBuffer
#define RENDEROBJECTRETURN(r, v) if (m_pBuffer == NULL) return v; \
  GdiRender* r = (GdiRender*)m_pBuffer

  MapPainterGDI::MapPainterGDI(const StyleConfigRef &styleConfig)
    : MapPainter(styleConfig),
      m_labelLayouter(this),
      m_pBuffer(nullptr) {
    if (m_gdiplusInstCount == 0) {
      Gdiplus::GdiplusStartupInput gdiplusStartupInput;
      Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, nullptr);
    }

    m_gdiplusInstCount++;
  }

  MapPainterGDI::~MapPainterGDI() {
    if (m_pBuffer != nullptr) {
      ((GdiRender *) m_pBuffer)->Release();
      delete (GdiRender *) m_pBuffer;
      m_pBuffer = nullptr;
    }

    m_gdiplusInstCount--;

    if (m_gdiplusToken > 0 && m_gdiplusInstCount == 0) {
      Gdiplus::GdiplusShutdown(m_gdiplusToken);
      m_gdiplusToken = 0;
    }
  }

  template<>
  std::vector<Glyph<MapPainterGDI::NativeGlyph>> MapPainterGDI::GdiLabel::ToGlyphs() const {
    std::vector<Glyph<MapPainterGDI::NativeGlyph>> result;
    double horizontalOffset = 0;
    std::array<wchar_t, 2> buffer;

    buffer[1] = '\0';

    result.reserve(label.wstr.length());
    for (wchar_t ch: label.wstr) {
      buffer[0] = ch;

      result.emplace_back();
      result.back().glyph.character = WStringToUTF8String(buffer.data());
      result.back().position=Vertex2D(horizontalOffset,0);

      Gdiplus::RectF bb;

      ((GdiRender *) label.render)->m_pGraphics->MeasureString(buffer.data(),
                                                               -1,
                                                               (const Gdiplus::Font *) label.font,
                                                               Gdiplus::PointF(0, fontSize * 2),
                                                               &bb);
      horizontalOffset += bb.Width;
    }

    return result;
  }

  DoubleRectangle MapPainterGDI::GlyphBoundingBox(const NativeGlyph &glyph) const {
    return {0.0,
            (double) (glyph.height * -1),
            (double) glyph.width,
            (double) glyph.height};
  }

  std::shared_ptr<MapPainterGDI::GdiLabel> MapPainterGDI::Layout(const Projection &projection,
                                                                 const MapParameter &parameter,
                                                                 const std::string &text,
                                                                 double fontSize,
                                                                 double /*objectWidth*/,
                                                                 bool /*enableWrapping*/,
                                                                 bool /*contourLabel*/) {
    auto label = std::make_shared<MapPainterGDI::GdiLabel>();

    label->label.wstr = UTF8StringToWString(text);
    label->text = text;
    label->fontSize = fontSize;
    label->height = projection.ConvertWidthToPixel(fontSize * parameter.GetFontSize());
    Gdiplus::Font *pFont = ((GdiRender *) m_pBuffer)->GetFont(parameter.GetFontName(), projection.ConvertWidthToPixel(
      fontSize * parameter.GetFontSize()));
    Gdiplus::RectF bb;
    std::wstring wtext = UTF8StringToWString(text);
    ((GdiRender *) m_pBuffer)->m_pGraphics->MeasureString(wtext.c_str(),
                                                          -1,
                                                          pFont,
                                                          Gdiplus::PointF(0, label->height),
                                                          &bb);
    label->width = bb.Width;
    label->label.font = pFont;
    label->label.render = m_pBuffer;

    return label;
  }

  void MapPainterGDI::DrawLabel(const Projection &projection,
                                const MapParameter &parameter,
                                const DoubleRectangle &labelRectangle,
                                const LabelData &label,
                                const NativeLabel &/*layout*/) {
    RENDEROBJECT(pRender);
    Gdiplus::RectF rectF(labelRectangle.x, labelRectangle.y, labelRectangle.width, labelRectangle.height);
    Gdiplus::StringFormat stringFormat;

    if (const TextStyle *textStyle = dynamic_cast<const TextStyle *>(label.style.get()); textStyle != nullptr) {
      std::wstring text = osmscout::UTF8StringToWString(label.text);
      Gdiplus::Font *pFont = pRender->GetFont(parameter.GetFontName(),
                                              projection.ConvertWidthToPixel(label.fontSize * parameter.GetFontSize()));

      if (textStyle->GetStyle() == TextStyle::normal) {
        osmscout::Color fill = textStyle->GetTextColor();

        if (label.alpha != 1.0) {
          fill = osmscout::Color(fill.GetR(), fill.GetG(), fill.GetB(), label.alpha);
        }

        Gdiplus::SolidBrush brush(pRender->ColorToGdiColor(fill));

        pRender->m_pGraphics->DrawString(text.c_str(),
                                         (INT) text.length(),
                                         pFont,
                                         rectF,
                                         &stringFormat,
                                         &brush);
      } else if (textStyle->GetStyle() == TextStyle::emphasize) {
        Gdiplus::FontFamily fontFamily;
        pFont->GetFamily(&fontFamily);
        Gdiplus::GraphicsPath path;
        path.AddString(text.c_str(),
                       (INT) text.length(),
                       &fontFamily,
                       pFont->GetStyle(),
                       projection.ConvertWidthToPixel(label.fontSize * parameter.GetFontSize()),
                       rectF,
                       &stringFormat);
        osmscout::Color fill = textStyle->GetEmphasizeColor();

        if (label.alpha != 1.0) {
          fill = osmscout::Color(fill.GetR(), fill.GetG(), fill.GetB(), label.alpha);
        }

        Gdiplus::SolidBrush brush(pRender->ColorToGdiColor(fill));

        Gdiplus::Pen *pPen = pRender->GetPen(fill);
        pRender->m_pGraphics->DrawPath(pPen, &path);
        pRender->m_pGraphics->FillPath(&brush, &path);
      }
    } else if (const ShieldStyle *shieldStyle = dynamic_cast<const ShieldStyle *>(label.style.get()); shieldStyle != nullptr) {
      // Shield background
      Gdiplus::SolidBrush backgroundBrush(pRender->ColorToGdiColor(shieldStyle->GetBgColor()));
      pRender->m_pGraphics->FillRectangle(&backgroundBrush,
                                          (INT) labelRectangle.x - 2,
                                          (INT) labelRectangle.y - 2,
                                          (INT) labelRectangle.width + 5,
                                          (INT) labelRectangle.height + 7);

      // Shield inner border
      Gdiplus::Pen *pPen = pRender->GetPen(shieldStyle->GetBorderColor());
      pRender->m_pGraphics->DrawRectangle(pPen,
                                          (INT) labelRectangle.x,
                                          (INT) labelRectangle.y,
                                          (INT) labelRectangle.width,
                                          (INT) labelRectangle.height + 2);

      Gdiplus::Font *pFont = pRender->GetFont(parameter.GetFontName(),
                                              projection.ConvertWidthToPixel(label.fontSize * parameter.GetFontSize()));
      osmscout::Color textColor = shieldStyle->GetTextColor();

      if (label.alpha != 1.0) {
        textColor.Alpha(label.alpha);
      }

      Gdiplus::SolidBrush textBrush(pRender->ColorToGdiColor(textColor));
      std::wstring text = osmscout::UTF8StringToWString(label.text);
      pRender->m_pGraphics->DrawString(text.c_str(),
                                       (INT) text.length(),
                                       pFont,
                                       rectF,
                                       &stringFormat,
                                       &textBrush);
    }
  }

  void MapPainterGDI::DrawGlyphs(const Projection &projection,
                                 const MapParameter &parameter,
                                 const osmscout::PathTextStyleRef &style,
                                 const std::vector<GdiGlyph> &glyphs) {
    assert(!glyphs.empty());
    RENDEROBJECT(pRender);

    Gdiplus::Font *pFont = pRender->GetFont(parameter.GetFontName(),
                                            projection.ConvertWidthToPixel(style->GetSize() * parameter.GetFontSize()));
    Gdiplus::SolidBrush brush(pRender->ColorToGdiColor(style->GetTextColor()));

    for (auto const &glyph: glyphs) {
      if (glyph.glyph.character.empty() ||
          (glyph.glyph.character.length() == 1 &&
           (glyph.glyph.character == " " || glyph.glyph.character == "\t" || glyph.glyph.character == " "))) {
        continue;
      }
      std::wstring text = osmscout::UTF8StringToWString(glyph.glyph.character);
      Gdiplus::PointF ptf((Gdiplus::REAL) glyph.position.GetX(), (Gdiplus::REAL) glyph.position.GetY());
      Gdiplus::RectF bb;

      pRender->m_pGraphics->MeasureString(text.c_str(),
                                          -1,
                                          pFont,
                                          ptf,
                                          &bb);

      Gdiplus::Matrix m;

      m.RotateAt((Gdiplus::REAL) RadToDeg(glyph.angle), ptf);

      pRender->m_pGraphics->SetTransform(&m);
      pRender->m_pGraphics->DrawString(text.c_str(),
                                       -1,
                                       pFont,
                                       ptf - Gdiplus::PointF(0.5f * bb.Width, 0.5f * bb.Height),
                                       &brush);
      pRender->m_pGraphics->ResetTransform();
    }
  }

  void MapPainterGDI::AfterPreprocessing(const StyleConfig & /*styleConfig*/,
                                         const Projection & /*projection*/,
                                         const MapParameter & /*parameter*/,
                                         const MapData & /*data*/) {
    // Not implemented
  }

  void MapPainterGDI::BeforeDrawing(const StyleConfig & /*styleConfig*/,
                                    const Projection &projection,
                                    const MapParameter &parameter,
                                    const MapData & /*data*/) {
    DoubleRectangle viewport(0.0, 0.0, (double) projection.GetWidth(), (double) projection.GetHeight());
    m_labelLayouter.SetViewport(viewport);
    m_labelLayouter.SetLayoutOverlap(projection.ConvertWidthToPixel(parameter.GetLabelLayouterOverlap()));
  }

  void MapPainterGDI::AfterDrawing(const StyleConfig & /*styleConfig*/,
                                   const Projection & /*projection*/,
                                   const MapParameter & /*parameter*/,
                                   const MapData & /*data*/) {
    // Not implemented
  }

  bool MapPainterGDI::HasIcon(const StyleConfig & /*styleConfig*/,
                              const Projection &projection,
                              const MapParameter &parameter,
                              IconStyle &style) {
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
    } else if (parameter.GetIconMode() == MapParameter::IconMode::ScaledPixmap) {
      dimension = std::round(projection.ConvertWidthToPixel(parameter.GetIconSize()));
    } else {
      dimension = std::round(parameter.GetIconPixelSize());
    }

    style.SetWidth((unsigned int) dimension);
    style.SetHeight((unsigned int) dimension);

    Gdiplus::Image *pImage = pRender->GetIcon(idx);
    if (pImage != nullptr) {
      return true;
    }

    for (const auto &path: parameter.GetIconPaths()) {
      pImage = pRender->GetIcon(idx, AppendFileToDir(path, style.GetIconName()));
      if (pImage != nullptr) {
        return true;
      }
    }

    style.SetIconId(0);

    return false;
  }

  double MapPainterGDI::GetFontHeight(const Projection &projection,
                                      const MapParameter &parameter,
                                      double fontSize) {
    return projection.ConvertWidthToPixel(fontSize * parameter.GetFontSize());
  }

  void MapPainterGDI::DrawGround(const Projection &projection,
                                 const MapParameter & /*parameter*/,
                                 const FillStyle &style) {
    RENDEROBJECT(pRender);

    Gdiplus::SolidBrush brush(pRender->ColorToGdiColor(style.GetFillColor()));

    pRender->m_pGraphics->FillRectangle(&brush, 0, 0, (INT) projection.GetWidth(), (INT) projection.GetHeight());
  }

  void MapPainterGDI::RegisterRegularLabel(const Projection &projection,
                                           const MapParameter &parameter,
                                           const std::vector<LabelData> &labels,
                                           const Vertex2D &position,
                                           double objectWidth) {
    m_labelLayouter.RegisterLabel(projection, parameter, position, labels, objectWidth);
  }

  void MapPainterGDI::RegisterContourLabel(const Projection &projection,
                                           const MapParameter &parameter,
                                           const PathLabelData &label,
                                           const LabelPath &labelPath) {
    m_labelLayouter.RegisterContourLabel(projection, parameter, label, labelPath);
  }

  void MapPainterGDI::DrawLabels(const Projection &projection,
                                 const MapParameter &parameter,
                                 const MapData & /*data*/) {
    m_labelLayouter.Layout(projection, parameter);
    m_labelLayouter.DrawLabels(projection,
                               parameter,
                               this);
    m_labelLayouter.Reset();
  }

  void MapPainterGDI::DrawSymbol(const Projection &projection,
                                 const MapParameter & /*parameter*/,
                                 const Symbol &symbol,
                                 const Vertex2D& screenPos,
                                 double /*scaleFactor*/) {
    Gdiplus::Pen *pPen;

    RENDEROBJECT(pRender);
    ScreenBox boundingBox=symbol.GetBoundingBox(projection);
    Vertex2D center=boundingBox.GetCenter();

    for (const auto &primitive: symbol.GetPrimitives()) {
      const DrawPrimitive *primitivePtr = primitive.get();

      const auto *polygon = dynamic_cast<const PolygonPrimitive *>(primitivePtr);
      const auto *rectangle = dynamic_cast<const RectanglePrimitive *>(primitivePtr);
      const auto *circle = dynamic_cast<const CirclePrimitive *>(primitivePtr);

      if (polygon != nullptr) {
        pPen = (polygon->GetBorderStyle()) ? pRender->GetPen(polygon->GetBorderStyle()) : nullptr;

        pRender->pointBuffer.ResetAndReserve(polygon->GetCoords().size());

        for (const auto &pixel: polygon->GetCoords()) {
          pRender->pointBuffer.AddPoint(screenPos.GetX() + projection.ConvertWidthToPixel(pixel.GetX()) - center.GetX(),
                                        screenPos.GetY() + projection.ConvertWidthToPixel(pixel.GetY()) - center.GetY());
        }

        if (polygon->GetFillStyle()) {
          Gdiplus::SolidBrush fillBrush(pRender->ColorToGdiColor(polygon->GetFillStyle()->GetFillColor()));

          pRender->m_pGraphics->FillPolygon(&fillBrush,
                                            pRender->pointBuffer.m_Data.data(),
                                            (INT) pRender->pointBuffer.m_Data.size());
        }

        if (pPen != nullptr) {
          pRender->m_pGraphics->DrawPolygon(pPen,
                                            pRender->pointBuffer.m_Data.data(),
                                            (INT) pRender->pointBuffer.m_Data.size());
        }
      } else if (rectangle != nullptr) {
        pPen = (rectangle->GetBorderStyle()) ? pRender->GetPen(rectangle->GetBorderStyle()) : nullptr;
        Gdiplus::RectF rect(
          (Gdiplus::REAL) (screenPos.GetX() + projection.ConvertWidthToPixel(rectangle->GetTopLeft().GetX()) - center.GetX()),
          (Gdiplus::REAL) (screenPos.GetY() + projection.ConvertWidthToPixel(rectangle->GetTopLeft().GetY()) - center.GetY()),
          (Gdiplus::REAL) (projection.ConvertWidthToPixel(rectangle->GetWidth())),
          (Gdiplus::REAL) (projection.ConvertWidthToPixel(rectangle->GetHeight()))
        );

        if (rectangle->GetFillStyle()) {
          Gdiplus::SolidBrush fillBrush(pRender->ColorToGdiColor(rectangle->GetFillStyle()->GetFillColor()));

          pRender->m_pGraphics->FillRectangle(&fillBrush, rect);
        }

        if (pPen != nullptr) {
          pRender->m_pGraphics->DrawRectangle(pPen, rect);
        }
      } else if (circle != nullptr) {
        pPen = (circle->GetBorderStyle()) ? pRender->GetPen(circle->GetBorderStyle()) : nullptr;
        Gdiplus::RectF rect(
          (Gdiplus::REAL) (screenPos.GetX() + projection.ConvertWidthToPixel(circle->GetCenter().GetX()) - center.GetX() -
                           2 * projection.ConvertWidthToPixel(circle->GetRadius())),
          (Gdiplus::REAL) (screenPos.GetY() + projection.ConvertWidthToPixel(circle->GetCenter().GetY()) - center.GetY() -
                           2 * projection.ConvertWidthToPixel(circle->GetRadius())),
          (Gdiplus::REAL) (2 * projection.ConvertWidthToPixel(circle->GetRadius())),
          (Gdiplus::REAL) (2 * projection.ConvertWidthToPixel(circle->GetRadius()))
        );

        if (circle->GetFillStyle()) {
          Gdiplus::SolidBrush fillBrush(pRender->ColorToGdiColor(circle->GetFillStyle()->GetFillColor()));

          pRender->m_pGraphics->FillEllipse(&fillBrush, rect);
        }

        if (pPen != nullptr) {
          pRender->m_pGraphics->DrawEllipse(pPen, rect);
        }
      }
    }
  }

  void MapPainterGDI::DrawIcon(const IconStyle *style,
                               const Vertex2D& centerPos,
                               double width, double height) {
    RENDEROBJECT(pRender);
    Gdiplus::Image *pImage = pRender->GetIcon(style->GetIconId());

    if (pImage != nullptr) {
      pRender->m_pGraphics->DrawImage(pImage,
                                      (INT) (centerPos.GetX() - width / 2.0),
                                      (INT) (centerPos.GetY() - height / 2.0),
                                      (INT) width,
                                      (INT) height);
    }
  }

  void MapPainterGDI::DrawPath(const Projection & /*projection*/,
                               const MapParameter & /*parameter*/,
                               const Color &color,
                               double width,
                               const std::vector<double> &dash,
                               LineStyle::CapStyle startCap,
                               LineStyle::CapStyle endCap,
                               const CoordBufferRange& coordRange) {
    RENDEROBJECT(pRender);
    Gdiplus::Pen *pPen = pRender->GetPen(color, width, dash, startCap, endCap);

    pRender->pointBuffer.ResetAndReserve(coordRange.GetSize());
    for (size_t i = coordRange.GetStart(); i <= coordRange.GetEnd(); i++) {
      pRender->pointBuffer.AddPoint(coordBuffer.buffer[i].GetX(), coordBuffer.buffer[i].GetY());
    }

    pRender->m_pGraphics->DrawLines(pPen,
                                    pRender->pointBuffer.m_Data.data(),
                                    (INT) pRender->pointBuffer.m_Data.size());
  }

  void MapPainterGDI::DrawWayOutline(const StyleConfig & /*styleConfig*/,
                                     const Projection & /*projection*/,
                                     const MapParameter & /*parameter*/,
                                     const WayData & /*data*/) {
    // Not implemented
  }

  void MapPainterGDI::DrawWay(const StyleConfig & /*styleConfig*/,
                              const Projection &projection,
                              const MapParameter &parameter,
                              const WayData &data) {
    if (!data.lineStyle->GetDash().empty() && data.lineStyle->GetGapColor().GetA() > 0.0) {
      DrawPath(projection,
               parameter,
               data.lineStyle->GetGapColor(),
               data.lineWidth,
               emptyDash,
               data.startIsClosed ? data.lineStyle->GetEndCap() : data.lineStyle->GetJoinCap(),
               data.endIsClosed ? data.lineStyle->GetEndCap() : data.lineStyle->GetJoinCap(),
               data.coordRange);
    }

    DrawPath(projection,
             parameter,
             data.lineStyle->GetLineColor(),
             data.lineWidth,
             data.lineStyle->GetDash(),
             data.startIsClosed ? data.lineStyle->GetEndCap() : data.lineStyle->GetJoinCap(),
             data.endIsClosed ? data.lineStyle->GetEndCap() : data.lineStyle->GetJoinCap(),
             data.coordRange);
  }

  void MapPainterGDI::DrawContourSymbol(const Projection & /*projection*/,
                                        const MapParameter & /*parameter*/,
                                        const Symbol & /*symbol*/,
                                        const ContourSymbolData& /*data*/) {
    // Not implemented
  }

  void MapPainterGDI::DrawArea(const Projection & /*projection*/,
                               const MapParameter & /*parameter*/,
                               const MapPainter::AreaData &area) {
    RENDEROBJECT(pRender);

    pRender->pointBuffer.ResetAndReserve(area.coordRange.GetEnd() - area.coordRange.GetStart() + 1);
    for (size_t i = area.coordRange.GetStart(); i <= area.coordRange.GetEnd(); i++) {
      pRender->pointBuffer.AddPoint(coordBuffer.buffer[i].GetX(),
                                    coordBuffer.buffer[i].GetY());
    }

    struct clippingRegion {
      PointFBuffer points;
      Gdiplus::GraphicsPath *path;
      Gdiplus::Region *region;
    };

    std::vector<clippingRegion> clippings;

    clippings.reserve(area.clippings.size());
    for (const auto &clippingPolygon: area.clippings) {
      clippingRegion clipping;

      clipping.points.ResetAndReserve(clippingPolygon.GetEnd() - clippingPolygon.GetStart() + 1);
      for (size_t i = clippingPolygon.GetStart(); i <= clippingPolygon.GetEnd(); i++) {
        clipping.points.AddPoint(coordBuffer.buffer[i].GetX(),
                                 coordBuffer.buffer[i].GetY());
      }

      clipping.path = new Gdiplus::GraphicsPath();
      clipping.path->AddLines(clipping.points.m_Data.data(),
                              (INT) clipping.points.m_Data.size());

      clipping.region = new Gdiplus::Region(clipping.path);
      pRender->m_pGraphics->ExcludeClip(clipping.region);

      clippings.push_back(clipping);
    }

    if (area.fillStyle) {
      Gdiplus::SolidBrush brush(pRender->ColorToGdiColor(area.fillStyle->GetFillColor()));
      pRender->m_pGraphics->FillPolygon(&brush,
                                        pRender->pointBuffer.m_Data.data(),
                                        (INT) pRender->pointBuffer.m_Data.size());
    }

    pRender->m_pGraphics->ResetClip();

    if (area.borderStyle) {
      Gdiplus::Pen *pPen = pRender->GetPen(area.borderStyle);

      pRender->m_pGraphics->DrawPolygon(pPen,
                                        pRender->pointBuffer.m_Data.data(),
                                        (INT)pRender->pointBuffer.m_Data.size());
    }

    for (auto &clipping: clippings) {
      delete clipping.region;
      delete clipping.path;
    }

    clippings.clear();
  }

  bool MapPainterGDI::DrawMap(const Projection &projection,
                              const MapParameter &parameter,
                              const MapData &data,
                              HDC hdc,
                              RenderSteps startStep,
                              RenderSteps endStep) {
    if (startStep==RenderSteps::Initialize) {
      if (m_pBuffer == nullptr || ((GdiRender *) m_pBuffer)->m_pGraphics->GetHDC()!=hdc) {
        if (m_pBuffer != nullptr) {
          ((GdiRender *) m_pBuffer)->Release();
          delete ((GdiRender *) m_pBuffer);
        }
        m_pBuffer = new GdiRender(hdc);
      }
    }

    return Draw(projection,
                parameter,
                data,
                startStep,
                endStep);
  }
}
