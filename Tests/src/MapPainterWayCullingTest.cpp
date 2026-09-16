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
#include <osmscout/TypeConfig.h>
#include <osmscout/TypeInfoSet.h>
#include <osmscout/Way.h>
#include <osmscout/projection/MercatorProjection.h>
#include <osmscout/util/Magnification.h>
#include <osmscout/util/Transformation.h>
#include <osmscoutmap/LabelProvider.h>
#include <osmscoutmap/MapData.h>
#include <osmscoutmap/MapPainter.h>
#include <osmscoutmap/MapPainterNoOp.h>
#include <osmscoutmap/MapParameter.h>
#include <osmscoutmap/StyleConfig.h>
#include <osmscoutmap/Styles.h>

/*
 * Tests for the contract of the painter's early rejection of ways that cannot be visible
 * (spec map-painter-way-culling): a way that no line style of the level can bring into the
 * view is rejected before its style resolution, before its shield labels are registered and
 * before its geometry is transformed, the reach of that decision follows the style sheet, and
 * the prepared ways, their coordinates and their draw order stay what the unculled pipeline
 * produces.
 *
 * The tests drive the painter directly with synthetic ways, so they need no database and no
 * style sheet file. Ways are placed relative to the viewport using pixel distances measured
 * from the projection, so the tests do not depend on the absolute scale of a zoom level.
 */

/*
 * Counts the heap allocations of the preparing painter, so that a test can state that loaded
 * ways outside the view do not add work. Disabled in sanitizer builds, whose runtimes provide
 * their own operators (the same approach as Tests/src/MapPainterAreaPreparationTest.cpp).
 */
#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__)
#define WAY_CULL_HAVE_ALLOCATION_COUNTER 0
#elif defined(__has_feature)
#if __has_feature(memory_sanitizer) || __has_feature(address_sanitizer) || __has_feature(thread_sanitizer)
#define WAY_CULL_HAVE_ALLOCATION_COUNTER 0
#else
#define WAY_CULL_HAVE_ALLOCATION_COUNTER 1
#endif
#else
#define WAY_CULL_HAVE_ALLOCATION_COUNTER 1
#endif

#if WAY_CULL_HAVE_ALLOCATION_COUNTER

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

// ---------------------------------------------------------------------------
// Type config: one way type that the style sheet draws as a line and can
// resolve a shield style for
// ---------------------------------------------------------------------------
  struct TestTypes
  {
    osmscout::TypeConfigRef typeConfig;
    osmscout::TypeInfoRef   wayType;
  };

  TestTypes MakeTypes()
  {
    TestTypes types;

    types.typeConfig=std::make_shared<osmscout::TypeConfig>();

    types.wayType=std::make_shared<osmscout::TypeInfo>("test_way");
    types.wayType->CanBeWay(true);
    types.typeConfig->RegisterType(types.wayType);

    return types;
  }

  /**
   * Label provider with a fixed text, so that a shield label does not depend on the features
   * of the way or on a label provider factory.
   */
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
   * Style sheet with a line style of the given width for the way type, optionally with a
   * shield style whose label carries the given text.
   */
  osmscout::StyleConfigRef MakeStyles(const TestTypes& types,
                                      double lineWidth,
                                      const std::string& shieldLabel)
  {
    auto                  styleConfig=std::make_shared<osmscout::StyleConfig>(types.typeConfig);

    osmscout::TypeInfoSet wayTypes(*types.typeConfig);

    wayTypes.Set(types.wayType);

    osmscout::StyleFilter lineFilter;

    lineFilter.SetTypes(wayTypes);

    osmscout::LinePartialStyle lineStyle;

    lineStyle.SetColorValue(osmscout::LineStyle::attrLineColor,osmscout::Color(0.0,0.0,1.0));
    lineStyle.SetDoubleValue(osmscout::LineStyle::attrWidth,lineWidth);
    styleConfig->AddWayLineStyle(lineFilter,lineStyle);

    if (!shieldLabel.empty()) {
      osmscout::StyleFilter shieldFilter;

      shieldFilter.SetTypes(wayTypes);

      osmscout::PathShieldPartialStyle shieldStyle;

      shieldStyle.SetLabelValue(osmscout::PathShieldStyle::attrLabel,
                                std::make_shared<FixedLabelProvider>(shieldLabel));
      shieldStyle.SetColorValue(osmscout::PathShieldStyle::attrTextColor,osmscout::Color(0.0,0.0,0.0));
      shieldStyle.SetColorValue(osmscout::PathShieldStyle::attrBgColor,osmscout::Color(1.0,1.0,1.0));
      shieldStyle.SetColorValue(osmscout::PathShieldStyle::attrBorderColor,osmscout::Color(0.0,0.0,0.0));
      // The size has to be set as an attribute of the style: the shield style of a path shield
      // style is copied with its attributes when the style is added to the style config
      shieldStyle.SetDoubleValue(osmscout::PathShieldStyle::attrSize,1.0);
      styleConfig->AddWayPathShieldStyle(shieldFilter,shieldStyle);
    }

    styleConfig->Postprocess();

    return styleConfig;
  }

// ---------------------------------------------------------------------------
// Painter that records what the backend sees: the prepared ways and the
// registered shield labels
// ---------------------------------------------------------------------------
  class RecordingPainter CLASS_FINAL : public osmscout::MapPainterNoOp
  {
  private:
    size_t                          labelCount=0;
    std::vector<osmscout::Vertex2D> labelPositions;

  public:
    const std::vector<osmscout::MapPainter::WayData>& Ways() const
    {
      return GetWayData();
    }

    size_t LabelCount() const
    {
      return labelCount;
    }

    const std::vector<osmscout::Vertex2D>& LabelPositions() const
    {
      return labelPositions;
    }

    void ResetLabels()
    {
      labelCount=0;
      labelPositions.clear();
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

// ---------------------------------------------------------------------------
// Geometry helpers
// ---------------------------------------------------------------------------

  /**
   * Projection of a fixed 400x400 viewport at 300 DPI. The magnification level is the one the
   * painter uses, so the test can relate pixel distances to the reach of a line style.
   */
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

  osmscout::WayRef MakeWay(const osmscout::TypeInfoRef& type,
                           const osmscout::GeoCoord& c1,
                           const osmscout::GeoCoord& c2)
  {
    auto way=std::make_shared<osmscout::Way>();

    way->nodes.push_back(osmscout::Point(1,c1));
    way->nodes.push_back(osmscout::Point(2,c2));

    osmscout::FeatureValueBuffer buffer;

    buffer.SetType(type);

    way->SetFeatures(buffer);

    return way;
  }

  /**
   * Pixels per degree at the center of the projection, one value per axis, measured from the
   * projection itself.
   */
  struct PixelScale
  {
    double lon=0.0; //!< pixels per degree of longitude
    double lat=0.0; //!< pixels per degree of latitude
  };

  PixelScale MeasurePixelScale(const osmscout::MercatorProjection& projection)
  {
    constexpr double   probeDeltaDegrees=0.001;

    osmscout::Vertex2D center{};
    osmscout::Vertex2D east{};
    osmscout::Vertex2D north{};

    REQUIRE(projection.GeoToPixel(projection.GetCenter(),center));
    REQUIRE(projection.GeoToPixel(osmscout::GeoCoord(projection.GetCenter().GetLat(),
                                                     projection.GetCenter().GetLon()+probeDeltaDegrees),
                                  east));
    REQUIRE(projection.GeoToPixel(osmscout::GeoCoord(projection.GetCenter().GetLat()+probeDeltaDegrees,
                                                     projection.GetCenter().GetLon()),
                                  north));

    PixelScale scale;

    scale.lon=(east.GetX()-center.GetX())/probeDeltaDegrees;
    scale.lat=(center.GetY()-north.GetY())/probeDeltaDegrees;

    REQUIRE(scale.lon>0.0);
    REQUIRE(scale.lat>0.0);

    return scale;
  }

  double DegreesLonForPixels(const osmscout::MercatorProjection& projection,
                             double pixels)
  {
    return pixels/MeasurePixelScale(projection).lon;
  }

  /**
   * A vertical segment whose distance to the right edge of the viewport is the given number of
   * pixels.
   */
  osmscout::WayRef MakeWayRightOfView(const osmscout::TypeInfoRef& type,
                                      const osmscout::MercatorProjection& projection,
                                      double distancePixels,
                                      double lengthPixels=10.0)
  {
    constexpr double halfViewportWidthPixels=200.0;

    double           halfLat=DegreesLonForPixels(projection,lengthPixels/2.0);
    double           lon=projection.GetCenter().GetLon()+
                          DegreesLonForPixels(projection,halfViewportWidthPixels+distancePixels);

    return MakeWay(type,
                   osmscout::GeoCoord(projection.GetCenter().GetLat()-halfLat,lon),
                   osmscout::GeoCoord(projection.GetCenter().GetLat()+halfLat,lon));
  }

  /**
   * A diagonal segment through the middle of the viewport. It is long enough to cross the grid
   * cells the painter places shield labels on, because a short segment produces no grid point
   * and therefore no shield label at all.
   */
  osmscout::WayRef MakeWayInsideView(const osmscout::TypeInfoRef& type,
                                     const osmscout::MercatorProjection& projection)
  {
    constexpr double halfExtentPixels=150.0;

    double           halfLat=DegreesLonForPixels(projection,halfExtentPixels);
    double           halfLon=DegreesLonForPixels(projection,halfExtentPixels);

    return MakeWay(type,
                   osmscout::GeoCoord(projection.GetCenter().GetLat()-halfLat,
                                      projection.GetCenter().GetLon()-halfLon),
                   osmscout::GeoCoord(projection.GetCenter().GetLat()+halfLat,
                                      projection.GetCenter().GetLon()+halfLon));
  }

  /**
   * Pixels a line style of the given width reaches beyond an object, as the painter derives it
   * from the style sheet (half of the projected width).
   */
  double ReachForWidth(const osmscout::MercatorProjection& projection,
                       double lineWidth)
  {
    return lineWidth/projection.GetPixelSize()/2.0;
  }

  /**
   * Pixels the widest width a data carried width value can contribute reaches beyond an object
   */
  double ReachForWidthFeature(const osmscout::MercatorProjection& projection)
  {
    constexpr double maxWidthFeatureWidth=255.0;

    return ReachForWidth(projection,maxWidthFeatureWidth);
  }

  /**
   * Prepares the frame, i.e. all steps up to and including the postprocessing, without drawing.
   * What the backend sees during the postprocessing callback is the prepared frame.
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
                         osmscout::AfterPreprocessing));
  }

  /**
   * The transformed coordinates of the prepared ways of a frame. A coordinate range refers to
   * the coordinate buffer of the painter that produced it, so the coordinates have to be copied
   * while the painter is alive.
   */
  std::vector<double> FrameCoordinates(const RecordingPainter& painter)
  {
    std::vector<double> coords;

    for (const auto& way : painter.Ways()) {
      const osmscout::CoordBufferRange & range=way.coordRange;

      for (size_t i=0; i<range.GetSize(); i++) {
        osmscout::Vertex2D coord=range.Get(range.GetStart()+i);

        coords.push_back(coord.GetX());
        coords.push_back(coord.GetY());
      }
    }

    return coords;
  }
}

/**
 * A way that no line style of the level can bring into the view contributes nothing: it is not
 * prepared, it adds no transformed coordinates and it does not change the frame (spec
 * map-painter-way-culling, requirement "A way that cannot be visible is rejected before its
 * preparation").
 */
TEST_CASE("A way outside the view is not prepared","[MapPainterWayCulling]")
{
  const auto types=MakeTypes();

  auto       styleConfig=MakeStyles(types,4.0,"");

  const auto projection=MakeProjection();
  const auto parameter=MakeParameter();

  auto       inside=MakeWayInsideView(types.wayType,projection);
  auto       outside=MakeWayRightOfView(types.wayType,
                                        projection,
                                        // far outside any reach of any style of the sheet
                                        4.0*200.0);

  RecordingPainter reference;
  auto             referenceData=MakeData(styleConfig);

  referenceData.ways.push_back(inside);

  PrepareFrame(reference,projection,parameter,referenceData);

  REQUIRE(reference.Ways().size()==1);

  RecordingPainter painter;
  auto             data=MakeData(styleConfig);

  data.ways.push_back(inside);
  data.ways.push_back(outside);

  PrepareFrame(painter,projection,parameter,data);

  // The visible way is prepared as before, including its coordinates
  REQUIRE(painter.Ways().size()==1);
  REQUIRE(FrameCoordinates(painter)==FrameCoordinates(reference));
}

/**
 * The early rejection follows the style sheet and never removes a way that the per-line-style
 * decision keeps: a way that lies outside the viewport beyond the reach of a narrow line style
 * is kept by a style sheet whose line styles reach that far (spec map-painter-way-culling,
 * requirement "A way that cannot be visible is rejected before its preparation").
 */
TEST_CASE("The way rejection follows the reach of the style sheet","[MapPainterWayCulling]")
{
  const auto types=MakeTypes();

  const auto projection=MakeProjection();
  const auto parameter=MakeParameter();

  // Far enough that the width a data carried width value can contribute does not reach it, but
  // inside the reach of the 1000 units wide line style
  double distance=ReachForWidthFeature(projection)+20.0;

  REQUIRE(distance>ReachForWidthFeature(projection)+ReachForWidth(projection,4.0));
  REQUIRE(distance<ReachForWidth(projection,1000.0));

  auto             way=MakeWayRightOfView(types.wayType,projection,distance);

  auto             narrowConfig=MakeStyles(types,4.0,"");
  auto             wideConfig=MakeStyles(types,1000.0,"");

  RecordingPainter narrowPainter;
  auto             narrowData=MakeData(narrowConfig);

  narrowData.ways.push_back(way);

  PrepareFrame(narrowPainter,projection,parameter,narrowData);

  REQUIRE(narrowPainter.Ways().empty());

  RecordingPainter widePainter;
  auto             wideData=MakeData(wideConfig);

  wideData.ways.push_back(way);

  PrepareFrame(widePainter,projection,parameter,wideData);

  // The wider style sheet reaches the way, so the rejection has to keep it
  REQUIRE(widePainter.Ways().size()==1);
}

/**
 * A shield styled way outside the view registers no shield labels, and the labels of a visible
 * way are unchanged (spec map-painter-way-culling, requirement "Shield labels of a rejected way
 * are not registered").
 */
TEST_CASE("A shield styled way outside the view adds no labels","[MapPainterWayCulling]")
{
  const auto types=MakeTypes();

  auto       styleConfig=MakeStyles(types,4.0,"A1");

  const auto projection=MakeProjection();
  const auto parameter=MakeParameter();

  auto       inside=MakeWayInsideView(types.wayType,projection);
  auto       outside=MakeWayRightOfView(types.wayType,
                                        projection,
                                        4.0*200.0);

  RecordingPainter reference;
  auto             referenceData=MakeData(styleConfig);

  referenceData.ways.push_back(inside);

  // Preconditions of the fixture: the style sheet defines a shield style at the level of the
  // projection and resolves one for the way
  REQUIRE(styleConfig->HasWayPathShieldStyle(projection));
  REQUIRE(styleConfig->GetWayPathShieldStyle(inside->GetFeatureValueBuffer(),
                                             projection)!=nullptr);
  REQUIRE(styleConfig->GetWayPathShieldStyle(inside->GetFeatureValueBuffer(),
                                             projection)->GetLabel()!=nullptr);

  PrepareFrame(reference,projection,parameter,referenceData);

  // The style sheet registers shields for the visible way
  REQUIRE(reference.Ways().size()==1);
  REQUIRE(reference.LabelCount()>0);

  RecordingPainter painter;
  auto             data=MakeData(styleConfig);

  data.ways.push_back(inside);
  data.ways.push_back(outside);

  PrepareFrame(painter,projection,parameter,data);

  REQUIRE(painter.Ways().size()==1);
  REQUIRE(painter.LabelCount()==reference.LabelCount());
  REQUIRE(painter.LabelPositions()==reference.LabelPositions());
}

/**
 * Way preparation work and its heap allocation do not grow with the ways that are loaded into
 * the view but cannot contribute a pixel (spec map-painter-way-culling, requirement "Way
 * preparation work follows the visible ways, not the loaded ways").
 */
TEST_CASE("Loaded ways outside the view do not add work","[MapPainterWayCulling]")
{
  const auto       types=MakeTypes();

  auto             styleConfig=MakeStyles(types,4.0,"");

  const auto       projection=MakeProjection();
  const auto       parameter=MakeParameter();

  constexpr size_t farWayCount=64;

  RecordingPainter lightPainter;
  auto             lightData=MakeData(styleConfig);

  lightData.ways.push_back(MakeWayInsideView(types.wayType,projection));

  PrepareFrame(lightPainter,projection,parameter,lightData);

  REQUIRE(lightPainter.Ways().size()==1);

  RecordingPainter heavyPainter;
  auto             heavyData=MakeData(styleConfig);

  heavyData.ways.push_back(MakeWayInsideView(types.wayType,projection));

  for (size_t i=0; i<farWayCount; i++) {
    heavyData.ways.push_back(MakeWayRightOfView(types.wayType,
                                                projection,
                                                4.0*200.0+10.0*(double)i));
  }

  PrepareFrame(heavyPainter,projection,parameter,heavyData);

  // The prepared ways, their coordinates and their order are the ones of the view without the
  // further loaded ways
  REQUIRE(heavyPainter.Ways().size()==1);
  REQUIRE(FrameCoordinates(heavyPainter)==FrameCoordinates(lightPainter));

#if WAY_CULL_HAVE_ALLOCATION_COUNTER
  // Measure a further frame of each view, so that the one-time allocations of the first frame
  // of a painter do not count
  size_t lightBefore=allocationCounter.load(std::memory_order_relaxed);

  PrepareFrame(lightPainter,projection,parameter,lightData);

  size_t lightAllocations=allocationCounter.load(std::memory_order_relaxed)-lightBefore;

  size_t heavyBefore=allocationCounter.load(std::memory_order_relaxed);

  PrepareFrame(heavyPainter,projection,parameter,heavyData);

  size_t heavyAllocations=allocationCounter.load(std::memory_order_relaxed)-heavyBefore;

  // The further loaded ways do not add a per-object allocation to the frame
  REQUIRE(heavyAllocations<=lightAllocations+4);
#endif
}
