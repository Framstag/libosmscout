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

#include <cmath>
#include <string>

#include <cairo/cairo.h>

#include <QApplication>
#include <QImage>
#include <QPainter>
#include <QtGlobal>

#include <osmscout/projection/MercatorProjection.h>
#include <osmscoutmap/MapParameter.h>

#include <osmscoutmapcairo/MapPainterCairo.h>
#include <osmscoutmapqt/MapPainterQt.h>
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

    parameter.SetFontSize(2.0);

    return parameter;
  }

  /**
   * Per-glyph advance increments (pos[i+1]-pos[i]): this removes the
   * per-backend convention for anchoring the first glyph (pen relative vs.
   * ink relative) and makes positions comparable across backends.
   */
  std::vector<double> AdvanceIncrements(const osmscout::TextMetrics &metrics)
  {
    std::vector<double> result;

    for (size_t i=1; i<metrics.glyphs.size(); i++) {
      result.push_back(metrics.glyphs[i].position.GetX()-
                       metrics.glyphs[i-1].position.GetX());
    }

    return result;
  }
} // namespace

/**
 * The Cairo (Pango), Qt and Skia backends must measure the same text to the
 * same dimensions and lay out the glyphs on the same baseline positions
 * (label rectangle derived from the backend's measured label rectangle is
 * only consistent if the metrics match) - spec text-metrics-api, "Consistent
 * measurement across backends".
 */
TEST_CASE("Cairo, Qt and Skia measure the same label dimensions", "[TextMetricsCrossBackend]")
{
#if !defined(_WIN32)
  qputenv("QT_QPA_PLATFORM", "offscreen");

#endif

  int               argc = 1;
  char              arg0[] = "TextMetricsCrossBackendTest";
  char              * argv[1] = {arg0};
  QApplication      app(argc, argv);

  const std::string text="Musterstraße";
  auto              projection=CreateProjection();
  auto              parameter=CreateParameter();

  // --- Cairo ---
  cairo_surface_t * surface=cairo_image_surface_create(CAIRO_FORMAT_RGB24, 800, 480);

  REQUIRE(surface!=nullptr);

  cairo_t * cr=cairo_create(surface);

  REQUIRE(cr!=nullptr);

  cairo_set_source_rgb(cr, 1, 1, 1);
  cairo_paint(cr);

  osmscout::MapPainterCairo cairoPainter;

  cairoPainter.DrawMap(projection, parameter, {}, cr);

  // font size 4 with fontSizeParam 2 at 96dpi = ~30px: large enough that
  // per-glyph hinting differences do not dominate the metrics comparison
  const double fontSize=4.0;

  auto         cairoMetrics=cairoPainter.MeasureText(projection, parameter, text, fontSize);

  cairo_destroy(cr);
  cairo_surface_destroy(surface);

  // --- Qt ---
  QImage image(800, 480, QImage::Format_RGB32);

  image.fill(Qt::white);

  QPainter               qp(&image);

  osmscout::MapPainterQt qtPainter;

  qtPainter.DrawMap(projection, parameter, {}, &qp);

  auto qtMetrics=qtPainter.MeasureText(projection, parameter, text, 4.0);

  qp.end();

  // --- Skia ---
  // MapPainterSkia::MeasureText() works without a drawing context
  osmscout::MapPainterSkia skiaPainter;

  auto                     skiaMetrics=skiaPainter.MeasureText(projection, parameter, text, 4.0);

  // --- label dimensions ---
  REQUIRE_FALSE(cairoMetrics.glyphs.empty());
  REQUIRE(qtMetrics.glyphs.size()==cairoMetrics.glyphs.size());
  REQUIRE(skiaMetrics.glyphs.size()==cairoMetrics.glyphs.size());

  REQUIRE(qtMetrics.width==Catch::Approx(cairoMetrics.width).margin(0.5));
  REQUIRE(skiaMetrics.width==Catch::Approx(cairoMetrics.width).margin(0.5));
  REQUIRE(qtMetrics.height==Catch::Approx(cairoMetrics.height).margin(1.0));
  REQUIRE(skiaMetrics.height==Catch::Approx(cairoMetrics.height).margin(1.0));

  // --- glyph offsets: compare per-glyph advance increments between backends ---
  std::vector<double> cairoDeltas=AdvanceIncrements(cairoMetrics);
  std::vector<double> qtDeltas=AdvanceIncrements(qtMetrics);
  std::vector<double> skiaDeltas=AdvanceIncrements(skiaMetrics);

  REQUIRE(qtDeltas.size()==cairoDeltas.size());
  REQUIRE(skiaDeltas.size()==cairoDeltas.size());

  for (size_t i=0; i<cairoDeltas.size(); i++) {
    INFO("glyph pair " << i << " of " << cairoDeltas.size());

    REQUIRE(qtDeltas[i]==Catch::Approx(cairoDeltas[i]).margin(1.5));
    REQUIRE(skiaDeltas[i]==Catch::Approx(cairoDeltas[i]).margin(1.5));
  }
}
