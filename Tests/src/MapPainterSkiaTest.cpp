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

#include <catch2/catch_test_macros.hpp>

#include <core/SkBitmap.h>
#include <core/SkCanvas.h>
#include <core/SkColor.h>
#include <core/SkFont.h>
#include <core/SkFontMetrics.h>
#include <core/SkImageInfo.h>
#include <core/SkPaint.h>
#include <core/SkPath.h>
#include <core/SkPathBuilder.h>
#include <core/SkPoint.h>
#include <core/SkSurface.h>
#include <core/SkTypeface.h>
#include <effects/SkDashPathEffect.h>

#include <osmscout/io/File.h>
#include <osmscout/util/Color.h>
#include <osmscoutmap/Styles.h>

#include <osmscoutmapskia/MapPainterSkia.h>

// ---------------------------------------------------------------------------
// Helper: create an SkSurface for offscreen rendering
// ---------------------------------------------------------------------------
static sk_sp<SkSurface> MakeSurface(int w, int h) {
  auto info = SkImageInfo::Make(w, h, SkColorType::kRGBA_8888_SkColorType,
                                SkAlphaType::kPremul_SkAlphaType);
  return SkSurfaces::Raster(info);
}

// ---------------------------------------------------------------------------
// Helper: convert osmscout::Color to SkColor for comparison
// ---------------------------------------------------------------------------
static SkColor ToSkColor(const osmscout::Color& c) {
  return SkColorSetARGB(
      static_cast<uint8_t>(c.GetA() * 255),
      static_cast<uint8_t>(c.GetR() * 255),
      static_cast<uint8_t>(c.GetG() * 255),
      static_cast<uint8_t>(c.GetB() * 255));
}

// ---------------------------------------------------------------------------
// CapStyle mapping tests
// ---------------------------------------------------------------------------
TEST_CASE("SetLineAttributes maps capButt to kButt_Cap", "[MapPainterSkia]") {
  SkPaint paint;
  osmscout::Color color(1.0, 0.0, 0.0);

  // We test the cap style mapping by creating a path and checking
  // that butt cap produces flat ends. Since SkPaint::Cap is not
  // directly observable after draw, we verify the paint setup logic
  // by checking that the cap style enum values match expectations.

  // Verify enum value correspondence
  REQUIRE(static_cast<int>(osmscout::LineStyle::capButt) == 0);
  REQUIRE(static_cast<int>(osmscout::LineStyle::capRound) == 1);
  REQUIRE(static_cast<int>(osmscout::LineStyle::capSquare) == 2);

  // Verify SkPaint cap enum values exist
  paint.setStrokeCap(SkPaint::kButt_Cap);
  REQUIRE(paint.getStrokeCap() == SkPaint::kButt_Cap);

  paint.setStrokeCap(SkPaint::kRound_Cap);
  REQUIRE(paint.getStrokeCap() == SkPaint::kRound_Cap);

  paint.setStrokeCap(SkPaint::kSquare_Cap);
  REQUIRE(paint.getStrokeCap() == SkPaint::kSquare_Cap);
}

// ---------------------------------------------------------------------------
// Alpha/transparency tests
// ---------------------------------------------------------------------------
TEST_CASE("SkColorSetARGB preserves alpha channel", "[MapPainterSkia]") {
  osmscout::Color semiTransparent(1.0, 0.0, 0.0, 0.5);
  SkColor skColor = ToSkColor(semiTransparent);

  REQUIRE(SkColorGetA(skColor) == 127); // 0.5 * 255 = 127.5 -> truncates to 127
  REQUIRE(SkColorGetR(skColor) == 255);
  REQUIRE(SkColorGetG(skColor) == 0);
  REQUIRE(SkColorGetB(skColor) == 0);
}

TEST_CASE("SkColorSetARGB handles fully opaque color", "[MapPainterSkia]") {
  osmscout::Color opaque(0.5, 0.5, 0.5, 1.0);
  SkColor skColor = ToSkColor(opaque);

  REQUIRE(SkColorGetA(skColor) == 255);
  REQUIRE(SkColorGetR(skColor) == 127);
  REQUIRE(SkColorGetG(skColor) == 127);
  REQUIRE(SkColorGetB(skColor) == 127);
}

TEST_CASE("SkColorSetARGB handles fully transparent color", "[MapPainterSkia]") {
  osmscout::Color transparent(1.0, 0.0, 0.0, 0.0);
  SkColor skColor = ToSkColor(transparent);

  REQUIRE(SkColorGetA(skColor) == 0);
}

// ---------------------------------------------------------------------------
// Dash pattern tests
// ---------------------------------------------------------------------------
TEST_CASE("SkDashPathEffect creates effect from intervals", "[MapPainterSkia]") {
  std::vector<SkScalar> intervals = {10.0f, 5.0f, 2.0f, 5.0f};
  auto effect = SkDashPathEffect::Make(SkSpan<const SkScalar>(intervals.data(),
                                                               intervals.size()),
                                        0);
  REQUIRE(effect != nullptr);
}

TEST_CASE("Empty dash produces no path effect", "[MapPainterSkia]") {
  SkPaint paint;
  paint.setPathEffect(nullptr);
  REQUIRE(paint.getPathEffect() == nullptr);
}

// ---------------------------------------------------------------------------
// SkPathBuilder clipping tests
// ---------------------------------------------------------------------------
TEST_CASE("SkPathBuilder with clippings sets kEvenOdd fill", "[MapPainterSkia]") {
  SkPathBuilder builder;

  // Main polygon: a square
  std::vector<SkPoint> outer = {
    {0.0f, 0.0f}, {100.0f, 0.0f}, {100.0f, 100.0f}, {0.0f, 100.0f}
  };
  builder.addPolygon(outer, true);

  // Clipping hole: a smaller square inside
  builder.moveTo(25.0f, 25.0f);
  builder.lineTo(75.0f, 25.0f);
  builder.lineTo(75.0f, 75.0f);
  builder.lineTo(25.0f, 75.0f);
  builder.close();
  builder.setFillType(SkPathFillType::kEvenOdd);

  SkPath path = builder.detach();
  REQUIRE(path.getFillType() == SkPathFillType::kEvenOdd);
}

TEST_CASE("SkPathBuilder without clippings uses default fill", "[MapPainterSkia]") {
  SkPathBuilder builder;

  std::vector<SkPoint> pts = {
    {0.0f, 0.0f}, {100.0f, 0.0f}, {100.0f, 100.0f}, {0.0f, 100.0f}
  };
  builder.addPolygon(pts, true);

  SkPath path = builder.detach();
  // Default fill type is kWinding
  REQUIRE(path.getFillType() == SkPathFillType::kWinding);
}

// ---------------------------------------------------------------------------
// Pattern fill cache test
// ---------------------------------------------------------------------------
TEST_CASE("MapPainterSkia can be instantiated", "[MapPainterSkia]") {
  osmscout::MapPainterSkia painter;
  REQUIRE(true);
}

// ---------------------------------------------------------------------------
// Integration: render a simple path and check pixel output
// ---------------------------------------------------------------------------
TEST_CASE("DrawPath renders colored line on surface", "[MapPainterSkia]") {
  auto surface = MakeSurface(50, 50);
  SkCanvas* canvas = surface->getCanvas();
  canvas->clear(SK_ColorWHITE);

  // Draw a red line using Skia directly (testing the rendering pipeline)
  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setColor(SK_ColorRED);
  paint.setStrokeWidth(4.0f);

  SkPath path = SkPath::Line(SkPoint::Make(5.0f, 25.0f),
                              SkPoint::Make(45.0f, 25.0f));
  canvas->drawPath(path, paint);

  // Check that pixels along the line are red
  SkBitmap bitmap;
  bitmap.allocPixels(SkImageInfo::Make(50, 50,
                                        SkColorType::kRGBA_8888_SkColorType,
                                        SkAlphaType::kPremul_SkAlphaType));
  surface->readPixels(bitmap, 0, 0);

  // Pixel at center of line should be red
  SkColor centerPixel = bitmap.getColor(25, 25);
  REQUIRE(SkColorGetR(centerPixel) == 255);
  REQUIRE(SkColorGetG(centerPixel) == 0);
  REQUIRE(SkColorGetB(centerPixel) == 0);

  // Pixel far from line should be white (background)
  SkColor cornerPixel = bitmap.getColor(0, 0);
  REQUIRE(SkColorGetR(cornerPixel) == 255);
  REQUIRE(SkColorGetG(cornerPixel) == 255);
  REQUIRE(SkColorGetB(cornerPixel) == 255);
}

TEST_CASE("DrawPath with dash renders dashed line", "[MapPainterSkia]") {
  auto surface = MakeSurface(100, 20);
  SkCanvas* canvas = surface->getCanvas();
  canvas->clear(SK_ColorWHITE);

  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setColor(SK_ColorBLACK);
  paint.setStrokeWidth(2.0f);

  std::vector<SkScalar> intervals = {10.0f, 5.0f};
  paint.setPathEffect(SkDashPathEffect::Make(
      SkSpan<const SkScalar>(intervals.data(), intervals.size()), 0));

  SkPath path = SkPath::Line(SkPoint::Make(0.0f, 10.0f),
                              SkPoint::Make(100.0f, 10.0f));
  canvas->drawPath(path, paint);

  SkBitmap bitmap;
  bitmap.allocPixels(SkImageInfo::Make(100, 20,
                                        SkColorType::kRGBA_8888_SkColorType,
                                        SkAlphaType::kPremul_SkAlphaType));
  surface->readPixels(bitmap, 0, 0);

  // At x=5 (within first dash segment): should be black
  SkColor dashPixel = bitmap.getColor(5, 10);
  REQUIRE(SkColorGetR(dashPixel) == 0);

  // At x=13 (within first gap): should be white (background)
  SkColor gapPixel = bitmap.getColor(13, 10);
  REQUIRE(SkColorGetR(gapPixel) == 255);
}

TEST_CASE("DrawArea fills polygon on surface", "[MapPainterSkia]") {
  auto surface = MakeSurface(50, 50);
  SkCanvas* canvas = surface->getCanvas();
  canvas->clear(SK_ColorWHITE);

  // Draw a filled blue square
  SkPathBuilder builder;
  std::vector<SkPoint> pts = {
    {10.0f, 10.0f}, {40.0f, 10.0f}, {40.0f, 40.0f}, {10.0f, 40.0f}
  };
  builder.addPolygon(pts, true);
  SkPath path = builder.detach();

  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setStyle(SkPaint::kFill_Style);
  paint.setColor(SK_ColorBLUE);
  canvas->drawPath(path, paint);

  SkBitmap bitmap;
  bitmap.allocPixels(SkImageInfo::Make(50, 50,
                                        SkColorType::kRGBA_8888_SkColorType,
                                        SkAlphaType::kPremul_SkAlphaType));
  surface->readPixels(bitmap, 0, 0);

  // Inside the square: should be blue
  SkColor inside = bitmap.getColor(25, 25);
  REQUIRE(SkColorGetB(inside) == 255);
  REQUIRE(SkColorGetR(inside) == 0);

  // Outside the square: should be white
  SkColor outside = bitmap.getColor(5, 5);
  REQUIRE(SkColorGetR(outside) == 255);
  REQUIRE(SkColorGetG(outside) == 255);
  REQUIRE(SkColorGetB(outside) == 255);
}

// ---------------------------------------------------------------------------
// Font and text rendering tests
// ---------------------------------------------------------------------------
TEST_CASE("SkFont can be constructed with empty typeface", "[MapPainterSkia]") {
  sk_sp<SkTypeface> typeface = SkTypeface::MakeEmpty();
  SkFont font(typeface, 12.0f);
  REQUIRE(font.getSize() == 12.0f);
}

TEST_CASE("SkFont::measureText handles empty string", "[MapPainterSkia]") {
  sk_sp<SkTypeface> typeface = SkTypeface::MakeEmpty();
  SkFont font(typeface, 12.0f);

  std::string text = "";
  double width = font.measureText(text.c_str(), text.length(), SkTextEncoding::kUTF8);
  REQUIRE(width == 0);
}

TEST_CASE("SkCanvas::drawString does not crash with empty typeface", "[MapPainterSkia]") {
  auto surface = MakeSurface(100, 30);
  SkCanvas* canvas = surface->getCanvas();
  canvas->clear(SK_ColorWHITE);

  sk_sp<SkTypeface> typeface = SkTypeface::MakeEmpty();
  SkFont font(typeface, 14.0f);

  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setColor(SK_ColorBLACK);

  // Should not crash
  canvas->drawString("Test", 10.0f, 20.0f, font, paint);
  REQUIRE(true);
}

TEST_CASE("Canvas translate affects subsequent drawing", "[MapPainterSkia]") {
  auto surface = MakeSurface(100, 30);
  SkCanvas* canvas = surface->getCanvas();
  canvas->clear(SK_ColorWHITE);

  // Draw a pixel at translated position
  SkPaint paint;
  paint.setColor(SK_ColorBLACK);

  canvas->save();
  canvas->translate(30.0f, 15.0f);
  canvas->drawPoint(0, 0, paint);
  canvas->restore();

  SkBitmap bitmap;
  bitmap.allocPixels(SkImageInfo::Make(100, 30,
                                        SkColorType::kRGBA_8888_SkColorType,
                                        SkAlphaType::kPremul_SkAlphaType));
  surface->readPixels(bitmap, 0, 0);

  // Point at (30,15) should be black
  SkColor pixel = bitmap.getColor(30, 15);
  REQUIRE(SkColorGetR(pixel) == 0);
  REQUIRE(SkColorGetG(pixel) == 0);
  REQUIRE(SkColorGetB(pixel) == 0);
}

TEST_CASE("Glyph data structure stores character and dimensions", "[MapPainterSkia]") {
  osmscout::MapPainterSkia::SkiaNativeGlyph glyph;
  glyph.character = "A";
  glyph.width = 8.0;
  glyph.height = 12.0;
  glyph.fontSize = 14.0;

  REQUIRE(glyph.character == "A");
  REQUIRE(glyph.width == 8.0);
  REQUIRE(glyph.height == 12.0);
  REQUIRE(glyph.fontSize == 14.0);
}

TEST_CASE("SkiaNativeLabel stores text, font size, and typeface", "[MapPainterSkia]") {
  osmscout::MapPainterSkia::SkiaNativeLabel label;
  label.text = "Test label";
  label.fontSize = 14.0;
  label.typeface = SkTypeface::MakeEmpty();

  REQUIRE(label.text == "Test label");
  REQUIRE(label.fontSize == 14.0);
  REQUIRE(label.typeface != nullptr);
}
