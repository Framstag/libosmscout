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
 * Tests for the daylight road fills of the standard and the winter-sports style sheet: the fills of
 * the motorway, trunk, primary and secondary class, the thinner fills the style sheets derive from
 * them for the zooms below the full road width, and the fact that the dark presentation keeps its
 * own fills.
 *
 * The tests resolve the styles of the real style sheets, so they need the style sheet files but
 * neither a database nor a renderer: a style sheet that parses with a wrong colour would still pass
 * the CheckStyleSheet tests, and this is the check that pins the values.
 *
 * A road class is drawn cased (an outline stroke below a fill stroke) once the class is wide enough
 * on screen and as a single thinner stroke below that, so a SIZE filter of the style sheet picks the
 * paint. The filters are evaluated against the projection's meterInMM/meterInPixel, which is why the
 * projection carries an explicit dpi - with the default of zero every SIZE filter degenerates and
 * only the thin stroke resolves. Which zoom draws the cased paint depends on the widths each style
 * sheet declares for the class, so the tests try a list of zooms instead of hardcoding one.
 */

namespace {

  using osmscout::Color;
  using osmscout::LineStyleRef;
  using osmscout::Magnification;

  /** The projected view the styles are resolved for. */
  constexpr double PROJECTION_LAT = 50.001;
  constexpr double PROJECTION_LON = 8.001;
  constexpr size_t PROJECTION_WIDTH = 300;
  constexpr size_t PROJECTION_HEIGHT = 400;
  /** A screen dpi, so the SIZE filters of the road styles resolve against a real meterInMM. */
  constexpr double PROJECTION_DPI = 96.0;

  /**
   * The zooms a cased, full width road can be resolved at, the farthest one the style sheets draw.
   * Built per call: a static container would move the Magnification statics into a global initialiser.
   */
  std::vector<Magnification> CasedZooms()
  {
    return {Magnification(Magnification::magClose),
            Magnification(Magnification::magCloser),
            Magnification(Magnification::magVeryClose),
            Magnification(Magnification::magBlock),
            Magnification(Magnification::magStreet),
            Magnification(Magnification::magHouse)};
  }

  /** The zooms the thin variant of a road can be resolved at, from the closest one down. */
  std::vector<Magnification> ThinZooms()
  {
    return {Magnification(Magnification::magDetail),
            Magnification(Magnification::magSuburb),
            Magnification(Magnification::magCity),
            Magnification(Magnification::magCounty)};
  }

  /** The daylight fills of the standard style sheet. */
  Color MotorwayDaylight()
  {
    return Color::FromHexString("#7d7af5");
  }

  Color TrunkDaylight()
  {
    return Color::FromHexString("#a3a1f5");
  }

  Color PrimaryDaylight()
  {
    return Color::FromHexString("#f58b8b");
  }

  Color SecondaryDaylight()
  {
    return Color::FromHexString("#fdd08a");
  }

  /** The thinner fills the style sheets derive from the daylight fills (lighten(fill, 0.2)). */
  Color MotorwayThinDaylight()
  {
    return Color::FromHexString("#9794f7");
  }

  Color TrunkThinDaylight()
  {
    return Color::FromHexString("#b5b3f7");
  }

  Color PrimaryThinDaylight()
  {
    return Color::FromHexString("#f7a2a2");
  }

  /** The darkened shield backgrounds the style sheets derive from their daylight fills. */
  Color MotorwayShieldDaylight()
  {
    return Color::FromHexString("#444386");
  }

  Color TrunkShieldDaylight()
  {
    return Color::FromHexString("#595886");
  }

  Color PrimaryShieldDaylight()
  {
    return Color::FromHexString("#864c4c");
  }

  /** The dark end of the daylight palette gives the motorway junction label. */
  Color JunctionLabelDaylight()
  {
    return Color::FromHexString("#a3a1f8");
  }

  /** The style sheets that carry their own road fills; both draw the roads of include/roads.oss. */
  constexpr std::array<const char*, 2> ROAD_STYLE_SHEETS = {"standard.oss",
                                                            "winter-sports.oss"};

  /** The two strokes a cased road class resolves, or a missing outline for a thin one. */
  struct RoadPaint
  {
    LineStyleRef outline;
    LineStyleRef fill;

    /**
     * Whether the paint is of the wanted kind: a cased road resolves an outline stroke below its
     * fill, a thin road resolves its fill alone.
     */
    bool Is(const bool cased) const
    {
      return fill != nullptr &&
             cased == (outline != nullptr);
    }
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

    // The style sheets include the shared route module, which styles the overlay types the clients
    // register at runtime (`withCustomPoiType` in the JNI, `AddCustomPoiType` in the Qt client): a
    // style rule whose type is unknown is dropped at load time, so the test registers them the same
    // way. `_route` itself is registered by TypeConfig already.
    RegisterInternalType(typeConfig, "_track");
    RegisterInternalType(typeConfig, "_route_start");
    RegisterInternalType(typeConfig, "_route_end");
    RegisterInternalType(typeConfig, "_favorite");
    RegisterInternalType(typeConfig, "_search_selected");

    return typeConfig;
  }

  osmscout::MercatorProjection MakeProjection(const Magnification& magnification)
  {
    osmscout::MercatorProjection projection;

    REQUIRE(projection.Set(osmscout::GeoCoord(PROJECTION_LAT, PROJECTION_LON),
                           magnification,
                           PROJECTION_DPI,
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

  /** The line styles the style sheet resolves for a way of the given type at the given zoom. */
  std::vector<LineStyleRef> ResolveRoadStyles(const osmscout::StyleConfigRef& styleConfig,
                                              const osmscout::TypeInfoRef& typeInfo,
                                              const Magnification& magnification)
  {
    osmscout::FeatureValueBuffer buffer;

    buffer.SetType(typeInfo);

    auto                      projection = MakeProjection(magnification);

    std::vector<LineStyleRef> styles;

    styleConfig->GetWayLineStyles(buffer, projection, styles);

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
   * The fills are compared as hex strings, not as Color objects: lighten() recomputes the channels of
   * a parsed literal and leaves them one ULP apart, while ToHexString() rounds to the byte a reader of
   * the style sheet sees.
   */
  std::string ToHex(const LineStyleRef& style)
  {
    return style->GetLineColor().ToHexString();
  }

  /**
   * The cased (the outline stroke and the fill) or the thin paint (the fill alone) of a road class,
   * taken from the first of the given zooms where the class resolves that kind of paint. Which zoom
   * that is depends on the widths the style sheet declares for the class.
   */
  RoadPaint ResolveRoadPaint(const osmscout::TypeConfigRef& typeConfig,
                             const std::string& styleSheet,
                             const osmscout::TypeInfoRef& typeInfo,
                             const std::vector<Magnification>& zooms,
                             bool daylight,
                             bool cased)
  {
    auto styleConfig = LoadStyleSheet(typeConfig, styleSheet, daylight);

    for (const auto& zoom : zooms) {
      std::vector<LineStyleRef> styles = ResolveRoadStyles(styleConfig, typeInfo, zoom);

      RoadPaint                 paint;

      paint.fill = FindStyleBySlot(styles, "");
      paint.outline = FindStyleBySlot(styles, "outline");

      if (paint.Is(cased)) {
        return paint;
      }
    }

    return {};
  }

  /** The style the type config carries for a road class; the style sheets key their rules on it. */
  osmscout::TypeInfoRef RoadType(const osmscout::TypeConfigRef& typeConfig,
                                 const std::string& type)
  {
    auto typeInfo = typeConfig->GetTypeInfo(type);

    REQUIRE(typeInfo != nullptr);

    return typeInfo;
  }

  /** The fill of a road class at the zoom where the style sheet draws it cased and full width. */
  LineStyleRef ResolveFullWidthFill(const osmscout::TypeConfigRef& typeConfig,
                                    const std::string& styleSheet,
                                    const osmscout::TypeInfoRef& typeInfo)
  {
    RoadPaint paint = ResolveRoadPaint(typeConfig, styleSheet, typeInfo, CasedZooms(), true, true);

    REQUIRE(paint.fill != nullptr);

    return paint.fill;
  }

  /** The fill of a road class at the zoom where the style sheet draws it thin. */
  LineStyleRef ResolveThinFill(const osmscout::TypeConfigRef& typeConfig,
                               const std::string& styleSheet,
                               const osmscout::TypeInfoRef& typeInfo)
  {
    RoadPaint paint = ResolveRoadPaint(typeConfig, styleSheet, typeInfo, ThinZooms(), true, false);

    REQUIRE(paint.fill != nullptr);

    return paint.fill;
  }

  /** A road class and the fill a style sheet is expected to resolve for it. */
  struct ExpectedFill
  {
    const char* type;
    Color     (*color)();
  };

  /** Asserts the resolved fill of every class of the list against its expected fill. */
  void CheckFills(const osmscout::TypeConfigRef& typeConfig,
                  const std::string& styleSheet,
                  const std::vector<ExpectedFill>& expected,
                  const bool fullWidth)
  {
    for (const auto& fill : expected) {
      INFO("style sheet: " << styleSheet << ", type: " << fill.type);

      LineStyleRef style = fullWidth
                             ? ResolveFullWidthFill(typeConfig, styleSheet, RoadType(typeConfig, fill.type))
                             : ResolveThinFill(typeConfig, styleSheet, RoadType(typeConfig, fill.type));

      REQUIRE(ToHex(style) == fill.color().ToHexString());
    }
  }

  /** The daylight fills of the standard style sheet: the lightened road classes and their thin variants. */
  void CheckStandardDaylightFills(const osmscout::TypeConfigRef& typeConfig)
  {
    const std::string styleSheet = "standard.oss";

    CheckFills(typeConfig,
               styleSheet,
               {{.type = "highway_motorway", .color = MotorwayDaylight},
                 {.type = "highway_trunk", .color = TrunkDaylight},
                 {.type = "highway_primary", .color = PrimaryDaylight},
                 {.type = "highway_secondary", .color = SecondaryDaylight}},
               true);

    CheckFills(typeConfig,
               styleSheet,
               {{.type = "highway_motorway", .color = MotorwayThinDaylight},
                 {.type = "highway_trunk", .color = TrunkThinDaylight},
                 {.type = "highway_primary", .color = PrimaryThinDaylight}},
               false);
  }

  /** The winter-sports style sheet carries its own warm reds and oranges, but the same blues. */
  void CheckWinterSportsDaylightFills(const osmscout::TypeConfigRef& typeConfig)
  {
    const std::string styleSheet = "winter-sports.oss";

    CheckFills(typeConfig,
               styleSheet,
               {{.type = "highway_motorway", .color = MotorwayDaylight},
                 {.type = "highway_trunk", .color = TrunkDaylight}},
               true);

    CheckFills(typeConfig,
               styleSheet,
               {{.type = "highway_motorway", .color = MotorwayThinDaylight},
                 {.type = "highway_trunk", .color = TrunkThinDaylight}},
               false);
  }

  /** The shield style a style sheet resolves for a way of the given class. */
  osmscout::PathShieldStyleRef ResolveShield(const osmscout::TypeConfigRef& typeConfig,
                                             const std::string& styleSheet,
                                             const osmscout::TypeInfoRef& typeInfo)
  {
    auto                         styleConfig = LoadStyleSheet(typeConfig, styleSheet, true);

    osmscout::FeatureValueBuffer buffer;

    buffer.SetType(typeInfo);

    auto projection = MakeProjection(Magnification(Magnification::magClose));

    return styleConfig->GetWayPathShieldStyle(buffer, projection);
  }

  /** Asserts the background a shield paints its white text on. */
  void CheckShieldBackground(const osmscout::TypeConfigRef& typeConfig,
                             const std::string& styleSheet,
                             const std::string& type,
                             const Color& background)
  {
    auto shield = ResolveShield(typeConfig, styleSheet, RoadType(typeConfig, type));

    INFO("style sheet: " << styleSheet << ", type: " << type);

    REQUIRE(shield != nullptr);

    REQUIRE(shield->GetBgColor().ToHexString() == background.ToHexString());
    REQUIRE(shield->GetTextColor().ToHexString() == Color::FromHexString("#ffffff").ToHexString());
  }

  /** The colour the style sheet resolves for the label of a motorway junction. */
  Color ResolveJunctionLabel(const osmscout::TypeConfigRef& typeConfig,
                             const std::string& styleSheet)
  {
    auto                         styleConfig = LoadStyleSheet(typeConfig, styleSheet, true);
    auto                         typeInfo = RoadType(typeConfig, "highway_motorway_junction");

    osmscout::FeatureValueBuffer buffer;

    buffer.SetType(typeInfo);

    // The junction rule is declared for the suburb zoom and closer.
    auto                                projection = MakeProjection(Magnification(Magnification::magSuburb));

    std::vector<osmscout::TextStyleRef> styles;

    styleConfig->GetNodeTextStyles(buffer, projection, styles);

    REQUIRE(styles.size() == 1);

    return styles.at(0)->GetTextColor();
  }

  /** The casing and the fill of the cased motorway paint, as the style sheet resolves them. */
  RoadPaint ResolveCasedMotorway(const osmscout::TypeConfigRef& typeConfig,
                                 const std::string& styleSheet,
                                 const bool daylight)
  {
    RoadPaint paint = ResolveRoadPaint(typeConfig,
                                       styleSheet,
                                       RoadType(typeConfig, "highway_motorway"),
                                       CasedZooms(),
                                       daylight,
                                       true);

    INFO("style sheet: " << styleSheet);

    REQUIRE(paint.fill != nullptr);
    REQUIRE(paint.outline != nullptr);

    return paint;
  }
}

TEST_CASE("The daylight road fills are the lightened values of the style sheets")
{
  auto typeConfig = LoadTypeConfig();

  SECTION("the standard style sheet")
  {
    CheckStandardDaylightFills(typeConfig);
  }

  SECTION("the winter-sports style sheet takes the same motorway and trunk fills")
  {
    CheckWinterSportsDaylightFills(typeConfig);
  }
}

TEST_CASE("The highway shields keep white text on a darkened road fill")
{
  auto typeConfig = LoadTypeConfig();

  for (const char* styleSheet : ROAD_STYLE_SHEETS) {
    CheckShieldBackground(typeConfig, styleSheet, "highway_motorway", MotorwayShieldDaylight());
    CheckShieldBackground(typeConfig, styleSheet, "highway_trunk", TrunkShieldDaylight());
  }

  // The primary class is the only one of the two style sheets whose fill differs (winter-sports
  // paints its own warm red), so its shield is asserted for the standard style sheet alone.
  CheckShieldBackground(typeConfig, "standard.oss", "highway_primary", PrimaryShieldDaylight());
}

TEST_CASE("The motorway junction label keeps a step from the road it is drawn on")
{
  auto  typeConfig = LoadTypeConfig();

  Color label = ResolveJunctionLabel(typeConfig, "standard.oss");

  REQUIRE(label.ToHexString() == JunctionLabelDaylight().ToHexString());
  REQUIRE(label.ToHexString() != MotorwayDaylight().ToHexString());
}

TEST_CASE("The cased road outline stays darker than its fill")
{
  auto typeConfig = LoadTypeConfig();

  for (const char* styleSheet : ROAD_STYLE_SHEETS) {
    RoadPaint paint = ResolveCasedMotorway(typeConfig, styleSheet, true);

    // The outline is the wider stroke and is painted below the fill: the painter orders the
    // strokes of a way by their priority.
    REQUIRE(paint.outline->GetLineColor() != paint.fill->GetLineColor());
    REQUIRE(paint.outline->GetDisplayWidth() > paint.fill->GetDisplayWidth());
    REQUIRE(paint.outline->GetPriority() < paint.fill->GetPriority());
  }
}

TEST_CASE("The dark presentation keeps its own road fills")
{
  auto typeConfig = LoadTypeConfig();

  for (const char* styleSheet : ROAD_STYLE_SHEETS) {
    RoadPaint paint = ResolveCasedMotorway(typeConfig, styleSheet, false);

    // The lightened fills are the daylight variant only; a flag change has to stay visible.
    REQUIRE(ToHex(paint.fill) != MotorwayDaylight().ToHexString());
  }
}
