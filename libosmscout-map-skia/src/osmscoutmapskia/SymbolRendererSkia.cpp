/*
  This source is part of the libosmscout-map library
  Copyright (C) 2026  Tim Teulings

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

#include <osmscoutmapskia/SymbolRendererSkia.h>

#include <osmscout/log/Logger.h>

#include <core/SkPathBuilder.h>
#include <effects/SkDashPathEffect.h>

namespace osmscout {

  SymbolRendererSkia::SymbolRendererSkia(SkCanvas* draw)
    :
    draw(draw)
  {}

  void SymbolRendererSkia::BeginPrimitive()
  {
    fillStyle=nullptr;
    borderStyle=nullptr;
  }

  void SymbolRendererSkia::SetFill(const FillStyleRef& fillStyle)
  {
    this->fillStyle=fillStyle;

    if (fillStyle && fillStyle->HasPattern()) {
      log.Warn() << "Pattern is not supported for symbols";
    }
  }

  void SymbolRendererSkia::SetBorder(const BorderStyleRef& borderStyle,
                                     double screenMmInPixel)
  {
    this->borderStyle=borderStyle;
    this->screenMmInPixel=screenMmInPixel;
  }

  void SymbolRendererSkia::DrawPolygon(const std::vector<Vertex2D>& polygonPixels)
  {
    SkPathBuilder builder;

    for (auto pixel=polygonPixels.begin();
         pixel!=polygonPixels.end();
         ++pixel) {
      if (pixel==polygonPixels.begin()) {
        builder.moveTo(static_cast<SkScalar>(pixel->GetX()),
                       static_cast<SkScalar>(pixel->GetY()));
      }
      else {
        builder.lineTo(static_cast<SkScalar>(pixel->GetX()),
                       static_cast<SkScalar>(pixel->GetY()));
      }
    }

    builder.close();
    path = builder.detach();
  }

  void SymbolRendererSkia::DrawRect(double x,
                                    double y,
                                    double w,
                                    double h)
  {
    SkPathBuilder builder;

    builder.moveTo(static_cast<SkScalar>(x), static_cast<SkScalar>(y));
    builder.lineTo(static_cast<SkScalar>(x + w), static_cast<SkScalar>(y));
    builder.lineTo(static_cast<SkScalar>(x + w), static_cast<SkScalar>(y + h));
    builder.lineTo(static_cast<SkScalar>(x), static_cast<SkScalar>(y + h));
    builder.close();

    path = builder.detach();
  }

  void SymbolRendererSkia::DrawCircle(double x,
                                     double y,
                                     double radius)
  {
    SkPathBuilder builder;

    // Approximate circle with line segments
    const int numSegments = 32;
    for (int i = 0; i <= numSegments; i++) {
      double angle = 2.0 * M_PI * i / numSegments;
      SkScalar px = static_cast<SkScalar>(x + radius * cos(angle));
      SkScalar py = static_cast<SkScalar>(y + radius * sin(angle));
      if (i == 0) {
        builder.moveTo(px, py);
      } else {
        builder.lineTo(px, py);
      }
    }

    builder.close();
    path = builder.detach();
  }

  void SymbolRendererSkia::EndPrimitive()
  {
    bool   hasFill=false;
    bool   hasBorder=false;
    double borderWidth=borderStyle ? borderStyle->GetWidth() * screenMmInPixel : 0.0;

    if (fillStyle) {
      if (fillStyle->HasPattern()) {
        // Warning already logged in SetFill
      }
      else if (fillStyle->GetFillColor().IsVisible()) {
        hasFill = true;
      }
    }

    if (borderStyle) {
      hasBorder = borderWidth > 0 &&
                  borderStyle->GetColor().IsVisible();
    }

    if (hasFill) {
      SkPaint fillPaint;
      fillPaint.setAntiAlias(true);
      fillPaint.setStyle(SkPaint::kFill_Style);
      Color fillColor = fillStyle->GetFillColor();
      fillPaint.setColor(SkColorSetARGB(
          static_cast<uint8_t>(fillColor.GetA() * 255),
          static_cast<uint8_t>(fillColor.GetR() * 255),
          static_cast<uint8_t>(fillColor.GetG() * 255),
          static_cast<uint8_t>(fillColor.GetB() * 255)));

      draw->drawPath(path, fillPaint);
    }

    if (hasBorder) {
      Color borderColor = borderStyle->GetColor();
      SkPaint borderPaint;
      borderPaint.setAntiAlias(true);
      borderPaint.setStyle(SkPaint::kStroke_Style);
      borderPaint.setStrokeWidth(static_cast<SkScalar>(borderWidth));
      borderPaint.setColor(SkColorSetARGB(
          static_cast<uint8_t>(borderColor.GetA() * 255),
          static_cast<uint8_t>(borderColor.GetR() * 255),
          static_cast<uint8_t>(borderColor.GetG() * 255),
          static_cast<uint8_t>(borderColor.GetB() * 255)));

      if (!borderStyle->HasDashes()) {
        borderPaint.setPathEffect(nullptr);
      }
      else {
        std::vector<SkScalar> dashArray;
        dashArray.reserve(borderStyle->GetDash().size());
        for (double d : borderStyle->GetDash()) {
          dashArray.push_back(static_cast<SkScalar>(d * borderWidth));
        }
        borderPaint.setPathEffect(SkDashPathEffect::Make(
            SkSpan<const SkScalar>(dashArray.data(), dashArray.size()),
            0.0f));
      }

      borderPaint.setStrokeCap(SkPaint::kButt_Cap);
      draw->drawPath(path, borderPaint);
    }
  }
}
