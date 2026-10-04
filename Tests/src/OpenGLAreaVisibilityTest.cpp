/*
  This source is part of the libosmscout-map-opengl library
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

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <memory>
#include <vector>

#include <osmscout/Area.h>
#include <osmscout/Pixel.h>
#include <osmscout/TypeConfig.h>
#include <osmscout/TypeInfoSet.h>
#include <osmscout/projection/MercatorProjection.h>
#include <osmscout/util/GeoBox.h>
#include <osmscout/util/Magnification.h>

#include <osmscoutmap/AreaBorderReach.h>
#include <osmscoutmap/MapData.h>
#include <osmscoutmap/MapParameter.h>
#include <osmscoutmap/StyleConfig.h>
#include <osmscoutmap/Styles.h>

#include <osmscoutmapopengl/AreaVisibility.h>
#include <osmscoutmapopengl/MapPainterOpenGL.h>

#include <GLFW/glfw3.h>

/*
 * Tests for the OpenGL backend's area visibility decision: the tolerance of the decision is a
 * screen-space length of the frame, so an area whose border still crosses the viewport edge is kept,
 * and one beyond that tolerance keeps contributing nothing. The tolerance itself is computed from the
 * border styles the ring resolves by GetAreaRingTolerancePixel in libosmscout-map, which the OpenGL
 * area step shares with the painters of that library; the cases exercise the decision with a
 * tolerance in pixels and cover the conversion of the helper for the two DPIs.
 *
 * The decision is a free function, so most cases need neither a database, a style sheet nor a GL
 * context. Areas are placed relative to the viewport using pixel distances measured from the
 * projection, so the tests do not depend on the absolute scale of a magnification level.
 */

namespace {

  /**
   * The DPI the fixed viewport of this file is projected with and the lower DPI the test of the DPI
   * dependence compares it with.
   */
  constexpr double referenceDpi=300.0;
  constexpr double lowerDpi=96.0;

  /**
   * Side of the fixed viewport, in pixels, on both axes.
   */
  constexpr size_t viewportSize=400;

  /**
   * Projection of the fixed viewport, set with the DPI first and the image dimensions second, so
   * the viewport is 400x400 pixels at the given DPI.
   */
  osmscout::MercatorProjection MakeProjectionWithDpi(double dpi)
  {
    osmscout::MercatorProjection projection;

    REQUIRE(projection.Set(osmscout::GeoCoord(50.001,8.001),
                           osmscout::Magnification(osmscout::Magnification::magClose),
                           dpi,
                           viewportSize,
                           viewportSize));

    return projection;
  }

  /**
   * Pixels per degree of longitude at the latitude of the projection center. The horizontal scale of
   * a Mercator projection does not change with the latitude, so a distance derived with this value
   * holds for every latitude of the view.
   */
  double PixelsPerDegreeLon(const osmscout::MercatorProjection& projection)
  {
    constexpr double probeDeltaDegrees=0.001;

    osmscout::Vertex2D center{};
    osmscout::Vertex2D east{};

    REQUIRE(projection.GeoToPixel(projection.GetCenter(),center));
    REQUIRE(projection.GeoToPixel(osmscout::GeoCoord(projection.GetCenter().GetLat(),
                                                     projection.GetCenter().GetLon()+probeDeltaDegrees),
                                  east));

    double scale=(east.GetX()-center.GetX())/probeDeltaDegrees;

    REQUIRE(scale>0.0);

    return scale;
  }

  /**
   * Degrees of longitude that correspond to the given number of pixels.
   */
  double DegreesLonForPixels(const osmscout::MercatorProjection& projection,
                             double pixels)
  {
    return pixels/PixelsPerDegreeLon(projection);
  }

  /**
   * Pixels per degree of latitude at the latitude of the projection center.
   */
  double PixelsPerDegreeLat(const osmscout::MercatorProjection& projection)
  {
    constexpr double probeDeltaDegrees=0.001;

    osmscout::Vertex2D center{};
    osmscout::Vertex2D north{};

    REQUIRE(projection.GeoToPixel(projection.GetCenter(),center));
    REQUIRE(projection.GeoToPixel(osmscout::GeoCoord(projection.GetCenter().GetLat()+probeDeltaDegrees,
                                                     projection.GetCenter().GetLon()),
                                  north));

    // Screen y grows southwards, so the more northern probe is above the center
    double scale=(center.GetY()-north.GetY())/probeDeltaDegrees;

    REQUIRE(scale>0.0);

    return scale;
  }

  /**
   * Degrees of latitude that correspond to the given number of pixels.
   */
  double DegreesLatForPixels(const osmscout::MercatorProjection& projection,
                             double pixels)
  {
    return pixels/PixelsPerDegreeLat(projection);
  }

  /**
   * A box centered on the center of the view, with the given half size in pixels on both axes.
   */
  osmscout::GeoBox BoxInView(const osmscout::MercatorProjection& projection,
                             double halfSizeLonPx,
                             double halfSizeLatPx)
  {
    osmscout::GeoCoord center=projection.GetCenter();

    double halfSizeLon=DegreesLonForPixels(projection,halfSizeLonPx);
    double halfSizeLat=DegreesLatForPixels(projection,halfSizeLatPx);

    return osmscout::GeoBox(osmscout::GeoCoord(center.GetLat()-halfSizeLat,
                                               center.GetLon()-halfSizeLon),
                            osmscout::GeoCoord(center.GetLat()+halfSizeLat,
                                               center.GetLon()+halfSizeLon));
  }

  /**
   * A box whose western edge lies distancePx pixels east of the eastern edge of the view, with the
   * latitude extent of BoxInView.
   */
  osmscout::GeoBox BoxEastOfView(const osmscout::MercatorProjection& projection,
                                 double distancePx,
                                 double halfSizePx)
  {
    osmscout::GeoBox dimensions=projection.GetDimensions();
    osmscout::GeoCoord center=projection.GetCenter();

    double halfSizeLon=DegreesLonForPixels(projection,halfSizePx);
    double halfSizeLat=(dimensions.GetMaxLat()-dimensions.GetMinLat())/4.0;
    double westLon=dimensions.GetMaxLon()+DegreesLonForPixels(projection,distancePx);

    return osmscout::GeoBox(osmscout::GeoCoord(center.GetLat()-halfSizeLat,
                                               westLon),
                            osmscout::GeoCoord(center.GetLat()+halfSizeLat,
                                               westLon+2.0*halfSizeLon));
  }

  /**
   * The area type the cases that need a painter load, and a style sheet that draws it by a fill and
   * a border of the given width and offset. A border is needed because the tolerance of the visibility
   * decision is derived from it.
   */
  osmscout::StyleConfigRef MakeStyleConfig(const osmscout::TypeConfigRef& typeConfig,
                                           const osmscout::TypeInfoRef& areaType,
                                           double borderWidthMM,
                                           double borderOffsetMapUnits=0.0)
  {
    osmscout::StyleConfigRef styleConfig=std::make_shared<osmscout::StyleConfig>(typeConfig);

    osmscout::TypeInfoSet areaTypes(*typeConfig);

    areaTypes.Set(areaType);

    osmscout::StyleFilter areaFilter;

    areaFilter.SetTypes(areaTypes);

    osmscout::FillPartialStyle fillStyle;

    fillStyle.SetColorValue(osmscout::FillStyle::attrFillColor,osmscout::Color(0.0,1.0,0.0));
    styleConfig->AddAreaFillStyle(areaFilter,fillStyle);

    osmscout::StyleFilter borderFilter;

    borderFilter.SetTypes(areaTypes);
    borderFilter.SetMinLevel(0);
    borderFilter.SetMaxLevel(25);

    osmscout::BorderPartialStyle borderStyle;

    borderStyle.SetDoubleValue(osmscout::BorderStyle::attrWidth,borderWidthMM);
    borderStyle.SetDoubleValue(osmscout::BorderStyle::attrOffset,borderOffsetMapUnits);
    borderStyle.SetColorValue(osmscout::BorderStyle::attrColor,osmscout::Color(1.0,0.0,0.0));
    styleConfig->AddAreaBorderStyle(borderFilter,borderStyle);

    styleConfig->Postprocess();

    return styleConfig;
  }

  /**
   * Area with one outer ring that is an axis parallel rectangle covering the given box.
   */
  osmscout::AreaRef MakeArea(const osmscout::TypeInfoRef& type,
                             const osmscout::GeoBox& box)
  {
    auto area=std::make_shared<osmscout::Area>();

    osmscout::Area::Ring ring;

    ring.MarkAsOuterRing();
    ring.SetType(type);
    ring.nodes.push_back(osmscout::Point(1,osmscout::GeoCoord(box.GetMinLat(),box.GetMinLon())));
    ring.nodes.push_back(osmscout::Point(2,osmscout::GeoCoord(box.GetMinLat(),box.GetMaxLon())));
    ring.nodes.push_back(osmscout::Point(3,osmscout::GeoCoord(box.GetMaxLat(),box.GetMaxLon())));
    ring.nodes.push_back(osmscout::Point(4,osmscout::GeoCoord(box.GetMaxLat(),box.GetMinLon())));
    ring.center=box.GetCenter();

    area->rings.push_back(ring);

    return area;
  }
}

/**
 * The scenario "a painter that owns its decision keeps the border of an area crossing the edge": an
 * area whose border still crosses the viewport edge contributes to the frame, and one further out
 * than the tolerance contributes nothing. The tolerance is the one GetAreaRingTolerancePixel returns
 * for the declared border, i.e. it is a screen-space length of the frame.
 */
TEST_CASE("An area within the converted border tolerance is kept")
{
  constexpr double borderWidthMM=10.0;
  constexpr double halfSizePx=50.0;

  for (double dpi: {lowerDpi,referenceDpi}) {
    osmscout::MercatorProjection projection=MakeProjectionWithDpi(dpi);

    double tolerancePx=projection.ConvertWidthToPixel(borderWidthMM/2.0);

    REQUIRE(tolerancePx>1.0);

    REQUIRE(osmscout::IsAreaRingVisible(projection,
                                        BoxEastOfView(projection,0.4*tolerancePx,halfSizePx),
                                        tolerancePx,
                                        0.0));

    REQUIRE_FALSE(osmscout::IsAreaRingVisible(projection,
                                              BoxEastOfView(projection,1.5*tolerancePx,halfSizePx),
                                              tolerancePx,
                                              0.0));
  }
}

/**
 * The tolerance of the decision is a screen-space length: the helper that computes it converts the
 * width a style sheet declares with the projection of the frame, so the same border style reaches
 * further at the higher DPI, and the decision keeps a ring that only the higher DPI reaches (spec
 * map-painter-area-culling, requirement "Every painter applies a stylesheet-derived tolerance in the
 * frame's pixels", scenario "The tolerance of such a painter follows the DPI of the frame").
 */
TEST_CASE("The tolerance of the decision follows the DPI of the frame")
{
  constexpr double             borderWidthMM=10.0;
  constexpr double             halfSizePx=50.0;

  osmscout::MercatorProjection projection96=MakeProjectionWithDpi(lowerDpi);
  osmscout::MercatorProjection projection300=MakeProjectionWithDpi(referenceDpi);

  osmscout::BorderStyleRef     borderStyle=std::make_shared<osmscout::BorderStyle>();

  borderStyle->SetWidth(borderWidthMM);

  double tolerance96=osmscout::GetAreaRingTolerancePixel(projection96,{borderStyle});
  double tolerance300=osmscout::GetAreaRingTolerancePixel(projection300,{borderStyle});

  REQUIRE(tolerance300>tolerance96);

  double distancePx=(tolerance96+tolerance300)/2.0;

  REQUIRE_FALSE(osmscout::IsAreaRingVisible(projection96,
                                            BoxEastOfView(projection96,distancePx,halfSizePx),
                                            tolerance96,
                                            0.0));

  REQUIRE(osmscout::IsAreaRingVisible(projection300,
                                      BoxEastOfView(projection300,distancePx,halfSizePx),
                                      tolerance300,
                                      0.0));
}

/**
 * A ring whose border is drawn at an offset reaches further out than half of the width of that
 * border, so a tolerance that covers only the width rejects a ring the drawing shows (spec
 * map-painter-area-culling, requirement "A ring's visibility tolerance covers every border style the
 * ring resolves").
 */
TEST_CASE("A tolerance that covers the offset of the border keeps the ring")
{
  constexpr double             borderWidthMM=10.0;
  constexpr double             halfSizePx=50.0;
  constexpr double             offsetMapUnits=1000.0;

  osmscout::MercatorProjection projection=MakeProjectionWithDpi(referenceDpi);

  osmscout::BorderStyleRef     borderStyle=std::make_shared<osmscout::BorderStyle>();

  borderStyle->SetWidth(borderWidthMM);
  borderStyle->SetOffset(offsetMapUnits);

  const double tolerancePx=osmscout::GetAreaRingTolerancePixel(projection,{borderStyle});
  const double widthOnlyPx=projection.ConvertWidthToPixel(borderWidthMM/2.0);

  // The distance lies between the reach of half the width and the reach of the offset
  const double distancePx=widthOnlyPx+(offsetMapUnits/projection.GetPixelSize())/2.0;

  INFO("tolerance of the border: " << tolerancePx);
  INFO("reach of half of its width: " << widthOnlyPx);

  REQUIRE(distancePx>widthOnlyPx);
  REQUIRE(distancePx<tolerancePx);

  REQUIRE(osmscout::IsAreaRingVisible(projection,
                                      BoxEastOfView(projection,distancePx,halfSizePx),
                                      tolerancePx,
                                      0.0));

  REQUIRE_FALSE(osmscout::IsAreaRingVisible(projection,
                                            BoxEastOfView(projection,distancePx,halfSizePx),
                                            widthOnlyPx,
                                            0.0));
}

/**
 * The scenario "an area beyond the converted tolerance still contributes nothing", for an area that
 * lies outside the view in all directions.
 */
TEST_CASE("An area outside the view contributes nothing")
{
  constexpr double borderWidthMM=10.0;

  osmscout::MercatorProjection projection=MakeProjectionWithDpi(referenceDpi);

  double tolerancePx=projection.ConvertWidthToPixel(borderWidthMM/2.0);

  REQUIRE_FALSE(osmscout::IsAreaRingVisible(projection,
                                            BoxEastOfView(projection,4.0*tolerancePx,50.0),
                                            tolerancePx,
                                            0.0));
}

/**
 * An area inside the view is kept, and one smaller than the smallest dimension the draw parameters
 * draw is rejected even inside the view.
 */
TEST_CASE("An area smaller than the smallest drawn dimension contributes nothing")
{
  constexpr double borderWidthMM=10.0;
  constexpr double minDimensionMM=10.0;

  constexpr double smallHalfSizePx=1.0;
  constexpr double largeHalfSizePx=100.0;

  osmscout::MercatorProjection projection=MakeProjectionWithDpi(referenceDpi);

  double minDimensionPx=projection.ConvertWidthToPixel(minDimensionMM);

  // The small box is below the smallest drawn dimension on both axes, the large one above it
  REQUIRE(2.0*smallHalfSizePx<=minDimensionPx);
  REQUIRE(2.0*largeHalfSizePx>minDimensionPx);

  REQUIRE_FALSE(osmscout::IsAreaRingVisible(projection,
                                            BoxInView(projection,smallHalfSizePx,smallHalfSizePx),
                                            0.0,
                                            minDimensionMM));

  REQUIRE(osmscout::IsAreaRingVisible(projection,
                                      BoxInView(projection,largeHalfSizePx,largeHalfSizePx),
                                      projection.ConvertWidthToPixel(borderWidthMM/2.0),
                                      minDimensionMM));
}

#if defined(OPENGL_TEST_FONT_FILE) && defined(OPENGL_TEST_SHADER_DIR)

namespace {

  /**
   * Error callback of the offscreen context of the cases below. GLFW reports a missing display or a
   * missing driver through it; the cases decide themselves whether they can run, so the report is
   * dropped rather than printed as noise of a skipped case.
   */
  void SilentGlfwError(int /*error*/, const char * /*description*/)
  {
    // no code
  }

}

/**
 * The scenario "a ring the decision discards costs no per-ring geometry work", for the step itself:
 * a view that loads areas outside its visible area examines their rings, keeps the rings of the one
 * visible area, and prepares geometry only for those.
 *
 * The step needs an OpenGL context to be constructed, so the case skips - it never fails - when no
 * offscreen context can be created.
 */
TEST_CASE("The per-ring work of the area step follows the rings it keeps")
{
  constexpr double borderWidthMM=10.0;
  constexpr double minDimensionMM=1.0;
  constexpr double halfSizePx=50.0;
  constexpr size_t outsideAreaCount=3;

  osmscout::MercatorProjection projection=MakeProjectionWithDpi(referenceDpi);

  osmscout::TypeConfigRef typeConfig=std::make_shared<osmscout::TypeConfig>();
  osmscout::TypeInfoRef areaType=std::make_shared<osmscout::TypeInfo>("test_area");

  areaType->CanBeArea(true);
  typeConfig->RegisterType(areaType);

  osmscout::StyleConfigRef styleConfig=MakeStyleConfig(typeConfig,areaType,borderWidthMM);

  double tolerancePx=projection.ConvertWidthToPixel(borderWidthMM/2.0);

  osmscout::MapData data;

  data.styleConfig=styleConfig;
  data.areas.push_back(MakeArea(areaType,BoxInView(projection,halfSizePx,halfSizePx)));

  for (size_t i=0; i<outsideAreaCount; i++) {
    data.areas.push_back(MakeArea(areaType,
                                  BoxEastOfView(projection,
                                                4.0*tolerancePx+(double)i*100.0,
                                                halfSizePx)));
  }

  osmscout::MapParameter parameter;

  parameter.SetAreaMinDimensionMM(minDimensionMM);

  glfwSetErrorCallback(SilentGlfwError);

  if (glfwInit()!=GLFW_TRUE) {
    SKIP("GLFW could not be initialized");
  }

  glfwWindowHint(GLFW_VISIBLE,GLFW_FALSE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,2);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *context=glfwCreateWindow((int)viewportSize,(int)viewportSize,"",nullptr,nullptr);

  if (context==nullptr) {
    glfwTerminate();
    SKIP("no offscreen OpenGL context");
  }

  glfwMakeContextCurrent(context);

  {
    osmscout::MapPainterOpenGL painter((int)viewportSize,
                                       (int)viewportSize,
                                       referenceDpi,
                                       OPENGL_TEST_FONT_FILE,
                                       OPENGL_TEST_SHADER_DIR,
                                       parameter);

    if (!painter.IsInitialized()) {
      glfwDestroyWindow(context);
      glfwTerminate();
      SKIP("the OpenGL painter could not be initialized");
    }

    painter.SetCenter(projection.GetCenter());
    painter.SetMagnification(projection.GetMagnification());
    painter.ProcessData(data,projection,styleConfig);

    // One ring per area, and only the ring of the visible area is prepared
    CHECK(painter.GetExaminedRingCount()==1+outsideAreaCount);
    CHECK(painter.GetKeptRingCount()==1);
  }

  glfwDestroyWindow(context);
  glfwTerminate();
}

/**
 * The step decides a ring with the tolerance of every border style the ring resolves: an area outside
 * the view whose border reaches it only through its offset is kept, and one beyond that reach is not
 * (spec map-painter-area-culling, requirement "A ring's visibility tolerance covers every border
 * style the ring resolves").
 *
 * The step needs an OpenGL context to be constructed, so the case skips - it never fails - when no
 * offscreen context can be created.
 */
TEST_CASE("The area step keeps a ring whose border reaches the view through its offset")
{
  constexpr double             borderWidthMM=10.0;
  constexpr double             minDimensionMM=1.0;
  constexpr double             halfSizePx=50.0;

  osmscout::MercatorProjection projection=MakeProjectionWithDpi(referenceDpi);

  const double                 widthReachPx=projection.ConvertWidthToPixel(borderWidthMM/2.0);

  // An offset that reaches three times as far as half of the border width
  const double            offsetMapUnits=3.0*widthReachPx*projection.GetPixelSize();
  const double            offsetReachPx=offsetMapUnits/projection.GetPixelSize();

  osmscout::TypeConfigRef typeConfig=std::make_shared<osmscout::TypeConfig>();
  osmscout::TypeInfoRef   areaType=std::make_shared<osmscout::TypeInfo>("test_area");

  areaType->CanBeArea(true);
  typeConfig->RegisterType(areaType);

  osmscout::StyleConfigRef styleConfig=MakeStyleConfig(typeConfig,areaType,borderWidthMM,offsetMapUnits);

  INFO("reach of half of the border width: " << widthReachPx);
  INFO("reach of the offset: " << offsetReachPx);

  const double      reachPx=widthReachPx+offsetReachPx;

  osmscout::MapData data;

  data.styleConfig=styleConfig;

  // The first area lies outside the view by more than half of the border width can reach, the second
  // one beyond the offset as well
  data.areas.push_back(MakeArea(areaType,
                                BoxEastOfView(projection,widthReachPx+0.5*offsetReachPx,halfSizePx)));
  data.areas.push_back(MakeArea(areaType,
                                BoxEastOfView(projection,reachPx+10.0,halfSizePx)));

  osmscout::MapParameter parameter;

  parameter.SetAreaMinDimensionMM(minDimensionMM);

  glfwSetErrorCallback(SilentGlfwError);

  if (glfwInit()!=GLFW_TRUE) {
    SKIP("GLFW could not be initialized");
  }

  glfwWindowHint(GLFW_VISIBLE,GLFW_FALSE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,2);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *context=glfwCreateWindow((int)viewportSize,(int)viewportSize,"",nullptr,nullptr);

  if (context==nullptr) {
    glfwTerminate();
    SKIP("no offscreen OpenGL context");
  }

  glfwMakeContextCurrent(context);

  {
    osmscout::MapPainterOpenGL painter((int)viewportSize,
                                       (int)viewportSize,
                                       referenceDpi,
                                       OPENGL_TEST_FONT_FILE,
                                       OPENGL_TEST_SHADER_DIR,
                                       parameter);

    if (!painter.IsInitialized()) {
      glfwDestroyWindow(context);
      glfwTerminate();
      SKIP("the OpenGL painter could not be initialized");
    }

    painter.SetCenter(projection.GetCenter());
    painter.SetMagnification(projection.GetMagnification());
    painter.ProcessData(data,projection,styleConfig);

    // Both rings are examined, and only the ring the offset of the border reaches is kept
    CHECK(painter.GetExaminedRingCount()==2);
    CHECK(painter.GetKeptRingCount()==1);
  }

  glfwDestroyWindow(context);
  glfwTerminate();
}

#endif
