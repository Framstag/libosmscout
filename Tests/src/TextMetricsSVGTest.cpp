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
#include <cmath>
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

  /**
   * The second font family the font-name cases use to trigger a change: a generic family that
   * fontconfig resolves to a different font than the bundled family on a host with fonts, and
   * that the pango-less FreeType path resolves through fontconfig as well.
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
 * A configured font file is measured as the face the file holds, without the test preparing the
 * text stack in any way (spec: font-management, "Both backends that resolve by family draw the
 * file's face").
 *
 * This case registers nothing itself: the painter alone has to make the configured file resolvable
 * to its text stack. It runs in both variants of the backend - the FreeType variant has always
 * resolved a file, so the case pins that the variant which resolves by family catches up with it.
 */
TEST_CASE("SVG measures a configured font file as the face of that file", "[TextMetricsSVG]")
{
  TextMetricsAll::ReferenceMetrics reference;
  std::string                      error;

  REQUIRE(TextMetricsAll::MeasureReference(TEXT_METRICS_FONT_PATH,
                                          ScenarioText,
                                          ScenarioFontSize,
                                          ScenarioFontSizeParam,
                                          ScenarioDpi,
                                          reference,
                                          error));

  REQUIRE(error.empty());
  REQUIRE_FALSE(reference.glyphs.empty());

  // The painter is configured with the font FILE, and the test process prepares nothing
  osmscout::MapPainterSVG painter;

  auto metrics=painter.MeasureText(CreateProjection(),
                                   CreateParameter(TEXT_METRICS_FONT_PATH),
                                   ScenarioText,
                                   ScenarioFontSize);

  auto substituted=painter.MeasureText(CreateProjection(),
                                       CreateParameter("OsmscoutNoSuchFontFamily"),
                                       ScenarioText,
                                       ScenarioFontSize);

  REQUIRE(metrics.glyphs.size()==reference.glyphs.size());

  double tolerance=3.0;

  for (size_t i=0; i<reference.glyphs.size() && i<metrics.glyphs.size(); i++) {
    REQUIRE(metrics.glyphs[i].box.width==Catch::Approx(static_cast<double>(reference.glyphs[i].width)).margin(tolerance));
    REQUIRE(metrics.glyphs[i].box.height==Catch::Approx(static_cast<double>(reference.glyphs[i].height)).margin(tolerance));
  }

  // A configured font file must not be measured with the face the host substitutes for a name it
  // cannot resolve, otherwise this case cannot tell the two apart
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
}

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

  // The painter is configured with the font file the reference was measured from. Resolving that
  // file to the face it holds is the job of the backend, so the test prepares nothing itself
  // (spec: font-dependent-test-fonts).
  const std::string fontName=TEXT_METRICS_FONT_PATH;

  osmscout::MapPainterSVG painter;

  auto                    metrics=painter.MeasureText(CreateProjection(),
                                                      CreateParameter(fontName),
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
  // The painter is configured with the font file the repository ships; resolving that file to the
  // face it holds is the job of the backend (spec: font-dependent-test-fonts)
  const std::string fontName=TEXT_METRICS_FONT_PATH;

#if defined(OSMSCOUT_MAP_SVG_HAVE_LIB_PANGO)
  INFO("SVG text stack: pango");
#else
  INFO("SVG text stack: FreeType (no pango)");

#endif

  osmscout::MapPainterSVG painter;

  auto                    metrics=painter.MeasureText(CreateProjection(),
                                                      CreateParameter(fontName),
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

/**
 * A resolved font depends on the requested font name and on the font size, so a painter kept open
 * across a font-name change must resolve a font for the new name instead of serving the font of
 * the earlier one. The painter counts the fonts it has resolved: an unchanged name and size reuse
 * their font, a new name resolves another one.
 */
TEST_CASE("SVG resolves a font for each requested font name", "[TextMetricsSVG]")
{
#if defined(HAVE_LIB_FONTCONFIG)
  // The painter is configured with the font file the repository ships; resolving that file to the
  // face it holds is the job of the backend (spec: font-dependent-test-fonts)
  const std::string fontName=TEXT_METRICS_FONT_PATH;

  osmscout::MapPainterSVG painter;

  painter.MeasureText(CreateProjection(),
                      CreateParameter(fontName),
                      ScenarioText,
                      ScenarioFontSize);

  size_t afterFirst=painter.GetResolvedFontCount();

  REQUIRE(afterFirst>=1);

  // the same font name and the same font size reuse the resolved font
  painter.MeasureText(CreateProjection(),
                      CreateParameter(fontName),
                      ScenarioText,
                      ScenarioFontSize);

  REQUIRE(painter.GetResolvedFontCount()==afterFirst);

  // another font name resolves another font instead of serving the earlier one
  painter.MeasureText(CreateProjection(),
                      CreateParameter(SecondFontFamily),
                      ScenarioText,
                      ScenarioFontSize);

  REQUIRE(painter.GetResolvedFontCount()==afterFirst+1);
#else
  INFO("this build has no fontconfig, a family resolves to no font and the case cannot count");

#endif
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
TEST_CASE("SVG measurement follows a font-name change on one painter", "[TextMetricsSVG]")
{
#if defined(HAVE_LIB_FONTCONFIG)
  // The painter is configured with the font file the repository ships; resolving that file to the
  // face it holds is the job of the backend (spec: font-dependent-test-fonts)
  const std::string fontName=TEXT_METRICS_FONT_PATH;

  // Ask fontconfig how it resolves the two configured names: the comparison below discriminates
  // only when they are not the same font. This only reads the font configuration, it does not
  // change it.
  if (ResolvedFontFile(fontName)==ResolvedFontFile(SecondFontFamily)) {
    INFO("fontconfig resolves \"" << fontName << "\" and \"" << SecondFontFamily
                                  << "\" to the same font; the font-name comparison cannot discriminate");

    return;
  }

  osmscout::MercatorProjection projection=CreateProjection();

  // a painter that only ever used the second font name
  osmscout::MapPainterSVG freshPainter;

  osmscout::TextMetrics   fresh=freshPainter.MeasureText(projection,
                                                         CreateParameter(SecondFontFamily),
                                                         ScenarioText,
                                                         ScenarioFontSize);

  // one painter that measured with the first font name and then with the second
  osmscout::MapPainterSVG livePainter;

  livePainter.MeasureText(projection,
                          CreateParameter(fontName),
                          ScenarioText,
                          ScenarioFontSize);

  osmscout::TextMetrics live=livePainter.MeasureText(projection,
                                                     CreateParameter(SecondFontFamily),
                                                     ScenarioText,
                                                     ScenarioFontSize);

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
#else
  INFO("this build has no fontconfig, two font names cannot be resolved to distinct fonts");
#endif
}
