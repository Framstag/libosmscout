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

//#define DEBUG_GROUNDTILES

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

    if (a.minLon==b.minLon) {
      if (a.maxLon==b.maxLon) {
        if (a.minLat==b.minLat) {
          return a.maxLat>b.maxLat;
        }
        else {
          return a.minLat<b.minLat;
        }
      }
      else {
        return a.maxLon>b.maxLon;
      }
    }
    else {
      return a.minLon<b.minLon;
    }
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
    addressReader(*styleConfig->GetTypeConfig()),
    labelSpace(1.0),
    shieldLabelSpace(1.0),
    sameLabelSpace(1.0)
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

        if (ring.ring==Area::masterRingId) {
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

        if (ring.ring==Area::masterRingId) {
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

    log.Info() << "Type|NodeCount|WayCount|AreaCount|Nodes|Labels|Icons";
    for (const auto& entry : statisticList) {
      log.Info() << entry.type->GetName() << " "
          << entry.nodeCount << " " << entry.wayCount << " " << entry.areaCount << " " << entry.coordCount << " "
          << entry.labelCount << " " << entry.iconCount;
    }
  }

  bool MapPainter::IsVisible(const Projection& projection,
                             const std::vector<GeoCoord>& nodes,
                             double pixelOffset) const
  {
    if (nodes.empty()) {
      return false;
    }

    // Bounding box
    double lonMin=nodes[0].GetLon();
    double lonMax=lonMin;
    double latMin=nodes[0].GetLat();
    double latMax=latMin;

    for (size_t i=1; i<nodes.size(); i++) {
      lonMin=std::min(lonMin,nodes[i].GetLon());
      lonMax=std::max(lonMax,nodes[i].GetLon());
      latMin=std::min(latMin,nodes[i].GetLat());
      latMax=std::max(latMax,nodes[i].GetLat());
    }

    double x1;
    double x2;
    double y1;
    double y2;

    projection.GeoToPixel(lonMin,
                          latMin,
                          x1,
                          y1);

    projection.GeoToPixel(lonMax,
                          latMax,
                          x2,
                          y2);

    double xMin=std::min(x1,x2);
    double xMax=std::max(x1,x2);
    double yMin=std::min(y1,y2);
    double yMax=std::max(y1,y2);


    xMin-=pixelOffset;
    yMin-=pixelOffset;

    xMax+=pixelOffset;
    yMax+=pixelOffset;

    return !(xMin>=projection.GetWidth() ||
             yMin>=projection.GetHeight() ||
             xMax<0 ||
             yMax<0);
  }

  void MapPainter::ClearLabelMarks(std::list<LabelData>& labels)
  {
    for (std::list<LabelData>::iterator label=labels.begin();
         label!=labels.end();
         ++label) {
      label->mark=false;
    }
  }

  void MapPainter::RemoveMarkedLabels(std::list<LabelData>& labels)
  {
    std::list<LabelData>::iterator label=labels.begin();

    while (label!=labels.end()) {
      if (label->mark) {
        label=labels.erase(label);
      }
      else {
        ++label;
      }
    }
  }

  bool MapPainter::MarkAllInBoundingBox(double bx1,
                                        double bx2,
                                        double by1,
                                        double by2,
                                        const LabelStyle& style,
                                        std::list<LabelData>& labels)
  {
    for (auto& label : labels) {
      // We only look at labels, that are not already marked.
      if (label.mark) {
        continue;
      }

      double hx1;
      double hx2;
      double hy1;
      double hy2;

      double horizLabelSpace;
      double vertLabelSpace;

      GetLabelSpace(style,
                    *label.style,
                    horizLabelSpace,
                    vertLabelSpace);

      hx1=bx1-horizLabelSpace;
      hx2=bx2+horizLabelSpace;
      hy1=by1-vertLabelSpace;
      hy2=by2+vertLabelSpace;

      // Check for labels that intersect (including space). If our priority is lower,
      // we stop processing, else we mark the other label (as to be deleted)
      if (!(hx1>=label.bx2 ||
            hx2<=label.bx1 ||
            hy1>=label.by2 ||
            hy2<=label.by1)) {
        if (label.style->GetPriority()<=style.GetPriority()) {
          return false;
        }

        label.mark=true;
      }
    }

    return true;
  }

  bool MapPainter::MarkCloseLabelsWithSameText(double bx1,
                                               double bx2,
                                               double by1,
                                               double by2,
                                               const LabelStyle& style,
                                               const std::string& text,
                                               std::list<LabelData>& labels)
  {
    for (auto & label : labels) {
      if (label.mark) {
        continue;
      }

      if (dynamic_cast<const ShieldStyle*>(&style)!=NULL &&
          dynamic_cast<const ShieldStyle*>(label.style.get())!=NULL) {
        double hx1=bx1-sameLabelSpace;
        double hx2=bx2+sameLabelSpace;
        double hy1=by1-sameLabelSpace;
        double hy2=by2+sameLabelSpace;

        if (!(hx1>label.bx2 ||
              hx2<label.bx1 ||
              hy1>label.by2 ||
              hy2<label.by1)) {
          if (text==label.text) {
            // TODO: It may be possible that the labels belong to the same "thing".
            // perhaps we should not just draw one or the other, but also change
            // final position of the label (but this would require more complex
            // collision handling and perhaps processing labels in different order)?
            return false;
          }
        }
      }
    }

    return true;
  }

  void MapPainter::Transform(const Projection& projection,
                             const MapParameter& /*parameter*/,
                             double lon,
                             double lat,
                             double& x,
                             double& y)
  {
    projection.GeoToPixel(lon,lat,
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
      std::set<Coord> drawnLabels;
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

    FillStyleRef          seaFill;
    FillStyleRef          coastFill;
    FillStyleRef          unknownFill;
    LineStyleRef          coastlineLine;
    std::vector<GeoCoord> points;
    size_t                start=0; // Make the compiler happy
    size_t                end=0;   // Make the compiler happy
    double                errorTolerancePixel=parameter.GetOptimizeErrorToleranceMm()*projection.GetDPI()/25.4;

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

      areaData.minLat=tile->yAbs*tile->cellHeight-90.0;
      areaData.maxLat=areaData.minLat+tile->cellHeight;
      areaData.minLon=tile->xAbs*tile->cellWidth-180.0;
      areaData.maxLon=areaData.minLon+tile->cellWidth;

      if (tile->coords.empty()) {
        points.resize(5);

        points[0].Set(areaData.minLat,areaData.minLon);
        points[1].Set(areaData.minLat,areaData.maxLon);
        points[2].Set(areaData.maxLat,areaData.maxLon);
        points[3].Set(areaData.maxLat,areaData.minLon);
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

          lat=areaData.minLat+tile->coords[i].y*tile->cellHeight/GroundTile::Coord::CELL_MAX;
          lon=areaData.minLon+tile->coords[i].x*tile->cellWidth/GroundTile::Coord::CELL_MAX;

          points[i].Set(lat,lon);
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
              data.wayPriority=std::numeric_limits<size_t>::max();
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
      double ccLon=areaData.minLon+(areaData.maxLon-areaData.minLon)/2;
      double ccLat=areaData.minLat+(areaData.maxLat-areaData.minLat)/2;

      std::string label;

      size_t x=(ccLon+180)/tile->cellWidth;
      size_t y=(ccLat+90)/tile->cellHeight;

      label=NumberToString(tile->xRel);
      label+=",";
      label+=NumberToString(tile->yRel);

      double lon=(x*tile->cellWidth+tile->cellWidth/2)-180.0;
      double lat=(y*tile->cellHeight+tile->cellHeight/2)-90.0;

      double px;
      double py;

      projection.GeoToPixel(lon,
                            lat,
                            px,py);


      if (drawnLabels.find(Coord(x,y))!=drawnLabels.end()) {
        continue;
      }

      LabelData labelData;

      labelData.x=px;
      labelData.y=py;
      labelData.alpha=0.5;
      labelData.fontSize=1.2;
      labelData.style=debugLabel;
      labelData.text=label;

      labels.push_back(labelData);

      drawnLabels.insert(Coord(x,y));
#endif
    }
  }

  void MapPainter::RegisterPointWayLabel(const Projection& projection,
                                         const MapParameter& parameter,
                                         const PathShieldStyleRef& shieldStyle,
                                         const std::string& text,
                                         size_t transStart, size_t transEnd)
  {
    size_t stepSizeInPixel = (size_t)projection.ConvertWidthToPixel(shieldStyle->GetShieldSpace());
    double fontHeight;

    GetFontHeight(projection,
                  parameter,
                  1.0,
                  fontHeight);

    wayScanlines.clear();

    transBuffer.buffer->ScanConvertLine(transStart,
                                        transEnd,
                                        wayScanlines);

    size_t i=0;
    while (i<wayScanlines.size()) {
      RegisterPointLabel(projection,
                         parameter,
                         shieldStyle->GetShieldStyle(),
                         text,
                         shieldStyle->GetShieldStyle()->GetSize(),
                         fontHeight,
                         1.0,
                         wayScanlines[i].x+0.5,
                         wayScanlines[i].y+0.5);

      i+=stepSizeInPixel;
    }
  }

  bool MapPainter::RegisterPointLabel(const Projection& projection,
                                      const MapParameter& parameter,
                                      const LabelStyleRef& style,
                                      const std::string& text,
                                      double fontSize,
                                      double height,
                                      double alpha,
                                      double x,
                                      double y)
  {
    // Something is an overlay, if its alpha is <0.8
    bool overlay=alpha<0.8;

    // Reset all marks on labels, because we needs marks
    // for our internal collision handling
    if (overlay) {
      ClearLabelMarks(overlayLabels);
    }
    else {
      ClearLabelMarks(labels);
    }

    // First rough minimum bounding box, estimated without calculating text dimensions (since this is expensive).
    double bx1;
    double bx2;
    double by1;
    double by2;

    double frameHoriz;
    double frameVert;

    // Get the amount of delta to add to the frame, depending on the actual style
    GetLabelFrame(*style,
                  frameHoriz,
                  frameVert);

    // The bounding box of the label including some optional border
    bx1=x-frameHoriz;
    bx2=x+frameHoriz;
    by1=y-height/2-frameVert;
    by2=y+height/2+frameVert;

    // is box visible?
    // We do not check the horizontal extends, since the width is
    // not yet calculated
    if (parameter.GetDropNotVisiblePointLabels()) {
      if (by2<0 || by1>=projection.GetHeight()) {
        return false;
      }
    }

    if (overlay) {
      if (!MarkAllInBoundingBox(bx1,bx2,by1,by2,
                                *style,
                                overlayLabels)) {

        return false;
      }
    }
    else {
      if (!MarkAllInBoundingBox(bx1,bx2,by1,by2,
                                *style,
                                labels)) {
        return false;
      }
    }

    // We passed the first intersection test, we now calculate the real bounding box
    // by measuring text dimensions

    double xOff,yOff,width;

    GetTextDimension(projection,
                     parameter,
                     fontSize,
                     text,
                     xOff,yOff,width,height);

    bx1=x-width/2-frameHoriz;
    bx2=x+width/2+frameHoriz;
    by1=y-height/2-frameVert;
    by2=y+height/2+frameVert;

    // Again - is box visible? No also checking horizontal extends
    if (parameter.GetDropNotVisiblePointLabels()) {
      if (bx1>=projection.GetWidth() ||
          bx2<0 ||
          by1>=projection.GetHeight() ||
          by2<0) {
        return false;
      }
    }

    if (overlay) {
      if (!MarkAllInBoundingBox(bx1,bx2,by1,by2,
                                *style,
                                overlayLabels)) {
        return false;
      }

      // As an optional final processing step, we check if labels
      // with the same value (text) have at least sameLabelSpace distance.
      if (sameLabelSpace>shieldLabelSpace) {
        if (!MarkCloseLabelsWithSameText(bx1,
                                         bx2,
                                         by1,
                                         by2,
                                         *style,
                                         text,
                                         overlayLabels)) {
          return false;
        }
      }

      // Remove every marked (aka "in conflict" or "intersecting but of lower
      // priority") label.
      RemoveMarkedLabels(overlayLabels);
    }
    else {
      if (!MarkAllInBoundingBox(bx1,bx2,by1,by2,
                                *style,
                                labels)) {
        return false;
      }

      // As an optional final processing step, we check if labels
      // with the same value (text) have at least sameLabelSpace distance.
      if (sameLabelSpace>shieldLabelSpace) {
        if (!MarkCloseLabelsWithSameText(bx1,
                                         bx2,
                                         by1,
                                         by2,
                                         *style,
                                         text,
                                         labels)) {
          return false;
        }
      }

      // Remove every marked (aka "in conflict" or "intersecting but of lower
      // priority") label.
      RemoveMarkedLabels(labels);
    }


    // We passed the test, lets put ourself into the "draw label" job list

    LabelData label;

    label.x=x-width/2;
    label.y=y-height/2;
    label.bx1=bx1;
    label.by1=by1;
    label.bx2=bx2;
    label.by2=by2;
    label.alpha=alpha;
    label.fontSize=fontSize;
    label.style=style;
    label.text=text;

    if (overlay) {
      overlayLabels.push_back(label);
    }
    else {
      labels.push_back(label);
    }

    return true;
  }

  void MapPainter::LayoutPointLabels(const Projection& projection,
                                     const MapParameter& parameter,
                                     const FeatureValueBuffer& buffer,
                                     double minX,
                                     double minY,
                                     double maxX,
                                     double maxY,
                                     const IconStyleRef iconStyle,
                                     const std::vector<TextStyleRef>& textStyles)
  {
    labelLayoutData.clear();

    double centerX=(minX+maxX)/2;
    double centerY=(minY+maxY)/2;

    if (iconStyle) {
      if (!iconStyle->GetIconName().empty() &&
          HasIcon(*styleConfig,
                  parameter,
                  *iconStyle)) {
        LabelLayoutData data;

        data.position=iconStyle->GetPosition();
        data.height=14; // TODO
        data.icon=true;
        data.iconStyle=iconStyle;

        labelLayoutData.push_back(data);
      }
      else if (iconStyle->GetSymbol()) {
        LabelLayoutData data;

        data.position=iconStyle->GetPosition();
        data.height=projection.ConvertWidthToPixel(iconStyle->GetSymbol()->GetHeight());
        data.icon=false;
        data.iconStyle=iconStyle;

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

        GetFontHeight(projection,
                      parameter,
                      data.fontSize,
                      data.height);

        double alpha=textStyle->GetAlpha()/factor;

        if (alpha>1.0) {
          alpha=1.0;
        }

        data.alpha=alpha;
      }
      else if (textStyle->GetAutoSize()) {
        double height=std::abs((maxY-minY)*0.1);

        if (height==0) {
          continue;
        }

        if (height<standardFontSize) {
          continue;
        }

        data.fontSize=height/standardFontSize;
        data.height=height;
        data.alpha=textStyle->GetAlpha();
      }
      else {
        data.fontSize=textStyle->GetSize();

        GetFontHeight(projection,
                      parameter,
                      data.fontSize,
                      data.height);

        data.alpha=textStyle->GetAlpha();
      }

      data.position=textStyle->GetPosition();
      data.label=label;
      data.textStyle=textStyle;
      data.icon=false;

      labelLayoutData.push_back(data);
    }

    if (labelLayoutData.empty()) {
      return;
    }

    std::stable_sort(labelLayoutData.begin(),
                     labelLayoutData.end(),
                     LabelLayoutDataSorter);

    double offset=centerY;

    for (const auto& data : labelLayoutData) {
      if (data.textStyle) {

        RegisterPointLabel(projection,
                           parameter,
                           data.textStyle,
                           data.label,
                           data.fontSize,
                           data.height,
                           data.alpha,
                           centerX,offset);
      }
      else if (data.icon) {
        DrawIcon(data.iconStyle.get(),
                 centerX,offset);
      }
      else {
        DrawSymbol(projection,
                   parameter,
                   *data.iconStyle->GetSymbol(),
                   centerX,offset);
      }

      offset+=data.height;
    }
  }

  void MapPainter::DrawNodes(const StyleConfig& styleConfig,
                             const Projection& projection,
                             const MapParameter& parameter,
                             const MapData& data)
  {
    for (const auto& node : data.nodes) {
      DrawNode(styleConfig,
               projection,
               parameter,
               node);
    }
  }

  void MapPainter::DrawAreas(const StyleConfig& /*styleConfig*/,
                             const Projection& projection,
                             const MapParameter& parameter,
                             const MapData& /*data*/)
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
                                 const TypeInfoRef& type,
                                 const FeatureValueBuffer& buffer,
                                 const GeoBox& boundingBox)
  {
    IconStyleRef iconStyle;

    styleConfig.GetAreaTextStyles(type,
                                  buffer,
                                  projection,
                                  textStyles);
    styleConfig.GetAreaIconStyle(type,
                                 buffer,
                                 projection,
                                 iconStyle);

    double minX;
    double maxX;
    double minY;
    double maxY;

    projection.GeoToPixel(boundingBox.GetMinCoord(),
                          minX,minY);

    projection.GeoToPixel(boundingBox.GetMaxCoord(),
                          maxX,maxY);

    LayoutPointLabels(projection,
                      parameter,
                      buffer,
                      minX,minY,
                      maxX,maxY,
                      iconStyle,
                      textStyles);
  }

  void MapPainter::DrawAreaLabels(const StyleConfig& styleConfig,
                                  const Projection& projection,
                                  const MapParameter& parameter,
                                  const MapData& data)
  {
    for (const auto& area : data.areas) {
      for (const auto& ring :area->rings) {
        if (ring.ring==Area::masterRingId) {
          GeoBox   boundingBox;

          area->GetBoundingBox(boundingBox);

          DrawAreaLabel(styleConfig,
                        projection,
                        parameter,
                        area->GetType(),
                        ring.GetFeatureValueBuffer(),
                        boundingBox);
        }
        else {
          GeoBox boundingBox;

          ring.GetBoundingBox(boundingBox);

          DrawAreaLabel(styleConfig,
                        projection,
                        parameter,
                        ring.GetType(),
                        ring.GetFeatureValueBuffer(),
                        boundingBox);
        }
      }
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
              node->GetCoords().GetLon(),
              node->GetCoords().GetLat(),
              x,y);

    LayoutPointLabels(projection,
                      parameter,
                      node->GetFeatureValueBuffer(),
                      x,y,
                      x,y,
                      iconStyle,
                      textStyles);

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
                            const MapParameter& parameter,
                            const MapData& /*data*/)
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
    for (std::list<WayPathData>::const_iterator way=wayPathData.begin();
        way!=wayPathData.end();
        way++)
    {
      PathSymbolStyleRef pathSymbolStyle;

      styleConfig.GetWayPathSymbolStyle(*way->buffer,
                                        projection,
                                        pathSymbolStyle);

      if (pathSymbolStyle) {
        double symbolSpace=projection.ConvertWidthToPixel(pathSymbolStyle->GetSymbolSpace());

        DrawContourSymbol(projection,
                          parameter,
                          *pathSymbolStyle->GetSymbol(),
                          symbolSpace,
                          way->transStart, way->transEnd);
      }
    }
  }

  void MapPainter::DrawWayLabel(const StyleConfig& styleConfig,
                                const Projection& projection,
                                const MapParameter& parameter,
                                const WayPathData& data)
  {
    PathShieldStyleRef shieldStyle;
    PathTextStyleRef   pathTextStyle;

    styleConfig.GetWayPathShieldStyle(*data.buffer,
                                      projection,
                                      shieldStyle);
    styleConfig.GetWayPathTextStyle(*data.buffer,
                                    projection,
                                    pathTextStyle);

    if (pathTextStyle) {
      std::string textLabel=pathTextStyle->GetLabel()->GetLabel(parameter,
                                                                *data.buffer);

      if (!textLabel.empty()) {
        DrawContourLabel(projection,
                         parameter,
                         *pathTextStyle,
                         textLabel,
                         data.transStart,
                         data.transEnd);
        waysLabelDrawn++;
      }
    }

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

  void MapPainter::DrawWayLabels(const StyleConfig& styleConfig,
                                 const Projection& projection,
                                 const MapParameter& parameter,
                                 const MapData& /*data*/)
  {
    for (const auto& way : wayPathData) {
      DrawWayLabel(styleConfig,
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
      DrawLabel(projection,
                parameter,
                label);
      labelsDrawn++;
    }

    //
    // Draw overlays
    //

    for (const auto& label : overlayLabels) {
      DrawLabel(projection,
                parameter,
                label);
      labelsDrawn++;
    }
  }

  void MapPainter::PrepareAreas(const StyleConfig& styleConfig,
                                const Projection& projection,
                                const MapParameter& parameter,
                                const MapData& data)
  {
    double errorTolerancePixel=parameter.GetOptimizeErrorToleranceMm()*projection.GetDPI()/25.4;

    areaData.clear();

    //Areas
    for (const auto& area : data.areas) {
      std::vector<PolyData> data(area->rings.size());

      for (size_t i=0; i<area->rings.size(); i++) {
        if (area->rings[i].ring==Area::masterRingId) {
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

          if (ring.ring==ringId) {
            FillStyleRef fillStyle;

            if (ring.ring==Area::outerRingId) {
              styleConfig.GetAreaFillStyle(area->GetType(),
                                           ring.GetFeatureValueBuffer(),
                                           projection,
                                           fillStyle);
            }
            else if (!ring.GetType()->GetIgnore()) {
              styleConfig.GetAreaFillStyle(ring.GetType(),
                                           ring.GetFeatureValueBuffer(),
                                           projection,
                                           fillStyle);
            }

            if (!fillStyle) {
              continue;
            }

            foundRing=true;

            if (!IsVisible(projection,
                           ring.nodes,
                           fillStyle->GetBorderWidth()/2)) {
              continue;
            }

            AreaData a;

            // Collect possible clippings. We only take into account, inner rings of the next level
            // that do not have a type and thus act as a clipping region. If a inner ring has a type,
            // we currently assume that it does not have alpha and paints over its region and clipping is
            // not required.
            // Since we know that rings a created deep first, we only take into account direct followers
            // in the list with ring+1.
            size_t j=i+1;
            while (j<area->rings.size() &&
                   area->rings[j].ring==ringId+1 &&
                   area->rings[j].GetType()->GetIgnore()) {
              a.clippings.push_back(data[j]);

              j++;
            }

            a.ref=ObjectFileRef(area->GetFileOffset(),refArea);
            a.buffer=&ring.GetFeatureValueBuffer();
            a.fillStyle=fillStyle;
            a.transStart=data[i].transStart;
            a.transEnd=data[i].transEnd;

            a.minLat=ring.nodes[0].GetLat();
            a.maxLat=ring.nodes[0].GetLat();
            a.maxLon=ring.nodes[0].GetLon();
            a.minLon=ring.nodes[0].GetLon();

            for (size_t r=1; r<ring.nodes.size(); r++) {
              a.minLat=std::min(a.minLat,ring.nodes[r].GetLat());
              a.maxLat=std::min(a.maxLat,ring.nodes[r].GetLat());
              a.minLon=std::min(a.minLon,ring.nodes[r].GetLon());
              a.maxLon=std::min(a.maxLon,ring.nodes[r].GetLon());
            }

            areaData.push_back(a);

            areasSegments++;
          }
        }

        ringId++;
      }
    }

    areaData.sort(AreaSorter);
  }

  void MapPainter::PrepareWaySegment(const StyleConfig& styleConfig,
                                     const Projection& projection,
                                     const MapParameter& parameter,
                                     const ObjectFileRef& ref,
                                     const FeatureValueBuffer& buffer,
                                     const std::vector<GeoCoord>& nodes,
                                     const std::vector<Id>& ids)
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
    double errorTolerancePixel=parameter.GetOptimizeErrorToleranceMm()*projection.GetDPI()/25.4;

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

      if (!IsVisible(projection,
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
      data.startIsClosed=ids.empty() || ids[0]==0;
      data.endIsClosed=ids.empty() || ids[ids.size()-1]==0;

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
      PrepareWaySegment(styleConfig,
                        projection,
                        parameter,
                        ObjectFileRef(way->GetFileOffset(),refWay),
                        way->GetFeatureValueBuffer(),
                        way->nodes,
                        way->ids);
    }

    for (const auto& way : data.poiWays) {
      PrepareWaySegment(styleConfig,
                        projection,
                        parameter,
                        ObjectFileRef(way->GetFileOffset(),refWay),
                        way->GetFeatureValueBuffer(),
                        way->nodes,
                        way->ids);
    }

    wayData.sort();
  }

  void MapPainter::GetLabelFrame(const LabelStyle& style,
                                 double& horizontal,
                                 double& vertical)
  {
    horizontal=0;
    vertical=0;

    if (dynamic_cast<const ShieldStyle*>(&style)!=NULL) {
      horizontal=5;
      vertical=5;
    }
  }

  void MapPainter::GetLabelSpace(const LabelStyle& styleA,
                                 const LabelStyle& styleB,
                                 double& horizontal,
                                 double& vertical)
  {
    if (dynamic_cast<const ShieldStyle*>(&styleA)!=NULL &&
        dynamic_cast<const ShieldStyle*>(&styleB)!=NULL) {
      horizontal=shieldLabelSpace;
      vertical=shieldLabelSpace;
    }
    else {
      horizontal=labelSpace;
      vertical=0;
    }
  }

  bool MapPainter::Draw(const Projection& projection,
                        const MapParameter& parameter,
                        const MapData& data)
  {
    waysSegments=0;
    waysDrawn=0;
    waysLabelDrawn=0;

    areasSegments=0;
    areasDrawn=0;

    nodesDrawn=0;

    labelsDrawn=0;

    labels.clear();
    overlayLabels.clear();

    transBuffer.Reset();

    labelSpace=projection.ConvertWidthToPixel(parameter.GetLabelSpace());
    shieldLabelSpace=projection.ConvertWidthToPixel(parameter.GetPlateLabelSpace());
    sameLabelSpace=projection.ConvertWidthToPixel(parameter.GetSameLabelSpace());

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

    //
    // Draw areas
    //

    StopClock areasTimer;

    DrawAreas(*styleConfig,
              projection,
              parameter,
              data);

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
             parameter,
             data);

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

    StopClock pathLabelsTimer;

    DrawWayLabels(*styleConfig,
                  projection,
                  parameter,
                  data);

    pathLabelsTimer.Stop();

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
                   parameter,
                   data);

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
          << prepareWaysTimer << "/" << pathsTimer << "/" << pathLabelsTimer << " (sec)";

      log.Info()
          << "Areas: "
          << data.areas.size() << "/" << areasSegments << "/" << areasDrawn << " (pcs) "
          << prepareAreasTimer << "/" << areasTimer << "/" << areaLabelsTimer << " (sec)";

      log.Info()
          << "Nodes: "
          << data.nodes.size() <<"+" << data.poiNodes.size() << "/" << nodesDrawn << " (pcs) "
          << nodesTimer << "/" << poisTimer << " (sec)";

      log.Info()
          << "Labels: " << labels.size() << "/" << overlayLabels.size() << "/" << labelsDrawn << " (pcs) "
          << labelsTimer << " (sec)";
    }

    return true;
  }
}
