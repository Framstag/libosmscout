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

  /**
   * The DPI the fixed viewport of this file is projected with and the lower DPI the test of the DPI
   * dependence compares it with.
   */
  constexpr double referenceDpi=300.0;
  constexpr double lowerDpi=96.0;

  /**
   * Highest magnification level the synthetic style sheets of the tests apply at.
   */
  constexpr size_t maxStyleLevel=25;

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
   * Nodes of an axis parallel rectangle of the given extent around the given center.
   */
  std::vector<osmscout::Point> MakeRectangleNodes(const osmscout::GeoCoord& center,
                                                  const Extent& extent)
  {
    std::vector<osmscout::Point> nodes;

    nodes.push_back(osmscout::Point(1,
                                    osmscout::GeoCoord(center.GetLat()-extent.halfLat,
                                                       center.GetLon()-extent.halfLon)));
    nodes.push_back(osmscout::Point(2,
                                    osmscout::GeoCoord(center.GetLat()-extent.halfLat,
                                                       center.GetLon()+extent.halfLon)));
    nodes.push_back(osmscout::Point(3,
                                    osmscout::GeoCoord(center.GetLat()+extent.halfLat,
                                                       center.GetLon()+extent.halfLon)));
    nodes.push_back(osmscout::Point(4,
                                    osmscout::GeoCoord(center.GetLat()+extent.halfLat,
                                                       center.GetLon()-extent.halfLon)));

    return nodes;
  }

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
    ring.nodes=MakeRectangleNodes(center,extent);
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
    ring.nodes=MakeRectangleNodes(center,extent);

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
   * viewport is 400x400 pixels at the given DPI.
   */
  osmscout::MercatorProjection MakeProjectionWithDpi(double dpi)
  {
    osmscout::MercatorProjection projection;

    REQUIRE(projection.Set(osmscout::GeoCoord(50.001,8.001),
                           osmscout::Magnification(osmscout::Magnification::magClose),
                           dpi,
                           400,
                           400));

    return projection;
  }

  /**
   * Projection of the fixed viewport at the reference DPI, the DPI the tests of this file project
   * with.
   */
  osmscout::MercatorProjection MakeProjection()
  {
    return MakeProjectionWithDpi(referenceDpi);
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
    // Screen y grows southwards, so the more northern probe is above the center
    scale.lat=(center.GetY()-north.GetY())/probeDeltaDegrees;

    REQUIRE(scale.lon>0.0);
    REQUIRE(scale.lat>0.0);

    return scale;
  }

  /**
   * Degrees of longitude that correspond to the given number of pixels. The projection center is the
   * middle of the screen box.
   */
  double DegreesLonForPixels(const osmscout::MercatorProjection& projection,
                             double pixels)
  {
    return pixels/MeasurePixelScale(projection).lon;
  }

  /**
   * Degrees of latitude that correspond to the given number of pixels.
   */
  double DegreesLatForPixels(const osmscout::MercatorProjection& projection,
                             double pixels)
  {
    return pixels/MeasurePixelScale(projection).lat;
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
   * Load the given number of areas well outside the view of the given projection.
   */
  void AddOutsideAreas(osmscout::MapData& data,
                       const osmscout::TypeInfoRef& type,
                       const osmscout::MercatorProjection& projection,
                       const osmscout::GeoCoord& center,
                       const Extent& extent,
                       size_t count)
  {
    for (size_t i=0; i<count; i++) {
      data.areas.push_back(MakeArea(type,
                                    {center.GetLat(),OutsideLon(projection,i)},
                                    extent));
    }
  }

  /**
   * Load one way inside the view, so that a frame has ways to prepare as well.
   */
  void AddVisibleWay(osmscout::MapData& data,
                     const osmscout::TypeInfoRef& type,
                     const osmscout::GeoCoord& center,
                     const Extent& extent)
  {
    data.ways.push_back(MakeWay(type,
                                {center.GetLat()-(0.5*extent.halfLat),center.GetLon()-(0.5*extent.halfLon)},
                                {center.GetLat()+(0.5*extent.halfLat),center.GetLon()+(0.5*extent.halfLon)}));
  }

  /**
   * A visibility decision extends a ring by half of a border width, the relation the painter uses
   * when it derives the tolerance of its early decision and the per-ring decision uses for a ring.
   */
  constexpr double borderWidthToTolerance=0.5;

  /**
   * One border style of a test style sheet. The slot separates the styles of one ring, so a ring
   * with two specifications resolves two border styles; the same slot would merge them into one.
   */
  struct BorderSpec
  {
    std::string slot;
    double      widthMM=0.0;
    double      offsetMapUnits=0.0;
    double      displayOffsetMM=0.0;
  };

  /**
   * Style sheet with a fill style for the area type and one border style per given specification,
   * all of them applying at every level.
   */
  osmscout::StyleConfigRef MakeStylesWithBorders(const TestTypes& types,
                                                 const std::vector<BorderSpec>& borders)
  {
    auto                  styleConfig=std::make_shared<osmscout::StyleConfig>(types.typeConfig);

    osmscout::TypeInfoSet areaTypes(*types.typeConfig);

    areaTypes.Set(types.styledAreaType);

    osmscout::StyleFilter areaFilter;

    areaFilter.SetTypes(areaTypes);

    osmscout::FillPartialStyle fillStyle;

    fillStyle.SetColorValue(osmscout::FillStyle::attrFillColor,osmscout::Color(0.0,1.0,0.0));
    styleConfig->AddAreaFillStyle(areaFilter,fillStyle);

    for (const auto& border : borders) {
      osmscout::BorderPartialStyle borderStyle;

      borderStyle.style->SetSlot(border.slot);
      borderStyle.SetDoubleValue(osmscout::BorderStyle::attrWidth,border.widthMM);
      borderStyle.SetDoubleValue(osmscout::BorderStyle::attrOffset,border.offsetMapUnits);
      borderStyle.SetDoubleValue(osmscout::BorderStyle::attrDisplayOffset,border.displayOffsetMM);
      borderStyle.SetColorValue(osmscout::BorderStyle::attrColor,osmscout::Color(1.0,0.0,0.0));
      styleConfig->AddAreaBorderStyle(areaFilter,borderStyle);
    }

    styleConfig->Postprocess();

    return styleConfig;
  }

  /**
   * Prepared area entries of a frame that loads one area of the given half size whose left screen
   * edge lies at the given x position. One or more entries means the ring passed the visibility
   * decision of the painter.
   */
  size_t PreparedEntriesForEdge(const osmscout::StyleConfigRef& styleConfig,
                                const TestTypes& types,
                                const osmscout::MercatorProjection& projection,
                                const osmscout::MapParameter& parameter,
                                double leftEdgePx,
                                double halfSizePx)
  {
    // The area has to be larger than the minimum dimension, else it is rejected for being tiny
    REQUIRE(halfSizePx>projection.ConvertWidthToPixel(parameter.GetAreaMinDimensionMM()));

    auto         data=MakeData(styleConfig);

    const Extent extent=ExtentForPixels(projection,halfSizePx);

    const double screenMiddleX=projection.GetWidth()/2.0;

    // The area center is half its width to the right of its left edge
    const double centerLonOffset=DegreesLonForPixels(projection,
                                                     leftEdgePx+halfSizePx-screenMiddleX);

    data.areas.push_back(MakeArea(types.styledAreaType,
                                  {projection.GetCenter().GetLat(),
                                   projection.GetCenter().GetLon()+centerLonOffset},
                                  extent));

    RecordingPainter painter;

    Render(painter,projection,parameter,data);

    return painter.Areas().size();
  }
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
    // The per-ring decision extends a ring by half of the style width converted to screen pixels
    const double perRingTolerancePx=projection.ConvertWidthToPixel(borderWidthMM*borderWidthToTolerance);

    auto         styleConfig=MakeStyles(types,borderWidthMM,0,25);

    const double bound=styleConfig->GetMaxAreaBorderWidthMM(projection.GetMagnification());

    INFO("border width, mm: " << borderWidthMM);
    INFO("tolerance of a per-ring decision: " << perRingTolerancePx);
    INFO("derived bound, mm: " << bound);

    // The bound grows with the style sheet, so the second style sheet gets the larger tolerance
    REQUIRE(bound>previousBound);

    previousBound=bound;

    // And it is never smaller than the tolerance of the per-ring decision, whatever the DPI of the
    // projection, because both are half of a border width of the same style sheet converted with the
    // same projection
    REQUIRE(projection.ConvertWidthToPixel(bound*borderWidthToTolerance)>=perRingTolerancePx);

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

    // The tolerance is a converted screen length, not the raw millimetre number: for a wide border
    // the converted value is far larger than the raw one, so an area that is beyond the raw number
    // still has to survive (the millimetre-as-pixel behaviour rejects it)
    const double rawTolerancePx=borderWidthMM*borderWidthToTolerance;

    if (perRingTolerancePx>rawTolerancePx+1.0) {
      REQUIRE(preparedWithLeftEdgeAt(screenRight+rawTolerancePx+1.0)==1);
    }

    // Far outside any tolerance: nothing is prepared, which is what makes the assertion above a
    // statement about the early decision and not about a pipeline that prepares everything
    REQUIRE(preparedWithLeftEdgeAt(screenRight+(20.0*perRingTolerancePx))==0);
  }
}

TEST_CASE("The border tolerance of a stylesheet width follows the DPI of the projection",
          "[MapPainterAreaVisibilityCull]")
{
  auto         types=MakeTypes();

  const double borderWidthMM=10.0;

  auto         styleConfig=MakeStyles(types,borderWidthMM,0,maxStyleLevel);

  auto         projection96=MakeProjectionWithDpi(lowerDpi);
  auto         projection300=MakeProjectionWithDpi(referenceDpi);

  // The two projections describe the same viewport and the same pixel geometry; only the DPI differs
  REQUIRE(projection96.GetWidth()==projection300.GetWidth());
  REQUIRE(projection96.GetHeight()==projection300.GetHeight());

  const double tolerancePx96=projection96.ConvertWidthToPixel(borderWidthMM*borderWidthToTolerance);
  const double tolerancePx300=projection300.ConvertWidthToPixel(borderWidthMM*borderWidthToTolerance);

  REQUIRE(tolerancePx300>tolerancePx96);

  const double halfSizePx=60.0;

  auto         parameter=MakeParameter();

  const Extent extent=ExtentForPixels(projection300,halfSizePx);

  // The area reaches into the view by just over the converted tolerance of the 96 DPI projection and
  // well within the tolerated reach of the 300 DPI projection, so only the latter may keep it
  const double reachPx=tolerancePx96+1.0;

  REQUIRE(reachPx<tolerancePx300);

  auto preparedAt=[&](const osmscout::MercatorProjection& projection) {
                     auto         data=MakeData(styleConfig);

                     const double screenRight=static_cast<double>(projection.GetWidth());
                     const double screenMiddleX=screenRight/2.0;

                     // The area center is half its width to the right of its left edge
                     double centerLonOffset=DegreesLonForPixels(projection,
                                                                screenRight+reachPx+halfSizePx-screenMiddleX);

                     data.areas.push_back(MakeArea(types.styledAreaType,
                                                   {projection.GetCenter().GetLat(),
                                                    projection.GetCenter().GetLon()+centerLonOffset},
                                                   extent));

                     RecordingPainter painter;

                     Render(painter,projection,parameter,data);

                     return painter.Areas().size();
                   };

  INFO("tolerance at 96 DPI: " << tolerancePx96);
  INFO("tolerance at 300 DPI: " << tolerancePx300);
  INFO("the area reaches the view by: " << reachPx);

  // The area lies outside the view, so its ring is prepared only if the converted border width of the
  // style sheet reaches it - a raw millimetre number used as pixels reaches neither
  REQUIRE(preparedAt(projection300)==1);
  REQUIRE(preparedAt(projection96)==0);
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

  AddOutsideAreas(data,
                  types.styledAreaType,
                  projection,
                  projection.GetCenter(),
                  extent,
                  outsideCount);

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
                     AddVisibleWay(data,types.wayType,center,extent);

                     AddOutsideAreas(data,
                                     types.styledAreaType,
                                     projection,
                                     center,
                                     extent,
                                     outsideCount);

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

    AddVisibleWay(data,types.wayType,center,extent);

    AddOutsideAreas(data,
                    types.styledAreaType,
                    projection,
                    center,
                    extent,
                    128);

    Render(painter,projection,parameter,data);

    REQUIRE(painter.Areas().empty());
    REQUIRE(painter.Ways().size()==1);
  }
}

// ---------------------------------------------------------------------------
// Requirement: A ring's visibility tolerance covers every border style the ring
// resolves
// ---------------------------------------------------------------------------

/**
 * A ring's border can be drawn at an offset from the ring, so the decision has to extend the ring by
 * that offset as well (spec map-painter-area-culling, requirement "A ring's visibility tolerance
 * covers every border style the ring resolves"). The offset is measured in map units and the width
 * in millimetres, so both terms are converted with the projection of the frame.
 */
TEST_CASE("A border drawn at an offset keeps its ring","[MapPainterAreaVisibilityCull]")
{
  auto types=MakeTypes();
  auto projection=MakeProjection();
  auto parameter=MakeParameter();

  // The width keeps the early decision of the unmodified pipeline wide enough to let the area reach
  // the per-ring decision, so what this case observes is that decision
  constexpr double borderWidthMM=40.0;

  const double     widthReachPx=projection.ConvertWidthToPixel(borderWidthMM*borderWidthToTolerance);

  // An offset that reaches half as far as half of the border width, expressed in the map units the
  // style sheet declares it in
  const double offsetMapUnits=0.5*widthReachPx*projection.GetPixelSize();

  auto         styleConfig=MakeStylesWithBorders(types,{{"line",borderWidthMM,offsetMapUnits,0.0}});

  const double offsetReachPx=offsetMapUnits/projection.GetPixelSize();

  INFO("reach of half of the border width: " << widthReachPx);
  INFO("reach of the offset: " << offsetReachPx);

  REQUIRE(offsetReachPx>10.0);

  const double     screenRight=projection.GetWidth();
  constexpr double halfSizePx=60.0;

  // The area lies outside the view by three quarters of the reach of half the border width. Its
  // border still reaches the view through the offset, so the ring has to be kept; the decision that
  // read the front style alone used a tolerance of zero for a style that carries an offset, so this
  // area was rejected before
  REQUIRE(PreparedEntriesForEdge(styleConfig,
                                 types,
                                 projection,
                                 parameter,
                                 screenRight+0.75*widthReachPx,
                                 halfSizePx)>0);

  // Beyond the reach of the drawn border nothing is prepared
  REQUIRE(PreparedEntriesForEdge(styleConfig,
                                 types,
                                 projection,
                                 parameter,
                                 screenRight+widthReachPx+offsetReachPx+10.0,
                                 halfSizePx)==0);
}

/**
 * The tolerance is the widest reach of the border styles the ring resolves, not the reach of the
 * style the decision reads first (spec map-painter-area-culling, requirement "A ring's visibility
 * tolerance covers every border style the ring resolves").
 */
TEST_CASE("The widest drawn border decides the tolerance","[MapPainterAreaVisibilityCull]")
{
  auto             types=MakeTypes();
  auto             projection=MakeProjection();
  auto             parameter=MakeParameter();

  constexpr double wideWidthMM=40.0;
  constexpr double narrowWidthMM=0.4;

  // The wide style carries a display offset, which the decision that reads the front style alone
  // treats as "not the border of the ring" and therefore reduces its tolerance to zero; the narrow
  // style has no offsets and gives it a tolerance of a fraction of a pixel. Whichever of the two the
  // decision reads first, it used a tolerance far smaller than the reach of the wide border
  auto styleConfig=MakeStylesWithBorders(types,
                                         {{"casing",wideWidthMM,0.0,0.6},
                                           {"line",narrowWidthMM,0.0,0.0}});

  const double wideReachPx=projection.ConvertWidthToPixel(wideWidthMM*borderWidthToTolerance);

  INFO("reach of half of the wide border width: " << wideReachPx);

  const double     screenRight=projection.GetWidth();
  constexpr double halfSizePx=60.0;

  REQUIRE(PreparedEntriesForEdge(styleConfig,
                                 types,
                                 projection,
                                 parameter,
                                 screenRight+0.5*wideReachPx,
                                 halfSizePx)>0);

  REQUIRE(PreparedEntriesForEdge(styleConfig,
                                 types,
                                 projection,
                                 parameter,
                                 screenRight+2.0*wideReachPx,
                                 halfSizePx)==0);
}

/**
 * A ring whose drawn borders are farther away than their own reach contributes nothing, so the wider
 * tolerance does not prepare every loaded area (spec map-painter-area-culling, requirement "A ring's
 * visibility tolerance covers every border style the ring resolves").
 */
TEST_CASE("A ring beyond the reach of its drawn borders contributes nothing",
          "[MapPainterAreaVisibilityCull]")
{
  auto             types=MakeTypes();
  auto             projection=MakeProjection();
  auto             parameter=MakeParameter();

  constexpr double borderWidthMM=10.0;

  const double     widthReachPx=projection.ConvertWidthToPixel(borderWidthMM*borderWidthToTolerance);
  const double     offsetMapUnits=1000.0;
  const double     offsetReachPx=offsetMapUnits/projection.GetPixelSize();

  auto             styleConfig=MakeStylesWithBorders(types,{{"line",borderWidthMM,offsetMapUnits,0.0}});

  const double     screenRight=projection.GetWidth();
  constexpr double halfSizePx=60.0;

  INFO("reach of the drawn border: " << widthReachPx+offsetReachPx);

  REQUIRE(offsetReachPx>widthReachPx);

  // The area lies outside the view by more than the offset and half of the width, so no border of it
  // can reach the view
  REQUIRE(PreparedEntriesForEdge(styleConfig,
                                 types,
                                 projection,
                                 parameter,
                                 screenRight+offsetReachPx+2.0*widthReachPx,
                                 halfSizePx)==0);
}

/**
 * A ring whose border styles declare one width and no offset keeps the tolerance it had: half of
 * that width, converted with the projection (spec map-painter-area-culling, requirement "A ring's
 * visibility tolerance covers every border style the ring resolves").
 */
TEST_CASE("A ring without an offset border keeps its tolerance","[MapPainterAreaVisibilityCull]")
{
  auto             types=MakeTypes();
  auto             projection=MakeProjection();
  auto             parameter=MakeParameter();

  constexpr double borderWidthMM=2.0;

  // The tolerance of the decision is half of the declared width, converted to the pixels of the frame
  const double tolerancePx=projection.ConvertWidthToPixel(borderWidthMM*borderWidthToTolerance);

  auto         styleConfig=MakeStylesWithBorders(types,{{"line",borderWidthMM,0.0,0.0}});

  REQUIRE(styleConfig->GetMaxAreaBorderWidthMM(projection.GetMagnification())==borderWidthMM);

  const double     screenRight=projection.GetWidth();
  constexpr double halfSizePx=60.0;

  INFO("tolerance of the ring: " << tolerancePx);

  REQUIRE(tolerancePx>1.0);

  // Within the tolerance the ring is kept, beyond it nothing is prepared
  REQUIRE(PreparedEntriesForEdge(styleConfig,
                                 types,
                                 projection,
                                 parameter,
                                 screenRight+0.9*tolerancePx,
                                 halfSizePx)>0);

  REQUIRE(PreparedEntriesForEdge(styleConfig,
                                 types,
                                 projection,
                                 parameter,
                                 screenRight+1.5*tolerancePx,
                                 halfSizePx)==0);
}

// ---------------------------------------------------------------------------
// Requirement: The early rejection is conservative (the offset reach)
// ---------------------------------------------------------------------------

/**
 * An area whose border reaches the view only through the offset it is drawn at has to arrive at
 * per-ring preparation: the frame-wide bound the early rejection uses has to cover the offset term
 * as well (spec map-painter-area-culling, requirement "The early rejection is conservative",
 * scenario "An area whose only reachable border is offset is not rejected early").
 */
TEST_CASE("An area whose only reachable border is offset is not rejected early",
          "[MapPainterAreaVisibilityCull]")
{
  auto             types=MakeTypes();
  auto             projection=MakeProjection();
  auto             parameter=MakeParameter();

  constexpr double borderWidthMM=40.0;

  const double     widthReachPx=projection.ConvertWidthToPixel(borderWidthMM*borderWidthToTolerance);
  const double     offsetMapUnits=0.5*widthReachPx*projection.GetPixelSize();
  const double     offsetReachPx=offsetMapUnits/projection.GetPixelSize();

  auto             styleConfig=MakeStylesWithBorders(types,{{"line",borderWidthMM,offsetMapUnits,0.0}});

  const double     screenRight=projection.GetWidth();
  constexpr double halfSizePx=60.0;

  INFO("reach of half of the border width: " << widthReachPx);
  INFO("reach of the offset: " << offsetReachPx);

  // The bound of this style sheet's width alone, i.e. the tolerance of the early rejection that read
  // no offset, is not enough to reach this area
  const double distancePx=1.25*widthReachPx;

  REQUIRE(distancePx>widthReachPx);
  REQUIRE(distancePx<widthReachPx+offsetReachPx);

  REQUIRE(PreparedEntriesForEdge(styleConfig,
                                 types,
                                 projection,
                                 parameter,
                                 screenRight+distancePx,
                                 halfSizePx)>0);

  // Beyond the width and the offset of the drawn border nothing is prepared
  REQUIRE(PreparedEntriesForEdge(styleConfig,
                                 types,
                                 projection,
                                 parameter,
                                 screenRight+widthReachPx+offsetReachPx+10.0,
                                 halfSizePx)==0);
}

/**
 * The frame-wide area reach is converted with the projection of the frame, so a style sheet whose
 * borders reach through a display offset gets the larger tolerance at the higher DPI; a style sheet
 * without any area border style has no area reach at all (spec map-painter-area-culling, requirement
 * "The early rejection is conservative").
 */
TEST_CASE("The area reach of the frame follows the DPI and the offsets of the stylesheet",
          "[MapPainterAreaVisibilityCull]")
{
  auto             types=MakeTypes();

  constexpr double borderWidthMM=0.1;
  constexpr double displayOffsetMM=4.0;

  auto             styleConfig=MakeStylesWithBorders(types,{{"line",borderWidthMM,0.0,displayOffsetMM}});

  auto             projection96=MakeProjectionWithDpi(lowerDpi);
  auto             projection300=MakeProjectionWithDpi(referenceDpi);
  auto             parameter=MakeParameter();

  const double     tolerancePx96=projection96.ConvertWidthToPixel((borderWidthMM*borderWidthToTolerance)+
                                                                  displayOffsetMM);
  const double     tolerancePx300=projection300.ConvertWidthToPixel((borderWidthMM*borderWidthToTolerance)+
                                                                    displayOffsetMM);

  INFO("area reach at 96 DPI: " << tolerancePx96);
  INFO("area reach at 300 DPI: " << tolerancePx300);

  REQUIRE(tolerancePx300>tolerancePx96);

  constexpr double halfSizePx=60.0;

  // The area reaches into the view by just over the reach of the 96 DPI projection and well within
  // the reach of the 300 DPI projection, so only the latter may keep it
  const double reachPx=tolerancePx96+1.0;

  REQUIRE(reachPx<tolerancePx300);

  auto preparedAt=[&](const osmscout::MercatorProjection& projection) {
                     return PreparedEntriesForEdge(styleConfig,
                                                   types,
                                                   projection,
                                                   parameter,
                                                   projection.GetWidth()+reachPx,
                                                   halfSizePx);
                   };

  REQUIRE(preparedAt(projection300)>0);
  REQUIRE(preparedAt(projection96)==0);

  // A style sheet without any area border style has no area reach, so an area outside the view is
  // not prepared at all
  auto                  noBorderStyles=std::make_shared<osmscout::StyleConfig>(types.typeConfig);

  osmscout::TypeInfoSet areaTypes(*types.typeConfig);

  areaTypes.Set(types.styledAreaType);

  osmscout::StyleFilter areaFilter;

  areaFilter.SetTypes(areaTypes);

  osmscout::FillPartialStyle fillStyle;

  fillStyle.SetColorValue(osmscout::FillStyle::attrFillColor,osmscout::Color(0.0,1.0,0.0));
  noBorderStyles->AddAreaFillStyle(areaFilter,fillStyle);
  noBorderStyles->Postprocess();

  REQUIRE(noBorderStyles->GetVisibilityBounds(projection300.GetMagnification()).maxAreaBorderWidth==0.0);
  REQUIRE(noBorderStyles->GetVisibilityBounds(projection300.GetMagnification()).maxAreaBorderOffset==0.0);
  REQUIRE(noBorderStyles->GetVisibilityBounds(projection300.GetMagnification()).maxAreaBorderDisplayOffset==0.0);

  REQUIRE(PreparedEntriesForEdge(noBorderStyles,
                                 types,
                                 projection300,
                                 parameter,
                                 projection300.GetWidth()+1.0,
                                 halfSizePx)==0);
}
