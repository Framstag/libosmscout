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

#include <osmscout/private/Math.h>

namespace osmscout {

  static inline bool AreaSortByLon(const MapPainter::AreaData& a, const MapPainter::AreaData& b)
  {
    return a.minLon<b.minLon;
  }

  MapParameter::MapParameter()
  : dpi(96.0),
    fontName("sans-serif"),
    fontSize(2.0),
    lineMinWidthPixel(0.2),
    drawBridgeMagnification(magVeryClose),
    drawTunnelMagnification(magVeryClose),
    optimizeWayNodes(false),
    optimizeAreaNodes(false),
    drawFadings(true),
    drawWaysWithFixedWidth(false),
    labelSpace(3.0),
    plateLabelSpace(5.0),
    sameLabelSpace(40.0),
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

  void MapParameter::SetOptimizeWayNodes(bool optimize)
  {
    optimizeWayNodes=optimize;
  }

  void MapParameter::SetOptimizeAreaNodes(bool optimize)
  {
    optimizeAreaNodes=optimize;
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

  void MapParameter::SetDebugPerformance(bool debug)
  {
    debugPerformance=debug;
  }

  MapPainter::MapPainter()
  {
    tunnelDash.push_back(0.4);
    tunnelDash.push_back(0.4);

    areaMarkStyle.SetStyle(FillStyle::plain);
    areaMarkStyle.SetFillColor(1,0,0,0.5);
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

    if (!projection.GeoToPixel(lonMin,
                               latMax,
                               xMin,
                               yMin)) {
      return false;
    }

    if (!projection.GeoToPixel(lonMax,
                               latMin,
                               xMax,
                               yMax)) {
      return false;
    }

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
    // Calculate effective font size and alpha value
    if (projection.GetMagnification()>style.GetScaleAndFadeMag()) {
      if (parameter.GetDrawFadings()) {
        double factor=log2(projection.GetMagnification())-log2(style.GetScaleAndFadeMag());
        fontSize=fontSize*pow(2,factor);
        alpha=alpha/factor;

        if (alpha>1.0) {
          alpha=1.0;
        }
      }
    }
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
    FillStyle landFill;
    FillStyle seeFill;
    FillStyle coastFill;
    FillStyle unknownFill;
    LabelStyle labelStyle;

    landFill.SetFillColor(241.0/255,238.0/255,233.0/255,1.0);
    seeFill.SetFillColor(181.0/255,208.0/255,208.0/255,1.0);
    coastFill.SetFillColor(255.0/255,192.0/255,203.0/255,1.0);
    unknownFill.SetFillColor(255.0/255,255.0/255,173.0/255,1.0);

    labelStyle.SetStyle(LabelStyle::normal);
    labelStyle.SetMinMag(magCity);

    DrawArea(landFill,
             parameter,
             0,0,projection.GetWidth(),projection.GetHeight());

    return;
/*
    double cellWidth=360.0;
    double cellHeight=180.0;

    for (size_t i=2; i<=14; i++) {
      cellWidth=cellWidth/2;
      cellHeight=cellHeight/2;
    }*/

    for (std::list<GroundTile>::const_iterator tile=data.groundTiles.begin();
        tile!=data.groundTiles.end();
        ++tile) {
      double x1,x2,y1,y2;

      projection.GeoToPixel(tile->minlon,tile->minlat,x1,y1);
      projection.GeoToPixel(tile->maxlon,tile->maxlat,x2,y2);

      switch (tile->type) {
      case GroundTile::land:
        DrawArea(landFill,
                 parameter,
                 x1,y1,x2-x1,y2-y1);
        break;
      case GroundTile::water:
        DrawArea(seeFill,
                 parameter,
                 x1,y1,x2-x1,y2-y1);
        break;
      case GroundTile::coast:
        DrawArea(coastFill,
                 parameter,
                 x1,y1,x2-x1,y2-y1);
        break;
      case GroundTile::unknown:
        DrawArea(unknownFill,
                 parameter,
                 x1,y1,x2-x1,y2-y1);
        break;
      }
/*
      std::string label;

      label=NumberToString((long)((tile->minlon+180)/cellWidth));
      label+=",";
      label+=NumberToString((long)((tile->minlat+90)/cellHeight));

      DrawLabel(projection,parameter,labelStyle,label,x1+(x2-x1)/2,y1+(y2-y1)/2);*/
    }
  }

  void MapPainter::RegisterPointWayLabel(const Projection& projection,
                                         const MapParameter& parameter,
                                         const LabelStyle& style,
                                         const std::string& text,
                                         size_t transStart, size_t transEnd)
  {
    if (style.GetStyle()!=LabelStyle::plate) {
      return;
    }

    double stepSizeInPixel=ConvertWidthToPixel(parameter,parameter.GetLabelSpace());

    wayScanlines.clear();
    ScanConvertLine(transStart,transEnd,1,1,wayScanlines);

    size_t i=0;
    while (i<wayScanlines.size()) {
      RegisterPointLabel(projection,
                         parameter,
                         style,
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
    assert(style.IsPointStyle());

    double fontSize=style.GetSize();
    double a=style.GetTextA();
    double labelSpace=ConvertWidthToPixel(parameter,parameter.GetLabelSpace());
    double plateLabelSpace=ConvertWidthToPixel(parameter,parameter.GetPlateLabelSpace());
    double sameLabelSpace=ConvertWidthToPixel(parameter,parameter.GetSameLabelSpace());

    CalculateEffectiveLabelStyle(projection,
                                 parameter,
                                 style,
                                 fontSize,
                                 a);

    // Something is a overlay, if its alpha is <0.8
    bool overlay=a<0.8;

    // Reset all marks on labels, if something is solid, because we needs marks
    // for our internal collision handling
    if (!overlay) {
      for (size_t i=0; i<labels.size(); i++) {
        labels[i].mark=false;
      }
    }

    if (!overlay) {
      // First rough minimum bounding box, estimated without calculating text dimensions (since this is expensive).
      double bx1;
      double bx2;
      double by1;
      double by2;

      if (style.GetStyle()==LabelStyle::plate) {
        bx1=x-1-4;
        bx2=x+1+4;
        by1=y-fontSize/2-1-4;
        by2=y+fontSize/2+1+4;
      }
      else {
        bx1=x;
        bx2=x;
        by1=y-fontSize/2;
        by2=y+fontSize/2;
      }

      for (std::vector<LabelData>::iterator l=labels.begin();
          l!=labels.end();
          ++l) {
        LabelData& label=*l;

        if (label.overlay || !label.draw || label.mark) {
          continue;
        }

        double hx1;
        double hx2;
        double hy1;
        double hy2;

        if (style.GetStyle()==LabelStyle::plate &&
            label.style->GetStyle()==LabelStyle::plate) {
          hx1=bx1-plateLabelSpace;
          hx2=bx2+plateLabelSpace;
          hy1=by1-plateLabelSpace;
          hy2=by2+plateLabelSpace;
        }
        else {
          hx1=bx1-labelSpace;
          hx2=bx2+labelSpace;
          hy1=by1-labelSpace;
          hy2=by2+labelSpace;
        }

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
    }

    // We passed the first intersection test, we now calculate the real bounding box
    // by measuring text dimensions

    double xOff,yOff,width,height;

    GetTextDimension(parameter,
                     fontSize,
                     text,
                     xOff,yOff,width,height);

    double bx1;
    double bx2;
    double by1;
    double by2;

    if (style.GetStyle()==LabelStyle::plate) {
      bx1=x-width/2-4;
      bx2=x+width/2+4;
      by1=y-height/2-4;
      by2=y+height/2+4;
    }
    else {
      bx1=x-width/2;
      bx2=x+width/2;
      by1=y-height/2;
      by2=y+height/2;
    }

    // is box visible?
    if (bx1>=projection.GetWidth() ||
        bx2<0 ||
        by1>=projection.GetHeight() ||
        by2<0) {
      return false;
    }

    // again for non-overlay labels we check for intersection with an existing label
    // but now with the complete bounding box

    if (!overlay) {
      for (std::vector<LabelData>::iterator l=labels.begin();
          l!=labels.end();
          ++l) {
        LabelData& label=*l;

        if (label.overlay || !label.draw || label.mark) {
          continue;
        }

        double hx1;
        double hx2;
        double hy1;
        double hy2;

        if (style.GetStyle()==LabelStyle::plate &&
            label.style->GetStyle()==LabelStyle::plate) {
          hx1=bx1-plateLabelSpace;
          hx2=bx2+plateLabelSpace;
          hy1=by1-plateLabelSpace;
          hy2=by2+plateLabelSpace;
        }
        else {
          hx1=bx1-labelSpace;
          hx2=bx2+labelSpace;
          hy1=by1-labelSpace;
          hy2=by2+labelSpace;
        }

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
    }

    // As an optional final processing step, we check if labels
    // with the same value (text) have at least sameLabelSpace distance.
    if (!overlay &&
        sameLabelSpace>plateLabelSpace) {
      for (std::vector<LabelData>::iterator l=labels.begin();
          l!=labels.end();
          ++l) {
        LabelData& label=*l;

        if (label.overlay || !label.draw || label.mark) {
          continue;
        }

        if (style.GetStyle()==label.style->GetStyle() &&
            style.GetPriority()==label.style->GetPriority()) {
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
    }

    if (!overlay) {
      // Set everything else to draw=false, if we marked it before as "intersecting but of lower
      // priority". We also add our own (future) index to all the cells we intersect with

      for (std::vector<LabelData>::iterator l=labels.begin();
           l!=labels.end();
           ++l) {
        LabelData& label=*l;

        if (label.mark) {
          label.mark=false;
          label.draw=false;
        }
      }
    }

    // We passed the test, lets put ourself into the "draw label" job list

    LabelData label;

    label.draw=true;
    label.overlay=overlay;
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

    labels.push_back(label);

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
    for (std::vector<WayRef>::const_iterator a=data.areas.begin();
         a!=data.areas.end();
         ++a) {
      const WayRef& area=*a;

      const LabelStyle  *labelStyle=styleConfig.GetAreaLabelStyle(area->GetType());
      IconStyle         *iconStyle=styleConfig.GetAreaIconStyle(area->GetType());
      const SymbolStyle *symbolStyle=iconStyle!=NULL ? NULL : styleConfig.GetAreaSymbolStyle(area->GetType());

      bool hasLabel=labelStyle!=NULL &&
                    labelStyle->GetStyle()!=LabelStyle::none &&
                    projection.GetMagnification()>=labelStyle->GetMinMag() &&
                    projection.GetMagnification()<=labelStyle->GetMaxMag();

      bool hasSymbol=symbolStyle!=NULL &&
                     projection.GetMagnification()>=symbolStyle->GetMinMag();

      bool hasIcon=iconStyle!=NULL &&
                   projection.GetMagnification()>=iconStyle->GetMinMag();

      std::string label;

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

      if (!GetCenterPixel(projection,area->nodes,x,y)) {
        continue;
      }

      if (hasLabel) {
        if (hasSymbol) {
          RegisterPointLabel(projection,
                             parameter,
                             *labelStyle,
                             label,
                             x,y+symbolStyle->GetSize()+5); // TODO: Better layout to real size of symbol
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

      if (hasSymbol) {
        DrawSymbol(symbolStyle,x,y);
      }
    }

    for (std::vector<RelationRef>::const_iterator r=data.relationAreas.begin();
         r!=data.relationAreas.end();
         ++r) {
      const RelationRef& relation=*r;

      for (size_t m=0; m<relation->roles.size(); m++) {
        const LabelStyle  *labelStyle=styleConfig.GetAreaLabelStyle(relation->roles[m].GetType());
        IconStyle         *iconStyle=styleConfig.GetAreaIconStyle(relation->roles[m].GetType());
        const SymbolStyle *symbolStyle=iconStyle!=NULL ? NULL : styleConfig.GetAreaSymbolStyle(relation->roles[m].GetType());

        bool hasLabel=labelStyle!=NULL &&
                      labelStyle->GetStyle()!=LabelStyle::none &&
                      projection.GetMagnification()>=labelStyle->GetMinMag() &&
                      projection.GetMagnification()<=labelStyle->GetMaxMag();

        bool hasSymbol=symbolStyle!=NULL &&
                       projection.GetMagnification()>=symbolStyle->GetMinMag();

        bool hasIcon=iconStyle!=NULL &&
                     projection.GetMagnification()>=iconStyle->GetMinMag();

        std::string label;

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

          hasLabel=!label.empty();
        }

        if (!hasSymbol && !hasLabel && !hasIcon) {
          continue;
        }

        double x,y;

        if (!GetCenterPixel(projection,relation->roles[m].nodes,x,y)) {
          continue;
        }

        if (hasLabel) {
          if (hasSymbol) {
            RegisterPointLabel(projection,
                               parameter,
                               *labelStyle,
                               label,
                               x,y+symbolStyle->GetSize()+5); // TODO: Better layout to real size of symbol
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

        if (hasSymbol) {
          DrawSymbol(symbolStyle,x,y);
        }
      }
    }
  }

  void MapPainter::DrawNode(const StyleConfig& styleConfig,
                            const Projection& projection,
                            const MapParameter& parameter,
                            const NodeRef& node)
  {
    const LabelStyle  *labelStyle=styleConfig.GetNodeLabelStyle(node->GetType());
    IconStyle         *iconStyle=styleConfig.GetNodeIconStyle(node->GetType());
    const SymbolStyle *symbolStyle=iconStyle!=NULL ? NULL : styleConfig.GetNodeSymbolStyle(node->GetType());

    bool hasLabel=labelStyle!=NULL &&
                  projection.GetMagnification()>=labelStyle->GetMinMag() &&
                  projection.GetMagnification()<=labelStyle->GetMaxMag();

    bool hasSymbol=symbolStyle!=NULL &&
                   projection.GetMagnification()>=symbolStyle->GetMinMag();

    bool hasIcon=iconStyle!=NULL &&
                 projection.GetMagnification()>=iconStyle->GetMinMag();

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
                           x,y+symbolStyle->GetSize()+5); // TODO: Better layout to real size of symbol
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

    if (hasSymbol) {
      DrawSymbol(symbolStyle,x,y);
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
                 data.lineStyle->GetOutlineR(),
                 data.lineStyle->GetOutlineG(),
                 data.lineStyle->GetOutlineB(),
                 data.lineStyle->GetOutlineA(),
                 data.outlineWidth,
                 tunnelDash,
                 data.attributes->StartIsJoint() ? capButt : capRound,
                 data.attributes->EndIsJoint() ? capButt : capRound,
                 data.transStart,data.transEnd);
      }
      else if (projection.GetMagnification()>=10000) {
        // light grey dashes

        DrawPath(projection,
                 parameter,
                 0.5,
                 0.5,
                 0.5,
                 1.0,
                 data.outlineWidth,
                 tunnelDash,
                 data.attributes->StartIsJoint() ? capButt : capRound,
                 data.attributes->EndIsJoint() ? capButt : capRound,
                 data.transStart,data.transEnd);
      }
      else {
        // dark grey dashes

        DrawPath(projection,
                 parameter,
                 0.5,
                 0.5,
                 0.5,
                 1.0,
                 data.outlineWidth,
                 tunnelDash,
                 data.attributes->StartIsJoint() ? capButt : capRound,
                 data.attributes->EndIsJoint() ? capButt : capRound,
                     data.transStart,data.transEnd);
      }
    }
    else {
      // normal path, normal outline color

      DrawPath(projection,
               parameter,
               data.lineStyle->GetOutlineR(),
               data.lineStyle->GetOutlineG(),
               data.lineStyle->GetOutlineB(),
               data.lineStyle->GetOutlineA(),
               data.outlineWidth,
               emptyDash,
               data.attributes->StartIsJoint() ? capButt : capRound,
               data.attributes->EndIsJoint() ? capButt : capRound,
               data.transStart,data.transEnd);
    }

    waysOutlineDrawn++;
  }

  void MapPainter::DrawWay(const StyleConfig& styleConfig,
                           const Projection& projection,
                           const MapParameter& parameter,
                           const WayData& data)
  {
    double r,g,b,a;

    if (data.outline) {
      // Draw line with normal color
      r=data.lineStyle->GetLineR();
      g=data.lineStyle->GetLineG();
      b=data.lineStyle->GetLineB();
      a=data.lineStyle->GetLineA();
    }
    else {
      // Should draw outline, but resolution is too low
      // Draw line with alternate color
      r=data.lineStyle->GetAlternateR();
      g=data.lineStyle->GetAlternateG();
      b=data.lineStyle->GetAlternateB();
      a=data.lineStyle->GetAlternateA();
    }

    if (data.drawTunnel) {
      r=r+(1-r)*50/100;
      g=g+(1-g)*50/100;
      b=b+(1-b)*50/100;
    }

    if (!data.lineStyle->GetDash().empty() &&
        data.lineStyle->GetGapA()>0.0) {
      DrawPath(projection,
               parameter,
               data.lineStyle->GetGapR(),
               data.lineStyle->GetGapG(),
               data.lineStyle->GetGapB(),
               data.lineStyle->GetGapA(),
               data.lineWidth,
               emptyDash,
               capRound,
               capRound,
               data.transStart,data.transEnd);
    }

    DrawPath(projection,
             parameter,
             r,g,b,a,
             data.lineWidth,
             data.lineStyle->GetDash(),
             capRound,
             capRound,
             data.transStart,data.transEnd);

    if (data.drawBridge) {
      DrawPath(projection,
               parameter,
               0.0,
               0.0,
               0.0,
               1.0,
               1,
               emptyDash,
               capButt,
               capButt,
               data.par1Start,data.par1End);

      DrawPath(projection,
               parameter,
               0.0,
               0.0,
               0.0,
               1.0,
               1,
               emptyDash,
               capButt,
               capButt,
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


  void MapPainter::DrawWayLabels(const StyleConfig& styleConfig,
                                 const Projection& projection,
                                 const MapParameter& parameter,
                                 const MapData& data)
  {
    for (std::list<WayData>::const_iterator way=wayData.begin();
        way!=wayData.end();
        way++)
    {
      if (way->nameLabelStyle!=NULL) {
        if (way->nameLabelStyle->IsContourStyle()) {
          DrawContourLabel(projection,
                           parameter,
                           *way->nameLabelStyle,
                           way->attributes->GetName(),
                           way->transStart, way->transEnd);

        }
        else {
          RegisterPointWayLabel(projection,
                                parameter,
                                *way->nameLabelStyle,
                                way->attributes->GetName(),
                                way->transStart,way->transEnd);
        }

        waysLabelDrawn++;
      }

      if (way->refLabelStyle!=NULL) {
        if (way->refLabelStyle->IsContourStyle()) {
          DrawContourLabel(projection,
                           parameter,
                           *way->refLabelStyle,
                           way->attributes->GetRefName(),
                           way->transStart, way->transEnd);

        }
        else {
          RegisterPointWayLabel(projection,
                                parameter,
                                *way->refLabelStyle,
                                way->attributes->GetRefName(),
                                way->transStart,way->transEnd);
        }

        waysLabelDrawn++;
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

    for (size_t i=0; i<labels.size(); i++) {
      if (!labels[i].draw ||
          labels[i].overlay) {
        continue;
      }

      if (labels[i].style->GetStyle()==LabelStyle::normal ||
          labels[i].style->GetStyle()==LabelStyle::emphasize) {
        DrawLabel(projection,
                  parameter,
                  labels[i]);
        labelsDrawn++;
      }
      else if (labels[i].style->GetStyle()==LabelStyle::plate) {
        DrawPlateLabel(projection,
                       parameter,
                       labels[i]);
        labelsDrawn++;
      }
    }

    //
    // Draw overlays
    //

    for (size_t i=0; i<labels.size(); i++) {
      if (!labels[i].draw ||
          !labels[i].overlay) {
        continue;
      }

      if (labels[i].style->GetStyle()==LabelStyle::normal ||
          labels[i].style->GetStyle()==LabelStyle::emphasize) {
        DrawLabel(projection,
                  parameter,
                  labels[i]);
        labelsDrawn++;
      }
      else if (labels[i].style->GetStyle()==LabelStyle::plate) {
        DrawPlateLabel(projection,
                       parameter,
                       labels[i]);
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
    const FillStyle *fillStyle=styleConfig.GetAreaFillStyle(attributes.GetType());

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
                              start,end);

    AreaData data;

    data.ref=ref;
    data.attributes=&attributes;
    data.fillStyle=fillStyle;
    data.transStart=start;
    data.transEnd=end;

    data.minLon=nodes[0].GetLon();

    for (size_t i=1; i<nodes.size(); i++) {
      data.minLon=std::min(data.minLon,nodes[i].GetLon());
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
                                  data[i].transStart,data[i].transEnd);
      }

      size_t ring=0;
      bool foundRing=true;

      while (foundRing) {
        std::list<size_t>   roles;
        std::list<PolyData> clippings;

        for (size_t i=0; i<relation->roles.size(); i++) {
          const Relation::Role& role=relation->roles[i];

          if (role.ring==ring && role.GetType()!=typeIgnore)
          {
            roles.push_back(i);
          }
          else if (role.ring==ring+1 && role.GetType()==typeIgnore) {
            clippings.push_back(data[i]);
          }
        }

        for (std::list<size_t>::const_iterator r=roles.begin();
            r!=roles.end();
            r++) {
          const Relation::Role& role=relation->roles[*r];

          const FillStyle *fillStyle=styleConfig.GetAreaFillStyle(role.attributes.GetType());

          if (fillStyle==NULL)
          {
            continue;
          }

          if (!IsVisible(projection, role.nodes, fillStyle->GetBorderWidth()/2)) {
            continue;
          }

          AreaData a;

          a.ref=ObjectRef(relation->GetId(),refRelation);
          a.attributes=&role.attributes;
          a.fillStyle=fillStyle;
          a.transStart=data[*r].transStart;
          a.transEnd=data[*r].transEnd;

          a.minLon=role.nodes[0].GetLon();

          for (size_t i=1; i<role.nodes.size(); i++) {
            a.minLon=std::min(a.minLon,role.nodes[i].GetLon());
          }

          a.clippings=clippings;

          areaData.push_back(a);

          areasSegments++;
        }


        foundRing=!roles.empty();

        ring++;
      }
    }

    areaData.sort(AreaSortByLon);
  }

  void MapPainter::PrepareWaySegment(const StyleConfig& styleConfig,
                                     const Projection& projection,
                                     const MapParameter& parameter,
                                     const ObjectRef& ref,
                                     const SegmentAttributes& attributes,
                                     const std::vector<Point>& nodes)
  {
    const LineStyle *lineStyle=styleConfig.GetWayLineStyle(attributes.GetType());

    if (lineStyle==NULL) {
      return;
    }

    double lineWidth;

    if (lineStyle->GetWidth()==0) {
      lineWidth=ConvertWidthToPixel(parameter,lineStyle->GetMinWidth());
    }
    else if (parameter.GetDrawWaysWithFixedWidth() ||
        attributes.GetWidth()==0) {
      lineWidth=GetProjectedWidth(projection,
                                  ConvertWidthToPixel(parameter,lineStyle->GetMinWidth()),
                                  lineStyle->GetWidth());
    }
    else {
      lineWidth=GetProjectedWidth(projection,
                                  ConvertWidthToPixel(parameter,lineStyle->GetMinWidth()),
                                  attributes.GetWidth());
    }

    WayData data;

    data.ref=ref;
    data.lineWidth=lineWidth;

    if (lineStyle->GetOutline()>0.0) {
      double convertedOutlineWidth=ConvertWidthToPixel(parameter,2*lineStyle->GetOutline());

      if (lineStyle->GetOutlineA()==1.0) {
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
                             parameter.GetOptimizeAreaNodes(),
                             nodes,
                             start,end);

    data.attributes=&attributes;
    data.lineStyle=lineStyle;

    data.nameLabelStyle=NULL;
    data.refLabelStyle=NULL;

    if (!attributes.GetName().empty()) {
      const LabelStyle *style=styleConfig.GetWayNameLabelStyle(attributes.GetType());

      if (style!=NULL) {
        if (style->IsContourStyle()) {
          if (projection.GetMagnification()>=style->GetMinMag() &&
              projection.GetMagnification()<=style->GetMaxMag() &&
              IsVisible(projection,
                       nodes,
                       style->GetSize())) {
            data.nameLabelStyle=style;
          }
        }
        else if (projection.GetMagnification()>=style->GetMinMag() &&
                 projection.GetMagnification()<=style->GetMaxMag()) {
          data.nameLabelStyle=style;
        }
      }
    }

    if (!attributes.GetRefName().empty()) {
      const LabelStyle *style=styleConfig.GetWayRefLabelStyle(attributes.GetType());

      if (style!=NULL) {
        if (style->IsContourStyle()) {
          if (projection.GetMagnification()>=style->GetMinMag() &&
              projection.GetMagnification()<=style->GetMaxMag() &&
              IsVisible(projection,
                       nodes,
                       style->GetSize())) {
            data.refLabelStyle=style;
          }
        }
        else if (projection.GetMagnification()>=style->GetMinMag() &&
                 projection.GetMagnification()<=style->GetMaxMag()) {
          data.refLabelStyle=style;
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
        lineStyle->HasDashValues()) {
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

    transBuffer.Reset();

    if (parameter.IsAborted()) {
      return false;
    }

    if (parameter.IsDebugPerformance()) {
      std::cout << "Draw [";
      std::cout << projection.GetLatMin() <<",";
      std::cout << projection.GetLonMin() << " - ";
      std::cout << projection.GetLatMax() << ",";
      std::cout << projection.GetLonMax() << "]  with mag. ";
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

      std::cout << "Labels: " << labels.size() << "/" << labelsDrawn << " (pcs) ";
      std::cout << labelsTimer << " (sec)" << std::endl;
    }

    return true;
  }
}
