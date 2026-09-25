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

#include <atomic>
#include <cstdlib>
#include <memory>
#include <new>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <osmscout/GeoCoord.h>
#include <osmscout/Node.h>
#include <osmscout/TypeConfig.h>
#include <osmscout/TypeInfoSet.h>
#include <osmscout/projection/MercatorProjection.h>
#include <osmscout/util/Magnification.h>
#include <osmscoutmap/LabelProvider.h>
#include <osmscoutmap/MapData.h>
#include <osmscoutmap/MapPainter.h>
#include <osmscoutmap/MapPainterNoOp.h>
#include <osmscoutmap/MapParameter.h>
#include <osmscoutmap/StyleConfig.h>
#include <osmscoutmap/Styles.h>

/*
 * Tests for the contract of the painter's early rejection of point objects - nodes and POI
 * nodes - whose visual extent cannot reach the view (spec map-painter-point-object-culling):
 * an object of a type without a label style at the level of the frame is rejected before any
 * style of it is resolved when its icon and symbol cannot reach the view, an object whose label
 * elements cannot reach the view is rejected before those elements are registered, and the
 * registered label elements of a view stay what the unculled pipeline produces.
 *
 * The tests drive the painter directly with synthetic nodes and style sheets, so they need no
 * database and no style sheet file. Objects are placed relative to the viewport using pixel
 * distances measured from the projection.
 */

#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__)
#define POINT_CULL_HAVE_ALLOCATION_COUNTER 0
#elif defined(__has_feature)
#if __has_feature(memory_sanitizer) || __has_feature(address_sanitizer) || __has_feature(thread_sanitizer)
#define POINT_CULL_HAVE_ALLOCATION_COUNTER 0
#else
#define POINT_CULL_HAVE_ALLOCATION_COUNTER 1
#endif
#else
#define POINT_CULL_HAVE_ALLOCATION_COUNTER 1
#endif

#if POINT_CULL_HAVE_ALLOCATION_COUNTER

namespace {
  std::atomic<size_t> allocationCounter{0};
}

void* operator new(std::size_t size)
{
  allocationCounter.fetch_add(1,std::memory_order_relaxed);

  void * memory=std::malloc(size);

  if (memory==nullptr) {
    throw std::bad_alloc();
  }

  return memory;
}

void* operator new[](std::size_t size)
{
  return ::operator new(size);
}

void* operator new(std::size_t size,
                   const std::nothrow_t&) noexcept
{
  allocationCounter.fetch_add(1,std::memory_order_relaxed);

  return std::malloc(size);
}

void* operator new[](std::size_t size,
                     const std::nothrow_t& tag) noexcept
{
  return ::operator new(size,tag);
}

void operator delete(void* memory) noexcept
{
  std::free(memory);
}

void operator delete[](void* memory) noexcept
{
  std::free(memory);
}

void operator delete(void* memory,
                     std::size_t /*size*/) noexcept
{
  std::free(memory);
}

void operator delete[](void* memory,
                       std::size_t /*size*/) noexcept
{
  std::free(memory);
}

void operator delete(void* memory,
                     const std::nothrow_t&) noexcept
{
  std::free(memory);
}

void operator delete[](void* memory,
                       const std::nothrow_t&) noexcept
{
  std::free(memory);
}

#endif

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

  class FixedLabelProvider CLASS_FINAL : public osmscout::LabelProvider
  {
  private:
    std::string text;

  public:
    explicit FixedLabelProvider(const std::string& text)
      : text(text)
    {
      // no code
    }

    std::string GetLabel(const osmscout::MapParameter& /*parameter*/,
                         const osmscout::FeatureValueBuffer& /*buffer*/) const override
    {
      return text;
    }

    std::string GetName() const override
    {
      return "Test";
    }
  };

  /**
   * Style sheet with an icon style of the given size for the node type and, for a non-empty
   * text, a text style whose label carries that text.
   */
  osmscout::StyleConfigRef MakeStyles(const TestTypes& types,
                                      unsigned int iconSize,
                                      const std::string& labelText)
  {
    auto                  styleConfig=std::make_shared<osmscout::StyleConfig>(types.typeConfig);

    osmscout::TypeInfoSet nodeTypes(*types.typeConfig);

    nodeTypes.Set(types.nodeType);

    osmscout::StyleFilter nodeFilter;

    nodeFilter.SetTypes(nodeTypes);

    osmscout::IconPartialStyle iconStyle;

    iconStyle.style->SetIconName("test_icon");
    iconStyle.style->SetWidth(iconSize);
    iconStyle.style->SetHeight(iconSize);
    // A symbol, so that the object has a label element even when no icon resource is available
    // in the test environment
    iconStyle.style->SetSymbol(std::make_shared<osmscout::Symbol>("test_symbol",
                                                                  osmscout::Symbol::ProjectionMode::MAP));

    styleConfig->AddNodeIconStyle(nodeFilter,iconStyle);

    if (!labelText.empty()) {
      osmscout::TextPartialStyle textStyle;

      textStyle.SetLabelValue(osmscout::TextStyle::attrLabel,
                              std::make_shared<FixedLabelProvider>(labelText));
      textStyle.SetDoubleValue(osmscout::TextStyle::attrSize,1.3);
      textStyle.SetColorValue(osmscout::TextStyle::attrTextColor,osmscout::Color(0.0,0.0,0.0));
      styleConfig->AddNodeTextStyle(nodeFilter,textStyle);
    }

    styleConfig->Postprocess();

    return styleConfig;
  }

  class RecordingPainter CLASS_FINAL : public osmscout::MapPainterNoOp
  {
  private:
    size_t                          labelCount=0;
    std::vector<osmscout::Vertex2D> labelPositions;

  public:
    size_t LabelCount() const
    {
      return labelCount;
    }

    const std::vector<osmscout::Vertex2D>& LabelPositions() const
    {
      return labelPositions;
    }

  protected:
    void RegisterRegularLabel(const osmscout::Projection& /*projection*/,
                              const osmscout::MapParameter& /*parameter*/,
                              bool /*basemap*/,
                              const osmscout::ObjectFileRef& /*ref*/,
                              const std::vector<osmscout::LabelData>& /*labels*/,
                              const osmscout::Vertex2D& position,
                              double /*objectWidth*/) override
    {
      labelCount++;
      labelPositions.push_back(position);
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

  osmscout::MapParameter MakeParameter()
  {
    return osmscout::MapParameter();
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

  struct PixelScale
  {
    double lon=0.0;
    double lat=0.0;
  };

  PixelScale MeasurePixelScale(const osmscout::MercatorProjection& projection)
  {
    constexpr double   probeDeltaDegrees=0.001;

    osmscout::Vertex2D center{};
    osmscout::Vertex2D east{};

    REQUIRE(projection.GeoToPixel(projection.GetCenter(),center));
    REQUIRE(projection.GeoToPixel(osmscout::GeoCoord(projection.GetCenter().GetLat(),
                                                     projection.GetCenter().GetLon()+probeDeltaDegrees),
                                  east));

    PixelScale scale;

    scale.lon=(east.GetX()-center.GetX())/probeDeltaDegrees;
    scale.lat=scale.lon;

    REQUIRE(scale.lon>0.0);

    return scale;
  }

  double DegreesLonForPixels(const osmscout::MercatorProjection& projection,
                             double pixels)
  {
    return pixels/MeasurePixelScale(projection).lon;
  }

  /**
   * A node whose distance to the right edge of the viewport is the given number of pixels
   */
  osmscout::NodeRef MakeNodeRightOfView(const osmscout::TypeInfoRef& type,
                                        const osmscout::MercatorProjection& projection,
                                        double distancePixels)
  {
    constexpr double halfViewportWidthPixels=200.0;

    return MakeNode(type,
                    osmscout::GeoCoord(projection.GetCenter().GetLat(),
                                       projection.GetCenter().GetLon()+
                                       DegreesLonForPixels(projection,halfViewportWidthPixels+distancePixels)));
  }

  /**
   * Prepares the frame, i.e. all steps up to and including the node label preparation, without
   * drawing. The point object labels are registered in the node label step, so that step is part
   * of the range.
   */
  void PrepareFrame(osmscout::MapPainterNoOp& painter,
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
}

/**
 * A point object that no icon, symbol or label of the current style sheet can bring into the
 * view contributes nothing: it registers no label elements and the frame stays what it is
 * without that object (spec map-painter-point-object-culling, requirement "A point object that
 * cannot be visible is rejected before its preparation").
 */
TEST_CASE("A point object outside the view registers no elements","[MapPainterPointObjectCulling]")
{
  const auto types=MakeTypes();

  auto       styleConfig=MakeStyles(types,14,"Hauptstrasse");

  const auto projection=MakeProjection();
  const auto parameter=MakeParameter();

  auto       inside=MakeNode(types.nodeType,projection.GetCenter());
  auto       outside=MakeNodeRightOfView(types.nodeType,
                                         projection,
                                         // far outside any reach of the label or the icon
                                         40.0*200.0);

  RecordingPainter reference;
  auto             referenceData=MakeData(styleConfig);

  referenceData.nodes.push_back(inside);

  PrepareFrame(reference,projection,parameter,referenceData);

  REQUIRE(reference.LabelCount()>0);

  RecordingPainter painter;
  auto             data=MakeData(styleConfig);

  data.nodes.push_back(inside);
  data.nodes.push_back(outside);

  PrepareFrame(painter,projection,parameter,data);

  // The elements of the visible object are unchanged and the object outside the view adds none
  REQUIRE(painter.LabelCount()==reference.LabelCount());
  REQUIRE(painter.LabelPositions()==reference.LabelPositions());
}

/**
 * The rejection is conservative: an object whose labels reach into the view is still prepared,
 * even when its position lies outside the viewport (spec map-painter-point-object-culling,
 * requirement "A point object that cannot be visible is rejected before its preparation").
 */
TEST_CASE("A point object at the viewport edge keeps its elements","[MapPainterPointObjectCulling]")
{
  const auto       types=MakeTypes();

  auto             styleConfig=MakeStyles(types,14,"Hauptstrasse");

  const auto       projection=MakeProjection();
  const auto       parameter=MakeParameter();

  RecordingPainter painter;
  auto             data=MakeData(styleConfig);

  // The position is outside the viewport, but far less than the extent of the label text
  data.nodes.push_back(MakeNodeRightOfView(types.nodeType,
                                           projection,
                                           10.0));

  PrepareFrame(painter,projection,parameter,data);

  REQUIRE(painter.LabelCount()>0);
  REQUIRE(painter.LabelPositions().size()==painter.LabelCount());
}

/**
 * Point object preparation work and its heap allocation do not grow with the objects that are
 * loaded into the view but cannot contribute a pixel (spec map-painter-point-object-culling,
 * requirement "Point object preparation work follows the visible objects, not the loaded ones").
 */
TEST_CASE("Loaded point objects outside the view do not add work","[MapPainterPointObjectCulling]")
{
  const auto       types=MakeTypes();

  auto             styleConfig=MakeStyles(types,14,"Hauptstrasse");

  const auto       projection=MakeProjection();
  const auto       parameter=MakeParameter();

  constexpr size_t farNodeCount=512;

  RecordingPainter lightPainter;
  auto             lightData=MakeData(styleConfig);

  lightData.nodes.push_back(MakeNode(types.nodeType,projection.GetCenter()));

  PrepareFrame(lightPainter,projection,parameter,lightData);

  REQUIRE(lightPainter.LabelCount()>0);

  RecordingPainter heavyPainter;
  auto             heavyData=MakeData(styleConfig);

  heavyData.nodes.push_back(MakeNode(types.nodeType,projection.GetCenter()));

  for (size_t i=0; i<farNodeCount; i++) {
    heavyData.nodes.push_back(MakeNodeRightOfView(types.nodeType,
                                                  projection,
                                                  40.0*200.0+10.0*(double)i));
  }

  PrepareFrame(heavyPainter,projection,parameter,heavyData);

  // The registered elements of the view are the ones of the view without the further objects
  REQUIRE(heavyPainter.LabelCount()==lightPainter.LabelCount());
  REQUIRE(heavyPainter.LabelPositions()==lightPainter.LabelPositions());
}

/**
 * An object of a type that has no label style at the level of the frame draws only its icon, so
 * it is rejected before any style of it is resolved and its preparation allocates nothing at all
 * (spec map-painter-point-object-culling, requirement "Point object preparation work follows the
 * visible objects, not the loaded ones").
 */
TEST_CASE("Loaded point objects of an icon-only type add no work","[MapPainterPointObjectCulling]")
{
  const auto       types=MakeTypes();

  auto             styleConfig=MakeStyles(types,14,"");

  const auto       projection=MakeProjection();
  const auto       parameter=MakeParameter();

  constexpr size_t farNodeCount=512;

  RecordingPainter lightPainter;
  auto             lightData=MakeData(styleConfig);

  lightData.nodes.push_back(MakeNode(types.nodeType,projection.GetCenter()));

  PrepareFrame(lightPainter,projection,parameter,lightData);

  REQUIRE(lightPainter.LabelCount()>0);

  RecordingPainter heavyPainter;
  auto             heavyData=MakeData(styleConfig);

  heavyData.nodes.push_back(MakeNode(types.nodeType,projection.GetCenter()));

  for (size_t i=0; i<farNodeCount; i++) {
    heavyData.nodes.push_back(MakeNodeRightOfView(types.nodeType,
                                                  projection,
                                                  40.0*200.0+10.0*(double)i));
  }

  PrepareFrame(heavyPainter,projection,parameter,heavyData);

  REQUIRE(heavyPainter.LabelCount()==lightPainter.LabelCount());
  REQUIRE(heavyPainter.LabelPositions()==lightPainter.LabelPositions());

#if POINT_CULL_HAVE_ALLOCATION_COUNTER
  size_t lightBefore=allocationCounter.load(std::memory_order_relaxed);

  PrepareFrame(lightPainter,projection,parameter,lightData);

  size_t lightAllocations=allocationCounter.load(std::memory_order_relaxed)-lightBefore;

  size_t heavyBefore=allocationCounter.load(std::memory_order_relaxed);

  PrepareFrame(heavyPainter,projection,parameter,heavyData);

  size_t heavyAllocations=allocationCounter.load(std::memory_order_relaxed)-heavyBefore;

  // The further loaded objects are rejected before their styles are resolved, so they add no
  // allocation at all
  REQUIRE(heavyAllocations<=lightAllocations);
#endif
}
