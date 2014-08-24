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

#include <osmscout/system/Math.h>

#include <osmscout/util/HashSet.h>
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

  MapParameter::MapParameter()
  : dpi(96.0),
    fontName("sans-serif"),
    fontSize(2.0),
    lineMinWidthPixel(0.2),
    optimizeWayNodes(TransPolygon::none),
    optimizeAreaNodes(TransPolygon::none),
    optimizeErrorToleranceMm(25.4/dpi), //1 pixel
    drawFadings(true),
    drawWaysWithFixedWidth(false),
    labelSpace(3.0),
    plateLabelSpace(5.0),
    sameLabelSpace(40.0),
    dropNotVisiblePointLabels(true),
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

  void MapParameter::SetDropNotVisiblePointLabels(bool dropNotVisiblePointLabels)
  {
    this->dropNotVisiblePointLabels=dropNotVisiblePointLabels;
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

  MapPainter::MapPainter(const StyleConfigRef& styleConfig,
                         CoordBuffer *buffer)
  : coordBuffer(buffer),
    styleConfig(styleConfig),
    transBuffer(coordBuffer),
    nameReader(styleConfig->GetTypeConfig()),
    nameAltReader(styleConfig->GetTypeConfig()),
    refReader(styleConfig->GetTypeConfig()),
    layerReader(styleConfig->GetTypeConfig()),
    widthReader(styleConfig->GetTypeConfig()),
    addressReader(styleConfig->GetTypeConfig()),
    labelSpace(1.0),
    shieldLabelSpace(1.0),
    sameLabelSpace(1.0)
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
  }

  MapPainter::~MapPainter()
  {
    // no code
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

    yMax = std::max(y1,y2);
    yMin = std::min(y1,y2);

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
      if (projection.GetMagnification()>textStyle->GetScaleAndFadeMag() &&
          parameter.GetDrawFadings()) {
        double factor=projection.GetMagnification().GetLevel()-textStyle->GetScaleAndFadeMag().GetLevel();
        fontSize=fontSize*pow(1.5,factor);
        alpha=alpha/factor;

        if (alpha>1.0) {
          alpha=1.0;
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
          dynamic_cast<const ShieldStyle*>(label.style.Get())!=NULL) {
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

  bool MapPainter::GetBoundingBox(const std::vector<GeoCoord>& nodes,
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
                                  const std::vector<GeoCoord>& nodes,
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
                                 parameter.GetDPI(),
                                 landFill);

    if (landFill.Invalid()) {
      landFill=this->landFill;
    }

    DrawGround(projection,
               parameter,
               *landFill);

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

    styleConfig.GetSeaFillStyle(projection,
                                parameter.GetDPI(),
                                seaFill);
    styleConfig.GetCoastFillStyle(projection,
                                  parameter.GetDPI(),
                                  coastFill);
    styleConfig.GetUnknownFillStyle(projection,
                                    parameter.GetDPI(),
                                    unknownFill);
    styleConfig.GetCoastlineLineStyle(projection,
                                      parameter.GetDPI(),
                                      coastlineLine);

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

        transBuffer.transPolygon.TransformArea(projection,
                                               TransPolygon::none,
                                               points,
                                               parameter.GetOptimizeErrorToleranceDots());

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
                                               parameter.GetOptimizeErrorToleranceDots());

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

              data.buffer=&coastlineSegmentAttributes;
              data.layer=0;
              data.lineStyle=coastlineLine;
              data.wayPriority=std::numeric_limits<size_t>::max();
              data.transStart=start+lineStart;
              data.transEnd=start+lineEnd;
              data.lineWidth=GetProjectedWidth(projection,
                                               ConvertWidthToPixel(parameter,coastlineLine->GetDisplayWidth()),
                                               coastlineLine->GetWidth());
              wayData.push_back(data);
            }

            lineStart=lineEnd+1;
          }
        }
      }

      areaData.ref=ObjectFileRef();
//      areaData.buffer.SetType();
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
    double stepSizeInPixel=ConvertWidthToPixel(parameter,shieldStyle->GetShieldSpace());

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
                         wayScanlines[i].x+0.5,
                         wayScanlines[i].y+0.5);

      i+=stepSizeInPixel;
    }
  }

  bool MapPainter::RegisterPointLabel(const Projection& projection,
                                      const MapParameter& parameter,
                                      const LabelStyleRef& style,
                                      const std::string& text,
                                      double x,
                                      double y)
  {
    double fontSize=style->GetSize();
    double a=style->GetAlpha();

    CalculateEffectiveLabelStyle(projection,
                                 parameter,
                                 *dynamic_cast<const TextStyle*>(style.Get()),
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

    GetLabelFrame(*style,
                  frameHoriz,
                  frameVert);

    bx1=x-frameHoriz;
    bx2=x+frameHoriz;
    by1=y-fontSize/2-frameVert;
    by2=y+fontSize/2+frameVert;

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
    label.alpha=a;
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

  void MapPainter::DrawAreas(const StyleConfig& /*styleConfig*/,
                             const Projection& projection,
                             const MapParameter& parameter,
                             const MapData& /*data*/)
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

  void MapPainter::DrawAreaLabel(const StyleConfig& styleConfig,
                                 const Projection& projection,
                                 const MapParameter& parameter,
                                 const TypeId& type,
                                 const FeatureValueBuffer& buffer,
                                 double x,
                                 double y)
  {
    TextStyleRef  textStyle;
    IconStyleRef  iconStyle;

    styleConfig.GetAreaTextStyle(type,
                                 buffer,
                                 projection,
                                 parameter.GetDPI(),
                                 textStyle);
    styleConfig.GetAreaIconStyle(type,
                                 buffer,
                                 projection,
                                 parameter.GetDPI(),
                                 iconStyle);

    bool          hasLabel=textStyle.Valid();
    bool          hasSymbol=iconStyle.Valid() && iconStyle->GetSymbol().Valid();
    bool          hasIcon=iconStyle.Valid() && !iconStyle->GetIconName().empty();
    std::string   label;

    if (hasIcon) {
      hasIcon=HasIcon(styleConfig,
                      parameter,
                      *iconStyle);
    }

    if (!hasSymbol && !hasLabel && !hasIcon) {
      return;
    }

    if (hasLabel) {
      NameFeatureValue    *nameValue=nameReader.GetValue(buffer);
      AddressFeatureValue *addressValue=addressReader.GetValue(buffer);

      if (nameValue!=NULL) {
        label=nameValue->GetName();
      }
      else if (addressValue!=NULL) {
        label=addressValue->GetAddress();
      }

      hasLabel=!label.empty();
    }

    if (!hasSymbol && !hasLabel && !hasIcon) {
      return;
    }

    if (hasLabel) {
      if (hasSymbol) {
        RegisterPointLabel(projection,
                           parameter,
                           textStyle,
                           label,
                           x,y+ConvertWidthToPixel(parameter,iconStyle->GetSymbol()->GetHeight())/2+
                               ConvertWidthToPixel(parameter,1.0)+
                               ConvertWidthToPixel(parameter,textStyle->GetSize())/2);
      }
      else if (hasIcon) {
        RegisterPointLabel(projection,
                           parameter,
                           textStyle,
                           label,
                           x,y+14+5); // TODO: Better layout to real size of icon
      }
      else {
        RegisterPointLabel(projection,
                           parameter,
                           textStyle,
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

  void MapPainter::DrawAreaLabels(const StyleConfig& styleConfig,
                                  const Projection& projection,
                                  const MapParameter& parameter,
                                  const MapData& data)
  {
    for (std::vector<AreaRef>::const_iterator r=data.areas.begin();
         r!=data.areas.end();
         ++r) {
      const AreaRef& area=*r;

      for (size_t m=0; m<area->rings.size(); m++) {
        if (area->rings[m].ring==Area::masterRingId) {
          double lat,lon;
          double x,y;

          area->GetCenter(lat,lon);

          projection.GeoToPixel(lon,
                                lat,
                                x,y);

          DrawAreaLabel(styleConfig,
                        projection,
                        parameter,
                        area->GetType()->GetId(),
                        area->rings[m].GetFeatureValueBuffer(),
                        x,y);
        }
        else {
          double lat,lon;
          double x,y;

          area->rings[m].GetCenter(lat,lon);

          projection.GeoToPixel(lon,
                                lat,
                                x,y);

          DrawAreaLabel(styleConfig,
                        projection,
                        parameter,
                        area->rings[m].GetType()->GetId(),
                        area->rings[m].GetFeatureValueBuffer(),
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
    TextStyleRef textStyle;
    IconStyleRef iconStyle;

    styleConfig.GetNodeTextStyle(node->GetFeatureValueBuffer(),
                                 projection,
                                 parameter.GetDPI(),
                                 textStyle);
    styleConfig.GetNodeIconStyle(node->GetFeatureValueBuffer(),
                                 projection,
                                 parameter.GetDPI(),
                                 iconStyle);

    bool         hasLabel=textStyle.Valid();
    bool         hasSymbol=iconStyle.Valid() && iconStyle->GetSymbol().Valid();
    bool         hasIcon=iconStyle.Valid() && !iconStyle->GetIconName().empty();
    std::string  label;

    if (hasLabel) {
      NameFeatureValue    *nameValue=nameReader.GetValue(node->GetFeatureValueBuffer());
      AddressFeatureValue *addressValue=addressReader.GetValue(node->GetFeatureValueBuffer());

      if (nameValue!=NULL) {
        label=nameValue->GetName();
      }
      else if (addressValue!=NULL) {
        label=addressValue->GetAddress();
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
                           textStyle,
                           label,
                           x,y+ConvertWidthToPixel(parameter,iconStyle->GetSymbol()->GetHeight())/2+
                               ConvertWidthToPixel(parameter,1.0)+
                               ConvertWidthToPixel(parameter,textStyle->GetSize())/2);
      }
      else if (hasIcon) {
        RegisterPointLabel(projection,
                           parameter,
                           textStyle,
                           label,
                           x,y+14+5); // TODO: Better layout to real size of icon
      }
      else {
        RegisterPointLabel(projection,
                           parameter,
                           textStyle,
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
    for (std::list<WayData>::const_iterator way=wayData.begin();
         way!=wayData.end();
         ++way) {
      DrawWay(styleConfig,
              projection,
              parameter,
              *way);
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
                                        parameter.GetDPI(),
                                        pathSymbolStyle);

      if (pathSymbolStyle.Valid()) {
        double symbolSpace=ConvertWidthToPixel(parameter,
                                               pathSymbolStyle->GetSymbolSpace());

        DrawContourSymbol(projection,
                          parameter,
                          pathSymbolStyle->GetSymbol(),
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
    NameFeatureValue *nameValue=nameReader.GetValue(*data.buffer);
    RefFeatureValue  *refValue=refReader.GetValue(*data.buffer);


    if (nameValue== NULL &&
        refValue==NULL) {
      return;
    }

    PathShieldStyleRef shieldStyle;
    PathTextStyleRef   pathTextStyle;

    styleConfig.GetWayPathShieldStyle(*data.buffer,
                                      projection,
                                      parameter.GetDPI(),
                                      shieldStyle);
    styleConfig.GetWayPathTextStyle(*data.buffer,
                                    projection,
                                    parameter.GetDPI(),
                                    pathTextStyle);

    if (pathTextStyle.Valid()) {
      switch (pathTextStyle->GetLabel()) {
      case PathTextStyle::none:
        break;
      case PathTextStyle::name:
        if (nameValue!=NULL) {
          DrawContourLabel(projection,
                           parameter,
                           *pathTextStyle,
                           nameValue->GetName(),
                           data.transStart,
                           data.transEnd);
          waysLabelDrawn++;
        }
        break;
      case PathTextStyle::ref:
        if (refValue!=NULL) {
          DrawContourLabel(projection,
                           parameter,
                           *pathTextStyle,
                           refValue->GetRef(),
                           data.transStart,
                           data.transEnd);
          waysLabelDrawn++;
        }
        break;
      }
    }

    if (shieldStyle.Valid()) {
      switch(shieldStyle->GetLabel()) {
      case ShieldStyle::none:
        break;
      case ShieldStyle::name:
        if (nameValue!=NULL) {
          RegisterPointWayLabel(projection,
                                parameter,
                                shieldStyle,
                                nameValue->GetName(),
                                data.transStart,
                                data.transEnd);
          waysLabelDrawn++;
        }
        break;
      case ShieldStyle::ref:
        if (refValue!=NULL) {
          RegisterPointWayLabel(projection,
                                parameter,
                                shieldStyle,
                                refValue->GetRef(),
                                data.transStart,
                                data.transEnd);
          waysLabelDrawn++;
        }
        break;
      }
    }
  }

  void MapPainter::DrawWayLabels(const StyleConfig& styleConfig,
                                 const Projection& projection,
                                 const MapParameter& parameter,
                                 const MapData& /*data*/)
  {
    for (std::list<WayPathData>::const_iterator way=wayPathData.begin();
        way!=wayPathData.end();
        way++) {
      DrawWayLabel(styleConfig,
                   projection,
                   parameter,
                   *way);
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

  void MapPainter::DrawLabels(const StyleConfig& /*styleConfig*/,
                              const Projection& projection,
                              const MapParameter& parameter)
  {
    //
    // Draw normal
    //

    for (std::list<LabelData>::const_iterator label=labels.begin();
         label!=labels.end();
         ++label) {
      DrawLabel(projection,
                parameter,
                *label);
      labelsDrawn++;
    }

    //
    // Draw overlays
    //

    for (std::list<LabelData>::const_iterator label=overlayLabels.begin();
         label!=overlayLabels.end();
         ++label) {
      DrawLabel(projection,
                parameter,
                *label);
      labelsDrawn++;
    }
  }

  void MapPainter::PrepareAreas(const StyleConfig& styleConfig,
                                const Projection& projection,
                                const MapParameter& parameter,
                                const MapData& data)
  {
    areaData.clear();

    //Areas
    for (std::vector<AreaRef>::const_iterator a=data.areas.begin();
         a!=data.areas.end();
         ++a) {
      const AreaRef& area=*a;

      std::vector<PolyData> data(area->rings.size());

      for (size_t i=0; i<area->rings.size(); i++) {
        if (area->rings[i].ring==Area::masterRingId) {
          continue;
        }

        transBuffer.TransformArea(projection,
                                  parameter.GetOptimizeAreaNodes(),
                                  area->rings[i].nodes,
                                  data[i].transStart,data[i].transEnd,
                                  parameter.GetOptimizeErrorToleranceDots());
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
              styleConfig.GetAreaFillStyle(area->GetType()->GetId(),
                                           ring.GetFeatureValueBuffer(),
                                           projection,
                                           parameter.GetDPI(),
                                           fillStyle);
            }
            else if (ring.GetType()->GetId()!=typeIgnore) {
              styleConfig.GetAreaFillStyle(ring.GetType()->GetId(),
                                           ring.GetFeatureValueBuffer(),
                                           projection,
                                           parameter.GetDPI(),
                                           fillStyle);
            }

            if (fillStyle.Invalid())
            {
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
                   area->rings[j].GetType()->GetId()==typeIgnore) {
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

            for (size_t i=1; i<ring.nodes.size(); i++) {
              a.minLat=std::min(a.minLat,ring.nodes[i].GetLat());
              a.maxLat=std::min(a.maxLat,ring.nodes[i].GetLat());
              a.minLon=std::min(a.minLon,ring.nodes[i].GetLon());
              a.maxLon=std::min(a.maxLon,ring.nodes[i].GetLon());
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
                                 parameter.GetDPI(),
                                 lineStyles);

    if (lineStyles.empty()) {
      return;
    }

    bool   transformed=false;
    size_t transStart=0; // Make the compiler happy
    size_t transEnd=0;   // Make the compiler happy

    for (std::vector<LineStyleRef>::const_iterator ls=lineStyles.begin();
         ls!=lineStyles.end();
         ++ls) {
      LineStyleRef lineStyle(*ls);
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
        lineWidth+=ConvertWidthToPixel(parameter,
                                       lineStyle->GetDisplayWidth());
      }

      if (lineWidth==0.0) {
        continue;
      }

      if (lineStyle->GetOffset()!=0.0) {
        lineOffset+=GetProjectedWidth(projection,
                                      lineStyle->GetOffset());
      }

      if (lineStyle->GetDisplayOffset()!=0.0) {
        lineOffset+=ConvertWidthToPixel(parameter,
                                        lineStyle->GetDisplayOffset());
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
                                 parameter.GetOptimizeErrorToleranceDots());

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
      data.wayPriority=styleConfig.GetWayPrio(buffer.GetTypeId());
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

    for (std::vector<WayRef>::const_iterator w=data.ways.begin();
         w!=data.ways.end();
         ++w) {
      const WayRef& way=*w;

      PrepareWaySegment(styleConfig,
                        projection,
                        parameter,
                        ObjectFileRef(way->GetFileOffset(),refWay),
                        way->GetFeatureValueBuffer(),
                        way->nodes,
                        way->ids);
    }

    for (std::list<WayRef>::const_iterator p=data.poiWays.begin();
         p!=data.poiWays.end();
         ++p) {
      const WayRef& way=*p;

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
      vertical=labelSpace;
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
      std::cout << "Draw: [";
      std::cout << projection.GetLatMin() <<",";
      std::cout << projection.GetLonMin() << "-";
      std::cout << projection.GetLatMax() << ",";
      std::cout << projection.GetLonMax() << "] ";
      std::cout << projection.GetMagnification().GetMagnification() << "x" << "/" << projection.GetMagnification().GetLevel() << " ";
      std::cout << projection.GetWidth() << "x" << projection.GetHeight() << " " << parameter.GetDPI()<< " DPI" << std::endl;
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

    // Optional callback after preprocessing data
    AfterPreprocessing(styleConfig,
                       projection,
                       parameter,
                       data);

    // Optional callback after preprocessing data
    BeforeDrawing(styleConfig,
                  projection,
                  parameter,
                  data);

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

    AfterDrawing(styleConfig,
                 projection,
                 parameter,
                 data);

    if (parameter.IsDebugPerformance()) {
      std::cout << "Paths: ";
      std::cout << data.ways.size() << "/" << waysSegments << "/" << waysDrawn << "/" << waysLabelDrawn << " (pcs) ";
      std::cout << prepareWaysTimer << "/" << pathsTimer << "/" << pathLabelsTimer << " (sec)" << std::endl;

      std::cout << "Areas: ";
      std::cout << data.areas.size() << "/" << areasSegments << "/" << areasDrawn << "/" << areasLabelDrawn << " (pcs) ";
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
