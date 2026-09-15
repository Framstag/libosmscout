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

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <osmscout/Area.h>
#include <osmscout/GeoCoord.h>
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
#include <osmscoutmap/StyleProcessor.h>
#include <osmscoutmap/Styles.h>

/*
 * Tests for the contract of the painter's early rejection of areas that cannot be visible: an
 * area that a per-ring visibility decision could never keep is rejected before its rings are
 * prepared, the tolerance of that decision covers every per-ring tolerance the loaded style
 * sheet can produce, and the prepared area entries, their clipping geometry and their draw
 * order are exactly what the unculled pipeline produces.
 *
 * The tests drive the painter directly with synthetic objects, so they need no database and no
 * style sheet file. Areas are placed relative to the viewport using pixel distances measured
 * from the projection, so the tests do not depend on the absolute scale of a magnification level.
 */

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

    const std::vector<osmscout::MapPainter::WayData>& Ways() const
    {
      return GetWayData();
    }

  protected:
    void AfterPreprocessingCallback(const osmscout::Projection& /*projection*/,
                                    const osmscout::MapParameter& /*parameter*/,
                                    const std::vector<osmscout::MapData>& /*data*/) override
    {
      // no code
    }
  };

  /**
   * Snapshot of a prepared entry. A CoordBufferRange refers to the coordinate buffer of the
   * painter that produced it, which does not outlive the painter, so the coordinates have to be
   * copied while the painter is alive. Preparation may renumber the coordinate buffer of a
   * frame, so the coordinates are the property to compare, not the indices of a range.
   */
  struct EntrySnapshot
  {
    std::string                      typeName;
    bool                             isOuter=false;
    std::vector<double>              coords;    //!< x,y pairs
    std::vector<std::vector<double>> clippings; //!< x,y pairs per clipping ring
  };

  std::vector<double> RangeCoordinates(const osmscout::CoordBufferRange& range)
  {
    std::vector<double> coords;

    coords.reserve(2*range.GetSize());

    for (size_t i=0; i<range.GetSize(); i++) {
      osmscout::Vertex2D coord=range.Get(range.GetStart()+i);

      coords.push_back(coord.GetX());
      coords.push_back(coord.GetY());
    }

    return coords;
  }

  std::vector<EntrySnapshot> Snapshot(const RecordingPainter& painter)
  {
    std::vector<EntrySnapshot> result;

    for (const auto& area : painter.Areas()) {
      EntrySnapshot entry;

      entry.typeName=area.type ? area.type->GetName() : std::string{};
      entry.isOuter=area.isOuter;
      entry.coords=RangeCoordinates(area.coordRange);

      for (const auto& clipping : area.clippings) {
        entry.clippings.push_back(RangeCoordinates(clipping));
      }

      result.push_back(entry);
    }

    return result;
  }

// ---------------------------------------------------------------------------
// Counts how often per-ring style resolution reached a ring. The fill style
// processor of a type is invoked by the per-ring preparation right after the
// fill style of the ring has been resolved, so an invocation means "this ring
// was prepared ring by ring".
// ---------------------------------------------------------------------------
  class CountingFillStyleProcessor : public osmscout::FillStyleProcessor
  {
  public:
    osmscout::FillStyleRef Process(const osmscout::FeatureValueBuffer& /*features*/,
                                   const osmscout::FillStyleRef& style) const override
    {
      invocations++;

      return style;
    }

    mutable size_t invocations=0;
  };

// ---------------------------------------------------------------------------
// Type config: one way type that the style sheet draws as a line, one area type
// that the style sheet styles by fill and border, and one type for inner rings
// that act as clipping regions
// ---------------------------------------------------------------------------
  struct TestTypes
  {
    osmscout::TypeConfigRef typeConfig;
    osmscout::TypeInfoRef   wayType;
    osmscout::TypeInfoRef   styledAreaType;
    osmscout::TypeInfoRef   clippingRingType;
  };

  TestTypes MakeTypes()
  {
    TestTypes types;

    types.typeConfig=std::make_shared<osmscout::TypeConfig>();

    types.wayType=std::make_shared<osmscout::TypeInfo>("test_way");
    types.wayType->CanBeWay(true);
    types.typeConfig->RegisterType(types.wayType);

    types.styledAreaType=std::make_shared<osmscout::TypeInfo>("test_area_styled");
    types.styledAreaType->CanBeArea(true);
    types.typeConfig->RegisterType(types.styledAreaType);

    types.clippingRingType=std::make_shared<osmscout::TypeInfo>("test_area_clipping_ring");
    types.clippingRingType->CanBeArea(true);
    types.clippingRingType->SetIgnore(true);
    types.typeConfig->RegisterType(types.clippingRingType);

    return types;
  }

// ---------------------------------------------------------------------------
// Style sheet: a fill style plus a border style of the given width in
// millimetres. The level range makes it possible to give levels different
// border widths.
// ---------------------------------------------------------------------------
  osmscout::StyleConfigRef MakeStyles(const TestTypes& types,
                                      double borderWidthMM,
                                      size_t borderMinLevel,
                                      size_t borderMaxLevel)
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
    styleConfig->AddWayLineStyle(wayFilter,lineStyle);

    osmscout::TypeInfoSet areaTypes(*types.typeConfig);

    areaTypes.Set(types.styledAreaType);

    osmscout::StyleFilter areaFilter;

    areaFilter.SetTypes(areaTypes);

    osmscout::FillPartialStyle fillStyle;

    fillStyle.SetColorValue(osmscout::FillStyle::attrFillColor,osmscout::Color(0.0,1.0,0.0));
    styleConfig->AddAreaFillStyle(areaFilter,fillStyle);

    osmscout::StyleFilter borderFilter;

    borderFilter.SetTypes(areaTypes);
    borderFilter.SetMinLevel(borderMinLevel);
    borderFilter.SetMaxLevel(borderMaxLevel);

    osmscout::BorderPartialStyle borderStyle;

    borderStyle.SetDoubleValue(osmscout::BorderStyle::attrWidth,borderWidthMM);
    borderStyle.SetColorValue(osmscout::BorderStyle::attrColor,osmscout::Color(1.0,0.0,0.0));
    styleConfig->AddAreaBorderStyle(borderFilter,borderStyle);

    styleConfig->Postprocess();

    return styleConfig;
  }

// ---------------------------------------------------------------------------
// Synthetic areas
// ---------------------------------------------------------------------------

  /**
   * Half extent of a synthetic rectangle in degrees, per axis, so that a test can state the size
   * of an area in pixels.
   */
  struct Extent
  {
    double halfLat=0.0;
    double halfLon=0.0;
  };

  /**
   * Area with one outer ring that is an axis parallel rectangle of the given extent around the
   * given center.
   */
  osmscout::AreaRef MakeArea(const osmscout::TypeInfoRef& type,
                             const osmscout::GeoCoord& center,
                             const Extent& extent)
  {
    auto                 area=std::make_shared<osmscout::Area>();

    osmscout::Area::Ring ring;

    ring.MarkAsOuterRing();
    ring.SetType(type);

    ring.nodes.push_back(osmscout::Point(1,
                                         osmscout::GeoCoord(center.GetLat()-extent.halfLat,
                                                            center.GetLon()-extent.halfLon)));
    ring.nodes.push_back(osmscout::Point(2,
                                         osmscout::GeoCoord(center.GetLat()-extent.halfLat,
                                                            center.GetLon()+extent.halfLon)));
    ring.nodes.push_back(osmscout::Point(3,
                                         osmscout::GeoCoord(center.GetLat()+extent.halfLat,
                                                            center.GetLon()+extent.halfLon)));
    ring.nodes.push_back(osmscout::Point(4,
                                         osmscout::GeoCoord(center.GetLat()+extent.halfLat,
                                                            center.GetLon()-extent.halfLon)));

    ring.center=center;

    area->rings.push_back(ring);

    return area;
  }

  /**
   * Append an inner ring without a own style to the given area. Such a ring is not drawn but acts
   * as clipping region of the ring it is nested in.
   */
  void AddClippingRing(const osmscout::AreaRef& area,
                       const osmscout::TypeInfoRef& clippingType,
                       const osmscout::GeoCoord& center,
                       const Extent& extent)
  {
    osmscout::Area::Ring ring;

    // Level 2, i.e. an inner ring of the top level outer ring
    ring.SetRing(2);
    ring.SetType(clippingType);

    ring.nodes.push_back(osmscout::Point(1,
                                         osmscout::GeoCoord(center.GetLat()-extent.halfLat,
                                                            center.GetLon()-extent.halfLon)));
    ring.nodes.push_back(osmscout::Point(2,
                                         osmscout::GeoCoord(center.GetLat()-extent.halfLat,
                                                            center.GetLon()+extent.halfLon)));
    ring.nodes.push_back(osmscout::Point(3,
                                         osmscout::GeoCoord(center.GetLat()+extent.halfLat,
                                                            center.GetLon()+extent.halfLon)));
    ring.nodes.push_back(osmscout::Point(4,
                                         osmscout::GeoCoord(center.GetLat()+extent.halfLat,
                                                            center.GetLon()-extent.halfLon)));

    area->rings.push_back(ring);
  }

  /**
   * Way with two nodes, drawn by the line style of MakeStyles.
   */
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
   * Projection of a fixed viewport, set with the DPI first and the image dimensions second, so the
   * viewport is 400x400 pixels at 300 DPI.
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

  void Render(RecordingPainter& painter,
              const osmscout::MercatorProjection& projection,
              const osmscout::MapParameter& parameter,
              const osmscout::MapData& data)
  {
    REQUIRE(painter.DrawMap(projection,
                            parameter,
                            std::vector<osmscout::MapData> {data}));
  }

  /**
   * Degrees of longitude that correspond to the given number of pixels, measured from the
   * projection itself. The projection center is the middle of the screen box.
   */
  double DegreesLonForPixels(const osmscout::MercatorProjection& projection,
                             double pixels)
  {
    constexpr double   probeDeltaDegrees=0.001;

    osmscout::Vertex2D a{};
    osmscout::Vertex2D b{};

    REQUIRE(projection.GeoToPixel(projection.GetCenter(),a));
    REQUIRE(projection.GeoToPixel(osmscout::GeoCoord(projection.GetCenter().GetLat(),
                                                     projection.GetCenter().GetLon()+probeDeltaDegrees),
                                  b));

    double pixelsPerDegree=(b.GetX()-a.GetX())/probeDeltaDegrees;

    REQUIRE(pixelsPerDegree>0.0);

    return pixels/pixelsPerDegree;
  }

  /**
   * Degrees of latitude that correspond to the given number of pixels.
   */
  double DegreesLatForPixels(const osmscout::MercatorProjection& projection,
                             double pixels)
  {
    constexpr double   probeDeltaDegrees=0.001;

    osmscout::Vertex2D a{};
    osmscout::Vertex2D b{};

    REQUIRE(projection.GeoToPixel(projection.GetCenter(),a));
    REQUIRE(projection.GeoToPixel(osmscout::GeoCoord(projection.GetCenter().GetLat()+probeDeltaDegrees,
                                                     projection.GetCenter().GetLon()),
                                  b));

    // Screen y grows southwards, so a more northern probe is above the center
    double pixelsPerDegree=(a.GetY()-b.GetY())/probeDeltaDegrees;

    REQUIRE(pixelsPerDegree>0.0);

    return pixels/pixelsPerDegree;
  }

  /**
   * Extent of a rectangle with the given half size in pixels. Both axes have a different scale,
   * so both are derived separately.
   */
  Extent ExtentForPixels(const osmscout::MercatorProjection& projection,
                         double halfSizePx)
  {
    return {DegreesLatForPixels(projection,halfSizePx),
            DegreesLonForPixels(projection,halfSizePx)};
  }

  /**
   * Longitude well outside the view of the given projection, derived from the geographic
   * dimensions of that view so that the value does not depend on the scale of a magnification
   * level. The index moves the position further out.
   */
  double OutsideLon(const osmscout::MercatorProjection& projection,
                    size_t index)
  {
    osmscout::GeoBox dimensions=projection.GetDimensions();

    double           viewportWidth=dimensions.GetMaxLon()-dimensions.GetMinLon();

    REQUIRE(viewportWidth>0.0);

    return projection.GetCenter().GetLon()+(2.0*viewportWidth)+(0.001*viewportWidth*(double)index);
  }

  /**
   * A visibility decision extends a ring by half of a border width, the relation the painter uses
   * when it derives the tolerance of its early decision and the per-ring decision uses for a ring.
   */
  constexpr double borderWidthToTolerance=0.5;
} // namespace

// ---------------------------------------------------------------------------
// Requirement: The early rejection is conservative
// ---------------------------------------------------------------------------

TEST_CASE("The border width bound is the widest border style of a level","[MapPainterAreaVisibilityCull]")
{
  auto                  types=MakeTypes();

  auto                  styleConfig=std::make_shared<osmscout::StyleConfig>(types.typeConfig);

  osmscout::TypeInfoSet areaTypes(*types.typeConfig);

  areaTypes.Set(types.styledAreaType);

  osmscout::StyleFilter areaFilter;

  areaFilter.SetTypes(areaTypes);

  osmscout::FillPartialStyle fillStyle;

  fillStyle.SetColorValue(osmscout::FillStyle::attrFillColor,osmscout::Color(0.0,1.0,0.0));
  styleConfig->AddAreaFillStyle(areaFilter,fillStyle);

  auto addBorder=[&](double width,size_t minLevel,size_t maxLevel) {
                    osmscout::StyleFilter filter;

                    filter.SetTypes(areaTypes);
                    filter.SetMinLevel(minLevel);
                    filter.SetMaxLevel(maxLevel);

                    osmscout::BorderPartialStyle borderStyle;

                    borderStyle.SetDoubleValue(osmscout::BorderStyle::attrWidth,width);
                    borderStyle.SetColorValue(osmscout::BorderStyle::attrColor,osmscout::Color(1.0,0.0,0.0));
                    styleConfig->AddAreaBorderStyle(filter,borderStyle);
                  };

  addBorder(1.0,0,9);
  addBorder(2.0,10,14);
  addBorder(0.5,15,19);

  styleConfig->Postprocess();

  auto boundAt=[&](size_t level) {
                  return styleConfig->GetMaxAreaBorderWidthMM(osmscout::Magnification(osmscout::MagnificationLevel(
                                                                                        level)));
                };

  INFO("bound at level 5: " << boundAt(5));
  INFO("bound at level 12: " << boundAt(12));
  INFO("bound at level 17: " << boundAt(17));

  REQUIRE(boundAt(5)==1.0);
  REQUIRE(boundAt(12)==2.0);
  REQUIRE(boundAt(17)==0.5);
}

TEST_CASE("An area within the border tolerance is not rejected","[MapPainterAreaVisibilityCull]")
{
  auto         types=MakeTypes();
  auto         projection=MakeProjection();
  auto         parameter=MakeParameter();

  const double screenRight=projection.GetWidth();
  const double screenMiddleX=screenRight/2.0;
  const double halfSizePx=60.0;

  // The area has to be larger than the minimum dimension, else it is rejected for being tiny
  REQUIRE(halfSizePx>projection.ConvertWidthToPixel(parameter.GetAreaMinDimensionMM()));

  const Extent       extent=ExtentForPixels(projection,halfSizePx);

  osmscout::Vertex2D centerPixel{};

  REQUIRE(projection.GeoToPixel(projection.GetCenter(),centerPixel));
  REQUIRE(centerPixel.GetX()==screenMiddleX);

  // Two style sheets whose area borders differ in width by a factor of 1000, so that the per-ring
  // tolerance differs by the same factor and the early decision has to follow it
  double previousBound=0.0;

  for (double borderWidthMM : {0.1,100.0}) {
    // The per-ring decision extends a ring by half of the raw style width
    const double perRingTolerancePx=borderWidthMM*borderWidthToTolerance;

    auto         styleConfig=MakeStyles(types,borderWidthMM,0,25);

    const double bound=styleConfig->GetMaxAreaBorderWidthMM(projection.GetMagnification());

    INFO("border width, mm: " << borderWidthMM);
    INFO("tolerance of a per-ring decision: " << perRingTolerancePx);
    INFO("derived bound, mm: " << bound);

    // The bound grows with the style sheet, so the second style sheet gets the larger tolerance
    REQUIRE(bound>previousBound);

    previousBound=bound;

    // And it is never smaller than the tolerance of the per-ring decision, whatever the DPI of the
    // projection, because both are half of a border width of the same style sheet
    REQUIRE(bound*borderWidthToTolerance>=perRingTolerancePx);

    /**
     * Prepare a frame with a single area whose left screen edge is at the given x position.
     */
    auto preparedWithLeftEdgeAt=[&](double leftEdgePx) {
                                   auto             data=MakeData(styleConfig);
                                   RecordingPainter painter;

                                   // The area center is half its width to the right of its left edge
                                   double centerLonOffset=DegreesLonForPixels(projection,
                                                                              leftEdgePx+halfSizePx-screenMiddleX);

                                   data.areas.push_back(MakeArea(types.styledAreaType,
                                                                 {projection.GetCenter().GetLat(),
                                                                  projection.GetCenter().GetLon()+centerLonOffset},
                                                                 extent));

                                   Render(painter,projection,parameter,data);

                                   return painter.Areas().size();
                                 };

    // Inside the viewport
    REQUIRE(preparedWithLeftEdgeAt(screenRight-(2.0*halfSizePx))==1);

    // Outside the viewport but within the tolerance of the per-ring decision: the early decision
    // has to use at least the tolerance the per-ring decision can use, so this area must survive
    REQUIRE(preparedWithLeftEdgeAt(screenRight+perRingTolerancePx-10.0)==1);

    // Far outside any tolerance: nothing is prepared, which is what makes the assertion above a
    // statement about the early decision and not about a pipeline that prepares everything
    REQUIRE(preparedWithLeftEdgeAt(screenRight+(20.0*perRingTolerancePx))==0);
  }
}

// ---------------------------------------------------------------------------
// Requirement: Preparation work follows the visible areas, not the loaded ones
// ---------------------------------------------------------------------------

TEST_CASE("Areas outside the view are not prepared ring by ring","[MapPainterAreaVisibilityCull]")
{
  auto types=MakeTypes();
  auto styleConfig=MakeStyles(types,0.1,0,25);
  auto projection=MakeProjection();
  auto parameter=MakeParameter();

  auto countingProcessor=std::make_shared<CountingFillStyleProcessor>();

  parameter.RegisterFillStyleProcessor(types.styledAreaType->GetIndex(),
                                       countingProcessor);

  const size_t     outsideCount=64;

  const Extent     extent=ExtentForPixels(projection,60.0);

  auto             data=MakeData(styleConfig);
  RecordingPainter painter;

  // One area inside the view, so the pipeline does have work to do
  data.areas.push_back(MakeArea(types.styledAreaType,
                                projection.GetCenter(),
                                extent));

  for (size_t i=0; i<outsideCount; i++) {
    data.areas.push_back(MakeArea(types.styledAreaType,
                                  {projection.GetCenter().GetLat(),
                                   OutsideLon(projection,i)},
                                  extent));
  }

  Render(painter,projection,parameter,data);

  // Only the area inside the view is prepared
  REQUIRE(painter.Areas().size()==1);

  // And per-ring style resolution ran for that area's single ring only, not for the 64 loaded
  // areas outside the view
  INFO("style resolution invocations: " << countingProcessor->invocations);
  REQUIRE(countingProcessor->invocations==1);
}

// ---------------------------------------------------------------------------
// Requirement: Prepared entries, clipping geometry, orders and rendered output
// are unchanged
// ---------------------------------------------------------------------------

TEST_CASE("Prepared entries and clipping geometry do not depend on the loaded areas","[MapPainterAreaVisibilityCull]")
{
  auto                     types=MakeTypes();
  auto                     styleConfig=MakeStyles(types,0.1,0,25);
  auto                     projection=MakeProjection();
  auto                     parameter=MakeParameter();

  const osmscout::GeoCoord center=projection.GetCenter();

  const Extent             extent=ExtentForPixels(projection,60.0);

  const Extent             clippingExtent=ExtentForPixels(projection,15.0);

  /**
   * Prepared frame of a view: the prepared area entries and the number of prepared ways.
   */
  struct ViewSnapshot
  {
    std::vector<EntrySnapshot> areas;
    size_t                     ways=0;
  };

  /**
   * Load a visible way, the two visible areas, one of them with a clipping ring, and the given
   * number of areas far outside the view.
   */
  auto renderView=[&](size_t outsideCount) {
                     auto                     data=MakeData(styleConfig);
                     RecordingPainter         painter;

                     const osmscout::GeoCoord outerCenter{center.GetLat()-(0.1*extent.halfLat),
                                                          center.GetLon()-(0.1*extent.halfLon)};

                     auto outer=MakeArea(types.styledAreaType,
                                         outerCenter,
                                         extent);

                     AddClippingRing(outer,
                                     types.clippingRingType,
                                     outerCenter,
                                     clippingExtent);

                     data.areas.push_back(outer);

                     data.areas.push_back(MakeArea(types.styledAreaType,
                                                   {center.GetLat()+(0.1*extent.halfLat),
                                                    center.GetLon()+(0.1*extent.halfLon)},
                                                   extent));

                     // A way inside the view, so the frame prepares ways as well
                     data.ways.push_back(MakeWay(types.wayType,
                                                 {center.GetLat()-(0.5*extent.halfLat),
                                                  center.GetLon()-(0.5*extent.halfLon)},
                                                 {center.GetLat()+(0.5*extent.halfLat),
                                                  center.GetLon()+(0.5*extent.halfLon)}));

                     for (size_t i=0; i<outsideCount; i++) {
                       data.areas.push_back(MakeArea(types.styledAreaType,
                                                     {center.GetLat(),
                                                      OutsideLon(projection,i)},
                                                     extent));
                     }

                     Render(painter,projection,parameter,data);

                     ViewSnapshot snapshot;

                     snapshot.areas=Snapshot(painter);
                     snapshot.ways=painter.Ways().size();

                     return snapshot;
                   };

  auto reference=renderView(0);
  auto culled=renderView(128);

  // The way is prepared, so the comparison of the ways below is not vacuous
  REQUIRE(reference.ways==1);

  // Loading further areas outside the view does not change the prepared ways
  REQUIRE(culled.ways==reference.ways);

  REQUIRE(reference.areas.size()==2);
  REQUIRE(culled.areas.size()==reference.areas.size());

  for (size_t i=0; i<reference.areas.size(); i++) {
    INFO("entry " << i);

    const EntrySnapshot & expected=reference.areas.at(i);
    const EntrySnapshot & actual=culled.areas.at(i);

    REQUIRE(actual.typeName==expected.typeName);
    REQUIRE(actual.isOuter==expected.isOuter);

    REQUIRE(actual.coords.size()==expected.coords.size());

    for (size_t j=0; j<expected.coords.size(); j++) {
      REQUIRE(actual.coords.at(j)==expected.coords.at(j));
    }

    REQUIRE(actual.clippings.size()==expected.clippings.size());

    for (size_t j=0; j<expected.clippings.size(); j++) {
      const std::vector<double> & expectedClipping=expected.clippings.at(j);
      const std::vector<double> & actualClipping=actual.clippings.at(j);

      REQUIRE(actualClipping.size()==expectedClipping.size());

      for (size_t k=0; k<expectedClipping.size(); k++) {
        REQUIRE(actualClipping.at(k)==expectedClipping.at(k));
      }
    }
  }

  // The clipping ring of the first entry is still available
  REQUIRE(culled.areas.front().clippings.size()==1);

  // A view that loads only areas outside the viewport prepares no area at all, which is what makes
  // the comparison above a statement about the early decision rather than about a pipeline that
  // prepares everything it loaded. The way of the same view is still prepared, because the early
  // decision is about areas.
  {
    auto             data=MakeData(styleConfig);
    RecordingPainter painter;

    data.ways.push_back(MakeWay(types.wayType,
                                {center.GetLat()-(0.5*extent.halfLat),
                                 center.GetLon()-(0.5*extent.halfLon)},
                                {center.GetLat()+(0.5*extent.halfLat),
                                 center.GetLon()+(0.5*extent.halfLon)}));

    for (size_t i=0; i<128; i++) {
      data.areas.push_back(MakeArea(types.styledAreaType,
                                    {center.GetLat(),
                                     OutsideLon(projection,i)},
                                    extent));
    }

    Render(painter,projection,parameter,data);

    REQUIRE(painter.Areas().empty());
    REQUIRE(painter.Ways().size()==1);
  }
}
