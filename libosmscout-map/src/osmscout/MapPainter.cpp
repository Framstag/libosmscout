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

#include <limits>

#include <osmscout/system/Math.h>

#include <osmscout/util/Logger.h>
#include <osmscout/util/StopClock.h>
#include <osmscout/util/String.h>
#include <osmscout/util/Tiling.h>
#include <iostream>

//#define DEBUG_GROUNDTILES
//#define DEBUG_NODE_DRAW

#if defined(DEBUG_GROUNDTILES)
#include <osmscout/Coord.h>
#endif

namespace osmscout {

  static void GetGridPoints(const std::vector<Point>& nodes,
                            double gridSizeHoriz,
                            double gridSizeVert,
                            std::set<GeoCoord>& intersections)
  {
    assert(nodes.size()>=2);

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
      else if (!a.fillStyle->GetFillColor().IsSolid() && b.fillStyle->GetFillColor().IsSolid()) {
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
          return a.boundingBox.GetMaxCoord().GetLat()>b.boundingBox.GetMaxCoord().GetLat();
        }
        else {
          return a.boundingBox.GetMinCoord().GetLat()<b.boundingBox.GetMinCoord().GetLat();
        }
      }
      else {
        return a.boundingBox.GetMaxCoord().GetLon()>b.boundingBox.GetMaxCoord().GetLon();
      }
    }
    else {
      return a.boundingBox.GetMinCoord().GetLon()<b.boundingBox.GetMinCoord().GetLon();
    }
  }

  /**
   * Deletes the content hold by this instance.
   */
  void MapData::ClearDBData()
  {
    nodes.clear();
    areas.clear();
    ways.clear();
  }

  /**
   * Sort labels for the same object by position
   */
  static inline bool LabelLayoutDataSorter(const MapPainter::LabelLayoutData& a,
                                           const MapPainter::LabelLayoutData& b)
  {
    return a.position<b.position;
  }

  MapPainter::ContourLabelHelper::ContourLabelHelper(const MapPainter& painter)
  : contourLabelOffset(painter.contourLabelOffset),
    contourLabelSpace(painter.contourLabelSpace)
  {
    // no code
  }

  bool MapPainter::ContourLabelHelper::Init(double pathLength,
                                            double textWidth)
  {
    this->pathLength=pathLength;
    this->textWidth=textWidth;

    if (pathLength-textWidth-2*contourLabelOffset<=0.0) {
      return false;
    }

    currentOffset=fmod(pathLength-textWidth-2*contourLabelOffset,
                  textWidth+contourLabelSpace)/2+contourLabelOffset;

    return true;
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
    addressReader(*styleConfig->GetTypeConfig())
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
        IconStyleRef iconStyle;

        styleConfig->GetNodeTextStyles(node->GetFeatureValueBuffer(),
                                       projection,
                                       textStyles);
        styleConfig->GetNodeIconStyle(node->GetFeatureValueBuffer(),
                                      projection,
                                      iconStyle);

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
        IconStyleRef iconStyle;

        styleConfig->GetNodeTextStyles(node->GetFeatureValueBuffer(),
                                       projection,
                                       textStyles);
        styleConfig->GetNodeIconStyle(node->GetFeatureValueBuffer(),
                                      projection,
                                      iconStyle);

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
        PathShieldStyleRef shieldStyle;
        PathTextStyleRef   pathTextStyle;

        styleConfig->GetWayPathShieldStyle(way->GetFeatureValueBuffer(),
                                           projection,
                                           shieldStyle);
        styleConfig->GetWayPathTextStyle(way->GetFeatureValueBuffer(),
                                         projection,
                                         pathTextStyle);

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
        PathShieldStyleRef shieldStyle;
        PathTextStyleRef   pathTextStyle;

        styleConfig->GetWayPathShieldStyle(way->GetFeatureValueBuffer(),
                                           projection,
                                           shieldStyle);
        styleConfig->GetWayPathTextStyle(way->GetFeatureValueBuffer(),
                                         projection,
                                         pathTextStyle);

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
          if (ring.IsMasterRing()) {
            IconStyleRef iconStyle;

            styleConfig->GetAreaTextStyles(area->GetType(),
                                           ring.GetFeatureValueBuffer(),
                                           projection,
                                           textStyles);
            styleConfig->GetAreaIconStyle(area->GetType(),
                                          ring.GetFeatureValueBuffer(),
                                          projection,
                                          iconStyle);

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
          if (ring.IsMasterRing()) {
            IconStyleRef iconStyle;

            styleConfig->GetAreaTextStyles(area->GetType(),
                                           ring.GetFeatureValueBuffer(),
                                           projection,
                                           textStyles);
            styleConfig->GetAreaIconStyle(area->GetType(),
                                          ring.GetFeatureValueBuffer(),
                                          projection,
                                          iconStyle);

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
                                const std::vector<Point>& nodes,
                                double pixelOffset) const
  {
    if (nodes.empty()) {
      return false;
    }

    osmscout::GeoBox boundingBox;

    osmscout::GetBoundingBox(nodes,
                             boundingBox);

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
    else {
      return width;
    }
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

  void MapPainter::DrawGroundTiles(const StyleConfig& styleConfig,
                                   const Projection& projection,
                                   const MapParameter& parameter,
                                   const MapData& data)
  {
    FillStyleRef      landFill;

#if defined(DEBUG_GROUNDTILES)
      std::set<GeoCoord> drawnLabels;
#endif

    styleConfig.GetLandFillStyle(projection,
                                 landFill);

    if (!landFill) {
      landFill=this->landFill;
    }

    if (parameter.GetRenderBackground()) {
      DrawGround(projection,
                 parameter,
                 *landFill);
    }

    if (!parameter.GetRenderSeaLand()) {
      return;
    }

    FillStyleRef       seaFill;
    FillStyleRef       coastFill;
    FillStyleRef       unknownFill;
    LineStyleRef       coastlineLine;
    std::vector<Point> points;
    size_t             start=0; // Make the compiler happy
    size_t             end=0;   // Make the compiler happy

    styleConfig.GetSeaFillStyle(projection,
                                seaFill);
    styleConfig.GetCoastFillStyle(projection,
                                  coastFill);
    styleConfig.GetUnknownFillStyle(projection,
                                    unknownFill);
    styleConfig.GetCoastlineLineStyle(projection,
                                      coastlineLine);

    if (!seaFill) {
      seaFill=this->seaFill;
    }

    for (const auto& tile : data.groundTiles) {
      AreaData areaData;

      if (tile.type==GroundTile::unknown &&
          !parameter.GetRenderUnknowns()) {
        continue;
      }

      switch (tile.type) {
      case GroundTile::land:
#if defined(DEBUG_GROUNDTILES)
        std::cout << "Drawing land tile: " << tile.xRel << "," << tile.yRel << std::endl;
#endif
        areaData.fillStyle=landFill;
        break;
      case GroundTile::water:
#if defined(DEBUG_GROUNDTILES)
        std::cout << "Drawing water tile: " << tile.xRel << "," << tile.yRel << std::endl;
#endif
        areaData.fillStyle=seaFill;
        break;
      case GroundTile::coast:
#if defined(DEBUG_GROUNDTILES)
        std::cout << "Drawing coast tile: " << tile.xRel << "," << tile.yRel << std::endl;
#endif
        areaData.fillStyle=coastFill;
        break;
      case GroundTile::unknown:
#if defined(DEBUG_GROUNDTILES)
        std::cout << "Drawing unknown tile: " << tile.xRel << "," << tile.yRel << std::endl;
#endif
        areaData.fillStyle=unknownFill;
        break;
      }
      if (!areaData.fillStyle){
        continue;
      }

      GeoCoord minCoord(tile.yAbs*tile.cellHeight-90.0,
                        tile.xAbs*tile.cellWidth-180.0);
      GeoCoord maxCoord(minCoord.GetLat()+tile.cellHeight,
                        minCoord.GetLon()+tile.cellWidth);

      areaData.boundingBox.Set(minCoord,maxCoord);

      if (tile.coords.empty()) {
#if defined(DEBUG_GROUNDTILES)
        std::cout << " >= fill" << std::endl;
#endif
        // Fill the cell completely with the fill for the given cell type
        points.resize(5);

        points[0].SetCoord(areaData.boundingBox.GetMinCoord());
        points[1].SetCoord(GeoCoord(areaData.boundingBox.GetMinCoord().GetLat(),
                                    areaData.boundingBox.GetMaxCoord().GetLon()));
        points[2].SetCoord(areaData.boundingBox.GetMaxCoord());
        points[3].SetCoord(GeoCoord(areaData.boundingBox.GetMaxCoord().GetLat(),
                                    areaData.boundingBox.GetMinCoord().GetLon()));
        points[4]=points[0];

        transBuffer.transPolygon.TransformArea(projection,
                                               TransPolygon::none,
                                               points,
                                               errorTolerancePixel);

        size_t s=transBuffer.transPolygon.GetStart();

        start=transBuffer.buffer->PushCoord(floor(transBuffer.transPolygon.points[s+0].x),
                                            ceil(transBuffer.transPolygon.points[s+0].y));


        transBuffer.buffer->PushCoord(ceil(transBuffer.transPolygon.points[s+1].x),
                                      ceil(transBuffer.transPolygon.points[s+1].y));

        transBuffer.buffer->PushCoord(ceil(transBuffer.transPolygon.points[s+2].x),
                                      floor(transBuffer.transPolygon.points[s+2].y));

        transBuffer.buffer->PushCoord(floor(transBuffer.transPolygon.points[s+3].x),
                                      floor(transBuffer.transPolygon.points[s+3].y));

        end=transBuffer.buffer->PushCoord(floor(transBuffer.transPolygon.points[s+4].x),
                                          ceil(transBuffer.transPolygon.points[s+4].y));
      }
      else {
#if defined(DEBUG_GROUNDTILES)
        std::cout << " >= sub" << std::endl;
#endif
        points.resize(tile.coords.size());

        for (size_t i=0; i<tile.coords.size(); i++) {
          double lat;
          double lon;

          lat=areaData.boundingBox.GetMinCoord().GetLat()+tile.coords[i].y*tile.cellHeight/GroundTile::Coord::CELL_MAX;
          lon=areaData.boundingBox.GetMinCoord().GetLon()+tile.coords[i].x*tile.cellWidth/GroundTile::Coord::CELL_MAX;

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
              wd.wayPriority=std::numeric_limits<int>::max();
              wd.transStart=start+lineStart;
              wd.transEnd=start+lineEnd;
              wd.lineWidth=GetProjectedWidth(projection,
                                               projection.ConvertWidthToPixel(coastlineLine->GetDisplayWidth()),
                                               coastlineLine->GetWidth());
              wd.startIsClosed=false;
              wd.endIsClosed=false;

              /*
              DrawWay(styleConfig,
                      projection,
                      parameter,
                      data);*/

              wayData.push_back(wd);
            }

            lineStart=lineEnd+1;
          }
        }
      }

      areaData.ref=ObjectFileRef();
      areaData.transStart=start;
      areaData.transEnd=end;

      DrawArea(projection,parameter,areaData);

#if defined(DEBUG_GROUNDTILES)
      size_t   labelId=nextLabelId++;
      GeoCoord cc=areaData.boundingBox.GetCenter();

      std::string label;

      size_t x=(cc.GetLon()+180)/tile.cellWidth;
      size_t y=(cc.GetLat()+90)/tile.cellHeight;

      label=NumberToString(tile.xRel)+","+NumberToString(tile.yRel);

      double lon=(x*tile.cellWidth+tile.cellWidth/2)-180.0;
      double lat=(y*tile.cellHeight+tile.cellHeight/2)-90.0;

      double px;
      double py;

      projection.GeoToPixel(GeoCoord(lat,lon),
                            px,py);

      if (drawnLabels.find(GeoCoord(x,y))!=drawnLabels.end()) {
        continue;
      }

      LabelLayoutData labelData;

      labelData.fontSize=debugLabel->GetSize();

      labelData.dimension=GetTextDimension(projection,
                                           parameter,
                                           -1,
                                           labelData.fontSize,
                                           label);

      labelData.alpha=debugLabel->GetAlpha();
      labelData.position=0;
      labelData.label=label;
      labelData.textStyle=debugLabel;
      labelData.icon=false;

      RegisterPointLabel(projection,
                         parameter,
                         labelData,
                         px,py,
                         labelId);

      drawnLabels.insert(GeoCoord(x,y));
#endif
    }
  }

  void MapPainter::RegisterPointWayLabel(const Projection& projection,
                                         const MapParameter& parameter,
                                         const PathShieldStyleRef& shieldStyle,
                                         const std::string& text,
                                         const std::vector<Point>& nodes)
  {
    const LabelStyleRef& style=shieldStyle->GetShieldStyle();
    std::set<GeoCoord>   gridPoints;

    //SymbolRef symbol=styleConfig->GetSymbol("marker");

    GetGridPoints(nodes,
                  shieldGridSizeHoriz,
                  shieldGridSizeVert,
                  gridPoints);

    double frameHoriz=5;
    double frameVert=5;

    TextDimension dimension=GetTextDimension(projection,
                                             parameter,
                                             -1,
                                             style->GetSize(),
                                             text);


    for (const auto& gridPoint : gridPoints) {
      double x,y;

      projection.GeoToPixel(gridPoint,x,y);

      /*
      if (x<0 || x>projection.GetWidth()) {
        continue;
      }
      if (y<0 || y>projection.GetHeight()) {
        continue;
      }

      DrawSymbol(projection,
                 parameter,
                 *symbol,
                 x,y);*/

      LabelData labelBox;

      labelBox.id=nextLabelId++;
      labelBox.bx1=x-dimension.width/2-frameHoriz;
      labelBox.bx2=x+dimension.width/2+frameHoriz;
      labelBox.by1=y-dimension.height/2-frameVert;
      labelBox.by2=y+dimension.height/2+frameVert;
      labelBox.priority=style->GetPriority();
      labelBox.x=x-dimension.xOff-dimension.width/2;
      labelBox.y=y-dimension.yOff-dimension.height/2;
      labelBox.alpha=1.0;
      labelBox.fontSize=style->GetSize();
      labelBox.style=style;
      labelBox.text=text;

      LabelDataRef label;

      labels.Placelabel(labelBox,
                        label);
    }
  }

  /**
   * Register a label with the given parameter.The given coordinates
   * define the center of the label. The resulting label will be
   * vertically and horizontally aligned to the given coordinate.
   */
  bool MapPainter::RegisterPointLabel(const Projection& /*projection*/,
                                      const MapParameter& /*parameter*/,
                                      const LabelLayoutData& data,
                                      double x,
                                      double y,
                                      size_t id)
  {
    // Something is an overlay, if its alpha is <0.8
    bool overlay=data.alpha<0.8;

    LabelData labelBox;

    labelBox.id=id;
    labelBox.bx1=x-data.dimension.width/2;
    labelBox.bx2=x+data.dimension.width/2;
    labelBox.by1=y-data.dimension.height/2;
    labelBox.by2=y+data.dimension.height/2;
    labelBox.priority=data.textStyle->GetPriority();
    labelBox.x=x-data.dimension.xOff-data.dimension.width/2;
    labelBox.y=y-data.dimension.yOff-data.dimension.height/2;
    labelBox.alpha=data.alpha;
    labelBox.fontSize=data.fontSize;
    labelBox.style=data.textStyle;
    labelBox.text=data.label;

    LabelDataRef label;

    if (overlay) {
      if (!overlayLabels.Placelabel(labelBox,
                                    label)) {
        return false;
      }
    }
    else {
      if (!labels.Placelabel(labelBox,
                             label)) {
        return false;
      }
    }

    return true;
  }

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
    labelLayoutData.clear();

    /*
    SymbolRef symbol=styleConfig->GetSymbol("marker");

    if (symbol) {
      DrawSymbol(projection,
                 parameter,
                 *symbol,
                 x,y);
    }*/

    size_t labelId=nextLabelId++;
    double overallTextHeight=0;
    bool   hasSymbol=false;

    if (iconStyle) {
      if (!iconStyle->GetIconName().empty() &&
          HasIcon(*styleConfig,
                  parameter,
                  *iconStyle)) {
        LabelLayoutData data;

        data.position=iconStyle->GetPosition();
        data.dimension=TextDimension(0.0,0.0,14.0,14.0);
        data.icon=true;
        data.iconStyle=iconStyle;

        hasSymbol=true;

        labelLayoutData.push_back(data);
      }
      else if (iconStyle->GetSymbol()) {
        LabelLayoutData data;

        data.position=iconStyle->GetPosition();
        data.dimension=TextDimension(0.0,0.0,
                                     projection.ConvertWidthToPixel(iconStyle->GetSymbol()->GetWidth()),
                                     projection.ConvertWidthToPixel(iconStyle->GetSymbol()->GetHeight()));
        data.icon=false;
        data.iconStyle=iconStyle;

        hasSymbol=true;

        labelLayoutData.push_back(data);
      }
    }

    for (const auto& textStyle : textStyles) {
      std::string label=textStyle->GetLabel()->GetLabel(parameter,
                                                        buffer);

      if (label.empty()) {
        continue;
      }

      LabelLayoutData data;

      if (projection.GetMagnification()>textStyle->GetScaleAndFadeMag() &&
          parameter.GetDrawFadings()) {
        double factor=projection.GetMagnification().GetLevel()-textStyle->GetScaleAndFadeMag().GetLevel();
        data.fontSize=textStyle->GetSize()*pow(1.5,factor);

        data.dimension=GetTextDimension(projection,
                                        parameter,
                                        objectWidth,
                                        data.fontSize,
                                        label);

        data.alpha=std::min(textStyle->GetAlpha()/factor, 1.0);
      }
      else if (textStyle->GetAutoSize()) {
        double height=std::abs((objectHeight)*0.1);

        if (height==0 || height<standardFontSize) {
          continue;
        }

        // Retricts the height of a label to maxHeight
        double alpha=textStyle->GetAlpha();
        double maxHeight=projection.GetHeight()/5;

        if (height>maxHeight) {
            // If the height exeeds maxHeight the alpha value will be decreased
            double minAlpha=projection.GetHeight();
            double normHeight=(height-maxHeight)/(minAlpha-maxHeight);
            alpha*=std::min(std::max(1-normHeight,0.2),1.0);
            height=maxHeight;
        }

        data.fontSize=height/standardFontSize;
        data.alpha=alpha;

        data.dimension=GetTextDimension(projection,
                                        parameter,
                                        objectWidth,
                                        data.fontSize,
                                        label);
      }
      else {
        data.fontSize=textStyle->GetSize();

        data.dimension=GetTextDimension(projection,
                                        parameter,
                                        objectWidth,
                                        data.fontSize,
                                        label);

        data.alpha=textStyle->GetAlpha();
      }

      data.position=textStyle->GetPosition();
      data.label=label;
      data.textStyle=textStyle;
      data.icon=false;

      overallTextHeight+=data.dimension.height;

      labelLayoutData.push_back(data);
    }

    if (labelLayoutData.empty()) {
      return;
    }

    std::stable_sort(labelLayoutData.begin(),
                     labelLayoutData.end(),
                     LabelLayoutDataSorter);

    // This is the top center position of the initial label element.
    // Note that RegisterPointLabel gets passed the center of the label,
    // thus we need to convert it...
    double offset = hasSymbol ? y : y-overallTextHeight/2;

    //std::cout << ">>>" << std::endl;
    for (const auto& data : labelLayoutData) {
      if (data.textStyle) {
        //std::cout << "# Text '" << data.label << "' " << offset << " " << data.height << " " << projection.ConvertWidthToPixel(parameter.GetLabelSpace()) << std::endl;
        RegisterPointLabel(projection,
                           parameter,
                           data,
                           x,offset+data.dimension.height/2,
                           labelId);
      }
      else if (data.icon) {
        //std::cout << "# Icon " << offset << " " << data.height << " " << projection.ConvertWidthToPixel(parameter.GetLabelSpace()) << std::endl;
        DrawIcon(data.iconStyle.get(),
                 x,offset);
      }
      else {
        //std::cout << "# Symbol " << offset << " " << data.height << " " << projection.ConvertWidthToPixel(parameter.GetLabelSpace()) << std::endl;
        DrawSymbol(projection,
                   parameter,
                   *data.iconStyle->GetSymbol(),
                   x,offset);
      }

      offset+=data.dimension.height;
    }
    //std::cout << "<<<" << std::endl;
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

  void MapPainter::DrawNodes(const StyleConfig& styleConfig,
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

      DrawNode(styleConfig,
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

  void MapPainter::DrawAreas(const StyleConfig& /*styleConfig*/,
                             const Projection& projection,
                             const MapParameter& parameter)
  {
    for (const auto& area : areaData)
    {
      DrawArea(projection,
               parameter,
               area);

      areasDrawn++;
    }
  }

  void MapPainter::DrawAreaLabel(const StyleConfig& styleConfig,
                                 const Projection& projection,
                                 const MapParameter& parameter,
                                 const AreaData& areaData)
  {
    IconStyleRef iconStyle;

    styleConfig.GetAreaTextStyles(areaData.type,
                                  *areaData.buffer,
                                  projection,
                                  textStyles);

    styleConfig.GetAreaIconStyle(areaData.type,
                                 *areaData.buffer,
                                 projection,
                                 iconStyle);

    if (!iconStyle && textStyles.empty()) {
      return;
    }

    double minX;
    double maxX;
    double minY;
    double maxY;

    projection.GeoToPixel(areaData.boundingBox.GetMinCoord(),
                          minX,minY);

    projection.GeoToPixel(areaData.boundingBox.GetMaxCoord(),
                          maxX,maxY);

    LayoutPointLabels(projection,
                      parameter,
                      *areaData.buffer,
                      iconStyle,
                      textStyles,
                      (minX+maxX)/2,
                      (minY+maxY)/2,
                      maxX-minX,
                      maxY-minY);
  }

  void MapPainter::DrawAreaBorderLabel(const StyleConfig& styleConfig,
                                       const Projection& projection,
                                       const MapParameter& parameter,
                                       const AreaData& areaData)
  {
    PathTextStyleRef borderTextStyle;

    styleConfig.GetAreaBorderTextStyle(areaData.type,
                                       *areaData.buffer,
                                       projection,
                                       borderTextStyle);

    if (!borderTextStyle) {
      return;
    }

    double      lineOffset=0.0;
    size_t      transStart=areaData.transStart;
    size_t      transEnd=areaData.transEnd;
    std::string label=borderTextStyle->GetLabel()->GetLabel(parameter,
                                                            *areaData.buffer);

    if (label.empty()) {
      return;
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

    ContourLabelHelper helper(*this);

    DrawContourLabel(projection,
                     parameter,
                     *borderTextStyle,
                     label,
                     transStart,
                     transEnd,
                     helper);
  }

  void MapPainter::DrawAreaBorderSymbol(const StyleConfig& styleConfig,
                                        const Projection& projection,
                                        const MapParameter& parameter,
                                        const AreaData& areaData)
  {
    PathSymbolStyleRef borderSymbolStyle;

    styleConfig.GetAreaBorderSymbolStyle(areaData.type,
                                         *areaData.buffer,
                                         projection,
                                         borderSymbolStyle);

    if (!borderSymbolStyle) {
      return;
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
  }

  void MapPainter::DrawAreaLabels(const StyleConfig& styleConfig,
                                  const Projection& projection,
                                  const MapParameter& parameter)
  {
    for (const auto& area : areaData)
    {
      if (area.buffer==nullptr) {
        continue;
      }

      DrawAreaLabel(styleConfig,
                    projection,
                    parameter,
                    area);

      DrawAreaBorderLabel(styleConfig,
                          projection,
                          parameter,
                          area);

      DrawAreaBorderSymbol(styleConfig,
                           projection,
                           parameter,
                           area);

      areasDrawn++;
    }
  }

  void MapPainter::DrawNode(const StyleConfig& styleConfig,
                            const Projection& projection,
                            const MapParameter& parameter,
                            const NodeRef& node)
  {
    IconStyleRef iconStyle;

    styleConfig.GetNodeTextStyles(node->GetFeatureValueBuffer(),
                                 projection,
                                 textStyles);
    styleConfig.GetNodeIconStyle(node->GetFeatureValueBuffer(),
                                 projection,
                                 iconStyle);

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

    nodesDrawn++;
  }

  void MapPainter::DrawWay(const StyleConfig& /*styleConfig*/,
                           const Projection& projection,
                           const MapParameter& parameter,
                           const WayData& data)
  {
    Color color=data.lineStyle->GetLineColor();

    if (data.lineStyle->HasDashes() &&
        data.lineStyle->GetGapColor().GetA()>0.0) {
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
             color,
             data.lineWidth,
             data.lineStyle->GetDash(),
             data.startIsClosed ? data.lineStyle->GetEndCap() : data.lineStyle->GetJoinCap(),
             data.endIsClosed ? data.lineStyle->GetEndCap() : data.lineStyle->GetJoinCap(),
             data.transStart,data.transEnd);

    waysDrawn++;
  }

  void MapPainter::DrawWays(const StyleConfig& styleConfig,
                            const Projection& projection,
                            const MapParameter& parameter)
  {
    for (const auto& way : wayData) {
      DrawWay(styleConfig,
              projection,
              parameter,
              way);
    }
  }

  void MapPainter::DrawWayDecorations(const StyleConfig& styleConfig,
                                      const Projection& projection,
                                      const MapParameter& parameter,
                                      const MapData& /*data*/)
  {
    for (const auto way: wayPathData)
    {
      PathSymbolStyleRef pathSymbolStyle;

      styleConfig.GetWayPathSymbolStyle(*way.buffer,
                                        projection,
                                        pathSymbolStyle);

      if (!pathSymbolStyle) {
        continue;
      }

      double lineOffset=0.0;
      size_t transStart=way.transStart;
      size_t transEnd=way.transEnd;
      double symbolSpace=projection.ConvertWidthToPixel(pathSymbolStyle->GetSymbolSpace());

      if (pathSymbolStyle->GetOffset()!=0.0) {
        lineOffset+=GetProjectedWidth(projection,
                                      pathSymbolStyle->GetOffset());
      }

      if (pathSymbolStyle->GetDisplayOffset()!=0.0) {
        lineOffset+=projection.ConvertWidthToPixel(pathSymbolStyle->GetDisplayOffset());
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
                        *pathSymbolStyle->GetSymbol(),
                        symbolSpace,
                        transStart,transEnd);
    }
  }


  void MapPainter::DrawWayShieldLabel(const StyleConfig& styleConfig,
                                      const Projection& projection,
                                      const MapParameter& parameter,
                                      const Way& data)
  {
    PathShieldStyleRef shieldStyle;

    styleConfig.GetWayPathShieldStyle(data.GetFeatureValueBuffer(),
                                      projection,
                                      shieldStyle);

    if (!shieldStyle) {
      return;
    }

    std::string shieldLabel=shieldStyle->GetLabel()->GetLabel(parameter,
                                                              data.GetFeatureValueBuffer());

    if (shieldLabel.empty()) {
      return;
    }


    RegisterPointWayLabel(projection,
                          parameter,
                          shieldStyle,
                          shieldLabel,
                          data.nodes);
    waysLabelDrawn++;
  }

  void MapPainter::DrawWayContourLabel(const StyleConfig& styleConfig,
                                       const Projection& projection,
                                       const MapParameter& parameter,
                                       const WayPathData& data)
  {
    PathTextStyleRef   pathTextStyle;

    styleConfig.GetWayPathTextStyle(*data.buffer,
                                    projection,
                                    pathTextStyle);

    if (!pathTextStyle) {
      return;
    }

    double      lineOffset=0.0;
    size_t      transStart=data.transStart;
    size_t      transEnd=data.transEnd;
    std::string textLabel=pathTextStyle->GetLabel()->GetLabel(parameter,
                                                              *data.buffer);

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

    if (!textLabel.empty()) {
      ContourLabelHelper helper(*this);

      DrawContourLabel(projection,
                       parameter,
                       *pathTextStyle,
                       textLabel,
                       transStart,
                       transEnd,
                       helper);
      waysLabelDrawn++;
    }
  }

  void MapPainter::DrawWayShieldLabels(const StyleConfig& styleConfig,
                                       const Projection& projection,
                                       const MapParameter& parameter,
                                       const MapData& data)
  {
    for (const auto& way : data.ways) {
      DrawWayShieldLabel(styleConfig,
                         projection,
                         parameter,
                         *way);
    }

    for (const auto& way : data.poiWays) {
      DrawWayShieldLabel(styleConfig,
                         projection,
                         parameter,
                         *way);
    }
  }

  void MapPainter::DrawWayContourLabels(const StyleConfig& styleConfig,
                                        const Projection& projection,
                                        const MapParameter& parameter)
  {
    for (const auto& way : wayPathData) {
      DrawWayContourLabel(styleConfig,
                          projection,
                          parameter,
                          way);
    }
  }

  void MapPainter::DrawPOINodes(const StyleConfig& styleConfig,
                                const Projection& projection,
                                const MapParameter& parameter,
                                const MapData& data)
  {
    for (const auto& node : data.poiNodes) {
      DrawNode(styleConfig,
               projection,
               parameter,
               node);
    }
  }

  void MapPainter::DrawLabels(const StyleConfig& /*styleConfig*/,
                              const Projection& projection,
                              const MapParameter& parameter)
  {
    //
    // Draw normal
    //

    for (const auto& label : labels) {
      //std::cout << "Drawing label: " << label.text << std::endl;
      DrawLabel(projection,
                parameter,
                label);
      labelsDrawn++;
    }

    //
    // Draw overlays
    //

    for (const auto& label : overlayLabels) {
      //std::cout << "Drawing overlay: " << label.text << std::endl;
      DrawLabel(projection,
                parameter,
                label);
      labelsDrawn++;
    }
  }

  void MapPainter::DrawOSMTiles(const StyleConfig& /*styleConfig*/,
                                const Projection& projection,
                                const MapParameter& parameter,
                                const Magnification& magnification,
                                const LineStyleRef& osmTileLine)
  {
    GeoBox boundingBox;

    projection.GetDimensions(boundingBox);

    osmscout::OSMTileId     tileA(OSMTileId::GetOSMTile(boundingBox.GetMinCoord(),
                                                        magnification));
    osmscout::OSMTileId     tileB(OSMTileId::GetOSMTile(boundingBox.GetMaxCoord(),
                                                        magnification));
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

    for (size_t y=startTileY; y<=endTileY; y++) {
      points.resize(endTileX-startTileX+1);

      for (size_t x=startTileX; x<=endTileX; x++) {
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
      data.wayPriority=std::numeric_limits<int>::max();
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

    for (size_t x=startTileX; x<=endTileX; x++) {
      points.resize(endTileY-startTileY+1);

      for (size_t y=startTileY; y<=endTileY; y++) {
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
      data.wayPriority=std::numeric_limits<int>::max();
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

  void MapPainter::DrawOSMTiles(const StyleConfig& styleConfig,
                                const Projection& projection,
                                const MapParameter& parameter)
  {
    LineStyleRef osmSubTileLine;

    styleConfig.GetOSMSubTileBorderLineStyle(projection,
                                             osmSubTileLine);

    if (osmSubTileLine) {
      Magnification magnification=projection.GetMagnification();

      magnification.SetLevel(magnification.GetLevel()+1);

      DrawOSMTiles(styleConfig,
                   projection,
                   parameter,
                   magnification,
                   osmSubTileLine);
    }

    LineStyleRef osmTileLine;

    styleConfig.GetOSMTileBorderLineStyle(projection,
                                          osmTileLine);

    if (osmTileLine) {
      Magnification magnification=projection.GetMagnification();

      magnification.SetLevel(magnification.GetLevel());

      DrawOSMTiles(styleConfig,
                   projection,
                   parameter,
                   magnification,
                   osmTileLine);
    }
  }

  void MapPainter::PrepareArea(const StyleConfig& styleConfig,
                               const Projection& projection,
                               const MapParameter& parameter,
                               const AreaRef &area)
  {
    std::vector<PolyData> td(area->rings.size());

    for (size_t i=0; i<area->rings.size(); i++) {
      // The master ring does not have any nodes, skipping...
      if (area->rings[i].IsMasterRing() || area->rings[i].nodes.size() < 2) {
        continue;
      }

      transBuffer.TransformArea(projection,
                                parameter.GetOptimizeAreaNodes(),
                                area->rings[i].nodes,
                                td[i].transStart,td[i].transEnd,
                                errorTolerancePixel);
    }

    size_t ringId=Area::outerRingId;
    bool foundRing=true;

    while (foundRing) {
      foundRing=false;

      for (size_t i=0; i<area->rings.size(); i++) {
        const Area::Ring& ring=area->rings[i];

        if (ring.GetRing()!=ringId) {
          continue;
        }

        if (!ring.IsOuterRing() &&
            ring.GetType()->GetIgnore()) {
          continue;
        }

        TypeInfoRef                 type;
        FillStyleRef                fillStyle;
        std::vector<BorderStyleRef> borderStyles;
        BorderStyleRef              borderStyle;

        if (ring.IsOuterRing()) {
          type=area->GetType();
        }
        else {
          type=ring.GetType();
        }

        styleConfig.GetAreaFillStyle(type,
                                     ring.GetFeatureValueBuffer(),
                                     projection,
                                     fillStyle);

        styleConfig.GetAreaBorderStyles(type,
                                        ring.GetFeatureValueBuffer(),
                                        projection,
                                        borderStyles);

        if (!fillStyle && borderStyles.empty()) {
          continue;
        }

        size_t borderStyleIndex=0;

        if (!borderStyles.empty() &&
            borderStyles.front()->GetDisplayOffset()==0.0 &&
            borderStyles.front()->GetOffset()==0.0) {
          borderStyle=borderStyles[borderStyleIndex];
          borderStyleIndex++;
        }

        foundRing=true;

        AreaData a;
        double   borderWidth=borderStyle ? borderStyle->GetWidth() : 0.0;

        ring.GetBoundingBox(a.boundingBox);

        if (!IsVisibleArea(projection,
                           a.boundingBox,
                           borderWidth/2.0)) {
          continue;
        }

        // Collect possible clippings. We only take into account inner rings of the next level
        // that do not have a type and thus act as a clipping region. If a inner ring has a type,
        // we currently assume that it does not have alpha and paints over its region and clipping is
        // not required.
        // Since we know that rings a created deep first, we only take into account direct followers
        // in the list with ring+1.
        size_t j=i+1;
        while (j<area->rings.size() &&
               area->rings[j].GetRing()==ringId+1 &&
               area->rings[j].GetType()->GetIgnore()) {
          a.clippings.push_back(td[j]);

          j++;
        }

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

        areasSegments++;
      }

      ringId++;
    }
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

  void MapPainter::PrepareWay(const StyleConfig& styleConfig,
                              const Projection& projection,
                              const MapParameter& parameter,
                              const ObjectFileRef& ref,
                              const FeatureValueBuffer& buffer,
                              const std::vector<Point>& nodes)
  {
    styleConfig.GetWayLineStyles(buffer,
                                 projection,
                                 lineStyles);

    if (lineStyles.empty()) {
      return;
    }

    bool   transformed=false;
    size_t transStart=0; // Make the compiler happy
    size_t transEnd=0;   // Make the compiler happy

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

      if (lineWidth==0.0) {
        continue;
      }

      if (lineStyle->GetOffset()!=0.0) {
        lineOffset+=GetProjectedWidth(projection,
                                      lineStyle->GetOffset());
      }

      if (lineStyle->GetDisplayOffset()!=0.0) {
        lineOffset+=projection.ConvertWidthToPixel(lineStyle->GetDisplayOffset());
      }

      WayData data;

      data.ref=ref;
      data.lineWidth=lineWidth;

      if (!IsVisibleWay(projection,
                        nodes,
                        lineWidth/2)) {
        continue;
      }

      if (!transformed) {
        transBuffer.TransformWay(projection,
                                 parameter.GetOptimizeWayNodes(),
                                 nodes,
                                 transStart,
                                 transEnd,
                                 errorTolerancePixel);

        WayPathData pathData;

        pathData.ref=ref;
        pathData.buffer=&buffer;
        pathData.transStart=transStart;
        pathData.transEnd=transEnd;

        wayPathData.push_back(pathData);

        transformed=true;
      }

      data.layer=0;
      data.buffer=&buffer;
      data.lineStyle=lineStyle;
      data.wayPriority=styleConfig.GetWayPrio(buffer.GetType());
      data.startIsClosed=nodes[0].GetSerial()==0;
      data.endIsClosed=nodes[nodes.size()-1].GetSerial()==0;

      LayerFeatureValue *layerValue=layerReader.GetValue(buffer);

      if (layerValue!=NULL) {
        data.layer=layerValue->GetLayer();
      }

      if (lineOffset!=0.0) {
        coordBuffer->GenerateParallelWay(transStart,transEnd,
                                         lineOffset,
                                         data.transStart,
                                         data.transEnd);
      }
      else {
        data.transStart=transStart;
        data.transEnd=transEnd;
      }

      waysSegments++;
      wayData.push_back(data);
    }
  }

  void MapPainter::PrepareWays(const StyleConfig& styleConfig,
                               const Projection& projection,
                               const MapParameter& parameter,
                               const MapData& data)
  {
    wayData.clear();
    wayPathData.clear();

    for (const auto& way : data.ways) {
      PrepareWay(styleConfig,
                 projection,
                 parameter,
                 ObjectFileRef(way->GetFileOffset(),refWay),
                 way->GetFeatureValueBuffer(),
                 way->nodes);
    }

    for (const auto& way : data.poiWays) {
      PrepareWay(styleConfig,
                 projection,
                 parameter,
                 ObjectFileRef(way->GetFileOffset(),refWay),
                 way->GetFeatureValueBuffer(),
                 way->nodes);
    }

    wayData.sort();
  }

  bool MapPainter::Draw(const Projection& projection,
                        const MapParameter& parameter,
                        const MapData& data)
  {
    errorTolerancePixel=projection.ConvertWidthToPixel(parameter.GetOptimizeErrorToleranceMm());
    areaMinDimension   =projection.ConvertWidthToPixel(parameter.GetAreaMinDimensionMM());
    contourLabelOffset =projection.ConvertWidthToPixel(parameter.GetContourLabelOffset());
    contourLabelSpace  =projection.ConvertWidthToPixel(parameter.GetContourLabelSpace());

    shieldGridSizeHoriz=360.0/(std::pow(2,projection.GetMagnification().GetLevel()+1));
    shieldGridSizeVert=180.0/(std::pow(2,projection.GetMagnification().GetLevel()+1));

    waysSegments  =0;
    waysDrawn     =0;
    waysLabelDrawn=0;

    areasSegments=0;
    areasDrawn   =0;

    nodesDrawn=0;

    labelsDrawn=0;

    nextLabelId=0;
    labels.Initialize(projection,
                      parameter);
    overlayLabels.Initialize(projection,
                             parameter);

    transBuffer.Reset();

    standardFontSize=GetFontHeight(projection,
                                   parameter,
                                   1.0);

    if (parameter.IsAborted()) {
      return false;
    }

    if (parameter.IsDebugPerformance()) {
      GeoBox boundingBox;

      projection.GetDimensions(boundingBox);

      log.Info()
        << "Draw: " << boundingBox.GetDisplayText() << " "
        << (int) projection.GetMagnification().GetMagnification() << "x" << "/"
        << projection.GetMagnification().GetLevel() << " "
        << projection.GetWidth() << "x" << projection.GetHeight() << " " << projection.GetDPI() << " DPI";
    }

    if (parameter.GetWarningCoordCountLimit()>0 ||
        parameter.GetWarningObjectCountLimit()>0 ||
        parameter.IsDebugData()) {
      DumpDataStatistics(projection,
                         parameter,
                         data);
    }

    //
    // Setup and Precalculation
    //

    StopClock prepareAreasTimer;

    PrepareAreas(*styleConfig,
                 projection,
                 parameter,
                 data);

    prepareAreasTimer.Stop();

    if (parameter.IsAborted()) {
      return false;
    }

    StopClock prepareWaysTimer;

    PrepareWays(*styleConfig,
                projection,
                parameter,
                data);

    prepareWaysTimer.Stop();

    if (parameter.IsAborted()) {
      return false;
    }

    // Optional callback after preprocessing data
    AfterPreprocessing(*styleConfig,
                       projection,
                       parameter,
                       data);

    // Optional callback after preprocessing data
    BeforeDrawing(*styleConfig,
                  projection,
                  parameter,
                  data);

    //
    // Clear area with background color
    //

    DrawGroundTiles(*styleConfig,
                    projection,
                    parameter,
                    data);

    if (parameter.IsAborted()) {
      return false;
    }

    DrawOSMTiles(*styleConfig,
                 projection,
                 parameter);

    if (parameter.IsAborted()) {
      return false;
    }

    //
    // Draw areas
    //

    StopClock areasTimer;

    DrawAreas(*styleConfig,
              projection,
              parameter);

    areasTimer.Stop();

    if (parameter.IsAborted()) {
      return false;
    }

    //
    // Drawing ways
    //

    StopClock pathsTimer;

    DrawWays(*styleConfig,
             projection,
             parameter);

    pathsTimer.Stop();

    if (parameter.IsAborted()) {
      return false;
    }

    //
    // Path decorations (like arrows and similar)
    //

    StopClock pathDecorationsTimer;

    DrawWayDecorations(*styleConfig,
                       projection,
                       parameter,
                       data);

    pathDecorationsTimer.Stop();

    if (parameter.IsAborted()) {
      return false;
    }

    //
    // Path labels
    //

    // TODO: Draw labels only if there is a style for the current zoom level
    // that requires labels

    StopClock pathShieldLabelsTimer;

    DrawWayShieldLabels(*styleConfig,
                        projection,
                        parameter,
                        data);

    pathShieldLabelsTimer.Stop();

    if (parameter.IsAborted()) {
      return false;
    }

    StopClock pathContourLabelsTimer;

    DrawWayContourLabels(*styleConfig,
                         projection,
                         parameter);

    pathContourLabelsTimer.Stop();

    if (parameter.IsAborted()) {
      return false;
    }

    //
    // Nodes symbols & Node labels
    //

    StopClock nodesTimer;

    DrawNodes(*styleConfig,
              projection,
              parameter,
              data);

    nodesTimer.Stop();

    if (parameter.IsAborted()) {
      return false;
    }

    //
    // Area labels
    //

    StopClock areaLabelsTimer;

    DrawAreaLabels(*styleConfig,
                   projection,
                   parameter);

    areaLabelsTimer.Stop();

    if (parameter.IsAborted()) {
      return false;
    }

    //
    // POI Nodes
    //

    StopClock poisTimer;

    DrawPOINodes(*styleConfig,
                 projection,
                 parameter,
                 data);

    poisTimer.Stop();

    if (parameter.IsAborted()) {
      return false;
    }

    StopClock labelsTimer;

    DrawLabels(*styleConfig,
               projection,
               parameter);

    labelsTimer.Stop();

    AfterDrawing(*styleConfig,
                 projection,
                 parameter,
                 data);

    if (parameter.IsDebugPerformance()) {
      log.Info()
          << "Paths: "
          << data.ways.size() << "/" << waysSegments << "/" << waysDrawn << "/" << waysLabelDrawn << " (pcs) "
          << prepareWaysTimer << "/" << pathsTimer << "/" << pathShieldLabelsTimer << "/" << pathContourLabelsTimer << " (sec)";

      log.Info()
          << "Areas: "
          << data.areas.size() << "/" << areasSegments << "/" << areasDrawn << " (pcs) "
          << prepareAreasTimer << "/" << areasTimer << "/" << areaLabelsTimer << " (sec)";

      log.Info()
          << "Nodes: "
          << data.nodes.size() <<"+" << data.poiNodes.size() << "/" << nodesDrawn << " (pcs) "
          << nodesTimer << "/" << poisTimer << " (sec)";

      log.Info()
          << "Labels: " << labels.Size() << "/" << labels.GetLabelsAdded() << " " << overlayLabels.Size() << "/" << overlayLabels.GetLabelsAdded() << " " << labelsDrawn << " (pcs) "
          << labelsTimer << " (sec)";
    }

    return true;
  }
}
