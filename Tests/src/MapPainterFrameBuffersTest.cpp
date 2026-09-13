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
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <osmscout/Area.h>
#include <osmscout/GeoCoord.h>
#include <osmscout/ObjectRef.h>
#include <osmscout/Pixel.h>
#include <osmscout/Route.h>
#include <osmscout/TypeConfig.h>
#include <osmscout/TypeInfoSet.h>
#include <osmscout/Way.h>
#include <osmscout/feature/LayerFeature.h>
#include <osmscout/feature/NameFeature.h>
#include <osmscout/projection/MercatorProjection.h>
#include <osmscout/projection/Projection.h>
#include <osmscout/util/Magnification.h>
#include <osmscoutmap/LabelPath.h>
#include <osmscoutmap/LabelProvider.h>
#include <osmscoutmap/LabelLayouter.h>
#include <osmscoutmap/MapData.h>
#include <osmscoutmap/MapPainter.h>
#include <osmscoutmap/MapPainterNoOp.h>
#include <osmscoutmap/MapParameter.h>
#include <osmscoutmap/StyleConfig.h>
#include <osmscoutmap/Styles.h>

/*
 * Tests for the contract of the prepared per-frame data of the map painter: the stores
 * are reused between frames, they are contiguous, their order is stable, the route
 * labels keep resolving their prepared way path, and the backend callback keeps read
 * access to the prepared areas and ways.
 *
 * The tests drive the painter directly with synthetic objects, so they need no
 * database and no graphics backend.
 *
 * Limitation: `Way::fileOffset` and `Area::fileOffset` have no public setter, so
 * every synthetic object carries offset 0. Test data is therefore built so that
 * it stays unambiguous, and a growth of the prepared way path store with several
 * distinct route member ways is not expressible here. The append case that occurs
 * in production - a route member way that is not part of the rendered data - is
 * covered.
 */
namespace {

// ---------------------------------------------------------------------------
// Painter that records what the backend sees: the prepared data during the
// post-preprocessing callback and the draw calls afterwards
// ---------------------------------------------------------------------------
  struct ContourLabelRecord
  {
    osmscout::ObjectFileRef ref;
    std::string             text;
    osmscout::Vertex2D      start;
    osmscout::Vertex2D      end;
    bool                    hasGeometry=false;
  };

  class RecordingPainter : public osmscout::MapPainterNoOp
  {
  public:
    std::vector<osmscout::ObjectFileRef> callbackAreaRefs;
    std::vector<const void*>             callbackWays;
    std::vector<osmscout::ObjectFileRef> drawnAreaRefs;
    std::vector<const void*>             drawnWays;
    std::vector<ContourLabelRecord>      contourLabels;
    size_t                               callbackCount=0;

    const std::vector<osmscout::MapPainter::AreaData>& Areas() const
    {
      return GetAreaData();
    }

    const std::vector<osmscout::MapPainter::WayData>& Ways() const
    {
      return GetWayData();
    }

  protected:
    void AfterPreprocessingCallback(const osmscout::Projection& /*projection*/,
                                    const osmscout::MapParameter& /*parameter*/,
                                    const std::vector<osmscout::MapData>& /*data*/) override
    {
      callbackCount++;

      for (const auto& area : GetAreaData()) {
        callbackAreaRefs.push_back(area.ref);
      }

      for (const auto& way : GetWayData()) {
        callbackWays.push_back(&way);
      }
    }

    void RegisterContourLabel(const osmscout::Projection& /*projection*/,
                              const osmscout::MapParameter& /*parameter*/,
                              bool /*basemap*/,
                              const osmscout::ObjectFileRef& ref,
                              const osmscout::PathLabelData& label,
                              const osmscout::LabelPath& labelPath) override
    {
      ContourLabelRecord record{ref,label.text,osmscout::Vertex2D(),osmscout::Vertex2D(),false};

      if (labelPath.GetLength()>0.0) {
        record.start=labelPath.PointAtLength(0.0);
        record.end=labelPath.PointAtLength(labelPath.GetLength());
        record.hasGeometry=true;
      }

      contourLabels.push_back(record);
    }

    void DrawArea(const osmscout::Projection& /*projection*/,
                  const osmscout::MapParameter& /*parameter*/,
                  const osmscout::MapPainter::AreaData& area) override
    {
      drawnAreaRefs.push_back(area.ref);
    }

    void DrawWay(const osmscout::Projection& /*projection*/,
                 const osmscout::MapParameter& /*parameter*/,
                 const osmscout::MapPainter::WayData& way) override
    {
      drawnWays.push_back(&way);
    }
  };

// ---------------------------------------------------------------------------
// Type config with a way, an area and a route type, each carrying the features
// the styles below need
// ---------------------------------------------------------------------------
  struct TestTypes
  {
    osmscout::TypeConfigRef typeConfig;
    osmscout::TypeInfoRef   wayType;
    osmscout::TypeInfoRef   areaType;
    osmscout::TypeInfoRef   routeType;
  };

  TestTypes MakeTypes()
  {
    TestTypes types;

    types.typeConfig=std::make_shared<osmscout::TypeConfig>();

    auto layerFeature=types.typeConfig->GetFeature("Layer");

    REQUIRE(layerFeature);

    auto nameFeature=types.typeConfig->GetFeature("Name");

    REQUIRE(nameFeature);

    types.wayType=std::make_shared<osmscout::TypeInfo>("test_way");
    types.wayType->CanBeWay(true);
    types.wayType->AddFeature(layerFeature);
    types.typeConfig->RegisterType(types.wayType);

    types.areaType=std::make_shared<osmscout::TypeInfo>("test_area");
    types.areaType->CanBeArea(true);
    types.typeConfig->RegisterType(types.areaType);

    types.routeType=std::make_shared<osmscout::TypeInfo>("test_route");
    types.routeType->CanBeWay(true);
    types.routeType->AddFeature(layerFeature);
    types.routeType->AddFeature(nameFeature);
    types.typeConfig->RegisterType(types.routeType);

    return types;
  }

// ---------------------------------------------------------------------------
// Style config: a line style for the way, a fill style for the area, a route
// line style and a route path text style with a name based label
// ---------------------------------------------------------------------------
  osmscout::StyleConfigRef MakeStyles(const TestTypes& types)
  {
    auto                  styleConfig=std::make_shared<osmscout::StyleConfig>(types.typeConfig);

    osmscout::TypeInfoSet wayTypes(*types.typeConfig);

    wayTypes.Set(types.wayType);

    osmscout::StyleFilter wayFilter;

    wayFilter.SetTypes(wayTypes);

    osmscout::LinePartialStyle lineStyle;

    lineStyle.SetColorValue(osmscout::LineStyle::attrLineColor,osmscout::Color(0.0,0.0,1.0));
    lineStyle.SetDoubleValue(osmscout::LineStyle::attrDisplayWidth,2.0);
    lineStyle.SetDoubleValue(osmscout::LineStyle::attrWidth,10.0);
    lineStyle.SetIntValue(osmscout::LineStyle::attrPriority,1);
    styleConfig->AddWayLineStyle(wayFilter,lineStyle);

    osmscout::TypeInfoSet areaTypes(*types.typeConfig);

    areaTypes.Set(types.areaType);

    osmscout::StyleFilter areaFilter;

    areaFilter.SetTypes(areaTypes);

    osmscout::FillPartialStyle fillStyle;

    fillStyle.SetColorValue(osmscout::FillStyle::attrFillColor,osmscout::Color(0.0,1.0,0.0));
    styleConfig->AddAreaFillStyle(areaFilter,fillStyle);

    osmscout::TypeInfoSet routeTypes(*types.typeConfig);

    routeTypes.Set(types.routeType);

    osmscout::StyleFilter routeFilter;

    routeFilter.SetTypes(routeTypes);

    osmscout::LinePartialStyle routeLineStyle;

    routeLineStyle.SetColorValue(osmscout::LineStyle::attrLineColor,osmscout::Color(1.0,0.0,0.0));
    routeLineStyle.SetDoubleValue(osmscout::LineStyle::attrDisplayWidth,2.0);
    routeLineStyle.SetDoubleValue(osmscout::LineStyle::attrWidth,10.0);
    routeLineStyle.SetIntValue(osmscout::LineStyle::attrPriority,1);
    styleConfig->AddRouteLineStyle(routeFilter,routeLineStyle);

    osmscout::INameLabelProviderFactory labelFactory;

    osmscout::PathTextPartialStyle      routeTextStyle;

    routeTextStyle.style->SetLabel(labelFactory.Create(*types.typeConfig));
    routeTextStyle.SetDoubleValue(osmscout::PathTextStyle::attrSize,10.0);
    routeTextStyle.SetColorValue(osmscout::PathTextStyle::attrTextColor,osmscout::Color(0.0,0.0,0.0));
    routeTextStyle.SetUIntValue(osmscout::PathTextStyle::attrPriority,1);
    styleConfig->AddRoutePathTextStyle(routeFilter,routeTextStyle);

    styleConfig->Postprocess();

    return styleConfig;
  }

// ---------------------------------------------------------------------------
// Synthetic objects. Ways and areas cannot be assigned a file offset through
// the public API, so all of them carry offset 0 and identity is established
// through other visible state of the prepared entries.
// ---------------------------------------------------------------------------
  osmscout::WayRef MakeWay(const osmscout::TypeInfoRef& type,
                           const osmscout::GeoCoord& c1,
                           const osmscout::GeoCoord& c2,
                           int8_t layer)
  {
    auto way=std::make_shared<osmscout::Way>();

    way->nodes.push_back(osmscout::Point(1,c1));
    way->nodes.push_back(osmscout::Point(2,c2));

    osmscout::FeatureValueBuffer buffer;

    buffer.SetType(type);

    if (layer!=0) {
      for (size_t i=0; i<type->GetFeatureCount(); ++i) {
        if (type->GetFeature(i).GetFeature()->GetName()=="Layer") {
          auto * value=buffer.AllocateValue(i);

          static_cast<osmscout::LayerFeatureValue*>(value)->SetLayer(layer);
          break;
        }
      }
    }

    way->SetFeatures(buffer);

    return way;
  }

  osmscout::AreaRef MakeArea(const osmscout::TypeInfoRef& type,
                             const osmscout::GeoCoord& center,
                             double interiorLon)
  {
    auto                 area=std::make_shared<osmscout::Area>();

    osmscout::Area::Ring ring;

    ring.MarkAsOuterRing();
    ring.SetType(type);

    // A ring with four nodes, so the ring has a bounding box and is not skipped
    ring.nodes.push_back(osmscout::Point(1,osmscout::GeoCoord(50.0000,8.0000)));
    ring.nodes.push_back(osmscout::Point(2,osmscout::GeoCoord(50.0000,8.0020)));
    ring.nodes.push_back(osmscout::Point(3,osmscout::GeoCoord(50.0020,8.0020)));
    ring.nodes.push_back(osmscout::Point(4,osmscout::GeoCoord(50.0020,8.0000)));
    // Interior node, different per area, so a prepared area can be identified
    ring.nodes.push_back(osmscout::Point(5,osmscout::GeoCoord(50.0010,interiorLon)));

    // All areas of the ordering test share this center
    ring.center=center;

    area->rings.push_back(ring);

    return area;
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

  void Render(RecordingPainter& painter,
              const osmscout::MercatorProjection& projection,
              const osmscout::MapData& data)
  {
    osmscout::MapParameter parameter;

    REQUIRE(painter.DrawMap(projection,
                            parameter,
                            std::vector<osmscout::MapData> {data}));
  }

  osmscout::GeoCoord RouteStart()
  {
    return {50.0005,8.0005};
  }

  osmscout::GeoCoord RouteEnd()
  {
    return {50.0005,8.0015};
  }
} // namespace

// ---------------------------------------------------------------------------
// Requirement: Prepared frame data is reused across frames
// ---------------------------------------------------------------------------
TEST_CASE("Prepared stores are reused between frames","[MapPainterFrameBuffers]")
{
  auto              types=MakeTypes();
  auto              styleConfig=MakeStyles(types);

  osmscout::MapData data;

  data.styleConfig=styleConfig;
  data.ways.push_back(MakeWay(types.wayType,RouteStart(),RouteEnd(),0));
  data.ways.push_back(MakeWay(types.wayType,{50.0025,8.0005},{50.0025,8.0015},0));

  RecordingPainter painter;

  auto             projection=MakeProjection();

  Render(painter,projection,data);

  size_t     wayCapacity=0;
  const void * wayBuffer=nullptr;
  size_t     areaCapacity=0;
  const void * areaBuffer=nullptr;
  size_t     wayCount=0;
  size_t     areaCount=0;

  wayCapacity=painter.Ways().capacity();
  wayBuffer=painter.Ways().data();
  areaCapacity=painter.Areas().capacity();
  areaBuffer=painter.Areas().data();
  wayCount=painter.Ways().size();

  REQUIRE(wayCount==2);

  Render(painter,projection,data);

  // The stores keep their capacity and their storage between the frames
  REQUIRE(painter.Ways().capacity()>=wayCapacity);
  REQUIRE(painter.Ways().data()==wayBuffer);
  REQUIRE(painter.Ways().size()==wayCount);
  REQUIRE(painter.Areas().capacity()>=areaCapacity);
  REQUIRE(painter.Areas().data()==areaBuffer);
  REQUIRE(painter.Areas().size()==areaCount);
}

// ---------------------------------------------------------------------------
// Requirement: Prepared frame data is contiguous per object kind
// ---------------------------------------------------------------------------
TEST_CASE("Prepared areas and ways are contiguous","[MapPainterFrameBuffers]")
{
  auto              types=MakeTypes();
  auto              styleConfig=MakeStyles(types);

  osmscout::MapData data;

  data.styleConfig=styleConfig;
  data.ways.push_back(MakeWay(types.wayType,RouteStart(),RouteEnd(),0));
  data.ways.push_back(MakeWay(types.wayType,{50.0025,8.0005},{50.0025,8.0015},0));
  data.areas.push_back(MakeArea(types.areaType,{50.0010,8.0010},8.0005));
  data.areas.push_back(MakeArea(types.areaType,{50.0010,8.0010},8.0015));

  RecordingPainter painter;

  auto             projection=MakeProjection();

  Render(painter,projection,data);

  const auto & areas=painter.Areas();
  const auto & ways=painter.Ways();

  REQUIRE(areas.size()==2);
  REQUIRE(ways.size()==2);

  // Consecutive prepared entries are adjacent, i.e. the store is one block
  REQUIRE(reinterpret_cast<const char*>(&areas[1])-reinterpret_cast<const char*>(&areas[0])==
          static_cast<ptrdiff_t>(sizeof(osmscout::MapPainter::AreaData)));
  REQUIRE(reinterpret_cast<const char*>(&ways[1])-reinterpret_cast<const char*>(&ways[0])==
          static_cast<ptrdiff_t>(sizeof(osmscout::MapPainter::WayData)));
}

// ---------------------------------------------------------------------------
// Requirement: Draw order is stable for prepared areas and ways
// ---------------------------------------------------------------------------
TEST_CASE("Equal-comparing areas keep their preparation order","[MapPainterFrameBuffers]")
{
  auto types=MakeTypes();
  auto styleConfig=MakeStyles(types);

  // All areas share the bounding box and the outer/inner role, so the area
  // ordering compares them equal. They are identified by their center.
  const size_t      areaCount=32;

  osmscout::MapData data;

  data.styleConfig=styleConfig;

  std::vector<double> expectedCenters;

  for (size_t i=0; i<areaCount; i++) {
    double lon=8.0004+0.00004*(double)i;

    data.areas.push_back(MakeArea(types.areaType,{50.0010,lon},lon));
    expectedCenters.push_back(lon);
  }

  RecordingPainter painter;

  auto             projection=MakeProjection();

  Render(painter,projection,data);

  REQUIRE(painter.Areas().size()==areaCount);

  std::vector<double> firstOrder;

  for (const auto& area : painter.Areas()) {
    REQUIRE(area.center.has_value());
    firstOrder.push_back(area.center->GetLon());
  }

  // The preparation order is preserved for entries that compare equal
  REQUIRE(firstOrder==expectedCenters);

  // ... and it is the same in the next frame
  Render(painter,projection,data);

  std::vector<double> secondOrder;

  for (const auto& area : painter.Areas()) {
    REQUIRE(area.center.has_value());
    secondOrder.push_back(area.center->GetLon());
  }

  REQUIRE(secondOrder==firstOrder);
}

// ---------------------------------------------------------------------------
// Requirement: Route labels resolve their prepared way path
// ---------------------------------------------------------------------------
TEST_CASE("Route labels keep resolving their prepared way path","[MapPainterFrameBuffers]")
{
  auto types=MakeTypes();
  auto styleConfig=MakeStyles(types);

  auto memberWay=MakeWay(types.wayType,RouteStart(),RouteEnd(),0);

  auto route=std::make_shared<osmscout::Route>();

  route->SetType(types.routeType);

  osmscout::FeatureValueBuffer routeBuffer;

  routeBuffer.SetType(types.routeType);

  for (size_t i=0; i<types.routeType->GetFeatureCount(); ++i) {
    if (types.routeType->GetFeature(i).GetFeature()->GetName()=="Name") {
      auto * value=routeBuffer.AllocateValue(i);

      static_cast<osmscout::NameFeatureValue*>(value)->SetName("Test route");
      break;
    }
  }

  route->SetFeatures(routeBuffer);
  route->bbox=osmscout::GeoBox(RouteStart(),RouteEnd());

  osmscout::Route::SegmentMember member;

  member.direction=osmscout::Route::MemberDirection::forward;
  member.way=memberWay->GetFileOffset();

  osmscout::Route::Segment routeSegment;

  routeSegment.members.push_back(member);
  route->segments.push_back(routeSegment);

  // Frame 1: the route member way is rendered from the map data
  osmscout::MapData data;

  data.styleConfig=styleConfig;
  data.ways.push_back(memberWay);
  data.routes.push_back(route);

  RecordingPainter   painter;

  auto               projection=MakeProjection();

  osmscout::Vertex2D expectedStart;
  osmscout::Vertex2D expectedEnd;

  projection.GeoToPixel(RouteStart(),expectedStart);
  projection.GeoToPixel(RouteEnd(),expectedEnd);

  Render(painter,projection,data);

  REQUIRE(painter.contourLabels.size()==1);
  REQUIRE(painter.contourLabels.front().ref.GetFileOffset()==memberWay->GetFileOffset());
  REQUIRE(painter.contourLabels.front().text=="Test route");
  REQUIRE(painter.contourLabels.front().hasGeometry);
  REQUIRE(std::abs(painter.contourLabels.front().start.GetX()-expectedStart.GetX())<2.0);
  REQUIRE(std::abs(painter.contourLabels.front().start.GetY()-expectedStart.GetY())<2.0);
  REQUIRE(std::abs(painter.contourLabels.front().end.GetX()-expectedEnd.GetX())<2.0);
  REQUIRE(std::abs(painter.contourLabels.front().end.GetY()-expectedEnd.GetY())<2.0);

  // Frame 2: the route member way is not part of the map data, so the route
  // processing appends a prepared way path of its own, while further prepared
  // data is added and the prepared stores are ordered again
  painter.contourLabels.clear();

  osmscout::Route::MemberCache resolvedMembers;

  resolvedMembers[memberWay->GetFileOffset()]=memberWay;
  route->SetResolvedMembers(resolvedMembers);

  osmscout::MapData resolvedData;

  resolvedData.styleConfig=styleConfig;
  resolvedData.routes.push_back(route);

  for (size_t i=0; i<4; i++) {
    resolvedData.areas.push_back(MakeArea(types.areaType,{50.0010,8.0005},8.0005));
  }

  Render(painter,projection,resolvedData);

  REQUIRE(painter.Areas().size()==4);
  REQUIRE(painter.contourLabels.size()==1);
  REQUIRE(painter.contourLabels.front().ref.GetFileOffset()==memberWay->GetFileOffset());
  REQUIRE(painter.contourLabels.front().text=="Test route");
  REQUIRE(painter.contourLabels.front().hasGeometry);
  REQUIRE(std::abs(painter.contourLabels.front().start.GetX()-expectedStart.GetX())<2.0);
  REQUIRE(std::abs(painter.contourLabels.front().start.GetY()-expectedStart.GetY())<2.0);
  REQUIRE(std::abs(painter.contourLabels.front().end.GetX()-expectedEnd.GetX())<2.0);
  REQUIRE(std::abs(painter.contourLabels.front().end.GetY()-expectedEnd.GetY())<2.0);
}

// ---------------------------------------------------------------------------
// Requirement: Backends retain read access to prepared areas and ways
// ---------------------------------------------------------------------------
TEST_CASE("Backend callback sees every prepared area and way in draw order","[MapPainterFrameBuffers]")
{
  auto              types=MakeTypes();
  auto              styleConfig=MakeStyles(types);

  osmscout::MapData data;

  data.styleConfig=styleConfig;
  data.ways.push_back(MakeWay(types.wayType,RouteStart(),RouteEnd(),0));
  data.ways.push_back(MakeWay(types.wayType,{50.0025,8.0005},{50.0025,8.0015},1));
  data.ways.push_back(MakeWay(types.wayType,{50.0030,8.0005},{50.0030,8.0015},-1));
  data.areas.push_back(MakeArea(types.areaType,{50.0010,8.0010},8.0005));
  data.areas.push_back(MakeArea(types.areaType,{50.0010,8.0010},8.0015));

  RecordingPainter painter;

  auto             projection=MakeProjection();

  Render(painter,projection,data);

  REQUIRE(painter.callbackCount==1);

  // The callback observes every prepared entry ...
  REQUIRE(painter.callbackWays.size()==painter.Ways().size());
  REQUIRE(painter.callbackAreaRefs.size()==painter.Areas().size());

  REQUIRE(painter.drawnWays.size()==painter.Ways().size());
  REQUIRE(painter.drawnAreaRefs.size()==painter.Areas().size());

  // ... and in the same order in which the entries are drawn
  REQUIRE(painter.callbackWays==painter.drawnWays);
  REQUIRE(painter.callbackAreaRefs==painter.drawnAreaRefs);
}
