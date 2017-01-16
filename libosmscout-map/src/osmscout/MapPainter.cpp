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

//#define DEBUG_GROUNDTILES
//#define DEBUG_NODE_DRAW

#if defined(DEBUG_GROUNDTILES)
#include <osmscout/Coord.h>
#endif

namespace osmscout {

  /**
   * Return if a > b, a should be drawn before b
   */
  static inline bool AreaSorter(const MapPainter::AreaData& a, const MapPainter::AreaData& b)
  {
    if (a.fillStyle->GetFillColor().IsSolid() && !b.fillStyle->GetFillColor().IsSolid()) {
      return true;
    }
    else if (!a.fillStyle->GetFillColor().IsSolid() && b.fillStyle->GetFillColor().IsSolid()) {
      return false;
    }

    if (a.boundingBox.minCoord.GetLon()==b.boundingBox.minCoord.GetLon()) {
      if (a.boundingBox.maxCoord.GetLon()==b.boundingBox.maxCoord.GetLon()) {
        if (a.boundingBox.minCoord.GetLat()==b.boundingBox.minCoord.GetLat()) {
          return a.boundingBox.maxCoord.GetLat()>b.boundingBox.maxCoord.GetLat();
        }
        else {
          return a.boundingBox.minCoord.GetLat()<b.boundingBox.minCoord.GetLat();
        }
      }
      else {
        return a.boundingBox.maxCoord.GetLon()>b.boundingBox.maxCoord.GetLon();
      }
    }
    else {
      return a.boundingBox.minCoord.GetLon()<b.boundingBox.minCoord.GetLon();
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
    textStyle->SetTextColor(Color(0,0,0,0.5));
    textStyle->SetSize(1.2);

    debugLabel=textStyle;
  }

  MapPainter::~MapPainter()
  {
    log.Debug() << "MapPainter::~MapPainter()";
  }

  void MapPainter::DumpDataStatistics(const Projection& projection,
                                      const MapData& data)
  {
    std::map<TypeInfoRef,DataStatistic> statistics;
    TypeInfoSet                         types;

    // Prefilling all possible types from style sheet

    styleConfig->GetNodeTypesWithMaxMag(projection.GetMagnification(),
                                        types);

    for (const auto& type : types) {
      /* ignore */ statistics[type];
    }

    styleConfig->GetWayTypesWithMaxMag(projection.GetMagnification(),
                                       types);

    for (const auto& type : types) {
      /* ignore */ statistics[type];
    }

    styleConfig->GetAreaTypesWithMaxMag(projection.GetMagnification(),
                                        types);

    for (const auto& type : types) {
      /* ignore */ statistics[type];
    }

    // Now analyse the actual data

    for (const auto& node : data.nodes) {
      DataStatistic& entry=statistics[node->GetType()];

      entry.nodeCount++;
      entry.coordCount++;

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

    for (const auto& node : data.poiNodes) {
      DataStatistic& entry=statistics[node->GetType()];

      entry.nodeCount++;
      entry.coordCount++;

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

    for (const auto& way : data.ways) {
      DataStatistic& entry=statistics[way->GetType()];

      entry.wayCount++;
      entry.coordCount+=way->nodes.size();

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

    for (const auto& way : data.poiWays) {
      DataStatistic& entry=statistics[way->GetType()];

      entry.wayCount++;
      entry.coordCount+=way->nodes.size();

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

    for (const auto& area : data.areas) {
      DataStatistic& entry=statistics[area->GetType()];

      entry.areaCount++;

      for (const auto& ring : area->rings) {
        entry.coordCount+=ring.nodes.size();

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

    for (const auto& area : data.poiAreas) {
      DataStatistic& entry=statistics[area->GetType()];

      entry.areaCount++;

      for (const auto& ring : area->rings) {
        entry.coordCount+=ring.nodes.size();

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

    std::list<DataStatistic> statisticList;

    for (auto& entry : statistics) {
      entry.second.type=entry.first;

      entry.second.objectCount=entry.second.nodeCount+entry.second.wayCount+entry.second.areaCount;

      statisticList.push_back(entry.second);
    }

    statistics.clear();

    statisticList.sort([](const DataStatistic& a, const DataStatistic& b)->bool{return a.objectCount>b.objectCount;});

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

  bool MapPainter::IsVisibleArea(const Projection& projection,
                                 const GeoBox& boundingBox,
                                 double pixelOffset) const
  {
    double x1;
    double x2;
    double y1;
    double y2;

    projection.GeoToPixel(boundingBox.GetMinCoord(),
                          x1,
                          y1);

    projection.GeoToPixel(boundingBox.GetMaxCoord(),
                          x2,
                          y2);

    double xMin=std::min(x1,x2)-pixelOffset;
    double xMax=std::max(x1,x2)+pixelOffset;
    double yMin=std::min(y1,y2)-pixelOffset;
    double yMax=std::max(y1,y2)+pixelOffset;

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

    double x1;
    double x2;
    double y1;
    double y2;

    projection.GeoToPixel(boundingBox.GetMinCoord(),
                          x1,
                          y1);

    projection.GeoToPixel(boundingBox.GetMaxCoord(),
                          x2,
                          y2);

    double xMin=std::min(x1,x2)-pixelOffset;
    double xMax=std::max(x1,x2)+pixelOffset;
    double yMin=std::min(y1,y2)-pixelOffset;
    double yMax=std::max(y1,y2)+pixelOffset;

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

    if (!coastFill) {
      coastFill=this->seaFill;
    }

    if (!unknownFill) {
      unknownFill=this->seaFill;
    }

    for (std::list<GroundTile>::const_iterator tile=data.groundTiles.begin();
        tile!=data.groundTiles.end();
        ++tile) {
      AreaData areaData;

      if (tile->type==GroundTile::unknown && !parameter.GetRenderUnknowns()){
        continue;
      }

      switch (tile->type) {
      case GroundTile::land:
        areaData.fillStyle=landFill;
        break;
      case GroundTile::water:
        areaData.fillStyle=seaFill;
        break;
      case GroundTile::coast:
        areaData.fillStyle=coastFill;
        break;
      case GroundTile::unknown:
        areaData.fillStyle=unknownFill;
        break;
      }

      GeoCoord minCoord(tile->yAbs*tile->cellHeight-90.0,
                        tile->xAbs*tile->cellWidth-180.0);
      GeoCoord maxCoord(minCoord.GetLat()+tile->cellHeight,
                        minCoord.GetLon()+tile->cellWidth);

      areaData.boundingBox.Set(minCoord,maxCoord);

      if (tile->coords.empty()) {
        points.resize(5);

        points[0].SetCoord(areaData.boundingBox.minCoord);
        points[1].SetCoord(GeoCoord(areaData.boundingBox.minCoord.GetLat(),areaData.boundingBox.maxCoord.GetLon()));
        points[2].SetCoord(areaData.boundingBox.maxCoord);
        points[3].SetCoord(GeoCoord(areaData.boundingBox.maxCoord.GetLat(),areaData.boundingBox.minCoord.GetLon()));
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
        points.resize(tile->coords.size());

        for (size_t i=0; i<tile->coords.size(); i++) {
          double lat;
          double lon;

          lat=areaData.boundingBox.minCoord.GetLat()+tile->coords[i].y*tile->cellHeight/GroundTile::Coord::CELL_MAX;
          lon=areaData.boundingBox.minCoord.GetLon()+tile->coords[i].x*tile->cellWidth/GroundTile::Coord::CELL_MAX;

          points[i].SetCoord(GeoCoord(lat,lon));
        }

        transBuffer.transPolygon.TransformArea(projection,
                                               TransPolygon::none,
                                               points,
                                               errorTolerancePixel);

        for (size_t i=transBuffer.transPolygon.GetStart(); i<=transBuffer.transPolygon.GetEnd(); i++) {
          double x,y;

          if (tile->coords[i].x==0) {
            x=floor(transBuffer.transPolygon.points[i].x);
          }
          else if (tile->coords[i].x==GroundTile::Coord::CELL_MAX) {
            x=ceil(transBuffer.transPolygon.points[i].x);
          }
          else {
            x=transBuffer.transPolygon.points[i].x;
          }

          if (tile->coords[i].y==0) {
            y=ceil(transBuffer.transPolygon.points[i].y);
          }
          else if (tile->coords[i].y==GroundTile::Coord::CELL_MAX) {
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

          while (lineStart<tile->coords.size()) {
            while (lineStart<tile->coords.size() &&
                   !tile->coords[lineStart].coast) {
              lineStart++;
            }

            if (lineStart>=tile->coords.size()) {
              continue;
            }

            lineEnd=lineStart;

            while (lineEnd<tile->coords.size() &&
                   tile->coords[lineEnd].coast) {
              lineEnd++;
            }

            if (lineStart!=lineEnd) {
              WayData data;

              data.buffer=&coastlineSegmentAttributes;
              data.layer=0;
              data.lineStyle=coastlineLine;
              data.wayPriority=std::numeric_limits<int>::max();
              data.transStart=start+lineStart;
              data.transEnd=start+lineEnd;
              data.lineWidth=GetProjectedWidth(projection,
                                               projection.ConvertWidthToPixel(coastlineLine->GetDisplayWidth()),
                                               coastlineLine->GetWidth());
              data.startIsClosed=false;
              data.endIsClosed=false;
              wayData.push_back(data);
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
      GeoCoord cc=areaData.boundingBox.GetCenter();

      std::string label;

      size_t x=(cc.GetLon()+180)/tile->cellWidth;
      size_t y=(cc.GetLat()+90)/tile->cellHeight;

      label=NumberToString(tile->xRel);
      label+=",";
      label+=NumberToString(tile->yRel);

      double lon=(x*tile->cellWidth+tile->cellWidth/2)-180.0;
      double lat=(y*tile->cellHeight+tile->cellHeight/2)-90.0;

      double px;
      double py;

      projection.GeoToPixel(GeoCoord(lat,lon),
                            px,py);


      if (drawnLabels.find(GeoCoord(x,y))!=drawnLabels.end()) {
        continue;
      }

      LabelData labelData;

      labelData.id=nextLabelId++;
      labelData.x=px;
      labelData.y=py;
      labelData.alpha=0.5;
      labelData.fontSize=1.2;
      labelData.style=debugLabel;
      labelData.text=label;

      LabelDataRef ref;
      labels.Placelabel(labelData, ref);

      drawnLabels.insert(GeoCoord(x,y));
#endif
    }
  }

  void MapPainter::RegisterPointWayLabel(const Projection& projection,
                                         const MapParameter& parameter,
                                         const PathShieldStyleRef& shieldStyle,
                                         const std::string& text,
                                         size_t transStart, size_t transEnd)
  {
    double               fontHeight;
    const LabelStyleRef& style=shieldStyle->GetShieldStyle();

    GetFontHeight(projection,
                  parameter,
                  style->GetSize(),
                  fontHeight);

    wayScanlines.clear();

    size_t               stepSizeInPixel=(size_t)projection.ConvertWidthToPixel(2*fontHeight/*shieldStyle->GetShieldSpace()*/);

    transBuffer.buffer->ScanConvertLine(transStart,
                                        transEnd,
                                        wayScanlines);

    double frameHoriz=5;
    double frameVert=5;

    double xOff,yOff,width,height;

    GetTextDimension(projection,
                     parameter,
                     style->GetSize(),
                     text,
                     xOff,yOff,width,height);

    size_t i=0;
    while (i<wayScanlines.size()) {
      LabelData labelBox;
      double    x=wayScanlines[i].x+0.5;
      double    y=wayScanlines[i].y+0.5;

      labelBox.id=nextLabelId++;
      labelBox.bx1=x-width/2-frameHoriz;
      labelBox.bx2=x+width/2+frameHoriz;
      labelBox.by1=y-height/2-frameVert;
      labelBox.by2=y+height/2+frameVert;
      labelBox.priority=style->GetPriority();
      labelBox.x=x-xOff-width/2;
      labelBox.y=y-yOff-height/2;
      labelBox.alpha=1.0;
      labelBox.fontSize=style->GetSize();
      labelBox.style=style;
      labelBox.text=text;

      LabelDataRef label;

      labels.Placelabel(labelBox,
                        label);

      i+=stepSizeInPixel;
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
    labelBox.bx1=x-data.width/2;
    labelBox.bx2=x+data.width/2;
    labelBox.by1=y-data.height/2;
    labelBox.by2=y+data.height/2;
    labelBox.priority=data.textStyle->GetPriority();
    labelBox.x=x-data.xOff-data.width/2;
    labelBox.y=y-data.yOff-data.height/2;
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
                                     const IconStyleRef iconStyle,
                                     const std::vector<TextStyleRef>& textStyles,
                                     double x,
                                     double y,
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
        data.xOff=0;
        data.yOff=0;
        data.width=14;  // TODO
        data.height=14; // TODO
        data.icon=true;
        data.iconStyle=iconStyle;

        hasSymbol=true;

        labelLayoutData.push_back(data);
      }
      else if (iconStyle->GetSymbol()) {
        LabelLayoutData data;

        data.position=iconStyle->GetPosition();
        data.xOff=0;
        data.yOff=0;
        data.width=projection.ConvertWidthToPixel(iconStyle->GetSymbol()->GetWidth());
        data.height=projection.ConvertWidthToPixel(iconStyle->GetSymbol()->GetHeight());
        data.icon=false;
        data.iconStyle=iconStyle;

        hasSymbol=true;

        labelLayoutData.push_back(data);
      }
    }

    for (const auto textStyle : textStyles) {
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

        GetTextDimension(projection,
                        parameter,
                        data.fontSize,
                        label,
                        data.xOff,
                        data.yOff,
                        data.width,
                        data.height);

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

        GetTextDimension(projection,
                         parameter,
                         data.fontSize,
                         label,
                         data.xOff,
                         data.yOff,
                         data.width,
                         data.height);
      }
      else {
        data.fontSize=textStyle->GetSize();

        GetTextDimension(projection,
                         parameter,
                         data.fontSize,
                         label,
                         data.xOff,
                         data.yOff,
                         data.width,
                         data.height);

        data.alpha=textStyle->GetAlpha();
      }

      data.position=textStyle->GetPosition();
      data.label=label;
      data.textStyle=textStyle;
      data.icon=false;

      overallTextHeight+=data.height;

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
                           x,offset+data.height/2,
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

      offset+=data.height;
    }
    //std::cout << "<<<" << std::endl;
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

    DrawContourLabel(projection,
                     parameter,
                     *borderTextStyle,
                     label,
                     transStart,
                     transEnd);
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
                      x,y,0);

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
                                      const WayPathData& data)
  {
    PathShieldStyleRef shieldStyle;

    styleConfig.GetWayPathShieldStyle(*data.buffer,
                                      projection,
                                      shieldStyle);

    if (shieldStyle) {
      std::string shieldLabel=shieldStyle->GetLabel()->GetLabel(parameter,
                                                                *data.buffer);

      if (!shieldLabel.empty()) {
        RegisterPointWayLabel(projection,
                              parameter,
                              shieldStyle,
                              shieldLabel,
                              data.transStart,
                              data.transEnd);
        waysLabelDrawn++;
      }
    }
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
      DrawContourLabel(projection,
                       parameter,
                       *pathTextStyle,
                       textLabel,
                       transStart,
                       transEnd);
      waysLabelDrawn++;
    }
  }

  void MapPainter::DrawWayShieldLabels(const StyleConfig& styleConfig,
                                       const Projection& projection,
                                       const MapParameter& parameter)
  {
    for (const auto& way : wayPathData) {
      DrawWayShieldLabel(styleConfig,
                         projection,
                         parameter,
                         way);
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

    size_t startTileX=LonToTileX(boundingBox.GetMinLon(),
                                 magnification);
    size_t endTileX=LonToTileX(boundingBox.GetMaxLon(),
                               magnification)+1;

    size_t startTileY=LatToTileY(boundingBox.GetMaxLat(),
                                 magnification);
    size_t endTileY=LatToTileY(boundingBox.GetMinLat(),
                               magnification)+1;

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
        points[x-startTileX].Set(0,GeoCoord(TileYToLat(y,magnification),
                                            TileXToLon(x,magnification)));
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
        points[y-startTileY].Set(0,GeoCoord(TileYToLat(y,magnification),
                                            TileXToLon(x,magnification)));
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

  void MapPainter::PrepareAreas(const StyleConfig& styleConfig,
                                const Projection& projection,
                                const MapParameter& parameter,
                                const MapData& data)
  {
    areaData.clear();

    //Areas
    for (const auto& area : data.areas) {
      std::vector<PolyData> data(area->rings.size());

      for (size_t i=0; i<area->rings.size(); i++) {
        // The master ring does not have any nodes, skipping...
        if (area->rings[i].IsMasterRing()) {
          continue;
        }

        transBuffer.TransformArea(projection,
                                  parameter.GetOptimizeAreaNodes(),
                                  area->rings[i].nodes,
                                  data[i].transStart,data[i].transEnd,
                                  errorTolerancePixel);
      }

      size_t ringId=Area::outerRingId;
      bool foundRing=true;

      while (foundRing) {
        foundRing=false;

        for (size_t i=0; i<area->rings.size(); i++) {
          const Area::Ring& ring=area->rings[i];

          if (ring.GetRing()==ringId) {
            TypeInfoRef  type;
            FillStyleRef fillStyle;

            if (ring.IsOuterRing()) {
              type=area->GetType();
            }
            else if (!ring.GetType()->GetIgnore()) {
              type=ring.GetType();
            }
            else {
              continue;
            }

            if (type->GetIgnore()) {
              continue;
            }

            styleConfig.GetAreaFillStyle(type,
                                         ring.GetFeatureValueBuffer(),
                                         projection,
                                         fillStyle);

            if (!fillStyle) {
              continue;
            }

            foundRing=true;

            AreaData a;

            ring.GetBoundingBox(a.boundingBox);

            if (!IsVisibleArea(projection,
                               a.boundingBox,
                               fillStyle->GetBorderWidth()/2)) {
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
              a.clippings.push_back(data[j]);

              j++;
            }

            a.ref=area->GetObjectFileRef();
            a.type=type;
            a.buffer=&ring.GetFeatureValueBuffer();
            a.fillStyle=fillStyle;
            a.transStart=data[i].transStart;
            a.transEnd=data[i].transEnd;

            areaData.push_back(a);

            areasSegments++;
          }
        }

        ringId++;
      }
    }

    areaData.sort(AreaSorter);
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


        if (widthValue!=NULL) {
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
    areaMinDimension=projection.ConvertWidthToPixel(parameter.GetAreaMinDimensionMM());
    contourLabelOffset=projection.ConvertWidthToPixel(parameter.GetContourLabelOffset());
    contourLabelSpace=projection.ConvertWidthToPixel(parameter.GetContourLabelSpace());

    waysSegments=0;
    waysDrawn=0;
    waysLabelDrawn=0;

    areasSegments=0;
    areasDrawn=0;

    nodesDrawn=0;

    labelsDrawn=0;

    nextLabelId=0;
    labels.Initialize(projection,
                      parameter);
    overlayLabels.Initialize(projection,
                             parameter);

    transBuffer.Reset();

    GetFontHeight(projection,
                  parameter,
                  1.0,
                  standardFontSize);

    if (parameter.IsAborted()) {
      return false;
    }

    if (parameter.IsDebugPerformance()) {
      GeoBox boundingBox;

      projection.GetDimensions(boundingBox);

      log.Info()
          << "Draw: " << boundingBox.GetDisplayText() << " "
          << (int)projection.GetMagnification().GetMagnification() << "x" << "/" << projection.GetMagnification().GetLevel() << " "
          << projection.GetWidth() << "x" << projection.GetHeight() << " " << projection.GetDPI()<< " DPI";
    }

    if (parameter.IsDebugData()) {
      DumpDataStatistics(projection,
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
                        parameter);

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
