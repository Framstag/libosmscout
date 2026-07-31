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

#include <osmscoutmapskia/MapPainterSkia.h>

#include <osmscoutmapskia/SymbolRendererSkia.h>

#include <core/SkBitmap.h>
#include <core/SkColor.h>
#include <core/SkData.h>
#include <core/SkFont.h>
#include <core/SkFontMetrics.h>
#include <core/SkFontMgr.h>
#include <core/SkImage.h>
#include <core/SkImageInfo.h>
#include <core/SkPaint.h>
#include <core/SkPath.h>
#include <core/SkPathBuilder.h>
#include <core/SkPoint.h>
#include <core/SkRRect.h>
#include <core/SkShader.h>
#include <core/SkSurface.h>
#include <core/SkTypeface.h>
#include <effects/SkDashPathEffect.h>

#if defined(_WIN32)
  #include <ports/SkTypeface_win.h>
#elif defined(__APPLE__)
  #include <ports/SkFontMgr_mac_ct.h>
#else
  #include <ports/SkFontMgr_fontconfig.h>
  #include <ports/SkFontScanner_FreeType.h>
#endif

#include <osmscoutmapskia/MapSkiaFeatures.h>

#include <osmscout/io/File.h>

#ifdef OSMSCOUT_HAVE_SKIA_SVG
  #include <svg/include/SkSVGDOM.h>
  #include "include/core/SkStream.h"
#else
  #define NANOSVG_IMPLEMENTATION
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wsign-compare"
  #pragma GCC diagnostic ignored "-Wcast-qual"
  #include "nanosvg.h"
  #pragma GCC diagnostic pop
  #define NANOSVGRAST_IMPLEMENTATION
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wsign-compare"
  #pragma GCC diagnostic ignored "-Wcast-qual"
  #include "nanosvgrast.h"
  #pragma GCC diagnostic pop
#endif

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

#include <osmscout/log/Logger.h>
#include <osmscout/util/Geometry.h>
#include <osmscout/util/String.h>

namespace osmscout {

  MapPainterSkia::MapPainterSkia()
  : labelLayouter(this),
#if defined(_WIN32)
    fontMgr(SkFontMgr_New_DirectWrite())
#elif defined(__APPLE__)
    fontMgr(SkFontMgr_New_CoreText(nullptr))
#else
    fontMgr(SkFontMgr_New_FontConfig(nullptr, SkFontScanner_Make_FreeType()))
#endif
  {
    log.Debug() << "MapPainterSkia::MapPainterSkia() fontMgr=" << (fontMgr ? "valid" : "null");
  }

  MapPainterSkia::~MapPainterSkia()
  {
    // no code
  }

  sk_sp<SkTypeface> MapPainterSkia::GetTypeface(const std::string& fontName)
  {
    FontDescriptor desc{fontName, 0.0};
    auto it = fontCache.find(desc);
    if (it != fontCache.end()) {
      return it->second;
    }

    sk_sp<SkTypeface> typeface;
    if (fontMgr) {
      if (!fontName.empty()) {
        typeface = fontMgr->legacyMakeTypeface(fontName.c_str(), SkFontStyle::Normal());
        log.Debug() << "MapPainterSkia: loading font '" << fontName << "': "
                    << (typeface ? "found" : "not found");
      }
      if (!typeface) {
        typeface = fontMgr->legacyMakeTypeface(nullptr, SkFontStyle::Normal());
        log.Debug() << "MapPainterSkia: loading default font: "
                    << (typeface ? "found" : "not found");
      }
    } else {
      log.Error() << "MapPainterSkia: fontMgr is null!";
    }
    if (!typeface) {
      typeface = SkTypeface::MakeEmpty();
      log.Warn() << "MapPainterSkia: using empty typeface";
    }

    fontCache[desc] = typeface;
    return typeface;
  }

  void MapPainterSkia::SetLineAttributes(SkPaint& paint,
                                          const Color& color,
                                          double width,
                                          const std::vector<double>& dash,
                                          LineStyle::CapStyle startCap,
                                          LineStyle::CapStyle endCap)
  {
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setColor(SkColorSetARGB(
        static_cast<uint8_t>(color.GetA() * 255),
        static_cast<uint8_t>(color.GetR() * 255),
        static_cast<uint8_t>(color.GetG() * 255),
        static_cast<uint8_t>(color.GetB() * 255)));
    paint.setStrokeWidth(static_cast<SkScalar>(width));
    paint.setStrokeJoin(SkPaint::kRound_Join);

    // Map cap styles
    SkPaint::Cap skCap = SkPaint::kRound_Cap;
    if (startCap == LineStyle::capButt || endCap == LineStyle::capButt) {
      skCap = SkPaint::kButt_Cap;
    } else if (startCap == LineStyle::capSquare || endCap == LineStyle::capSquare) {
      skCap = SkPaint::kSquare_Cap;
    }
    paint.setStrokeCap(skCap);

    // Dash pattern
    if (!dash.empty()) {
      std::vector<SkScalar> intervals;
      intervals.reserve(dash.size());
      for (double d : dash) {
        intervals.push_back(static_cast<SkScalar>(d * width));
      }
      paint.setPathEffect(SkDashPathEffect::Make(SkSpan<const SkScalar>(intervals.data(),
                                                                          intervals.size()),
                                                  0));
    } else {
      paint.setPathEffect(nullptr);
    }
  }

  void MapPainterSkia::DrawFillStyle(const Projection& projection,
                                      const MapParameter& parameter,
                                      const FillStyleRef& fill,
                                      const BorderStyleRef& border,
                                      const SkPath& path)
  {
    bool hasFill = false;

    if (fill) {
      if (fill->HasPattern() &&
          projection.GetMagnification() >= fill->GetPatternMinMag()) {
        // Try to load pattern
        for (const auto& patternPath : parameter.GetPatternPaths()) {
          std::string filename = patternPath + fill->GetPatternName() + ".png";

          auto it = patternCache.find(filename);
          sk_sp<SkShader> shader;

          if (it != patternCache.end()) {
            shader = it->second;
          } else {
            sk_sp<SkData> data = SkData::MakeFromFileName(filename.c_str());
            if (data) {
              sk_sp<SkImage> image = SkImages::DeferredFromEncodedData(std::move(data));
              if (image) {
                shader = image->makeShader(SkTileMode::kRepeat,
                                           SkTileMode::kRepeat,
                                           SkSamplingOptions());
              }
            }
            patternCache[filename] = shader;
          }

          if (shader) {
            SkPaint fillPaint;
            fillPaint.setAntiAlias(true);
            fillPaint.setStyle(SkPaint::kFill_Style);
            fillPaint.setShader(shader);
            draw->drawPath(path, fillPaint);
            hasFill = true;
            break;
          }
        }
      }

      if (!hasFill && fill->GetFillColor().IsVisible()) {
        const Color& fillColor = fill->GetFillColor();
        SkPaint fillPaint;
        fillPaint.setAntiAlias(true);
        fillPaint.setStyle(SkPaint::kFill_Style);
        fillPaint.setColor(SkColorSetARGB(
            static_cast<uint8_t>(fillColor.GetA() * 255),
            static_cast<uint8_t>(fillColor.GetR() * 255),
            static_cast<uint8_t>(fillColor.GetG() * 255),
            static_cast<uint8_t>(fillColor.GetB() * 255)));
        draw->drawPath(path, fillPaint);
        hasFill = true;
      }
    }

    if (border) {
      bool hasBorder = border->GetWidth() > 0 &&
                       border->GetColor().IsVisible();

      if (hasBorder) {
        double borderWidth = projection.ConvertWidthToPixel(border->GetWidth());

        if (borderWidth >= parameter.GetLineMinWidthPixel()) {
          // Gap color pass
          if (border->HasDashes() && border->GetGapColor().IsVisible()) {
            SkPaint gapPaint;
            SetLineAttributes(gapPaint,
                              border->GetGapColor(),
                              borderWidth,
                              std::vector<double>(), // solid
                              LineStyle::capButt,
                              LineStyle::capButt);
            draw->drawPath(path, gapPaint);
          }

          // Border pass
          SkPaint borderPaint;
          SetLineAttributes(borderPaint,
                            border->GetColor(),
                            borderWidth,
                            border->GetDash(),
                            LineStyle::capButt,
                            LineStyle::capButt);
          draw->drawPath(path, borderPaint);
        }
      }
    }
  }

  bool MapPainterSkia::HasIcon(const StyleConfig& /*styleConfig*/,
                                const Projection& projection,
                                const MapParameter& parameter,
                                IconStyle& style)
  {
    // Already loaded with error
    if (style.GetIconName().empty()) {
      return false;
    }

    // Setup dimensions for all IconStyle instances with same iconId
    if (parameter.GetIconMode()==MapParameter::IconMode::Scalable ||
        parameter.GetIconMode()==MapParameter::IconMode::ScaledPixmap){
      style.SetWidth(std::round(projection.ConvertWidthToPixel(parameter.GetIconSize())));
      style.SetHeight(style.GetWidth());
    }else{
      style.SetWidth(std::round(parameter.GetIconPixelSize()));
      style.SetHeight(style.GetWidth());
    }

    // Already cached?
    auto cacheIt = iconCache.find(style.GetIconName());
    if (cacheIt != iconCache.end() && cacheIt->second) {
      if (parameter.GetIconMode()==MapParameter::IconMode::OriginalPixmap){
        style.SetWidth(cacheIt->second->width());
        style.SetHeight(cacheIt->second->height());
      }
      return true;
    }

    for (const auto& path : parameter.GetIconPaths()) {
      std::string filename = AppendFileToDir(path, style.GetIconName() + ".png");

      sk_sp<SkData> data = SkData::MakeFromFileName(filename.c_str());
      if (data) {
        sk_sp<SkImage> image = SkImages::DeferredFromEncodedData(std::move(data));
        if (image) {
          iconCache[style.GetIconName()] = image;

          if (parameter.GetIconMode()==MapParameter::IconMode::OriginalPixmap){
            style.SetWidth(image->width());
            style.SetHeight(image->height());
          }
          return true;
        }
      }
    }

    // No PNG found — try SVG
    for (const auto& path : parameter.GetIconPaths()) {
      std::string filename = AppendFileToDir(path, style.GetIconName() + ".svg");

#ifdef OSMSCOUT_HAVE_SKIA_SVG
      // SkSVGDOM path
      SkFILEStream stream(filename.c_str());
      if (!stream.isValid()) {
        continue;
      }

      sk_sp<SkSVGDOM> svgDom = SkSVGDOM::Make(stream);
      if (!svgDom) {
        continue;
      }

      // Determine size
      SkSize svgSize = svgDom->containerSize();
      if (svgSize.isEmpty()) {
        svgSize = SkSize::Make(100, 100);
      }

      if (parameter.GetIconMode()==MapParameter::IconMode::Scalable ||
          parameter.GetIconMode()==MapParameter::IconMode::ScaledPixmap) {
        float targetSize = style.GetWidth();
        float scale = targetSize / std::max(svgSize.width(), svgSize.height());
        svgSize = SkSize::Make(svgSize.width() * scale, svgSize.height() * scale);
      }
      svgDom->setContainerSize(svgSize);

      auto surface = SkSurface::MakeRasterN32Premul(
          static_cast<int>(svgSize.width()),
          static_cast<int>(svgSize.height()));
      if (!surface) {
        continue;
      }
      svgDom->render(surface->getCanvas());

      sk_sp<SkImage> image = surface->makeImageSnapshot();
      if (image) {
        iconCache[style.GetIconName()] = image;

        if (parameter.GetIconMode()==MapParameter::IconMode::OriginalPixmap) {
          style.SetWidth(image->width());
          style.SetHeight(image->height());
        }
        return true;
      }
#else
      // nanosvg fallback path
      NSVGimage* svgImage = nsvgParseFromFile(filename.c_str(), "px", 96.0f);
      if (!svgImage) {
        continue;
      }

      float scale = 1.0f;
      int w = static_cast<int>(svgImage->width);
      int h = static_cast<int>(svgImage->height);

      if (parameter.GetIconMode()==MapParameter::IconMode::Scalable ||
          parameter.GetIconMode()==MapParameter::IconMode::ScaledPixmap) {
        float targetSize = style.GetWidth();
        scale = targetSize / std::max(static_cast<float>(w), static_cast<float>(h));
        w = static_cast<int>(w * scale);
        h = static_cast<int>(h * scale);
      }

      unsigned char* rgba = static_cast<unsigned char*>(malloc(static_cast<size_t>(w) * h * 4));
      if (!rgba) {
        nsvgDelete(svgImage);
        continue;
      }

      NSVGrasterizer* rast = nsvgCreateRasterizer();
      if (!rast) {
        free(rgba);
        nsvgDelete(svgImage);
        continue;
      }

      nsvgRasterize(rast, svgImage, 0, 0, scale, rgba, w, h, w * 4);
      nsvgDeleteRasterizer(rast);
      nsvgDelete(svgImage);

      sk_sp<SkData> pixelData = SkData::MakeFromMalloc(rgba,
          static_cast<size_t>(w) * h * 4);
      SkImageInfo info = SkImageInfo::Make(w, h,
          kRGBA_8888_SkColorType, kPremul_SkAlphaType);
      sk_sp<SkImage> image = SkImages::RasterFromData(info, pixelData, w * 4);

      if (image) {
        iconCache[style.GetIconName()] = image;

        if (parameter.GetIconMode()==MapParameter::IconMode::OriginalPixmap) {
          style.SetWidth(image->width());
          style.SetHeight(image->height());
        }
        return true;
      }
#endif
    }

    // Mark as not found
    iconCache[style.GetIconName()] = nullptr;
    return false;
  }

  double MapPainterSkia::GetFontHeight(const Projection& projection,
                                        const MapParameter& parameter,
                                        double fontSize)
  {
    double size = fontSize * projection.ConvertWidthToPixel(parameter.GetFontSize());
    sk_sp<SkTypeface> typeface = GetTypeface(parameter.GetFontName());
    SkFont font(typeface, static_cast<SkScalar>(size));
    font.setSubpixel(true);

    SkFontMetrics metrics;
    font.getMetrics(&metrics);
    double height = metrics.fDescent - metrics.fAscent;
    log.Debug() << "MapPainterSkia::GetFontHeight font='" << parameter.GetFontName()
                << "' size=" << size << " height=" << height;
    return height;
  }

  void MapPainterSkia::DrawGround(const Projection& /*projection*/,
                                   const MapParameter& /*parameter*/,
                                   const FillStyle& style)
  {
    SkColor color = SkColorSetARGB(
        static_cast<uint8_t>(style.GetFillColor().GetA() * 255),
        static_cast<uint8_t>(style.GetFillColor().GetR() * 255),
        static_cast<uint8_t>(style.GetFillColor().GetG() * 255),
        static_cast<uint8_t>(style.GetFillColor().GetB() * 255));

    draw->drawColor(color);
  }

  void MapPainterSkia::DrawIcon(const IconStyle* style,
                                 const Vertex2D& centerPos,
                                 double width, double height)
  {
    auto it = iconCache.find(style->GetIconName());

    if (it == iconCache.end() || !it->second) {
      return;
    }

    sk_sp<SkImage> icon = it->second;
    int w = icon->width();
    int h = icon->height();

    SkRect destRect = SkRect::MakeXYWH(
        static_cast<SkScalar>(centerPos.GetX() - width / 2.0),
        static_cast<SkScalar>(centerPos.GetY() - height / 2.0),
        static_cast<SkScalar>(width),
        static_cast<SkScalar>(height));

    SkRect srcRect = SkRect::MakeWH(static_cast<SkScalar>(w),
                                    static_cast<SkScalar>(h));

    draw->drawImageRect(icon,
                        srcRect,
                        destRect,
                        SkSamplingOptions(),
                        nullptr,
                        SkCanvas::kFast_SrcRectConstraint);
  }

  void MapPainterSkia::DrawSymbol(const Projection& projection,
                                   const MapParameter& /*parameter*/,
                                   const Symbol& symbol,
                                   const Vertex2D& screenPos,
                                   double scaleFactor)
  {
    SymbolRendererSkia renderer(draw);

    renderer.Render(projection,
                    symbol,
                    screenPos,
                    scaleFactor);
  }

  void MapPainterSkia::DrawPath(const Projection& /*projection*/,
                                 const MapParameter& /*parameter*/,
                                 const Color& color,
                                 double width,
                                 const std::vector<double>& dash,
                                 LineStyle::CapStyle startCap,
                                 LineStyle::CapStyle endCap,
                                 const CoordBufferRange& coordRange)
  {
    size_t count = coordRange.GetEnd() - coordRange.GetStart() + 1;
    if (count < 2) {
      return;
    }

    std::vector<SkPoint> points;
    points.reserve(count);
    for (size_t i = coordRange.GetStart(); i <= coordRange.GetEnd(); ++i) {
      points.emplace_back(SkPoint::Make(
          static_cast<SkScalar>(coordBuffer.buffer[i].GetX()),
          static_cast<SkScalar>(coordBuffer.buffer[i].GetY())));
    }

    SkPath path = SkPath::Polygon(points, false);

    // Determine the more restrictive cap for the main stroke
    // Restrictiveness order: Butt > Square > Round
    SkPaint::Cap effectiveCap = SkPaint::kRound_Cap;
    if (startCap == LineStyle::capButt || endCap == LineStyle::capButt) {
      effectiveCap = SkPaint::kButt_Cap;
    } else if (startCap == LineStyle::capSquare || endCap == LineStyle::capSquare) {
      effectiveCap = SkPaint::kSquare_Cap;
    }

    // Draw main path with the more restrictive cap
    SkPaint paint;
    SetLineAttributes(paint, color, width, dash, startCap, endCap);
    paint.setStrokeCap(effectiveCap);
    draw->drawPath(path, paint);

    // Draw round caps at ends that need them
    SkColor skColor = SkColorSetARGB(
        static_cast<uint8_t>(color.GetA() * 255),
        static_cast<uint8_t>(color.GetR() * 255),
        static_cast<uint8_t>(color.GetG() * 255),
        static_cast<uint8_t>(color.GetB() * 255));

    if (startCap == LineStyle::capRound && effectiveCap != SkPaint::kRound_Cap) {
      SkPaint capPaint;
      capPaint.setAntiAlias(true);
      capPaint.setStyle(SkPaint::kFill_Style);
      capPaint.setColor(skColor);
      draw->drawCircle(static_cast<SkScalar>(coordBuffer.buffer[coordRange.GetStart()].GetX()),
                       static_cast<SkScalar>(coordBuffer.buffer[coordRange.GetStart()].GetY()),
                       static_cast<SkScalar>(width / 2.0),
                       capPaint);
    }

    if (endCap == LineStyle::capRound && effectiveCap != SkPaint::kRound_Cap) {
      SkPaint capPaint;
      capPaint.setAntiAlias(true);
      capPaint.setStyle(SkPaint::kFill_Style);
      capPaint.setColor(skColor);
      draw->drawCircle(static_cast<SkScalar>(coordBuffer.buffer[coordRange.GetEnd()].GetX()),
                       static_cast<SkScalar>(coordBuffer.buffer[coordRange.GetEnd()].GetY()),
                       static_cast<SkScalar>(width / 2.0),
                       capPaint);
    }
  }

  void MapPainterSkia::DrawLabel(const Projection& /*projection*/,
                                  const MapParameter& parameter,
                                  const ScreenVectorRectangle& labelRectangle,
                                  const LabelData& label,
                                  const SkiaNativeLabel& layout)
  {
    // Split multi-line text on newlines
    std::vector<std::string> lines;
    std::string current;
    for (char c : layout.text) {
      if (c == '\n') {
        lines.push_back(current);
        current.clear();
      } else {
        current += c;
      }
    }
    lines.push_back(current);

    if (const auto* style = dynamic_cast<const TextStyle*>(label.style.get());
        style != nullptr) {

      double r = style->GetTextColor().GetR();
      double g = style->GetTextColor().GetG();
      double b = style->GetTextColor().GetB();

      sk_sp<SkTypeface> typeface = layout.typeface
                                      ? layout.typeface
                                      : GetTypeface(parameter.GetFontName());
      SkFont font(typeface, static_cast<SkScalar>(layout.fontSize));
      font.setSubpixel(true);

      SkFontMetrics metrics;
      font.getMetrics(&metrics);
      double lineHeight = metrics.fDescent - metrics.fAscent;

      if (style->GetStyle() == TextStyle::normal) {
        SkPaint textPaint;
        textPaint.setAntiAlias(true);
        textPaint.setColor(SkColorSetARGB(
            static_cast<uint8_t>(label.alpha * 255),
            static_cast<uint8_t>(r * 255),
            static_cast<uint8_t>(g * 255),
            static_cast<uint8_t>(b * 255)));

        float y = static_cast<SkScalar>(labelRectangle.y + layout.fontSize);
        for (const auto& line : lines) {
          if (!line.empty()) {
            double lineWidth = font.measureText(line.c_str(), line.length(),
                                                SkTextEncoding::kUTF8);
            float x = static_cast<SkScalar>(labelRectangle.x
                      + (labelRectangle.width - lineWidth) / 2.0);
            draw->drawString(line.c_str(),
                             x,
                             y,
                             font,
                             textPaint);
          }
          y += static_cast<SkScalar>(lineHeight);
        }
      }
      else /* emphasize */ {
        double er = style->GetEmphasizeColor().GetR();
        double eg = style->GetEmphasizeColor().GetG();
        double eb = style->GetEmphasizeColor().GetB();

        SkPaint outlinePaint;
        outlinePaint.setAntiAlias(true);
        outlinePaint.setColor(SkColorSetARGB(
            static_cast<uint8_t>(label.alpha * 255),
            static_cast<uint8_t>(er * 255),
            static_cast<uint8_t>(eg * 255),
            static_cast<uint8_t>(eb * 255)));

        SkPaint textPaint;
        textPaint.setAntiAlias(true);
        textPaint.setColor(SkColorSetARGB(
            static_cast<uint8_t>(label.alpha * 255),
            static_cast<uint8_t>(r * 255),
            static_cast<uint8_t>(g * 255),
            static_cast<uint8_t>(b * 255)));

        float y = static_cast<SkScalar>(labelRectangle.y + layout.fontSize);
        for (const auto& line : lines) {
          if (!line.empty()) {
            double lineWidth = font.measureText(line.c_str(), line.length(),
                                                SkTextEncoding::kUTF8);
            float x = static_cast<SkScalar>(labelRectangle.x
                      + (labelRectangle.width - lineWidth) / 2.0);
            // Draw outline by offsetting text 1px in each direction
            draw->drawString(line.c_str(), x - 1, y, font, outlinePaint);
            draw->drawString(line.c_str(), x + 1, y, font, outlinePaint);
            draw->drawString(line.c_str(), x, y - 1, font, outlinePaint);
            draw->drawString(line.c_str(), x, y + 1, font, outlinePaint);

            // Draw text on top
            draw->drawString(line.c_str(), x, y, font, textPaint);
          }
          y += static_cast<SkScalar>(lineHeight);
        }
      }
    }
    else if (const auto* style = dynamic_cast<const ShieldStyle*>(label.style.get());
             style != nullptr) {

      sk_sp<SkTypeface> typeface = layout.typeface
                                      ? layout.typeface
                                      : GetTypeface(""); // default typeface
      SkFont font(typeface, static_cast<SkScalar>(layout.fontSize));
      font.setSubpixel(true);

      // Shield background
      SkPaint bgPaint;
      bgPaint.setAntiAlias(true);
      bgPaint.setStyle(SkPaint::kFill_Style);
      bgPaint.setColor(SkColorSetARGB(
          static_cast<uint8_t>(style->GetBgColor().GetA() * 255),
          static_cast<uint8_t>(style->GetBgColor().GetR() * 255),
          static_cast<uint8_t>(style->GetBgColor().GetG() * 255),
          static_cast<uint8_t>(style->GetBgColor().GetB() * 255)));
      draw->drawRect(SkRect::MakeXYWH(
          static_cast<SkScalar>(labelRectangle.x - 2),
          static_cast<SkScalar>(labelRectangle.y),
          static_cast<SkScalar>(labelRectangle.width + 3),
          static_cast<SkScalar>(labelRectangle.height + 1)),
          bgPaint);

      // Shield border
      SkPaint borderPaint;
      borderPaint.setAntiAlias(true);
      borderPaint.setStyle(SkPaint::kStroke_Style);
      borderPaint.setColor(SkColorSetARGB(
          static_cast<uint8_t>(style->GetBorderColor().GetA() * 255),
          static_cast<uint8_t>(style->GetBorderColor().GetR() * 255),
          static_cast<uint8_t>(style->GetBorderColor().GetG() * 255),
          static_cast<uint8_t>(style->GetBorderColor().GetB() * 255)));
      borderPaint.setStrokeWidth(1.0f);
      draw->drawRect(SkRect::MakeXYWH(
          static_cast<SkScalar>(labelRectangle.x),
          static_cast<SkScalar>(labelRectangle.y + 2),
          static_cast<SkScalar>(labelRectangle.width + 3 - 4),
          static_cast<SkScalar>(labelRectangle.height + 1 - 4)),
          borderPaint);

      // Shield text
      SkPaint textPaint;
      textPaint.setAntiAlias(true);
      textPaint.setColor(SkColorSetARGB(
          static_cast<uint8_t>(style->GetTextColor().GetA() * 255),
          static_cast<uint8_t>(style->GetTextColor().GetR() * 255),
          static_cast<uint8_t>(style->GetTextColor().GetG() * 255),
          static_cast<uint8_t>(style->GetTextColor().GetB() * 255)));
      draw->drawString(layout.text.c_str(),
                       static_cast<SkScalar>(labelRectangle.x),
                       static_cast<SkScalar>(labelRectangle.y + layout.fontSize),
                       font,
                       textPaint);
    }
  }

  void MapPainterSkia::DrawGlyphs(const Projection &projection,
                                   const MapParameter &parameter,
                                   const osmscout::PathTextStyleRef style,
                                   const std::vector<SkiaGlyph> &glyphs)
  {

    const Color& color = style->GetTextColor();
    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(SkColorSetARGB(
        static_cast<uint8_t>(color.GetA() * 255),
        static_cast<uint8_t>(color.GetR() * 255),
        static_cast<uint8_t>(color.GetG() * 255),
        static_cast<uint8_t>(color.GetB() * 255)));

    // Use font from first glyph if available, otherwise fall back to parameter font
    sk_sp<SkTypeface> typeface;
    double fontSize = 0;
    if (!glyphs.empty() && glyphs.front().glyph.fontSize > 0) {
      typeface = GetTypeface(parameter.GetFontName());
      fontSize = glyphs.front().glyph.fontSize;
    } else {
      typeface = GetTypeface(parameter.GetFontName());
      fontSize = style->GetSize() * projection.ConvertWidthToPixel(parameter.GetFontSize());
    }
      SkFont font(typeface, static_cast<SkScalar>(fontSize));
      font.setSubpixel(true);

      for (const auto& glyph : glyphs) {

      draw->save();
      draw->translate(static_cast<SkScalar>(glyph.position.GetX()),
                      static_cast<SkScalar>(glyph.position.GetY()));
      draw->rotate(static_cast<SkScalar>(glyph.angle * 180.0 / M_PI));
      draw->drawString(glyph.glyph.character.c_str(), 0, 0, font, textPaint);
      draw->restore();
    }
  }

  osmscout::ScreenVectorRectangle MapPainterSkia::GlyphBoundingBox(const SkiaNativeGlyph &glyph) const
  {
    return ScreenVectorRectangle(0,
                                 glyph.yMin,
                                 glyph.width,
                                 glyph.yMax - glyph.yMin);
  }

  std::shared_ptr<MapPainterSkia::SkiaLabel> MapPainterSkia::Layout(
      const Projection& projection,
      const MapParameter& parameter,
      const std::string& text,
      double fontSize,
      double objectWidth,
      bool enableWrapping,
      bool /*contourLabel*/)
  {
    auto label = std::make_shared<SkiaLabel>();

    double size = fontSize * projection.ConvertWidthToPixel(parameter.GetFontSize());
    sk_sp<SkTypeface> typeface = GetTypeface(parameter.GetFontName());
    SkFont font(typeface, static_cast<SkScalar>(size));
    font.setSubpixel(true);

    SkFontMetrics metrics;
    font.getMetrics(&metrics);
    double fontHeight = metrics.fDescent - metrics.fAscent;

    double proposedWidth = -1;
    if (enableWrapping) {
      // Calculate average character width for GetProposedLabelWidth
      double averageCharWidth = font.measureText("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789", 62,
                                                  SkTextEncoding::kUTF8) / 62.0;
      proposedWidth = GetProposedLabelWidth(parameter,
                                            averageCharWidth,
                                            objectWidth,
                                            text.length());
    }

    if (enableWrapping && proposedWidth > 0) {
      // Word wrapping: split on spaces, measure each line
      std::vector<std::string> words;
      std::string current;
      for (char c : text) {
        if (c == ' ') {
          if (!current.empty()) {
            words.push_back(current);
            current.clear();
          }
        } else {
          current += c;
        }
      }
      if (!current.empty()) {
        words.push_back(current);
      }

      std::string wrapped;
      std::string line;
      double lineWidth = 0;
      double totalWidth = 0;
      double totalHeight = fontHeight;

      for (const auto& word : words) {
        std::string testLine = line.empty() ? word : line + " " + word;
        double testWidth = font.measureText(testLine.c_str(), testLine.length(),
                                            SkTextEncoding::kUTF8);

        if (!line.empty() && testWidth > proposedWidth) {
          // Flush current line
          wrapped += line + "\n";
          totalWidth = std::max(totalWidth, lineWidth);
          totalHeight += fontHeight;
          line = word;
          lineWidth = font.measureText(word.c_str(), word.length(), SkTextEncoding::kUTF8);
        } else {
          line = testLine;
          lineWidth = testWidth;
        }
      }
      if (!line.empty()) {
        wrapped += line;
        totalWidth = std::max(totalWidth, lineWidth);
      }

      label->text = wrapped;
      label->width = totalWidth;
      label->height = totalHeight;
    } else {
      double textWidth = font.measureText(text.c_str(), text.length(), SkTextEncoding::kUTF8);
      label->text = text;
      label->width = textWidth;
      label->height = fontHeight;
    }

    label->fontSize = size;
    label->label.text = label->text;
    label->label.fontSize = size;
    label->label.typeface = typeface;

    return label;
  }

  // Inline UTF-8 decoder (SkUTF::NextUTF8 moved out of public API in Skia M143)
  static SkUnichar NextUTF8(const char** ptr, const char* end)
  {
    if (*ptr >= end) return -1;
    uint8_t lead = static_cast<uint8_t>(**ptr);
    if (lead < 0x80) {
      *ptr += 1;
      return lead;
    }
    int len;
    SkUnichar cp;
    if (lead < 0xC0) return -1;
    else if (lead < 0xE0) { len = 2; cp = lead & 0x1F; }
    else if (lead < 0xF0) { len = 3; cp = lead & 0x0F; }
    else if (lead < 0xF8) { len = 4; cp = lead & 0x07; }
    else return -1;
    if (*ptr + len > end) return -1;
    for (int i = 1; i < len; ++i) {
      uint8_t b = static_cast<uint8_t>((*ptr)[i]);
      if ((b & 0xC0) != 0x80) return -1;
      cp = (cp << 6) | (b & 0x3F);
    }
    *ptr += len;
    return cp;
  }

  template<>
  std::vector<Glyph<MapPainterSkia::SkiaNativeGlyph>> MapPainterSkia::SkiaLabel::ToGlyphs() const
  {
    std::vector<Glyph<MapPainterSkia::SkiaNativeGlyph>> result;

    sk_sp<SkTypeface> typeface = label.typeface;
    if (!typeface) {
      typeface = SkTypeface::MakeEmpty();
    }
    SkFont font(typeface, static_cast<SkScalar>(fontSize));
    font.setSubpixel(true);
    SkFontMetrics metrics;
    font.getMetrics(&metrics);
    double fontHeight = metrics.fDescent - metrics.fAscent;

    double horizontalOffset = 0;
    for (size_t ch = 0; ch < text.length();) {
      // Get next UTF-8 character
      size_t charLen = 1;
      while ((ch + charLen) < text.length() && (text[ch + charLen] & 0xC0) == 0x80) {
        charLen++;
      }
      std::string character = text.substr(ch, charLen);

      result.emplace_back();
      result.back().glyph.character = character;
      result.back().glyph.width = font.measureText(character.c_str(), character.length(),
                                                    SkTextEncoding::kUTF8);
      result.back().glyph.height = fontHeight;

      // Get actual glyph bounds for tighter bounding box
      const char* charPtr = character.c_str();
      const char* charEnd = charPtr + character.length();
      SkUnichar uni = NextUTF8(&charPtr, charEnd);
      if (uni >= 0) {
        SkGlyphID glyphID = font.unicharToGlyph(uni);
        SkRect glyphBounds;
        font.getBounds(SkSpan<const SkGlyphID>(&glyphID, 1),
                       SkSpan<SkRect>(&glyphBounds, 1),
                       nullptr);
        result.back().glyph.yMin = glyphBounds.fTop;
        result.back().glyph.yMax = glyphBounds.fBottom;
      } else {
        result.back().glyph.yMin = metrics.fAscent;
        result.back().glyph.yMax = metrics.fAscent + fontHeight;
      }
      result.back().glyph.fontSize = fontSize;
      result.back().position = Vertex2D(horizontalOffset, 0);

      horizontalOffset += result.back().glyph.width;
      ch += charLen;
    }

    return result;
  }

  void MapPainterSkia::RegisterRegularLabel(const Projection &projection,
                                             const MapParameter &parameter,
                                             bool basemap,
                                             const ObjectFileRef& ref,
                                             const std::vector<LabelData> &labels,
                                             const Vertex2D &position,
                                             double objectWidth)
  {
    labelLayouter.RegisterLabel(projection,
                                parameter,
                                basemap,
                                ref,
                                position,
                                labels,
                                objectWidth);
  }

  void MapPainterSkia::RegisterContourLabel(const Projection &projection,
                                              const MapParameter &parameter,
                                              bool basemap,
                                              const ObjectFileRef& ref,
                                              const PathLabelData &label,
                                              const LabelPath &labelPath)
  {
    labelLayouter.RegisterContourLabel(projection,
                                       parameter,
                                       basemap,
                                       ref,
                                       label,
                                       labelPath);
  }

  void MapPainterSkia::DrawLabels(const Projection& projection,
                                   const MapParameter& parameter,
                                   const std::vector<MapData>& /*data*/)
  {
    log.Debug() << "MapPainterSkia::DrawLabels";
    labelLayouter.Layout(projection, parameter);

    labelLayouter.DrawLabels(projection,
                             parameter,
                             this);

    labelLayouter.Reset();
  }

  void MapPainterSkia::BeforeDrawingCallback(const Projection& projection,
                                              const MapParameter& parameter,
                                              const std::vector<MapData>& /*data*/)
  {
    ScreenVectorRectangle viewport;
    viewport = ScreenVectorRectangle(0, 0,
                                     projection.GetWidth(),
                                     projection.GetHeight());

    log.Debug() << "MapPainterSkia::BeforeDrawingCallback viewport="
                << viewport.width << "x" << viewport.height;

    labelLayouter.SetViewport(viewport);
    labelLayouter.SetLayoutOverlap(projection.ConvertWidthToPixel(parameter.GetLabelLayouterOverlap()));
  }

  void MapPainterSkia::FollowPathInit(FollowPathHandle &hnd,
                                      const CoordBufferRange& coordRange,
                                      Vertex2D &origin,
                                      bool isClosed,
                                      bool keepOrientation)
  {
    hnd.i=0;
    hnd.nVertex=coordRange.GetEnd() >= coordRange.GetStart() ? coordRange.GetEnd() - coordRange.GetStart() : coordRange.GetStart()-coordRange.GetEnd();
    bool isReallyClosed=(coordRange.GetFirst()==coordRange.GetLast());

    if (isClosed && !isReallyClosed) {
      hnd.nVertex++;
      hnd.closeWay=true;
    }
    else {
      hnd.closeWay=false;
    }

    if (keepOrientation ||
        coordRange.GetFirst().GetX()<coordRange.GetLast().GetX()) {
      hnd.transStart=coordRange.GetStart();
      hnd.transEnd=coordRange.GetEnd();
    }
    else {
      hnd.transStart=coordRange.GetEnd();
      hnd.transEnd=coordRange.GetStart();
    }

    hnd.direction=(hnd.transStart < hnd.transEnd) ? 1 : -1;
    origin=coordRange.Get(hnd.transStart);
  }

  bool MapPainterSkia::FollowPath(FollowPathHandle &hnd,
                                  const CoordBufferRange& coordRange,
                                  double l,
                                  Vertex2D &origin)
  {
    double x=origin.GetX();
    double y=origin.GetY();
    double x2;
    double y2;

    while (hnd.i<hnd.nVertex) {
      if (hnd.closeWay && hnd.nVertex-hnd.i==1) {
        x2=coordRange.Get(hnd.transStart).GetX();
        y2=coordRange.Get(hnd.transStart).GetY();
      }
      else {
        x2=coordRange.Get(hnd.transStart+(hnd.i+1)*hnd.direction).GetX();
        y2=coordRange.Get(hnd.transStart+(hnd.i+1)*hnd.direction).GetY();
      }

      double deltaX=(x2-x);
      double deltaY=(y2-y);
      double len=sqrt(deltaX*deltaX + deltaY*deltaY);
      double fracToGo=l/len;

      if (fracToGo<=1.0) {
        origin=Vertex2D(x + deltaX*fracToGo,
                        y + deltaY*fracToGo);
        return true;
      }

      //advance to next point on the path
      l-=len;
      x=x2;
      y=y2;
      hnd.i++;
    }

    return false;
  }

  void MapPainterSkia::DrawContourSymbol(const Projection& projection,
                                          const MapParameter& parameter,
                                          const Symbol& symbol,
                                          const ContourSymbolData& data)
  {
    double symbolWidth=symbol.GetWidth(projection);
    double space=data.symbolSpace;
    double offset=data.symbolOffset;
    assert(space>0);
    assert(offset>0);

    bool             isClosed=false;
    Vertex2D         origin;
    double           x1;
    double           y1;
    double           x2;
    double           y2;
    double           x3;
    double           y3;
    double           slope;
    FollowPathHandle followPathHnd;

    FollowPathInit(followPathHnd,
                   data.coordRange,
                   origin,
                   isClosed,
                   true);

    if (!isClosed &&
        !FollowPath(followPathHnd,
                    data.coordRange,
                    offset,
                    origin)) {
      return;
    }

    SkMatrix savedMatrix=draw->getTotalMatrix();
    bool     loop=true;

    while (loop) {
      x1=origin.GetX();
      y1=origin.GetY();
      loop=FollowPath(followPathHnd,
                      data.coordRange,
                      symbolWidth/2,
                      origin);

      if (loop) {
        x2=origin.GetX();
        y2=origin.GetY();
        loop=FollowPath(followPathHnd,
                        data.coordRange,
                        symbolWidth/2,
                        origin);

        if (loop) {
          x3=origin.GetX();
          y3=origin.GetY();
          slope=atan2(y3-y1,x3-x1);

          draw->save();
          draw->translate(static_cast<SkScalar>(x2),
                          static_cast<SkScalar>(y2));
          draw->rotate(static_cast<SkScalar>(slope * 180.0 / M_PI));

          DrawSymbol(projection,
                     parameter,
                     symbol,
                     Vertex2D::ZERO,
                     data.symbolScale);

          draw->restore();

          loop=FollowPath(followPathHnd,
                          data.coordRange,
                          space,
                          origin);
        }
      }
    }

    draw->setMatrix(savedMatrix);
  }

  void MapPainterSkia::DrawArea(const Projection& projection,
                                 const MapParameter& parameter,
                                 const AreaData& area)
  {
    size_t count = area.coordRange.GetEnd() - area.coordRange.GetStart() + 1;
    if (count < 3) {
      return;
    }

    // Build the main polygon path using SkPathBuilder for clippings support
    SkPathBuilder builder;

    // Main polygon ring
    {
      std::vector<SkPoint> points;
      points.reserve(count);
      for (size_t i = area.coordRange.GetStart(); i <= area.coordRange.GetEnd(); ++i) {
        points.emplace_back(SkPoint::Make(
            static_cast<SkScalar>(area.coordRange.Get(i).GetX()),
            static_cast<SkScalar>(area.coordRange.Get(i).GetY())));
      }
      builder.addPolygon(points, true);
    }

    // Add clipping sub-paths (interior holes) using even-odd fill rule
    if (!area.clippings.empty()) {
      for (const auto& clip : area.clippings) {
        size_t clipCount = clip.GetEnd() - clip.GetStart() + 1;
        if (clipCount < 3) {
          continue;
        }
        builder.moveTo(static_cast<SkScalar>(clip.GetFirst().GetX()),
                       static_cast<SkScalar>(clip.GetFirst().GetY()));
        for (size_t i = clip.GetStart() + 1; i <= clip.GetEnd(); ++i) {
          builder.lineTo(static_cast<SkScalar>(clip.Get(i).GetX()),
                         static_cast<SkScalar>(clip.Get(i).GetY()));
        }
        builder.close();
      }
      builder.setFillType(SkPathFillType::kEvenOdd);
    }

    SkPath path = builder.detach();

    // Delegate fill and border rendering to DrawFillStyle
    DrawFillStyle(projection,
                  parameter,
                  area.fillStyle,
                  area.borderStyle,
                  path);
  }

  void MapPainterSkia::StyleSheetChanged([[maybe_unused]] const Projection& projection,
                                          [[maybe_unused]] const MapParameter& parameter,
                                          [[maybe_unused]] const std::vector<MapData>& data)
  {
    iconCache.clear();
    patternCache.clear();
  }

  bool MapPainterSkia::DrawMap(const Projection& projection,
                                const MapParameter& parameter,
                                const std::vector<MapData>& data,
                                SkCanvas* canvas,
                                RenderSteps startStep,
                                RenderSteps endStep)
  {
    std::lock_guard<std::mutex> guard(mutex);

    this->draw = canvas;

    return Draw(projection,
                  parameter,
                  data,
                  startStep,
                  endStep);
  }
}
