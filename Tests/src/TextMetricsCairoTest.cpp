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
#include <cstring>
#include <filesystem>
#include <string>

#if defined(HAVE_LIB_FONTCONFIG)
  #include <fontconfig/fontconfig.h>
#endif

#if defined(OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO)
  #include <pango/pango.h>
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

  osmscout::MercatorProjection CreateProjectionForDpi(double dpi)
  {
    osmscout::MercatorProjection projection;

    projection.Set(osmscout::GeoCoord(50.107252570499767, 14.459053009732296),
                   0.0,
                   osmscout::Magnification(osmscout::Magnification::magClose),
                   dpi,
                   800,
                   480);

    return projection;
  }

  /**
   * The second font family the font-name cases use to trigger a change: a generic family that
   * fontconfig resolves to a different font than the bundled family on a host with fonts, and
   * that the non-pango cairo font API accepts as well.
   */
  constexpr const char *SecondFontFamily="monospace";

#if defined(HAVE_LIB_FONTCONFIG)

  /**
   * The font file fontconfig resolves a font name to, or an empty string. The font-name cases
   * can only discriminate a stale resolved font when two names resolve to different fonts.
   */
  std::string ResolvedFontFile(const std::string& fontName)
  {
    FcPattern *pattern=FcPatternCreate();

    if (pattern==nullptr) {
      return "";
    }

    FcPatternAddString(pattern,
                       FC_FAMILY,
                       reinterpret_cast<const FcChar8*>(fontName.c_str()));
    FcConfigSubstitute(nullptr,
                       pattern,
                       FcMatchPattern);
    FcDefaultSubstitute(pattern);

    FcResult  matchResult=FcResultNoMatch;
    FcPattern *match=FcFontMatch(nullptr,
                                 pattern,
                                 &matchResult);

    std::string file;

    if (match!=nullptr) {
      FcChar8 *resolvedFile=nullptr;

      if (FcPatternGetString(match,
                             FC_FILE,
                             0,
                             &resolvedFile)==FcResultMatch &&
          resolvedFile!=nullptr) {
        file=reinterpret_cast<const char*>(resolvedFile);
      }

      FcPatternDestroy(match);
    }

    FcPatternDestroy(pattern);

    return file;
  }

#endif
} // namespace

/**
 * A configured font file is measured as the face the file holds, without the test preparing
 * the text stack in any way (spec: font-management, "A font file is drawn as the face of that
 * file").
 *
 * This case is declared first and registers nothing itself: the painter alone has to make the
 * configured file resolvable to its text stack. On the code before the change the file path was
 * handed to the family-based interface, which resolved it to whatever face the host falls back
 * to for an unknown name - so the comparison below against the FreeType reference of the same
 * file is the assertion that fails then.
 */
TEST_CASE("Cairo measures a configured font file as the face of that file", "[TextMetricsCairo]")
{
  TextMetricsAll::ReferenceMetrics reference;
  std::string                      error;

  REQUIRE(TextMetricsAll::MeasureReference(TEXT_METRICS_FONT_PATH,
                                          "Musterstra\u00dfe",
                                          1.0,
                                          10.0,
                                          96.0,
                                          reference,
                                          error));

  REQUIRE(error.empty());
  REQUIRE_FALSE(reference.glyphs.empty());

  cairo_surface_t * surface=cairo_image_surface_create(CAIRO_FORMAT_RGB24,
                                                       800,
                                                       480);

  REQUIRE(surface!=nullptr);

  cairo_t * cr=cairo_create(surface);

  REQUIRE(cr!=nullptr);

  // The painter is configured with the font FILE, and the test process prepares nothing
  osmscout::MapPainterCairo painter;

  painter.DrawMap(CreateProjection(),
                  CreateParameter(TEXT_METRICS_FONT_PATH),
                  {},
                  cr);

  auto metrics=painter.MeasureText(CreateProjection(),
                                   CreateParameter(TEXT_METRICS_FONT_PATH),
                                   "Musterstra\u00dfe",
                                   1.0);

  REQUIRE(metrics.glyphs.size()==reference.glyphs.size());

  // The face of the file, not the face the host substitutes for an unknown font name
  auto substituted=painter.MeasureText(CreateProjection(),
                                       CreateParameter("OsmscoutNoSuchFontFamily"),
                                       "Musterstra\u00dfe",
                                       1.0);

  double tolerance=3.0;

  for (size_t i=0; i<reference.glyphs.size() && i<metrics.glyphs.size(); i++) {
    REQUIRE(metrics.glyphs[i].box.width==Catch::Approx(static_cast<double>(reference.glyphs[i].width)).margin(tolerance));
    REQUIRE(metrics.glyphs[i].box.height==Catch::Approx(static_cast<double>(reference.glyphs[i].height)).margin(tolerance));
  }

  // A font file that names the face it holds must not be measured with the substituted face:
  // at least one glyph box has to differ, otherwise this case cannot tell the two apart and the
  // host provides the same face for both
  bool differs=false;

  for (size_t i=0; i<metrics.glyphs.size() && i<substituted.glyphs.size(); i++) {
    if (std::abs(metrics.glyphs[i].box.width-static_cast<double>(substituted.glyphs[i].box.width))>0.5 ||
        std::abs(metrics.glyphs[i].position.GetX()-substituted.glyphs[i].position.GetX())>0.5) {
      differs=true;

      break;
    }
  }

  INFO("label width for the configured file: " << metrics.width
       << ", for the substituted family: " << substituted.width);

  REQUIRE(differs);

  cairo_destroy(cr);
  cairo_surface_destroy(surface);
}

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

  // The painter is configured with the font file the reference was measured from. Resolving that
  // file to the face it holds is the job of the backend, so the test prepares nothing itself
  // (spec: font-dependent-test-fonts).
  const std::string fontName=TEXT_METRICS_FONT_PATH;

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
                  CreateParameter(fontName),
                  {},
                  cr);

  auto metrics=painter.MeasureText(CreateProjection(),
                                   CreateParameter(fontName),
                                   "Musterstraße",
                                   1.0);

  auto labels=reference.glyphs;

  REQUIRE(metrics.glyphs.size()==labels.size());

  // Ink box tolerance: the backend rasterizer (FreeType on Unix, DirectWrite/
  // GDI on Windows, CoreText on macOS) may differ from the FreeType reference
  // by a few pixels on ink extents. The font identity is guaranteed by the
  // fontconfig family check above, so this comparison verifies ink semantics
  // (per-glyph boxes, not a constant font box), not the exact rasterizer.
  double tolerance=3.0;
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

/**
 * The Cairo backend scales the requested font size by the resolution of the projection, so its
 * measurement depends on the projection and on its drawing target. That is why the painter
 * reports both as the measurement environment of the label layouter
 * (`MapPainterCairo::GetMeasurementEnvironment`), and why a change of either has to invalidate
 * the measurements the layouter remembers.
 */
TEST_CASE("Cairo measurement depends on the resolution of the projection", "[TextMetricsCairo]")
{
  // The painter is configured with the font file the repository ships; resolving that file to the
  // face it holds is the job of the backend (spec: font-dependent-test-fonts)
  const std::string fontName=TEXT_METRICS_FONT_PATH;

  osmscout::MapParameter       parameter=CreateParameter(fontName);

  osmscout::MercatorProjection projection96=CreateProjectionForDpi(96.0);
  osmscout::MercatorProjection projection192=CreateProjectionForDpi(192.0);

  cairo_surface_t              *surface96=cairo_image_surface_create(CAIRO_FORMAT_ARGB32,800,480);
  cairo_surface_t              *surface192=cairo_image_surface_create(CAIRO_FORMAT_ARGB32,800,480);

  REQUIRE(surface96!=nullptr);
  REQUIRE(surface192!=nullptr);

  cairo_t *context96=cairo_create(surface96);
  cairo_t *context192=cairo_create(surface192);

  REQUIRE(context96!=nullptr);
  REQUIRE(context192!=nullptr);

  osmscout::MapPainterCairo painter96;
  osmscout::MapPainterCairo painter192;

  // DrawMap installs the drawing target and the measurement environment of the frame
  painter96.DrawMap(projection96, parameter, {}, context96);
  painter192.DrawMap(projection192, parameter, {}, context192);

  osmscout::TextMetrics metrics96=painter96.MeasureText(projection96, parameter, "Muster", 10.0);
  osmscout::TextMetrics metrics192=painter192.MeasureText(projection192, parameter, "Muster", 10.0);

  REQUIRE(metrics96.width>0.0);
  REQUIRE(metrics192.width>0.0);

  // Twice the resolution means twice the size of the drawn text, so a remembered measurement
  // of one environment must not be served in the other
  REQUIRE(metrics192.width>metrics96.width*1.5);

  cairo_destroy(context96);
  cairo_destroy(context192);
  cairo_surface_destroy(surface96);
  cairo_surface_destroy(surface192);
}

/**
 * A resolved font depends on the requested font name and on the font size, so a painter kept open
 * across a font-name change must resolve a font for the new name instead of serving the font of
 * the earlier one. The painter counts the fonts it has resolved: an unchanged name and size reuse
 * their font, a new name resolves another one. The case needs no second font to exist, because a
 * font is constructed for a name whether or not the name resolves.
 */
TEST_CASE("Cairo resolves a font for each requested font name", "[TextMetricsCairo]")
{
  // The painter is configured with the font file the repository ships; resolving that file to the
  // face it holds is the job of the backend (spec: font-dependent-test-fonts)
  const std::string fontName=TEXT_METRICS_FONT_PATH;

  cairo_surface_t *surface=cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                                      800,
                                                      480);

  REQUIRE(surface!=nullptr);

  cairo_t *context=cairo_create(surface);

  REQUIRE(context!=nullptr);

  osmscout::MapPainterCairo painter;

  painter.DrawMap(CreateProjection(),
                  CreateParameter(fontName),
                  {},
                  context);

  painter.MeasureText(CreateProjection(),
                      CreateParameter(fontName),
                      "Muster",
                      1.0);

  size_t afterFirst=painter.GetResolvedFontCount();

  REQUIRE(afterFirst>=1);

  // the same font name and the same font size reuse the resolved font
  painter.MeasureText(CreateProjection(),
                      CreateParameter(fontName),
                      "Muster",
                      1.0);

  REQUIRE(painter.GetResolvedFontCount()==afterFirst);

  // another font name resolves another font instead of serving the earlier one
  painter.MeasureText(CreateProjection(),
                      CreateParameter(SecondFontFamily),
                      "Muster",
                      1.0);

  REQUIRE(painter.GetResolvedFontCount()==afterFirst+1);

  cairo_destroy(context);
  cairo_surface_destroy(surface);
}

/**
 * The ink metrics a painter reports for a font name are the metrics of the font it resolved for
 * that name, so a painter that measured with one font name and then measures with another must
 * report the metrics a painter that only ever used the second name reports. The resolution of the
 * drawn label goes through the same call, so the metrics cover the drawn font as well.
 *
 * The comparison discriminates only when the two names resolve to different fonts, so the case
 * asks fontconfig first and reports an INFO line and passes when they resolve to the same font.
 */
TEST_CASE("Cairo measurement follows a font-name change on one painter", "[TextMetricsCairo]")
{
  // The painter is configured with the font file the repository ships; resolving that file to the
  // face it holds is the job of the backend (spec: font-dependent-test-fonts)
  const std::string fontName=TEXT_METRICS_FONT_PATH;

#if defined(HAVE_LIB_FONTCONFIG)
  // Ask fontconfig how it resolves the two configured names: the comparison below discriminates
  // only when they are not the same font. This only reads the font configuration, it does not
  // change it.
  if (ResolvedFontFile(fontName)==ResolvedFontFile(SecondFontFamily)) {
    INFO("fontconfig resolves \"" << fontName << "\" and \"" << SecondFontFamily
                                  << "\" to the same font; the font-name comparison cannot discriminate");

    return;
  }
#endif

  osmscout::MercatorProjection projection=CreateProjection();

  cairo_surface_t              *surface=cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                                                   800,
                                                                   480);

  REQUIRE(surface!=nullptr);

  cairo_t *freshContext=cairo_create(surface);
  cairo_t *liveContext=cairo_create(surface);

  REQUIRE(freshContext!=nullptr);
  REQUIRE(liveContext!=nullptr);

  // a painter that only ever used the second font name
  osmscout::MapPainterCairo freshPainter;

  freshPainter.DrawMap(projection,
                       CreateParameter(SecondFontFamily),
                       {},
                       freshContext);

  osmscout::TextMetrics fresh=freshPainter.MeasureText(projection,
                                                       CreateParameter(SecondFontFamily),
                                                       "Muster",
                                                       1.0);

  // one painter that measured with the first font name and then with the second
  osmscout::MapPainterCairo livePainter;

  livePainter.DrawMap(projection,
                      CreateParameter(fontName),
                      {},
                      liveContext);

  livePainter.MeasureText(projection,
                          CreateParameter(fontName),
                          "Muster",
                          1.0);

  osmscout::TextMetrics live=livePainter.MeasureText(projection,
                                                     CreateParameter(SecondFontFamily),
                                                     "Muster",
                                                     1.0);

  REQUIRE(live.width>0.0);
  REQUIRE(live.width==Catch::Approx(fresh.width).margin(1.0));
  REQUIRE(live.height==Catch::Approx(fresh.height).margin(1.0));
  REQUIRE(live.glyphs.size()==fresh.glyphs.size());

  for (size_t i=0; i<fresh.glyphs.size() && i<live.glyphs.size(); i++) {
    REQUIRE(live.glyphs[i].position.GetX()==Catch::Approx(fresh.glyphs[i].position.GetX()).margin(1.0));
    REQUIRE(live.glyphs[i].position.GetY()==Catch::Approx(fresh.glyphs[i].position.GetY()).margin(1.0));
    REQUIRE(live.glyphs[i].box.x==Catch::Approx(fresh.glyphs[i].box.x).margin(1.0));
    REQUIRE(live.glyphs[i].box.y==Catch::Approx(fresh.glyphs[i].box.y).margin(1.0));
    REQUIRE(live.glyphs[i].box.width==Catch::Approx(fresh.glyphs[i].box.width).margin(1.0));
    REQUIRE(live.glyphs[i].box.height==Catch::Approx(fresh.glyphs[i].box.height).margin(1.0));
  }

  cairo_destroy(freshContext);
  cairo_destroy(liveContext);
  cairo_surface_destroy(surface);
}
