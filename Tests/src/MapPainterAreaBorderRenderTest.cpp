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

#include <cstddef>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <osmscout/Area.h>
#include <osmscout/GeoCoord.h>
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

#include <osmscoutmapsvg/MapPainterSVG.h>

/*
 * Tests for the rendered output of the per-ring visibility decision: a ring whose border is drawn at
 * an offset and reaches the view through it appears in the frame, and one whose drawn border does not
 * reach the view contributes nothing (spec map-painter-area-culling, requirement "A ring's visibility
 * tolerance covers every border style the ring resolves").
 *
 * The frame is rendered by the SVG backend into a string, which needs no window, no font and no
 * database, so the case compares the drawn output of a synthetic area directly.
 */

namespace {

  constexpr double referenceDpi=300.0;
  constexpr size_t viewportSize=400;

  /**
   * The colour the border of the test style sheet is drawn with; it is the marker the cases look for
   * in the rendered frame.
   */
  constexpr const char *borderColor="#ff00ff";

  /**
   * A border drawn at an offset in the map units the style sheet declares it in. It is large enough
   * that the offset dominates the reach of the border.
   */
  constexpr double borderOffsetMapUnits=2000.0;

  osmscout::MercatorProjection MakeProjection()
  {
    osmscout::MercatorProjection projection;

    REQUIRE(projection.Set(osmscout::GeoCoord(50.001,8.001),
                           osmscout::Magnification(osmscout::Magnification::magClose),
                           referenceDpi,
                           viewportSize,
                           viewportSize));

    return projection;
  }

  /**
   * Type config with one area type the style sheet draws.
   */
  osmscout::TypeInfoRef MakeAreaType(const osmscout::TypeConfigRef& typeConfig)
  {
    osmscout::TypeInfoRef areaType=std::make_shared<osmscout::TypeInfo>("test_area");

    areaType->CanBeArea(true);
    typeConfig->RegisterType(areaType);

    return areaType;
  }

  /**
   * Style sheet that draws the area type by a fill and a border of the given width, drawn at the
   * given offset.
   */
  osmscout::StyleConfigRef MakeStyles(const osmscout::TypeConfigRef& typeConfig,
                                      const osmscout::TypeInfoRef& areaType,
                                      double borderWidthMM,
                                      double borderOffsetMapUnits)
  {
    auto                  styleConfig=std::make_shared<osmscout::StyleConfig>(typeConfig);

    osmscout::TypeInfoSet areaTypes(*typeConfig);

    areaTypes.Set(areaType);

    osmscout::StyleFilter areaFilter;

    areaFilter.SetTypes(areaTypes);

    osmscout::FillPartialStyle fillStyle;

    fillStyle.SetColorValue(osmscout::FillStyle::attrFillColor,osmscout::Color(0.0,1.0,0.0));
    styleConfig->AddAreaFillStyle(areaFilter,fillStyle);

    osmscout::BorderPartialStyle borderStyle;

    borderStyle.SetDoubleValue(osmscout::BorderStyle::attrWidth,borderWidthMM);
    borderStyle.SetDoubleValue(osmscout::BorderStyle::attrOffset,borderOffsetMapUnits);
    borderStyle.SetColorValue(osmscout::BorderStyle::attrColor,osmscout::Color(1.0,0.0,1.0));
    styleConfig->AddAreaBorderStyle(areaFilter,borderStyle);

    styleConfig->Postprocess();

    return styleConfig;
  }

  /**
   * Pixels per degree of longitude at the latitude of the projection center.
   */
  double PixelsPerDegreeLon(const osmscout::MercatorProjection& projection)
  {
    constexpr double   probeDeltaDegrees=0.001;

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
   * Area with one outer ring that is an axis parallel rectangle whose western edge lies the given
   * number of pixels east of the eastern edge of the view.
   */
  osmscout::AreaRef MakeAreaEastOfView(const osmscout::MercatorProjection& projection,
                                       const osmscout::TypeInfoRef& areaType,
                                       double distancePx,
                                       double halfSizePx)
  {
    osmscout::GeoBox     dimensions=projection.GetDimensions();
    osmscout::GeoCoord   center=projection.GetCenter();

    double               pixelScale=PixelsPerDegreeLon(projection);

    double               halfSizeLon=halfSizePx/pixelScale;
    double               halfSizeLat=(dimensions.GetMaxLat()-dimensions.GetMinLat())/4.0;
    double               westLon=dimensions.GetMaxLon()+distancePx/pixelScale;

    auto                 area=std::make_shared<osmscout::Area>();

    osmscout::Area::Ring ring;

    ring.MarkAsOuterRing();
    ring.SetType(areaType);
    ring.nodes.push_back(osmscout::Point(1,osmscout::GeoCoord(center.GetLat()-halfSizeLat,westLon)));
    ring.nodes.push_back(osmscout::Point(2,osmscout::GeoCoord(center.GetLat()-halfSizeLat,westLon+2.0*halfSizeLon)));
    ring.nodes.push_back(osmscout::Point(3,osmscout::GeoCoord(center.GetLat()+halfSizeLat,westLon+2.0*halfSizeLon)));
    ring.nodes.push_back(osmscout::Point(4,osmscout::GeoCoord(center.GetLat()+halfSizeLat,westLon)));
    ring.center=osmscout::GeoCoord(center.GetLat(),westLon+halfSizeLon);

    area->rings.push_back(ring);

    return area;
  }

  /**
   * Renders one frame that loads a single area at the given distance from the view and returns the
   * SVG document of that frame.
   */
  std::string RenderFrame(const osmscout::MercatorProjection& projection,
                          const osmscout::StyleConfigRef& styleConfig,
                          const osmscout::TypeInfoRef& areaType,
                          double distancePx)
  {
    osmscout::MapData data;

    data.styleConfig=styleConfig;
    data.areas.push_back(MakeAreaEastOfView(projection,areaType,distancePx,60.0));

    osmscout::MapParameter  parameter;

    osmscout::MapPainterSVG painter;

    std::ostringstream      stream;

    REQUIRE(painter.DrawMap(projection,
                            parameter,
                            std::vector<osmscout::MapData> {data},
                            stream));

    return stream.str();
  }
}

/**
 * A ring outside the view whose border reaches into it through the offset the border is drawn at is
 * rendered, and one beyond that reach is not.
 */
TEST_CASE("A border drawn at an offset appears in the rendered frame","[MapPainterAreaBorderRender]")
{
  auto             typeConfig=std::make_shared<osmscout::TypeConfig>();
  auto             areaType=MakeAreaType(typeConfig);
  auto             projection=MakeProjection();

  constexpr double borderWidthMM=2.0;

  auto             styleConfig=MakeStyles(typeConfig,
                                          areaType,
                                          borderWidthMM,
                                          borderOffsetMapUnits);

  osmscout::BorderStyleRef borderStyle=std::make_shared<osmscout::BorderStyle>();

  borderStyle->SetWidth(borderWidthMM);
  borderStyle->SetOffset(borderOffsetMapUnits);

  const double tolerancePx=osmscout::GetAreaRingTolerancePixel(projection,{borderStyle});
  const double widthOnlyPx=projection.ConvertWidthToPixel(borderWidthMM/2.0);

  INFO("tolerance of the border: " << tolerancePx);
  INFO("reach of half of its width: " << widthOnlyPx);

  REQUIRE(tolerancePx>widthOnlyPx+10.0);

  // The area lies outside the view by more than half of its border width can reach, so its border
  // reaches the view only through the offset
  const double distancePx=widthOnlyPx+0.25*(tolerancePx-widthOnlyPx);

  REQUIRE(distancePx>widthOnlyPx);
  REQUIRE(distancePx<tolerancePx);

  const std::string nearFrame=RenderFrame(projection,styleConfig,areaType,distancePx);

  INFO("distance of the area from the view: " << distancePx);

  REQUIRE(nearFrame.find(borderColor)!=std::string::npos);

  // Beyond the reach of the drawn border the frame holds no border at all
  const std::string farFrame=RenderFrame(projection,styleConfig,areaType,tolerancePx+10.0);

  REQUIRE(farFrame.find(borderColor)==std::string::npos);
}
