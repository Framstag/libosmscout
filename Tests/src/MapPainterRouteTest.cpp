/*
  This source is part of the libosmscout-map library
  Copyright (C) 2025  Tim Teulings

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

#include <cairo.h>

#include <osmscout/TypeConfig.h>
#include <osmscout/Way.h>
#include <osmscout/feature/LayerFeature.h>
#include <osmscout/projection/MercatorProjection.h>
#include <osmscoutmap/MapPainter.h>
#include <osmscoutmap/MapParameter.h>
#include <osmscoutmap/StyleConfig.h>
#include <osmscoutmap/Styles.h>
#include <osmscoutmapcairo/MapPainterCairo.h>

namespace {

// ---------------------------------------------------------------------------
// Helper: build a WayData with the given draw-order attributes
// ---------------------------------------------------------------------------
static osmscout::MapPainter::WayData MakeWayData(int8_t layer,
                                                 int priority,
                                                 size_t wayPriority)
{
  auto style=std::make_shared<osmscout::LineStyle>();
  style->SetPriority(priority);
  style->SetZIndex(0);

  osmscout::MapPainter::WayData way;
  way.buffer=nullptr;
  way.layer=layer;
  way.lineStyle=style;
  way.wayPriority=wayPriority;
  return way;
}

// ---------------------------------------------------------------------------
// Helper: build a type config with a "bridge" way type carrying the layer
// feature. The internal "_route" type comes from TypeConfig itself and also
// carries the layer feature, so callers can stack the route via a layer value.
// ---------------------------------------------------------------------------
static osmscout::TypeConfigRef MakeTypeConfig(osmscout::TypeInfoRef& bridgeType,
                                               osmscout::TypeInfoRef& routeType)
{
  auto typeConfig=std::make_shared<osmscout::TypeConfig>();

  auto layerFeature=typeConfig->GetFeature("Layer");
  REQUIRE(layerFeature);

  bridgeType=std::make_shared<osmscout::TypeInfo>("bridge");
  bridgeType->CanBeWay(true);
  bridgeType->AddFeature(layerFeature);
  typeConfig->RegisterType(bridgeType);

  routeType=typeConfig->GetTypeInfo("_route");
  REQUIRE(routeType);

  return typeConfig;
}

// ---------------------------------------------------------------------------
// Helper: build a style config with a blue bridge style and a cased route
// style (white outline + red fill), following the roads.oss casing pattern
// ---------------------------------------------------------------------------
static osmscout::StyleConfigRef MakeStyleConfig(const osmscout::TypeConfigRef& typeConfig,
                                                 const osmscout::TypeInfoRef& bridgeType,
                                                 const osmscout::TypeInfoRef& routeType)
{
  auto styleConfig=std::make_shared<osmscout::StyleConfig>(typeConfig);

  osmscout::TypeInfoSet bridgeSet(*typeConfig);
  bridgeSet.Set(bridgeType);
  osmscout::StyleFilter bridgeFilter;
  bridgeFilter.SetTypes(bridgeSet);

  osmscout::LinePartialStyle bridgeStyle;
  bridgeStyle.SetColorValue(osmscout::LineStyle::attrLineColor, osmscout::Color(0.0, 0.0, 1.0));
  bridgeStyle.SetDoubleValue(osmscout::LineStyle::attrDisplayWidth, 3.0);
  bridgeStyle.SetDoubleValue(osmscout::LineStyle::attrWidth, 20.0);
  bridgeStyle.SetIntValue(osmscout::LineStyle::attrPriority, 0);
  styleConfig->AddWayLineStyle(bridgeFilter, bridgeStyle);

  osmscout::TypeInfoSet routeSet(*typeConfig);
  routeSet.Set(routeType);
  osmscout::StyleFilter routeFilter;
  routeFilter.SetTypes(routeSet);

  osmscout::LinePartialStyle outlineStyle;
  outlineStyle.style->SetSlot("outline");
  outlineStyle.SetColorValue(osmscout::LineStyle::attrLineColor, osmscout::Color(1.0, 1.0, 1.0));
  outlineStyle.SetDoubleValue(osmscout::LineStyle::attrDisplayWidth, 2.2);
  outlineStyle.SetDoubleValue(osmscout::LineStyle::attrWidth, 8.0);
  outlineStyle.SetIntValue(osmscout::LineStyle::attrPriority, 99);
  styleConfig->AddWayLineStyle(routeFilter, outlineStyle);

  osmscout::LinePartialStyle fillStyle;
  fillStyle.SetColorValue(osmscout::LineStyle::attrLineColor, osmscout::Color(1.0, 0.0, 0.0));
  fillStyle.SetDoubleValue(osmscout::LineStyle::attrDisplayWidth, 1.5);
  fillStyle.SetDoubleValue(osmscout::LineStyle::attrWidth, 6.0);
  fillStyle.SetIntValue(osmscout::LineStyle::attrPriority, 100);
  styleConfig->AddWayLineStyle(routeFilter, fillStyle);

  // Build the per-type style selectors (normally done at the end of Load)
  styleConfig->Postprocess();

  return styleConfig;
}

// ---------------------------------------------------------------------------
// Helper: build a way with the given type and optional layer value
// ---------------------------------------------------------------------------
static osmscout::WayRef MakeWay(const osmscout::TypeInfoRef& type,
                                const osmscout::GeoCoord& c1,
                                const osmscout::GeoCoord& c2,
                                int8_t layer)
{
  auto way=std::make_shared<osmscout::Way>();
  way->nodes.push_back(osmscout::Point(1, c1));
  way->nodes.push_back(osmscout::Point(2, c2));

  osmscout::FeatureValueBuffer buffer;
  buffer.SetType(type);

  if (layer!=0) {
    for (size_t i=0; i<type->GetFeatureCount(); ++i) {
      if (type->GetFeature(i).GetFeature()->GetName()=="Layer") {
        auto* value=buffer.AllocateValue(i);
        static_cast<osmscout::LayerFeatureValue*>(value)->SetLayer(layer);
        break;
      }
    }
  }

  way->SetFeatures(buffer);
  return way;
}

// ---------------------------------------------------------------------------
// Helper: render a scene with a bridge way and a route way on the same line
// and return the ARGB32 pixel data (premultiplied, 400x400)
// ---------------------------------------------------------------------------
static std::vector<uint32_t> RenderScene(const osmscout::StyleConfigRef& styleConfig,
                                         const osmscout::WayRef& bridge,
                                         const osmscout::WayRef& route)
{
  osmscout::MapData data;
  data.styleConfig=styleConfig;
  data.ways.push_back(bridge);
  data.poiWays.push_back(route);

  osmscout::MercatorProjection projection;
  REQUIRE(projection.Set(osmscout::GeoCoord(50.0, 8.005),
                         osmscout::Magnification(osmscout::Magnification::magClose),
                         300,
                         400,
                         400));

  osmscout::MapParameter parameter;

  cairo_surface_t* surface=cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 400, 400);
  cairo_t* draw=cairo_create(surface);

  osmscout::MapPainterCairo painter;
  REQUIRE(painter.DrawMap(projection,
                          parameter,
                          std::vector<osmscout::MapData>{data},
                          draw));

  cairo_destroy(draw);

  std::vector<uint32_t> pixels(400*400);
  const uint32_t* dataPtr=reinterpret_cast<const uint32_t*>(cairo_image_surface_get_data(surface));
  std::copy(dataPtr, dataPtr + 400*400, pixels.begin());

  cairo_surface_destroy(surface);
  return pixels;
}

// ---------------------------------------------------------------------------
// Helper: extract R/G/B from a premultiplied ARGB32 pixel (little-endian)
// ---------------------------------------------------------------------------
static void GetRGB(uint32_t pixel, int& r, int& g, int& b)
{
  r=(pixel >> 16) & 0xFF;
  g=(pixel >> 8) & 0xFF;
  b=pixel & 0xFF;
}

} // namespace

// ---------------------------------------------------------------------------
// Draw order: WayData::operator< must place the route above all map ways
// ---------------------------------------------------------------------------
TEST_CASE("WayData sort places route above bridges and normal ways", "[MapPainterRoute]")
{
  auto route=MakeWayData(osmscout::MapPainter::routeLayer, 100, 100);
  auto bridge=MakeWayData(1, 0, 100);
  auto normal=MakeWayData(0, 0, 100);
  auto tunnel=MakeWayData(-1, 0, 100);

  REQUIRE(bridge < route);
  REQUIRE(normal < route);
  REQUIRE(tunnel < route);
  REQUIRE(!(route < bridge));
  REQUIRE(!(route < normal));
  REQUIRE(!(route < tunnel));
}

// ---------------------------------------------------------------------------
// Render: the route must be painted on top of a bridge (layer 1) and show a
// white casing around the red fill
// ---------------------------------------------------------------------------
TEST_CASE("Route renders above bridge with cased style", "[MapPainterRoute]")
{
  osmscout::TypeInfoRef bridgeType;
  osmscout::TypeInfoRef routeType;
  auto typeConfig=MakeTypeConfig(bridgeType, routeType);
  auto styleConfig=MakeStyleConfig(typeConfig, bridgeType, routeType);

  osmscout::GeoCoord c1(50.0, 8.0);
  osmscout::GeoCoord c2(50.0, 8.01);

  auto bridge=MakeWay(bridgeType, c1, c2, 1);
  // The active route carries the layer feature value; the renderer stays
  // generic and just honors the layer (no type-name special casing).
  auto route=MakeWay(routeType, c1, c2, osmscout::MapPainter::routeLayer);

  auto pixels=RenderScene(styleConfig, bridge, route);

  // At 300 DPI, 1mm = 11.81px. The line runs horizontally through y=200.
  //   bridge: 3.0mm wide  -> half width ~17.7px
  //   outline: 2.2mm wide -> half width ~13.0px
  //   fill: 1.5mm wide    -> half width ~8.9px
  const int cx=200;
  const int cy=200;

  int r, g, b;

  // Center of the line: red route fill on top of the blue bridge
  GetRGB(pixels[cy*400 + cx], r, g, b);
  REQUIRE(r > 200);
  REQUIRE(g < 50);
  REQUIRE(b < 50);

  // Within the outline but outside the fill: white casing over the bridge
  GetRGB(pixels[(cy + 11)*400 + cx], r, g, b);
  REQUIRE(r > 200);
  REQUIRE(g > 200);
  REQUIRE(b > 200);

  // Outside the outline but within the bridge: blue bridge still visible
  GetRGB(pixels[(cy + 15)*400 + cx], r, g, b);
  REQUIRE(r < 50);
  REQUIRE(g < 50);
  REQUIRE(b > 200);
}
