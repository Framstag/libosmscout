/*
  This source is part of the libosmscout library
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

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <osmscout/TypeConfig.h>
#include <osmscout/projection/MercatorProjection.h>
#include <osmscout/util/Color.h>
#include <osmscout/util/Magnification.h>
#include <osmscoutmap/StyleConfig.h>
#include <osmscoutmap/Styles.h>

/*
 * Tests for the paint of the active route (spec route-visualization): the route takes its fill
 * and casing colours from stylesheets/include/route.oss, the colours follow the presentation the
 * style sheet is loaded with, and every style sheet that can draw a route resolves those shared
 * styles instead of a route line of its own.
 *
 * The tests resolve the styles of the real style sheets, so they need the style sheet files but
 * neither a database nor a renderer: a style sheet that parses with a wrong colour would still
 * pass the CheckStyleSheet tests, and this is the check that pins the values.
 *
 * The order in which the resolved styles come back is not part of the contract - the painter
 * orders the strokes of a way by their priority (WayData::wayPriority, stable sorted after
 * preprocessing), so the tests identify the casing by its slot and assert the priority relation
 * rather than a position in the vector.
 */

namespace {

  using osmscout::Color;
  using osmscout::LineStyleRef;

  /** The projected view the styles are resolved for; any level inside the route's zoom range. */
  constexpr double PROJECTION_LAT = 50.001;
  constexpr double PROJECTION_LON = 8.001;
  constexpr size_t PROJECTION_WIDTH = 300;
  constexpr size_t PROJECTION_HEIGHT = 400;

  /** Daylight: an opaque violet fill over a dark violet casing. */
  Color RouteFillDaylight()
  {
    return Color::FromHexString("#7b1fa2");
  }

  Color RouteCasingDaylight()
  {
    return Color::FromHexString("#311b92");
  }

  /** Dark presentation: the fill and casing the route carried before the daylight variant. */
  Color RouteFillDark()
  {
    return Color::FromHexString("#ff000088");
  }

  Color RouteCasingDark()
  {
    return Color::FromHexString("#ffffff");
  }

  /** The style sheets that can draw an active route. */
  constexpr std::array<const char*, 3> ROUTE_STYLE_SHEETS = {"standard.oss",
                                                             "winter-sports.oss",
                                                             "cycle.oss"};

  /** The two strokes the shared module defines for the route. */
  struct RoutePaint
  {
    LineStyleRef casing;
    LineStyleRef fill;
  };

  std::string GetEnv(const char* name,
                     const std::string& fallback)
  {
    // NOLINTNEXTLINE(concurrency-mt-unsafe) read once, before any test runs, like the sibling tests
    const char * value = std::getenv(name);

    return value != nullptr ? std::string(value) : fallback;
  }

  std::filesystem::path StylesheetPath(const std::string& name)
  {
    return std::filesystem::path(GetEnv("TESTS_TOP_DIR", "..")) / ".." / "stylesheets" / name;
  }

  void RegisterInternalType(const osmscout::TypeConfigRef& typeConfig,
                            const std::string& name)
  {
    auto typeInfo = std::make_shared<osmscout::TypeInfo>(name);

    typeInfo->SetInternal()
    .CanBeWay(true)
    .CanBeArea(true)
    .CanBeNode(true);

    typeConfig->RegisterType(typeInfo);
  }

  osmscout::TypeConfigRef LoadTypeConfig()
  {
    std::filesystem::path ost = StylesheetPath("map.ost");
    auto                  typeConfig = std::make_shared<osmscout::TypeConfig>();

    if (!typeConfig->LoadFromOSTFile(ost.string())) {
      FAIL("Cannot load type config file '" << ost.string() << "'");
    }

    // The shared route module styles the overlay types that the clients register at runtime
    // (`withCustomPoiType` in the JNI, `AddCustomPoiType` in the Qt client): a style rule whose
    // type is unknown is dropped at load time, so the test registers them the same way. `_route`
    // itself is registered by TypeConfig already.
    RegisterInternalType(typeConfig, "_track");
    RegisterInternalType(typeConfig, "_route_start");
    RegisterInternalType(typeConfig, "_route_end");
    RegisterInternalType(typeConfig, "_favorite");
    RegisterInternalType(typeConfig, "_search_selected");

    return typeConfig;
  }

  osmscout::MercatorProjection MakeProjection()
  {
    osmscout::MercatorProjection projection;

    REQUIRE(projection.Set(osmscout::GeoCoord(PROJECTION_LAT, PROJECTION_LON),
                           osmscout::Magnification(osmscout::Magnification::magClose),
                           PROJECTION_WIDTH,
                           PROJECTION_HEIGHT));

    return projection;
  }

  /** A style sheet loaded with the given presentation, for the shared type config. */
  osmscout::StyleConfigRef LoadStyleSheet(const osmscout::TypeConfigRef& typeConfig,
                                          const std::string& styleSheet,
                                          bool daylight)
  {
    auto styleConfig = std::make_shared<osmscout::StyleConfig>(typeConfig);

    // The style sheets declare daylight = true themselves; set the flag first, so the value a
    // client pushes takes precedence over that default (the parser only fills in a flag that is
    // not set yet).
    styleConfig->AddFlag("daylight", daylight);

    REQUIRE(styleConfig->Load(StylesheetPath(styleSheet).string()));

    return styleConfig;
  }

  osmscout::FeatureValueBuffer MakeBuffer(const osmscout::TypeConfigRef& typeConfig,
                                          const std::string& type)
  {
    osmscout::TypeInfoRef typeInfo = typeConfig->GetTypeInfo(type);

    REQUIRE(typeInfo != nullptr);

    osmscout::FeatureValueBuffer buffer;

    buffer.SetType(typeInfo);

    return buffer;
  }

  std::vector<LineStyleRef> ResolveRouteStyles(const osmscout::TypeConfigRef& typeConfig,
                                               const std::string& styleSheet,
                                               bool daylight)
  {
    auto                         styleConfig = LoadStyleSheet(typeConfig, styleSheet, daylight);
    osmscout::FeatureValueBuffer buffer = MakeBuffer(typeConfig, "_route");
    std::vector<LineStyleRef>    styles;

    // The active route reaches the painter as a poi way of type `_route`, so its paint is a way
    // line style (the `ROUTE { }` blocks of a style sheet style OSM route relations instead).
    styleConfig->GetWayLineStyles(buffer, MakeProjection(), styles);

    return styles;
  }

  /** The style of the given slot, or nullptr when no style of the paint has it. */
  LineStyleRef FindStyleBySlot(const std::vector<LineStyleRef>& styles,
                               const std::string& slot)
  {
    for (const auto& style : styles) {
      if (style->GetSlot() == slot) {
        return style;
      }
    }

    return nullptr;
  }

  /**
   * The casing (slot "outline") and the fill (no slot) of the resolved route paint. With the two
   * styles a shared route module defines, finding both means exactly one of each carries a slot.
   */
  RoutePaint FindRoutePaint(const std::vector<LineStyleRef>& styles)
  {
    RoutePaint paint;

    paint.casing = FindStyleBySlot(styles, "outline");
    paint.fill = FindStyleBySlot(styles, "");

    REQUIRE(paint.casing != nullptr);
    REQUIRE(paint.fill != nullptr);

    return paint;
  }

  /** One style sheet's route paint, so the test case over all of them stays a plain loop. */
  void CheckSharedRoutePaint(const osmscout::TypeConfigRef& typeConfig,
                             const std::string& styleSheet)
  {
    std::vector<LineStyleRef> styles = ResolveRouteStyles(typeConfig, styleSheet, true);

    INFO("style sheet: " << styleSheet);

    REQUIRE(styles.size() == 2);

    RoutePaint paint = FindRoutePaint(styles);

    REQUIRE(paint.casing->GetLineColor() == RouteCasingDaylight());
    REQUIRE(paint.fill->GetLineColor() == RouteFillDaylight());
  }
}

TEST_CASE("Route line styles follow the presentation flag")
{
  auto typeConfig = LoadTypeConfig();

  SECTION("daylight presentation")
  {
    std::vector<LineStyleRef> styles = ResolveRouteStyles(typeConfig, "standard.oss", true);

    REQUIRE(styles.size() == 2);

    RoutePaint paint = FindRoutePaint(styles);

    REQUIRE(paint.casing->GetLineColor() == RouteCasingDaylight());
    REQUIRE(paint.fill->GetLineColor() == RouteFillDaylight());

    // The casing is the wider stroke and is painted below the fill: the painter orders the
    // strokes of a way by their priority.
    REQUIRE(paint.casing->GetDisplayWidth() > paint.fill->GetDisplayWidth());
    REQUIRE(paint.casing->GetPriority() < paint.fill->GetPriority());
  }

  SECTION("dark presentation")
  {
    std::vector<LineStyleRef> styles = ResolveRouteStyles(typeConfig, "standard.oss", false);

    REQUIRE(styles.size() == 2);

    RoutePaint paint = FindRoutePaint(styles);

    REQUIRE(paint.casing->GetLineColor() == RouteCasingDark());
    REQUIRE(paint.fill->GetLineColor() == RouteFillDark());

    // The two presentations are different paints, so a flag change is visible.
    REQUIRE(paint.casing->GetLineColor() != RouteCasingDaylight());
    REQUIRE(paint.fill->GetLineColor() != RouteFillDaylight());
  }
}

TEST_CASE("Every style sheet that draws a route resolves the shared route paint")
{
  auto typeConfig = LoadTypeConfig();

  for (const char* styleSheet : ROUTE_STYLE_SHEETS) {
    CheckSharedRoutePaint(typeConfig, styleSheet);
  }
}

TEST_CASE("Including the shared route module brings the route and marker styles")
{
  auto typeConfig = LoadTypeConfig();
  auto styleConfig = LoadStyleSheet(typeConfig, "cycle.oss", true);

  // The route start marker of the cycle style comes from the shared module, so the style sheet
  // cannot include the route paint without the overlay presentation around it.
  osmscout::FeatureValueBuffer startBuffer = MakeBuffer(typeConfig, "_route_start");

  osmscout::IconStyleRef       startStyle = styleConfig->GetNodeIconStyle(startBuffer, MakeProjection());

  REQUIRE(startStyle != nullptr);
  REQUIRE(startStyle->GetSymbol() != nullptr);
  REQUIRE(startStyle->GetSymbol()->GetName() == "route_start");
}
