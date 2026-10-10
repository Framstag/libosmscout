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

#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <osmscout/GeoCoord.h>
#include <osmscout/Node.h>
#include <osmscout/ObjectRef.h>
#include <osmscout/TypeConfig.h>
#include <osmscout/projection/MercatorProjection.h>
#include <osmscout/util/Magnification.h>
#include <osmscout/util/Transformation.h>
#include <osmscoutmap/MapData.h>
#include <osmscoutmap/MapParameter.h>
#include <osmscoutmap/MapPainterNoOp.h>
#include <osmscoutmap/StyleConfig.h>
#include <osmscoutmap/Styles.h>

/*
 * Tests for the icon/symbol preference of the point label stage: a style entry may carry both a
 * raster icon name and a vector symbol, and the preference decides which one is drawn. Without the
 * preference the raster icon keeps its precedence and the symbol stays the fallback.
 *
 * The tests drive the production path (the frame preparation up to and including the node label
 * step) with a no-op painter that decides whether the icon image counts as available and records
 * the label data the stage registers, so they need no backend, no image file and no database.
 */

namespace {

  struct TestTypes
  {
    osmscout::TypeConfigRef typeConfig;
    osmscout::TypeInfoRef   nodeType;
  };

  TestTypes MakeTypes()
  {
    TestTypes types;

    types.typeConfig=std::make_shared<osmscout::TypeConfig>();

    types.nodeType=std::make_shared<osmscout::TypeInfo>("test_node");
    types.nodeType->CanBeNode(true);
    types.typeConfig->RegisterType(types.nodeType);

    return types;
  }

  constexpr double SymbolSize=5.0;

  /**
   * A symbol with a measurable box, so a test can tell the extent of the symbol from the extent of
   * an icon image.
   */
  osmscout::SymbolRef MakeSymbol()
  {
    auto symbol=std::make_shared<osmscout::Symbol>("test_symbol",
                                                  osmscout::Symbol::ProjectionMode::MAP);

    symbol->AddPrimitive(std::make_shared<osmscout::RectanglePrimitive>(osmscout::Vertex2D(0,0),
                                                                       SymbolSize,
                                                                       SymbolSize,
                                                                       osmscout::FillStyleRef(),
                                                                       osmscout::BorderStyleRef()));

    return symbol;
  }

  /**
   * Style sheet with one node icon style: a raster icon name, a symbol, or both, like the shipped
   * `amenity_hospital` and `highway_bus_stop` entries that carry both renderings.
   */
  osmscout::StyleConfigRef MakeStyles(const TestTypes& types,
                                      bool hasIconName,
                                      bool hasSymbol)
  {
    auto styleConfig=std::make_shared<osmscout::StyleConfig>(types.typeConfig);

    osmscout::TypeInfoSet nodeTypes(*types.typeConfig);

    nodeTypes.Set(types.nodeType);

    osmscout::StyleFilter nodeFilter;

    nodeFilter.SetTypes(nodeTypes);

    osmscout::IconPartialStyle iconStyle;

    if (hasIconName) {
      iconStyle.style->SetIconName("test_icon");
    }

    if (hasSymbol) {
      iconStyle.style->SetSymbol(MakeSymbol());
    }

    styleConfig->AddNodeIconStyle(nodeFilter,iconStyle);

    styleConfig->Postprocess();

    return styleConfig;
  }

  /**
   * No-op painter that answers the icon availability question from a field and records the label
   * data the point label stage registers. It counts the icon availability queries, because that
   * query is what makes the renderer look for an image file.
   */
  class TestPainter : public osmscout::MapPainterNoOp
  {
  private:
    bool                             iconAvailable=true;
    size_t                           iconQueries=0;
    bool                             registered=false;
    std::vector<osmscout::LabelData> registeredLabels;

  protected:
    bool HasIcon(const osmscout::StyleConfig& /*styleConfig*/,
                 const osmscout::Projection& /*projection*/,
                 const osmscout::MapParameter& parameter,
                 osmscout::IconStyle& style) override
    {
      iconQueries++;

      if (!iconAvailable) {
        return false;
      }

      style.SetWidth(std::round(parameter.GetIconPixelSize()));
      style.SetHeight(style.GetWidth());

      return true;
    }

    void RegisterRegularLabel(const osmscout::Projection& /*projection*/,
                              const osmscout::MapParameter& /*parameter*/,
                              bool /*basemap*/,
                              const osmscout::ObjectFileRef& /*ref*/,
                              const std::vector<osmscout::LabelData>& labels,
                              const osmscout::Vertex2D& /*position*/,
                              double /*objectWidth*/) override
    {
      registeredLabels=labels;
      registered=true;
    }

    void RegisterContourLabel(const osmscout::Projection& /*projection*/,
                              const osmscout::MapParameter& /*parameter*/,
                              bool /*basemap*/,
                              const osmscout::ObjectFileRef& /*ref*/,
                              const osmscout::PathLabelData& /*label*/,
                              const osmscout::LabelPath& /*labelPath*/) override
    {
      // no code
    }

    void AfterPreprocessingCallback(const osmscout::Projection& /*projection*/,
                                    const osmscout::MapParameter& /*parameter*/,
                                    const std::vector<osmscout::MapData>& /*data*/) override
    {
      // no code
    }

  public:
    void SetIconAvailable(bool available)
    {
      iconAvailable=available;
    }

    size_t IconQueries() const
    {
      return iconQueries;
    }

    const osmscout::LabelData& RegisteredLabel() const
    {
      REQUIRE(registered);
      REQUIRE(registeredLabels.size()==1);

      return registeredLabels[0];
    }
  };

  osmscout::MercatorProjection MakeProjection()
  {
    osmscout::MercatorProjection projection;

    REQUIRE(projection.Set(osmscout::GeoCoord(50.001,8.001),
                           osmscout::Magnification(osmscout::Magnification::magClose),
                           300,
                           400,
                           400));

    return projection;
  }

  osmscout::MapData MakeData(const osmscout::StyleConfigRef& styleConfig)
  {
    osmscout::MapData data;

    data.styleConfig=styleConfig;

    return data;
  }

  osmscout::NodeRef MakeNode(const osmscout::TypeInfoRef& type,
                             const osmscout::GeoCoord& coord)
  {
    auto node=std::make_shared<osmscout::Node>();

    node->SetCoords(coord);

    osmscout::FeatureValueBuffer buffer;

    buffer.SetType(type);

    node->SetFeatures(buffer);

    return node;
  }

  /**
   * Prepares the frame, i.e. all steps up to and including the node label preparation, without
   * drawing. The point object labels are registered in the node label step, so that step is part of
   * the range.
   */
  void PrepareFrame(TestPainter& painter,
                    const osmscout::MercatorProjection& projection,
                    const osmscout::MapParameter& parameter,
                    const osmscout::MapData& data)
  {
    REQUIRE(painter.Draw(projection,
                         parameter,
                         std::vector<osmscout::MapData> {data},
                         osmscout::Initialize,
                         osmscout::PrepareNodeLabels));
  }

  /**
   * Prepares one frame with a single node of a style carrying the given renderings and returns the
   * label data the stage registered.
   */
  osmscout::LabelData PrepareOneNode(TestPainter& painter,
                                     bool hasIconName,
                                     bool hasSymbol,
                                     bool preferSymbolIcons,
                                     bool iconAvailable=true)
  {
    const auto               types=MakeTypes();
    const osmscout::StyleConfigRef styleConfig=MakeStyles(types,hasIconName,hasSymbol);
    const auto               projection=MakeProjection();
    osmscout::MapParameter   parameter;

    parameter.SetPreferSymbolIcons(preferSymbolIcons);

    painter.SetIconAvailable(iconAvailable);

    osmscout::MapData data=MakeData(styleConfig);

    data.nodes.push_back(MakeNode(types.nodeType,projection.GetCenter()));

    PrepareFrame(painter,projection,parameter,data);

    return painter.RegisteredLabel();
  }
}

TEST_CASE("An icon and symbol entry draws the icon by default","[MapPainterIconSymbolPreference]")
{
  TestPainter painter;

  const auto label=PrepareOneNode(painter,true,true,false);

  REQUIRE(label.type==osmscout::LabelData::Type::Icon);
  REQUIRE(painter.IconQueries()==1);
}

TEST_CASE("The preference draws the symbol of an icon and symbol entry","[MapPainterIconSymbolPreference]")
{
  TestPainter           painter;
  const auto            projection=MakeProjection();
  const auto            label=PrepareOneNode(painter,true,true,true);

  REQUIRE(label.type==osmscout::LabelData::Type::Symbol);

  // The extent of the drawn element comes from the symbol, not from an icon image.
  REQUIRE(label.iconWidth==Catch::Approx(projection.ConvertWidthToPixel(SymbolSize)));
  REQUIRE(label.iconHeight==Catch::Approx(projection.ConvertWidthToPixel(SymbolSize)));
}

TEST_CASE("The preference does not look for an icon image","[MapPainterIconSymbolPreference]")
{
  TestPainter painter;

  PrepareOneNode(painter,true,true,true);

  REQUIRE(painter.IconQueries()==0);
}

TEST_CASE("The preference keeps an icon-only entry on its raster icon","[MapPainterIconSymbolPreference]")
{
  TestPainter painter;

  const auto label=PrepareOneNode(painter,true,false,true);

  REQUIRE(label.type==osmscout::LabelData::Type::Icon);
  REQUIRE(painter.IconQueries()==1);
}

TEST_CASE("A symbol-only entry draws its symbol with and without the preference","[MapPainterIconSymbolPreference]")
{
  for (bool preferSymbolIcons : {false,true}) {
    TestPainter painter;

    const auto label=PrepareOneNode(painter,false,true,preferSymbolIcons);

    REQUIRE(label.type==osmscout::LabelData::Type::Symbol);
    REQUIRE(painter.IconQueries()==0);
  }
}

TEST_CASE("An unavailable icon image falls back to the symbol without the preference","[MapPainterIconSymbolPreference]")
{
  TestPainter painter;

  const auto label=PrepareOneNode(painter,true,true,false,false);

  REQUIRE(label.type==osmscout::LabelData::Type::Symbol);
  REQUIRE(painter.IconQueries()==1);
}
