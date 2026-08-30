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

#include <filesystem>
#include <string>

#if defined(HAVE_LIB_FONTCONFIG)
  #include <fontconfig/fontconfig.h>
#endif

#if defined(__WIN32__) || defined(WIN32) || (defined(__APPLE__) && __APPLE__)
  #include <cairo.h>
#else
  #include <cairo/cairo.h>
#endif

#include <osmscout/projection/MercatorProjection.h>
#include <osmscoutmap/MapParameter.h>

#include <osmscoutmapcairo/MapPainterCairo.h>

#include <TextMetricsAll.h>

#ifndef TEXT_METRICS_FONT_PATH
#define TEXT_METRICS_FONT_PATH "../libosmscout-map-opengl/data/fonts/LiberationSans-Regular.ttf"
#endif

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

  osmscout::MapParameter CreateParameter(const std::string& fontFamily)
  {
    osmscout::MapParameter parameter;

    parameter.SetFontSize(10.0);
    // measure with the same font that the FreeType reference loads from file;
    // the family name is the name stored inside the font file, not its file name
    parameter.SetFontName(fontFamily);

    return parameter;
  }
} // namespace

/**
 * The Cairo backend (Pango text stack) must measure the same text like the
 * FreeType reference: per-glyph ink bounding boxes, glyph positions and the
 * ink width and height of the label.
 */
TEST_CASE("Cairo measurement matches the FreeType reference", "[TextMetricsCairo]")
{
  TextMetricsAll::ReferenceMetrics reference;
  std::string                      error;

  bool                             ok=TextMetricsAll::MeasureReference(TEXT_METRICS_FONT_PATH,
                                                                       "Musterstraße",
                                                                       1.0,
                                                                       10.0,
                                                                       96.0,
                                                                       reference,
                                                                       error);

  REQUIRE(ok);
  REQUIRE(error.empty());
  REQUIRE_FALSE(reference.glyphs.empty());

  // Resolve the family name stored inside the font file. The cairo backend
  // resolves fonts by family name through fontconfig; asking for the file
  // name (which is not a family name) silently substitutes another font on
  // systems without the font installed.
  std::string fontFamily;

  REQUIRE(TextMetricsAll::ReferenceFontFamily(TEXT_METRICS_FONT_PATH,
                                              fontFamily,
                                              error));
  REQUIRE(error.empty());

#if defined(HAVE_LIB_FONTCONFIG)
  // Make the bundled font file visible to fontconfig so that fontconfig
  // (and thus the Pango/cairo font resolution) can find the exact font even
  // if the font is not installed system-wide
  REQUIRE(FcConfigAppFontAddFile(nullptr,
                                 reinterpret_cast<const FcChar8*>(TEXT_METRICS_FONT_PATH)));
#endif

  cairo_surface_t * surface=cairo_image_surface_create(CAIRO_FORMAT_RGB24,
                                                       800,
                                                       480);

  REQUIRE(surface!=nullptr);

  cairo_t * cr=cairo_create(surface);

  REQUIRE(cr!=nullptr);

  cairo_set_source_rgb(cr,1,1,1);
  cairo_paint(cr);

  osmscout::MapPainterCairo painter;

  // DrawMap sets the internal cairo context used by Layout()/MeasureText()
  painter.DrawMap(CreateProjection(),
                  CreateParameter(fontFamily),
                  {},
                  cr);

  auto metrics=painter.MeasureText(CreateProjection(),
                                   CreateParameter(fontFamily),
                                   "Musterstraße",
                                   1.0);

  auto labels=reference.glyphs;

  REQUIRE(metrics.glyphs.size()==labels.size());

  double tolerance=2.0;
  double previousX=0.0;

  for (size_t i=0; i<labels.size() && i<metrics.glyphs.size(); i++) {
    // glyph origins: the backend must lay out the glyphs like the FreeType
    // reference: first glyph at the baseline start, increments like the
    // reference advances (small deviations from hinting are tolerated)
    REQUIRE(metrics.glyphs[i].position.GetX()==
            Catch::Approx(static_cast<double>(previousX)).margin(tolerance*i+tolerance));

    // the ink bounding box must match the reference ink box
    REQUIRE(metrics.glyphs[i].box.x==Catch::Approx(static_cast<double>(labels[i].x)).margin(tolerance));
    REQUIRE(metrics.glyphs[i].box.y==Catch::Approx(static_cast<double>(labels[i].y)).margin(tolerance));
    REQUIRE(metrics.glyphs[i].box.width==Catch::Approx(static_cast<double>(labels[i].width)).margin(tolerance));
    REQUIRE(metrics.glyphs[i].box.height==Catch::Approx(static_cast<double>(labels[i].height)).margin(tolerance));

    previousX+=labels[i].advance;
  }

  cairo_destroy(cr);
  cairo_surface_destroy(surface);
}
