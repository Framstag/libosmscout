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

#include <memory>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include <osmscout/TypeConfig.h>
#include <osmscout/TypeInfoSet.h>
#include <osmscout/util/Magnification.h>
#include <osmscoutmap/LabelLayouterHelper.h>
#include <osmscoutmap/StyleConfig.h>
#include <osmscoutmap/Styles.h>

/*
 * Tests for the conservative per-level visibility bounds of a style sheet: the way reach bound
 * is the widest line style of a level and grows with a style sheet of wider line styles, the
 * icon reach bound is the widest icon and symbol of a level, and the label extent bound covers
 * the extent of a label and grows with its text and font size.
 *
 * The bounds of a level are an upper bound of the visual reach of the styles of that level, so
 * a painter may reject objects that cannot be visible with them (specs
 * map-painter-way-culling, map-painter-point-object-culling and map-painter-label-culling).
 *
 * The tests build the style sheets programmatically, so they need no style sheet file and no
 * database.
 */

namespace {

  struct TestTypes
  {
    osmscout::TypeConfigRef typeConfig;
    osmscout::TypeInfoRef   wayType;
    osmscout::TypeInfoRef   nodeType;
  };

  TestTypes MakeTypes()
  {
    TestTypes types;

    types.typeConfig=std::make_shared<osmscout::TypeConfig>();

    types.wayType=std::make_shared<osmscout::TypeInfo>("test_way");
    types.wayType->CanBeWay(true);
    types.typeConfig->RegisterType(types.wayType);

    types.nodeType=std::make_shared<osmscout::TypeInfo>("test_node");
    types.nodeType->CanBeNode(true);
    types.typeConfig->RegisterType(types.nodeType);

    return types;
  }

  /**
   * Style sheet with a line style for the way type and an icon style for the node type. The
   * level ranges make it possible to give levels different line widths and icons.
   */
  osmscout::StyleConfigRef MakeStyles(const TestTypes& types,
                                      double lineWidth,
                                      size_t lineMinLevel,
                                      size_t lineMaxLevel,
                                      unsigned int iconWidth,
                                      unsigned int iconHeight,
                                      const osmscout::SymbolRef& symbol)
  {
    auto                  styleConfig=std::make_shared<osmscout::StyleConfig>(types.typeConfig);

    osmscout::TypeInfoSet wayTypes(*types.typeConfig);

    wayTypes.Set(types.wayType);

    osmscout::StyleFilter wayFilter;

    wayFilter.SetTypes(wayTypes);
    wayFilter.SetMinLevel(lineMinLevel);
    wayFilter.SetMaxLevel(lineMaxLevel);

    osmscout::LinePartialStyle lineStyle;

    lineStyle.SetColorValue(osmscout::LineStyle::attrLineColor,osmscout::Color(0.0,0.0,1.0));
    lineStyle.SetDoubleValue(osmscout::LineStyle::attrWidth,lineWidth);
    styleConfig->AddWayLineStyle(wayFilter,lineStyle);

    osmscout::TypeInfoSet nodeTypes(*types.typeConfig);

    nodeTypes.Set(types.nodeType);

    osmscout::StyleFilter iconFilter;

    iconFilter.SetTypes(nodeTypes);

    osmscout::IconPartialStyle iconStyle;

    iconStyle.style->SetIconName("test_icon");
    iconStyle.style->SetWidth(iconWidth);
    iconStyle.style->SetHeight(iconHeight);

    if (symbol) {
      iconStyle.style->SetSymbol(symbol);
    }

    styleConfig->AddNodeIconStyle(iconFilter,iconStyle);

    styleConfig->Postprocess();

    return styleConfig;
  }

  osmscout::Magnification Level(size_t level)
  {
    return osmscout::Magnification(osmscout::MagnificationLevel(level));
  }
}

/**
 * The reach of the ways of a level is the widest line style the level can resolve (spec
 * map-painter-way-culling, requirement "A way that cannot be visible is rejected before its
 * preparation").
 */
TEST_CASE("The way reach bound is the widest line style of a level","[StyleConfigVisibilityBounds]")
{
  const auto types=MakeTypes();

  // 4.0 at the levels 0-9, 11.0 at the levels 10-19
  auto                  styleConfig=std::make_shared<osmscout::StyleConfig>(types.typeConfig);

  osmscout::TypeInfoSet wayTypes(*types.typeConfig);

  wayTypes.Set(types.wayType);

  osmscout::StyleFilter narrowFilter;

  narrowFilter.SetTypes(wayTypes);
  narrowFilter.SetMinLevel(0);
  narrowFilter.SetMaxLevel(9);

  osmscout::LinePartialStyle narrowStyle;

  narrowStyle.SetColorValue(osmscout::LineStyle::attrLineColor,osmscout::Color(0.0,0.0,1.0));
  narrowStyle.SetDoubleValue(osmscout::LineStyle::attrWidth,4.0);
  narrowStyle.SetDoubleValue(osmscout::LineStyle::attrDisplayWidth,1.0);
  styleConfig->AddWayLineStyle(narrowFilter,narrowStyle);

  osmscout::StyleFilter wideFilter;

  wideFilter.SetTypes(wayTypes);
  wideFilter.SetMinLevel(10);
  wideFilter.SetMaxLevel(19);

  osmscout::LinePartialStyle wideStyle;

  wideStyle.SetColorValue(osmscout::LineStyle::attrLineColor,osmscout::Color(0.0,0.0,1.0));
  wideStyle.SetDoubleValue(osmscout::LineStyle::attrWidth,11.0);
  wideStyle.SetDoubleValue(osmscout::LineStyle::attrDisplayWidth,2.0);
  styleConfig->AddWayLineStyle(wideFilter,wideStyle);

  styleConfig->Postprocess();

  REQUIRE(styleConfig->GetVisibilityBounds(Level(5)).maxWayLineWidth==4.0);
  REQUIRE(styleConfig->GetVisibilityBounds(Level(5)).maxWayDisplayWidth==1.0);
  REQUIRE(styleConfig->GetVisibilityBounds(Level(15)).maxWayLineWidth==11.0);
  REQUIRE(styleConfig->GetVisibilityBounds(Level(15)).maxWayDisplayWidth==2.0);

  // A level of the style sheet without a line style has no reach at all, so a way of that
  // level cannot draw anything
  REQUIRE(styleConfig->GetVisibilityBounds(Level(20)).maxWayLineWidth==0.0);
  REQUIRE(styleConfig->GetVisibilityBounds(Level(20)).maxWayDisplayWidth==0.0);
}

/**
 * The way reach bound follows the style sheet: a style sheet with wider line styles has a
 * wider bound, so a rejection built on the bound can never be more aggressive than the
 * per-line-style decision it precedes (spec map-painter-way-culling, requirement "A way that
 * cannot be visible is rejected before its preparation").
 */
TEST_CASE("The way reach bound grows with the line widths of the style sheet","[StyleConfigVisibilityBounds]")
{
  const auto types=MakeTypes();

  auto       narrow=MakeStyles(types,4.0,0,19,14,14,nullptr);
  auto       wide=MakeStyles(types,1000.0,0,19,14,14,nullptr);

  REQUIRE(narrow->GetVisibilityBounds(Level(12)).maxWayLineWidth==4.0);
  REQUIRE(wide->GetVisibilityBounds(Level(12)).maxWayLineWidth==1000.0);
  REQUIRE(wide->GetVisibilityBounds(Level(12)).maxWayLineWidth>
          narrow->GetVisibilityBounds(Level(12)).maxWayLineWidth);
}

/**
 * The reach of the point objects of a level is the widest icon and symbol the level can
 * resolve (spec map-painter-point-object-culling, requirement "A point object that cannot be
 * visible is rejected before its preparation").
 */
TEST_CASE("The icon reach bound is the widest icon and symbol of a level","[StyleConfigVisibilityBounds]")
{
  const auto types=MakeTypes();

  auto       symbol=std::make_shared<osmscout::Symbol>("test_symbol",
                                                       osmscout::Symbol::ProjectionMode::MAP);

  auto       styleConfig=MakeStyles(types,4.0,0,19,24,18,symbol);

  const auto bounds=styleConfig->GetVisibilityBounds(Level(12));

  REQUIRE(bounds.maxIconWidth==24.0);
  REQUIRE(bounds.maxIconHeight==18.0);

  REQUIRE(bounds.symbols.size()==1);
  REQUIRE(bounds.symbols.front().get()==symbol.get());
}

/**
 * The label extent bound covers the extent of a label and grows with the number of characters,
 * with the number of words (wrapping) and with the font size (spec map-painter-label-culling,
 * requirement "A label that cannot intersect the view is neither measured nor stored nor laid
 * out").
 */
TEST_CASE("The label extent bound grows with the text and the font size","[StyleConfigVisibilityBounds]")
{
  constexpr double fontSizePixel=12.0;

  // No text or no font size has no extent
  REQUIRE(osmscout::GetLabelExtentBound(0,0,fontSizePixel)==0.0);
  REQUIRE(osmscout::GetLabelExtentBound(5,1,0.0)==0.0);

  double shortText=osmscout::GetLabelExtentBound(5,1,fontSizePixel);
  double longText=osmscout::GetLabelExtentBound(50,1,fontSizePixel);
  double manyWords=osmscout::GetLabelExtentBound(5,50,fontSizePixel);
  double largeFont=osmscout::GetLabelExtentBound(5,1,2*fontSizePixel);

  REQUIRE(shortText>0.0);
  REQUIRE(longText>shortText);
  REQUIRE(manyWords>shortText);
  REQUIRE(largeFont>shortText);

  // The bound is an upper bound of half of the larger side of the label rectangle, so it
  // covers at least the width of an em box per character and the height of an em box per line.
  REQUIRE(shortText>=5*fontSizePixel/2.0);
  REQUIRE(manyWords>=51*fontSizePixel/2.0);

  // Words are counted as sequences of non-whitespace characters
  REQUIRE(osmscout::CountLabelWords("")==0);
  REQUIRE(osmscout::CountLabelWords("a")==1);
  REQUIRE(osmscout::CountLabelWords("a b")==2);
  REQUIRE(osmscout::CountLabelWords("  a  b\tc\nd ")==4);
}
