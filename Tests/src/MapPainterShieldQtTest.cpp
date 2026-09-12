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

#include <QApplication>
#include <QImage>
#include <QPainter>
#include <QtGlobal>

#include <osmscout/projection/MercatorProjection.h>
#include <osmscoutmap/MapParameter.h>
#include <osmscoutmap/Styles.h>

#include <osmscoutmapqt/MapPainterQt.h>

namespace {

  /**
   * Test painter exposing the protected label drawing entry points.
   */
  class TestPainterQt : public osmscout::MapPainterQt
  {
  public:
    std::shared_ptr<osmscout::QtLabel> LayoutLabel(const osmscout::Projection& projection,
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
                         const QTextLayout& layout)
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

  bool IsColor(const QColor& c, int sr, int sg, int sb, int tolerance=40)
  {
    return std::abs(c.red()-sr)<=tolerance &&
           std::abs(c.green()-sg)<=tolerance &&
           std::abs(c.blue()-sb)<=tolerance;
  }
} // namespace

/**
 * Shield text centered in the shield background (Qt backend).
 */
TEST_CASE("Qt shield label text is centered in the shield background", "[MapPainterShieldQt]")
{
#if !defined(_WIN32)
  // Run headless on Unix; the test does not need a windowing system.
  // On Windows the default platform plugin is used (runs in a desktop session).
  qputenv("QT_QPA_PLATFORM", "offscreen");

#endif

  int           argc = 1;
  char          arg0[] = "MapPainterShieldQtTest";
  char          * argv[1] = {arg0};
  QApplication  app(argc, argv);

  constexpr int CanvasWidth=400;
  constexpr int CanvasHeight=200;

  QImage        image(CanvasWidth, CanvasHeight, QImage::Format_RGB32);

  image.fill(QColor(255, 255, 255));

  QPainter      qp(&image);

  TestPainterQt painter;

  auto          projection=CreateProjection();
  auto          parameter=CreateParameter();

  // Assign the internal QPainter used by Layout()/MeasureText()
  painter.DrawMap(CreateProjection(), CreateParameter(), {}, &qp);

  auto shieldStyle=std::make_shared<osmscout::ShieldStyle>();

  shieldStyle->SetBgColor(osmscout::Color(1.0, 0.0, 1.0));
  shieldStyle->SetBorderColor(osmscout::Color(0.0, 0.0, 1.0));
  shieldStyle->SetTextColor(osmscout::Color(0.0, 0.0, 0.0));

  std::string text="X XX";

  auto        layout=painter.LayoutLabel(CreateProjection(), CreateParameter(), text, 1.0);

  REQUIRE(layout!=nullptr);
  REQUIRE(layout->width>0.0);
  REQUIRE(layout->height>0.0);

  osmscout::LabelData labelData;

  labelData.type=osmscout::LabelData::Type::Text;
  labelData.style=shieldStyle;
  labelData.alpha=1.0;
  labelData.fontSize=1.0;
  labelData.text=text;

  double x=(CanvasWidth-layout->width)/2.0;
  double y=(CanvasHeight-layout->height)/2.0;

  painter.DrawShieldLabel(CreateProjection(),
                          CreateParameter(),
                          osmscout::ScreenVectorRectangle(x,
                                                          y,
                                                          layout->width,
                                                          layout->height),
                          labelData,
                          layout->label);

  qp.end();

  PixelBounds background;
  PixelBounds textInk;

  for (int py=0; py<CanvasHeight; py++) {
    for (int px=0; px<CanvasWidth; px++) {
      QColor c=image.pixelColor(px, py);

      if (IsColor(c, 255, 0, 255)) {
        background.minX=std::min(background.minX, px);
        background.minY=std::min(background.minY, py);
        background.maxX=std::max(background.maxX, px);
        background.maxY=std::max(background.maxY, py);
      }
      else if (IsColor(c, 0, 0, 0)) {
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
}
