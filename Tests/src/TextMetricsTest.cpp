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
#include <string>

#include <osmscout/projection/MercatorProjection.h>
#include <osmscoutmap/MapParameter.h>

#include <osmscoutmapskia/MapPainterSkia.h>

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

TEST_CASE("MeasureText returns label dimensions for non-empty text", "[TextMetrics]")
{
  osmscout::MapPainterSkia painter;
  auto                     projection = CreateProjection();
  auto                     parameter = CreateParameter();

  auto                     metrics = painter.MeasureText(projection, parameter, "Hello", 1.0);

  REQUIRE(metrics.width > 0.0);
  REQUIRE(metrics.height > 0.0);
}

TEST_CASE("MeasureText returns one glyph entry per character", "[TextMetrics]")
{
  osmscout::MapPainterSkia painter;
  auto                     projection = CreateProjection();
  auto                     parameter = CreateParameter();

  const std::string        text = "Hello";
  auto                     metrics = painter.MeasureText(projection, parameter, text, 1.0);

  REQUIRE(metrics.glyphs.size() == text.length());
}

TEST_CASE("MeasureText glyph box is relative to glyph base point", "[TextMetrics]")
{
  osmscout::MapPainterSkia painter;
  auto                     projection = CreateProjection();
  auto                     parameter = CreateParameter();

  auto                     metrics = painter.MeasureText(projection, parameter, "Hello", 1.0);

  REQUIRE_FALSE(metrics.glyphs.empty());

  for (const auto& glyph : metrics.glyphs) {
    // Box x is the ink bearing relative to the glyph base point: ink may start
    // slightly right of the base point, but never far from it
    REQUIRE(glyph.box.x >= -1.0);
    // Ink extends above the baseline, so box top is negative
    REQUIRE(glyph.box.y <= 0.0);
    REQUIRE(glyph.box.width > 0.0);
    REQUIRE(glyph.box.height > 0.0);
  }
}

TEST_CASE("MeasureText glyph boxes differ between glyphs with different ink", "[TextMetrics]")
{
  osmscout::MapPainterSkia painter;
  auto                     projection = CreateProjection();
  auto                     parameter = CreateParameter();

  // a full-height glyph and a small glyph: their boxes must not be identical
  const std::string text = "i";
  auto              metrics = painter.MeasureText(projection, parameter, text, 1.0);
  auto              metricsHel = painter.MeasureText(projection, parameter, "H", 1.0);

  REQUIRE(metrics.glyphs.size() == 1);
  REQUIRE(metricsHel.glyphs.size() == 1);

  // the narrow glyph with small ink must not report the same constant
  // font-level box as a wide glyph (would indicate font-box metrics)
  REQUIRE_FALSE(metrics.glyphs[0].box.width == metricsHel.glyphs[0].box.width);
}

TEST_CASE("MeasureText label dimensions equal the union of the glyph boxes", "[TextMetrics]")
{
  osmscout::MapPainterSkia painter;
  auto                     projection = CreateProjection();
  auto                     parameter = CreateParameter();

  const std::string        text = "Hello World";
  auto                     metrics = painter.MeasureText(projection, parameter, text, 1.0);

  REQUIRE(metrics.glyphs.size() == text.length());

  double minX = std::numeric_limits<double>::max();
  double maxX = std::numeric_limits<double>::lowest();
  double minY = std::numeric_limits<double>::max();
  double maxY = std::numeric_limits<double>::lowest();

  for (const auto& glyph : metrics.glyphs) {
    double x1 = glyph.position.GetX() + glyph.box.x;
    double y1 = glyph.position.GetY() + glyph.box.y;
    double x2 = x1 + glyph.box.width;
    double y2 = y1 + glyph.box.height;

    minX = std::min(minX, x1);
    minY = std::min(minY, y1);
    maxX = std::max(maxX, x2);
    maxY = std::max(maxY, y2);
  }

  REQUIRE(metrics.width == Catch::Approx(maxX-minX).margin(1.0));
  REQUIRE(metrics.height == Catch::Approx(maxY-minY).margin(1.0));
}

TEST_CASE("MeasureText glyph positions are relative to label origin", "[TextMetrics]")
{
  osmscout::MapPainterSkia painter;
  auto                     projection = CreateProjection();
  auto                     parameter = CreateParameter();

  auto                     metrics = painter.MeasureText(projection, parameter, "Hello", 1.0);

  REQUIRE_FALSE(metrics.glyphs.empty());

  // First glyph sits at the label origin
  REQUIRE(metrics.glyphs.front().position.GetX() == 0.0);
  REQUIRE(metrics.glyphs.front().position.GetY() == 0.0);

  // Glyph positions advance monotonically along the baseline
  double previousX = metrics.glyphs.front().position.GetX();

  for (size_t i = 1; i < metrics.glyphs.size(); ++i) {
    REQUIRE(metrics.glyphs[i].position.GetX() > previousX);
    previousX = metrics.glyphs[i].position.GetX();
  }
}
