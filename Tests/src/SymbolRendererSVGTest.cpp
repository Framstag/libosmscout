/*
  SymbolRendererSVGTest - a test program for libosmscout
  Copyright (C) 2023  Tim Teulings

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <sstream>
#include <string>

#include <osmscout/projection/MercatorProjection.h>

#include <osmscoutmap/SymbolRenderer.h>
#include <osmscoutmap/Styles.h>

#include <osmscoutmapsvg/SymbolRendererSVG.h>

#include <catch2/catch_test_macros.hpp>

using namespace osmscout;

// Test helper: exposes protected SymbolRendererSVG methods for unit testing
class TestSymbolRenderer : public SymbolRendererSVG {
public:
  using SymbolRendererSVG::SymbolRendererSVG;
  using SymbolRendererSVG::SetFill;
  using SymbolRendererSVG::SetBorder;
  using SymbolRendererSVG::DrawPolygon;
  using SymbolRendererSVG::DrawRect;
  using SymbolRendererSVG::DrawCircle;
};

static MercatorProjection CreateDefaultProjection()
{
  MercatorProjection projection;

  projection.Set(GeoCoord(51.51241, 7.46525),
                0.0,
                 Magnification(osmscout::Magnification::magClose),
                 1.0 /*dpi*/,
                 800 /*width*/,
                 600 /*height*/);

  return projection;
}

static FillStyleRef CreateFill(const Color& color)
{
  auto fill = std::make_shared<FillStyle>();
  fill->SetFillColor(color);
  return fill;
}

static BorderStyleRef CreateBorder(const Color& color, double width)
{
  auto border = std::make_shared<BorderStyle>();
  border->SetColor(color);
  border->SetWidth(width);
  return border;
}

TEST_CASE("SetFill with visible color emits fill attribute")
{
  std::ostringstream stream;
  TestSymbolRenderer renderer(stream);

  renderer.SetFill(CreateFill(Color::RED));
  renderer.SetBorder(BorderStyleRef(), 1.0);
  renderer.DrawCircle(10.0, 20.0, 5.0);

  std::string output = stream.str();
  REQUIRE(output.find("fill=\"#ff0000\"") != std::string::npos);
  REQUIRE(output.find("fill=\"none\"") == std::string::npos);
}

TEST_CASE("SetFill with null style emits fill=\"none\"")
{
  std::ostringstream stream;
  TestSymbolRenderer renderer(stream);

  renderer.SetFill(FillStyleRef());
  renderer.SetBorder(BorderStyleRef(), 1.0);
  renderer.DrawRect(0.0, 0.0, 10.0, 10.0);

  std::string output = stream.str();
  REQUIRE(output.find("fill=\"none\"") != std::string::npos);
}

TEST_CASE("SetBorder emits stroke and stroke-width")
{
  std::ostringstream stream;
  TestSymbolRenderer renderer(stream);

  renderer.SetFill(FillStyleRef());
  renderer.SetBorder(CreateBorder(Color::BLUE, 2.0), 1.0);
  renderer.DrawRect(5.0, 5.0, 20.0, 20.0);

  std::string output = stream.str();
  REQUIRE(output.find("stroke=\"#0000ff\"") != std::string::npos);
  REQUIRE(output.find("stroke-width=\"2\"") != std::string::npos);
}

TEST_CASE("SetBorder with null style emits stroke=\"none\"")
{
  std::ostringstream stream;
  TestSymbolRenderer renderer(stream);

  renderer.SetFill(FillStyleRef());
  renderer.SetBorder(BorderStyleRef(), 1.0);
  renderer.DrawRect(0.0, 0.0, 10.0, 10.0);

  std::string output = stream.str();
  REQUIRE(output.find("stroke=\"none\"") != std::string::npos);
}

TEST_CASE("DrawRect outputs rect element with attributes")
{
  std::ostringstream stream;
  TestSymbolRenderer renderer(stream);

  renderer.SetFill(FillStyleRef());
  renderer.SetBorder(BorderStyleRef(), 1.0);
  renderer.DrawRect(10.5, 20.5, 30.0, 40.0);

  std::string output = stream.str();
  REQUIRE(output.find("<rect") != std::string::npos);
  REQUIRE(output.find("x=\"10.5\"") != std::string::npos);
  REQUIRE(output.find("y=\"20.5\"") != std::string::npos);
  REQUIRE(output.find("width=\"30\"") != std::string::npos);
  REQUIRE(output.find("height=\"40\"") != std::string::npos);
  REQUIRE(output.find("/>") != std::string::npos);
}

TEST_CASE("DrawCircle outputs circle element with attributes")
{
  std::ostringstream stream;
  TestSymbolRenderer renderer(stream);

  renderer.SetFill(FillStyleRef());
  renderer.SetBorder(BorderStyleRef(), 1.0);
  renderer.DrawCircle(15.0, 25.0, 8.0);

  std::string output = stream.str();
  REQUIRE(output.find("<circle") != std::string::npos);
  REQUIRE(output.find("cx=\"15\"") != std::string::npos);
  REQUIRE(output.find("cy=\"25\"") != std::string::npos);
  REQUIRE(output.find("r=\"8\"") != std::string::npos);
  REQUIRE(output.find("/>") != std::string::npos);
}

TEST_CASE("DrawPolygon outputs polyline element with points")
{
  std::ostringstream stream;
  TestSymbolRenderer renderer(stream);

  renderer.SetFill(FillStyleRef());
  renderer.SetBorder(BorderStyleRef(), 1.0);

  std::vector<Vertex2D> polygon;
  polygon.emplace_back(0.0, 0.0);
  polygon.emplace_back(10.0, 0.0);
  polygon.emplace_back(10.0, 10.0);
  polygon.emplace_back(0.0, 10.0);
  renderer.DrawPolygon(polygon);

  std::string output = stream.str();
  REQUIRE(output.find("<polyline") != std::string::npos);
  REQUIRE(output.find("points=") != std::string::npos);
  REQUIRE(output.find("0,0") != std::string::npos);
  REQUIRE(output.find("10,10") != std::string::npos);
  REQUIRE(output.find("/>") != std::string::npos);
}

TEST_CASE("Render with polygon primitive")
{
  std::ostringstream stream;
  MercatorProjection projection = CreateDefaultProjection();

  auto fill = CreateFill(Color::RED);
  auto border = CreateBorder(Color::BLACK, 1.0);
  auto polygon = std::make_shared<PolygonPrimitive>(fill, border);
  polygon->AddCoord(Vertex2D(0.0, 0.0));
  polygon->AddCoord(Vertex2D(10.0, 0.0));
  polygon->AddCoord(Vertex2D(10.0, 10.0));

  Symbol symbol("test-symbol", Symbol::ProjectionMode::MAP);
  symbol.AddPrimitive(polygon);

  SymbolRendererSVG renderer(stream);
  renderer.Render(projection, symbol, Vertex2D(100.0, 100.0));

  std::string output = stream.str();
  REQUIRE(output.find("<polyline") != std::string::npos);
  REQUIRE(output.find("fill=\"#ff0000\"") != std::string::npos);
  REQUIRE(output.find("stroke=\"#000000\"") != std::string::npos);
}

TEST_CASE("Render with rectangle primitive")
{
  std::ostringstream stream;
  MercatorProjection projection = CreateDefaultProjection();

  auto fill = CreateFill(Color::GREEN);
  auto rectangle = std::make_shared<RectanglePrimitive>(
      Vertex2D(5.0, 5.0), 20.0, 15.0, fill, BorderStyleRef());

  Symbol symbol("rect-symbol", Symbol::ProjectionMode::MAP);
  symbol.AddPrimitive(rectangle);

  SymbolRendererSVG renderer(stream);
  renderer.Render(projection, symbol, Vertex2D(200.0, 200.0));

  std::string output = stream.str();
  REQUIRE(output.find("<rect") != std::string::npos);
  REQUIRE(output.find("fill=\"#00ff00\"") != std::string::npos);
}

TEST_CASE("Render with circle primitive")
{
  std::ostringstream stream;
  MercatorProjection projection = CreateDefaultProjection();

  auto fill = CreateFill(Color::BLUE);
  auto circle = std::make_shared<CirclePrimitive>(
      Vertex2D(0.0, 0.0), 10.0, fill, BorderStyleRef());

  Symbol symbol("circle-symbol", Symbol::ProjectionMode::MAP);
  symbol.AddPrimitive(circle);

  SymbolRendererSVG renderer(stream);
  renderer.Render(projection, symbol, Vertex2D(150.0, 150.0));

  std::string output = stream.str();
  REQUIRE(output.find("<circle") != std::string::npos);
  REQUIRE(output.find("fill=\"#0000ff\"") != std::string::npos);
}

TEST_CASE("Render multiple primitives")
{
  std::ostringstream stream;
  MercatorProjection projection = CreateDefaultProjection();

  Symbol symbol("multi-symbol", Symbol::ProjectionMode::MAP);
  symbol.AddPrimitive(std::make_shared<CirclePrimitive>(
      Vertex2D(0.0, 0.0), 5.0, CreateFill(Color::RED), BorderStyleRef()));
  symbol.AddPrimitive(std::make_shared<RectanglePrimitive>(
      Vertex2D(-5.0, -5.0), 10.0, 10.0, CreateFill(Color::GREEN), BorderStyleRef()));

  SymbolRendererSVG renderer(stream);
  renderer.Render(projection, symbol, Vertex2D(100.0, 100.0));

  std::string output = stream.str();
  REQUIRE(output.find("<circle") != std::string::npos);
  REQUIRE(output.find("<rect") != std::string::npos);
  // Both fills should appear
  REQUIRE(output.find("fill=\"#ff0000\"") != std::string::npos);
  REQUIRE(output.find("fill=\"#00ff00\"") != std::string::npos);
}

TEST_CASE("Render with scaleFactor scales coordinates")
{
  std::ostringstream streamSmall;
  std::ostringstream streamLarge;

  MercatorProjection projection = CreateDefaultProjection();

  Symbol symbol("scaled-symbol", Symbol::ProjectionMode::MAP);
  symbol.AddPrimitive(std::make_shared<RectanglePrimitive>(
      Vertex2D(0.0, 0.0), 10.0, 10.0, CreateFill(Color::RED), BorderStyleRef()));

  SymbolRendererSVG rendererSmall(streamSmall);
  rendererSmall.Render(projection, symbol, Vertex2D(100.0, 100.0), 1.0);

  SymbolRendererSVG rendererLarge(streamLarge);
  rendererLarge.Render(projection, symbol, Vertex2D(100.0, 100.0), 2.0);

  // With scaleFactor=2, dimensions should be larger than with scaleFactor=1
  // Both outputs contain rect width/height but scaled differently
  std::string small = streamSmall.str();
  std::string large = streamLarge.str();
  REQUIRE(small != large);
  // scaleFactor=2 produces larger coordinate values
}

TEST_CASE("Render with GROUND vs MAP projection mode")
{
  std::ostringstream streamMap;
  std::ostringstream streamGround;

  MercatorProjection projection = CreateDefaultProjection();

  auto circle = std::make_shared<CirclePrimitive>(
      Vertex2D(0.0, 0.0), 10.0, CreateFill(Color::RED), BorderStyleRef());

  Symbol mapSym("map-symbol", Symbol::ProjectionMode::MAP);
  mapSym.AddPrimitive(circle);

  Symbol groundSym("ground-symbol", Symbol::ProjectionMode::GROUND);
  groundSym.AddPrimitive(circle);

  SymbolRendererSVG rendererMap(streamMap);
  rendererMap.Render(projection, mapSym, Vertex2D(100.0, 100.0));

  SymbolRendererSVG rendererGround(streamGround);
  rendererGround.Render(projection, groundSym, Vertex2D(100.0, 100.0));

  // Different projection modes produce different coordinate scaling
  std::string mapOut = streamMap.str();
  std::string groundOut = streamGround.str();
  REQUIRE(mapOut != groundOut);
}
