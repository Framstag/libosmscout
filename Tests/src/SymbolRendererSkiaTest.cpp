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
#include <core/SkImageInfo.h>
#include <core/SkPaint.h>
#include <core/SkPath.h>
#include <core/SkSurface.h>

#include <osmscout/util/Color.h>

#include <osmscoutmap/Styles.h>

#include <osmscoutmapskia/SymbolRendererSkia.h>

// Test helper: exposes protected SymbolRendererSkia methods for unit testing
class TestSymbolRenderer : public osmscout::SymbolRendererSkia {
public:
  using osmscout::SymbolRendererSkia::SymbolRendererSkia;
  using osmscout::SymbolRendererSkia::BeginPrimitive;
  using osmscout::SymbolRendererSkia::SetFill;
  using osmscout::SymbolRendererSkia::SetBorder;
  using osmscout::SymbolRendererSkia::DrawPolygon;
  using osmscout::SymbolRendererSkia::DrawRect;
  using osmscout::SymbolRendererSkia::DrawCircle;
  using osmscout::SymbolRendererSkia::EndPrimitive;
};

// ---------------------------------------------------------------------------
// Helper: create an SkSurface for offscreen rendering
// ---------------------------------------------------------------------------
static sk_sp<SkSurface> MakeSurface(int w, int h) {
  auto info = SkImageInfo::Make(w, h, SkColorType::kRGBA_8888_SkColorType,
                                SkAlphaType::kPremul_SkAlphaType);
  return SkSurfaces::Raster(info);
}

// ---------------------------------------------------------------------------
// Helper: create a fill style with the given color
// ---------------------------------------------------------------------------
static osmscout::FillStyleRef CreateFill(const osmscout::Color& color) {
  auto fill = std::make_shared<osmscout::FillStyle>();
  fill->SetFillColor(color);
  return fill;
}

// ---------------------------------------------------------------------------
// Helper: create a border style with the given color and width
// ---------------------------------------------------------------------------
static osmscout::BorderStyleRef CreateBorder(const osmscout::Color& color, double width) {
  auto border = std::make_shared<osmscout::BorderStyle>();
  border->SetColor(color);
  border->SetWidth(width);
  return border;
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------
TEST_CASE("SymbolRendererSkia can be constructed with SkCanvas pointer", "[SymbolRendererSkia]") {
  auto surface = MakeSurface(50, 50);
  TestSymbolRenderer renderer(surface->getCanvas());
  REQUIRE(true);
}

// ---------------------------------------------------------------------------
// Fill rendering
// ---------------------------------------------------------------------------
TEST_CASE("SymbolRendererSkia fills polygon with color", "[SymbolRendererSkia]") {
  auto surface = MakeSurface(50, 50);
  SkCanvas* canvas = surface->getCanvas();
  canvas->clear(SK_ColorWHITE);

  TestSymbolRenderer renderer(canvas);
  renderer.BeginPrimitive();
  renderer.SetFill(CreateFill(osmscout::Color::RED));
  renderer.SetBorder(osmscout::BorderStyleRef(), 1.0);

  std::vector<osmscout::Vertex2D> polygon;
  polygon.emplace_back(5.0, 5.0);
  polygon.emplace_back(45.0, 5.0);
  polygon.emplace_back(45.0, 45.0);
  polygon.emplace_back(5.0, 45.0);
  renderer.DrawPolygon(polygon);
  renderer.EndPrimitive();

  SkBitmap bitmap;
  bitmap.allocPixels(SkImageInfo::Make(50, 50,
                                        SkColorType::kRGBA_8888_SkColorType,
                                        SkAlphaType::kPremul_SkAlphaType));
  surface->readPixels(bitmap, 0, 0);

  // Center of polygon should be red
  SkColor center = bitmap.getColor(25, 25);
  REQUIRE(SkColorGetR(center) == 255);
  REQUIRE(SkColorGetG(center) == 0);
  REQUIRE(SkColorGetB(center) == 0);
}

TEST_CASE("SymbolRendererSkia does not fill with invisible color", "[SymbolRendererSkia]") {
  auto surface = MakeSurface(50, 50);
  SkCanvas* canvas = surface->getCanvas();
  canvas->clear(SK_ColorWHITE);

  TestSymbolRenderer renderer(canvas);
  renderer.BeginPrimitive();
  renderer.SetFill(CreateFill(osmscout::Color(1.0, 0.0, 0.0, 0.0))); // transparent
  renderer.SetBorder(osmscout::BorderStyleRef(), 1.0);

  std::vector<osmscout::Vertex2D> polygon;
  polygon.emplace_back(5.0, 5.0);
  polygon.emplace_back(45.0, 5.0);
  polygon.emplace_back(45.0, 45.0);
  polygon.emplace_back(5.0, 45.0);
  renderer.DrawPolygon(polygon);
  renderer.EndPrimitive();

  SkBitmap bitmap;
  bitmap.allocPixels(SkImageInfo::Make(50, 50,
                                        SkColorType::kRGBA_8888_SkColorType,
                                        SkAlphaType::kPremul_SkAlphaType));
  surface->readPixels(bitmap, 0, 0);

  // Center should still be white (no fill)
  SkColor center = bitmap.getColor(25, 25);
  REQUIRE(SkColorGetR(center) == 255);
  REQUIRE(SkColorGetG(center) == 255);
  REQUIRE(SkColorGetB(center) == 255);
}

// ---------------------------------------------------------------------------
// Border rendering
// ---------------------------------------------------------------------------
TEST_CASE("SymbolRendererSkia strokes border around polygon", "[SymbolRendererSkia]") {
  auto surface = MakeSurface(50, 50);
  SkCanvas* canvas = surface->getCanvas();
  canvas->clear(SK_ColorWHITE);

  TestSymbolRenderer renderer(canvas);
  renderer.BeginPrimitive();
  renderer.SetFill(osmscout::FillStyleRef());
  renderer.SetBorder(CreateBorder(osmscout::Color::BLUE, 2.0), 1.0);

  std::vector<osmscout::Vertex2D> polygon;
  polygon.emplace_back(5.0, 5.0);
  polygon.emplace_back(45.0, 5.0);
  polygon.emplace_back(45.0, 45.0);
  polygon.emplace_back(5.0, 45.0);
  renderer.DrawPolygon(polygon);
  renderer.EndPrimitive();

  SkBitmap bitmap;
  bitmap.allocPixels(SkImageInfo::Make(50, 50,
                                        SkColorType::kRGBA_8888_SkColorType,
                                        SkAlphaType::kPremul_SkAlphaType));
  surface->readPixels(bitmap, 0, 0);

  // Border pixels should be blue (on the edge of the polygon)
  SkColor edge = bitmap.getColor(5, 5);
  REQUIRE(SkColorGetB(edge) > 0);
}

TEST_CASE("SymbolRendererSkia does not stroke border with zero width", "[SymbolRendererSkia]") {
  auto surface = MakeSurface(50, 50);
  SkCanvas* canvas = surface->getCanvas();
  canvas->clear(SK_ColorWHITE);

  TestSymbolRenderer renderer(canvas);
  renderer.BeginPrimitive();
  renderer.SetFill(osmscout::FillStyleRef());
  renderer.SetBorder(CreateBorder(osmscout::Color::BLUE, 0.0), 1.0); // zero width

  std::vector<osmscout::Vertex2D> polygon;
  polygon.emplace_back(5.0, 5.0);
  polygon.emplace_back(45.0, 5.0);
  polygon.emplace_back(45.0, 45.0);
  polygon.emplace_back(5.0, 45.0);
  renderer.DrawPolygon(polygon);
  renderer.EndPrimitive();

  SkBitmap bitmap;
  bitmap.allocPixels(SkImageInfo::Make(50, 50,
                                        SkColorType::kRGBA_8888_SkColorType,
                                        SkAlphaType::kPremul_SkAlphaType));
  surface->readPixels(bitmap, 0, 0);

  // Edge should be white (no border stroked)
  SkColor edge = bitmap.getColor(5, 5);
  REQUIRE(SkColorGetR(edge) == 255);
  REQUIRE(SkColorGetG(edge) == 255);
  REQUIRE(SkColorGetB(edge) == 255);
}

// ---------------------------------------------------------------------------
// Fill + Border combined
// ---------------------------------------------------------------------------
TEST_CASE("SymbolRendererSkia fills then strokes border", "[SymbolRendererSkia]") {
  auto surface = MakeSurface(50, 50);
  SkCanvas* canvas = surface->getCanvas();
  canvas->clear(SK_ColorWHITE);

  TestSymbolRenderer renderer(canvas);
  renderer.BeginPrimitive();
  renderer.SetFill(CreateFill(osmscout::Color::RED));
  renderer.SetBorder(CreateBorder(osmscout::Color::BLUE, 3.0), 1.0);

  std::vector<osmscout::Vertex2D> polygon;
  polygon.emplace_back(5.0, 5.0);
  polygon.emplace_back(45.0, 5.0);
  polygon.emplace_back(45.0, 45.0);
  polygon.emplace_back(5.0, 45.0);
  renderer.DrawPolygon(polygon);
  renderer.EndPrimitive();

  SkBitmap bitmap;
  bitmap.allocPixels(SkImageInfo::Make(50, 50,
                                        SkColorType::kRGBA_8888_SkColorType,
                                        SkAlphaType::kPremul_SkAlphaType));
  surface->readPixels(bitmap, 0, 0);

  // Center should be red (fill)
  SkColor center = bitmap.getColor(25, 25);
  REQUIRE(SkColorGetR(center) == 255);

  // Edge should show blue border
  SkColor edge = bitmap.getColor(5, 5);
  REQUIRE(SkColorGetB(edge) > 0);
}

// ---------------------------------------------------------------------------
// Rectangle primitive
// ---------------------------------------------------------------------------
TEST_CASE("SymbolRendererSkia fills rectangle", "[SymbolRendererSkia]") {
  auto surface = MakeSurface(50, 50);
  SkCanvas* canvas = surface->getCanvas();
  canvas->clear(SK_ColorWHITE);

  TestSymbolRenderer renderer(canvas);
  renderer.BeginPrimitive();
  renderer.SetFill(CreateFill(osmscout::Color::GREEN));
  renderer.SetBorder(osmscout::BorderStyleRef(), 1.0);
  renderer.DrawRect(10.0, 10.0, 30.0, 30.0);
  renderer.EndPrimitive();

  SkBitmap bitmap;
  bitmap.allocPixels(SkImageInfo::Make(50, 50,
                                        SkColorType::kRGBA_8888_SkColorType,
                                        SkAlphaType::kPremul_SkAlphaType));
  surface->readPixels(bitmap, 0, 0);

  // Inside rectangle: green
  SkColor inside = bitmap.getColor(25, 25);
  REQUIRE(SkColorGetG(inside) == 255);

  // Outside rectangle: white
  SkColor outside = bitmap.getColor(5, 5);
  REQUIRE(SkColorGetR(outside) == 255);
  REQUIRE(SkColorGetG(outside) == 255);
  REQUIRE(SkColorGetB(outside) == 255);
}

// ---------------------------------------------------------------------------
// Circle primitive
// ---------------------------------------------------------------------------
TEST_CASE("SymbolRendererSkia fills circle", "[SymbolRendererSkia]") {
  auto surface = MakeSurface(50, 50);
  SkCanvas* canvas = surface->getCanvas();
  canvas->clear(SK_ColorWHITE);

  TestSymbolRenderer renderer(canvas);
  renderer.BeginPrimitive();
  renderer.SetFill(CreateFill(osmscout::Color::BLUE));
  renderer.SetBorder(osmscout::BorderStyleRef(), 1.0);
  renderer.DrawCircle(25.0, 25.0, 15.0);
  renderer.EndPrimitive();

  SkBitmap bitmap;
  bitmap.allocPixels(SkImageInfo::Make(50, 50,
                                        SkColorType::kRGBA_8888_SkColorType,
                                        SkAlphaType::kPremul_SkAlphaType));
  surface->readPixels(bitmap, 0, 0);

  // Center: blue
  SkColor center = bitmap.getColor(25, 25);
  REQUIRE(SkColorGetB(center) == 255);

  // Far outside: white
  SkColor far = bitmap.getColor(2, 2);
  REQUIRE(SkColorGetR(far) == 255);
  REQUIRE(SkColorGetG(far) == 255);
  REQUIRE(SkColorGetB(far) == 255);
}

// ---------------------------------------------------------------------------
// Border with dashes
// ---------------------------------------------------------------------------
TEST_CASE("SymbolRendererSkia handles border dashes", "[SymbolRendererSkia]") {
  auto surface = MakeSurface(60, 60);
  SkCanvas* canvas = surface->getCanvas();
  canvas->clear(SK_ColorWHITE);

  auto border = std::make_shared<osmscout::BorderStyle>();
  border->SetColor(osmscout::Color::RED);
  border->SetWidth(3.0);
  std::vector<double> dashes = {10.0, 5.0};
  border->SetDashes(dashes);

  TestSymbolRenderer renderer(canvas);
  renderer.BeginPrimitive();
  renderer.SetFill(osmscout::FillStyleRef());
  renderer.SetBorder(border, 1.0);
  renderer.DrawRect(5.0, 5.0, 50.0, 50.0);
  renderer.EndPrimitive();

  SkBitmap bitmap;
  bitmap.allocPixels(SkImageInfo::Make(60, 60,
                                        SkColorType::kRGBA_8888_SkColorType,
                                        SkAlphaType::kPremul_SkAlphaType));
  surface->readPixels(bitmap, 0, 0);

  // Border pixels on the rect edge should have red color
  SkColor edge = bitmap.getColor(30, 5);
  REQUIRE(SkColorGetR(edge) > 0);

  // Inside the rect (away from border) should still be white
  SkColor inside = bitmap.getColor(30, 30);
  REQUIRE(SkColorGetR(inside) == 255);
  REQUIRE(SkColorGetG(inside) == 255);
  REQUIRE(SkColorGetB(inside) == 255);
}

// ---------------------------------------------------------------------------
// Multiple primitives
// ---------------------------------------------------------------------------
TEST_CASE("BeginPrimitive/EndPrimitive cycle works for multiple primitives", "[SymbolRendererSkia]") {
  auto surface = MakeSurface(50, 50);
  SkCanvas* canvas = surface->getCanvas();
  canvas->clear(SK_ColorWHITE);

  TestSymbolRenderer renderer(canvas);

  // First primitive: red rect
  renderer.BeginPrimitive();
  renderer.SetFill(CreateFill(osmscout::Color::RED));
  renderer.SetBorder(osmscout::BorderStyleRef(), 1.0);
  renderer.DrawRect(0.0, 0.0, 25.0, 25.0);
  renderer.EndPrimitive();

  // Second primitive: blue rect
  renderer.BeginPrimitive();
  renderer.SetFill(CreateFill(osmscout::Color::BLUE));
  renderer.SetBorder(osmscout::BorderStyleRef(), 1.0);
  renderer.DrawRect(25.0, 25.0, 25.0, 25.0);
  renderer.EndPrimitive();

  SkBitmap bitmap;
  bitmap.allocPixels(SkImageInfo::Make(50, 50,
                                        SkColorType::kRGBA_8888_SkColorType,
                                        SkAlphaType::kPremul_SkAlphaType));
  surface->readPixels(bitmap, 0, 0);

  // First rect area: red
  SkColor redArea = bitmap.getColor(12, 12);
  REQUIRE(SkColorGetR(redArea) == 255);
  REQUIRE(SkColorGetB(redArea) == 0);

  // Second rect area: blue
  SkColor blueArea = bitmap.getColor(37, 37);
  REQUIRE(SkColorGetB(blueArea) == 255);
  REQUIRE(SkColorGetR(blueArea) == 0);
}

// ---------------------------------------------------------------------------
// Pattern warning (no crash)
// ---------------------------------------------------------------------------
TEST_CASE("Pattern fill logs warning but does not crash", "[SymbolRendererSkia]") {
  auto surface = MakeSurface(50, 50);
  SkCanvas* canvas = surface->getCanvas();

  auto patternFill = std::make_shared<osmscout::FillStyle>();
  patternFill->SetFillColor(osmscout::Color::RED);
  patternFill->SetPattern("test_pattern");

  TestSymbolRenderer renderer(canvas);
  renderer.BeginPrimitive();
  renderer.SetFill(patternFill);
  renderer.SetBorder(osmscout::BorderStyleRef(), 1.0);
  renderer.DrawRect(0.0, 0.0, 10.0, 10.0);
  renderer.EndPrimitive();

  REQUIRE(true);
}
