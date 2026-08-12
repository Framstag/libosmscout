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

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <cmath>

#include <TextMetricsAll.h>

#ifndef TEXT_METRICS_FONT_PATH
#define TEXT_METRICS_FONT_PATH "../libosmscout-map-opengl/data/fonts/LiberationSans-Regular.ttf"
#endif

TEST_CASE("Reference pixel size conversion matches backend formula", "[TextMetricsReference]")
{
  // Backends convert font size as fontSize * fontSizeParam * dpi / 25.4
  double expected = 1.0 * 10.0 * 96.0 / 25.4;

  REQUIRE(TextMetricsAll::ReferencePixelSize(1.0, 10.0, 96.0) == Catch::Approx(expected));

  // FreeType pixel size is the rounded value
  long px = static_cast<long>(std::lround(TextMetricsAll::ReferencePixelSize(1.0, 10.0, 96.0)));

  REQUIRE(px == 38);
}

TEST_CASE("Reference ink box matches expected values for known glyph", "[TextMetricsReference]")
{
  TextMetricsAll::ReferenceMetrics metrics;
  std::string                      error;

  bool                             ok = TextMetricsAll::MeasureReference(TEXT_METRICS_FONT_PATH,
                                                                         "A",
                                                                         1.0, // fontSize
                                                                         10.0, // fontSizeParam
                                                                         96.0, // dpi
                                                                         metrics,
                                                                         error);

  REQUIRE(ok);
  REQUIRE(error.empty());

  REQUIRE(metrics.glyphs.size() == 1);

  // Expected values for 'A' in LiberationSans-Regular at 38px
  // (measured with FreeType: horiBearingX/Y, width, height, advance, /64)
  const auto & glyph = metrics.glyphs[0];

  REQUIRE(glyph.x == Catch::Approx(0.0));
  REQUIRE(glyph.y == Catch::Approx(-26.0));
  REQUIRE(glyph.width == Catch::Approx(26.0));
  REQUIRE(glyph.height == Catch::Approx(26.0));
  REQUIRE(glyph.advance == Catch::Approx(25.0));

  // Label width is the sum of advances
  REQUIRE(metrics.width == Catch::Approx(25.0));
  REQUIRE(metrics.height > 0.0);
}

TEST_CASE("Reference measures one glyph per character", "[TextMetricsReference]")
{
  TextMetricsAll::ReferenceMetrics metrics;
  std::string                      error;

  bool                             ok = TextMetricsAll::MeasureReference(TEXT_METRICS_FONT_PATH,
                                                                         "Hello",
                                                                         1.0,
                                                                         10.0,
                                                                         96.0,
                                                                         metrics,
                                                                         error);

  REQUIRE(ok);
  REQUIRE(metrics.glyphs.size() == 5);
  REQUIRE(metrics.width > 0.0);
}
