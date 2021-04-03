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

#include <osmscout/MapPainter.h>

#include <cstdint>

#include <algorithm>
#include <iostream>
#include <limits>

#include <osmscout/system/Math.h>

#include <osmscout/util/Logger.h>
#include <osmscout/util/StopClock.h>
#include <osmscout/util/String.h>
#include <osmscout/util/Tiling.h>

//#define DEBUG_GROUNDTILES
//#define DEBUG_NODE_DRAW

namespace osmscout {

  static std::set<GeoCoord> GetGridPoints(const std::vector<Point>& nodes,
                            double gridSizeHoriz,
                            double gridSizeVert)
  {
    assert(nodes.size()>=2);

    std::set<GeoCoord> intersections;

    for (size_t i=0; i<nodes.size()-1; i++) {
      size_t cellXStart=(size_t)((nodes[i].GetLon()+180.0)/gridSizeHoriz);
      size_t cellYStart=(size_t)((nodes[i].GetLat()+90.0)/gridSizeVert);

      size_t cellXEnd=(size_t)((nodes[i+1].GetLon()+180.0)/gridSizeHoriz);
      size_t cellYEnd=(size_t)((nodes[i+1].GetLat()+90.0)/gridSizeVert);

      if (cellXStart!=cellXEnd) {
        double lower=std::min(cellYStart,cellYEnd)*gridSizeVert-90.0;
        double upper=(std::max(cellYStart,cellYEnd)+1)*gridSizeVert-90.0;

        for (size_t xIndex=cellXStart+1; xIndex<=cellXEnd; xIndex++) {
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

        for (size_t yIndex=cellYStart+1; yIndex<=cellYEnd; yIndex++) {
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
  static bool AreaSorter(const MapPainter::AreaData& a, const MapPainter::AreaData& b)
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
  static inline bool LabelLayoutDataSorter(const LabelData& a,
                                           const LabelData& b)
  {
    return a.position<b.position;
  }

  MapPainter::MapPainter(const StyleConfigRef& styleConfig,
                         CoordBuffer *buffer)
  : coordBuffer(buffer),
    styleConfig(styleConfig),
    transBuffer(coordBuffer),
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

    TextStyleRef textStyle=std::make_shared<TextStyle>();

    textStyle->SetStyle(TextStyle::normal);
    textStyle->SetPriority(0);
    textStyle->SetTextColor(Color(0,0,0,0.9));
    textStyle->SetSize(0.7);

    debugLabel=textStyle;

    stepMethods.resize(RenderSteps::LastStep-RenderSteps::FirstStep+1);

    stepMethods[RenderSteps::Initialize]=&MapPainter::InitializeRender;
    stepMethods[RenderSteps::DumpStatistics]=&MapPainter::DumpStatistics;
    stepMethods[RenderSteps::PreprocessData]=&MapPainter::PreprocessData;
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

  void MapPainter::DumpDataStatistics(const Projection& projection,
                                      const MapParameter& parameter,
                                      const MapData& data)
  {
    std::unordered_map<TypeInfoRef,DataStatistic> statistics;
    TypeInfoSet                                   types;

    // Now analyse the actual data

    for (const auto& node : data.nodes) {
      DataStatistic& entry=statistics[node->GetType()];

      entry.nodeCount++;
      entry.coordCount++;

      if (parameter.IsDebugData()) {
        IconStyleRef iconStyle=styleConfig->GetNodeIconStyle(node->GetFeatureValueBuffer(),
                                                             projection);

        styleConfig->GetNodeTextStyles(node->GetFeatureValueBuffer(),
                                       projection,
                                       textStyles);

        entry.labelCount+=textStyles.size();

        if (iconStyle) {
          entry.iconCount++;
        }
      }
    }

    for (const auto& node : data.poiNodes) {
      DataStatistic& entry=statistics[node->GetType()];

      entry.nodeCount++;
      entry.coordCount++;

      if (parameter.IsDebugData()) {
        IconStyleRef iconStyle=styleConfig->GetNodeIconStyle(node->GetFeatureValueBuffer(),
                                                             projection);

        styleConfig->GetNodeTextStyles(node->GetFeatureValueBuffer(),
                                       projection,
                                       textStyles);

        entry.labelCount+=textStyles.size();

        if (iconStyle) {
          entry.iconCount++;
        }
      }
    }

    for (const auto& way : data.ways) {
      DataStatistic& entry=statistics[way->GetType()];

      entry.wayCount++;
      entry.coordCount+=way->nodes.size();

      if (parameter.IsDebugData()) {
        PathShieldStyleRef shieldStyle=styleConfig->GetWayPathShieldStyle(way->GetFeatureValueBuffer(),
                                                                          projection);
        PathTextStyleRef   pathTextStyle=styleConfig->GetWayPathTextStyle(way->GetFeatureValueBuffer(),
                                                                          projection);

        if (shieldStyle) {
          entry.labelCount++;
        }

        if (pathTextStyle) {
          entry.labelCount++;
        }
      }
    }

    for (const auto& way : data.poiWays) {
      DataStatistic& entry=statistics[way->GetType()];

      entry.wayCount++;
      entry.coordCount+=way->nodes.size();

      if (parameter.IsDebugData()) {
        PathShieldStyleRef shieldStyle=styleConfig->GetWayPathShieldStyle(way->GetFeatureValueBuffer(),
                                                                          projection);
        PathTextStyleRef   pathTextStyle=styleConfig->GetWayPathTextStyle(way->GetFeatureValueBuffer(),
                                                                          projection);

        if (shieldStyle) {
          entry.labelCount++;
        }

        if (pathTextStyle) {
          entry.labelCount++;
        }
      }
    }

    for (const auto& area : data.areas) {
      DataStatistic& entry=statistics[area->GetType()];

      entry.areaCount++;

      for (const auto& ring : area->rings) {
        entry.coordCount+=ring.nodes.size();

        if (parameter.IsDebugData()) {
          if (ring.IsMaster()) {
            IconStyleRef iconStyle=styleConfig->GetAreaIconStyle(area->GetType(),
                                                                 ring.GetFeatureValueBuffer(),
                                                                 projection);

            styleConfig->GetAreaTextStyles(area->GetType(),
                                           ring.GetFeatureValueBuffer(),
                                           projection,
                                           textStyles);
            if (iconStyle) {
              entry.iconCount++;
            }

            entry.labelCount+=textStyles.size();
          }
        }
      }
    }

    for (const auto& area : data.poiAreas) {
      DataStatistic& entry=statistics[area->GetType()];

      entry.areaCount++;

      for (const auto& ring : area->rings) {
        entry.coordCount+=ring.nodes.size();

        if (parameter.IsDebugData()) {
          if (ring.IsMaster()) {
            IconStyleRef iconStyle=styleConfig->GetAreaIconStyle(area->GetType(),
                                                                 ring.GetFeatureValueBuffer(),
                                                                 projection);

            styleConfig->GetAreaTextStyles(area->GetType(),
                                           ring.GetFeatureValueBuffer(),
                                           projection,
                                           textStyles);
            if (iconStyle) {
              entry.iconCount++;
            }

            entry.labelCount+=textStyles.size();
          }
        }
      }
    }

    for (auto& entry : statistics) {
      entry.second.objectCount=entry.second.nodeCount+entry.second.wayCount+entry.second.areaCount;

      if (entry.first) {
        if (parameter.GetWarningObjectCountLimit()>0 &&
          entry.second.objectCount>parameter.GetWarningObjectCountLimit()) {
          log.Warn() << "Type : " << entry.first->GetName() << " has " << entry.second.objectCount << " objects (performance limit: " << parameter.GetWarningObjectCountLimit() << ")";
        }

        if (parameter.GetWarningCoordCountLimit()>0 &&
            entry.second.coordCount>parameter.GetWarningCoordCountLimit()) {
          log.Warn() << "Type : " << entry.first->GetName() << " has " << entry.second.coordCount << " coords (performance limit: " << parameter.GetWarningCoordCountLimit() << ")";
        }

      }
    }

    if (parameter.IsDebugData()) {
      std::list<DataStatistic> statisticList;

      for (auto& entry : statistics) {
        entry.second.type=entry.first;

        statisticList.push_back(entry.second);
      }

      statistics.clear();

      statisticList.sort([](const DataStatistic& a,
                            const DataStatistic& b)->bool {return a.objectCount>b.objectCount;});

      log.Info() << "Type|ObjectCount|NodeCount|WayCount|AreaCount|Nodes|Labels|Icons";
      for (const auto& entry : statisticList) {
        log.Info() << entry.type->GetName() << " "
                   << entry.objectCount << " "
                   << entry.nodeCount << " " << entry.wayCount << " " << entry.areaCount << " "
                   << entry.coordCount << " "
                   << entry.labelCount << " "
                   << entry.iconCount;
      }
    }
  }

  bool MapPainter::IsVisibleArea(const Projection& projection,
                                 const GeoBox& boundingBox,
                                 double pixelOffset) const
  {
    if (!boundingBox.IsValid()){
      return false;
    }

    double x;
    double y;

    projection.GeoToPixel(boundingBox.GetMinCoord(),
                          x,
                          y);

    double xMin=x;
    double xMax=x;
    double yMin=y;
    double yMax=y;

    projection.GeoToPixel(boundingBox.GetMaxCoord(),
                          x,
                          y);

    xMin=std::min(xMin,x);
    xMax=std::max(xMax,x);
    yMin=std::min(yMin,y);
    yMax=std::max(yMax,y);

    projection.GeoToPixel(GeoCoord(boundingBox.GetMinLat(),
                                   boundingBox.GetMaxLon()),
                          x,
                          y);

    xMin=std::min(xMin,x);
    xMax=std::max(xMax,x);
    yMin=std::min(yMin,y);
    yMax=std::max(yMax,y);

    projection.GeoToPixel(GeoCoord(boundingBox.GetMaxLat(),
                                   boundingBox.GetMinLon()),
                          x,
                          y);

    xMin=std::min(xMin,x);
    xMax=std::max(xMax,x);
    yMin=std::min(yMin,y);
    yMax=std::max(yMax,y);

    xMin=xMin-pixelOffset;
    xMax=xMax+pixelOffset;
    yMin=yMin-pixelOffset;
    yMax=yMax+pixelOffset;

    if (xMax-xMin<=areaMinDimension &&
        yMax-yMin<=areaMinDimension) {
      return false;
    }

    return !(xMin>=projection.GetWidth() ||
             yMin>=projection.GetHeight() ||
             xMax<0 ||
             yMax<0);
  }

  bool MapPainter::IsVisibleWay(const Projection& projection,
                                const GeoBox& boundingBox,
                                double pixelOffset) const
  {
    if (!boundingBox.IsValid()){
      return false;
    }

    double x;
    double y;

    projection.GeoToPixel(boundingBox.GetMinCoord(),
                          x,
                          y);

    double xMin=x;
    double xMax=x;
    double yMin=y;
    double yMax=y;

    projection.GeoToPixel(boundingBox.GetMaxCoord(),
                          x,
                          y);

    xMin=std::min(xMin,x);
    xMax=std::max(xMax,x);
    yMin=std::min(yMin,y);
    yMax=std::max(yMax,y);

    projection.GeoToPixel(GeoCoord(boundingBox.GetMinLat(),
                                   boundingBox.GetMaxLon()),
                          x,
                          y);

    xMin=std::min(xMin,x);
    xMax=std::max(xMax,x);
    yMin=std::min(yMin,y);
    yMax=std::max(yMax,y);

    projection.GeoToPixel(GeoCoord(boundingBox.GetMaxLat(),
                                   boundingBox.GetMinLon()),
                          x,
                          y);

    xMin=std::min(xMin,x);
    xMax=std::max(xMax,x);
    yMin=std::min(yMin,y);
    yMax=std::max(yMax,y);

    xMin=xMin-pixelOffset;
    xMax=xMax+pixelOffset;
    yMin=yMin-pixelOffset;
    yMax=yMax+pixelOffset;

    return !(xMin>=projection.GetWidth() ||
             yMin>=projection.GetHeight() ||
             xMax<0 ||
             yMax<0);
  }

  void MapPainter::Transform(const Projection& projection,
                             const MapParameter& /*parameter*/,
                             const GeoCoord& coord,
                             double& x,
                             double& y)
  {
    projection.GeoToPixel(coord,
                          x,y);
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
    // No code
  }

  void MapPainter::BeforeDrawing(const StyleConfig& /*styleConfig*/,
                                 const Projection& /*projection*/,
                                 const MapParameter& /*parameter*/,
                                 const MapData& /*data*/)
  {

  }

  void MapPainter::AfterDrawing(const StyleConfig& /*styleConfig*/,
                                const Projection& /*projection*/,
                                const MapParameter& /*parameter*/,
                                const MapData& /*data*/)
  {

  }

  void MapPainter::RegisterPointWayLabel(const Projection& projection,
                                         const MapParameter& parameter,
                                         const PathShieldStyleRef& shieldStyle,
                                         const std::string& text,
                                         const std::vector<Point>& nodes)
  {
    const LabelStyleRef& style=shieldStyle->GetShieldStyle();
    std::set<GeoCoord> gridPoints=GetGridPoints(nodes,
                                                shieldGridSizeHoriz,
                                                shieldGridSizeVert);

    for (const auto& gridPoint : gridPoints) {
      double x,y;

      projection.GeoToPixel(gridPoint,x,y);

      LabelData labelBox;

      labelBox.priority=style->GetPriority();
      labelBox.alpha=1.0;
      labelBox.fontSize=style->GetSize();
      labelBox.style=style;
      labelBox.text=text;

      std::vector<LabelData> vect;
      vect.push_back(labelBox);
      RegisterRegularLabel(projection, parameter, vect, Vertex2D(x,y), /*proposedWidth*/ -1);
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
   * @param x
   *    X position to place the label at (currently always the center of the area or the coordinate of the node)
   * @param y
   *    Y position to place the label at (currently always the center of the area or the coordinate of the node)
   * @param objectWidth
   *    The (rough) width of the object
   * @param objectHeight
   *    The (rough) height of the object
   */
  void MapPainter::LayoutPointLabels(const Projection& projection,
                                     const MapParameter& parameter,
                                     const FeatureValueBuffer& buffer,
                                     const IconStyleRef& iconStyle,
                                     const std::vector<TextStyleRef>& textStyles,
                                     double x,
                                     double y,
                                     double objectWidth,
                                     double objectHeight)
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
      } else if (iconStyle->GetSymbol()) {
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
        data.fontSize=textStyle->GetSize()*pow(1.5,factor);

        data.alpha=std::min(textStyle->GetAlpha()/factor, 1.0);
      }
      else if (textStyle->GetAutoSize()) {
        double height=std::abs((objectHeight)*0.1);

        if (height==0 || height<standardFontSize) {
          continue;
        }

        // Retricts the height of a label to maxHeight
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

    RegisterRegularLabel(projection, parameter, labelLayoutData, Vertex2D(x,y), objectWidth);
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
#if defined(DEBUG_NODE_DRAW)
    std::vector<double> times;

    times.resize(styleConfig.GetTypeConfig()->GetMaxTypeId()+1,0.0);
#endif

    for (const auto& node : data.nodes) {
#if defined(DEBUG_NODE_DRAW)
      StopClockNano nodeTimer;
#endif

      PrepareNode(styleConfig,
                  projection,
                  parameter,
                  node);

#if defined(DEBUG_NODE_DRAW)
      nodeTimer.Stop();

      times[node->GetType()->GetNodeId()]+=nodeTimer.GetNanoseconds();
#endif
    }

    for (const auto& node : data.poiNodes) {
#if defined(DEBUG_NODE_DRAW)
      StopClockNano nodeTimer;
#endif

      PrepareNode(styleConfig,
                  projection,
                  parameter,
                  node);

#if defined(DEBUG_NODE_DRAW)
      nodeTimer.Stop();

      times[node->GetType()->GetNodeId()]+=nodeTimer.GetNanoseconds();
#endif
    }

#if defined(DEBUG_NODE_DRAW)
    for (auto type : styleConfig.GetTypeConfig()->GetTypes())
    {
      double overallTime=times[type->GetNodeId()];

      if (overallTime>0.0) {
        std::cout << "Node type " << type->GetName() << " " << times[type->GetNodeId()] << " nsecs" << std::endl;
      }
    }
#endif
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

    double x1;
    double x2;
    double y1;
    double y2;

    projection.GeoToPixel(areaData.boundingBox.GetMinCoord(),
                          x1,y1);

    projection.GeoToPixel(areaData.boundingBox.GetMaxCoord(),
                          x2,y2);

    LayoutPointLabels(projection,
                      parameter,
                      *areaData.buffer,
                      iconStyle,
                      textStyles,
                      (x1+x2)/2,
                      (y1+y2)/2,
                      std::max(x1, x2) - std::min(x1, x2),
                      std::max(y1, y2) - std::min(y1, y2));
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

    double      lineOffset=0.0;
    size_t      transStart=areaData.transStart;
    size_t      transEnd=areaData.transEnd;
    std::string label=borderTextStyle->GetLabel()->GetLabel(parameter,
                                                            *areaData.buffer);

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
      coordBuffer->GenerateParallelWay(transStart,
                                       transEnd,
                                       lineOffset,
                                       transStart,
                                       transEnd);
    }

    // TODO: use coordBuffer for label path
    LabelPath labelPath;

    for (size_t j=transStart; j<=transEnd; j++) {
      labelPath.AddPoint(
          coordBuffer->buffer[j].GetX(),
          coordBuffer->buffer[j].GetY());
    }

    PathLabelData labelData;
    labelData.priority=borderTextStyle->GetPriority();
    labelData.style=borderTextStyle;
    labelData.text=label;
    labelData.contourLabelOffset=contourLabelOffset;
    labelData.contourLabelSpace=contourLabelSpace;

    RegisterContourLabel(projection, parameter, labelData, labelPath);

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

    double lineOffset=0.0;
    size_t transStart=areaData.transStart;
    size_t transEnd=areaData.transEnd;
    double symbolSpace=projection.ConvertWidthToPixel(borderSymbolStyle->GetSymbolSpace());

    if (borderSymbolStyle->GetOffset()!=0.0) {
      lineOffset+=GetProjectedWidth(projection,
                                    borderSymbolStyle->GetOffset());
    }

    if (borderSymbolStyle->GetDisplayOffset()!=0.0) {
      lineOffset+=projection.ConvertWidthToPixel(borderSymbolStyle->GetDisplayOffset());
    }

    if (lineOffset!=0.0) {
      coordBuffer->GenerateParallelWay(transStart,
                                       transEnd,
                                       lineOffset,
                                       transStart,
                                       transEnd);
    }

    DrawContourSymbol(projection,
                      parameter,
                      *borderSymbolStyle->GetSymbol(),
                      symbolSpace,
                      transStart,transEnd);

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

    double x,y;

    Transform(projection,
              parameter,
              node->GetCoords(),
              x,y);

    LayoutPointLabels(projection,
                      parameter,
                      node->GetFeatureValueBuffer(),
                      iconStyle,
                      textStyles,
                      x,y);
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
               data.transStart,data.transEnd);
    }

    DrawPath(projection,
             parameter,
             data.color,
             data.lineWidth,
             data.lineStyle->GetDash(),
             data.startIsClosed ? data.lineStyle->GetEndCap() : data.lineStyle->GetJoinCap(),
             data.endIsClosed ? data.lineStyle->GetEndCap() : data.lineStyle->GetJoinCap(),
             data.transStart,data.transEnd);
  }

  bool MapPainter::DrawWayDecoration(const StyleConfig& styleConfig,
                                     const Projection& projection,
                                     const MapParameter& parameter,
                                     const MapPainter::WayPathData& data)
  {
    std::vector<PathSymbolStyleRef> symbolStyles;
    styleConfig.GetWayPathSymbolStyle(*data.buffer,
                                      projection,
                                      symbolStyles);

    if (symbolStyles.empty()) {
      return false;
    }

    LanesFeatureValue  *lanesValue=nullptr;
    std::vector<OffsetRel> laneTurns; // cached turns

    for (const auto &pathSymbolStyle : symbolStyles) {
      double lineOffset = 0.0;
      size_t transStart = data.transStart;
      size_t transEnd = data.transEnd;
      double symbolSpace = projection.ConvertWidthToPixel(pathSymbolStyle->GetSymbolSpace());

      if (pathSymbolStyle->GetOffsetRel() == OffsetRel::laneForwardLeft ||
          pathSymbolStyle->GetOffsetRel() == OffsetRel::laneForwardThroughLeft ||
          pathSymbolStyle->GetOffsetRel() == OffsetRel::laneForwardThrough ||
          pathSymbolStyle->GetOffsetRel() == OffsetRel::laneForwardThroughRight ||
          pathSymbolStyle->GetOffsetRel() == OffsetRel::laneForwardRight ||
          pathSymbolStyle->GetOffsetRel() == OffsetRel::laneBackwardLeft ||
          pathSymbolStyle->GetOffsetRel() == OffsetRel::laneBackwardThroughLeft ||
          pathSymbolStyle->GetOffsetRel() == OffsetRel::laneBackwardThrough ||
          pathSymbolStyle->GetOffsetRel() == OffsetRel::laneBackwardThroughRight ||
          pathSymbolStyle->GetOffsetRel() == OffsetRel::laneBackwardRight ) {

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
            coordBuffer->GenerateParallelWay(data.transStart,data.transEnd,
                                             laneOffset,
                                             transStart,transEnd);

            DrawContourSymbol(projection,
                              parameter,
                              *pathSymbolStyle->GetSymbol(),
                              symbolSpace,
                              transStart, transEnd);
          }
          laneOffset+=lanesSpace;
        }
      } else {

        if (pathSymbolStyle->GetOffset() != 0.0) {
          lineOffset += GetProjectedWidth(projection,
                                          pathSymbolStyle->GetOffset());
        }

        if (pathSymbolStyle->GetDisplayOffset() != 0.0) {
          lineOffset += projection.ConvertWidthToPixel(pathSymbolStyle->GetDisplayOffset());
        }

        if (lineOffset != 0.0) {
          coordBuffer->GenerateParallelWay(data.transStart,data.transEnd,
                                           lineOffset,
                                           transStart,transEnd);
        }

        DrawContourSymbol(projection,
                          parameter,
                          *pathSymbolStyle->GetSymbol(),
                          symbolSpace,
                          transStart, transEnd);
      }
    }
    return true;
  }

  bool MapPainter::CalculateWayShieldLabels(const StyleConfig& styleConfig,
                                            const Projection& projection,
                                            const MapParameter& parameter,
                                            const Way& data)
  {
    PathShieldStyleRef shieldStyle=styleConfig.GetWayPathShieldStyle(data.GetFeatureValueBuffer(),
                                                                     projection);

    if (!shieldStyle) {
      return false;
    }

    std::string shieldLabel=shieldStyle->GetLabel()->GetLabel(parameter,
                                                              data.GetFeatureValueBuffer());

    if (shieldLabel.empty()) {
      return false;
    }

    RegisterPointWayLabel(projection,
                          parameter,
                          shieldStyle,
                          shieldLabel,
                          data.nodes);

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
                                       const std::string &textLabel)
  {
    assert(pathTextStyle);

    double      lineOffset=0.0;
    size_t      transStart=data.transStart;
    size_t      transEnd=data.transEnd;

    if (pathTextStyle->GetOffset()!=0.0) {
      lineOffset+=GetProjectedWidth(projection,
                                    pathTextStyle->GetOffset());
    }

    if (pathTextStyle->GetDisplayOffset()!=0.0) {
      lineOffset+=projection.ConvertWidthToPixel(pathTextStyle->GetDisplayOffset());
    }

    if (lineOffset!=0.0) {
      coordBuffer->GenerateParallelWay(transStart,
                                       transEnd,
                                       lineOffset,
                                       transStart,
                                       transEnd);
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

    for (size_t j=transStart; j<=transEnd; j++) {
      labelPath.AddPoint(
          coordBuffer->buffer[j].GetX(),
          coordBuffer->buffer[j].GetY());
    }
    RegisterContourLabel(projection, parameter, labelData, labelPath);

    return true;
  }

  void MapPainter::DrawOSMTileGrid(const Projection& projection,
                                   const MapParameter& parameter,
                                   const Magnification& magnification,
                                   const LineStyleRef& osmTileLine)
  {
    GeoBox boundingBox;

    projection.GetDimensions(boundingBox);

    osmscout::OSMTileId     tileA(OSMTileId::GetOSMTile(magnification,
                                                        boundingBox.GetMinCoord()));
    osmscout::OSMTileId     tileB(OSMTileId::GetOSMTile(magnification,
                                                        boundingBox.GetMaxCoord()));
    uint32_t                startTileX=std::min(tileA.GetX(),tileB.GetX());
    uint32_t                endTileX=std::max(tileA.GetX(),tileB.GetX());
    uint32_t                startTileY=std::min(tileA.GetY(),tileB.GetY());
    uint32_t                endTileY=std::max(tileA.GetY(),tileB.GetY());

    if (startTileX>0) {
      startTileX--;
    }
    if (startTileY>0) {
      startTileY--;
    }

    std::vector<Point> points;

    // Horizontal lines

    for (uint32_t y=startTileY; y<=endTileY; y++) {
      points.resize(endTileX-startTileX+1);

      for (uint32_t x=startTileX; x<=endTileX; x++) {
        points[x-startTileX].Set(0,OSMTileId(x,y).GetTopLeftCoord(magnification));
      }

      size_t transStart;
      size_t transEnd;

      transBuffer.TransformWay(projection,
                               parameter.GetOptimizeWayNodes(),
                               points,
                               transStart,
                               transEnd,
                               errorTolerancePixel);

      WayData data;

      data.buffer=&coastlineSegmentAttributes;
      data.layer=0;
      data.lineStyle=osmTileLine;
      data.color=osmTileLine->GetLineColor();
      data.wayPriority=std::numeric_limits<size_t>::max();
      data.transStart=transStart;
      data.transEnd=transEnd;
      data.lineWidth=GetProjectedWidth(projection,
                                       projection.ConvertWidthToPixel(osmTileLine->GetDisplayWidth()),
                                       osmTileLine->GetWidth());
      data.startIsClosed=false;
      data.endIsClosed=false;
      wayData.push_back(data);
    }

    // Vertical lines

    for (uint32_t x=startTileX; x<=endTileX; x++) {
      points.resize(endTileY-startTileY+1);

      for (uint32_t y=startTileY; y<=endTileY; y++) {
        points[y-startTileY].Set(0,OSMTileId(x,y).GetTopLeftCoord(magnification));
      }

      size_t transStart;
      size_t transEnd;

      transBuffer.TransformWay(projection,
                               parameter.GetOptimizeWayNodes(),
                               points,
                               transStart,
                               transEnd,
                               errorTolerancePixel);

      WayData data;

      data.buffer=&coastlineSegmentAttributes;
      data.layer=0;
      data.lineStyle=osmTileLine;
      data.color=osmTileLine->GetLineColor();
      data.wayPriority=std::numeric_limits<size_t>::max();
      data.transStart=transStart;
      data.transEnd=transEnd;
      data.lineWidth=GetProjectedWidth(projection,
                                       projection.ConvertWidthToPixel(osmTileLine->GetDisplayWidth()),
                                       osmTileLine->GetWidth());
      data.startIsClosed=false;
      data.endIsClosed=false;
      wayData.push_back(data);
    }
  }

  void MapPainter::PrepareArea(const StyleConfig& styleConfig,
                               const Projection& projection,
                               const MapParameter& parameter,
                               const AreaRef &area)
  {
    std::vector<PolyData> td(area->rings.size());

    for (size_t i=0; i<area->rings.size(); i++) {
      const Area::Ring &ring = area->rings[i];
      // The master ring does not have any nodes, so we skip it
      // Rings with less than 3 nodes should be skipped, too (no area)
      if (ring.IsMaster() || ring.nodes.size() < 3) {
        td[i].transStart=0;
        td[i].transEnd=0;
        continue;
      }

      if (ring.segments.size() <= 1){
        transBuffer.TransformArea(projection,
                                  parameter.GetOptimizeAreaNodes(),
                                  ring.nodes,
                                  td[i].transStart,td[i].transEnd,
                                  errorTolerancePixel);
      }else{
        std::vector<Point> nodes;
        for (const auto &segment:ring.segments){
          if (projection.GetDimensions().Intersects(segment.bbox, false)){
            // TODO: add TransBuffer::Transform* methods with vector subrange (begin/end)
            nodes.insert(nodes.end(), ring.nodes.data() + segment.from, ring.nodes.data() + segment.to);
          } else {
            nodes.push_back(ring.nodes[segment.from]);
            nodes.push_back(ring.nodes[segment.to-1]);
          }
        }
        transBuffer.TransformArea(projection,
                                  parameter.GetOptimizeAreaNodes(),
                                  nodes,
                                  td[i].transStart,td[i].transEnd,
                                  errorTolerancePixel);
      }
    }

    area->VisitRings([&](size_t i, const Area::Ring &ring, const TypeInfoRef &type) -> bool {

      if (type->GetIgnore()) {
        // clipping inner ring, we will not render it, but still go deeper,
        // there may be nested outer rings
        return true;
      }
      if (td[i].transStart==td[i].transEnd) {
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
        return false;
      }

      size_t borderStyleIndex=0;

      if (!borderStyles.empty() &&
          borderStyles.front()->GetDisplayOffset()==0.0 &&
          borderStyles.front()->GetOffset()==0.0) {
        borderStyle=borderStyles[borderStyleIndex];
        borderStyleIndex++;
      }

      AreaData a;
      double   borderWidth=borderStyle ? borderStyle->GetWidth() : 0.0;

      a.boundingBox=ring.GetBoundingBox();
      a.isOuter = ring.IsOuter();

      if (!IsVisibleArea(projection,
                         a.boundingBox,
                         borderWidth/2.0)) {
        return false;
      }

      // Collect possible clippings. We only take into account inner rings of the next level
      // that do not have a type and thus act as a clipping region. If a inner ring has a type,
      // we currently assume that it does not have alpha and paints over its region and clipping is
      // not required.
      area->VisitClippingRings(i, [&a, &td](size_t j, const Area::Ring &, const TypeInfoRef &type) -> bool {
        if (type->GetIgnore() && td[j].transStart < td[j].transEnd) {
          a.clippings.push_back(td[j]);
        }
        return true;
      });

      a.ref=area->GetObjectFileRef();
      a.type=type;
      a.buffer=&ring.GetFeatureValueBuffer();
      a.fillStyle=fillStyle;
      a.borderStyle=borderStyle;
      a.transStart=td[i].transStart;
      a.transEnd=td[i].transEnd;

      areaData.push_back(a);

      for (size_t idx=borderStyleIndex;
           idx<borderStyles.size();
           idx++) {
        borderStyle=borderStyles[idx];

        double offset=0.0;

        size_t transStart=td[i].transStart;
        size_t transEnd=td[i].transEnd;

        if (borderStyle->GetOffset()!=0.0) {
          offset+=GetProjectedWidth(projection,
                                    borderStyle->GetOffset());
        }

        if (borderStyle->GetDisplayOffset()!=0.0) {
          offset+=projection.ConvertWidthToPixel(borderStyle->GetDisplayOffset());
        }

        if (offset!=0.0) {
          coordBuffer->GenerateParallelWay(transStart,
                                           transEnd,
                                           offset,
                                           transStart,
                                           transEnd);
        }

        a.ref=area->GetObjectFileRef();
        a.type=type;
        a.buffer=nullptr;
        a.fillStyle=nullptr;
        a.borderStyle=borderStyle;
        a.transStart=transStart;
        a.transEnd=transEnd;

        areaData.push_back(a);
      }
      return true;
    });
  }

  void MapPainter::PrepareAreas(const StyleConfig& styleConfig,
                                const Projection& projection,
                                const MapParameter& parameter,
                                const MapData& data)
  {
    areaData.clear();

    //Areas
    for (const auto& area : data.areas) {
      PrepareArea(styleConfig,
                   projection,
                   parameter,
                   area);
    }

    areaData.sort(AreaSorter);

    // POI Areas
    for (const auto& area : data.poiAreas) {
      PrepareArea(styleConfig,
                  projection,
                  parameter,
                  area);
    }
  }

  std::vector<OffsetRel> MapPainter::ParseLaneTurns(const LanesFeatureValue &lanesValue)
  {
    std::vector<OffsetRel> laneTurns;
    laneTurns.reserve(lanesValue.GetLanes());

    // backward
    auto turns=SplitString(lanesValue.GetTurnBackward(), "|");
    int lanes=lanesValue.GetBackwardLanes();
    int lane=0;
    for (const std::string &turn: turns) {
      if (lane>=lanes){
        break;
      }
      if (turn == "left" || turn == "merge_to_left" || turn == "slight_left" || turn == "sharp_left") {
        laneTurns.push_back(OffsetRel::laneBackwardLeft);
      } else if (turn == "through;left" || turn == "through;slight_left" || turn == "through;sharp_left") {
        laneTurns.push_back(OffsetRel::laneBackwardThroughLeft);
      } else if (turn == "through") {
        laneTurns.push_back(OffsetRel::laneBackwardThrough);
      } else if (turn == "through;right" || turn == "through;slight_right" || turn == "through;sharp_right") {
        laneTurns.push_back(OffsetRel::laneBackwardThroughRight);
      } else if (turn == "right" || turn == "merge_to_right" || turn == "slight_right" || turn == "sharp_right") {
        laneTurns.push_back(OffsetRel::laneBackwardRight);
      } else {
        laneTurns.push_back(OffsetRel::base);
      }
      lane++;
    }
    for (;lane<lanes;lane++){
      laneTurns.push_back(OffsetRel::base);
    }

    // forward
    turns=SplitString(lanesValue.GetTurnForward(), "|");
    lanes=lanesValue.GetForwardLanes();
    lane=0;
    for (const std::string &turn: turns) {
      if (lane>=lanes){
        break;
      }
      if (turn == "left" || turn == "merge_to_left" || turn == "slight_left" || turn == "sharp_left") {
        laneTurns.push_back(OffsetRel::laneForwardLeft);
      } else if (turn == "through;left" || turn == "through;slight_left" || turn == "through;sharp_left") {
        laneTurns.push_back(OffsetRel::laneForwardThroughLeft);
      } else if (turn == "through") {
        laneTurns.push_back(OffsetRel::laneForwardThrough);
      } else if (turn == "through;right" || turn == "through;slight_right" || turn == "through;sharp_right") {
        laneTurns.push_back(OffsetRel::laneForwardThroughRight);
      } else if (turn == "right" || turn == "merge_to_right" || turn == "slight_right" || turn == "sharp_right") {
        laneTurns.push_back(OffsetRel::laneForwardRight);
      } else {
        laneTurns.push_back(OffsetRel::base);
      }
      lane++;
    }
    for (;lane<lanes;lane++){
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
      transBuffer.TransformWay(projection,
                               parameter.GetOptimizeWayNodes(),
                               way.nodes,
                               pathData.transStart,
                               pathData.transEnd,
                               errorTolerancePixel);
    }
    else {
      std::vector<Point> nodes;
      for (const auto &segment : way.segments){
        if (projection.GetDimensions().Intersects(segment.bbox, false)){
          // TODO: add TransBuffer::Transform* methods with vector subrange (begin/end)
          nodes.insert(nodes.end(), way.nodes.data() + segment.from, way.nodes.data() + segment.to);
        } else {
          nodes.push_back(way.nodes[segment.from]);
          nodes.push_back(way.nodes[segment.to-1]);
        }
      }
      transBuffer.TransformWay(projection,
                               parameter.GetOptimizeWayNodes(),
                               nodes,
                               pathData.transStart,
                               pathData.transEnd,
                               errorTolerancePixel);
    }
  }

  void MapPainter::CalculatePaths(const StyleConfig& styleConfig,
                                  const Projection& projection,
                                  const MapParameter& parameter,
                                  const Way& way)
  {
    FileOffset ref = way.GetFileOffset();
    const FeatureValueBuffer &buffer = way.GetFeatureValueBuffer();

    styleConfig.GetWayLineStyles(buffer,
                             projection,
                             lineStyles);

    if (lineStyles.empty()) {
      return;
    }

    WayPathData pathData;

    pathData.ref=ref;
    pathData.buffer=&buffer;
    pathData.transStart=0; // Make the compiler happy
    pathData.transEnd=0;   // Make the compiler happy
    pathData.mainSlotWidth=0.0;
    bool transformed=false;
    AccessFeatureValue *accessValue=nullptr;
    LanesFeatureValue  *lanesValue=nullptr;
    std::vector<OffsetRel> laneTurns; // cached turns

    for (const auto& lineStyle : lineStyles) {
      double       lineWidth=0.0;
      double       lineOffset=0.0;

      if (lineStyle->GetWidth()>0.0) {
        WidthFeatureValue *widthValue=widthReader.GetValue(buffer);


        if (widthValue!=nullptr) {
          lineWidth+=GetProjectedWidth(projection,
                                       widthValue->GetWidth());
        }
        else {
          lineWidth+=GetProjectedWidth(projection,
                                       lineStyle->GetWidth());
        }
      }

      if (lineStyle->GetDisplayWidth()>0.0) {
        lineWidth+=projection.ConvertWidthToPixel(lineStyle->GetDisplayWidth());
      }

      if (lineStyle->GetSlot().empty()) {
        pathData.mainSlotWidth=lineWidth;
      }

      if (lineWidth==0.0) {
        continue;
      }

      switch (lineStyle->GetOffsetRel()) {
      case OffsetRel::base:
        lineOffset=0.0;
        break;
      case OffsetRel::leftOutline:
        lineOffset=-pathData.mainSlotWidth/2.0;
        break;
      case OffsetRel::rightOutline:
        lineOffset=pathData.mainSlotWidth/2.0;
        break;
      case OffsetRel::laneDivider:
        lineOffset=0.0;
        lanesValue=lanesReader.GetValue(buffer);
        accessValue=accessReader.GetValue(buffer);

        if (lanesValue==nullptr &&
            accessValue==nullptr) {
          continue;
        }
        break;
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
        lanesValue=lanesReader.GetValue(buffer);
        break;
      case OffsetRel::sidecar:
        lineOffset=-1.1 * pathData.mainSlotWidth/2.0; // special handling not supported for ordinary ways
        break;
      }

      if (lineStyle->GetOffset()!=0.0) {
        lineOffset+=GetProjectedWidth(projection,
                                      lineStyle->GetOffset());
      }

      if (lineStyle->GetDisplayOffset()!=0.0) {
        lineOffset+=projection.ConvertWidthToPixel(lineStyle->GetDisplayOffset());
      }

      WayData data;

      data.lineWidth=lineWidth;

      if (!IsVisibleWay(projection,
                        way.GetBoundingBox(),
                        lineWidth/2)) {
        continue;
      }

      if (!transformed) {
        TransformPathData(projection, parameter, way, pathData);
        transformed=true;
        wayPathData.push_back(pathData);
      }

      Color color=lineStyle->GetLineColor();
      if (lineStyle->GetPreferColorFeature()){
        ColorFeatureValue *colorValue=colorReader.GetValue(buffer);
        if (colorValue != nullptr){
          color=colorValue->GetColor();
        }
      }

      data.layer=0;
      data.buffer=&buffer;
      data.lineStyle=lineStyle;
      data.color=color;
      data.wayPriority=styleConfig.GetWayPrio(buffer.GetType());
      data.startIsClosed=way.nodes[0].GetSerial()==0;
      data.endIsClosed=way.nodes[way.nodes.size()-1].GetSerial()==0;

      LayerFeatureValue *layerValue=layerReader.GetValue(buffer);

      if (layerValue!=nullptr) {
        data.layer=layerValue->GetLayer();
      }

      if (lineOffset!=0.0) {
        coordBuffer->GenerateParallelWay(pathData.transStart,pathData.transEnd,
                                         lineOffset,
                                         data.transStart,
                                         data.transEnd);
      }
      else {
        data.transStart=pathData.transStart;
        data.transEnd=pathData.transEnd;
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

        double  lanesSpace=pathData.mainSlotWidth/lanes;
        double  laneOffset=-pathData.mainSlotWidth/2.0+lanesSpace;

        for (size_t lane=1; lane<lanes; lane++) {
          coordBuffer->GenerateParallelWay(pathData.transStart,pathData.transEnd,
                                           laneOffset,
                                           data.transStart,
                                           data.transEnd);
          wayData.push_back(data);
          laneOffset+=lanesSpace;
        }
      }
      else if (lineStyle->GetOffsetRel() == OffsetRel::laneForwardLeft ||
               lineStyle->GetOffsetRel() == OffsetRel::laneForwardThroughLeft ||
               lineStyle->GetOffsetRel() == OffsetRel::laneForwardThrough ||
               lineStyle->GetOffsetRel() == OffsetRel::laneForwardThroughRight ||
               lineStyle->GetOffsetRel() == OffsetRel::laneForwardRight ||
               lineStyle->GetOffsetRel() == OffsetRel::laneBackwardLeft ||
               lineStyle->GetOffsetRel() == OffsetRel::laneBackwardThroughLeft ||
               lineStyle->GetOffsetRel() == OffsetRel::laneBackwardThrough ||
               lineStyle->GetOffsetRel() == OffsetRel::laneBackwardThroughRight ||
               lineStyle->GetOffsetRel() == OffsetRel::laneBackwardRight ) {

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
            coordBuffer->GenerateParallelWay(pathData.transStart, pathData.transEnd,
                                             laneOffset,
                                             data.transStart,
                                             data.transEnd);
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

  void MapPainter::PrepareWays(const StyleConfig& styleConfig,
                               const Projection& projection,
                               const MapParameter& parameter,
                               const MapData& data)
  {
    bool hasShieldLabels = styleConfig.HasWayPathShieldStyle(projection);

    for (const auto& way : data.ways) {
      if (way->nodes.size() < 2) {
        continue; // algorithms require at least two points
      }
      CalculatePaths(styleConfig,
                     projection,
                     parameter,
                     *way);

      if (hasShieldLabels) {
        CalculateWayShieldLabels(styleConfig,
                                 projection,
                                 parameter,
                                 *way);
      }
    }

    for (const auto& way : data.poiWays) {
      if (way->nodes.size() < 2) {
        continue; // algorithms require at least two points
      }
      CalculatePaths(styleConfig,
                     projection,
                     parameter,
                     *way);

      if (hasShieldLabels) {
        CalculateWayShieldLabels(styleConfig,
                                 projection,
                                 parameter,
                                 *way);
      }
    }
  }

  void MapPainter::PrepareRoutes(const StyleConfig& styleConfig,
                                 const Projection& projection,
                                 const MapParameter& parameter,
                                 const MapData& data)
  {
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
    for (WayPathDataIt it=wayPathData.begin(); it != wayPathData.end(); ++it){
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

      styleConfig.GetRouteLineStyles(route->GetFeatureValueBuffer(),
                                     projection,
                                     lineStyles);

      if (lineStyles.empty()){
        continue;
      }
      LineStyleRef lineStyle=lineStyles.front(); // slots for routes are not supported yet
      assert(lineStyle);

      PathTextStyleRef routeTextStyle=styleConfig.GetRoutePathTextStyle(route->GetFeatureValueBuffer(),
                                                                        projection);

      Color color=lineStyle->GetLineColor();
      if (lineStyle->GetPreferColorFeature()){
        ColorFeatureValue *colorValue=colorReader.GetValue(route->GetFeatureValueBuffer());
        if (colorValue != nullptr){
          color=colorValue->GetColor();
        }
      }

      double lineWidth=0;
      if (lineStyle->GetWidth()>0.0) {
        lineWidth+=GetProjectedWidth(projection,lineStyle->GetWidth());
      }
      if (lineStyle->GetDisplayWidth()>0.0) {
        lineWidth+=projection.ConvertWidthToPixel(lineStyle->GetDisplayWidth());
      }

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
            segmentWay.transStart=segment.transStart;
            segmentWay.transEnd=segment.transEnd;
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
              pathData.transStart=0; // Make the compiler happy
              pathData.transEnd=0;   // Make the compiler happy
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
          auto &pathData=memberWay->second.wayData;
          if (memberWay->second.colors.find(color)!=memberWay->second.colors.end()){
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
          if (lineOffset==0){
            transStart=pathData->transStart;
            transEnd=pathData->transEnd;
          }else{
            coordBuffer->GenerateParallelWay(pathData->transStart,pathData->transEnd,
                                             lineOffset,
                                             transStart,
                                             transEnd);
          }
          if (routeTmp.transSegments.empty()){
            RouteSegmentData routeSegmentData{transStart, transEnd, member.direction};
            routeTmp.transSegments.push_back(routeSegmentData);
          }else {
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
                lastEnd=transBuffer.buffer->buffer[lastSegment.transEnd];
              }else{
                lastEnd=transBuffer.buffer->buffer[lastSegment.transStart];
              }
              Vertex2D currentStart;
              if (member.direction==Route::MemberDirection::forward){
                currentStart=transBuffer.buffer->buffer[transStart];
              }else{
                currentStart=transBuffer.buffer->buffer[transEnd];
              }
              RouteSegmentData segmentConnection{transBuffer.buffer->PushCoord(lastEnd.GetX(),lastEnd.GetY()),
                                                 transBuffer.buffer->PushCoord(currentStart.GetX(),currentStart.GetY()),
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
    for (const auto& it : wayDataMap){
      if (!it.second.labels.empty()){
        RouteLabelData labelData;

        labelData.wayData= it.second.wayData;
        labelData.labels = it.second.labels;

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
   * Base method that must get called to initial the renderer for a render action.
   * The derived method of the concrete renderer implementation can have
   *
   * @return
   *    false if there was either an error or of the rendering was already interrupted, else true
   */
  void MapPainter::InitializeRender(const Projection& projection,
                                    const MapParameter& parameter,
                                    const MapData& /*data*/)
  {
    errorTolerancePixel=projection.ConvertWidthToPixel(parameter.GetOptimizeErrorToleranceMm());
    areaMinDimension   =projection.ConvertWidthToPixel(parameter.GetAreaMinDimensionMM());
    contourLabelOffset =GetProjectedWidth(projection,parameter.GetContourLabelOffset());
    contourLabelSpace  =GetProjectedWidth(projection,parameter.GetContourLabelSpace());

    shieldGridSizeHoriz=360.0/(std::pow(2,projection.GetMagnification().GetLevel()+1));
    shieldGridSizeVert=180.0/(std::pow(2,projection.GetMagnification().GetLevel()+1));

    transBuffer.Reset();

    standardFontSize=GetFontHeight(projection,
                                   parameter,
                                   1.0);
  }

  void MapPainter::DumpStatistics(const Projection& projection,
                                  const MapParameter& parameter,
                                  const MapData& data)
  {
    if (parameter.IsDebugPerformance()) {
      log.Info()
        << "Data: " << data.nodes.size() << "+" << data.poiNodes.size() << " " << data.ways.size() << " " << data.areas.size();
    }

    if (parameter.GetWarningCoordCountLimit()>0 ||
        parameter.GetWarningObjectCountLimit()>0 ||
        parameter.IsDebugData()) {
      DumpDataStatistics(projection,
                         parameter,
                         data);
    }
  }

  void MapPainter::PreprocessData(const Projection& projection,
                                  const MapParameter& parameter,
                                  const MapData& data)
  {
    StopClock prepareWaysTimer;

    wayData.clear();
    wayPathData.clear();

    PrepareWays(*styleConfig,
                projection,
                parameter,
                data);

    prepareWaysTimer.Stop();

    if (parameter.IsAborted()) {
      return;
    }

    StopClock prepareRoutesTimer;
    routeLabelData.clear();

    PrepareRoutes(*styleConfig,
                  projection,
                  parameter,
                  data);

    wayData.sort();

    prepareRoutesTimer.Stop();

    if (parameter.IsAborted()) {
      return;
    }

    StopClock prepareAreasTimer;

    PrepareAreas(*styleConfig,
                 projection,
                 parameter,
                 data);

    prepareAreasTimer.Stop();

    // Optional callback after preprocessing data
    AfterPreprocessing(*styleConfig,
                       projection,
                       parameter,
                       data);

    if (parameter.IsDebugPerformance() &&
        (prepareWaysTimer.IsSignificant() ||
         prepareAreasTimer.IsSignificant() ||
         prepareRoutesTimer.IsSignificant())) {

      log.Info()
        << "Prep: "
        << prepareWaysTimer.ResultString() << " (sec) "
        << prepareAreasTimer.ResultString() << " (sec) "
        << prepareRoutesTimer.ResultString() << " (sec)";
    }
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
      GeoBox boundingBox;

      projection.GetDimensions(boundingBox);

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

    DrawGroundTiles(projection, parameter, data.baseMapTiles);
  }

  void MapPainter::DrawGroundTiles(const Projection& projection,
                                   const MapParameter& parameter,
                                   const MapData& data)
  {
    DrawGroundTiles(projection, parameter, data.groundTiles);
  }

#if defined(DEBUG_GROUNDTILES)
static void DumpGroundTile(const GroundTile& tile)
  {
    switch (tile.type) {
    case GroundTile::land:
      std::cout << "Drawing land tile: " << tile.xRel << "," << tile.yRel << std::endl;
      break;
    case GroundTile::water:
      std::cout << "Drawing water tile: " << tile.xRel << "," << tile.yRel << std::endl;
      break;
    case GroundTile::coast:
      std::cout << "Drawing coast tile: " << tile.xRel << "," << tile.yRel << std::endl;
      break;
    case GroundTile::unknown:
      std::cout << "Drawing unknown tile: " << tile.xRel << "," << tile.yRel << std::endl;
      break;
    }
  }
#endif

  void MapPainter::DrawGroundTiles(const Projection& projection,
                                   const MapParameter& parameter,
                                   const std::list<GroundTile>& groundTiles)
  {
    if (!parameter.GetRenderSeaLand()) {
      return;
    }

#if defined(DEBUG_GROUNDTILES)
    std::set<GeoCoord> drawnLabels;
#endif

    FillStyleRef       landFill=styleConfig->GetLandFillStyle(projection);
    FillStyleRef       seaFill=styleConfig->GetSeaFillStyle(projection);
    FillStyleRef       coastFill=styleConfig->GetCoastFillStyle(projection);
    FillStyleRef       unknownFill=styleConfig->GetUnknownFillStyle(projection);
    LineStyleRef       coastlineLine=styleConfig->GetCoastlineLineStyle(projection);
    std::vector<Point> points;
    size_t             start=0; // Make the compiler happy
    size_t             end=0;   // Make the compiler happy

    if (!landFill) {
      landFill=this->landFill;
    }

    if (!seaFill) {
      seaFill=this->seaFill;
    }

    for (const auto& tile : groundTiles) {
      AreaData groundTileData;

#if defined(DEBUG_GROUNDTILES)
      DumpGroundTile(tile);
#endif

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

      GeoCoord minCoord(tile.yAbs*tile.cellHeight-90.0,
                        tile.xAbs*tile.cellWidth-180.0);
      GeoCoord maxCoord(minCoord.GetLat()+tile.cellHeight,
                        minCoord.GetLon()+tile.cellWidth);

      groundTileData.boundingBox.Set(minCoord,maxCoord);

      // skip tiles that are completely outside projection
      if (!projection.GetDimensions().Intersects(groundTileData.boundingBox)){
#if defined(DEBUG_GROUNDTILES)
        std::cout << "Tile outside projection: " << tile.xRel << "," << tile.yRel
                  << " " << areaData.boundingBox.GetDisplayText() << std::endl;
#endif
        continue;
      }

      if (tile.coords.empty()) {
#if defined(DEBUG_GROUNDTILES)
        std::cout << " >= fill" << std::endl;
#endif
        transBuffer.TransformBoundingBox(projection,
                                         TransPolygon::none,
                                         groundTileData.boundingBox,
                                         start,
                                         end,
                                         errorTolerancePixel);
      }
      else {
#if defined(DEBUG_GROUNDTILES)
        std::cout << " >= sub" << std::endl;
#endif
        points.resize(tile.coords.size());

        for (size_t i=0; i<tile.coords.size(); i++) {
          double lat;
          double lon;

          lat=groundTileData.boundingBox.GetMinCoord().GetLat()+tile.coords[i].y*tile.cellHeight/GroundTile::Coord::CELL_MAX;
          lon=groundTileData.boundingBox.GetMinCoord().GetLon()+tile.coords[i].x*tile.cellWidth/GroundTile::Coord::CELL_MAX;

          points[i].SetCoord(GeoCoord(lat,lon));
        }

        transBuffer.transPolygon.TransformArea(projection,
                                               TransPolygon::none,
                                               points,
                                               errorTolerancePixel);

        for (size_t i=transBuffer.transPolygon.GetStart(); i<=transBuffer.transPolygon.GetEnd(); i++) {
          double x,y;

          if (tile.coords[i].x==0) {
            x=floor(transBuffer.transPolygon.points[i].x);
          }
          else if (tile.coords[i].x==GroundTile::Coord::CELL_MAX) {
            x=ceil(transBuffer.transPolygon.points[i].x);
          }
          else {
            x=transBuffer.transPolygon.points[i].x;
          }

          if (tile.coords[i].y==0) {
            y=ceil(transBuffer.transPolygon.points[i].y);
          }
          else if (tile.coords[i].y==GroundTile::Coord::CELL_MAX) {
            y=floor(transBuffer.transPolygon.points[i].y);
          }
          else {
            y=transBuffer.transPolygon.points[i].y;
          }

          size_t idx=transBuffer.buffer->PushCoord(x,y);

          if (i==transBuffer.transPolygon.GetStart()) {
            start=idx;
          }
          else if (i==transBuffer.transPolygon.GetEnd()) {
            end=idx;
          }
        }

        if (coastlineLine) {
          size_t lineStart=0;
          size_t lineEnd;

          while (lineStart<tile.coords.size()) {
            while (lineStart<tile.coords.size() &&
                   !tile.coords[lineStart].coast) {
              lineStart++;
            }

            if (lineStart>=tile.coords.size()) {
              continue;
            }

            lineEnd=lineStart;

            while (lineEnd<tile.coords.size() &&
                   tile.coords[lineEnd].coast) {
              lineEnd++;
            }

            if (lineStart!=lineEnd) {
              WayData wd;

              wd.buffer=&coastlineSegmentAttributes;
              wd.layer=0;
              wd.lineStyle=coastlineLine;
              wd.color=coastlineLine->GetLineColor();
              wd.wayPriority=std::numeric_limits<size_t>::max();
              wd.transStart=start+lineStart;
              wd.transEnd=start+lineEnd;
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
      groundTileData.transStart=start;
      groundTileData.transEnd  =end;

      DrawArea(projection,parameter,groundTileData);

#if defined(DEBUG_GROUNDTILES)
      GeoCoord cc=areaData.boundingBox.GetCenter();

      std::string label;

      size_t x=(cc.GetLon()+180)/tile.cellWidth;
      size_t y=(cc.GetLat()+90)/tile.cellHeight;

      label=std::to_string(tile.xRel)+","+std::to_string(tile.yRel);

      double lon=(x*tile.cellWidth+tile.cellWidth/2)-180.0;
      double lat=(y*tile.cellHeight+tile.cellHeight/2)-90.0;

      double px;
      double py;

      projection.GeoToPixel(GeoCoord(lat,lon),
                            px,py);

      if (drawnLabels.find(GeoCoord(x,y))!=drawnLabels.end()) {
        continue;
      }

      LabelData labelBox;

      labelBox.priority=0;
      labelBox.alpha=debugLabel->GetAlpha();;
      labelBox.fontSize=debugLabel->GetSize();
      labelBox.style=debugLabel;
      labelBox.text=label;

      std::vector<LabelData> vect;
      vect.push_back(labelBox);
      RegisterRegularLabel(projection,
                           parameter,
                           vect,
                           Vertex2D(px,py),
                           /*proposedWidth*/ -1);

      drawnLabels.insert(GeoCoord(x,y));
#endif
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

    for (const auto way: wayPathData) {
      if (DrawWayDecoration(*styleConfig,
                            projection,
                            parameter,
                            way)) {
        drawnCount++;
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
        drawnCount++;
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

      drawnCount++;
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
        drawnCount++;
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
        drawnCount++;
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
          drawnCount++;
        }
      }
    }

    timer.Stop();

    if (parameter.IsDebugPerformance() && timer.IsSignificant()) {
      log.Info()
          << "Draw route contour labels: " << drawnCount << " (pcs) " << timer.ResultString() << "(s)";
    }
  }

  double RoundDown(double value) {
    if (value>=0.0) {
      return floor(value);
    }

    return ceil(value);
  }

  double RoundUp(double value) {
    if (value>=0.0) {
      return ceil(value);
    }

    return floor(value);
  }

  bool CrossesElevationLine(int32_t ele, int32_t height1, int32_t height2)
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

      log.Info() << "Processing height map " << data.srtmTile->boundingBox.GetDisplayText() << " " << data.srtmTile->columns << "x" << data.srtmTile->rows;

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

      log.Info() << "Elevation range: " << minHeight << " => " << maxHeight << " | " << minElevation << " => " << maxElevation;

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
                log.Info() << "Left " << x << "," << y << " " << box.GetDisplayText() << " " << std::abs(otherHeight-height);

                FeatureValueBuffer     buffer;
                WayData                wd;
                std::array<GeoCoord,2> path;
                size_t                 start,end;

                path[0]=box.GetBottomLeft();
                path[1]=box.GetTopLeft();

                transBuffer.TransformWay(projection,
                                         TransPolygon::none,
                                         path,
                                         start,
                                         end,
                                         errorTolerancePixel);

                wd.buffer=&buffer;
                wd.layer=0;
                wd.lineStyle=contourLineStyles[0];
                wd.color=contourLineStyles[0]->GetLineColor();
                wd.wayPriority=std::numeric_limits<size_t>::max();
                wd.transStart=start;
                wd.transEnd=end;
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
                log.Info() << "Right " << x << "," << y << " " << box.GetDisplayText() << " " << std::abs(otherHeight-height);

                FeatureValueBuffer     buffer;
                WayData                wd;
                std::array<GeoCoord,2> path;
                size_t                 start,end;

                path[0]=box.GetBottomRight();
                path[1]=box.GetTopRight();

                transBuffer.TransformWay(projection,
                                         TransPolygon::none,
                                         path,
                                         start,
                                         end,
                                         errorTolerancePixel);

                wd.buffer=&buffer;
                wd.layer=0;
                wd.lineStyle=contourLineStyles[0];
                wd.color=contourLineStyles[0]->GetLineColor();
                wd.wayPriority=std::numeric_limits<size_t>::max();
                wd.transStart=start;
                wd.transEnd=end;
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
                log.Info() << "Top " << x << "," << y << " " << box.GetDisplayText() << " " << std::abs(otherHeight-height);

                FeatureValueBuffer     buffer;
                WayData                wd;
                std::array<GeoCoord,2> path;
                size_t                 start,end;

                path[0]=box.GetTopLeft();
                path[1]=box.GetTopRight();

                transBuffer.TransformWay(projection,
                                         TransPolygon::none,
                                         path,
                                         start,
                                         end,
                                         errorTolerancePixel);

                wd.buffer=&buffer;
                wd.layer=0;
                wd.lineStyle=contourLineStyles[0];
                wd.color=contourLineStyles[0]->GetLineColor();
                wd.wayPriority=std::numeric_limits<size_t>::max();
                wd.transStart=start;
                wd.transEnd=end;
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
                log.Info() << "Bottom " << x << "," << y << " " << box.GetDisplayText() << " " << std::abs(otherHeight-height);

                FeatureValueBuffer     buffer;
                WayData                wd;
                std::array<GeoCoord,2> path;
                size_t                 start,end;

                path[0]=box.GetBottomLeft();
                path[1]=box.GetBottomRight();

                transBuffer.TransformWay(projection,
                                         TransPolygon::none,
                                         path,
                                         start,
                                         end,
                                         errorTolerancePixel);

                wd.buffer=&buffer;
                wd.layer=0;
                wd.lineStyle=contourLineStyles[0];
                wd.color=contourLineStyles[0]->GetLineColor();
                wd.wayPriority=std::numeric_limits<size_t>::max();
                wd.transStart=start;
                wd.transEnd=end;
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

              size_t start,end;
              double xDelta         =subX*factor;
              double yDelta         =subY*factor;
              GeoBox tileBoundingBox=GeoBox(GeoCoord(y+yDelta,
                                                     x+xDelta),
                                            GeoCoord(y+yDelta+factor,
                                                     x+xDelta+factor));

              transBuffer.TransformBoundingBox(projection,
                                               TransPolygon::none,
                                               tileBoundingBox,
                                               start,
                                               end,
                                               errorTolerancePixel);

              for (const auto& borderStyle : areaBorderStyles) {
                AreaData tileData;

                tileData.borderStyle=borderStyle;
                tileData.boundingBox=tileBoundingBox;
                tileData.ref=ObjectFileRef();
                tileData.transStart=start;
                tileData.transEnd  =end;

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
    for (int x=minX; x<int(maxLon); x++) {
      for (int y=minY; y<int(maxLat); y++) {
        for (int subX=0; subX<1201; subX++) {
          for (int subY=0; subY<1201; subY++) {
            even=!even;

            size_t   start,end;
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
              double xPos,yPos;

              projection.GeoToPixel(center,xPos,yPos);

              double                  latitude=center.GetLat();
              double                  longitude=center.GetLon();
              double                  fracLat  =latitude>=0.0 ? 1-latitude+floor(latitude) : ceil(latitude)-latitude;
              double                  fracLon  =longitude>=0.0 ? longitude-floor(longitude) : 1-ceil(longitude)+longitude;
              int                     col      =int(floor(fracLon*data.srtmTile->columns));
              int                     row      =int(floor(fracLat*data.srtmTile->rows));

              int32_t height=data.srtmTile->GetHeight(col,row);

              LabelData labelBox;

              labelBox.priority=0;
              labelBox.alpha=1.0;
              labelBox.fontSize=textStyles[0]->GetSize();
              labelBox.style=textStyles[0];
              labelBox.text=std::to_string(height);

              std::vector<LabelData> vect;
              vect.push_back(labelBox);
              RegisterRegularLabel(projection, parameter, vect, Vertex2D(xPos,yPos), /*proposedWidth*/ -1);
            }

            // chess pattern
            if (!even) {
              continue;
            }

            if (hillShadingFill) {
              transBuffer.TransformBoundingBox(projection,
                                               TransPolygon::none,
                                               tileData.boundingBox,
                                               start,
                                               end,
                                               errorTolerancePixel);

              tileData.ref=ObjectFileRef();
              tileData.transStart=start;
              tileData.transEnd=end;

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
