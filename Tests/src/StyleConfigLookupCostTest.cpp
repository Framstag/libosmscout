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

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include <osmscout/TypeConfig.h>
#include <osmscout/feature/NameFeature.h>
#include <osmscout/projection/MercatorProjection.h>
#include <osmscout/util/Color.h>
#include <osmscout/util/Magnification.h>
#include <osmscoutmap/StyleConfig.h>
#include <osmscoutmap/Styles.h>

/*
 * Tests for what a loaded style configuration resolves and what building it costs (spec
 * style-configuration).
 *
 * The tests build the type configuration and the style sheet themselves, so they need neither a
 * style sheet file in the repository nor a database: the type configuration starts with one area
 * type, and the style sheets are written into the test temporary directory. The projection carries
 * an explicit dpi, because the filters of a resolved style are evaluated against its meterInMM and
 * meterInPixel.
 *
 * The style sheets declare their rules in one `[MAG detail-]` block, so a resolution at the detail
 * zoom resolves them and a resolution at the suburb zoom - one level farther out - resolves none of
 * them.
 */

namespace {

  using osmscout::Color;
  using osmscout::FillStyleRef;
  using osmscout::Magnification;
  using osmscout::TextStyleRef;

  /** The projected view the styles are resolved for. */
  constexpr double PROJECTION_LAT=50.001;
  constexpr double PROJECTION_LON=8.001;
  constexpr size_t PROJECTION_WIDTH=300;
  constexpr size_t PROJECTION_HEIGHT=400;
  /** A screen dpi, so a SIZE filter of a style sheet resolves against a real meterInMM. */
  constexpr double PROJECTION_DPI=96.0;

  /** The type the style sheets of these tests declare their rules for. */
  constexpr const char* AREA_TYPE_NAME="test_area";

  struct TestTypes
  {
    osmscout::TypeConfigRef typeConfig;
    osmscout::TypeInfoRef   areaType;
  };

  std::string GetEnv(const char* name,
                     const std::string& fallback)
  {
    // NOLINTNEXTLINE(concurrency-mt-unsafe) read once, before any test runs, like the sibling tests
    const char * value=std::getenv(name);

    return value!=nullptr ? std::string(value) : fallback;
  }

  std::filesystem::path TempDir()
  {
    return std::filesystem::path(GetEnv("TESTS_TMP_DIR",
                                        std::filesystem::temp_directory_path().string()));
  }

  /**
   * The type configuration of the tests: one area type, plus the given number of further types that
   * no style sheet of these tests references.
   */
  TestTypes MakeTypes(size_t unreferencedTypes=0)
  {
    TestTypes types;

    types.typeConfig=std::make_shared<osmscout::TypeConfig>();

    types.areaType=std::make_shared<osmscout::TypeInfo>(AREA_TYPE_NAME);
    types.areaType->CanBeArea(true);
    types.typeConfig->RegisterType(types.areaType);

    for (size_t i=0; i<unreferencedTypes; i++) {
      auto type=std::make_shared<osmscout::TypeInfo>("test_unreferenced_"+std::to_string(i));

      type->CanBeArea(true);
      types.typeConfig->RegisterType(type);
    }

    return types;
  }

  /** Writes the body of a style sheet and returns its path. */
  std::filesystem::path WriteStyleSheet(const std::string& name,
                                        const std::string& body)
  {
    std::filesystem::path path=TempDir() / name;

    std::filesystem::create_directories(path.parent_path());

    std::ofstream file(path);

    file << "OSS" << std::endl;
    file << body;
    file << "END" << std::endl;

    return path;
  }

  osmscout::StyleConfigRef LoadStyleSheet(const osmscout::TypeConfigRef& typeConfig,
                                          const std::filesystem::path& styleSheet)
  {
    auto styleConfig=std::make_shared<osmscout::StyleConfig>(typeConfig);

    REQUIRE(styleConfig->Load(styleSheet.string()));

    return styleConfig;
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

  osmscout::FeatureValueBuffer MakeBuffer(const osmscout::TypeInfoRef& typeInfo)
  {
    osmscout::FeatureValueBuffer buffer;

    buffer.SetType(typeInfo);

    return buffer;
  }

  /**
   * A buffer of the given type carrying a name value, which is what a rule that selects by the name
   * feature is matched against.
   */
  osmscout::FeatureValueBuffer MakeNamedBuffer(const osmscout::TypeInfoRef& typeInfo)
  {
    osmscout::FeatureValueBuffer buffer;
    size_t                      nameIndex=0;

    buffer.SetType(typeInfo);

    REQUIRE(typeInfo->GetFeature("Name",nameIndex));

    auto *nameValue=static_cast<osmscout::NameFeatureValue*>(buffer.AllocateValue(nameIndex));

    nameValue->SetName("Test");

    return buffer;
  }

  /** The fill style the configuration resolves for the given object at the given zoom. */
  FillStyleRef ResolveFill(const osmscout::StyleConfigRef& styleConfig,
                           const osmscout::TypeInfoRef& typeInfo,
                           const osmscout::FeatureValueBuffer& buffer,
                           const Magnification& magnification)
  {
    auto projection=MakeProjection(magnification);

    return styleConfig->GetAreaFillStyle(typeInfo,
                                         buffer,
                                         projection);
  }

  /** The fill style the configuration resolves for the area type at the given zoom. */
  FillStyleRef ResolveFill(const osmscout::StyleConfigRef& styleConfig,
                           const osmscout::TypeInfoRef& typeInfo,
                           const Magnification& magnification)
  {
    return ResolveFill(styleConfig,
                       typeInfo,
                       MakeBuffer(typeInfo),
                       magnification);
  }

  /** The text styles the configuration resolves for the area type at the given zoom. */
  std::vector<TextStyleRef> ResolveTexts(const osmscout::StyleConfigRef& styleConfig,
                                         const osmscout::TypeInfoRef& typeInfo,
                                         const Magnification& magnification)
  {
    auto                      buffer=MakeBuffer(typeInfo);
    auto                      projection=MakeProjection(magnification);

    std::vector<TextStyleRef> textStyles;

    styleConfig->GetAreaTextStyles(typeInfo,
                                   buffer,
                                   projection,
                                   textStyles);

    return textStyles;
  }

  /** A style sheet with one fill rule and one text rule for the area type of the tests. */
  std::string FillAndTextStyleSheet()
  {
    return "  STYLE\n"
           "    [MAG detail-] {\n"
           "      [TYPE test_area] AREA { color: #ff0000; }\n"
           "      [TYPE test_area] AREA.TEXT { label: Name.name; color: #0000ff; size: 1.0; }\n"
           "    }\n";
  }

  osmscout::FeatureRef GetFeatureRef(const osmscout::TypeConfigRef& typeConfig,
                                     const std::string& name)
  {
    osmscout::FeatureRef feature=typeConfig->GetFeature(name);

    REQUIRE(feature!=nullptr);

    return feature;
  }

  /**
   * The type configuration of the feature case: the area type carries the name feature, and one
   * further area type does not carry it.
   */
  TestTypes MakeTypesWithNamedArea()
  {
    TestTypes types;

    types.typeConfig=std::make_shared<osmscout::TypeConfig>();

    types.areaType=std::make_shared<osmscout::TypeInfo>(AREA_TYPE_NAME);
    types.areaType->CanBeArea(true);
    types.areaType->AddFeature(GetFeatureRef(types.typeConfig,"Name"));
    types.typeConfig->RegisterType(types.areaType);

    auto unnamedType=std::make_shared<osmscout::TypeInfo>("test_unnamed_area");

    unnamedType->CanBeArea(true);
    types.typeConfig->RegisterType(unnamedType);

    return types;
  }
}

/**
 * The fill and the text style a style sheet declares for a type are what a resolution returns for
 * that type (spec style-configuration, requirement "Style resolution of a referenced type is
 * unchanged").
 */
TEST_CASE("A stylesheet resolves the declared fill and text styles of a referenced type","[StyleConfigLookupCost]")
{
  const auto types=MakeTypes();
  auto       styleConfig=LoadStyleSheet(types.typeConfig,
                                        WriteStyleSheet("StyleConfigLookupCostFillAndText.oss",
                                                        FillAndTextStyleSheet()));

  FillStyleRef fillStyle=ResolveFill(styleConfig,
                                     types.areaType,
                                     Magnification(Magnification::magDetail));

  REQUIRE(fillStyle!=nullptr);
  REQUIRE(fillStyle->GetFillColor().ToHexString()==Color::FromHexString("#ff0000").ToHexString());

  std::vector<TextStyleRef> textStyles=ResolveTexts(styleConfig,
                                                   types.areaType,
                                                   Magnification(Magnification::magDetail));

  REQUIRE(textStyles.size()==1);
  REQUIRE(textStyles.front()->GetTextColor().ToHexString()==Color::FromHexString("#0000ff").ToHexString());
  REQUIRE(textStyles.front()->GetSize()==1.0);
}

/**
 * A rule that starts at a magnification level resolves nothing farther out than that level (spec
 * style-configuration, requirement "Style resolution of a referenced type is unchanged").
 */
TEST_CASE("A rule with a minimum magnification level resolves nothing below it","[StyleConfigLookupCost]")
{
  const auto types=MakeTypes();
  auto       styleConfig=LoadStyleSheet(types.typeConfig,
                                        WriteStyleSheet("StyleConfigLookupCostLevel.oss",
                                                        FillAndTextStyleSheet()));

  // The rule is declared for the detail zoom and closer; the suburb zoom is one level farther out
  REQUIRE(ResolveFill(styleConfig,
                      types.areaType,
                      Magnification(Magnification::magSuburb))==nullptr);
  REQUIRE(ResolveTexts(styleConfig,
                       types.areaType,
                       Magnification(Magnification::magSuburb)).empty());

  // The same configuration still resolves the rule at its own level
  REQUIRE(ResolveFill(styleConfig,
                      types.areaType,
                      Magnification(Magnification::magDetail))!=nullptr);
}

/**
 * Two rules of a style sheet for one type at one level compose into one style that carries the
 * attributes of both (spec style-configuration, requirement "Style resolution of a referenced type
 * is unchanged").
 */
TEST_CASE("Two rules for one type compose the declared attributes","[StyleConfigLookupCost]")
{
  const auto types=MakeTypes();
  auto       styleConfig=LoadStyleSheet(types.typeConfig,
                                        WriteStyleSheet("StyleConfigLookupCostCompose.oss",
                                                        "  STYLE\n"
                                                        "    [MAG detail-] {\n"
                                                        "      [TYPE test_area] AREA.TEXT { label: Name.name; color: #0000ff; size: 1.0; }\n"
                                                        "      [TYPE test_area] AREA.TEXT { label: Name.name; priority: 5; }\n"
                                                        "    }\n"));

  std::vector<TextStyleRef> textStyles=ResolveTexts(styleConfig,
                                                   types.areaType,
                                                   Magnification(Magnification::magDetail));

  REQUIRE(textStyles.size()==1);

  // The colour comes from the first rule, the priority the second one adds, and the size the first
  // rule declares is not lost by the second one
  REQUIRE(textStyles.front()->GetTextColor().ToHexString()==Color::FromHexString("#0000ff").ToHexString());
  REQUIRE(textStyles.front()->GetPriority()==5);
  REQUIRE(textStyles.front()->GetSize()==1.0);
}

/**
 * The build diagnostic of a loaded configuration reports the work the build did, so the cases below
 * can assert it (spec style-configuration, requirement "Style-configuration build cost is
 * observable").
 */
TEST_CASE("The build diagnostic reports what a style configuration prepared","[StyleConfigLookupCost]")
{
  const auto types=MakeTypes();
  auto       styleConfig=LoadStyleSheet(types.typeConfig,
                                        WriteStyleSheet("StyleConfigLookupCostDiagnostics.oss",
                                                        FillAndTextStyleSheet()));

  const auto& diagnostics=styleConfig->GetBuildDiagnostics();

  INFO("slots " << diagnostics.preparedSlots
                << ", evaluations " << diagnostics.typeConditionEvaluations
                << ", table bytes " << diagnostics.tableBytes
                << ", type set bytes " << diagnostics.typeSetBytes);

  REQUIRE(diagnostics.preparedSlots>0);
  REQUIRE(diagnostics.typeConditionEvaluations>0);
  REQUIRE(diagnostics.tableBytes>0);
  REQUIRE(diagnostics.typeSetBytes>0);
}

/**
 * Types the style sheet does not reference add no work to the build: neither prepares a slot nor is
 * evaluated against a type condition (spec style-configuration, requirement "Defined but unreferenced
 * types add no build work").
 */
TEST_CASE("Defined but unreferenced types add no build work","[StyleConfigLookupCost]")
{
  const auto smallTypes=MakeTypes();
  const auto largeTypes=MakeTypes(50);

  auto       small=LoadStyleSheet(smallTypes.typeConfig,
                                  WriteStyleSheet("StyleConfigLookupCostScalingSmall.oss",
                                                  FillAndTextStyleSheet()));
  auto       large=LoadStyleSheet(largeTypes.typeConfig,
                                  WriteStyleSheet("StyleConfigLookupCostScalingLarge.oss",
                                                  FillAndTextStyleSheet()));

  const auto& smallDiagnostics=small->GetBuildDiagnostics();
  const auto& largeDiagnostics=large->GetBuildDiagnostics();

  INFO("small type set: slots " << smallDiagnostics.preparedSlots
                                << ", evaluations " << smallDiagnostics.typeConditionEvaluations
                                << ", table bytes " << smallDiagnostics.tableBytes
                                << ", type set bytes " << smallDiagnostics.typeSetBytes);
  INFO("large type set: slots " << largeDiagnostics.preparedSlots
                                << ", evaluations " << largeDiagnostics.typeConditionEvaluations
                                << ", table bytes " << largeDiagnostics.tableBytes
                                << ", type set bytes " << largeDiagnostics.typeSetBytes);

  REQUIRE(smallDiagnostics.preparedSlots==largeDiagnostics.preparedSlots);
  REQUIRE(smallDiagnostics.typeConditionEvaluations==largeDiagnostics.typeConditionEvaluations);
  REQUIRE(smallDiagnostics.tableBytes==largeDiagnostics.tableBytes);

  // The per-level type sets stay sized by the defined type count until the set representation is
  // changed (a separate change), so their size is reported, not asserted here
  INFO("type set bytes: " << smallDiagnostics.typeSetBytes << " vs " << largeDiagnostics.typeSetBytes);
}

/**
 * A type the style sheet does not reference resolves to no style, in a debug build as well as in a
 * release build (spec style-configuration, requirement "A type the stylesheet does not reference
 * resolves to no style").
 */
TEST_CASE("A type the stylesheet does not reference resolves to no style","[StyleConfigLookupCost]")
{
  const auto types=MakeTypes(2);
  auto       styleConfig=LoadStyleSheet(types.typeConfig,
                                        WriteStyleSheet("StyleConfigLookupCostUnreferenced.oss",
                                                        FillAndTextStyleSheet()));

  auto       unreferencedType=types.typeConfig->GetTypeInfo("test_unreferenced_0");

  REQUIRE(unreferencedType!=nullptr);

  REQUIRE(ResolveFill(styleConfig,
                      unreferencedType,
                      Magnification(Magnification::magDetail))==nullptr);
  REQUIRE(ResolveTexts(styleConfig,
                       unreferencedType,
                       Magnification(Magnification::magDetail)).empty());

  // The same configuration still serves the type the sheet does reference
  REQUIRE(ResolveFill(styleConfig,
                      types.areaType,
                      Magnification(Magnification::magDetail))!=nullptr);
}

/**
 * A rule that selects by a feature covers exactly the types that carry the feature (spec
 * style-configuration, requirement "A rule that selects by feature covers the types that carry the
 * feature").
 */
TEST_CASE("A rule that selects by feature covers the types that carry the feature","[StyleConfigLookupCost]")
{
  const auto types=MakeTypesWithNamedArea();
  auto       styleConfig=LoadStyleSheet(types.typeConfig,
                                        WriteStyleSheet("StyleConfigLookupCostFeature.oss",
                                                        "  STYLE\n"
                                                        "    [MAG detail-] {\n"
                                                        "      [FEATURE Name] AREA { color: #00ff00; }\n"
                                                        "    }\n"));

  auto       unnamedType=types.typeConfig->GetTypeInfo("test_unnamed_area");

  REQUIRE(unnamedType!=nullptr);

  FillStyleRef namedFill=ResolveFill(styleConfig,
                                     types.areaType,
                                     MakeNamedBuffer(types.areaType),
                                     Magnification(Magnification::magDetail));

  REQUIRE(namedFill!=nullptr);
  REQUIRE(namedFill->GetFillColor().ToHexString()==Color::FromHexString("#00ff00").ToHexString());

  REQUIRE(ResolveFill(styleConfig,
                      unnamedType,
                      Magnification(Magnification::magDetail))==nullptr);
}
