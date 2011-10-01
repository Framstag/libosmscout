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

#include <osmscout/util/StopClock.h>

#include <osmscout/private/Math.h>

namespace osmscout {

  MapParameter::MapParameter()
  : dpi(96.0),
    fontName("sans-serif"),
    fontSize(9.0),
    outlineMinWidth(1.0),
    optimizeWayNodes(false),
    optimizeAreaNodes(false),
    drawFadings(true),
    debugPerformance(false)
  {
    // no code
  }

  MapParameter::~MapParameter()
  {
    // no code
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

  void MapParameter::SetOutlineMinWidth(double outlineMinWidth)
  {
    this->outlineMinWidth=outlineMinWidth;
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
    double lonMin=nodes[0].lon;
    double lonMax=nodes[0].lon;
    double latMin=nodes[0].lat;
    double latMax=nodes[0].lat;

    for (size_t i=1; i<nodes.size(); i++) {
      lonMin=std::min(lonMin,nodes[i].lon);
      lonMax=std::max(lonMax,nodes[i].lon);
      latMin=std::min(latMin,nodes[i].lat);
      latMax=std::max(latMax,nodes[i].lat);
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

    xmin=nodes[0].lon;
    xmax=nodes[0].lon;
    ymin=nodes[0].lat;
    ymax=nodes[0].lat;

    for (size_t j=1; j<nodes.size(); j++) {
      xmin=std::min(xmin,nodes[j].lon);
      xmax=std::max(xmax,nodes[j].lon);
      ymin=std::min(ymin,nodes[j].lat);
      ymax=std::max(ymax,nodes[j].lat);
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

    wayScanlines.clear();
    ScanConvertLine(transStart,transEnd,1,1,wayScanlines);

    for (size_t i=0; i<wayScanlines.size(); i++) {
      if (RegisterPointLabel(projection,
                             parameter,
                             style,
                             text,
                             wayScanlines[i].x+0.5,
                             wayScanlines[i].y+0.5)) {
        i+=parameter.GetFontSize()*6;
      }
      else {
        i+=2;
      }
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
    double plateSpace=parameter.GetFontSize()*6;
    double normalSpace=parameter.GetFontSize();

    // Calculate effective font size and alpha value
    if (projection.GetMagnification()>style.GetScaleAndFadeMag()) {
      if (parameter.GetDrawFadings()) {
        double factor=log2(projection.GetMagnification())-log2(style.GetScaleAndFadeMag());
        fontSize=fontSize*pow(2,factor);
        a=a/factor;
      }
    }

    if (a>=0.8) {
      for (size_t i=0; i<labels.size(); i++) {
        labels[i].mark=false;
      }
    }

    if (a>=0.8) {
      for (std::vector<Label>::iterator label=labels.begin();
           label!=labels.end();
           ++label) {
        if (label->overlay || !label->draw || label->mark) {
          continue;
        }

        if (style.GetStyle()==LabelStyle::plate &&
            label->style->GetStyle()==LabelStyle::plate) {
          if (!(x-plateSpace>label->bx+label->bwidth ||
                x+plateSpace<label->bx ||
                y-plateSpace>label->by+label->bheight ||
                y+plateSpace<label->by)) {
            if (label->style->GetPriority()<=style.GetPriority()) {
              return false;
            }

            label->mark=true;
          }
        }
        else {
          if (!(x-normalSpace>label->bx+label->bwidth ||
                x+normalSpace<label->bx ||
                y-normalSpace>label->by+label->bheight ||
                y+normalSpace<label->by)) {
            if (label->style->GetPriority()<=style.GetPriority()) {
              return false;
            }

            label->mark=true;
          }
        }
      }

      /*
      int xcMin,xcMax,ycMin,ycMax;

      if (style.GetStyle()==LabelStyle::plate) {
        xcMin=std::max(0,(int)floor((x-plateSpace)/cellWidth));
        xcMax=std::min(xCellCount-1,(int)floor((x+plateSpace)/cellWidth));
        ycMin=std::max(0,(int)floor((y-plateSpace)/cellHeight));
        ycMax=std::min(yCellCount-1,(int)floor((y+plateSpace)/cellHeight));
      }
      else {
        xcMin=std::max(0,(int)floor((x-normalSpace)/cellWidth));
        xcMax=std::min(xCellCount-1,(int)floor((x+normalSpace)/cellWidth));
        ycMin=std::max(0,(int)floor((y-normalSpace)/cellHeight));
        ycMax=std::min(yCellCount-1,(int)floor((y+normalSpace)/cellHeight));
      }


      // Check if we are visible
      for (int yc=ycMin; yc<=ycMax; yc++) {
        for (int xc=xcMin; xc<=xcMax; xc++) {
          int c=yc*xCellCount+xc;

          if (labelRefs[c].size()>0) {
            for (std::list<size_t>::const_iterator idx=labelRefs[c].begin();
                 idx!=labelRefs[c].end();
                 idx++) {
              if (labels[*idx].overlay || !labels[*idx].draw || labels[*idx].mark) {
                continue;
              }

              if (style.GetStyle()==LabelStyle::plate &&
                  labels[*idx].style->GetStyle()==LabelStyle::plate) {
                if (!(x-plateSpace>labels[*idx].bx+labels[*idx].bwidth ||
                      x+plateSpace<labels[*idx].bx ||
                      y-plateSpace>labels[*idx].by+labels[*idx].bheight ||
                      y+plateSpace<labels[*idx].by)) {
                  if (labels[*idx].style->GetPriority()<=style.GetPriority()) {
                    return;
                  }

                  labels[*idx].mark=true;
                }
              }
              else {
                if (!(x-normalSpace>labels[*idx].bx+labels[*idx].bwidth ||
                      x+normalSpace<labels[*idx].bx ||
                      y-normalSpace>labels[*idx].by+labels[*idx].bheight ||
                      y+normalSpace<labels[*idx].by)) {
                  if (labels[*idx].style->GetPriority()<=style.GetPriority()) {
                    return;
                  }

                  labels[*idx].mark=true;
                }
              }
            }
          }
        }
      }*/
    }

    // Bounding box

    double xOff,yOff,width,height;

    GetTextDimension(parameter,
                     fontSize,
                     text,
                     xOff,yOff,width,height);

    // Top left border
    x=x-width/2;
    y=y-height/2;

    double bx,by,bwidth,bheight;

    if (style.GetStyle()!=LabelStyle::plate) {
      bx=x;
      by=y;
      bwidth=width;
      bheight=height;
    }
    else {
      bx=x-4;
      by=y-4;
      bwidth=width+8;
      bheight=height+8;
    }

    // is visible?
    if (bx>=projection.GetWidth() ||
        bx+bwidth<0 ||
        by>=projection.GetHeight() ||
        by+bheight<0) {
      return false;
    }

    if (a>=0.8) {
      int xcMin,xcMax,ycMin,ycMax;

      if (style.GetStyle()==LabelStyle::plate) {
        xcMin=std::max(0,(int)floor((bx-plateSpace)/cellWidth));
        xcMax=std::min(xCellCount-1,(int)floor((bx+bwidth+plateSpace)/cellWidth));
        ycMin=std::max(0,(int)floor((by-plateSpace)/cellHeight));
        ycMax=std::min(yCellCount-1,(int)floor((by+bheight+plateSpace)/cellHeight));
      }
      else {
        xcMin=std::max(0,(int)floor((bx-normalSpace)/cellWidth));
        xcMax=std::min(xCellCount-1,(int)floor((bx+bwidth+normalSpace)/cellWidth));
        ycMin=std::max(0,(int)floor((by-normalSpace)/cellHeight));
        ycMax=std::min(yCellCount-1,(int)floor((by+bheight+normalSpace)/cellHeight));
      }

      // Check if we are visible
      for (int yc=ycMin; yc<=ycMax; yc++) {
        for (int xc=xcMin; xc<=xcMax; xc++) {
          int c=yc*xCellCount+xc;

          if (!labelRefs[c].empty()) {
            for (std::list<size_t>::const_iterator idx=labelRefs[c].begin();
                 idx!=labelRefs[c].end();
                 idx++) {
              if (labels[*idx].overlay || !labels[*idx].draw || labels[*idx].mark) {
                continue;
              }

              if (style.GetStyle()==LabelStyle::plate &&
                  labels[*idx].style->GetStyle()==LabelStyle::plate) {
                if (!(bx-plateSpace>labels[*idx].bx+labels[*idx].bwidth ||
                      bx+bwidth+plateSpace<labels[*idx].bx ||
                      by-plateSpace>labels[*idx].by+labels[*idx].bheight ||
                      by+bheight+plateSpace<labels[*idx].by)) {
                  if (labels[*idx].style->GetPriority()<=style.GetPriority()) {
                    return false;
                  }

                  labels[*idx].mark=true;
                }
              }
              else {
                if (!(bx-normalSpace>labels[*idx].bx+labels[*idx].bwidth ||
                      bx+bwidth+normalSpace<labels[*idx].bx ||
                      by-normalSpace>labels[*idx].by+labels[*idx].bheight ||
                      by+bheight+normalSpace<labels[*idx].by)) {
                  if (labels[*idx].style->GetPriority()<=style.GetPriority()) {
                    return false;
                  }

                  labels[*idx].mark=true;
                }
              }
            }
          }
        }
      }

      // Set everything else to draw=false
      for (int yc=ycMin; yc<=ycMax; yc++) {
        for (int xc=xcMin; xc<=xcMax; xc++) {
          int c=yc*xCellCount+xc;

          if (!labelRefs[c].empty()) {
            for (std::list<size_t>::const_iterator idx=labelRefs[c].begin();
                 idx!=labelRefs[c].end();
                 idx++) {
              if (labels[*idx].overlay) {
                continue;
              }

              if (labels[*idx].mark) {
                labels[*idx].draw=false;
              }
            }
          }

          labelRefs[c].push_front(labels.size());
        }
      }
    }

    Label label;

    label.draw=true;
    label.overlay=(a<0.8);
    label.x=x;
    label.y=y;
    label.width=width;
    label.height=height;
    label.bx=bx;
    label.by=by;
    label.bwidth=bwidth;
    label.bheight=bheight;
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
        continue;
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
        if (relation->roles[m].role=="0") {
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
  }

  void MapPainter::DrawWayOutline(const StyleConfig& styleConfig,
                                  const Projection& projection,
                                  const MapParameter& parameter,
                                  const WayData& data)
  {
    if (data.drawBridge) {
      // black outline for bridges
      DrawPath(projection,
               parameter,
               0.0,
               0.0,
               0.0,
               1.0,
               data.lineWidth+1,
               emptyDash,
               capButt,
               capButt,
               data.transStart,data.transEnd);

      if (data.outline) {
        DrawPath(projection,
                 parameter,
                 data.lineStyle->GetOutlineR(),
                 data.lineStyle->GetOutlineG(),
                 data.lineStyle->GetOutlineB(),
                 1.0,
                 data.lineWidth,
                 emptyDash,
                 data.attributes->StartIsJoint() ? capButt : capRound,
                 data.attributes->EndIsJoint() ? capButt : capRound,
                 data.transStart,data.transEnd);
      }
      else if (data.lineStyle->HasDashValues() ||
          data.lineStyle->GetLineA()<1.0 ||
          data.lineStyle->GetAlternateA()<1.0) {
        DrawPath(projection,
                 parameter,
                 1.0,
                 1.0,
                 1.0,
                 1.0,
                 data.lineWidth,
                 emptyDash,
                 data.attributes->StartIsJoint() ? capButt : capRound,
                 data.attributes->EndIsJoint() ? capButt : capRound,
                 data.transStart,data.transEnd);
      }
    }
    else if (data.drawTunnel) {
      tunnelDash[0]=4.0/data.lineWidth;
      tunnelDash[1]=2.0/data.lineWidth;

      if (data.outline) {
        DrawPath(projection,
                 parameter,
                 data.lineStyle->GetOutlineR(),
                 data.lineStyle->GetOutlineG(),
                 data.lineStyle->GetOutlineB(),
                 data.lineStyle->GetOutlineA(),
                 data.lineWidth,
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
                 data.lineWidth,
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
                 data.lineWidth,
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
               data.lineWidth,
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
    double lineWidth=data.lineWidth;

    if (data.outline) {
      lineWidth-=2*data.lineStyle->GetOutline();
    }

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

    if (!data.lineStyle->GetDash().empty() && data.lineStyle->GetGapA()>0.0) {
      DrawPath(projection,
               parameter,
               data.lineStyle->GetGapR(),data.lineStyle->GetGapG(),data.lineStyle->GetGapB(),data.lineStyle->GetGapA(),
               lineWidth,
               emptyDash,
               capRound,
               capRound,
               data.transStart,data.transEnd);
    }

    DrawPath(projection,
             parameter,
             r,g,b,a,
             lineWidth,
             data.lineStyle->GetDash(),
             capRound,
             capRound,
             data.transStart,data.transEnd);

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

      if (!projection.GeoIsIn(node->GetLon(),node->GetLat())) {
        continue;
      }

      double x,y;

      projection.GeoToPixel(node->GetLon(),node->GetLat(),
                            x,y);

      const SymbolStyle *symbolStyle=styleConfig.GetNodeSymbolStyle(node->GetType());

      if (symbolStyle!=NULL &&
          projection.GetMagnification()>=symbolStyle->GetMinMag()) {
        DrawSymbol(symbolStyle,x,y);
      }

      for (size_t i=0; i<node->GetTagCount(); i++) {
        // TODO: We should make sure we prefer one over the other
        if (node->GetTagKey(i)==styleConfig.GetTypeConfig()->tagName) {
          const LabelStyle *style=styleConfig.GetNodeLabelStyle(node->GetType());

          if (style==NULL ||
              projection.GetMagnification()<style->GetMinMag() ||
              projection.GetMagnification()>style->GetMaxMag()) {
            continue;
          }

          double x,y;

          projection.GeoToPixel(node->GetLon(),node->GetLat(),
                                x,y);

          RegisterPointLabel(projection,
                             parameter,
                             *style,
                             node->GetTagValue(i),
                             x,y);
        }
        else if (node->GetTagKey(i)==styleConfig.GetTypeConfig()->tagRef)  {
          const LabelStyle *style=styleConfig.GetNodeRefLabelStyle(node->GetType());

          if (style==NULL ||
              projection.GetMagnification()<style->GetMinMag() ||
              projection.GetMagnification()>style->GetMaxMag()) {
            continue;
          }

          double x,y;

          projection.GeoToPixel(node->GetLon(),node->GetLat(),
                                x,y);

          RegisterPointLabel(projection,
                             parameter,
                             *style,
                             node->GetTagValue(i),
                             x,y);
        }
      }

      //nodesDrawnCount++;
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
      }
      else if (labels[i].style->GetStyle()==LabelStyle::plate) {
        DrawPlateLabel(projection,
                       parameter,
                       labels[i]);
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
      }
      else if (labels[i].style->GetStyle()==LabelStyle::plate) {
        DrawPlateLabel(projection,
                       parameter,
                       labels[i]);
      }
    }
  }

  void MapPainter::PrepareAreaSegment(const StyleConfig& styleConfig,
                                      const Projection& projection,
                                      const MapParameter& parameter,
                                      const SegmentAttributes& attributes,
                                      const std::vector<Point>& nodes)
  {
    const FillStyle *fillStyle=styleConfig.GetAreaFillStyle(attributes.GetType());

    if (fillStyle==NULL)
    {
      return;
    }

    if (!IsVisible(projection, nodes, fillStyle->GetBorderWidth()/2)) {
      return;
    }

    size_t start,end;

    if (!transBuffer.TransformArea(projection,
                                   parameter.GetOptimizeAreaNodes(),
                                   nodes,
                                   start,end)) {
      return;
    }

    AreaData data;

    data.attributes=&attributes;
    data.fillStyle=fillStyle;
    data.transStart=start;
    data.transEnd=end;

    areaData.push_back(data);

    areasSegments++;
  }

  void MapPainter::PrepareAreas(const StyleConfig& styleConfig,
                                const Projection& projection,
                                const MapParameter& parameter,
                                const MapData& data)
  {
    areaData.clear();

    for (std::vector<WayRef>::const_iterator a=data.areas.begin();
         a!=data.areas.end();
         ++a) {
      const WayRef& area=*a;

      PrepareAreaSegment(styleConfig,
                         projection,
                         parameter,
                         area->GetAttributes(),
                         area->nodes);
    }

    for (std::vector<RelationRef>::const_iterator r=data.relationAreas.begin();
         r!=data.relationAreas.end();
         ++r) {
      const RelationRef& relation=*r;

      for (std::vector<Relation::Role>::const_iterator r=relation->roles.begin();
          r!=relation->roles.end();
          r++) {
        if (r->role=="0") {
          const Relation::Role& role=*r;

          PrepareAreaSegment(styleConfig,
                             projection,
                             parameter,
                             role.GetAttributes(),
                             role.nodes);
        }
      }
    }

    for (std::list<WayRef>::const_iterator p=data.poiWays.begin();
         p!=data.poiWays.end();
         ++p) {
      if ((*p)->IsArea()) {
        const WayRef& area=*p;

        PrepareAreaSegment(styleConfig,
                           projection,
                           parameter,
                           area->GetAttributes(),
                           area->nodes);
      }
    }

    areaData.sort();
  }

  void MapPainter::PrepareWaySegment(const StyleConfig& styleConfig,
                                     const Projection& projection,
                                     const MapParameter& parameter,
                                     const SegmentAttributes& attributes,
                                     const std::vector<Point>& nodes)
  {
    const LineStyle *lineStyle=styleConfig.GetWayLineStyle(attributes.GetType());

    if (lineStyle==NULL) {
      return;
    }

    double lineWidth;

    if (lineStyle->GetFixedWidth()) {
      lineWidth=GetProjectedWidth(projection,
                                  lineStyle->GetMinPixel(),
                                  lineStyle->GetWidth());
    }
    else {
      lineWidth=GetProjectedWidth(projection,
                                  lineStyle->GetMinPixel(),
                                  attributes.GetWidth()>0 ? attributes.GetWidth() : lineStyle->GetWidth());
    }

    if (!IsVisible(projection,
                  nodes,
                  lineWidth/2)) {
      return;
    }

    size_t start,end;

    if (!transBuffer.TransformArea(projection,
                                   parameter.GetOptimizeAreaNodes(),
                                   nodes,
                                   start,end)) {
      return;
    }

    WayData data;

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
    data.lineWidth=lineWidth;
    data.drawBridge=attributes.IsBridge();
    data.drawTunnel=attributes.IsTunnel();
    data.outline=lineStyle->GetOutline()>0 &&
                 lineWidth-2*lineStyle->GetOutline()>=parameter.GetOutlineMinWidth();

    if (data.drawBridge &&
        projection.GetMagnification()<magCity) {
      data.drawBridge=false;
    }

    if (data.drawTunnel &&
        projection.GetMagnification()<magCity) {
      data.drawTunnel=false;
    }

    // Drawing tunnel style for dashed lines is currently not supported
    if (data.drawTunnel &&
        lineStyle->HasDashValues()) {
      data.drawTunnel=false;
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
                          way->GetAttributes(),
                          way->nodes);
      }
    }

    wayData.sort();
  }

  void MapPainter::Draw(const StyleConfig& styleConfig,
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

    cellWidth=parameter.GetFontSize()*4;
    cellHeight=parameter.GetFontSize()*4;
    xCellCount=(int)ceil((double)projection.GetWidth()/cellWidth);
    yCellCount=(int)ceil((double)projection.GetHeight()/cellHeight);

    labels.clear();
    labelRefs.clear();
    labelRefs.resize(xCellCount*yCellCount);

    transBuffer.Reset();

    if (parameter.IsDebugPerformance()) {
      std::cout << "Draw ";
      std::cout << projection.GetLat() <<", ";
      std::cout << projection.GetLon() << " with mag. ";
      std::cout << projection.GetMagnification() << "x" << "/" << log(projection.GetMagnification())/log(2.0);
      std::cout << " area " << projection.GetWidth() << "x" << projection.GetHeight() << std::endl;
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

    StopClock prepareWaysTimer;

    PrepareWays(styleConfig,
                projection,
                parameter,
                data);

    prepareWaysTimer.Stop();

    //
    // Clear area with background color
    //

    DrawGroundTiles(styleConfig,
                    projection,
                    parameter,
                    data);

    //
    // Draw areas
    //

    StopClock areasTimer;

    DrawAreas(styleConfig,
              projection,
              parameter,
              data);

    areasTimer.Stop();

    //
    // Drawing ways
    //

    StopClock pathsTimer;

    DrawWays(styleConfig,
             projection,
             parameter,
             data);

    pathsTimer.Stop();

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

    //
    // Nodes symbols & Node labels
    //

    StopClock nodesTimer;

    DrawNodes(styleConfig,
              projection,
              parameter,
              data);

    nodesTimer.Stop();

    //
    // Area labels
    //

    StopClock areaLabelsTimer;

    DrawAreaLabels(styleConfig,
                   projection,
                   parameter,
                   data);

    areaLabelsTimer.Stop();

    //
    // POI Nodes
    //

    StopClock poisTimer;

    DrawPOINodes(styleConfig,
                 projection,
                 parameter,
                 data);

    poisTimer.Stop();

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

      std::cout << "Nodes: " << nodesTimer << " ";
      std::cout << "POIs: " << poisTimer << " ";
      std::cout << "Labels: " << labelsTimer << std::endl;
    }
  }
}
