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
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstring>
#include <limits>
#include <string>

#if defined(HAVE_LIB_FONTCONFIG)
  #include <fontconfig/fontconfig.h>
#endif

#include <osmscout/projection/MercatorProjection.h>
#include <osmscoutmap/MapParameter.h>

#include <osmscoutmapsvg/MapPainterSVG.h>

#include <TextMetricsAll.h>

#ifndef TEXT_METRICS_FONT_PATH
#define TEXT_METRICS_FONT_PATH "../libosmscout-map-opengl/data/fonts/LiberationSans-Regular.ttf"
#endif

namespace {

  /**
   * Pango baseline of the SVG backend for the label "Musterstraße 12" rendered
   * at 30.2362 px (fontSize=4, fontSizeParam=2, dpi=96): the pango build of the
   * SVG backend measures a label width of 216 px and a label height of 23 px
   * (recorded in the fix-text-metrics-shields evidence, together with the
   * FreeType based Cairo/Skia values of the same scenario).
   *
   * The FreeType variant of the SVG backend (build without pango) is compared
   * against the same value: both variants must stay within the consistency
   * margin of 10% of it, which is the pango/non-pango parity requirement. The
   * character count based approximation the backend used before measured
   * 340 px (57% over) and fails this check by a wide margin.
   */
  constexpr double  BaselineLabelWidth=216.0;
  constexpr double  BaselineLabelHeight=23.0;
  constexpr double  BaselineMargin=0.10;

  constexpr double  ScenarioFontSize=4.0;
  constexpr double  ScenarioFontSizeParam=2.0;
  constexpr double  ScenarioDpi=96.0;

  const std::string ScenarioText="Musterstraße 12";

  osmscout::MercatorProjection CreateProjection()
  {
    osmscout::MercatorProjection projection;

    projection.Set(osmscout::GeoCoord(50.107252570499767, 14.459053009732296),
                   0.0,
                   osmscout::Magnification(osmscout::Magnification::magClose),
                   ScenarioDpi,
                   800,
                   480);

    return projection;
  }

  osmscout::MapParameter CreateParameter(const std::string& fontFamily)
  {
    osmscout::MapParameter parameter;

    parameter.SetFontSize(ScenarioFontSizeParam);
    // measure with the same font that the FreeType reference loads from file;
    // the family name is the name stored inside the font file, not its file name
    parameter.SetFontName(fontFamily);

    return parameter;
  }

  /**
   * Union of the per-glyph ink boxes, offset by their glyph positions, as
   * required by the text-metrics-api contract.
   */
  void LabelUnion(const osmscout::TextMetrics& metrics,
                  double& width,
                  double& height)
  {
    double minX=std::numeric_limits<double>::max();
    double maxX=std::numeric_limits<double>::lowest();
    double minY=std::numeric_limits<double>::max();
    double maxY=std::numeric_limits<double>::lowest();

    for (const auto& glyph : metrics.glyphs) {
      double x1=glyph.position.GetX()+glyph.box.x;
      double y1=glyph.position.GetY()+glyph.box.y;
      double x2=x1+glyph.box.width;
      double y2=y1+glyph.box.height;

      minX=std::min(minX,x1);
      minY=std::min(minY,y1);
      maxX=std::max(maxX,x2);
      maxY=std::max(maxY,y2);
    }

    width=maxX-minX;
    height=maxY-minY;
  }
} // namespace

/**
 * Make the bundled font file known to the platform font resolution, so that the
 * font family of the font file resolves to the exact font even if it is not
 * installed system-wide: fontconfig application font for the FreeType variant
 * of the backend and for pangoft2 (which resolves through fontconfig on Linux)
 * of the pango variant.
 */
TEST_CASE("SVG measurement matches the FreeType reference", "[TextMetricsSVG]")
{
  TextMetricsAll::ReferenceMetrics reference;
  std::string                      error;

  bool                             ok=TextMetricsAll::MeasureReference(TEXT_METRICS_FONT_PATH,
                                                                       ScenarioText,
                                                                       ScenarioFontSize,
                                                                       ScenarioFontSizeParam,
                                                                       ScenarioDpi,
                                                                       reference,
                                                                       error);

  REQUIRE(ok);
  REQUIRE(error.empty());
  REQUIRE_FALSE(reference.glyphs.empty());

  std::string fontFamily;

  REQUIRE(TextMetricsAll::ReferenceFontFamily(TEXT_METRICS_FONT_PATH,
                                              fontFamily,
                                              error));
  REQUIRE(error.empty());

#if defined(HAVE_LIB_FONTCONFIG)
  bool appFontAdded=FcConfigAppFontAddFile(nullptr,
                                           reinterpret_cast<const FcChar8*>(TEXT_METRICS_FONT_PATH));

  if (!appFontAdded) {
    INFO("Cannot register font \"" << TEXT_METRICS_FONT_PATH << "\" as fontconfig application font");
  }
#endif

  osmscout::MapPainterSVG painter;

  auto                    metrics=painter.MeasureText(CreateProjection(),
                                                      CreateParameter(fontFamily),
                                                      ScenarioText,
                                                      ScenarioFontSize);

  const auto & labels=reference.glyphs;

  REQUIRE(metrics.glyphs.size()==labels.size());

#if defined(OSMSCOUT_MAP_SVG_HAVE_LIB_PANGO)
  // The pango text stack applies shaping and its own rounding, so individual
  // glyph boxes may differ from the FreeType reference by a few pixels (see
  // TextMetricsCairoTest), the ink semantics is what is verified here
  double tolerance=3.0;

#else
  // The pango-less variant measures with FreeType itself, so its glyph boxes
  // must match the FreeType reference exactly up to rounding
  double tolerance=1.0;

#endif

  double previousX=0.0;

  for (size_t i=0; i<labels.size() && i<metrics.glyphs.size(); i++) {
    REQUIRE(metrics.glyphs[i].position.GetX()==
            Catch::Approx(static_cast<double>(previousX)).margin(tolerance*i+tolerance));

    REQUIRE(metrics.glyphs[i].box.x==Catch::Approx(static_cast<double>(labels[i].x)).margin(tolerance));
    REQUIRE(metrics.glyphs[i].box.y==Catch::Approx(static_cast<double>(labels[i].y)).margin(tolerance));
    REQUIRE(metrics.glyphs[i].box.width==Catch::Approx(static_cast<double>(labels[i].width)).margin(tolerance));
    REQUIRE(metrics.glyphs[i].box.height==Catch::Approx(static_cast<double>(labels[i].height)).margin(tolerance));

    previousX+=labels[i].advance;
  }
}

/**
 * The label dimensions of both SVG text stacks must equal the union of the
 * per-glyph ink boxes and stay within the consistency margin of the pango
 * baseline: a build with pango and a build without pango therefore measure the
 * same text consistently, and the character count based approximation that was
 * used without pango (340 px instead of 216 px) is detected as drift.
 */
TEST_CASE("SVG label dimensions match the pango baseline", "[TextMetricsSVG]")
{
  std::string fontFamily;
  std::string error;

  REQUIRE(TextMetricsAll::ReferenceFontFamily(TEXT_METRICS_FONT_PATH,
                                              fontFamily,
                                              error));
  REQUIRE(error.empty());

#if defined(HAVE_LIB_FONTCONFIG)
  FcConfigAppFontAddFile(nullptr,
                         reinterpret_cast<const FcChar8*>(TEXT_METRICS_FONT_PATH));
#endif

#if defined(OSMSCOUT_MAP_SVG_HAVE_LIB_PANGO)
  INFO("SVG text stack: pango");
#else
  INFO("SVG text stack: FreeType (no pango)");

#endif

  osmscout::MapPainterSVG painter;

  auto                    metrics=painter.MeasureText(CreateProjection(),
                                                      CreateParameter(fontFamily),
                                                      ScenarioText,
                                                      ScenarioFontSize);

  REQUIRE(metrics.width==Catch::Approx(BaselineLabelWidth).margin(BaselineLabelWidth*BaselineMargin));
  REQUIRE(metrics.height==Catch::Approx(BaselineLabelHeight).margin(BaselineLabelHeight*BaselineMargin));

  // label rectangle equals the union of the glyph boxes (text-metrics-api)
  double unionWidth=0.0;
  double unionHeight=0.0;

  LabelUnion(metrics,
             unionWidth,
             unionHeight);

#if defined(OSMSCOUT_MAP_SVG_HAVE_LIB_PANGO)
  // The pango text stack derives the label ink extents from its own shaping and
  // its per-glyph boxes from the glyph ink extents; both disagree by about one
  // pixel more than the union (pre-existing; the pango path is unchanged here)
  double unionTolerance=2.0;

#else
  double unionTolerance=1.0;
#endif

  REQUIRE(metrics.width==Catch::Approx(unionWidth).margin(unionTolerance));
  REQUIRE(metrics.height==Catch::Approx(unionHeight).margin(unionTolerance));
}
