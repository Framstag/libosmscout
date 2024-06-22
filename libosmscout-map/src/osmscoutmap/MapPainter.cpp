/*
  This source is part of the libosmscout-map library
  Copyright (C) 2009  Tim Teulings

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

#include <osmscoutmap/MapPainter.h>

#include <algorithm>
#include <limits>
#include <sstream>

#include <osmscout/system/Math.h>

#include <osmscout/log/Logger.h>
#include <osmscout/util/StopClock.h>
#include <osmscout/util/String.h>
#include <osmscout/util/Tiling.h>
#include <osmscout/util/Geometry.h>

#include <osmscoutmap/MapPainterStatistics.h>

namespace osmscout {

#ifdef OSMSCOUT_DEBUG_GROUNDTILES
constexpr bool debugGroundTiles = true;
#else
constexpr bool debugGroundTiles = false;
#endif

  static std::set<GeoCoord> GetGridPoints(const std::vector<Point>& nodes,
                                          double gridSizeHoriz,
                                          double gridSizeVert)
  {
    assert(nodes.size()>=2);

    std::set<GeoCoord> intersections;

    for (size_t i=0; i<nodes.size()-1; ++i) {
      size_t cellXStart=(size_t)((nodes[i].GetLon()+180.0)/gridSizeHoriz);
      size_t cellYStart=(size_t)((nodes[i].GetLat()+90.0)/gridSizeVert);

      size_t cellXEnd=(size_t)((nodes[i+1].GetLon()+180.0)/gridSizeHoriz);
      size_t cellYEnd=(size_t)((nodes[i+1].GetLat()+90.0)/gridSizeVert);

      if (cellXStart!=cellXEnd) {
        double lower=std::min(cellYStart,cellYEnd)*gridSizeVert-90.0;
        double upper=(std::max(cellYStart,cellYEnd)+1)*gridSizeVert-90.0;

        for (size_t xIndex=cellXStart+1; xIndex<=cellXEnd; ++xIndex) {
          GeoCoord intersection;

          double xCoord=xIndex*gridSizeHoriz-180.0;

          if (GetLineIntersection(nodes[i].GetCoord(),
                                  nodes[i+1].GetCoord(),
                                  GeoCoord(lower,xCoord),
                                  GeoCoord(upper,xCoord),
                                  intersection)) {
            intersections.insert(intersection);
          }
        }
      }

      if (cellYStart!=cellYEnd) {
        double lower=std::min(cellXStart,cellXEnd)*gridSizeHoriz-180.0;
        double upper=(std::max(cellXStart,cellXEnd)+1)*gridSizeHoriz-180.0;

        for (size_t yIndex=cellYStart+1; yIndex<=cellYEnd; ++yIndex) {
          GeoCoord intersection;

          double yCoord=yIndex*gridSizeVert-90.0;

          if (GetLineIntersection(nodes[i].GetCoord(),
                                  nodes[i+1].GetCoord(),
                                  GeoCoord(yCoord,lower),
                                  GeoCoord(yCoord,upper),
                                  intersection)) {
            intersections.insert(intersection);
          }
        }
      }
    }

    return intersections;
  }

  /**
   * Return if a > b, a should be drawn before b
   */
  [[nodiscard]] static bool AreaSorter(const MapPainter::AreaData& a, const MapPainter::AreaData& b)
  {
    if (a.fillStyle && b.fillStyle) {
      if (a.fillStyle->GetFillColor().IsSolid() && !b.fillStyle->GetFillColor().IsSolid()) {
        return true;
      }

      if (!a.fillStyle->GetFillColor().IsSolid() && b.fillStyle->GetFillColor().IsSolid()) {
        return false;
      }
    }
    else if (a.fillStyle) {
      return true;
    }
    else if (b.fillStyle) {
      return false;
    }

    if (a.boundingBox.GetMinCoord().GetLon()==b.boundingBox.GetMinCoord().GetLon()) {
      if (a.boundingBox.GetMaxCoord().GetLon()==b.boundingBox.GetMaxCoord().GetLon()) {
        if (a.boundingBox.GetMinCoord().GetLat()==b.boundingBox.GetMinCoord().GetLat()) {
          if (a.boundingBox.GetMaxCoord().GetLat()==b.boundingBox.GetMaxCoord().GetLat()){
            /**
             * Condition for the case when one area exists in two relations
             *  - in one as outer ring (type of relation is used) and in second relation as inner ring.
             * In such case, we want to draw area with outer type after that one of inner type
             */
            return !a.isOuter && b.isOuter;
          }

          return a.boundingBox.GetMaxCoord().GetLat() > b.boundingBox.GetMaxCoord().GetLat();
        }

        return a.boundingBox.GetMinCoord().GetLat()<b.boundingBox.GetMinCoord().GetLat();
      }

      return a.boundingBox.GetMaxCoord().GetLon()>b.boundingBox.GetMaxCoord().GetLon();
    }

    return a.boundingBox.GetMinCoord().GetLon()<b.boundingBox.GetMinCoord().GetLon();
  }

  /**
   * Sort labels for the same object by position
   */
  [[nodiscard]] static inline bool LabelLayoutDataSorter(const LabelData& a,
                                                         const LabelData& b)
  {
    return a.position<b.position;
  }

  MapPainter::MapPainter(const StyleConfigRef& styleConfig)
  : styleConfig(styleConfig),
    nameReader(*styleConfig->GetTypeConfig()),
    nameAltReader(*styleConfig->GetTypeConfig()),
    refReader(*styleConfig->GetTypeConfig()),
    layerReader(*styleConfig->GetTypeConfig()),
    widthReader(*styleConfig->GetTypeConfig()),
    addressReader(*styleConfig->GetTypeConfig()),
    lanesReader(*styleConfig->GetTypeConfig()),
    accessReader(*styleConfig->GetTypeConfig()),
    colorReader(*styleConfig->GetTypeConfig())
  {
    log.Debug() << "MapPainter::MapPainter()";

    tunnelDash.push_back(0.4);
    tunnelDash.push_back(0.4);

    areaMarkStyle.SetFillColor(Color(1.0,0,0.0,0.5));

    landFill=std::make_shared<FillStyle>();
    landFill->SetFillColor(Color(241.0/255,238.0/255,233.0/255));

    seaFill=std::make_shared<FillStyle>();
    seaFill->SetFillColor(Color(181.0/255,208.0/255,208.0/255));

    auto textStyle=std::make_shared<TextStyle>();

    textStyle->SetStyle(TextStyle::normal);
    textStyle->SetPriority(0);
    textStyle->SetTextColor(Color(0,0,0,0.9));
    textStyle->SetSize(0.7);

    debugLabel=std::move(textStyle);

    stepMethods.resize(RenderSteps::LastStep-RenderSteps::FirstStep+1);

    stepMethods[RenderSteps::Initialize]=&MapPainter::InitializeRender;
    stepMethods[RenderSteps::DumpStatistics]=&MapPainter::DumpStatistics;
    stepMethods[RenderSteps::CalculatePaths]=&MapPainter::CalculatePaths;
    stepMethods[RenderSteps::CalculateWayShields]=&MapPainter::CalculateWayShields;
    stepMethods[RenderSteps::ProcessAreas]=&MapPainter::ProcessAreas;
    stepMethods[RenderSteps::ProcessRoutes]=&MapPainter::ProcessRoutes;
    stepMethods[RenderSteps::AfterPreprocessing]=&MapPainter::AfterPreprocessing;
    stepMethods[RenderSteps::Prerender]=&MapPainter::Prerender;
    stepMethods[RenderSteps::DrawBaseMapTiles]=&MapPainter::DrawBaseMapTiles;
    stepMethods[RenderSteps::DrawGroundTiles]=&MapPainter::DrawGroundTiles;
    stepMethods[RenderSteps::DrawOSMTileGrids]=&MapPainter::DrawOSMTileGrids;
    stepMethods[RenderSteps::DrawAreas]=&MapPainter::DrawAreas;
    stepMethods[RenderSteps::DrawWays]=&MapPainter::DrawWays;
    stepMethods[RenderSteps::DrawWayDecorations]=&MapPainter::DrawWayDecorations;
    stepMethods[RenderSteps::DrawWayContourLabels]=&MapPainter::DrawWayContourLabels;
    stepMethods[RenderSteps::PrepareAreaLabels]=&MapPainter::PrepareAreaLabels;
    stepMethods[RenderSteps::DrawAreaBorderLabels]=&MapPainter::DrawAreaBorderLabels;
    stepMethods[RenderSteps::DrawAreaBorderSymbols]=&MapPainter::DrawAreaBorderSymbols;
    stepMethods[RenderSteps::PrepareNodeLabels]=&MapPainter::PrepareNodeLabels;
    stepMethods[RenderSteps::PrepareRouteLabels]=&MapPainter::PrepareRouteLabels;
    stepMethods[RenderSteps::DrawLabels]=&MapPainter::DrawLabels;
    stepMethods[RenderSteps::DrawContourLines]=&MapPainter::DrawContourLines;
    stepMethods[RenderSteps::DrawHillShading]=&MapPainter::DrawHillShading;
    stepMethods[RenderSteps::Postrender]=&MapPainter::Postrender;
  }

  MapPainter::~MapPainter()
  {
    log.Debug() << "MapPainter::~MapPainter()";
  }

  bool MapPainter::IsVisibleArea(const Projection& projection,
                                 const GeoBox& boundingBox,
                                 double pixelOffset) const
  {
    if (!boundingBox.IsValid()){
      return false;
    }

    ScreenBox areaScreenBox;

    if (!projection.BoundingBoxToPixel(boundingBox,
                                       areaScreenBox)) {
      return false;
    }

    areaScreenBox=areaScreenBox.Resize(pixelOffset);

    if (areaScreenBox.GetWidth()<=areaMinDimension &&
        areaScreenBox.GetHeight()<=areaMinDimension) {
      return false;
    }

    return areaScreenBox.Intersects(projection.GetScreenBox());
  }

  bool MapPainter::IsVisibleWay(const Projection& projection,
                                const GeoBox& boundingBox,
                                double pixelOffset) const
  {
    ScreenBox wayScreenBox;

    if (!projection.BoundingBoxToPixel(boundingBox,
                                       wayScreenBox)) {
      return false;
    }

    wayScreenBox=wayScreenBox.Resize(pixelOffset);

    return wayScreenBox.Intersects(projection.GetScreenBox());
  }

  double MapPainter::GetProjectedWidth(const Projection& projection,
                                       double minPixel,
                                       double width) const
  {
    width=width/projection.GetPixelSize();

    if (width<minPixel) {
      return minPixel;
    }

    return width;
  }

  void MapPainter::AfterPreprocessing(const StyleConfig& /*styleConfig*/,
                                      const Projection& /*projection*/,
                                      const MapParameter& /*parameter*/,
                                      const MapData& /*data*/)
  {
    // Nothing to do in the base class implementation
  }

  void MapPainter::BeforeDrawing(const StyleConfig& /*styleConfig*/,
                                 const Projection& /*projection*/,
                                 const MapParameter& /*parameter*/,
                                 const MapData& /*data*/)
  {
    // Nothing to do in the base class implementation
  }

  void MapPainter::AfterDrawing(const StyleConfig& /*styleConfig*/,
                                const Projection& /*projection*/,
                                const MapParameter& /*parameter*/,
                                const MapData& /*data*/)
  {
    // Nothing to do in the base class implementation
  }

  void MapPainter::RegisterPointWayLabel(const Projection& projection,
                                         const MapParameter& parameter,
                                         const PathShieldStyleRef& style,
                                         const std::string_view& text,
                                         const std::vector<Point>& nodes)
  {
    LabelStyleRef      labelStyle=style->GetShieldStyle();
    std::set<GeoCoord> gridPoints=GetGridPoints(nodes,
                                                shieldGridSizeHoriz,
                                                shieldGridSizeVert);

    if (gridPoints.empty()) {
      return;
    }

    LabelData labelBox;

    labelBox.priority=labelStyle->GetPriority();
    labelBox.alpha=1.0;
    labelBox.fontSize=labelStyle->GetSize();
    labelBox.style=labelStyle;
    labelBox.text=text;

    std::vector<LabelData> labelData= {labelBox};

    for (const auto& gridPoint : gridPoints) {
      Vertex2D pixel;

      projection.GeoToPixel(gridPoint,
                            pixel);

      RegisterRegularLabel(projection,
                           parameter,
                           ObjectFileRef(),
                           labelData,
                           pixel,
                           /*proposedWidth*/ -1);
    }
  }

  /**
   * Request layout of a point label
   * @param projection
   *    Projection instance to use
   * @param parameter
   *    General map drawing parameter that might influence the result
   * @param buffer
   *    The FeatureValueBuffer of the object that owns the label
   * @param iconStyle
   *    An optional icon style to use
   * @param textStyles
   *    A list of text styles to use (the object could have more than
   *    label styles attached)
   * @param screenPos
   *    position to place the label at (currently always the center of the area or the coordinate of the node)
   * @param objectBox
   *    The (rough) size of the object
   */
  void MapPainter::LayoutPointLabels(const Projection& projection,
                                     const MapParameter& parameter,
                                     const ObjectFileRef& ref,
                                     const FeatureValueBuffer& buffer,
                                     const IconStyleRef& iconStyle,
                                     const std::vector<TextStyleRef>& textStyles,
                                     const Vertex2D& screenPos,
                                     const ScreenBox& objectBox)
  {
    std::vector<LabelData> labelLayoutData;

    if (iconStyle) {
      if (!iconStyle->GetIconName().empty() &&
          HasIcon(*styleConfig,
                  projection,
                  parameter,
                  *iconStyle)) {
        LabelData data;

        data.type=LabelData::Type::Icon;
        data.position=iconStyle->GetPosition();
        data.priority=iconStyle->GetPriority();
        data.iconStyle=iconStyle;
        data.iconWidth=iconStyle->GetWidth();
        data.iconHeight=iconStyle->GetHeight();

        labelLayoutData.push_back(data);
      }
      else if (iconStyle->GetSymbol()) {
        LabelData data;

        data.type=LabelData::Type::Symbol;
        data.position=iconStyle->GetPosition();
        data.iconStyle=iconStyle;
        data.priority=iconStyle->GetPriority();

        data.iconWidth=iconStyle->GetSymbol()->GetWidth(projection);
        data.iconHeight=iconStyle->GetSymbol()->GetHeight(projection);

        labelLayoutData.push_back(data);
      }
    }

    for (const auto& textStyle : textStyles) {
      std::string label=textStyle->GetLabel()->GetLabel(parameter,
                                                        buffer);

      if (label.empty()) {
        continue;
      }

      LabelData data;

      data.type=LabelData::Type::Text;
      data.priority=textStyle->GetPriority();

      if (projection.GetMagnification()>textStyle->GetScaleAndFadeMag() &&
          parameter.GetDrawFadings()) {
        double factor=projection.GetMagnification().GetLevel()-textStyle->GetScaleAndFadeMag().GetLevel();

        data.fontSize=textStyle->GetSize()*::pow(1.5,factor);
        data.alpha=std::min(textStyle->GetAlpha()/factor, 1.0);
      }
      else if (textStyle->GetAutoSize()) {
        double height=std::abs(objectBox.GetHeight()*0.1);

        if (height==0.0 || height<standardFontSize) {
          continue;
        }

        // Restricts the height of a label to maxHeight
        double alpha=textStyle->GetAlpha();
        double maxHeight=projection.GetHeight()/5.0;

        if (height>maxHeight) {
            // If the height exceeds maxHeight the alpha value will be decreased
            double minAlpha=(double)projection.GetHeight();
            double normHeight=(height-maxHeight)/(minAlpha-maxHeight);
            alpha*=std::min(std::max(1-normHeight,0.2),1.0);
            height=maxHeight;
        }

        data.fontSize=height/standardFontSize;
        data.alpha=alpha;
      }
      else {
        data.fontSize=textStyle->GetSize();
        data.alpha=textStyle->GetAlpha();
      }

      data.position=textStyle->GetPosition();
      data.text=label;
      data.style=textStyle;

      labelLayoutData.push_back(data);
    }

    if (labelLayoutData.empty()) {
      return;
    }

    std::stable_sort(labelLayoutData.begin(),
                     labelLayoutData.end(),
                     LabelLayoutDataSorter);

    RegisterRegularLabel(projection,
                         parameter,
                         ref,
                         labelLayoutData,
                         screenPos,
                         objectBox.GetWidth());
  }

  double MapPainter::GetProposedLabelWidth(const MapParameter& parameter,
                                           double averageCharWidth,
                                           double objectWidth,
                                           size_t stringLength)
  {
    double proposedWidth;
    // If there is just a few characters (less than LabelLineMinCharCount)
    // we should not wrap the words at all.
    if (stringLength>parameter.GetLabelLineMinCharCount()) {
      if (objectWidth>0 && parameter.GetLabelLineFitToArea()) {
        proposedWidth=std::min(objectWidth,parameter.GetLabelLineFitToWidth());
      }
      else {
        proposedWidth=parameter.GetLabelLineFitToWidth();
      }

      proposedWidth=std::min(proposedWidth,
                             (double)parameter.GetLabelLineMaxCharCount()*averageCharWidth);
      proposedWidth=std::max(proposedWidth,
                             (double)parameter.GetLabelLineMinCharCount()*averageCharWidth);
    }
    else {
      proposedWidth=parameter.GetLabelLineMaxCharCount()*averageCharWidth;
    }

    return proposedWidth;
  }

  void MapPainter::PrepareNodes(const StyleConfig& styleConfig,
                                const Projection& projection,
                                const MapParameter& parameter,
                                const MapData& data)
  {
    for (const auto& node : data.nodes) {
      PrepareNode(styleConfig,
                  projection,
                  parameter,
                  node);
    }

    for (const auto& node : data.poiNodes) {
      PrepareNode(styleConfig,
                  projection,
                  parameter,
                  node);
    }
  }

  void MapPainter::PrepareAreaLabel(const StyleConfig& styleConfig,
                                    const Projection& projection,
                                    const MapParameter& parameter,
                                    const AreaData& areaData)
  {
    IconStyleRef iconStyle=styleConfig.GetAreaIconStyle(areaData.type,
                                                        *areaData.buffer,
                                                        projection);

    styleConfig.GetAreaTextStyles(areaData.type,
                                  *areaData.buffer,
                                  projection,
                                  textStyles);

    if (!iconStyle && textStyles.empty()) {
      return;
    }

    ScreenBox areaScreenBox;

    projection.BoundingBoxToPixel(areaData.boundingBox,
                                  areaScreenBox);

    Vertex2D areaCenter;

    if (areaData.center.has_value()){
      projection.GeoToPixel(areaData.center.value(),
                            areaCenter);
    }
    else {
      areaCenter=areaScreenBox.GetCenter();
    }

    LayoutPointLabels(projection,
                      parameter,
                      areaData.ref,
                      *areaData.buffer,
                      iconStyle,
                      textStyles,
                      areaCenter,
                      areaScreenBox);
  }

  bool MapPainter::DrawAreaBorderLabel(const StyleConfig& styleConfig,
                                       const Projection& projection,
                                       const MapParameter& parameter,
                                       const AreaData& areaData)
  {
    PathTextStyleRef borderTextStyle=styleConfig.GetAreaBorderTextStyle(areaData.type,
                                                                        *areaData.buffer,
                                                                        projection);

    if (!borderTextStyle) {
      return false;
    }

    double           lineOffset=0.0;
    std::string      label=borderTextStyle->GetLabel()->GetLabel(parameter,
                                                            *areaData.buffer);
    CoordBufferRange range=areaData.coordRange;

    if (label.empty()) {
      return false;
    }

    if (borderTextStyle->GetOffset()!=0.0) {
      lineOffset+=GetProjectedWidth(projection,
                                    borderTextStyle->GetOffset());
    }

    if (borderTextStyle->GetDisplayOffset()!=0.0) {
      lineOffset+=projection.ConvertWidthToPixel(borderTextStyle->GetDisplayOffset());
    }

    if (lineOffset!=0.0) {
      range=coordBuffer.GenerateParallelWay(range,
                                            lineOffset);
    }

    // TODO: use coordBuffer for label path
    LabelPath labelPath;

    for (size_t j=range.GetStart(); j<=range.GetEnd(); ++j) {
      labelPath.AddPoint(
          range.Get(j));
    }

    PathLabelData labelData;
    labelData.priority=borderTextStyle->GetPriority();
    labelData.style=std::move(borderTextStyle);
    labelData.text=label;
    labelData.contourLabelOffset=contourLabelOffset;
    labelData.contourLabelSpace=contourLabelSpace;

    RegisterContourLabel(projection,
                         parameter,
                         areaData.ref,
                         labelData,
                         labelPath);

    return true;
  }

  bool MapPainter::DrawAreaBorderSymbol(const StyleConfig& styleConfig,
                                        const Projection& projection,
                                        const MapParameter& parameter,
                                        const AreaData& areaData)
  {
    PathSymbolStyleRef borderSymbolStyle=styleConfig.GetAreaBorderSymbolStyle(areaData.type,
                                                                              *areaData.buffer,
                                                                              projection);

    if (!borderSymbolStyle) {
      return false;
    }

    double           lineOffset=0.0;
    double           symbolSpace=projection.ConvertWidthToPixel(borderSymbolStyle->GetSymbolSpace());
    CoordBufferRange range=areaData.coordRange;

    if (borderSymbolStyle->HasOffset()) {
      lineOffset+=GetProjectedWidth(projection,
                                    borderSymbolStyle->GetOffset());
    }

    if (borderSymbolStyle->HasDisplayOffset()) {
      lineOffset+=projection.ConvertWidthToPixel(borderSymbolStyle->GetDisplayOffset());
    }

    if (lineOffset!=0.0) {
      range=coordBuffer.GenerateParallelWay(range,
                                             lineOffset);
    }

    ContourSymbolData data;

    data.coordRange=range;
    data.symbolOffset=0.0;
    data.symbolSpace=symbolSpace;

    DrawContourSymbol(projection,
                      parameter,
                      *borderSymbolStyle->GetSymbol(),
                      data);

    return true;
  }

  void MapPainter::PrepareNode(const StyleConfig& styleConfig,
                               const Projection& projection,
                               const MapParameter& parameter,
                               const NodeRef& node)
  {
    IconStyleRef iconStyle=styleConfig.GetNodeIconStyle(node->GetFeatureValueBuffer(),
                                                        projection);

    styleConfig.GetNodeTextStyles(node->GetFeatureValueBuffer(),
                                 projection,
                                 textStyles);

    Vertex2D screenPos;

    projection.GeoToPixel(node->GetCoords(),
                          screenPos);

    LayoutPointLabels(projection,
                      parameter,
                      node->GetObjectFileRef(),
                      node->GetFeatureValueBuffer(),
                      iconStyle,
                      textStyles,
                      screenPos,
                      ScreenBox::EMPTY);
  }

  void MapPainter::DrawWay(const StyleConfig& /*styleConfig*/,
                           const Projection& projection,
                           const MapParameter& parameter,
                           const WayData& data)
  {
    if (data.lineStyle->HasDashes() &&
        data.lineStyle->GetGapColor().IsVisible()) {
      // Draw the background of a dashed line
      DrawPath(projection,
               parameter,
               data.lineStyle->GetGapColor(),
               data.lineWidth,
               emptyDash,
               data.startIsClosed ? data.lineStyle->GetEndCap() : data.lineStyle->GetJoinCap(),
               data.endIsClosed ? data.lineStyle->GetEndCap() : data.lineStyle->GetJoinCap(),
               data.coordRange);
    }

    DrawPath(projection,
             parameter,
             data.color,
             data.lineWidth,
             data.lineStyle->GetDash(),
             data.startIsClosed ? data.lineStyle->GetEndCap() : data.lineStyle->GetJoinCap(),
             data.endIsClosed ? data.lineStyle->GetEndCap() : data.lineStyle->GetJoinCap(),
             data.coordRange);
  }

  bool MapPainter::DrawWayDecoration(const StyleConfig& styleConfig,
                                     const Projection& projection,
                                     const MapParameter& parameter,
                                     const MapPainter::WayPathData& data)
  {
    styleConfig.GetWayPathSymbolStyle(*data.buffer,
                                      projection,
                                      symbolStyles);

    if (symbolStyles.empty()) {
      // no symbols to draw
      return false;
    }
    if (data.mainSlotWidth==0) {
      // line to decorate has no width
      return false;
    }

    const LanesFeatureValue *lanesValue=nullptr;
    std::vector<OffsetRel>  laneTurns; // cached turns

    for (const auto& pathSymbolStyle : symbolStyles) {
      CoordBufferRange range=data.coordRange;
      double symbolSpace = projection.ConvertWidthToPixel(pathSymbolStyle->GetSymbolSpace());

      if (IsLaneOffset(pathSymbolStyle->GetOffsetRel())) {
        if (lanesValue==nullptr) {
          lanesValue=lanesReader.GetValue(*data.buffer);
        }

        if (lanesValue==nullptr || lanesValue->GetLanes()<1){
          continue;
        }

        if (laneTurns.empty()) {
          laneTurns=ParseLaneTurns(*lanesValue);
          assert(laneTurns.size() <= lanesValue->GetLanes());
        }

        int lanes=lanesValue->GetLanes();
        double lanesSpace=data.mainSlotWidth/lanes;
        double laneOffset=-data.mainSlotWidth/2.0+lanesSpace/2.0;

        for (const OffsetRel &laneTurn: laneTurns) {
          if (pathSymbolStyle->GetOffsetRel() == laneTurn) {
            range=coordBuffer.GenerateParallelWay(range,
                                                  laneOffset);

            ContourSymbolData data;

            data.symbolSpace=symbolSpace;
            data.symbolOffset=symbolSpace/2.0;
            data.symbolScale=1.0;
            data.coordRange=range;


            DrawContourSymbol(projection,
                              parameter,
                              *pathSymbolStyle->GetSymbol(),
                              data);
          }
          laneOffset+=lanesSpace;
        }
      }
      else {
        double lineOffset = 0.0;

        if (pathSymbolStyle->HasOffset()) {
          lineOffset += GetProjectedWidth(projection,
                                          pathSymbolStyle->GetOffset());
        }

        if (pathSymbolStyle->HasDisplayOffset()) {
          lineOffset += projection.ConvertWidthToPixel(pathSymbolStyle->GetDisplayOffset());
        }

        if (lineOffset != 0.0) {
          range=coordBuffer.GenerateParallelWay(range,
                                                lineOffset);
        }

        SymbolRef symbol=pathSymbolStyle->GetSymbol();

        ScreenBox symbolBoundingBox=symbol->GetBoundingBox(projection);

        double symbolScale;

        if (pathSymbolStyle->GetRenderMode()==PathSymbolStyle::RenderMode::scale) {
          symbolScale=data.mainSlotWidth*pathSymbolStyle->GetScale()/symbolBoundingBox.GetHeight();
          symbolSpace*=symbolScale;
        }
        else {
          symbolScale=1.0;
        }

        double symbolWidth=symbolBoundingBox.GetWidth()*symbolScale;
        double length=data.coordRange.GetLength();
        if (length < (symbolWidth+symbolSpace)) {
          continue; // too short way even for single symbol and required space
        }
        assert((symbolWidth+symbolSpace)>0);
        size_t countSymbols=std::max(size_t(1), size_t((length - symbolSpace) / (symbolWidth + symbolSpace)));
        size_t labelCountExp=log2(countSymbols);

        countSymbols=::pow(2, labelCountExp);

        double space = (length-countSymbols*symbolWidth) / (countSymbols+1);
        assert(space>0);

        ContourSymbolData symbolData;

        symbolData.symbolSpace = space;
        symbolData.symbolOffset = space;
        symbolData.coordRange = range;
        symbolData.symbolScale = symbolScale;

        DrawContourSymbol(projection,
                          parameter,
                          *symbol,
                          symbolData);
      }
    }
    return true;
  }

  bool MapPainter::CalculateWayShieldLabels(const StyleConfig& styleConfig,
                                            const Projection& projection,
                                            const MapParameter& parameter,
                                            const Way& way)
  {
    PathShieldStyleRef shieldStyle=styleConfig.GetWayPathShieldStyle(way.GetFeatureValueBuffer(),
                                                                     projection);

    if (!shieldStyle) {
      return false;
    }

    std::string shieldLabel=shieldStyle->GetLabel()->GetLabel(parameter,
                                                              way.GetFeatureValueBuffer());

    if (shieldLabel.empty()) {
      return false;
    }

    RegisterPointWayLabel(projection,
                          parameter,
                          shieldStyle,
                          shieldLabel,
                          way.nodes);

    return true;
  }

  bool MapPainter::DrawWayContourLabel(const StyleConfig& styleConfig,
                                       const Projection& projection,
                                       const MapParameter& parameter,
                                       const WayPathData& data)
  {
    PathTextStyleRef pathTextStyle = styleConfig.GetWayPathTextStyle(*data.buffer,
                                                                     projection);

    if (!pathTextStyle) {
      return false;
    }

    std::string textLabel = pathTextStyle->GetLabel()->GetLabel(parameter,
                                                                *data.buffer);

    if (textLabel.empty()) {
      return false;
    }

    return DrawWayContourLabel(projection,
                               parameter,
                               data,
                               pathTextStyle,
                               textLabel);
  }

  bool MapPainter::DrawWayContourLabel(const Projection& projection,
                                       const MapParameter& parameter,
                                       const WayPathData& data,
                                       const PathTextStyleRef &pathTextStyle,
                                       const std::string_view &textLabel)
  {
    assert(pathTextStyle);

    double           lineOffset=0.0;
    CoordBufferRange range=data.coordRange;

    if (pathTextStyle->GetOffset()!=0.0) {
      lineOffset+=GetProjectedWidth(projection,
                                    pathTextStyle->GetOffset());
    }

    if (pathTextStyle->GetDisplayOffset()!=0.0) {
      lineOffset+=projection.ConvertWidthToPixel(pathTextStyle->GetDisplayOffset());
    }

    if (lineOffset!=0.0) {
      range=coordBuffer.GenerateParallelWay(range,
                                            lineOffset);
    }

    PathLabelData labelData;

    labelData.priority=pathTextStyle->GetPriority();
    labelData.style=pathTextStyle;
    labelData.text=textLabel;
    labelData.height=pathTextStyle->GetSize();
    labelData.contourLabelOffset=contourLabelOffset;
    labelData.contourLabelSpace=contourLabelSpace;

    // TODO: use coordBuffer for label path
    LabelPath labelPath;

    for (size_t j=range.GetStart(); j<=range.GetEnd(); ++j) {
      labelPath.AddPoint(range.Get(j));
    }

    RegisterContourLabel(projection,
                         parameter,
                         ObjectFileRef(data.ref,RefType::refWay),
                         labelData,
                         labelPath);

    return true;
  }

  void MapPainter::DrawOSMTileGrid(const Projection& projection,
                                   const MapParameter& parameter,
                                   const Magnification& magnification,
                                   const LineStyleRef& osmTileLine)
  {
    GeoBox                  boundingBox(projection.GetDimensions());
    osmscout::OSMTileId     tileA(OSMTileId::GetOSMTile(magnification,
                                                        boundingBox.GetMinCoord()));
    osmscout::OSMTileId     tileB(OSMTileId::GetOSMTile(magnification,
                                                        boundingBox.GetMaxCoord()));
    uint32_t                startTileX=std::min(tileA.GetX(),tileB.GetX());
    uint32_t                endTileX=std::max(tileA.GetX(),tileB.GetX());
    uint32_t                startTileY=std::min(tileA.GetY(),tileB.GetY());
    uint32_t                endTileY=std::max(tileA.GetY(),tileB.GetY());

    if (startTileX>0) {
      --startTileX;
    }
    if (startTileY>0) {
      --startTileY;
    }

    std::vector<GeoCoord> coords;

    // Horizontal lines

    coords.resize(endTileX-startTileX+1);
    for (uint32_t y=startTileY; y<=endTileY; ++y) {
      for (uint32_t x=startTileX; x<=endTileX; ++x) {
        coords[x-startTileX]=OSMTileId(x,y).GetTopLeftCoord(magnification);
      }

      CoordBufferRange transRange=TransformWay(coords,
                                               transBuffer,
                                               coordBuffer,
                                               projection,
                                               parameter.GetOptimizeWayNodes(),
                                               errorTolerancePixel);

      WayData data;

      data.buffer=&coastlineSegmentAttributes;
      data.layer=0;
      data.lineStyle=osmTileLine;
      data.color=osmTileLine->GetLineColor();
      data.wayPriority=std::numeric_limits<size_t>::max();
      data.coordRange=transRange;
      data.lineWidth=GetProjectedWidth(projection,
                                       projection.ConvertWidthToPixel(osmTileLine->GetDisplayWidth()),
                                       osmTileLine->GetWidth());
      data.startIsClosed=false;
      data.endIsClosed=false;
      wayData.push_back(data);
    }

    // Vertical lines

    coords.resize(endTileY-startTileY+1);
    for (uint32_t x=startTileX; x<=endTileX; ++x) {
      for (uint32_t y=startTileY; y<=endTileY; ++y) {
        coords[y-startTileY]=OSMTileId(x,y).GetTopLeftCoord(magnification);
      }

      CoordBufferRange transRange=TransformWay(coords,
                                               transBuffer,
                                               coordBuffer,
                                               projection,
                                               parameter.GetOptimizeWayNodes(),
                                               errorTolerancePixel);

      WayData data;

      data.buffer=&coastlineSegmentAttributes;
      data.layer=0;
      data.lineStyle=osmTileLine;
      data.color=osmTileLine->GetLineColor();
      data.wayPriority=std::numeric_limits<size_t>::max();
      data.coordRange=transRange;
      data.lineWidth=GetProjectedWidth(projection,
                                       projection.ConvertWidthToPixel(osmTileLine->GetDisplayWidth()),
                                       osmTileLine->GetWidth());
      data.startIsClosed=false;
      data.endIsClosed=false;
      wayData.push_back(data);
    }
  }

  bool MapPainter::PrepareAreaRing(const StyleConfig& styleConfig,
                                   const Projection& projection,
                                   const MapParameter& parameter,
                                   const std::vector<CoordBufferRange>& coordRanges,
                                   const Area& area,
                                   const Area::Ring& ring,
                                   size_t i,
                                   const TypeInfoRef& type)
  {
    if (type->GetIgnore()) {
      // clipping inner ring, we will not render it, but still go deeper,
      // there may be nested outer rings
      return true;
    }

    if (!coordRanges[i].IsValid()) {
      return false; // ring was skipped or reduced to single point
    }

    FillStyleRef                fillStyle;
    std::vector<BorderStyleRef> borderStyles;
    BorderStyleRef              borderStyle;

    fillStyle=styleConfig.GetAreaFillStyle(type,
                                           ring.GetFeatureValueBuffer(),
                                           projection);

    FillStyleProcessorRef fillProcessor=parameter.GetFillStyleProcessor(ring.GetType()->GetIndex());

    if (fillProcessor) {
      fillStyle=fillProcessor->Process(ring.GetFeatureValueBuffer(),
                                       fillStyle);
    }

    styleConfig.GetAreaBorderStyles(type,
                                    ring.GetFeatureValueBuffer(),
                                    projection,
                                    borderStyles);

    if (!fillStyle && borderStyles.empty()) {
      // Nothing to draw
      return false;
    }

    size_t borderStyleIndex=0;

    // Get the borderStyle of the border itself
    if (!borderStyles.empty() &&
        borderStyles.front()->GetDisplayOffset()==0.0 &&
        borderStyles.front()->GetOffset()==0.0) {
      borderStyle=borderStyles[borderStyleIndex];
      ++borderStyleIndex;
    }

    AreaData a;
    double   borderWidth=borderStyle ? borderStyle->GetWidth() : 0.0;

    a.boundingBox=ring.GetBoundingBox();
    a.isOuter=ring.IsOuter();

    if (!IsVisibleArea(projection,
                       a.boundingBox,
                       borderWidth/2.0)) {
      return false;
    }

    // Collect possible clippings. We only take into account inner rings of the next level
    // that do not have a type and thus act as a clipping region. If a inner ring has a type,
    // we currently assume that it does not have alpha and paints over its region and clipping is
    // not required.
    area.VisitClippingRings(i, [&a, &coordRanges](size_t j, const Area::Ring &, const TypeInfoRef &type) -> bool {
      if (type->GetIgnore() && coordRanges[j].IsValid()) {
        a.clippings.push_back(coordRanges[j]);
      }

      return true;
    });

    a.ref=area.GetObjectFileRef();
    a.type=type;
    a.buffer=&ring.GetFeatureValueBuffer();
    a.center=ring.center;
    a.fillStyle=std::move(fillStyle);
    a.borderStyle=borderStyle;
    a.coordRange=coordRanges[i];

    areaData.push_back(a);

    for (size_t idx=borderStyleIndex;
         idx<borderStyles.size();
         ++idx) {
      borderStyle=borderStyles[idx];

      double offset=0.0;

      CoordBufferRange range=coordRanges[i];

      if (borderStyle->GetOffset()!=0.0) {
        offset+=GetProjectedWidth(projection,
                                  borderStyle->GetOffset());
      }

      if (borderStyle->GetDisplayOffset()!=0.0) {
        offset+=projection.ConvertWidthToPixel(borderStyle->GetDisplayOffset());
      }

      if (offset!=0.0) {
        range=coordBuffer.GenerateParallelWay(range,
                                              offset);
      }

      // Add a copy of the AreaData definition without the buffer and the fill but only the border

      a.ref=area.GetObjectFileRef();
      a.type=type;
      a.buffer=nullptr;
      a.fillStyle=nullptr;
      a.borderStyle=borderStyle;
      a.coordRange=range;

      areaData.push_back(a);
    }

    return true;
  }

  void MapPainter::PrepareArea(const StyleConfig& styleConfig,
                               const Projection& projection,
                               const MapParameter& parameter,
                               const AreaRef &area)
  {
    std::vector<CoordBufferRange> td(area->rings.size()); // Polygon information for each ring

    for (size_t i=0; i<area->rings.size(); i++) {
      const Area::Ring &ring = area->rings[i];
      // The master ring does not have any nodes, so we skip it
      // Rings with less than 3 nodes should be skipped, too (no area)
      if (ring.IsMaster() || ring.nodes.size() < 3) {
        // td is initialized to empty by default
        continue;
      }

      if (ring.segments.size() <= 1){
        td[i]=TransformArea(ring.nodes,
                            transBuffer,
                            coordBuffer,
                            projection,
                            parameter.GetOptimizeAreaNodes(),
                            errorTolerancePixel);
      }
      else {
        std::vector<Point> nodes;

        for (const auto &segment:ring.segments){
          if (projection.GetDimensions().Intersects(segment.bbox, false)){
            // TODO: add TransBuffer::Transform* methods with vector subrange (begin/end)
            nodes.insert(nodes.end(), ring.nodes.data() + segment.from, ring.nodes.data() + segment.to);
          }
          else {
            nodes.push_back(ring.nodes[segment.from]);
            nodes.push_back(ring.nodes[segment.to-1]);
          }
        }

        td[i]=TransformArea(nodes,
                            transBuffer,
                            coordBuffer,
                            projection,
                            parameter.GetOptimizeAreaNodes(),
                            errorTolerancePixel);
      }
    }

    area->VisitRings([this,&styleConfig,&projection,&parameter,&td,&area](size_t i,
                         const Area::Ring& ring,
                         const TypeInfoRef& type)->bool {
      return PrepareAreaRing(styleConfig,
                             projection,
                             parameter,
                             td,
                             *area,
                             ring,
                             i,
                             type);
    });
  }

  void MapPainter::ProcessAreas(const Projection& projection,
                                const MapParameter& parameter,
                                const MapData& data)
  {
    areaData.clear();

    //Areas
    for (const auto& area : data.areas) {
      PrepareArea(*styleConfig,
                  projection,
                  parameter,
                  area);
    }

    // POI Areas
    for (const auto& area : data.poiAreas) {
      PrepareArea(*styleConfig,
                  projection,
                  parameter,
                  area);
    }
  }

  std::vector<OffsetRel> MapPainter::ParseLaneTurns(const LanesFeatureValue &feature) const
  {
    std::vector<OffsetRel> laneTurns;

    laneTurns.reserve(feature.GetLanes());

    // backward

    auto turns=feature.GetTurnBackward();
    int lanes=feature.GetBackwardLanes();
    int lane=0;

    for (const LaneTurn &turn: turns) {
      if (lane>=lanes){
        break;
      }

      laneTurns.push_back(ParseBackwardTurnStringToOffset(turn));
      ++lane;
    }
    for (;lane<lanes;++lane){
      laneTurns.push_back(OffsetRel::base);
    }

    // forward

    turns=feature.GetTurnForward();
    lanes=feature.GetForwardLanes();
    lane=0;

    for (const LaneTurn &turn: turns) {
      if (lane>=lanes){
        break;
      }

      laneTurns.push_back(ParseForwardTurnStringToOffset(turn));
      ++lane;
    }
    for (;lane<lanes;++lane){
      laneTurns.push_back(OffsetRel::base);
    }

    return laneTurns;
  }

  void MapPainter::TransformPathData(const Projection& projection,
                                     const MapParameter& parameter,
                                     const Way& way,
                                     WayPathData &pathData)
  {
    if (way.segments.size() <= 1) {
      pathData.coordRange=TransformWay(way.nodes,
                                       transBuffer,
                                       coordBuffer,
                                       projection,
                                       parameter.GetOptimizeWayNodes(),
                                       errorTolerancePixel);
    }
    else {
      std::vector<Point> nodes;
      for (const auto &segment : way.segments){
        if (projection.GetDimensions().Intersects(segment.bbox, false)){
          // TODO: add TransBuffer::Transform* methods with vector subrange (begin/end)
          nodes.insert(nodes.end(), way.nodes.data() + segment.from, way.nodes.data() + segment.to);
        }
        else {
          nodes.push_back(way.nodes[segment.from]);
          nodes.push_back(way.nodes[segment.to-1]);
        }
      }

      pathData.coordRange=TransformWay(nodes,
                                       transBuffer,
                                       coordBuffer,
                                       projection,
                                       parameter.GetOptimizeWayNodes(),
                                       errorTolerancePixel);
    }
  }

  double MapPainter::CalculateLineWith(const Projection& projection,
                                       const FeatureValueBuffer& buffer,
                                       const LineStyle& lineStyle) const
  {
    double lineWidth=0.0;

    if (lineStyle.GetWidth()>0.0) {
      const WidthFeatureValue *widthValue=widthReader.GetValue(buffer);


      if (widthValue!=nullptr) {
        lineWidth+=GetProjectedWidth(projection,
                                     widthValue->GetWidth());
      }
      else {
        lineWidth+=GetProjectedWidth(projection,
                                     lineStyle.GetWidth());
      }
    }

    if (lineStyle.GetDisplayWidth()>0.0) {
      lineWidth+=projection.ConvertWidthToPixel(lineStyle.GetDisplayWidth());
    }

    return lineWidth;
  }

  double MapPainter::CalculateLineOffset(const Projection& projection,
                                         const LineStyle& lineStyle,
                                         double lineWidth) const
  {
    double lineOffset=0.0;

    switch (lineStyle.GetOffsetRel()) {
    case OffsetRel::base:
    case OffsetRel::laneDivider:
    case OffsetRel::laneForwardLeft:
    case OffsetRel::laneForwardThroughLeft:
    case OffsetRel::laneForwardThrough:
    case OffsetRel::laneForwardThroughRight:
    case OffsetRel::laneForwardRight:
    case OffsetRel::laneBackwardLeft:
    case OffsetRel::laneBackwardThroughLeft:
    case OffsetRel::laneBackwardThrough:
    case OffsetRel::laneBackwardThroughRight:
    case OffsetRel::laneBackwardRight:
      lineOffset=0.0;
      break;
    case OffsetRel::leftOutline:
      lineOffset=-lineWidth/2.0;
      break;
    case OffsetRel::rightOutline:
      lineOffset=lineWidth/2.0;
      break;
    case OffsetRel::sidecar:
      lineOffset=-1.1 * lineWidth/2.0; // special handling not supported for ordinary ways
      break;
    }

    if (lineStyle.GetOffset()!=0.0) {
      lineOffset+=GetProjectedWidth(projection,
                                    lineStyle.GetOffset());
    }

    if (lineStyle.GetDisplayOffset()!=0.0) {
      lineOffset+=projection.ConvertWidthToPixel(lineStyle.GetDisplayOffset());
    }

    return lineOffset;
  }

  Color MapPainter::CalculateLineColor(const FeatureValueBuffer& buffer,
                                       const LineStyle& lineStyle) const
  {
    Color color=lineStyle.GetLineColor();

    if (lineStyle.GetPreferColorFeature()){
      const ColorFeatureValue *colorValue=colorReader.GetValue(buffer);
      if (colorValue != nullptr){
        color=colorValue->GetColor();
      }
    }

    return color;
  }

  int8_t MapPainter::CalculateLineLayer(const FeatureValueBuffer& buffer) const
  {
    const LayerFeatureValue *layerValue=layerReader.GetValue(buffer);

    if (layerValue!=nullptr) {
      return layerValue->GetLayer();
    }

    return 0;
  }

  void MapPainter::CalculateWayPaths(const StyleConfig& styleConfig,
                                     const Projection& projection,
                                     const MapParameter& parameter,
                                     const Way& way)
  {
    FileOffset ref=way.GetFileOffset();
    const FeatureValueBuffer& buffer=way.GetFeatureValueBuffer();

    styleConfig.GetWayLineStyles(buffer,
                                 projection,
                                 lineStyles);

    if (lineStyles.empty()) {
      return;
    }

    bool transformed=false;
    const AccessFeatureValue *accessValue=nullptr;
    const LanesFeatureValue  *lanesValue=nullptr;
    std::vector<OffsetRel>   laneTurns; // cached turns

    WayPathData pathData;

    pathData.ref=ref;
    pathData.buffer=&buffer;
    pathData.mainSlotWidth=0.0;

    // Calculate mainSlotWidth
    for (const auto& lineStyle : lineStyles) {
      if (lineStyle->GetSlot().empty()) {
        pathData.mainSlotWidth=CalculateLineWith(projection,
                                                 buffer,
                                                 *lineStyle);
      }
    }

    if (pathData.mainSlotWidth==0.0) {
      log.Warn() << "Line style for way " << way.GetFileOffset()
                 << " of type " << way.GetType()->GetName()
                 << " results in empty mainSlotWidth";
    }

    for (const auto& lineStyle : lineStyles) {
      double lineWidth;

      if (lineStyle->GetSlot().empty()) {
        lineWidth=pathData.mainSlotWidth;
      }
      else {
        lineWidth=CalculateLineWith(projection,
                                    buffer,
                                    *lineStyle);
      }

      if (lineWidth==0.0) {
        continue;
      }

      if (!IsVisibleWay(projection,
                        way.GetBoundingBox(),
                        lineWidth/2)) {
        continue;
      }

      if (lineStyle->GetOffsetRel()==OffsetRel::laneDivider) {
        lanesValue=lanesReader.GetValue(buffer);
        accessValue=accessReader.GetValue(buffer);

        if (lanesValue==nullptr &&
            accessValue==nullptr) {
          continue;
        }
      }
      else if (IsLaneOffset(lineStyle->GetOffsetRel())) {
        lanesValue=lanesReader.GetValue(buffer);
      }

      double lineOffset=CalculateLineOffset(projection,
                                            *lineStyle,
                                            pathData.mainSlotWidth);

      WayData data;

      data.lineWidth=lineWidth;

      if (!transformed) {
        TransformPathData(projection, parameter, way, pathData);
        transformed=true;
        wayPathData.push_back(pathData);
      }

      data.buffer=&buffer;
      data.lineStyle=lineStyle;
      data.layer=CalculateLineLayer(buffer);
      data.color=CalculateLineColor(buffer,
                                    *lineStyle);
      data.wayPriority=styleConfig.GetWayPrio(buffer.GetType());
      data.coordRange=pathData.coordRange;
      data.startIsClosed=!way.GetFront().IsRelevant();
      data.endIsClosed=!way.GetBack().IsRelevant();

      if (lineOffset!=0.0) {
        data.coordRange=coordBuffer.GenerateParallelWay(pathData.coordRange,
                                                         lineOffset);
      }

      if (lineStyle->GetOffsetRel()==OffsetRel::laneDivider) {
        uint8_t lanes=0;

        if (lanesValue!=nullptr) {
          lanes=lanesValue->GetLanes();
        }
        else if (accessValue->IsOneway()) {
          lanes=buffer.GetType()->GetOnewayLanes();
        }
        else {
          lanes=buffer.GetType()->GetLanes();
        }

        if (lanes<2) {
          continue;
        }

        double lanesSpace=pathData.mainSlotWidth/lanes;
        double laneOffset=-pathData.mainSlotWidth/2.0+lanesSpace;

        for (size_t lane=1; lane<lanes; ++lane) {
          data.coordRange=coordBuffer.GenerateParallelWay(pathData.coordRange,
                                                           laneOffset);
          wayData.push_back(data);
          laneOffset+=lanesSpace;
        }
      }
      else if (IsLaneOffset(lineStyle->GetOffsetRel())) {
        if (lanesValue==nullptr || lanesValue->GetLanes()<1){
          continue;
        }

        if (laneTurns.empty()) {
          laneTurns=ParseLaneTurns(*lanesValue);
          assert(laneTurns.size() <= lanesValue->GetLanes());
        }

        int lanes=lanesValue->GetLanes();
        double lanesSpace=pathData.mainSlotWidth/lanes;
        double laneOffset=-pathData.mainSlotWidth/2.0+lanesSpace/2.0;

        for (const OffsetRel &laneTurn: laneTurns) {
          if (lineStyle->GetOffsetRel() == laneTurn) {
            data.coordRange=coordBuffer.GenerateParallelWay(pathData.coordRange,
                                                             laneOffset);
            wayData.push_back(data);
          }
          laneOffset+=lanesSpace;
        }
      }
      else {
        wayData.push_back(data);
      }
    }
  }

  void MapPainter::CalculatePaths(const Projection& projection,
                                  const MapParameter& parameter,
                                  const MapData& data)
  {
    wayData.clear();
    wayPathData.clear();
    routeLabelData.clear();

    for (const auto& way : data.ways) {
      if (way->IsValid()) {
        CalculateWayPaths(*styleConfig,
                          projection,
                          parameter,
                          *way);
      }
    }

    for (const auto& way : data.poiWays) {
      if (way->IsValid()) {
        CalculateWayPaths(*styleConfig,
                          projection,
                          parameter,
                          *way);
      }
    }
  }

  void MapPainter::CalculateWayShields(const Projection& projection,
                                       const MapParameter& parameter,
                                       const MapData& data)
  {
    if (!styleConfig->HasWayPathShieldStyle(projection)) {
      return;
    }

    for (const auto& way : data.ways) {
      if (way->IsValid()) {
        CalculateWayShieldLabels(*styleConfig,
                                 projection,
                                 parameter,
                                 *way);
      }
    }

    for (const auto& way : data.poiWays) {
      if (way->IsValid()) {
        CalculateWayShieldLabels(*styleConfig,
                                 projection,
                                 parameter,
                                 *way);
      }
    }
  }

  void MapPainter::ProcessRoutes(const Projection& projection,
                                 const MapParameter& parameter,
                                 const MapData& data)
  {
    routeLabelData.clear();

    if (data.routes.empty()){
      return;
    }

    struct RouteSegmentData
    {
      size_t                   transStart;      //!< Start of coordinates in transformation buffer
      size_t                   transEnd;        //!< End of coordinates in transformation buffer (inclusive)
      Route::MemberDirection   direction;
    };

    // Data structure for holding temporary data about route
    struct RouteData
    {
      LineStyleRef                lineStyle;       //!< Line style
      Color                       color;           //!< Color of route
      double                      lineWidth;
      std::list<RouteSegmentData> transSegments;   //!< Transformation buffer segments
    };

    struct WayRoutes
    {
      WayPathDataIt wayData;
      std::set<Color> colors; // collapse "sidecar" routes with same color
      double rightSideCarPos=0;
      double leftSideCarPos=0;
      std::map<PathTextStyleRef,std::set<std::string>> labels;
    };

    double sidecarOffset=std::min(projection.ConvertWidthToPixel(parameter.GetSidecarMaxDistanceMM()),
                                  GetProjectedWidth(projection,
                                                    parameter.GetSidecarDistance(),
                                                    projection.ConvertWidthToPixel(parameter.GetSidecarMinDistanceMM())));

    std::map<FileOffset,WayRoutes> wayDataMap;
    for (auto it=wayPathData.begin(); it != wayPathData.end(); ++it){
      auto &wayRoute=wayDataMap[it->ref];
      wayRoute.wayData=it;
      wayRoute.rightSideCarPos=(it->mainSlotWidth/2)+sidecarOffset;
      wayRoute.leftSideCarPos=wayRoute.rightSideCarPos*-1;
    }

    for (const auto &route:data.routes){
      if (!projection.GetDimensions().Intersects(route->bbox)) {
        // outside projection, continue
        continue;
      }

      styleConfig->GetRouteLineStyles(route->GetFeatureValueBuffer(),
                                      projection,
                                      lineStyles);

      if (lineStyles.empty()){
        continue;
      }
      LineStyleRef lineStyle=lineStyles.front(); // slots for routes are not supported yet
      assert(lineStyle);

      PathTextStyleRef routeTextStyle=styleConfig->GetRoutePathTextStyle(route->GetFeatureValueBuffer(),
                                                                         projection);

      Color color=CalculateLineColor(route->GetFeatureValueBuffer(),*lineStyle);

      double lineWidth=CalculateLineWith(projection,route->GetFeatureValueBuffer(),*lineStyle);

      RouteData routeTmp;
      auto FlushRouteData = [&](){
        if (!routeTmp.transSegments.empty()) {
          routeTmp.lineStyle=lineStyle;
          routeTmp.lineWidth=lineWidth;
          routeTmp.color=color;

          size_t size=routeTmp.transSegments.size();
          size_t i=0;
          for (const auto &segment : routeTmp.transSegments) {
            assert(segment.transStart < segment.transEnd);

            WayData segmentWay;

            segmentWay.buffer=&(route->GetFeatureValueBuffer());
            segmentWay.layer=0;
            segmentWay.lineStyle=lineStyle;
            segmentWay.color=color;
            segmentWay.wayPriority=lineStyle->GetPriority();
            segmentWay.coordRange=CoordBufferRange(coordBuffer,segment.transStart,segment.transEnd);
            segmentWay.lineWidth=lineWidth;
            segmentWay.startIsClosed=(i==0);
            segmentWay.endIsClosed=(i==size-1);
            wayData.push_back(segmentWay);
            i++;
          }

          routeTmp = RouteData();
        }
      };

      std::unique_ptr<Route::MemberCache> resolvedMembers;
      for (const auto &segment:route->segments){
        for (const auto &member:segment.members){
          auto memberWay=wayDataMap.find(member.way);
          if (memberWay==wayDataMap.end()){
            // when route member is not rendered, it is not present in wayPathData
            // but when we have it in resolved members, we may still process it
            // and render route on it
            if (resolvedMembers==nullptr) {
              // load member cache lazy
              resolvedMembers=std::make_unique<Route::MemberCache>(route->GetResolvedMembers());
            }
            if (auto it=resolvedMembers->find(member.way);
                it!=resolvedMembers->end()){

              assert(member.way==it->second->GetFileOffset());

              if (!projection.GetDimensions().Intersects(it->second->GetBoundingBox())){
                // outside projection, end segment and continue
                FlushRouteData();
                continue;
              }
              WayPathData pathData;

              pathData.ref=member.way;
              pathData.buffer=&(it->second->GetFeatureValueBuffer());
              TransformPathData(projection, parameter, *(it->second), pathData);
              pathData.mainSlotWidth=0.0;

              wayPathData.push_back(pathData);

              auto &wayRoute=wayDataMap[member.way];
              wayRoute.wayData=std::prev(wayPathData.end());
              wayRoute.rightSideCarPos=0;
              wayRoute.leftSideCarPos=0;
              memberWay=wayDataMap.find(member.way);
            } else {
              // we don't have way in data, end segment and continue
              FlushRouteData();
              continue;
            }
          }

          assert(memberWay!=wayDataMap.end());

          if (routeTextStyle){
            assert(routeTextStyle->GetLabel());
            std::string label = routeTextStyle->GetLabel()->GetLabel(parameter, route->GetFeatureValueBuffer());
            if (!label.empty()) {
              memberWay->second.labels[routeTextStyle].insert(label);
            }
          }

          // collapse colors
          const auto& pathData=memberWay->second.wayData;
          if (memberWay->second.colors.contains(color)){
            FlushRouteData();
            continue;
          }
          memberWay->second.colors.insert(color);

          double lineOffset=0;
          if (lineStyle->GetOffsetRel() == OffsetRel::sidecar) {
            if (member.direction==Route::MemberDirection::forward){
              lineOffset=memberWay->second.rightSideCarPos + (lineWidth/2);
              memberWay->second.rightSideCarPos += lineWidth;
            }else{
              lineOffset=memberWay->second.leftSideCarPos - (lineWidth/2);
              memberWay->second.leftSideCarPos -= lineWidth;
            }
          }
          if (lineStyle->GetOffset() != 0.0) {
            lineOffset += lineStyle->GetOffset() / projection.GetPixelSize();
          }

          if (lineStyle->GetDisplayOffset() != 0.0) {
            lineOffset += projection.ConvertWidthToPixel(lineStyle->GetDisplayOffset());
          }

          size_t transStart;
          size_t transEnd;

          if (lineOffset==0) {
            transStart=pathData->coordRange.GetStart();
            transEnd=pathData->coordRange.GetEnd();
          }
          else {
            CoordBufferRange range=coordBuffer.GenerateParallelWay(pathData->coordRange,
                                                                    lineOffset);

            transStart=range.GetStart();
            transEnd=range.GetEnd();
          }

          if (routeTmp.transSegments.empty()){
            RouteSegmentData routeSegmentData{transStart, transEnd, member.direction};
            routeTmp.transSegments.push_back(routeSegmentData);
          }
          else {
            auto &lastSegment=routeTmp.transSegments.back();
            if (lastSegment.transEnd+1==transStart &&
                member.direction==Route::MemberDirection::forward &&
                lastSegment.direction==Route::MemberDirection::forward){
              // prolong current segment
              lastSegment.transEnd=transEnd;
            }else {
              // insert connecting segment between end of lastSegment and transStart
              Vertex2D lastEnd;
              if (lastSegment.direction==Route::MemberDirection::forward){
                lastEnd=coordBuffer.buffer[lastSegment.transEnd];
              }
              else{
                lastEnd=coordBuffer.buffer[lastSegment.transStart];
              }
              Vertex2D currentStart;
              if (member.direction==Route::MemberDirection::forward){
                currentStart=coordBuffer.buffer[transStart];
              }
              else{
                currentStart=coordBuffer.buffer[transEnd];
              }
              RouteSegmentData segmentConnection{coordBuffer.PushCoord(lastEnd),
                                                 coordBuffer.PushCoord(currentStart),
                                                 Route::MemberDirection::forward};
              routeTmp.transSegments.push_back(segmentConnection);

              RouteSegmentData routeSegmentData{transStart, transEnd, member.direction};
              routeTmp.transSegments.push_back(routeSegmentData);
            }
          }
        }
        FlushRouteData();
      }
    }

    // collect data about route labels
    for (const auto& [offset, routes] : wayDataMap){
      if (!routes.labels.empty()){
        RouteLabelData labelData;

        labelData.wayData= routes.wayData;
        labelData.labels = routes.labels;

        routeLabelData.push_back(labelData);
      }
    }
  }

  bool MapPainter::Draw(const Projection& projection,
                        const MapParameter& parameter,
                        const MapData& data,
                        RenderSteps startStep,
                        RenderSteps endStep)
  {
    assert(startStep>=RenderSteps::FirstStep);
    assert(startStep<=RenderSteps::LastStep);
    assert(startStep<=endStep);

    for (size_t step=startStep; step<=endStep; step++) {
      StepMethod stepMethod=stepMethods[step];

      assert(stepMethod!=nullptr);

      (this->*stepMethod)(projection,parameter,data);

      if (parameter.IsAborted()) {
        return false;
      }
    }

    return true;
  }

  bool MapPainter::Draw(const Projection& projection,
                        const MapParameter& parameter,
                        const MapData& data)
  {
    return Draw(projection,
                parameter,
                data,
                RenderSteps::FirstStep,
                RenderSteps::LastStep);
  }

  /**
   * Base method that must get called to initialize the renderer for a render action.
   * The derived method of the concrete renderer implementation can add further initialisation.
   */
  void MapPainter::InitializeRender(const Projection& projection,
                                    const MapParameter& parameter,
                                    const MapData& /*data*/)
  {
    errorTolerancePixel=projection.ConvertWidthToPixel(parameter.GetOptimizeErrorToleranceMm());
    areaMinDimension   =projection.ConvertWidthToPixel(parameter.GetAreaMinDimensionMM());
    contourLabelOffset =projection.ConvertWidthToPixel(parameter.GetContourLabelOffset());
    contourLabelSpace  =projection.ConvertWidthToPixel(parameter.GetContourLabelSpace());

    shieldGridSizeHoriz=360.0/(std::pow(2,projection.GetMagnification().GetLevel()+1));
    shieldGridSizeVert=180.0/(std::pow(2,projection.GetMagnification().GetLevel()+1));

    coordBuffer.Reset();

    standardFontSize=GetFontHeight(projection,
                                   parameter,
                                   1.0);
  }

  void MapPainter::DumpStatistics(const Projection& projection,
                                  const MapParameter& parameter,
                                  const MapData& data)
  {
    MapPainterStatistics statistics;

    statistics.DumpMapPainterStatistics(*styleConfig,
                                        projection,
                                        parameter,
                                        data);
  }

  void MapPainter::AfterPreprocessing(const Projection& projection,
                                      const MapParameter& parameter,
                                      const MapData& data)
  {
    wayData.sort();
    areaData.sort(AreaSorter);

    // Optional callback after preprocessing data
    AfterPreprocessing(*styleConfig,
                       projection,
                       parameter,
                       data);
  }

  void MapPainter::Prerender(const Projection& projection,
                             const MapParameter& parameter,
                             const MapData& data)
  {
    BeforeDrawing(*styleConfig,
                  projection,
                  parameter,
                  data);

    if (parameter.IsDebugPerformance()) {
      GeoBox boundingBox(projection.GetDimensions());

      log.Info()
        << "Draw: " << boundingBox.GetDisplayText() << " "
        << (int) projection.GetMagnification().GetMagnification() << "x" << "/"
        << projection.GetMagnification().GetLevel() << " "
        << projection.GetWidth() << "x" << projection.GetHeight() << " " << projection.GetDPI() << " DPI";
    }
  }

  void MapPainter::DrawBaseMapTiles(const Projection& projection,
                                    const MapParameter& parameter,
                                    const MapData& data)
  {
    if (parameter.GetRenderBackground()) {
      DrawGround(projection,
                 parameter,
                 *landFill);
    }

    DrawGroundTiles(projection,
                    parameter,
                    data.baseMapTiles);
  }

  void MapPainter::DrawGroundTiles(const Projection& projection,
                                   const MapParameter& parameter,
                                   const MapData& data)
  {
    DrawGroundTiles(projection,
                    parameter,
                    data.groundTiles);
  }

  static void DumpGroundTile(const GroundTile& tile)
  {
    switch (tile.type) {
      case GroundTile::land:
        log.Info() << "Drawing land tile: " << tile.xRel << "," << tile.yRel;
        break;
      case GroundTile::water:
        log.Info() << "Drawing water tile: " << tile.xRel << "," << tile.yRel;
        break;
      case GroundTile::coast:
        log.Info() << "Drawing coast tile: " << tile.xRel << "," << tile.yRel;
        break;
      case GroundTile::unknown:
        log.Info() << "Drawing unknown tile: " << tile.xRel << "," << tile.yRel;
        break;
    }
  }

  void MapPainter::DrawGroundTiles(const Projection& projection,
                                   const MapParameter& parameter,
                                   const std::list<GroundTile>& groundTiles)
  {
    if (!parameter.GetRenderSeaLand()) {
      return;
    }

    [[maybe_unused]] std::set<GeoCoord> drawnLabels; // used when debugGroundTiles == true

    FillStyleRef          landFill=styleConfig->GetLandFillStyle(projection);
    FillStyleRef          seaFill=styleConfig->GetSeaFillStyle(projection);
    FillStyleRef          coastFill=styleConfig->GetCoastFillStyle(projection);
    FillStyleRef          unknownFill=styleConfig->GetUnknownFillStyle(projection);
    LineStyleRef          coastlineLine=styleConfig->GetCoastlineLineStyle(projection);
    std::vector<GeoCoord> coords;
    size_t                start=0; // Make the compiler happy
    size_t                end=0;   // Make the compiler happy

    if (!landFill) {
      landFill=this->landFill;
    }

    if (!seaFill) {
      seaFill=this->seaFill;
    }

    // Due to rounding errors during coordinate transformations there may be small
    // space between tiles. For that reason we increase tile size approximately by 1 pixel
    // to avoid visible grid.
    double pixelAsDegree = GetDistanceInLonDegrees(Meters(projection.GetPixelSize()));

    for (const auto& tile : groundTiles) {
      AreaData groundTileData;

      if constexpr (debugGroundTiles) {
        DumpGroundTile(tile);
      }

      if (tile.type==GroundTile::unknown &&
          !parameter.GetRenderUnknowns()) {
        continue;
      }

      switch (tile.type) {
      case GroundTile::land:
        groundTileData.fillStyle=landFill;
        break;
      case GroundTile::water:
        groundTileData.fillStyle=seaFill;
        break;
      case GroundTile::coast:
        groundTileData.fillStyle=coastFill;
        break;
      case GroundTile::unknown:
        groundTileData.fillStyle=unknownFill;
        break;
      }

      if (!groundTileData.fillStyle) {
        continue;
      }

      const GeoCoord minCoord(tile.yAbs*tile.cellHeight - 90.0 - pixelAsDegree/2,
                              tile.xAbs*tile.cellWidth - 180.0 - pixelAsDegree/2);
      const GeoCoord maxCoord(minCoord.GetLat() + tile.cellHeight + pixelAsDegree,
                              minCoord.GetLon() + tile.cellWidth + pixelAsDegree);

      groundTileData.boundingBox.Set(minCoord,maxCoord);

      // skip tiles that are completely outside projection
      if (!projection.GetDimensions().Intersects(groundTileData.boundingBox)){
        if constexpr (debugGroundTiles) {
          log.Info() << "Tile outside projection: " << tile.xRel << "," << tile.yRel
                     << " " << groundTileData.boundingBox.GetDisplayText();
        }
        continue;
      }

      if (tile.coords.empty()) {
        if constexpr (debugGroundTiles) {
          log.Info() << " >= fill";
        }
        CoordBufferRange range=TransformBoundingBox(groundTileData.boundingBox,
                                                    transBuffer,
                                                    coordBuffer,
                                                    projection,
                                                    TransPolygon::none,
                                                    errorTolerancePixel);

        start=range.GetStart();
        end=range.GetEnd();
      }
      else {
        if constexpr (debugGroundTiles) {
          log.Info() << " >= sub";
        }
        coords.resize(tile.coords.size());

        for (size_t i=0; i<tile.coords.size(); i++) {
          double lat;
          double lon;

          lat=groundTileData.boundingBox.GetMinCoord().GetLat() +
            double(tile.coords[i].y) / double(GroundTile::Coord::CELL_MAX) * (tile.cellHeight+pixelAsDegree);
          lon=groundTileData.boundingBox.GetMinCoord().GetLon() +
            double(tile.coords[i].x) / double(GroundTile::Coord::CELL_MAX) * (tile.cellWidth+pixelAsDegree);

          coords[i]=GeoCoord(lat,lon);
        }

        TransformArea(coords,
                      transBuffer,
                      projection,
                      TransPolygon::none,
                      errorTolerancePixel);

        for (size_t i=transBuffer.GetStart(); i<=transBuffer.GetEnd(); i++) {
          double x,y;

          if (tile.coords[i].x==0) {
            x=::floor(transBuffer.points[i].x);
          }
          else if (tile.coords[i].x==GroundTile::Coord::CELL_MAX) {
            x=::ceil(transBuffer.points[i].x);
          }
          else {
            x=transBuffer.points[i].x;
          }

          if (tile.coords[i].y==0) {
            y=::ceil(transBuffer.points[i].y);
          }
          else if (tile.coords[i].y==GroundTile::Coord::CELL_MAX) {
            y=::floor(transBuffer.points[i].y);
          }
          else {
            y=transBuffer.points[i].y;
          }

          size_t idx=coordBuffer.PushCoord(Vertex2D(x,y));

          if (i==transBuffer.GetStart()) {
            start=idx;
          }
          else if (i==transBuffer.GetEnd()) {
            end=idx;
          }
        }

        if (coastlineLine) {
          size_t lineStart=0;
          size_t lineEnd;

          while (lineStart<tile.coords.size()) {
            while (lineStart<tile.coords.size() &&
                   !tile.coords[lineStart].coast) {
              ++lineStart;
            }

            if (lineStart>=tile.coords.size()) {
              continue;
            }

            lineEnd=lineStart;

            while (lineEnd<tile.coords.size() &&
                   tile.coords[lineEnd].coast) {
              ++lineEnd;
            }

            if (lineStart!=lineEnd) {
              WayData wd;

              wd.buffer=&coastlineSegmentAttributes;
              wd.layer=0;
              wd.lineStyle=coastlineLine;
              wd.color=coastlineLine->GetLineColor();
              wd.wayPriority=std::numeric_limits<size_t>::max();
              wd.coordRange=CoordBufferRange(coordBuffer,start+lineStart,start+lineEnd);
              wd.lineWidth=GetProjectedWidth(projection,
                                             projection.ConvertWidthToPixel(coastlineLine->GetDisplayWidth()),
                                             coastlineLine->GetWidth());
              wd.startIsClosed=false;
              wd.endIsClosed=false;

              wayData.push_back(wd);
            }

            lineStart=lineEnd+1;
          }
        }
      }

      groundTileData.ref=ObjectFileRef();
      groundTileData.coordRange=CoordBufferRange(coordBuffer,start,end);

      DrawArea(projection,parameter,groundTileData);

      if constexpr (debugGroundTiles) {
        GeoCoord cc = groundTileData.boundingBox.GetCenter();

        std::string label;

        size_t x = (cc.GetLon() + 180) / tile.cellWidth;
        size_t y = (cc.GetLat() + 90) / tile.cellHeight;

        label = std::to_string(tile.xRel) + "," + std::to_string(tile.yRel);

        double lon = (x * tile.cellWidth + tile.cellWidth / 2) - 180.0;
        double lat = (y * tile.cellHeight + tile.cellHeight / 2) - 90.0;

        Vertex2D pixel;

        projection.GeoToPixel(GeoCoord(lat, lon),
                              pixel);

        if (drawnLabels.contains(GeoCoord(x, y))) {
          continue;
        }

        LabelData labelBox;

        labelBox.priority = 0;
        labelBox.alpha = debugLabel->GetAlpha();
        labelBox.fontSize = debugLabel->GetSize();
        labelBox.style = debugLabel;
        labelBox.text = label;

        std::vector<LabelData> vect;
        vect.push_back(labelBox);
        RegisterRegularLabel(projection,
                             parameter,
                             ObjectFileRef(),
                             vect,
                             pixel,
                             /*proposedWidth*/ -1);

        drawnLabels.insert(GeoCoord(x, y));
      }
    }
  }

  void MapPainter::DrawOSMTileGrids(const Projection& projection,
                                    const MapParameter& parameter,
                                    const MapData& /*data*/)
  {
    LineStyleRef osmSubTileLine=styleConfig->GetOSMSubTileBorderLineStyle(projection);

    if (osmSubTileLine) {
      Magnification magnification=projection.GetMagnification();

      ++magnification;

      DrawOSMTileGrid(projection,
                      parameter,
                      magnification,
                      osmSubTileLine);
    }

    LineStyleRef osmTileLine=styleConfig->GetOSMTileBorderLineStyle(projection);

    if (osmTileLine) {
      Magnification magnification=projection.GetMagnification();

      DrawOSMTileGrid(projection,
                      parameter,
                      magnification,
                      osmTileLine);
    }
  }

  void MapPainter::DrawAreas(const Projection& projection,
                             const MapParameter& parameter,
                             const MapData& /*data*/)
  {
    StopClock timer;

    for (const auto& area : areaData) {
      DrawArea(projection,
               parameter,
               area);
    }

    timer.Stop();

    if (parameter.IsDebugPerformance() && timer.IsSignificant()) {
      log.Info()
        << "Draw areas: " << areaData.size() << " (pcs) " << timer.ResultString() << " (s)";
    }
  }

  void MapPainter::DrawWays(const Projection& projection,
                            const MapParameter& parameter,
                            const MapData& /*data*/)
  {
    StopClock timer;

    for (const auto& way : wayData) {
      DrawWay(*styleConfig,
              projection,
              parameter,
              way);
    }

    timer.Stop();

    if (parameter.IsDebugPerformance() && timer.IsSignificant()) {
      log.Info()
        << "Draw ways: " << wayData.size() << " (pcs) " << timer.ResultString() << " (s)";
    }
  }

  void MapPainter::DrawWayDecorations(const Projection& projection,
                                      const MapParameter& parameter,
                                      const MapData& /*data*/)
  {
    //
    // Path decorations (like arrows and similar)
    //

    StopClock timer;
    size_t    drawnCount=0;

    for (const auto& way: wayPathData) {
      if (DrawWayDecoration(*styleConfig,
                            projection,
                            parameter,
                            way)) {
        ++drawnCount;
      }
    }

    timer.Stop();

    if (parameter.IsDebugPerformance() && timer.IsSignificant()) {
      log.Info()
        << "Draw way decorations: " << drawnCount << " (pcs) " << timer.ResultString() << "(s)";
    }
  }

  void MapPainter::DrawWayContourLabels(const Projection& projection,
                                       const MapParameter& parameter,
                                       const MapData& /*data*/)
  {
    // Draw labels only if there is a style for the current zoom level
    // that requires labels
    if (!styleConfig->HasWayPathTextStyle(projection)){
      return;
    }

    StopClock timer;
    size_t    drawnCount=0;

    for (const auto& way : wayPathData) {
      if (DrawWayContourLabel(*styleConfig,
                              projection,
                              parameter,
                              way)) {
        ++drawnCount;
      }
    }

    timer.Stop();

    if (parameter.IsDebugPerformance() && timer.IsSignificant()) {
      log.Info()
        << "Draw way contour labels: " << drawnCount << " (pcs) " << timer.ResultString() << "(s)";
    }
  }

  void MapPainter::PrepareAreaLabels(const Projection& projection,
                                     const MapParameter& parameter,
                                     const MapData& /*data*/)
  {
    StopClock timer;
    size_t    drawnCount=0;

    for (const auto& area : areaData)
    {
      if (area.buffer==nullptr) {
        continue;
      }

      PrepareAreaLabel(*styleConfig,
                       projection,
                       parameter,
                       area);

      ++drawnCount;
    }

    timer.Stop();

    if (parameter.IsDebugPerformance() && timer.IsSignificant()) {
      log.Info()
        << "Draw area labels: " << drawnCount << " (pcs) " << timer.ResultString() << "(s)";
    }
  }

  void MapPainter::DrawAreaBorderLabels(const Projection& projection,
                                        const MapParameter& parameter,
                                        const MapData& /*data*/)
  {
    StopClock timer;
    size_t    drawnCount=0;

    for (const auto& area : areaData) {
      if (area.buffer==nullptr) {
        continue;
      }

      if (DrawAreaBorderLabel(*styleConfig,
                              projection,
                              parameter,
                              area)) {
        ++drawnCount;
      }
    }

    timer.Stop();

    if (parameter.IsDebugPerformance() && timer.IsSignificant()) {
      log.Info()
        << "Draw area border labels: " << drawnCount << " (pcs) " << timer.ResultString() << "(s)";
    }
  }

  void MapPainter::DrawAreaBorderSymbols(const Projection& projection,
                                         const MapParameter& parameter,
                                         const MapData& /*data*/)
  {
    StopClock timer;
    size_t    drawnCount=0;

    for (const auto& area : areaData) {
      if (area.buffer==nullptr) {
        continue;
      }

      if (DrawAreaBorderSymbol(*styleConfig,
                               projection,
                               parameter,
                               area)) {
        ++drawnCount;
      }
    }

    timer.Stop();

    if (parameter.IsDebugPerformance() && timer.IsSignificant()) {
      log.Info()
        << "Draw area border symbols: " << drawnCount << " (pcs) " << timer.ResultString() << "(s)";
    }
  }

  void MapPainter::PrepareNodeLabels(const Projection& projection,
                                     const MapParameter& parameter,
                                     const MapData& data)
  {
    StopClock timer;

    PrepareNodes(*styleConfig,
                 projection,
                 parameter,
                 data);

    timer.Stop();

    if (parameter.IsDebugPerformance() && timer.IsSignificant()) {
      log.Info()
        << "Prepare node labels: " << timer.ResultString() << "(s)";
    }
  }

  void MapPainter::PrepareRouteLabels(const Projection& projection,
                                      const MapParameter& parameter,
                                      const MapData& /*data*/)
  {
    StopClock timer;
    size_t    drawnCount=0;

    for (const auto& routeLabel : routeLabelData) {
      for (const auto& labelEntry : routeLabel.labels) {
        // concatenate all labels for given style
        std::stringstream labels;
        for (auto it=labelEntry.second.begin();
             it!=labelEntry.second.end();
             ++it){
          if (it!=labelEntry.second.begin()){
            labels << parameter.GetRouteLabelSeparator();
          }
          labels << *it;
        }

        if (DrawWayContourLabel(projection,
                                parameter,
                                *(routeLabel.wayData),
                                labelEntry.first,
                                labels.str())) {
          ++drawnCount;
        }
      }
    }

    timer.Stop();

    if (parameter.IsDebugPerformance() && timer.IsSignificant()) {
      log.Info()
          << "Draw route contour labels: " << drawnCount << " (pcs) " << timer.ResultString() << "(s)";
    }
  }

  static double RoundDown(double value) {
    if (value>=0.0) {
      return ::floor(value);
    }

    return ::ceil(value);
  }

  static double RoundUp(double value) {
    if (value>=0.0) {
      return ::ceil(value);
    }

    return ::floor(value);
  }

  [[nodiscard]] static bool CrossesElevationLine(int32_t ele, int32_t height1, int32_t height2)
  {
    return (height1>=ele && height2 <ele) || (height1<ele && height2>=ele);
  }

  void MapPainter::DrawContourLines(const Projection& projection,
                                    const MapParameter& parameter,
                                    const MapData& data)
  {
    if (!parameter.GetRenderContourLines()) {
      return;
    }

    if (!data.srtmTile) {
      log.Warn() << "Contour lines activated but data available";
      return;
    }

    log.Info() << "Draw contour lines";

    TypeInfoRef srtmType=styleConfig->GetTypeConfig()->GetTypeInfo("srtm_tile");

    if (!srtmType) {
      log.Warn() << "Contour lines activated but no type 'srtm_tile' found";
      return;
    }

    if (true) {
      FeatureValueBuffer contourLinesBuffer;

      contourLinesBuffer.SetType(srtmType);

      std::vector<LineStyleRef> contourLineStyles;

      styleConfig->GetWayLineStyles(contourLinesBuffer,projection,contourLineStyles);

      if (contourLineStyles.empty()) {
        log.Warn() << "Contour lines activated but no line style for type 'srtm_tile' found";
      }

      log.Info() << "Processing height map " << data.srtmTile->boundingBox.GetDisplayText()
                 << " " << data.srtmTile->columns << "x" << data.srtmTile->rows;

      int32_t minHeight=std::numeric_limits<int32_t>::max();
      int32_t maxHeight=std::numeric_limits<int32_t>::min();
      for (auto height : data.srtmTile->heights) {
        if (height!=SRTM::nodata) {
          minHeight=std::min(minHeight,height);
          maxHeight=std::max(maxHeight,height);
        }
      }

      if (minHeight>maxHeight) {
        log.Warn() << "No valid height data available";
        return;
      }

      int32_t elevationSteps=25;
      int32_t minElevation=(minHeight/elevationSteps)*elevationSteps;
      int32_t maxElevation=((maxHeight+elevationSteps)/elevationSteps)*elevationSteps;
      GeoBox  boundingBox=projection.GetDimensions();

      log.Info() << "Elevation range: " << minHeight << " => " << maxHeight
                 << " | " << minElevation << " => " << maxElevation;

      for (int32_t ele=minElevation; ele<=maxElevation; ele+=elevationSteps) {
        log.Info() << "### Ele " << ele;
        for (size_t y=0; y<data.srtmTile->rows; y++) {
          for (size_t x=0; x<data.srtmTile->columns; x++) {
            int32_t height=data.srtmTile->GetHeight(x,y);
            GeoBox box=GeoBox(GeoCoord(data.srtmTile->boundingBox.GetMinLat()+(1-double(y)/data.srtmTile->rows),
                                       data.srtmTile->boundingBox.GetMinLon()+double(x)/data.srtmTile->columns),
                              GeoCoord(data.srtmTile->boundingBox.GetMinLat()+(1-double(y+1)/data.srtmTile->rows),
                                       data.srtmTile->boundingBox.GetMinLon()+double(x+1)/data.srtmTile->columns));

            if (!boundingBox.Intersects(box)) {
              continue;
            }

            if (x>0) {
              // left
              int32_t otherHeight=data.srtmTile->GetHeight(x-1,y);
              if (CrossesElevationLine(ele,height,otherHeight)) {
                log.Info() << "Left " << x << "," << y
                           << " " << box.GetDisplayText()
                           << " " << std::abs(otherHeight-height);

                FeatureValueBuffer     buffer;
                WayData                wd;
                std::array<GeoCoord,2> path;

                path[0]=box.GetBottomLeft();
                path[1]=box.GetTopLeft();

                CoordBufferRange range=TransformWay(path,
                                                    transBuffer,
                                                    coordBuffer,
                                                    projection,
                                                    TransPolygon::none,
                                                    errorTolerancePixel);

                wd.buffer=&buffer;
                wd.layer=0;
                wd.lineStyle=contourLineStyles[0];
                wd.color=contourLineStyles[0]->GetLineColor();
                wd.wayPriority=std::numeric_limits<size_t>::max();
                wd.coordRange=range;
                wd.lineWidth=GetProjectedWidth(projection,
                                               projection.ConvertWidthToPixel(contourLineStyles[0]->GetDisplayWidth()),
                                               contourLineStyles[0]->GetWidth());
                wd.startIsClosed=false;
                wd.endIsClosed=false;

                DrawWay(*styleConfig,projection,parameter,wd);
              }
            }

            if (x<data.srtmTile->columns-1) {
              // right
              int32_t otherHeight=data.srtmTile->GetHeight(x+1,y);
              if (CrossesElevationLine(ele,height,otherHeight)) {
                log.Info() << "Right " << x << "," << y
                           << " " << box.GetDisplayText()
                           << " " << std::abs(otherHeight-height);

                FeatureValueBuffer     buffer;
                WayData                wd;
                std::array<GeoCoord,2> path;

                path[0]=box.GetBottomRight();
                path[1]=box.GetTopRight();

                CoordBufferRange range=TransformWay(path,
                                                    transBuffer,
                                                    coordBuffer,
                                                    projection,
                                                    TransPolygon::none,
                                                    errorTolerancePixel);

                wd.buffer=&buffer;
                wd.layer=0;
                wd.lineStyle=contourLineStyles[0];
                wd.color=contourLineStyles[0]->GetLineColor();
                wd.wayPriority=std::numeric_limits<size_t>::max();
                wd.coordRange=range;
                wd.lineWidth=GetProjectedWidth(projection,
                                               projection.ConvertWidthToPixel(contourLineStyles[0]->GetDisplayWidth()),
                                               contourLineStyles[0]->GetWidth());
                wd.startIsClosed=false;
                wd.endIsClosed=false;

                DrawWay(*styleConfig,projection,parameter,wd);
              }
            }

            if (y>0) {
              // top
              int32_t otherHeight=data.srtmTile->GetHeight(x,y-1);
              if (CrossesElevationLine(ele,height,otherHeight)) {
                log.Info() << "Top " << x << "," << y
                           << " " << box.GetDisplayText()
                           << " " << std::abs(otherHeight-height);

                FeatureValueBuffer     buffer;
                WayData                wd;
                std::array<GeoCoord,2> path;

                path[0]=box.GetTopLeft();
                path[1]=box.GetTopRight();

                CoordBufferRange range=TransformWay(path,
                                                    transBuffer,
                                                    coordBuffer,
                                                    projection,
                                                    TransPolygon::none,
                                                    errorTolerancePixel);

                wd.buffer=&buffer;
                wd.layer=0;
                wd.lineStyle=contourLineStyles[0];
                wd.color=contourLineStyles[0]->GetLineColor();
                wd.wayPriority=std::numeric_limits<size_t>::max();
                wd.coordRange=range;
                wd.lineWidth=GetProjectedWidth(projection,
                                               projection.ConvertWidthToPixel(contourLineStyles[0]->GetDisplayWidth()),
                                               contourLineStyles[0]->GetWidth());
                wd.startIsClosed=false;
                wd.endIsClosed=false;

                DrawWay(*styleConfig,projection,parameter,wd);

              }
            }

            if (y<data.srtmTile->rows-1) {
              // bottom
              int32_t otherHeight=data.srtmTile->GetHeight(x,y+1);
              if (CrossesElevationLine(ele,height,otherHeight)) {
                log.Info() << "Bottom " << x << "," << y << " "
                  << box.GetDisplayText()
                  << " " << std::abs(otherHeight-height);

                FeatureValueBuffer     buffer;
                WayData                wd;
                std::array<GeoCoord,2> path;

                path[0]=box.GetBottomLeft();
                path[1]=box.GetBottomRight();

                CoordBufferRange range=TransformWay(path,
                                                    transBuffer,
                                                    coordBuffer,
                                                    projection,
                                                    TransPolygon::none,
                                                    errorTolerancePixel);

                wd.buffer=&buffer;
                wd.layer=0;
                wd.lineStyle=contourLineStyles[0];
                wd.color=contourLineStyles[0]->GetLineColor();
                wd.wayPriority=std::numeric_limits<size_t>::max();
                wd.coordRange=range;
                wd.lineWidth=GetProjectedWidth(projection,
                                               projection.ConvertWidthToPixel(contourLineStyles[0]->GetDisplayWidth()),
                                               contourLineStyles[0]->GetWidth());
                wd.startIsClosed=false;
                wd.endIsClosed=false;

                DrawWay(*styleConfig,projection,parameter,wd);

              }
            }
          }
        }
      }
    }

    if (false) {
      FeatureValueBuffer contourLinesBuffer;

      contourLinesBuffer.SetType(srtmType);

      std::vector<BorderStyleRef> areaBorderStyles;

      styleConfig->GetAreaBorderStyles(srtmType,
                                       contourLinesBuffer,
                                       projection,
                                       areaBorderStyles);

      if (areaBorderStyles.empty()) {
        log.Warn() << "Contour lines activated but no border style for type 'srtm_tile' found";
      }

      GeoBox mapBoundingBox=projection.GetDimensions();

      log.Info() << "Initial bounding box: " << mapBoundingBox.GetDisplayText();

      double minLat=RoundDown(mapBoundingBox.GetMinLat());
      double maxLat=RoundUp(mapBoundingBox.GetMaxLat());

      double minLon=RoundDown(mapBoundingBox.GetMinLon());
      double maxLon=RoundUp(mapBoundingBox.GetMaxLon());

      log.Info() << "SRTM bounding box: " << GeoBox(GeoCoord(minLat,
                                                             minLon),
                                                    GeoCoord(maxLat,
                                                             maxLon)).GetDisplayText();

      int minX=int(minLon);
      int minY=int(minLat);

      double factor=1.0/1201;

      bool even=true;

      for (int x=minX; x<int(maxLon); x++) {
        for (int y=minY; y<int(maxLat); y++) {
          for (int subX=0; subX<1201; subX++) {
            for (int subY=0; subY<1201; subY++) {
              even =!even;

              if (!even) {
                continue;
              }

              double           xDelta=subX*factor;
              double           yDelta=subY*factor;
              GeoBox           tileBoundingBox=GeoBox(GeoCoord(y+yDelta,
                                                                  x+xDelta),
                                            GeoCoord(y+yDelta+factor,
                                                        x+xDelta+factor));

              CoordBufferRange range=TransformBoundingBox(tileBoundingBox,
                                                          transBuffer,
                                                          coordBuffer,
                                                          projection,
                                                          TransPolygon::none,
                                                          errorTolerancePixel);

              for (const auto& borderStyle : areaBorderStyles) {
                AreaData tileData;

                tileData.borderStyle=borderStyle;
                tileData.boundingBox=tileBoundingBox;
                tileData.ref=ObjectFileRef();
                tileData.coordRange=range;

                DrawArea(projection,
                         parameter,
                         tileData);
              }
            }
          }
        }
      }
    }
  }

  void MapPainter::DrawHillShading(const Projection& projection,
                                   const MapParameter& parameter,
                                   const MapData& data)
  {
    if (!parameter.GetRenderHillShading()) {
      return;
    }

    log.Info() << "Draw hillshading";

    TypeInfoRef srtmType=styleConfig->GetTypeConfig()->GetTypeInfo("srtm_tile");

    if (!srtmType) {
      log.Warn() << "HillShading activated but no type 'srtm_tile' found";
      return;
    }

    FeatureValueBuffer hillShadingBuffer;

    hillShadingBuffer.SetType(srtmType);

    FillStyleRef hillShadingFill=styleConfig->GetAreaFillStyle(srtmType,
                                                               hillShadingBuffer,
                                                               projection);

    std::vector<TextStyleRef> textStyles;

    styleConfig->GetAreaTextStyles(srtmType,
                                   hillShadingBuffer,
                                   projection,
                                   textStyles);

    if (!hillShadingFill) {
      log.Warn() << "HillShading activated but no fill style for type 'srtm_tile' found";
    }

    if (textStyles.empty()) {
      log.Warn() << "HillShading activated but no text style for type 'srtm_tile' found";
    }

    GeoBox mapBoundingBox=projection.GetDimensions();

    log.Info() << "Initial bounding box: "  << mapBoundingBox.GetDisplayText();

    double minLat=RoundDown(mapBoundingBox.GetMinLat());
    double maxLat=RoundUp(mapBoundingBox.GetMaxLat());

    double minLon=RoundDown(mapBoundingBox.GetMinLon());
    double maxLon=RoundUp(mapBoundingBox.GetMaxLon());

    log.Info() << "SRTM bounding box: "  << GeoBox(GeoCoord(minLat,minLon),GeoCoord(maxLat,maxLon)).GetDisplayText();

    int minX=int(minLon);
    int minY=int(minLat);

    double factor=1.0/1201;

    bool even=true;
    for (int x=minX; x<int(maxLon); ++x) {
      for (int y=minY; y<int(maxLat); ++y) {
        for (int subX=0; subX<1201; ++subX) {
          for (int subY=0; subY<1201; ++subY) {
            even=!even;

            AreaData tileData;
            double   xDelta=subX*factor;
            double   yDelta=subY*factor;

            tileData.fillStyle  =hillShadingFill;
            tileData.boundingBox=GeoBox(GeoCoord(y+yDelta,x+xDelta),
                                        GeoCoord(y+yDelta+factor,x+xDelta+factor));

            if (!mapBoundingBox.Intersects(tileData.boundingBox)) {
              continue;
            }


            if (!textStyles.empty() && data.srtmTile) {
              GeoCoord center=tileData.boundingBox.GetCenter();
              Vertex2D screenPos;

              projection.GeoToPixel(center,
                                    screenPos);

              double  latitude =center.GetLat();
              double  longitude=center.GetLon();
              double  fracLat  =latitude>=0.0 ? 1-latitude+::floor(latitude) : ::ceil(latitude)-latitude;
              double  fracLon  =longitude>=0.0 ? longitude-::floor(longitude) : 1-::ceil(longitude)+longitude;
              int     col      =int(::floor(fracLon*data.srtmTile->columns));
              int     row      =int(::floor(fracLat*data.srtmTile->rows));
              int32_t height   =data.srtmTile->GetHeight(col,
                                                         row);

              LabelData labelBox;

              labelBox.priority=0;
              labelBox.alpha=1.0;
              labelBox.fontSize=textStyles[0]->GetSize();
              labelBox.style=textStyles[0];
              labelBox.text=std::to_string(height);

              std::vector<LabelData> vect;
              vect.push_back(labelBox);
              RegisterRegularLabel(projection,
                                   parameter,
                                   ObjectFileRef(),
                                   vect,
                                   screenPos,
                                   /*proposedWidth*/ -1);
            }

            // chess pattern
            if (!even) {
              continue;
            }

            if (hillShadingFill) {
              CoordBufferRange range=TransformBoundingBox(tileData.boundingBox,
                                                          transBuffer,
                                                          coordBuffer,
                                                          projection,
                                                          TransPolygon::none,
                                                          errorTolerancePixel);

              tileData.ref=ObjectFileRef();
              tileData.coordRange=range;

              DrawArea(projection,parameter,tileData);
            }
          }
        }
      }
    }
  }

  void MapPainter::Postrender(const Projection& projection,
                              const MapParameter& parameter,
                              const MapData& data)
  {
    AfterDrawing(*styleConfig,
                 projection,
                 parameter,
                 data);
  }
}
