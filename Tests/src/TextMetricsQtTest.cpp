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
#include <QPainter>
#include <QPixmap>
#include <QtGlobal>

#include <osmscout/projection/MercatorProjection.h>
#include <osmscoutmap/MapParameter.h>

#include <osmscoutmapqt/MapPainterQt.h>

namespace {

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
} // namespace

TEST_CASE("Qt MeasureText glyph positions are relative to label origin", "[TextMetricsQt]")
{
#if !defined(_WIN32)
  // Run headless on Unix; the test does not need a windowing system.
  // On Windows the default platform plugin is used (runs in a desktop session).
  qputenv("QT_QPA_PLATFORM", "offscreen");

#endif

  int          argc = 1;
  char         arg0[] = "TextMetricsQtTest";
  char         * argv[1] = {arg0};
  QApplication app(argc, argv);

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
