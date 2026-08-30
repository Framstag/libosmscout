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

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <limits>

#if defined(__WIN32__) || defined(WIN32) || (defined(__APPLE__) && __APPLE__)
  #include <cairo.h>
#else
  #include <cairo/cairo.h>
#endif

#include <osmscout/projection/MercatorProjection.h>
#include <osmscoutmap/MapParameter.h>
#include <osmscoutmap/Styles.h>

#include <osmscoutmapcairo/MapPainterCairo.h>

namespace {

  /**
   * Test painter exposing the protected label drawing entry points.
   */
  class TestPainterCairo : public osmscout::MapPainterCairo
  {
  public:
    osmscout::MapPainter::ShieldGeometry ShieldGeometryFor(
      const osmscout::ScreenVectorRectangle& labelRectangle)
    {
      return GetShieldGeometry(labelRectangle);
    }

    std::shared_ptr<osmscout::MapPainterCairo::CairoLabel> LayoutLabel(const osmscout::Projection& projection,
                                                                       const osmscout::MapParameter& parameter,
                                                                       const std::string& text,
                                                                       double fontSize)
    {
      return Layout(projection, parameter, text, fontSize, -1, false, false);
    }

    void DrawShieldLabel(const osmscout::Projection& projection,
                         const osmscout::MapParameter& parameter,
                         const osmscout::ScreenVectorRectangle& labelRectangle,
                         const osmscout::LabelData& label,
                         const CairoNativeLabel& layout)
    {
      DrawLabel(projection, parameter, labelRectangle, label, layout);
    }
  };

  osmscout::MercatorProjection CreateProjection()
  {
    osmscout::MercatorProjection projection;

    projection.Set(osmscout::GeoCoord(50.107252570499767, 14.459053009732296),
                   0.0,
                   osmscout::Magnification(osmscout::Magnification::magClose),
                   96.0,
                   400,
                   200);

    return projection;
  }

  osmscout::MapParameter CreateParameter()
  {
    osmscout::MapParameter parameter;

    parameter.SetFontSize(10.0);
    parameter.SetFontName("Liberation Sans");

    return parameter;
  }

  struct PixelBounds
  {
    int minX{std::numeric_limits<int>::max()};
    int minY{std::numeric_limits<int>::max()};
    int maxX{std::numeric_limits<int>::min()};
    int maxY{std::numeric_limits<int>::min()};

    bool IsValid() const
    {
      return minX<=maxX;
    }
  };

  bool IsColor(uint8_t r, uint8_t g, uint8_t b, uint8_t sr, uint8_t sg, uint8_t sb, uint8_t tolerance=40)
  {
    return std::abs(static_cast<int>(r)-static_cast<int>(sr))<=tolerance &&
           std::abs(static_cast<int>(g)-static_cast<int>(sg))<=tolerance &&
           std::abs(static_cast<int>(b)-static_cast<int>(sb))<=tolerance;
  }
} // namespace

/**
 * The text of a shield label must be centered inside the shield background:
 * the distance from the drawn ink to the background edges must be symmetric
 * in both axes (spec: shield-label-rendering, "Text is centered within the
 * shield").
 */
TEST_CASE("Shield label text is centered in the shield background", "[MapPainterShield]")
{
  constexpr int   CanvasWidth=400;
  constexpr int   CanvasHeight=200;

  cairo_surface_t * surface=cairo_image_surface_create(CAIRO_FORMAT_RGB24,
                                                       CanvasWidth,
                                                       CanvasHeight);

  REQUIRE(surface!=nullptr);

  cairo_t * cr=cairo_create(surface);

  REQUIRE(cr!=nullptr);

  cairo_set_source_rgb(cr,1,1,1);
  cairo_paint(cr);

  TestPainterCairo painter;

  auto             projection=CreateProjection();
  auto             parameter=CreateParameter();

  // assign the internal drawing context and run the (empty) map drawing pass
  painter.DrawMap(projection, parameter, {}, cr);

  // shield style: magenta background, black text, blue border
  auto shieldStyle=std::make_shared<osmscout::ShieldStyle>();

  shieldStyle->SetBgColor(osmscout::Color(1.0, 0.0, 1.0));
  shieldStyle->SetBorderColor(osmscout::Color(0.0, 0.0, 1.0));
  shieldStyle->SetTextColor(osmscout::Color(0.0, 0.0, 0.0));

  std::string text="X XX";

  auto        layout=painter.LayoutLabel(projection, parameter, text, 1.0);

  REQUIRE(layout!=nullptr);
  REQUIRE(layout->width>0.0);
  REQUIRE(layout->height>0.0);

  osmscout::LabelData labelData;

  labelData.type=osmscout::LabelData::Type::Text;
  labelData.style=shieldStyle;
  labelData.alpha=1.0;
  labelData.fontSize=1.0;
  labelData.text=text;

  // draw the shield centered on the canvas
  double x=(CanvasWidth-layout->width)/2.0;
  double y=(CanvasHeight-layout->height)/2.0;

  painter.DrawShieldLabel(projection,
                          parameter,
                          osmscout::ScreenVectorRectangle(x,
                                                          y,
                                                          layout->width,
                                                          layout->height),
                          labelData,
                          layout->label);

  cairo_surface_flush(surface);

  const unsigned char * data=cairo_image_surface_get_data(surface);
  int                 stride=cairo_image_surface_get_stride(surface);

  REQUIRE(data!=nullptr);

  PixelBounds background;
  PixelBounds textInk;

  for (int py=0; py<CanvasHeight; py++) {
    for (int px=0; px<CanvasWidth; px++) {
      const unsigned char * p=data+py*stride+px*4; // BGRA on little endian
      uint8_t             b=p[0];
      uint8_t             g=p[1];
      uint8_t             r=p[2];

      if (IsColor(r, g, b, 255, 0, 255)) {
        background.minX=std::min(background.minX, px);
        background.minY=std::min(background.minY, py);
        background.maxX=std::max(background.maxX, px);
        background.maxY=std::max(background.maxY, py);
      }
      else if (IsColor(r, g, b, 0, 0, 0)) {
        textInk.minX=std::min(textInk.minX, px);
        textInk.minY=std::min(textInk.minY, py);
        textInk.maxX=std::max(textInk.maxX, px);
        textInk.maxY=std::max(textInk.maxY, py);
      }
    }
  }

  REQUIRE(background.IsValid());
  REQUIRE(textInk.IsValid());

  double bgCenterX=(background.minX+background.maxX)/2.0;
  double bgCenterY=(background.minY+background.maxY)/2.0;
  double textCenterX=(textInk.minX+textInk.maxX)/2.0;
  double textCenterY=(textInk.minY+textInk.maxY)/2.0;

  REQUIRE(textCenterX==Catch::Approx(bgCenterX).margin(2.0));
  REQUIRE(textCenterY==Catch::Approx(bgCenterY).margin(2.0));

  cairo_destroy(cr);
  cairo_surface_destroy(surface);
}

/**
 * The shield border rectangle must lie fully inside the shield background
 * rectangle for arbitrary label geometries (spec shield-label-rendering,
 * "Shield border drawn inside the background").
 */
TEST_CASE("Shield border lies inside the shield background", "[MapPainterShield]")
{
  TestPainterCairo                             painter;

  std::vector<osmscout::ScreenVectorRectangle> cases={
    {  0.0,   0.0,  10.0,   6.0 }, // small label
    { 10.0, -20.0,  40.0,  12.0 }, // regular label
    {-30.0,  50.0, 120.0,  30.0 }  // large label
  };

  for (const auto& labelRectangle : cases) {
    auto geometry=painter.ShieldGeometryFor(labelRectangle);

    REQUIRE(geometry.border.x>=geometry.background.x);
    REQUIRE(geometry.border.y>=geometry.background.y);
    REQUIRE(geometry.border.x+geometry.border.width<=
            geometry.background.x+geometry.background.width);
    REQUIRE(geometry.border.y+geometry.border.height<=
            geometry.background.y+geometry.background.height);
  }
}
