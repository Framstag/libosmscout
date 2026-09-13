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
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <new>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <osmscout/Area.h>
#include <osmscout/GeoCoord.h>
#include <osmscout/Pixel.h>
#include <osmscout/TypeConfig.h>
#include <osmscout/TypeInfoSet.h>
#include <osmscout/projection/MercatorProjection.h>
#include <osmscout/projection/Projection.h>
#include <osmscout/util/Magnification.h>
#include <osmscout/util/Transformation.h>
#include <osmscoutmap/MapData.h>
#include <osmscoutmap/MapPainter.h>
#include <osmscoutmap/MapPainterNoOp.h>
#include <osmscoutmap/MapParameter.h>
#include <osmscoutmap/StyleConfig.h>
#include <osmscoutmap/Styles.h>

/*
 * Tests for the contract of the area preparation step of the map painter: preparing the
 * areas of a frame allocates a constant amount independent of how many areas are loaded,
 * the styling and visibility of a ring are decided before its geometry is transformed,
 * clipping rings keep their geometry, and the prepared area set and its order are
 * unchanged.
 *
 * The tests drive the painter directly with synthetic objects, so they need no database and
 * no stylesheet file. The allocation counter replaces the global operator new/delete (the
 * same approach as Tests/src/PerformanceTest.cpp); it is disabled in sanitizer builds,
 * whose runtimes provide their own operators.
 */

#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__)
#define AREA_PREP_HAVE_ALLOCATION_COUNTER 0
#elif defined(__has_feature)
#if __has_feature(memory_sanitizer) || __has_feature(address_sanitizer) || __has_feature(thread_sanitizer)
#define AREA_PREP_HAVE_ALLOCATION_COUNTER 0
#else
#define AREA_PREP_HAVE_ALLOCATION_COUNTER 1
#endif
#else
#define AREA_PREP_HAVE_ALLOCATION_COUNTER 1
#endif

#if AREA_PREP_HAVE_ALLOCATION_COUNTER

namespace {

  /**
   * Counts every heap allocation of the test process. The difference between two calls
   * measures the allocation volume of the code in between, which is the property that
   * tells whether preparing the areas of a frame allocates per loaded area or reuses its
   * buffers. Aligned allocations are not counted.
   */
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

size_t GetAllocationCount()
{
  return allocationCounter.load(std::memory_order_relaxed);
}

#else

size_t GetAllocationCount()
{
  return 0;
}

#endif

namespace {

// ---------------------------------------------------------------------------
// Painter that records the prepared areas, i.e. what the backend sees during
// the post-preprocessing callback
// ---------------------------------------------------------------------------
  class RecordingPainter : public osmscout::MapPainterNoOp
  {
  public:
    const std::vector<osmscout::MapPainter::AreaData>& Areas() const
    {
      return GetAreaData();
    }

  protected:
    void AfterPreprocessingCallback(const osmscout::Projection& /*projection*/,
                                    const osmscout::MapParameter& /*parameter*/,
                                    const std::vector<osmscout::MapData>& /*data*/) override
    {
      preparedCount=GetAreaData().size();
    }

  public:
    size_t preparedCount=0;
  };

// ---------------------------------------------------------------------------
// Type config: one area type that the stylesheet styles, one that it does not,
// and one type for inner rings that act as clipping regions
// ---------------------------------------------------------------------------
  struct TestTypes
  {
    osmscout::TypeConfigRef typeConfig;
    osmscout::TypeInfoRef   styledAreaType;
    osmscout::TypeInfoRef   unstyledAreaType;
    osmscout::TypeInfoRef   clippingRingType;
  };

  TestTypes MakeTypes()
  {
    TestTypes types;

    types.typeConfig=std::make_shared<osmscout::TypeConfig>();

    types.styledAreaType=std::make_shared<osmscout::TypeInfo>("test_area_styled");
    types.styledAreaType->CanBeArea(true);
    types.typeConfig->RegisterType(types.styledAreaType);

    types.unstyledAreaType=std::make_shared<osmscout::TypeInfo>("test_area_unstyled");
    types.unstyledAreaType->CanBeArea(true);
    types.typeConfig->RegisterType(types.unstyledAreaType);

    types.clippingRingType=std::make_shared<osmscout::TypeInfo>("test_area_clipping_ring");
    types.clippingRingType->CanBeArea(true);
    types.clippingRingType->SetIgnore(true);
    types.typeConfig->RegisterType(types.clippingRingType);

    return types;
  }

// ---------------------------------------------------------------------------
// Stylesheet: a fill style for the styled area type only, so the unstyled type
// resolves neither a fill nor a border
// ---------------------------------------------------------------------------
  osmscout::StyleConfigRef MakeStyles(const TestTypes& types)
  {
    auto                  styleConfig=std::make_shared<osmscout::StyleConfig>(types.typeConfig);

    osmscout::TypeInfoSet areaTypes(*types.typeConfig);

    areaTypes.Set(types.styledAreaType);

    osmscout::StyleFilter areaFilter;

    areaFilter.SetTypes(areaTypes);

    osmscout::FillPartialStyle fillStyle;

    fillStyle.SetColorValue(osmscout::FillStyle::attrFillColor,osmscout::Color(0.0,1.0,0.0));
    styleConfig->AddAreaFillStyle(areaFilter,fillStyle);

    styleConfig->Postprocess();

    return styleConfig;
  }

// ---------------------------------------------------------------------------

// Synthetic areas

// ---------------------------------------------------------------------------

  /**
   * Area with one outer ring that is a regular polygon with the given number of
   * nodes around the given center. Used where the node count of a ring matters.
   */
  osmscout::AreaRef MakeArea(const osmscout::TypeInfoRef& type,
                             const osmscout::GeoCoord& center,
                             double radius,
                             size_t nodeCount)
  {
    auto                 area=std::make_shared<osmscout::Area>();

    osmscout::Area::Ring ring;

    ring.MarkAsOuterRing();
    ring.SetType(type);

    for (size_t i=0; i<nodeCount; i++) {
      double angle=2.0*M_PI*(double)i/(double)nodeCount;

      ring.nodes.push_back(osmscout::Point(i+1,
                                           osmscout::GeoCoord(center.GetLat()+radius*std::sin(angle),
                                                              center.GetLon()+radius*std::cos(angle))));
    }

    ring.center=center;

    area->rings.push_back(ring);

    return area;
  }

  /**
   * Area with one outer ring of fixed geometry, so that all areas built with this
   * function share their bounding box and the area ordering compares them equal.
   * They are identified by their center.
   */
  osmscout::AreaRef MakeEqualComparingArea(const osmscout::TypeInfoRef& type,
                                           const osmscout::GeoCoord& center)
  {
    auto                 area=std::make_shared<osmscout::Area>();

    osmscout::Area::Ring ring;

    ring.MarkAsOuterRing();
    ring.SetType(type);

    ring.nodes.push_back(osmscout::Point(1,osmscout::GeoCoord(50.0000,8.0000)));
    ring.nodes.push_back(osmscout::Point(2,osmscout::GeoCoord(50.0000,8.0020)));
    ring.nodes.push_back(osmscout::Point(3,osmscout::GeoCoord(50.0020,8.0020)));
    ring.nodes.push_back(osmscout::Point(4,osmscout::GeoCoord(50.0020,8.0000)));

    ring.center=center;

    area->rings.push_back(ring);

    return area;
  }

  /**
   * Append an inner ring without a own style to the given area. Such a ring is not
   * drawn but acts as clipping region of the ring it is nested in.
   */
  void AddClippingRing(const osmscout::AreaRef& area,
                       const osmscout::TypeInfoRef& type,
                       const osmscout::GeoCoord& center,
                       double radius)
  {
    osmscout::Area::Ring ring;

    // Level 2, i.e. an inner ring of the top level outer ring
    ring.SetRing(2);
    ring.SetType(type);

    ring.nodes.push_back(osmscout::Point(1,osmscout::GeoCoord(center.GetLat()-radius,center.GetLon()-radius)));
    ring.nodes.push_back(osmscout::Point(2,osmscout::GeoCoord(center.GetLat()-radius,center.GetLon()+radius)));
    ring.nodes.push_back(osmscout::Point(3,osmscout::GeoCoord(center.GetLat()+radius,center.GetLon()+radius)));
    ring.nodes.push_back(osmscout::Point(4,osmscout::GeoCoord(center.GetLat()+radius,center.GetLon()-radius)));

    area->rings.push_back(ring);
  }

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

  void Render(RecordingPainter& painter,
              const osmscout::MercatorProjection& projection,
              const osmscout::MapParameter& parameter,
              const osmscout::MapData& data)
  {
    REQUIRE(painter.DrawMap(projection,
                            parameter,
                            std::vector<osmscout::MapData> {data}));
  }

  osmscout::MapData MakeData(const osmscout::StyleConfigRef& styleConfig)
  {
    osmscout::MapData data;

    data.styleConfig=styleConfig;

    return data;
  }

  /**
   * Transformed coordinates of the given ring, computed independently of the painter.
   * Preparation may renumber the coordinate buffer of a frame, so the coordinates are
   * the property to compare, not the indices of a prepared range.
   */
  osmscout::CoordBufferRange TransformRing(const osmscout::Area::Ring& ring,
                                           const osmscout::MercatorProjection& projection,
                                           const osmscout::MapParameter& parameter,
                                           osmscout::TransBuffer& transBuffer,
                                           osmscout::CoordBuffer& coordBuffer)
  {
    return osmscout::TransformArea(ring.nodes,
                                   transBuffer,
                                   coordBuffer,
                                   projection,
                                   parameter.GetOptimizeAreaNodes(),
                                   projection.ConvertWidthToPixel(parameter.GetOptimizeErrorToleranceMm()));
  }

  void CompareRanges(const osmscout::CoordBufferRange& expected,
                     const osmscout::CoordBufferRange& actual)
  {
    REQUIRE(actual.IsValid());
    REQUIRE(actual.GetSize()==expected.GetSize());

    for (size_t i=0; i<expected.GetSize(); i++) {
      REQUIRE(actual.Get(actual.GetStart()+i).GetX()==expected.Get(expected.GetStart()+i).GetX());
      REQUIRE(actual.Get(actual.GetStart()+i).GetY()==expected.Get(expected.GetStart()+i).GetY());
    }
  }
} // namespace

// ---------------------------------------------------------------------------
// Requirement: Area preparation does not allocate per loaded area
// ---------------------------------------------------------------------------
TEST_CASE("Area preparation allocates a constant amount per frame","[MapPainterAreaPreparation]")
{
  auto         types=MakeTypes();
  auto         styleConfig=MakeStyles(types);
  auto         projection=MakeProjection();
  auto         parameter=MakeParameter();

  const size_t smallLoadedCount=16;
  const size_t largeLoadedCount=512;

  auto         renderView=[&](size_t loadedCount) {
                             auto             data=MakeData(styleConfig);
                             RecordingPainter painter;

                             for (size_t i=0; i<loadedCount; i++) {
                               double lon=8.0005+0.0000005*(double)i;

                               data.areas.push_back(MakeArea(types.unstyledAreaType,{50.0010,lon},0.0004,8));
                             }

                             // First frame warms up the reused buffers, the second one is measured
                             Render(painter,projection,parameter,data);

                             size_t before=GetAllocationCount();

                             Render(painter,projection,parameter,data);

                             size_t allocations=GetAllocationCount()-before;

                             return std::make_pair(allocations,painter.Areas().size());
                           };

  auto smallView=renderView(smallLoadedCount);
  auto largeView=renderView(largeLoadedCount);

  // No area is styled, so nothing is prepared although many areas are loaded
  REQUIRE(smallView.second==0);
  REQUIRE(largeView.second==0);

#if AREA_PREP_HAVE_ALLOCATION_COUNTER
  INFO("allocations for " << smallLoadedCount << " loaded areas: " << smallView.first);
  INFO("allocations for " << largeLoadedCount << " loaded areas: " << largeView.first);

  // 32x more loaded areas, and no ring is transformed and no area prepared
  REQUIRE(smallView.first<=32);
  REQUIRE(largeView.first<=32);
  REQUIRE(largeView.first<=smallView.first+4);
#else
  SUCCEED("allocation counter is disabled in sanitizer builds");
#endif
}

TEST_CASE("Loaded areas that are not prepared do not add allocations","[MapPainterAreaPreparation]")
{
  auto         types=MakeTypes();
  auto         styleConfig=MakeStyles(types);
  auto         projection=MakeProjection();
  auto         parameter=MakeParameter();

  const size_t preparedCount=16;
  const size_t unstyledCount=512;

  auto         renderView=[&](bool withUnstyledAreas) {
                             auto             data=MakeData(styleConfig);
                             RecordingPainter painter;

                             for (size_t i=0; i<preparedCount; i++) {
                               double lon=8.0005+0.00002*(double)i;

                               data.areas.push_back(MakeArea(types.styledAreaType,{50.0010,lon},0.0002,8));
                             }

                             if (withUnstyledAreas) {
                               for (size_t i=0; i<unstyledCount; i++) {
                                 double lon=8.0005+0.0000005*(double)i;

                                 data.areas.push_back(MakeArea(types.unstyledAreaType,{50.0010,lon},0.0004,8));
                               }
                             }

                             Render(painter,projection,parameter,data);

                             size_t before=GetAllocationCount();

                             Render(painter,projection,parameter,data);

                             size_t allocations=GetAllocationCount()-before;

                             return std::make_pair(allocations,painter.Areas().size());
                           };

  auto lightView=renderView(false);
  auto heavyView=renderView(true);

  // Both views prepare the same areas, they only differ in the loaded area count
  REQUIRE(lightView.second==preparedCount);
  REQUIRE(heavyView.second==preparedCount);

#if AREA_PREP_HAVE_ALLOCATION_COUNTER
  INFO("allocations without unstyled areas: " << lightView.first);
  INFO("allocations with " << unstyledCount << " unstyled areas: " << heavyView.first);

  REQUIRE(heavyView.first<=lightView.first+4);
#else
  SUCCEED("allocation counter is disabled in sanitizer builds");
#endif
}

// ---------------------------------------------------------------------------
// Requirement: Styling and visibility are decided before geometry is transformed
// ---------------------------------------------------------------------------
TEST_CASE("Rings outside the viewport are not transformed","[MapPainterAreaPreparation]")
{
  auto types=MakeTypes();
  auto styleConfig=MakeStyles(types);
  auto projection=MakeProjection();
  auto parameter=MakeParameter();

  // A control view with the visible area only, so the prepared coordinate range of that
  // area is known without any other ring in the frame
  size_t controlStart=0;
  {
    auto             data=MakeData(styleConfig);
    RecordingPainter painter;

    data.areas.push_back(MakeArea(types.styledAreaType,{50.0010,8.0010},0.0002,8));

    Render(painter,projection,parameter,data);

    REQUIRE(painter.Areas().size()==1);
    REQUIRE(painter.Areas()[0].coordRange.IsValid());

    controlStart=painter.Areas()[0].coordRange.GetStart();
  }

  auto             data=MakeData(styleConfig);
  RecordingPainter painter;

  // Far outside of the viewport (the projection covers about 50.001/8.001) and with many
  // more nodes than the area that is actually drawn
  data.areas.push_back(MakeArea(types.styledAreaType,{50.5000,8.5000},0.0100,256));
  data.areas.push_back(MakeArea(types.styledAreaType,{50.0010,8.0010},0.0002,8));

  Render(painter,projection,parameter,data);

  REQUIRE(painter.Areas().size()==1);

  // The invisible area contributed no coordinates at all, otherwise the visible area
  // would start after them
  REQUIRE(painter.Areas()[0].coordRange.GetStart()==controlStart);
}

TEST_CASE("Unstyled rings are not transformed","[MapPainterAreaPreparation]")
{
  auto   types=MakeTypes();
  auto   styleConfig=MakeStyles(types);
  auto   projection=MakeProjection();
  auto   parameter=MakeParameter();

  size_t controlStart=0;
  {
    auto             data=MakeData(styleConfig);
    RecordingPainter painter;

    data.areas.push_back(MakeArea(types.styledAreaType,{50.0010,8.0010},0.0002,8));

    Render(painter,projection,parameter,data);

    REQUIRE(painter.Areas().size()==1);

    controlStart=painter.Areas()[0].coordRange.GetStart();
  }

  auto             data=MakeData(styleConfig);
  RecordingPainter painter;

  // Inside the viewport, but the stylesheet resolves neither a fill nor a border for it
  data.areas.push_back(MakeArea(types.unstyledAreaType,{50.0010,8.0010},0.0006,256));
  data.areas.push_back(MakeArea(types.styledAreaType,{50.0010,8.0010},0.0002,8));

  Render(painter,projection,parameter,data);

  REQUIRE(painter.Areas().size()==1);

  REQUIRE(painter.Areas()[0].coordRange.GetStart()==controlStart);
}

TEST_CASE("Styled and visible rings keep their transformed coordinates","[MapPainterAreaPreparation]")
{
  auto             types=MakeTypes();
  auto             styleConfig=MakeStyles(types);
  auto             projection=MakeProjection();
  auto             parameter=MakeParameter();

  auto             data=MakeData(styleConfig);
  RecordingPainter painter;

  auto             visibleArea=MakeArea(types.styledAreaType,{50.0010,8.0010},0.0002,8);

  // An unstyled and a ring-less area in front of it, so the ring of the visible area is
  // not simply the first thing that is transformed
  data.areas.push_back(MakeArea(types.unstyledAreaType,{50.0010,8.0010},0.0006,64));
  data.areas.push_back(visibleArea);

  Render(painter,projection,parameter,data);

  REQUIRE(painter.Areas().size()==1);

  osmscout::TransBuffer transBuffer;
  osmscout::CoordBuffer coordBuffer;

  CompareRanges(TransformRing(visibleArea->rings[0],projection,parameter,transBuffer,coordBuffer),
                painter.Areas()[0].coordRange);
}

// ---------------------------------------------------------------------------
// Requirement: Clipping rings keep valid geometry
// ---------------------------------------------------------------------------
TEST_CASE("Clipping rings of a drawn area keep their geometry","[MapPainterAreaPreparation]")
{
  auto             types=MakeTypes();
  auto             styleConfig=MakeStyles(types);
  auto             projection=MakeProjection();
  auto             parameter=MakeParameter();

  auto             data=MakeData(styleConfig);
  RecordingPainter painter;

  auto             area=MakeArea(types.styledAreaType,{50.0010,8.0010},0.0010,8);

  AddClippingRing(area,types.clippingRingType,{50.0010,8.0010},0.0002);

  data.areas.push_back(area);

  Render(painter,projection,parameter,data);

  // Only the styled outer ring is prepared, the clipping ring is not drawn
  REQUIRE(painter.Areas().size()==1);
  REQUIRE(painter.Areas()[0].clippings.size()==1);

  osmscout::TransBuffer transBuffer;
  osmscout::CoordBuffer coordBuffer;

  CompareRanges(TransformRing(area->rings[1],projection,parameter,transBuffer,coordBuffer),
                painter.Areas()[0].clippings.front());
}

// ---------------------------------------------------------------------------
// Requirement: Prepared areas, draw order and rendered output are unchanged
// ---------------------------------------------------------------------------
TEST_CASE("Prepared areas keep the set and the order of the loaded areas","[MapPainterAreaPreparation]")
{
  auto             types=MakeTypes();
  auto             styleConfig=MakeStyles(types);
  auto             projection=MakeProjection();
  auto             parameter=MakeParameter();

  const size_t     preparedCount=32;

  auto             data=MakeData(styleConfig);
  RecordingPainter painter;

  // All areas share their bounding box and their outer role, so the area ordering
  // compares them equal and the preparation order decides
  std::vector<double> expectedCenters;

  for (size_t i=0; i<preparedCount; i++) {
    double lon=8.0004+0.00004*(double)i;

    data.areas.push_back(MakeEqualComparingArea(types.styledAreaType,{50.0010,lon}));
    expectedCenters.push_back(lon);
  }

  Render(painter,projection,parameter,data);

  REQUIRE(painter.Areas().size()==preparedCount);

  std::vector<double> firstOrder;

  for (const auto& area : painter.Areas()) {
    REQUIRE(area.center.has_value());
    firstOrder.push_back(area.center->GetLon());
  }

  REQUIRE(firstOrder==expectedCenters);

  Render(painter,projection,parameter,data);

  REQUIRE(painter.Areas().size()==preparedCount);

  std::vector<double> secondOrder;

  for (const auto& area : painter.Areas()) {
    REQUIRE(area.center.has_value());
    secondOrder.push_back(area.center->GetLon());
  }

  REQUIRE(secondOrder==firstOrder);
}
