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
#include <memory>

#include <QApplication>
#include <QPainter>
#include <QPixmap>
#include <QtGlobal>

#include <osmscout/TypeConfig.h>
#include <osmscout/projection/MercatorProjection.h>
#include <osmscoutmap/MapData.h>
#include <osmscoutmap/MapParameter.h>
#include <osmscoutmap/StyleConfig.h>

#include <osmscoutmapqt/MapPainterQt.h>

namespace {

  /**
   * The one QApplication of this test process. Qt allows a single instance, while Catch2 runs every
   * case of this binary in one process, so the cases share it instead of constructing their own.
   */
  QApplication& GetApplication()
  {
#if !defined(_WIN32)
    // Run headless on Unix; the tests do not need a windowing system.
    // On Windows the default platform plugin is used (runs in a desktop session).
    qputenv("QT_QPA_PLATFORM", "offscreen");
#endif

    static int  argc=1;
    static char arg0[]="TextMetricsQtTest";
    static char *argv[1]={arg0};
    static QApplication app(argc, argv);

    return app;
  }

  osmscout::MercatorProjection CreateProjection()
  {
    osmscout::MercatorProjection projection;

    projection.Set(osmscout::GeoCoord(50.107252570499767, 14.459053009732296),
                   0.0,
                   osmscout::Magnification(osmscout::Magnification::magClose),
                   96.0,
                   800,
                   480);

    return projection;
  }

  osmscout::MapParameter CreateParameter()
  {
    osmscout::MapParameter parameter;

    parameter.SetFontSize(10.0);

    return parameter;
  }

  /**
   * A painter with a drawing device, so that its font resolution and label layout can be exercised.
   * The device is only what the layout measures through; nothing is drawn into it.
   */
  struct PainterFixture
  {
    QPixmap                      pixmap{800, 200};
    QPainter                     qp{&pixmap};
    osmscout::MercatorProjection projection{CreateProjection()};
    osmscout::MapParameter       parameter{CreateParameter()};
    osmscout::MapPainterQt       painter;

    PainterFixture()
    {
      pixmap.fill(Qt::white);

      painter.DrawMap(projection, parameter, {}, &qp);
    }

    ~PainterFixture()
    {
      qp.end();
    }

    /**
     * The factor the painter scales a label font size with before it quantizes it to the pixel grid
     * (see MapPainterQt::GetFont).
     */
    double Scale() const
    {
      return projection.ConvertWidthToPixel(parameter.GetFontSize());
    }
  };
} // namespace

TEST_CASE("Qt MeasureText glyph positions are relative to label origin", "[TextMetricsQt]")
{
  GetApplication();

  QPixmap      pixmap(800, 200);

  pixmap.fill(Qt::white);

  QPainter               qp(&pixmap);

  osmscout::MapPainterQt painter;

  // DrawMap sets the internal QPainter used by Layout()/MeasureText()
  painter.DrawMap(CreateProjection(), CreateParameter(), {}, &qp);

  auto metrics = painter.MeasureText(CreateProjection(), CreateParameter(), "Hello", 1.0);

  REQUIRE(metrics.glyphs.size() == 5);

  for (const auto& glyph : metrics.glyphs) {
    // Positions are relative to the label origin (left baseline origin),
    // not the QTextLayout origin which includes the line leading offset.
    REQUIRE(glyph.position.GetY() == 0.0);
    // Ink extends above the baseline, so box top is negative
    REQUIRE(glyph.box.y <= 0.0);
    REQUIRE(glyph.box.width > 0.0);
    REQUIRE(glyph.box.height > 0.0);
  }

  // Glyph positions advance monotonically along the baseline
  double previousX = metrics.glyphs.front().position.GetX();

  for (size_t i = 1; i < metrics.glyphs.size(); ++i) {
    REQUIRE(metrics.glyphs[i].position.GetX() > previousX);
    previousX = metrics.glyphs[i].position.GetX();
  }

  // Label dimensions describe the ink of the drawn text (single line here):
  // the label height must not be the font box height: for "Hello" the ink
  // spans from the topmost glyph to the baseline (l has an ascender, no
  // descender ink below the baseline)
  double minY = std::numeric_limits<double>::max();
  double maxY = std::numeric_limits<double>::lowest();
  double minX = std::numeric_limits<double>::max();
  double maxX = std::numeric_limits<double>::lowest();

  for (const auto& glyph : metrics.glyphs) {
    minX = std::min(minX, glyph.position.GetX() + glyph.box.x);
    minY = std::min(minY, glyph.position.GetY() + glyph.box.y);
    maxX = std::max(maxX, glyph.position.GetX() + glyph.box.x + glyph.box.width);
    maxY = std::max(maxY, glyph.position.GetY() + glyph.box.y + glyph.box.height);
  }

  REQUIRE(metrics.width == Catch::Approx(maxX-minX).margin(1.0));
  REQUIRE(metrics.height == Catch::Approx(maxY-minY).margin(1.0));

  qp.end();
}

TEST_CASE("Qt painter shares one resolved font for sizes within a device pixel", "[TextMetricsQt]")
{
  GetApplication();

  PainterFixture fixture;

  const double scale=fixture.Scale();

  REQUIRE(scale>0.0);

  const size_t baseline=fixture.painter.GetResolvedFontCount();

  // Both sizes scale to a value that truncates to the same device pixel
  auto firstMetrics=fixture.painter.MeasureText(fixture.projection, fixture.parameter, "Hello", 60.1/scale);

  const size_t afterFirst=fixture.painter.GetResolvedFontCount();

  REQUIRE(afterFirst==baseline+1);

  auto secondMetrics=fixture.painter.MeasureText(fixture.projection, fixture.parameter, "Hello", 60.9/scale);

  REQUIRE(fixture.painter.GetResolvedFontCount()==afterFirst);

  // Both labels are measured and drawn with the font the painter resolved for the first one
  REQUIRE(secondMetrics.width==Catch::Approx(firstMetrics.width));
  REQUIRE(secondMetrics.height==Catch::Approx(firstMetrics.height));
  REQUIRE(secondMetrics.glyphs.size()==firstMetrics.glyphs.size());

  for (size_t i=0; i<firstMetrics.glyphs.size(); i++) {
    REQUIRE(secondMetrics.glyphs[i].position.GetX()==Catch::Approx(firstMetrics.glyphs[i].position.GetX()));
    REQUIRE(secondMetrics.glyphs[i].box.width==Catch::Approx(firstMetrics.glyphs[i].box.width));
    REQUIRE(secondMetrics.glyphs[i].box.height==Catch::Approx(firstMetrics.glyphs[i].box.height));
  }
}

TEST_CASE("Qt painter resolves no font per label and none for a repeated frame", "[TextMetricsQt]")
{
  GetApplication();

  PainterFixture fixture;

  const double scale=fixture.Scale();

  const size_t baseline=fixture.painter.GetResolvedFontCount();
  const size_t baselineRetained=fixture.painter.GetRetainedFontCount();

  // Twenty distinct label sizes spread over five device pixel steps: four labels per step, each with
  // a sub-pixel difference inside the step
  for (size_t i=0; i<20; i++) {
    const double pixelSize=50.5+static_cast<double>(i%5)+0.02*static_cast<double>(i/5);

    fixture.painter.MeasureText(fixture.projection,
                                fixture.parameter,
                                "Hello",
                                pixelSize/scale);
  }

  // One font per device pixel step, not one per label
  REQUIRE(fixture.painter.GetResolvedFontCount()==baseline+5);
  REQUIRE(fixture.painter.GetRetainedFontCount()==baselineRetained+5);

  // The repeated frame resolves no further font
  for (size_t i=0; i<20; i++) {
    const double pixelSize=50.5+static_cast<double>(i%5)+0.02*static_cast<double>(i/5);

    fixture.painter.MeasureText(fixture.projection,
                                fixture.parameter,
                                "Hello",
                                pixelSize/scale);
  }

  REQUIRE(fixture.painter.GetResolvedFontCount()==baseline+5);
}

TEST_CASE("Qt painter releases the resolved fonts it retains", "[TextMetricsQt]")
{
  GetApplication();

  PainterFixture fixture;

  const double scale=fixture.Scale();
  const double fontSize=60.1/scale;

  fixture.painter.MeasureText(fixture.projection, fixture.parameter, "Hello", fontSize);

  const size_t afterFirst=fixture.painter.GetResolvedFontCount();

  REQUIRE(afterFirst>=1);
  REQUIRE(fixture.painter.GetRetainedFontCount()>=1);

  fixture.painter.ReleaseFonts();

  REQUIRE(fixture.painter.GetRetainedFontCount()==0);

  fixture.painter.MeasureText(fixture.projection, fixture.parameter, "Hello", fontSize);

  // The released font is resolved again rather than served from the cache
  REQUIRE(fixture.painter.GetResolvedFontCount()==afterFirst+1);
}

TEST_CASE("Qt painter draws a label as before after releasing its fonts", "[TextMetricsQt]")
{
  GetApplication();

  PainterFixture fixture;

  const double scale=fixture.Scale();
  const double fontSize=60.1/scale;

  auto before=fixture.painter.MeasureText(fixture.projection, fixture.parameter, "Hello", fontSize);

  fixture.painter.ReleaseFonts();

  auto after=fixture.painter.MeasureText(fixture.projection, fixture.parameter, "Hello", fontSize);

  REQUIRE(after.width==Catch::Approx(before.width));
  REQUIRE(after.height==Catch::Approx(before.height));
  REQUIRE(after.glyphs.size()==before.glyphs.size());

  for (size_t i=0; i<before.glyphs.size(); i++) {
    REQUIRE(after.glyphs[i].position.GetX()==Catch::Approx(before.glyphs[i].position.GetX()));
    REQUIRE(after.glyphs[i].position.GetY()==Catch::Approx(before.glyphs[i].position.GetY()));
    REQUIRE(after.glyphs[i].box.x==Catch::Approx(before.glyphs[i].box.x));
    REQUIRE(after.glyphs[i].box.y==Catch::Approx(before.glyphs[i].box.y));
    REQUIRE(after.glyphs[i].box.width==Catch::Approx(before.glyphs[i].box.width));
    REQUIRE(after.glyphs[i].box.height==Catch::Approx(before.glyphs[i].box.height));
  }
}

TEST_CASE("Qt painter releases the fonts of a replaced stylesheet", "[TextMetricsQt]")
{
  GetApplication();

  PainterFixture fixture;

  auto typeConfig=std::make_shared<osmscout::TypeConfig>();
  auto firstStylesheet=std::make_shared<osmscout::StyleConfig>(typeConfig);
  auto secondStylesheet=std::make_shared<osmscout::StyleConfig>(typeConfig);

  const double scale=fixture.Scale();
  const double fontSize=60.1/scale;

  osmscout::MapData data;

  data.styleConfig=firstStylesheet;

  fixture.painter.DrawMap(fixture.projection, fixture.parameter, {data}, &fixture.qp);
  fixture.painter.MeasureText(fixture.projection, fixture.parameter, "Hello", fontSize);

  const size_t afterFirst=fixture.painter.GetResolvedFontCount();

  REQUIRE(afterFirst>=1);

  // Another stylesheet replaces the first one, so the painter releases the fonts of the replaced one
  data.styleConfig=secondStylesheet;

  fixture.painter.DrawMap(fixture.projection, fixture.parameter, {data}, &fixture.qp);
  fixture.painter.MeasureText(fixture.projection, fixture.parameter, "Hello", fontSize);

  // The font of the first stylesheet was released and is resolved again rather than served
  REQUIRE(fixture.painter.GetResolvedFontCount()>afterFirst);
}
