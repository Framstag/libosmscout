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

#include <iostream>
#include <limits>

#include <osmscout/util/HashSet.h>
#include <osmscout/util/StopClock.h>
#include <osmscout/util/String.h>

#include <osmscout/private/Math.h>

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

  MapParameter::MapParameter()
  : dpi(96.0),
    fontName("sans-serif"),
    fontSize(2.0),
    lineMinWidthPixel(0.2),
    drawBridgeMagnification(magVeryClose),
    drawTunnelMagnification(magVeryClose),
    optimizeWayNodes(TransPolygon::none),
    optimizeAreaNodes(TransPolygon::none),
    optimizeErrorToleranceMm(25.4/dpi), //1 pixel
    drawFadings(true),
    drawWaysWithFixedWidth(false),
    labelSpace(3.0),
    plateLabelSpace(5.0),
    sameLabelSpace(40.0),
    renderSeaLand(false),
    debugPerformance(false)
  {
    // no code
  }

  MapParameter::~MapParameter()
  {
    // no code
  }

  void MapParameter::SetDPI(double dpi)
  {
    this->dpi=dpi;
  }

  void MapParameter::SetFontName(const std::string& fontName)
  {
    this->fontName=fontName;
  }

  void MapParameter::SetFontSize(double fontSize)
  {
    this->fontSize=fontSize;
  }

  void MapParameter::SetIconPaths(const std::list<std::string>& paths)
  {
    this->iconPaths=paths;
  }

  void MapParameter::SetPatternPaths(const std::list<std::string>& paths)
  {
    this->patternPaths=paths;
  }

  void MapParameter::SetLineMinWidthPixel(double lineMinWidthPixel)
  {
    this->lineMinWidthPixel=lineMinWidthPixel;
  }

  void MapParameter::SetDrawBridgeMagnification(double magnification)
  {
    this->drawBridgeMagnification=magnification;
  }

  void MapParameter::SetDrawTunnelMagnification(double magnification)
  {
    this->drawTunnelMagnification=magnification;
  }

  void MapParameter::SetOptimizeWayNodes(TransPolygon::OptimizeMethod optimize)
  {
    optimizeWayNodes=optimize;
  }

  void MapParameter::SetOptimizeAreaNodes(TransPolygon::OptimizeMethod optimize)
  {
    optimizeAreaNodes=optimize;
  }

  void MapParameter::SetOptimizeErrorToleranceMm(double errorToleranceMm)
  {
    optimizeErrorToleranceMm=errorToleranceMm;
  }

  void MapParameter::SetDrawFadings(bool drawFadings)
  {
    this->drawFadings=drawFadings;
  }

  void MapParameter::SetDrawWaysWithFixedWidth(bool drawWaysWithFixedWidth)
  {
    this->drawWaysWithFixedWidth=drawWaysWithFixedWidth;
  }

  void MapParameter::SetLabelSpace(double labelSpace)
  {
    this->labelSpace=labelSpace;
  }

  void MapParameter::SetPlateLabelSpace(double plateLabelSpace)
  {
    this->plateLabelSpace=plateLabelSpace;
  }

  void MapParameter::SetSameLabelSpace(double sameLabelSpace)
  {
    this->sameLabelSpace=sameLabelSpace;
  }

  void MapParameter::SetRenderSeaLand(bool render)
  {
    this->renderSeaLand=render;
  }

  void MapParameter::SetDebugPerformance(bool debug)
  {
    debugPerformance=debug;
  }

  void MapParameter::SetBreaker(const BreakerRef& breaker)
  {
    this->breaker=breaker;
  }

  MapPainter::MapPainter()
  {
    tunnelDash.push_back(0.4);
    tunnelDash.push_back(0.4);

    areaMarkStyle.SetFillColor(Color(1.0,0,0.0,0.5));

    landFill=new FillStyle();
    landFill->SetFillColor(Color(241.0/255,238.0/255,233.0/255));

    seaFill=new FillStyle();
    seaFill->SetFillColor(Color(181.0/255,208.0/255,208.0/255));

    debugLabel=new TextStyle();

    debugLabel->SetStyle(TextStyle::normal);
    debugLabel->SetPriority(0);
    debugLabel->SetTextColor(Color(0,0,0,0.5));
    debugLabel->SetSize(1.2);

    coastlineSegmentAttributes.layer=0;
  }

  MapPainter::~MapPainter()
  {
    // no code
  }

  void MapPainter::ScanConvertLine(size_t transStart, size_t transEnd,
                                   double cellWidth,
                                   double cellHeight,
                                   std::vector<ScanCell>& cells)
  {
    if (transStart==transEnd) {
      return;
    }

    for (size_t i=transStart; i<transEnd; i++) {
      size_t j=i+1;

      int x1=int(transBuffer.buffer[i].x/cellWidth);
      int x2=int(transBuffer.buffer[j].x/cellWidth);
      int y1=int(transBuffer.buffer[i].y/cellHeight);
      int y2=int(transBuffer.buffer[j].y/cellHeight);

      osmscout::ScanConvertLine(x1,y1,x2,y2,cells);
    }
  }

  bool MapPainter::IsVisible(const Projection& projection,
                             const std::vector<Point>& nodes,
                             double pixelOffset) const
  {
    if (nodes.empty()) {
      return false;
    }

    // Bounding box
    double lonMin=nodes[0].GetLon();
    double lonMax=nodes[0].GetLon();
    double latMin=nodes[0].GetLat();
    double latMax=nodes[0].GetLat();

    for (size_t i=1; i<nodes.size(); i++) {
      lonMin=std::min(lonMin,nodes[i].GetLon());
      lonMax=std::max(lonMax,nodes[i].GetLon());
      latMin=std::min(latMin,nodes[i].GetLat());
      latMax=std::max(latMax,nodes[i].GetLat());
    }

    double xMin;
    double xMax;
    double yMin;
    double yMax;
    double y1;
    double y2;

    if (!projection.GeoToPixel(lonMin,
                               latMax,
                               xMin,
                               y1)) {
      return false;
    }

    if (!projection.GeoToPixel(lonMax,
                               latMin,
                               xMax,
                               y2)) {
      return false;
    }

    yMax = fmax(y1,y2);
    yMin = fmin(y1,y2);

    xMin-=pixelOffset;
    yMin-=pixelOffset;

    xMax+=pixelOffset;
    yMax+=pixelOffset;

    return !(xMin>=projection.GetWidth() ||
             yMin>=projection.GetHeight() ||
             xMax<0 ||
             yMax<0);
  }

  void MapPainter::CalculateEffectiveLabelStyle(const Projection& projection,
                                                const MapParameter& parameter,
                                                const LabelStyle& style,
                                                double& fontSize,
                                                double& alpha)
  {
    if (dynamic_cast<const TextStyle*>(&style)!=NULL) {
      const TextStyle* textStyle=dynamic_cast<const TextStyle*>(&style);

      // Calculate effective font size and alpha value
      if (projection.GetMagnification()>textStyle->GetScaleAndFadeMag()) {
        if (parameter.GetDrawFadings()) {
          double factor=log2(projection.GetMagnification())-log2(textStyle->GetScaleAndFadeMag());
          fontSize=fontSize*pow(2,factor);
          alpha=alpha/factor;

          if (alpha>1.0) {
            alpha=1.0;
          }
        }
      }
    }
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
    for (std::list<LabelData>::iterator l=labels.begin();
        l!=labels.end();
        ++l) {
      LabelData& label=*l;

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
      if (!(hx1>label.bx2 ||
            hx2<label.bx1 ||
            hy1>label.by2 ||
            hy2<label.by1)) {
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
    for (std::list<LabelData>::iterator l=labels.begin();
        l!=labels.end();
        ++l) {
      LabelData& label=*l;

      if (label.mark) {
        continue;
      }

      if (dynamic_cast<const ShieldStyle*>(&style)!=NULL &&
          dynamic_cast<const ShieldStyle*>(label.style)!=NULL) {
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
                             const MapParameter& parameter,
                             double lon,
                             double lat,
                             double& x,
                             double& y)
  {
    projection.GeoToPixel(lon,lat,
                          x,y);
  }

  bool MapPainter::GetBoundingBox(const std::vector<Point>& nodes,
                                  double& xmin, double& ymin,
                                  double& xmax, double& ymax) const
  {
    if (nodes.empty()) {
      return false;
    }

    xmin=nodes[0].GetLon();
    xmax=nodes[0].GetLon();
    ymin=nodes[0].GetLat();
    ymax=nodes[0].GetLat();

    for (size_t j=1; j<nodes.size(); j++) {
      xmin=std::min(xmin,nodes[j].GetLon());
      xmax=std::max(xmax,nodes[j].GetLon());
      ymin=std::min(ymin,nodes[j].GetLat());
      ymax=std::max(ymax,nodes[j].GetLat());
    }

    return true;
  }

  bool MapPainter::GetCenterPixel(const Projection& projection,
                                  const std::vector<Point>& nodes,
                                  double& cx,
                                  double& cy) const
  {
    double xmin;
    double xmax;
    double ymin;
    double ymax;

    if (!GetBoundingBox(nodes,xmin,ymin,xmax,ymax)) {
      return false;
    }

    projection.GeoToPixel(xmin+(xmax-xmin)/2,
                          ymin+(ymax-ymin)/2,
                          cx,cy);

    return true;
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

  void MapPainter::DrawGroundTiles(const StyleConfig& styleConfig,
                                   const Projection& projection,
                                   const MapParameter& parameter,
                                   const MapData& data)
  {
    size_t            level=MagToLevel(projection.GetMagnification());
    FillStyleRef      landFill=styleConfig.GetAreaFillStyle(styleConfig.GetTypeConfig()->GetAreaTypeId("_tile_land"),level);

#if defined(DEBUG_GROUNDTILES)
      std::set<Coord> drawnLabels;
#endif

    if (landFill.Invalid()) {
      landFill=this->landFill;
    }

    DrawArea(*landFill,
             parameter,
             0,0,projection.GetWidth(),projection.GetHeight());

    if (!parameter.GetRenderSeaLand()) {
      return;
    }

    FillStyleRef       seaFill=styleConfig.GetAreaFillStyle(styleConfig.GetTypeConfig()->GetAreaTypeId("_tile_sea"),level);
    FillStyleRef       coastFill=styleConfig.GetAreaFillStyle(styleConfig.GetTypeConfig()->GetAreaTypeId("_tile_coast"),level);
    FillStyleRef       unknownFill=styleConfig.GetAreaFillStyle(styleConfig.GetTypeConfig()->GetAreaTypeId("_tile_unknown"),level);
    LineStyleRef       coastlineLine=styleConfig.GetWayLineStyle(styleConfig.GetTypeConfig()->GetWayTypeId("_tile_coastline"),level);
    std::vector<Point> points;
    size_t             start,end;

    if (seaFill.Invalid()) {
      seaFill=this->seaFill;
    }

    if (coastFill.Invalid()) {
      coastFill=this->seaFill;
    }

    if (unknownFill.Invalid()) {
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

        transBuffer.TransformArea(projection,
                                  TransPolygon::none,
                                  points,
                                  start,end,
                                  parameter.GetOptimizeErrorToleranceDots());

        transBuffer.buffer[start+0].x=floor(transBuffer.buffer[start+0].x);
        transBuffer.buffer[start+0].y=ceil(transBuffer.buffer[start+0].y);

        transBuffer.buffer[start+1].x=ceil(transBuffer.buffer[start+1].x);
        transBuffer.buffer[start+1].y=ceil(transBuffer.buffer[start+1].y);

        transBuffer.buffer[start+2].x=ceil(transBuffer.buffer[start+2].x);
        transBuffer.buffer[start+2].y=floor(transBuffer.buffer[start+2].y);

        transBuffer.buffer[start+3].x=floor(transBuffer.buffer[start+3].x);
        transBuffer.buffer[start+3].y=floor(transBuffer.buffer[start+3].y);

        transBuffer.buffer[start+4]=transBuffer.buffer[start];
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

        transBuffer.TransformArea(projection,
                                  TransPolygon::none,
                                  points,
                                  start,end,
                                  parameter.GetOptimizeErrorToleranceDots());

        for (size_t i=0; i<points.size(); i++) {
          if (tile->coords[i].x==0) {
            transBuffer.buffer[start+i].x=floor(transBuffer.buffer[start+i].x);
          }
          if (tile->coords[i].x==GroundTile::Coord::CELL_MAX) {
            transBuffer.buffer[start+i].x=ceil(transBuffer.buffer[start+i].x);
          }

          if (tile->coords[i].y==0) {
            transBuffer.buffer[start+i].y=ceil(transBuffer.buffer[start+i].y);
          }
          if (tile->coords[i].y==GroundTile::Coord::CELL_MAX) {
            transBuffer.buffer[start+i].y=floor(transBuffer.buffer[start+i].y);
          }
        }

        if (coastlineLine.Valid()) {
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

              data.attributes=&coastlineSegmentAttributes;
              data.lineStyle=coastlineLine.Get();
              data.pathTextStyle=NULL;
              data.shieldStyle=NULL;
              data.prio=std::numeric_limits<size_t>::max();
              data.transStart=start+lineStart;
              data.transEnd=start+lineEnd;
              data.lineWidth=GetProjectedWidth(projection,
                                               ConvertWidthToPixel(parameter,coastlineLine->GetDisplayWidth()),
                                               coastlineLine->GetWidth());
              data.drawBridge=false;
              data.drawTunnel=false;
              data.outline=false;

              wayData.push_back(data);
            }

            lineStart=lineEnd+1;
          }
        }
      }

      areaData.ref=ObjectRef();
      areaData.attributes=NULL;
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

  void MapPainter::DrawSymbol(const Projection& projection,
                              const MapParameter& parameter,
                              const Symbol& symbol,
                              double x, double y)
  {
    // no code - must be implemented by derived classes!
  }

  void MapPainter::DrawContourSymbol(const Projection& projection,
                                     const MapParameter& parameter,
                                     const Symbol& symbol,
                                     double space,
                                     size_t transStart, size_t transEnd)
  {
    // no code - must be implemented by derived classes!
  }

  void MapPainter::RegisterPointWayLabel(const Projection& projection,
                                         const MapParameter& parameter,
                                         const ShieldStyle& shieldStyle,
                                         const std::string& text,
                                         size_t transStart, size_t transEnd)
  {
    double stepSizeInPixel=ConvertWidthToPixel(parameter,parameter.GetLabelSpace());

    wayScanlines.clear();
    ScanConvertLine(transStart,transEnd,1,1,wayScanlines);

    size_t i=0;
    while (i<wayScanlines.size()) {
      RegisterPointLabel(projection,
                         parameter,
                         shieldStyle,
                         text,
                         wayScanlines[i].x+0.5,
                         wayScanlines[i].y+0.5);

      i+=stepSizeInPixel;
    }
  }

  bool MapPainter::RegisterPointLabel(const Projection& projection,
                                      const MapParameter& parameter,
                                      const LabelStyle& style,
                                      const std::string& text,
                                      double x,
                                      double y)
  {
    double fontSize=style.GetSize();
    double a=style.GetAlpha();

    CalculateEffectiveLabelStyle(projection,
                                 parameter,
                                 *dynamic_cast<const TextStyle*>(&style),
                                 fontSize,
                                 a);

    // Something is an overlay, if its alpha is <0.8
    bool overlay=a<0.8;

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

    GetLabelFrame(style,
                  frameHoriz,
                  frameVert);

    bx1=x-frameHoriz;
    bx2=x+frameHoriz;
    by1=y-fontSize/2-frameVert;
    by2=y+fontSize/2+frameVert;

    if (overlay) {
      if (!MarkAllInBoundingBox(bx1,bx2,by1,by2,
                                style,
                                overlayLabels)) {
        return false;
      }
    }
    else {
      if (!MarkAllInBoundingBox(bx1,bx2,by1,by2,
                                style,
                                labels)) {
        return false;
      }
    }

    // We passed the first intersection test, we now calculate the real bounding box
    // by measuring text dimensions

    double xOff,yOff,width,height;

    GetTextDimension(parameter,
                     fontSize,
                     text,
                     xOff,yOff,width,height);

    bx1=x-width/2-frameHoriz;
    bx2=x+width/2+frameHoriz;
    by1=y-height/2-frameVert;
    by2=y+height/2+frameVert;

    // is box visible?
    if (bx1>=projection.GetWidth() ||
        bx2<0 ||
        by1>=projection.GetHeight() ||
        by2<0) {
      return false;
    }

    if (overlay) {
      if (!MarkAllInBoundingBox(bx1,bx2,by1,by2,
                                style,
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
                                         style,
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
                                style,
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
                                         style,
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
    label.alpha=a;
    label.fontSize=fontSize;
    label.style=&style;
    label.text=text;

    if (overlay) {
      overlayLabels.push_back(label);
    }
    else {
      labels.push_back(label);
    }

    return true;
  }

  void MapPainter::DrawNodes(const StyleConfig& styleConfig,
                             const Projection& projection,
                             const MapParameter& parameter,
                             const MapData& data)
  {
    for (std::vector<NodeRef>::const_iterator n=data.nodes.begin();
         n!=data.nodes.end();
         ++n) {
      const NodeRef& node=*n;

      DrawNode(styleConfig,
               projection,
               parameter,
               node);
    }
  }

  void MapPainter::DrawAreas(const StyleConfig& styleConfig,
                             const Projection& projection,
                             const MapParameter& parameter,
                             const MapData& data)
  {
    for (std::list<AreaData>::const_iterator area=areaData.begin();
        area!=areaData.end();
        area++)
    {
      DrawArea(projection,
               parameter,
               *area);

      areasDrawn++;
    }
  }

  void MapPainter::DrawAreaLabels(const StyleConfig& styleConfig,
                                  const Projection& projection,
                                  const MapParameter& parameter,
                                  const MapData& data)
  {
    size_t level=MagToLevel(projection.GetMagnification());

    for (std::vector<WayRef>::const_iterator a=data.areas.begin();
         a!=data.areas.end();
         ++a) {
      const WayRef& area=*a;

      const TextStyle  *labelStyle=styleConfig.GetAreaTextStyle(area->GetType(),level);
      IconStyle         *iconStyle=styleConfig.GetAreaIconStyle(area->GetType(),level);
      bool              hasLabel=labelStyle!=NULL &&
                        labelStyle->IsVisible();
      bool              hasSymbol=iconStyle!=NULL && iconStyle->GetSymbol().Valid();
      bool              hasIcon=iconStyle!=NULL && !iconStyle->GetIconName().empty();
      std::string       label;

      if (hasIcon) {
        hasIcon=HasIcon(styleConfig,
                        parameter,
                        *iconStyle);
      }

      if (!hasSymbol && !hasLabel && !hasIcon) {
        continue;
      }

      if (hasLabel) {
        if (!area->GetRefName().empty()) {
          label=area->GetRefName();
        }
        else if (!area->GetName().empty()) {
          label=area->GetName();
        }
        else if (!area->GetHouseNr().empty()) {
          label=area->GetHouseNr();
        }

        hasLabel=!label.empty();
      }

      if (!hasSymbol && !hasLabel && !hasIcon) {
        continue;
      }

      double x,y;

      if (!GetCenterPixel(projection,
                          area->nodes,
                          x,y)) {
        continue;
      }

      if (hasLabel) {
        if (hasSymbol) {
          RegisterPointLabel(projection,
                             parameter,
                             *labelStyle,
                             label,
                             x,y+ConvertWidthToPixel(parameter,iconStyle->GetSymbol()->GetHeight())/2+
                                 ConvertWidthToPixel(parameter,1.0)+
                                 ConvertWidthToPixel(parameter,labelStyle->GetSize())/2);
        }
        else if (hasIcon) {
          RegisterPointLabel(projection,
                             parameter,
                             *labelStyle,
                             label,
                             x,y+14+5); // TODO: Better layout to real size of icon
        }
        else {
          RegisterPointLabel(projection,
                             parameter,
                             *labelStyle,
                             label,
                             x,y);
        }

        areasLabelDrawn++;
      }

      if (hasIcon) {
        DrawIcon(iconStyle,x,y);
      }
      else if (hasSymbol) {
        DrawSymbol(projection,
                   parameter,
                   *iconStyle->GetSymbol(),
                   x,y);
      }
    }

    for (std::vector<RelationRef>::const_iterator r=data.relationAreas.begin();
         r!=data.relationAreas.end();
         ++r) {
      const RelationRef& relation=*r;

      for (size_t m=0; m<relation->roles.size(); m++) {
        TypeId            type=relation->roles[m].ring==0 ? relation->GetType() : relation->roles[m].GetType();
        const TextStyle  *labelStyle=styleConfig.GetAreaTextStyle(type,level);
        IconStyle         *iconStyle=styleConfig.GetAreaIconStyle(type,level);
        bool              hasLabel=labelStyle!=NULL &&
                                   labelStyle->IsVisible();
        bool              hasSymbol=iconStyle!=NULL && iconStyle->GetSymbol().Valid();
        bool              hasIcon=iconStyle!=NULL && !iconStyle->GetIconName().empty();
        std::string       label;

        if (hasIcon) {
          hasIcon=HasIcon(styleConfig,
                          parameter,
                          *iconStyle);
        }

        if (!hasSymbol && !hasLabel && !hasIcon) {
          continue;
        }

        if (hasLabel) {
          if (!relation->roles[m].GetRefName().empty()) {
            label=relation->roles[m].GetRefName();
          }
          else if (!relation->roles[m].GetName().empty()) {
            label=relation->roles[m].GetName();
          }
          else if (!relation->roles[m].GetAttributes().GetHouseNr().empty()) {
            label=relation->roles[m].GetAttributes().GetHouseNr();
          }

          hasLabel=!label.empty();
        }

        if (!hasSymbol && !hasLabel && !hasIcon) {
          continue;
        }

        double x,y;

        if (!GetCenterPixel(projection,
                            relation->roles[m].nodes,
                            x,y)) {
          continue;
        }

        if (hasLabel) {
          if (hasSymbol) {
            RegisterPointLabel(projection,
                               parameter,
                               *labelStyle,
                               label,
                                x,y+ConvertWidthToPixel(parameter,iconStyle->GetSymbol()->GetHeight())/2+
                                    ConvertWidthToPixel(parameter,1.0)+
                                    ConvertWidthToPixel(parameter,labelStyle->GetSize())/2);
          }
          else if (hasIcon) {
            RegisterPointLabel(projection,
                               parameter,
                               *labelStyle,
                               label,
                               x,y+14+5); // TODO: Better layout to real size of icon
          }
          else {
            RegisterPointLabel(projection,
                               parameter,
                               *labelStyle,
                               label,
                               x,y);
          }

          areasLabelDrawn++;
        }

        if (hasIcon) {
          DrawIcon(iconStyle,x,y);
        }
        else if (hasSymbol) {
          DrawSymbol(projection,
                     parameter,
                     *iconStyle->GetSymbol(),
                     x,y);
        }
      }
    }
  }

  void MapPainter::DrawNode(const StyleConfig& styleConfig,
                            const Projection& projection,
                            const MapParameter& parameter,
                            const NodeRef& node)
  {
    size_t           level=MagToLevel(projection.GetMagnification());
    const TextStyle  *labelStyle=styleConfig.GetNodeTextStyle(node->GetType(),level);
    IconStyle        *iconStyle=styleConfig.GetNodeIconStyle(node->GetType(),level);
    bool             hasLabel=labelStyle!=NULL;
    bool             hasSymbol=iconStyle!=NULL && iconStyle->GetSymbol().Valid();
    bool             hasIcon=iconStyle!=NULL && !iconStyle->GetIconName().empty();

    std::string label;

    //nodesDrawnCount++;

    if (hasLabel) {
      for (size_t i=0; i<node->GetTagCount(); i++) {
        // TODO: We should make sure we prefer one over the other
        if (node->GetTagKey(i)==styleConfig.GetTypeConfig()->tagName) {
          label=node->GetTagValue(i);
          break;
        }
        else if (node->GetTagKey(i)==styleConfig.GetTypeConfig()->tagRef)  {
          label=node->GetTagValue(i);
        }
        else if (node->GetTagKey(i)==styleConfig.GetTypeConfig()->tagHouseNr)  {
          label=node->GetTagValue(i);
        }
      }

      hasLabel=!label.empty();
    }

    if (hasIcon) {
      hasIcon=HasIcon(styleConfig,
                      parameter,
                      *iconStyle);
    }

    if (!hasSymbol && !hasLabel && !hasIcon) {
      return;
    }

    double x,y;

    Transform(projection,
              parameter,
              node->GetLon(),
              node->GetLat(),
              x,y);

    if (hasLabel) {
      if (hasSymbol) {
        RegisterPointLabel(projection,
                           parameter,
                           *labelStyle,
                           label,
                           x,y+ConvertWidthToPixel(parameter,iconStyle->GetSymbol()->GetHeight())/2+
                               ConvertWidthToPixel(parameter,1.0)+
                               ConvertWidthToPixel(parameter,labelStyle->GetSize())/2);
      }
      else if (hasIcon) {
        RegisterPointLabel(projection,
                           parameter,
                           *labelStyle,
                           label,
                           x,y+14+5); // TODO: Better layout to real size of icon
      }
      else {
        RegisterPointLabel(projection,
                           parameter,
                           *labelStyle,
                           label,
                           x,y);
      }
    }

    if (hasIcon) {
      DrawIcon(iconStyle,x,y);
    }
    else if (hasSymbol) {
      DrawSymbol(projection,
                 parameter,
                 *iconStyle->GetSymbol(),
                 x,y);
    }

    nodesDrawn++;
  }

  void MapPainter::DrawWayOutline(const StyleConfig& styleConfig,
                                  const Projection& projection,
                                  const MapParameter& parameter,
                                  const WayData& data)
  {
    if (data.drawTunnel) {
      tunnelDash[0]=4.0/data.lineWidth;
      tunnelDash[1]=2.0/data.lineWidth;

      if (data.outline) {
        DrawPath(projection,
                 parameter,
                 data.lineStyle->GetOutlineColor(),
                 data.outlineWidth,
                 tunnelDash,
                 data.attributes->StartIsJoint() ? LineStyle::capButt : LineStyle::capRound,
                 data.attributes->EndIsJoint() ? LineStyle::capButt : LineStyle::capRound,
                 data.transStart,data.transEnd);
      }
      else if (projection.GetMagnification()>=10000) {
        // light grey dashes

        DrawPath(projection,
                 parameter,
                 Color(0.5,0.5,0.5),
                 data.outlineWidth,
                 tunnelDash,
                 data.attributes->StartIsJoint() ? LineStyle::capButt : LineStyle::capRound,
                 data.attributes->EndIsJoint() ? LineStyle::capButt : LineStyle::capRound,
                 data.transStart,data.transEnd);
      }
      else {
        // dark grey dashes

        DrawPath(projection,
                 parameter,
                 Color(0.5,0.5,0.5),
                 data.outlineWidth,
                 tunnelDash,
                 data.attributes->StartIsJoint() ? LineStyle::capButt : LineStyle::capRound,
                 data.attributes->EndIsJoint() ? LineStyle::capButt : LineStyle::capRound,
                     data.transStart,data.transEnd);
      }
    }
    else {
      // normal path, normal outline color

      DrawPath(projection,
               parameter,
               data.lineStyle->GetOutlineColor(),
               data.outlineWidth,
               emptyDash,
               data.attributes->StartIsJoint() ? LineStyle::capButt : LineStyle::capRound,
               data.attributes->EndIsJoint() ? LineStyle::capButt : LineStyle::capRound,
               data.transStart,data.transEnd);
    }

    waysOutlineDrawn++;
  }

  void MapPainter::DrawWay(const StyleConfig& styleConfig,
                           const Projection& projection,
                           const MapParameter& parameter,
                           const WayData& data)
  {
    Color color;

    if (data.outline) {
      // Draw line with normal color
      color=data.lineStyle->GetLineColor();
    }
    else {
      // Should draw outline, but resolution is too low
      // Draw line with alternate color
      color=data.lineStyle->GetAlternateColor();
    }

    if (data.drawTunnel) {
      color=color.Lighten(0.5);
    }

    if (data.lineStyle->HasDashes() &&
        data.lineStyle->GetGapColor().GetA()>0.0) {
      DrawPath(projection,
               parameter,
               data.lineStyle->GetGapColor(),
               data.lineWidth,
               emptyDash,
               LineStyle::capRound,
               LineStyle::capRound,
               data.transStart,data.transEnd);
    }

    DrawPath(projection,
             parameter,
             color,
             data.lineWidth,
             data.lineStyle->GetDash(),
             data.lineStyle->GetCapStyle(),
             data.lineStyle->GetCapStyle(),
             data.transStart,data.transEnd);

    if (data.drawBridge) {
      DrawPath(projection,
               parameter,
               Color::BLACK,
               1,
               emptyDash,
               LineStyle::capButt,
               LineStyle::capButt,
               data.par1Start,data.par1End);

      DrawPath(projection,
               parameter,
               Color::BLACK,
               1,
               emptyDash,
               LineStyle::capButt,
               LineStyle::capButt,
               data.par2Start,data.par2End);
    }

    waysDrawn++;
  }

  void MapPainter::DrawWays(const StyleConfig& styleConfig,
                            const Projection& projection,
                            const MapParameter& parameter,
                            const MapData& data)
  {
    std::list<WayData>::const_iterator start;

    start=wayData.begin();
    while (start!=wayData.end()) {

      std::list<WayData>::const_iterator way;

      way=start;
      while (way!=wayData.end() && way->attributes->GetLayer()==start->attributes->GetLayer()) {
        if (way->drawBridge ||
            way->drawTunnel ||
            way->outline) {
          DrawWayOutline(styleConfig,
                         projection,
                         parameter,
                         *way);
        }

        way++;
      }

      way=start;
      while (way!=wayData.end() && way->attributes->GetLayer()==start->attributes->GetLayer()) {
        DrawWay(styleConfig,
                projection,
                parameter,
                *way);

        way++;
      }

      start=way;
    }
  }

  void MapPainter::DrawWayDecorations(const StyleConfig& styleConfig,
                                      const Projection& projection,
                                      const MapParameter& parameter,
                                      const MapData& data)
  {
    SymbolRef onewayArrow=styleConfig.GetSymbol("oneway_arrow");
    double onewaySpace=ConvertWidthToPixel(parameter,15);

    for (std::list<WayData>::const_iterator way=wayData.begin();
        way!=wayData.end();
        way++)
    {
      if (projection.GetMagnification()>=magVeryClose &&
          onewayArrow.Valid() &&
          way->attributes->IsOneway()) {
        DrawContourSymbol(projection,
                          parameter,
                          *onewayArrow,
                          onewaySpace,
                          way->transStart, way->transEnd);
      }
    }
  }

  void MapPainter::DrawWayLabels(const StyleConfig& styleConfig,
                                 const Projection& projection,
                                 const MapParameter& parameter,
                                 const MapData& data)
  {
    for (std::list<WayData>::const_iterator way=wayData.begin();
        way!=wayData.end();
        way++)
    {
      if (way->pathTextStyle!=NULL) {
        switch (way->pathTextStyle->GetLabel()) {
        case PathTextStyle::none:
          break;
        case PathTextStyle::name:
          DrawContourLabel(projection,
                           parameter,
                           *way->pathTextStyle,
                           way->attributes->GetName(),
                           way->transStart,way->transEnd);
        waysLabelDrawn++;
        break;
        case PathTextStyle::ref:
          DrawContourLabel(projection,
                           parameter,
                           *way->pathTextStyle,
                           way->attributes->GetRefName(),
                           way->transStart,way->transEnd);
        waysLabelDrawn++;
        break;
        }
      }

      if (way->shieldStyle!=NULL) {
        switch(way->shieldStyle->GetLabel()) {
        case ShieldStyle::none:
          break;
        case ShieldStyle::name:
          RegisterPointWayLabel(projection,
                                parameter,
                                *way->shieldStyle,
                                way->attributes->GetName(),
                                way->transStart,way->transEnd);
        waysLabelDrawn++;
        break;
        case ShieldStyle::ref:
          RegisterPointWayLabel(projection,
                                parameter,
                                *way->shieldStyle,
                                way->attributes->GetRefName(),
                                way->transStart,way->transEnd);
          waysLabelDrawn++;
          break;
        }
      }
    }
  }

  void MapPainter::DrawPOINodes(const StyleConfig& styleConfig,
                                const Projection& projection,
                                const MapParameter& parameter,
                                const MapData& data)
  {
    for (std::list<NodeRef>::const_iterator n=data.poiNodes.begin();
         n!=data.poiNodes.end();
         ++n) {
      const NodeRef& node=*n;

      DrawNode(styleConfig,
               projection,
               parameter,
               node);
    }
  }

  void MapPainter::DrawLabels(const StyleConfig& styleConfig,
                              const Projection& projection,
                              const MapParameter& parameter)
  {
    //
    // Draw normal
    //

    for (std::list<LabelData>::const_iterator label=labels.begin();
         label!=labels.end();
         ++label) {
      if (dynamic_cast<const TextStyle*>(label->style)!=NULL) {
        DrawLabel(projection,
                  parameter,
                  *label);
        labelsDrawn++;
      }
      else if (dynamic_cast<const ShieldStyle*>(label->style)) {
        DrawPlateLabel(projection,
                       parameter,
                       *label);
        labelsDrawn++;
      }
    }

    //
    // Draw overlays
    //

    for (std::list<LabelData>::const_iterator label=overlayLabels.begin();
         label!=overlayLabels.end();
         ++label) {
      if (dynamic_cast<const TextStyle*>(label->style)!=NULL) {
        DrawLabel(projection,
                  parameter,
                  *label);
        labelsDrawn++;
      }
      else if (dynamic_cast<const ShieldStyle*>(label->style)) {
        DrawPlateLabel(projection,
                       parameter,
                       *label);
        labelsDrawn++;
      }
    }
  }

  bool MapPainter::PrepareAreaSegment(const StyleConfig& styleConfig,
                                      const Projection& projection,
                                      const MapParameter& parameter,
                                      const ObjectRef& ref,
                                      const SegmentAttributes& attributes,
                                      const std::vector<Point>& nodes)
  {
    size_t level=MagToLevel(projection.GetMagnification());

    const FillStyle *fillStyle=styleConfig.GetAreaFillStyle(attributes.GetType(),level);

    if (fillStyle==NULL)
    {
      return false;
    }

    if (!IsVisible(projection, nodes, fillStyle->GetBorderWidth()/2)) {
      return false;
    }

    size_t start,end;

    transBuffer.TransformArea(projection,
                              parameter.GetOptimizeAreaNodes(),
                              nodes,
                              start,end,
                              parameter.GetOptimizeErrorToleranceDots());

    AreaData data;

    data.ref=ref;
    data.attributes=&attributes;
    data.fillStyle=fillStyle;
    data.transStart=start;
    data.transEnd=end;

    data.minLat=nodes[0].GetLat();
    data.maxLat=nodes[0].GetLat();
    data.minLon=nodes[0].GetLon();
    data.maxLon=nodes[0].GetLon();

    for (size_t i=1; i<nodes.size(); i++) {
      data.minLat=std::min(data.minLat,nodes[i].GetLat());
      data.maxLat=std::max(data.maxLat,nodes[i].GetLat());
      data.minLon=std::min(data.minLon,nodes[i].GetLon());
      data.maxLon=std::max(data.maxLon,nodes[i].GetLon());
    }

    areaData.push_back(data);

    areasSegments++;

    return true;
  }

  void MapPainter::PrepareAreas(const StyleConfig& styleConfig,
                                const Projection& projection,
                                const MapParameter& parameter,
                                const MapData& data)
  {
    size_t level=MagToLevel(projection.GetMagnification());

    areaData.clear();

    // Simple areas
    for (std::vector<WayRef>::const_iterator a=data.areas.begin();
         a!=data.areas.end();
         ++a) {
      const WayRef& area=*a;

      PrepareAreaSegment(styleConfig,
                         projection,
                         parameter,
                         ObjectRef(area->GetId(),refWay),
                         area->GetAttributes(),
                         area->nodes);
    }

    // external Ways that are of type 'area'
    for (std::list<WayRef>::const_iterator p=data.poiWays.begin();
         p!=data.poiWays.end();
         ++p) {
      if ((*p)->IsArea()) {
        const WayRef& area=*p;

        PrepareAreaSegment(styleConfig,
                           projection,
                           parameter,
                           ObjectRef(area->GetId(),refWay),
                           area->GetAttributes(),
                           area->nodes);
      }
    }

    //Relations
    for (std::vector<RelationRef>::const_iterator a=data.relationAreas.begin();
         a!=data.relationAreas.end();
         ++a) {
      const RelationRef& relation=*a;

      std::vector<PolyData> data(relation->roles.size());

      for (size_t i=0; i<relation->roles.size(); i++) {
        transBuffer.TransformArea(projection,
                                  parameter.GetOptimizeAreaNodes(),
                                  relation->roles[i].nodes,
                                  data[i].transStart,data[i].transEnd,
                                  parameter.GetOptimizeErrorToleranceDots());
      }

      size_t ring=0;
      bool foundRing=true;

      while (foundRing) {
        foundRing=false;

        for (size_t i=0; i<relation->roles.size(); i++) {
          const Relation::Role& role=relation->roles[i];

          if (role.ring==ring)
          {
            const FillStyle *fillStyle=NULL;

            if (ring==0) {
              if (relation->GetType()!=typeIgnore) {
                fillStyle=styleConfig.GetAreaFillStyle(relation->GetType(),level);
              }
            }
            else {
              if (role.GetType()!=typeIgnore) {
                fillStyle=styleConfig.GetAreaFillStyle(role.attributes.GetType(),level);
              }
            }

            if (fillStyle==NULL)
            {
              continue;
            }

            std::list<PolyData> clippings;

            foundRing=true;

            if (!IsVisible(projection,
                           role.nodes,
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
            while (j<relation->roles.size() &&
                   relation->roles[j].ring==ring+1 &&
                   relation->roles[j].GetType()==typeIgnore) {
              a.clippings.push_back(data[j]);

              j++;
            }

            a.ref=ObjectRef(relation->GetId(),refRelation);
            a.attributes=&role.attributes;
            a.fillStyle=fillStyle;
            a.transStart=data[i].transStart;
            a.transEnd=data[i].transEnd;

            a.minLat=role.nodes[0].GetLat();
            a.maxLat=role.nodes[0].GetLat();
            a.maxLon=role.nodes[0].GetLon();
            a.minLon=role.nodes[0].GetLon();

            for (size_t i=1; i<role.nodes.size(); i++) {
              a.minLat=std::min(a.minLat,role.nodes[i].GetLat());
              a.maxLat=std::min(a.maxLat,role.nodes[i].GetLat());
              a.minLon=std::min(a.minLon,role.nodes[i].GetLon());
              a.maxLon=std::min(a.maxLon,role.nodes[i].GetLon());
            }

            areaData.push_back(a);

            areasSegments++;
          }
        }

        ring++;
      }
    }

    areaData.sort(AreaSorter);
  }

  void MapPainter::PrepareWaySegment(const StyleConfig& styleConfig,
                                     const Projection& projection,
                                     const MapParameter& parameter,
                                     const ObjectRef& ref,
                                     const SegmentAttributes& attributes,
                                     const std::vector<Point>& nodes)
  {
    size_t level=MagToLevel(projection.GetMagnification());

    const LineStyle *lineStyle=styleConfig.GetWayLineStyle(attributes.GetType(),level);

    if (lineStyle==NULL) {
      return;
    }

    double lineWidth;

    if (lineStyle->GetWidth()==0) {
      lineWidth=ConvertWidthToPixel(parameter,lineStyle->GetDisplayWidth());
    }
    else if (parameter.GetDrawWaysWithFixedWidth() ||
        attributes.GetWidth()==0) {
      lineWidth=GetProjectedWidth(projection,
                                  ConvertWidthToPixel(parameter,lineStyle->GetDisplayWidth()),
                                  lineStyle->GetWidth());
    }
    else {
      lineWidth=GetProjectedWidth(projection,
                                  ConvertWidthToPixel(parameter,lineStyle->GetDisplayWidth()),
                                  attributes.GetWidth());
    }

    WayData data;

    data.ref=ref;
    data.lineWidth=lineWidth;

    if (lineStyle->GetOutline()>0.0) {
      double convertedOutlineWidth=ConvertWidthToPixel(parameter,2*lineStyle->GetOutline());

      if (lineStyle->GetOutlineColor().IsSolid()) {
        data.outline=lineWidth>convertedOutlineWidth;
      }
      else {
        data.outline=true;
      }

      if (data.outline) {
        data.outlineWidth=lineWidth+convertedOutlineWidth;
      }
      else {
        data.outlineWidth=lineWidth;
      }
    }
    else {
      data.outline=false;
      data.outlineWidth=data.lineWidth;
    }

    if (data.outline) {

      if (!IsVisible(projection,
                     nodes,
                     data.outlineWidth/2)) {
        return;
      }
    }
    else {
      data.outlineWidth=data.lineWidth;

      if (!IsVisible(projection,
                    nodes,
                    lineWidth/2)) {
        return;
      }
    }

    size_t start,end;

    transBuffer.TransformWay(projection,
                             parameter.GetOptimizeWayNodes(),
                             nodes,
                             start,end,
                             parameter.GetOptimizeErrorToleranceDots());

    data.attributes=&attributes;
    data.lineStyle=lineStyle;

    data.pathTextStyle=NULL;
    data.shieldStyle=NULL;

    if (!attributes.GetName().empty() || !attributes.GetRefName().empty()) {
      const ShieldStyle   *shieldStyle=styleConfig.GetWayShieldStyle(attributes.GetType(),level);
      const PathTextStyle *pathTextStyle=styleConfig.GetWayPathTextStyle(attributes.GetType(),level);

      if (shieldStyle!=NULL) {
        if (shieldStyle->GetLabel()==ShieldStyle::name &&
            !attributes.GetName().empty()) {
          data.shieldStyle=shieldStyle;
        }

        if (shieldStyle->GetLabel()==ShieldStyle::ref &&
            !attributes.GetRefName().empty()) {
          data.shieldStyle=shieldStyle;
        }
      }

      if (pathTextStyle!=NULL &&
          IsVisible(projection,
                    nodes,
                    pathTextStyle->GetSize())) {
        if (pathTextStyle->GetLabel()==PathTextStyle::name &&
            !attributes.GetName().empty()) {
          data.pathTextStyle=pathTextStyle;
        }

        if (pathTextStyle->GetLabel()==PathTextStyle::ref &&
            !attributes.GetRefName().empty()) {
          data.pathTextStyle=pathTextStyle;
        }
      }
    }

    data.prio=styleConfig.GetWayPrio(attributes.GetType());

    data.transStart=start;
    data.transEnd=end;
    data.drawBridge=attributes.IsBridge();
    data.drawTunnel=attributes.IsTunnel();

    if (data.drawBridge &&
        projection.GetMagnification()<parameter.GetDrawBridgeMagnification()) {
      data.drawBridge=false;
    }

    if (data.drawTunnel &&
        projection.GetMagnification()<parameter.GetDrawTunnelMagnification()) {
      data.drawTunnel=false;
    }

    // Drawing tunnel style for dashed lines is currently not supported
    if (data.drawTunnel &&
        lineStyle->HasDashes()) {
      data.drawTunnel=false;
    }

    if (data.drawBridge) {
      bool par1=transBuffer.GenerateParallelWay(data.transStart,data.transEnd,
                                                data.outlineWidth/2+0.5,
                                                data.par1Start, data.par1End);
      bool par2=transBuffer.GenerateParallelWay(data.transStart,data.transEnd,
                                                -(data.outlineWidth/2+0.5),
                                                data.par2Start, data.par2End);

      data.drawBridge=par1 && par2;
    }

    waysSegments++;
    wayData.push_back(data);
  }

  void MapPainter::PrepareWays(const StyleConfig& styleConfig,
                               const Projection& projection,
                               const MapParameter& parameter,
                               const MapData& data)
  {
    wayData.clear();

    for (std::vector<WayRef>::const_iterator w=data.ways.begin();
         w!=data.ways.end();
         ++w) {
      const WayRef& way=*w;

      PrepareWaySegment(styleConfig,
                        projection,
                        parameter,
                        ObjectRef(way->GetId(),refWay),
                        way->GetAttributes(),
                        way->nodes);
    }

    for (std::vector<RelationRef>::const_iterator r=data.relationWays.begin();
         r!=data.relationWays.end();
         ++r) {
      const RelationRef& relation=*r;

      for (std::vector<Relation::Role>::const_iterator r=relation->roles.begin();
          r!=relation->roles.end();
          r++) {
        const Relation::Role& role=*r;

        PrepareWaySegment(styleConfig,
                          projection,
                          parameter,
                          ObjectRef(relation->GetId(),refRelation),
                          role.GetAttributes(),
                          role.nodes);
      }
    }

    for (std::list<WayRef>::const_iterator p=data.poiWays.begin();
         p!=data.poiWays.end();
         ++p) {
      if (!(*p)->IsArea()) {
        const WayRef& way=*p;

        PrepareWaySegment(styleConfig,
                          projection,
                          parameter,
                          ObjectRef(way->GetId(),refWay),
                          way->GetAttributes(),
                          way->nodes);
      }
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
      vertical=labelSpace;
    }
  }

  bool MapPainter::Draw(const StyleConfig& styleConfig,
                        const Projection& projection,
                        const MapParameter& parameter,
                        const MapData& data)
  {
    waysSegments=0;
    waysOutlineDrawn=0;
    waysDrawn=0;
    waysLabelDrawn=0;

    areasSegments=0;
    areasDrawn=0;
    areasLabelDrawn=0;

    nodesDrawn=0;

    labelsDrawn=0;

    labels.clear();
    overlayLabels.clear();

    transBuffer.Reset();

    labelSpace=ConvertWidthToPixel(parameter,parameter.GetLabelSpace());
    shieldLabelSpace=ConvertWidthToPixel(parameter,parameter.GetPlateLabelSpace());
    sameLabelSpace=ConvertWidthToPixel(parameter,parameter.GetSameLabelSpace());

    if (parameter.IsAborted()) {
      return false;
    }

    if (parameter.IsDebugPerformance()) {
      std::cout << "Draw [";
      std::cout << projection.GetLatMin() <<",";
      std::cout << projection.GetLonMin() << " - ";
      std::cout << projection.GetLatMax() << ",";
      std::cout << projection.GetLonMax() << "] with mag. ";
      std::cout << projection.GetMagnification() << "x" << "/" << log(projection.GetMagnification())/log(2.0);
      std::cout << " area " << projection.GetWidth() << "x" << projection.GetHeight() << " " << parameter.GetDPI()<< " DPI" << std::endl;
    }

    //
    // Setup and Precalculation
    //

    StopClock prepareAreasTimer;

    PrepareAreas(styleConfig,
                 projection,
                 parameter,
                 data);

    prepareAreasTimer.Stop();

    if (parameter.IsAborted()) {
      return false;
    }

    StopClock prepareWaysTimer;

    PrepareWays(styleConfig,
                projection,
                parameter,
                data);

    prepareWaysTimer.Stop();

    if (parameter.IsAborted()) {
      return false;
    }

    //
    // Clear area with background color
    //

    DrawGroundTiles(styleConfig,
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

    DrawAreas(styleConfig,
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

    DrawWays(styleConfig,
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

    DrawWayDecorations(styleConfig,
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

    DrawWayLabels(styleConfig,
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

    DrawNodes(styleConfig,
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

    DrawAreaLabels(styleConfig,
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

    DrawPOINodes(styleConfig,
                 projection,
                 parameter,
                 data);

    poisTimer.Stop();

    if (parameter.IsAborted()) {
      return false;
    }

    StopClock labelsTimer;

    DrawLabels(styleConfig,
               projection,
               parameter);

    labelsTimer.Stop();

    if (parameter.IsDebugPerformance()) {

      std::cout << "Paths: ";
      std::cout << data.ways.size() << "+" << data.relationWays.size() << "/" << waysSegments << "/" << waysDrawn << "/" << waysOutlineDrawn << "/" << waysLabelDrawn << " (pcs) ";
      std::cout << prepareWaysTimer << "/" << pathsTimer << "/" << pathLabelsTimer << " (sec)" << std::endl;

      std::cout << "Areas: ";
      std::cout << data.areas.size() << "+" << data.relationAreas.size() << "/" << areasSegments << "/" << areasDrawn << "/" << areasLabelDrawn << " (pcs) ";
      std::cout << prepareAreasTimer << "/" << areasTimer << "/" << areaLabelsTimer << " (sec)" << std::endl;

      std::cout << "Nodes: ";
      std::cout << data.nodes.size() <<"+" << data.poiNodes.size() << "/" << nodesDrawn << " (pcs) ";
      std::cout << nodesTimer << "/" << poisTimer << " (sec)" << std::endl;

      std::cout << "Labels: " << labels.size() << "/" << overlayLabels.size() << "/" << labelsDrawn << " (pcs) ";
      std::cout << labelsTimer << " (sec)" << std::endl;
    }

    return true;
  }
}
