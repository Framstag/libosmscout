/*
  SymbolRendererCairoTest - a test program for libosmscout
  Copyright (C) 2026  Tim Teulings

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

#include <vector>

#include <cairo.h>

#include <osmscoutmap/SymbolRenderer.h>
#include <osmscoutmap/Styles.h>

#include <osmscoutmapcairo/SymbolRendererCairo.h>

#include <catch2/catch_test_macros.hpp>

using namespace osmscout;

// Test helper: exposes protected SymbolRendererCairo methods for unit testing
class TestSymbolRenderer : public SymbolRendererCairo
{
public:
  using SymbolRendererCairo::SymbolRendererCairo;
  using SymbolRendererCairo::BeginPrimitive;
  using SymbolRendererCairo::SetFill;
  using SymbolRendererCairo::SetBorder;
  using SymbolRendererCairo::DrawPolygon;
  using SymbolRendererCairo::DrawRect;
  using SymbolRendererCairo::DrawCircle;
  using SymbolRendererCairo::EndPrimitive;
};

static FillStyleRef CreateFill(const Color& color)
{
  auto fill=std::make_shared<FillStyle>();
  fill->SetFillColor(color);
  return fill;
}

static BorderStyleRef CreateBorder(const Color& color, double width)
{
  auto border=std::make_shared<BorderStyle>();
  border->SetColor(color);
  border->SetWidth(width);
  return border;
}

namespace {
  constexpr unsigned int WHITE=0xffffff;
  constexpr unsigned int RED=0xff0000;
  constexpr unsigned int BLUE=0x0000ff;
}

// Renders primitives onto a small in-memory RGB24 surface and allows
// sampling of the resulting pixels
struct TestSurface
{
  static constexpr int WIDTH=24;
  static constexpr int HEIGHT=24;

  cairo_surface_t* surface;
  cairo_t* context;

  TestSurface()
  {
    surface=cairo_image_surface_create(CAIRO_FORMAT_RGB24,WIDTH,HEIGHT);
    REQUIRE(surface!=nullptr);

    context=cairo_create(surface);
    REQUIRE(context!=nullptr);

    cairo_set_source_rgb(context,1.0,1.0,1.0);
    cairo_paint(context);
  }

  ~TestSurface()
  {
    cairo_destroy(context);
    cairo_surface_destroy(surface);
  }

  // Returns the pixel at (x,y) as packed 0x00RRGGBB
  unsigned int GetPixel(unsigned int x,unsigned int y) const
  {
    cairo_surface_flush(surface);

    const unsigned char* data=cairo_image_surface_get_data(surface);
    const int stride=cairo_image_surface_get_stride(surface);
    const unsigned char* pixel=data+y*stride+x*4;

    return (static_cast<unsigned int>(pixel[2])<<16) |
           (static_cast<unsigned int>(pixel[1])<<8) |
           static_cast<unsigned int>(pixel[0]);
  }

  // True if the pixel is predominantly blue. Antialiased band edges
  // blend with the white background, lowering red/green but keeping
  // a full blue channel.
  bool IsBluish(unsigned int x,unsigned int y) const
  {
    const unsigned int pixel=GetPixel(x,y);
    const unsigned int r=(pixel>>16)&0xff;
    const unsigned int g=(pixel>>8)&0xff;
    const unsigned int b=pixel&0xff;

    return b>=200 && r<=180 && g<=180;
  }
};

TEST_CASE("Cairo: border width converted by mm-per-pixel factor")
{
  TestSurface surface;
  TestSymbolRenderer renderer(surface.context);

  renderer.BeginPrimitive();
  renderer.SetFill(FillStyleRef());
  renderer.SetBorder(CreateBorder(Color::BLUE,1.0),2.0);
  renderer.DrawRect(3.0,3.0,14.0,14.0);
  renderer.EndPrimitive();

  // border width 1.0mm at 2 px/mm -> stroke band [2,4] around path y=3
  REQUIRE(surface.GetPixel(7,3)==BLUE);  // inside stroke band
  REQUIRE(surface.GetPixel(7,1)==WHITE); // outside the band
  REQUIRE(surface.GetPixel(7,10)==WHITE); // interior without fill
}

TEST_CASE("Cairo: sub-pixel border width conversion")
{
  TestSurface surface;
  TestSymbolRenderer renderer(surface.context);

  renderer.BeginPrimitive();
  renderer.SetFill(FillStyleRef());
  renderer.SetBorder(CreateBorder(Color::BLUE,0.5),2.0);
  renderer.DrawRect(3.0,3.0,14.0,14.0);
  renderer.EndPrimitive();

  // border width 0.5mm at 2 px/mm -> 1px stroke band around path y=3;
  // a 1px line centered on a pixel boundary renders as two half-covered
  // pixels (antialiasing), so check for blue dominance here
  REQUIRE(surface.IsBluish(7,3));          // inside the 1px band
  REQUIRE(surface.GetPixel(7,4)==WHITE);   // adjacent pixel outside
  REQUIRE(surface.GetPixel(7,10)==WHITE);  // interior without fill
}

TEST_CASE("Cairo: stroked polygon outline includes closing edge")
{
  TestSurface surface;
  TestSymbolRenderer renderer(surface.context);

  renderer.BeginPrimitive();
  renderer.SetFill(FillStyleRef());
  renderer.SetBorder(CreateBorder(Color::BLUE,2.0),1.0);

  std::vector<Vertex2D> polygon;
  polygon.emplace_back(8.0,2.0);
  polygon.emplace_back(8.0,8.0);
  polygon.emplace_back(2.0,8.0);
  polygon.emplace_back(2.0,2.0); // closing edge back to (8,2) along y=2
  renderer.DrawPolygon(polygon);
  renderer.EndPrimitive();

  // closing edge along y=2, border 2.0mm at 1 px/mm -> stroke band [1,3]
  REQUIRE(surface.GetPixel(5,2)==BLUE);  // closing edge is stroked
  REQUIRE(surface.GetPixel(5,5)==WHITE); // interior without fill
  REQUIRE(surface.GetPixel(5,0)==WHITE); // outside the shape
}

TEST_CASE("Cairo: fill and border emitted")
{
  TestSurface surface;
  TestSymbolRenderer renderer(surface.context);

  renderer.BeginPrimitive();
  renderer.SetFill(CreateFill(Color::RED));
  renderer.SetBorder(CreateBorder(Color::BLUE,2.0),1.0);
  renderer.DrawRect(3.0,3.0,10.0,10.0);
  renderer.EndPrimitive();

  REQUIRE(surface.GetPixel(8,8)==RED);  // interior shows fill color
  REQUIRE(surface.GetPixel(8,3)==BLUE); // border band shows border color
}

TEST_CASE("Cairo: shape without fill or border draws nothing")
{
  TestSurface surface;
  TestSymbolRenderer renderer(surface.context);

  renderer.BeginPrimitive();
  renderer.SetFill(FillStyleRef());
  renderer.SetBorder(BorderStyleRef(),1.0);
  renderer.DrawRect(3.0,3.0,14.0,14.0);
  renderer.EndPrimitive();

  REQUIRE(surface.GetPixel(7,7)==WHITE);  // inside the rect
  REQUIRE(surface.GetPixel(7,3)==WHITE);  // on the rect border line
  REQUIRE(surface.GetPixel(0,0)==WHITE);  // corner of the canvas
}

TEST_CASE("Cairo: dashed border scales with border width")
{
  TestSurface surface;
  TestSymbolRenderer renderer(surface.context);

  auto border=CreateBorder(Color::BLUE,1.0);
  border->SetDashes({2.0,2.0});

  renderer.BeginPrimitive();
  renderer.SetFill(FillStyleRef());
  renderer.SetBorder(border,2.0);
  renderer.DrawRect(3.0,3.0,14.0,14.0);
  renderer.EndPrimitive();

  // borderWidth = 1.0mm * 2 px/mm = 2px, dashes scaled by borderWidth:
  // dash 4px / gap 4px, starting at x=3: dash [3,7], gap [7,11], dash [11,15]
  REQUIRE(surface.GetPixel(5,3)==BLUE);   // in first dash
  REQUIRE(surface.GetPixel(9,3)==WHITE);  // in first gap
  REQUIRE(surface.GetPixel(13,3)==BLUE);  // in second dash
}
