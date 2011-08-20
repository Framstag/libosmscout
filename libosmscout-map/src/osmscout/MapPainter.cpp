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

#include <cmath>
#include <iostream>
#include <limits>

#include <osmscout/util/StopClock.h>

namespace osmscout {

  MapParameter::MapParameter()
  : fontName("sans-serif"),
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

  void MapPainter::ScanConvertLine(const TransPolygon& polygon,
                                   double cellWidth,
                                   double cellHeight,
                                   std::vector<ScanCell>& cells)
  {
    if (polygon.GetLength()<2) {
      return;
    }

    for (size_t i=polygon.GetStart(); i<=polygon.GetEnd(); i++) {
      if (polygon.points[i].draw)
      {
        size_t j=i+1;

        while (j<=polygon.GetEnd() && !polygon.points[j].draw)
        {
          j++;
        }

        if (j<=polygon.GetEnd()) {
          int x1=int(polygon.points[i].x/cellWidth);
          int x2=int(polygon.points[j].x/cellWidth);
          int y1=int(polygon.points[i].y/cellHeight);
          int y2=int(polygon.points[j].y/cellHeight);

          osmscout::ScanConvertLine(x1,y1,x2,y2,cells);
        }
      }
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

    xMin+=pixelOffset;
    yMin+=pixelOffset;

    xMax-=pixelOffset;
    yMax-=pixelOffset;

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

    projection.GeoToPixel(xmin,ymin,xmin,ymin);
    projection.GeoToPixel(xmax,ymax,xmax,ymax);

    cx=xmin+(xmax-xmin)/2;
    cy=ymin+(ymax-ymin)/2;

    return true;
  }

  double MapPainter::GetProjectedWidth(const Projection& projection,
                                       double minPixel,
                                       double width) const
  {
    if (width==0.0) {
      return 0.0;
    }

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
                                         const TransPolygon& polygon)
  {
    if (style.GetStyle()!=LabelStyle::plate) {
      return;
    }

    wayScanlines.clear();
    ScanConvertLine(polygon,1,1,wayScanlines);

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
          int c=yc*yCellCount+xc;

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
          int c=yc*yCellCount+xc;

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
          int c=yc*yCellCount+xc;

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

  void MapPainter::PrecalculateStyleData(const StyleConfig& styleConfig,
                                         const Projection& projection,
                                         const MapParameter& parameter,
                                         const MapData& data)
  {
    //
    // Calculate available layers for ways and way relations
    //

    for (size_t i=0; i<11; i++) {
      wayLayers[i]=false;
    }

    for (std::vector<WayRef>::const_iterator w=data.ways.begin();
         w!=data.ways.end();
         ++w) {
      const WayRef& way=*w;

      if (way->GetLayer()>=-5 && way->GetLayer()<=5) {
        wayLayers[way->GetLayer()+5]=true;
      }
    }

    for (size_t i=0; i<11; i++) {
      relationWayLayers[i]=false;
    }

    for (std::vector<RelationRef>::const_iterator r=data.relationWays.begin();
         r!=data.relationWays.end();
         ++r) {
      const RelationRef& relation=*r;

      for (size_t m=0; m<relation->roles.size(); m++) {
        if (relation->roles[m].GetLayer()>=-5 && relation->roles[m].GetLayer()<=5) {
          relationWayLayers[relation->roles[m].GetLayer()+5]=true;
        }
      }
    }

    //
    // Calculate available layers for areas and area relations
    //

    for (size_t i=0; i<11; i++) {
      areaLayers[i]=false;
      relationAreaLayers[i]=false;
    }

    for (std::vector<WayRef>::const_iterator a=data.areas.begin();
         a!=data.areas.end();
         ++a) {
      const WayRef& area=*a;

      const FillStyle *style=styleConfig.GetAreaFillStyle(area->GetType());

      if (style!=NULL &&
          style->GetLayer()>=-5 &&
          style->GetLayer()<=5) {
        areaLayers[style->GetLayer()+5]=true;
      }
    }

    for (std::vector<RelationRef>::const_iterator r=data.relationAreas.begin();
         r!=data.relationAreas.end();
         ++r) {
      const RelationRef& relation=*r;

      const FillStyle *style=styleConfig.GetAreaFillStyle(relation->GetType());

      if (style!=NULL &&
          style->GetLayer()>=-5 &&
          style->GetLayer()<=5) {
        relationAreaLayers[style->GetLayer()+5]=true;
      }
    }
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
    for (size_t l=0; l<11; l++) {
      int layer=l-5;

      if (areaLayers[l]) {
        for (std::vector<WayRef>::const_iterator a=data.areas.begin();
             a!=data.areas.end();
             ++a) {
          const WayRef& area=*a;

          PatternStyle    *patternStyle=styleConfig.GetAreaPatternStyle(area->GetType());
          const FillStyle *fillStyle=styleConfig.GetAreaFillStyle(area->GetType());

          bool            hasPattern=patternStyle!=NULL &&
                                     patternStyle->GetLayer()==layer &&
                                     projection.GetMagnification()>=patternStyle->GetMinMag();
          bool            hasFill=fillStyle!=NULL &&
                                  fillStyle->GetLayer()==layer;

          if (hasPattern) {
            hasPattern=HasPattern(styleConfig,
                                  parameter,
                                  *patternStyle);
          }


          if (hasPattern) {
            polygon.TransformArea(projection,
                                  parameter.GetOptimizeAreaNodes(),
                                  area->nodes);

            DrawArea(projection,
                     parameter,
                     area->GetType(),
                     *patternStyle,
                     polygon);

            areasDrawn++;
          }
          else if (hasFill) {
            polygon.TransformArea(projection,
                                  parameter.GetOptimizeAreaNodes(),
                                  area->nodes);

            DrawArea(projection,
                     parameter,
                     area->GetType(),
                     *fillStyle,
                     polygon);

            areasDrawn++;
          }
        }
      }

      if (relationAreaLayers[l]) {
        for (std::vector<RelationRef>::const_iterator r=data.relationAreas.begin();
             r!=data.relationAreas.end();
             ++r) {
          const RelationRef& relation=*r;

          bool drawn=false;

          for (size_t m=0; m<relation->roles.size(); m++) {
            if (relation->roles[m].role=="0") {
              PatternStyle    *patternStyle=styleConfig.GetAreaPatternStyle(relation->roles[m].GetType());
              const FillStyle *fillStyle=styleConfig.GetAreaFillStyle(relation->roles[m].GetType());

              bool            hasPattern=patternStyle!=NULL &&
                                         patternStyle->GetLayer()==layer &&
                                         projection.GetMagnification()>=patternStyle->GetMinMag();
              bool            hasFill=fillStyle!=NULL &&
                                      fillStyle->GetLayer()==layer;

              if (hasPattern) {
                hasPattern=HasPattern(styleConfig,
                                      parameter,
                                      *patternStyle);
              }

              if (hasPattern) {
                polygon.TransformArea(projection,
                                      parameter.GetOptimizeAreaNodes(),
                                      relation->roles[m].nodes);

                DrawArea(projection,
                         parameter,
                         relation->roles[m].GetType(),
                         *patternStyle,
                         polygon);
                drawn=true;
              }
              else if (hasFill) {
                polygon.TransformArea(projection,
                                      parameter.GetOptimizeAreaNodes(),
                                      relation->roles[m].nodes);

                DrawArea(projection,
                         parameter,
                         relation->roles[m].GetType(),
                         *fillStyle,
                         polygon);
                drawn=true;
              }
            }
          }

          if (drawn) {
            relAreasDrawn++;
          }
        }
      }
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

            relAreasLabelDrawn++;
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

  void MapPainter::DrawWay(const Projection& projection,
                           const MapParameter& parameter,
                           const LineStyle& style,
                           const SegmentAttributes& attributes,
                           const TransPolygon& polygon)
  {
    double lineWidth=attributes.GetWidth();
    bool   drawBridge=attributes.IsBridge();
    bool   drawTunnel=attributes.IsTunnel();

    if (style.GetFixedWidth()) {
      lineWidth=style.GetWidth();

      lineWidth=std::max(style.GetMinPixel(),
                         lineWidth/projection.GetPixelSize());
    }
    else {
      lineWidth=attributes.GetWidth();

      if (lineWidth==0) {
        lineWidth=style.GetWidth();
      }

      lineWidth=std::max(style.GetMinPixel(),
                         lineWidth/projection.GetPixelSize());
    }

    bool outline=style.GetOutline()>0 &&
                 lineWidth-2*style.GetOutline()>=parameter.GetOutlineMinWidth();

    if (outline) {
      lineWidth-=2*style.GetOutline();
    }

    if (drawBridge &&
        projection.GetMagnification()<magCity) {
      drawBridge=false;
    }

    if (drawTunnel &&
        projection.GetMagnification()<magCity) {
      drawTunnel=false;
    }

    // Drawing tunnel style for dashed lines is currently not supported
    if (drawTunnel &&
        style.HasDashValues()) {
      drawTunnel=false;
    }

    double r,g,b,a;

    if (outline) {
      // Draw line with normal color
      r=style.GetLineR();
      g=style.GetLineG();
      b=style.GetLineB();
      a=style.GetLineA();
    }
    else {
      // Should draw outline, but resolution is too low
      // Draw line with alternate color
      r=style.GetAlternateR();
      g=style.GetAlternateG();
      b=style.GetAlternateB();
      a=style.GetAlternateA();
    }

    if (drawTunnel) {
      r=r+(1-r)*50/100;
      g=g+(1-g)*50/100;
      b=b+(1-b)*50/100;
    }

    if (!style.GetDash().empty() && style.GetGapA()>0.0) {
      DrawPath(projection,
               parameter,
               style.GetGapR(),style.GetGapG(),style.GetGapB(),style.GetGapA(),
               lineWidth,
               emptyDash,
               capRound,
               capRound,
               polygon);
    }

    DrawPath(projection,
             parameter,
             r,g,b,a,
             lineWidth,
             style.GetDash(),
             capRound,
             capRound,
             polygon);
  }

  bool MapPainter::DrawWayOutline(const Projection& projection,
                                  const MapParameter& parameter,
                                  const LineStyle& style,
                                  const SegmentAttributes& attributes,
                                  const std::vector<Point>& nodes)
  {
    double lineWidth=attributes.GetWidth();
    bool   drawBridge=attributes.IsBridge();
    bool   drawTunnel=attributes.IsTunnel();

    if (lineWidth==0) {
      lineWidth=style.GetWidth();
    }

    if (style.GetFixedWidth()) {
      lineWidth=std::max(style.GetMinPixel(),
                         lineWidth);
    }
    else {
      lineWidth=std::max(style.GetMinPixel(),
                         lineWidth/projection.GetPixelSize());
    }

    bool outline=style.GetOutline()>0 &&
                 lineWidth-2*style.GetOutline()>=parameter.GetOutlineMinWidth();

    if (drawBridge &&
        projection.GetMagnification()<magCity) {
      drawBridge=false;
    }

    if (drawTunnel &&
        projection.GetMagnification()<magCity) {
      drawTunnel=false;
    }

    // Drawing tunnel style for dashed lines is currently not supported
    if (drawTunnel &&
        style.HasDashValues()) {
      drawTunnel=false;
    }

    if (!(drawBridge ||
          drawTunnel ||
          outline)) {
      return false;
    }

    if (drawBridge) {
      polygon.TransformWay(projection,parameter.GetOptimizeWayNodes(),nodes);

      // black outline for bridges
      DrawPath(projection,
               parameter,
               0.0,
               0.0,
               0.0,
               1.0,
               lineWidth+1,
               emptyDash,
               capButt,
               capButt,
               polygon);

      if (outline) {
        DrawPath(projection,
                 parameter,
                 style.GetOutlineR(),
                 style.GetOutlineG(),
                 style.GetOutlineB(),
                 1.0,
                 lineWidth,
                 emptyDash,
                 attributes.StartIsJoint() ? capButt : capRound,
                 attributes.EndIsJoint() ? capButt : capRound,
                 polygon);
      }
      else if (style.HasDashValues() ||
               style.GetLineA()<1.0 ||
               style.GetAlternateA()<1.0) {
        DrawPath(projection,
                 parameter,
                 1.0,
                 1.0,
                 1.0,
                 1.0,
                 lineWidth,
                 emptyDash,
                 attributes.StartIsJoint() ? capButt : capRound,
                 attributes.EndIsJoint() ? capButt : capRound,
                 polygon);
      }
    }
    else if (drawTunnel) {
      tunnelDash[0]=4.0/lineWidth;
      tunnelDash[1]=2.0/lineWidth;

      polygon.TransformWay(projection,parameter.GetOptimizeWayNodes(),nodes);

      if (outline) {
        DrawPath(projection,
                 parameter,
                 style.GetOutlineR(),
                 style.GetOutlineG(),
                 style.GetOutlineB(),
                 style.GetOutlineA(),
                 lineWidth,
                 tunnelDash,
                 attributes.StartIsJoint() ? capButt : capRound,
                 attributes.EndIsJoint() ? capButt : capRound,
                 polygon);
      }
      else if (projection.GetMagnification()>=10000) {
        // light grey dashes

        DrawPath(projection,
                 parameter,
                 0.5,
                 0.5,
                 0.5,
                 1.0,
                 lineWidth,
                 tunnelDash,
                 attributes.StartIsJoint() ? capButt : capRound,
                 attributes.EndIsJoint() ? capButt : capRound,
                 polygon);
      }
      else {
        // dark grey dashes

        DrawPath(projection,
                 parameter,
                 0.5,
                 0.5,
                 0.5,
                 1.0,
                 lineWidth,
                 tunnelDash,
                 attributes.StartIsJoint() ? capButt : capRound,
                 attributes.EndIsJoint() ? capButt : capRound,
                 polygon);
      }
    }
    else {
      // normal path, normal outline color

      polygon.TransformWay(projection,parameter.GetOptimizeWayNodes(),nodes);

      DrawPath(projection,
               parameter,
               style.GetOutlineR(),
               style.GetOutlineG(),
               style.GetOutlineB(),
               style.GetOutlineA(),
               lineWidth,
               emptyDash,
               attributes.StartIsJoint() ? capButt : capRound,
               attributes.EndIsJoint() ? capButt : capRound,
               polygon);
    }

    return true;
  }


  void MapPainter::DrawWays(const StyleConfig& styleConfig,
                            const Projection& projection,
                            const MapParameter& parameter,
                            const MapData& data)
  {
    for (size_t l=0; l<11; l++) {
      int8_t layer=l-5;
      // Potential path outline

      if (wayLayers[l]) {
        for (std::vector<WayRef>::const_iterator w=data.ways.begin();
             w!=data. ways.end();
             ++w) {
          const WayRef& way=*w;

          if (way->GetLayer()!=layer) {
            continue;
          }

          const LineStyle *style=styleConfig.GetWayLineStyle(way->GetType());

          if (style==NULL) {
            continue;
          }

          if (!IsVisible(projection,
                        way->nodes,
                        style->GetWidth())) {
            continue;
          }

          if (DrawWayOutline(projection,
                             parameter,
                             *style,
                             way->GetAttributes(),
                             way->nodes)) {
            waysOutlineDrawn++;
          }
        }
      }

      if (relationWayLayers[l]) {
        for (std::vector<RelationRef>::const_iterator r=data.relationWays.begin();
             r!=data.relationWays.end();
             ++r) {
          const RelationRef& relation=*r;
          bool               drawn=false;

          for (size_t m=0; m<relation->roles.size(); m++) {
            TypeId type=relation->roles[m].GetType()==typeIgnore ? relation->GetType() : relation->roles[m].GetType();

            if (relation->roles[m].GetLayer()!=layer) {
              continue;
            }

            const LineStyle *style=styleConfig.GetWayLineStyle(type);

            if (style==NULL) {
              continue;
            }

            if (!IsVisible(projection,
                          relation->roles[m].nodes,
                          style->GetWidth())) {
              continue;
            }
            if (DrawWayOutline(projection,
                               parameter,
                               *style,
                               relation->roles[m].GetAttributes(),
                               relation->roles[m].nodes)) {
              drawn=true;
            }
          }

          if (drawn) {
            relWaysOutlineDrawn++;
          }
        }
      }

      if (wayLayers[l]) {
        for (std::vector<WayRef>::const_iterator w=data.ways.begin();
             w!=data.ways.end();
             ++w) {
          const WayRef& way=*w;

          if (way->GetLayer()!=layer) {
            continue;
          }

          const LineStyle *lineStyle=styleConfig.GetWayLineStyle(way->GetType());

          if (lineStyle==NULL) {
            continue;
          }

          if (!IsVisible(projection,
                         way->nodes,
                         lineStyle->GetWidth())) {
            continue;
          }

          polygon.TransformWay(projection,parameter.GetOptimizeWayNodes(),way->nodes);

          DrawWay(projection,
                  parameter,
                  *lineStyle,
                  way->GetAttributes(),
                  polygon);

          if (!way->GetName().empty()) {
            const LabelStyle *style=styleConfig.GetWayNameLabelStyle(way->GetType());

            if (style!=NULL &&
                style->IsPointStyle() &&
                projection.GetMagnification()>=style->GetMinMag() &&
                projection.GetMagnification()<=style->GetMaxMag()) {
              RegisterPointWayLabel(projection,
                                    parameter,
                                    *style,
                                    way->GetName(),
                                    polygon);

              waysLabelDrawn++;
            }
          }

          if (!way->GetRefName().empty()) {
            const LabelStyle *style=styleConfig.GetWayRefLabelStyle(way->GetType());

            if (style!=NULL &&
                style->IsPointStyle() &&
                projection.GetMagnification()>=style->GetMinMag() &&
                projection.GetMagnification()<=style->GetMaxMag()) {

              RegisterPointWayLabel(projection,
                                    parameter,
                                    *style,
                                    way->GetRefName(),
                                    polygon);

              waysLabelDrawn++;
            }
          }

          waysDrawn++;
        }
      }

      if (relationWayLayers[l]) {
        for (std::vector<RelationRef>::const_iterator r=data.relationWays.begin();
             r!=data.relationWays.end();
             ++r) {
          const RelationRef& relation=*r;
          bool               drawn=false;

          for (size_t m=0; m<relation->roles.size(); m++) {
            TypeId type=relation->roles[m].GetType()==typeIgnore ? relation->GetType() : relation->roles[m].GetType();

            if (relation->roles[m].GetLayer()!=layer) {
              continue;
            }

            const LineStyle *style=styleConfig.GetWayLineStyle(type);

            if (style==NULL) {
              continue;
            }

            if (!IsVisible(projection,
                          relation->roles[m].nodes,
                          style->GetWidth())) {
              continue;
            }

            polygon.TransformWay(projection,parameter.GetOptimizeWayNodes(),relation->roles[m].nodes);

            DrawWay(projection,
                    parameter,
                    *style,
                    relation->roles[m].GetAttributes(),
                    polygon);

            if (!relation->roles[m].GetName().empty()) {
              const LabelStyle *style=styleConfig.GetWayNameLabelStyle(relation->roles[m].GetType());

              if (style!=NULL &&
                  style->IsPointStyle() &&
                  projection.GetMagnification()>=style->GetMinMag() &&
                  projection.GetMagnification()<=style->GetMaxMag()) {
                RegisterPointWayLabel(projection,
                                      parameter,
                                      *style,
                                      relation->roles[m].GetName(),
                                      polygon);

                relWaysLabelDrawn++;
              }
            }

            if (!relation->roles[m].GetRefName().empty()) {
                const LabelStyle *style=styleConfig.GetWayRefLabelStyle(relation->roles[m].GetType());

              if (style!=NULL &&
                  style->IsPointStyle() &&
                  projection.GetMagnification()>=style->GetMinMag() &&
                  projection.GetMagnification()<=style->GetMaxMag()) {

                RegisterPointWayLabel(projection,
                                      parameter,
                                      *style,
                                      relation->roles[m].GetRefName(),
                                      polygon);

                relWaysLabelDrawn++;
              }
            }

            drawn=true;
          }

          if (drawn) {
            relWaysDrawn++;
          }
        }
      }
    }
  }

  void MapPainter::DrawWayLabels(const StyleConfig& styleConfig,
                                 const Projection& projection,
                                 const MapParameter& parameter,
                                 const MapData& data)
  {
    std::set<size_t> tileBlacklist;

    for (std::vector<WayRef>::const_iterator w=data.ways.begin();
         w!=data.ways.end();
         ++w) {
      const WayRef& way=*w;

      if (!way->GetName().empty()) {
        const LabelStyle *style=styleConfig.GetWayNameLabelStyle(way->GetType());

        if (style!=NULL &&
            style->IsContourStyle() &&
            projection.GetMagnification()>=style->GetMinMag() &&
            projection.GetMagnification()<=style->GetMaxMag()) {

          if (!IsVisible(projection,
                         way->nodes,
                         style->GetSize())) {
            continue;
          }

          polygon.TransformWay(projection,parameter.GetOptimizeWayNodes(),way->nodes);

          DrawContourLabel(projection,
                           parameter,
                           *style,
                           way->GetName(),
                           polygon);

          waysLabelDrawn++;
        }
      }

      if (!way->GetRefName().empty()) {
        const LabelStyle *style=styleConfig.GetWayRefLabelStyle(way->GetType());

        if (style!=NULL &&
            style->IsContourStyle() &&
            projection.GetMagnification()>=style->GetMinMag() &&
            projection.GetMagnification()<=style->GetMaxMag()) {

          if (!IsVisible(projection,
                         way->nodes,
                         style->GetSize())) {
            continue;
          }

          polygon.TransformWay(projection,parameter.GetOptimizeWayNodes(),way->nodes);

          DrawContourLabel(projection,
                           parameter,
                           *style,
                           way->GetRefName(),
                           polygon);

          waysLabelDrawn++;
        }
      }
    }

    for (std::vector<RelationRef>::const_iterator r=data.relationWays.begin();
         r!=data.relationWays.end();
         ++r) {
      const RelationRef& relation=*r;

      for (size_t m=0; m<relation->roles.size(); m++) {
        if (!relation->roles[m].GetName().empty()) {
          const LabelStyle *style=styleConfig.GetWayNameLabelStyle(relation->roles[m].GetType());

          if (style!=NULL &&
              style->IsContourStyle() &&
              projection.GetMagnification()>=style->GetMinMag() &&
              projection.GetMagnification()<=style->GetMaxMag()) {

            if (!IsVisible(projection,
                           relation->roles[m].nodes,
                           style->GetSize())) {
              continue;
            }

            polygon.TransformWay(projection,parameter.GetOptimizeWayNodes(),relation->roles[m].nodes);

            DrawContourLabel(projection,
                             parameter,
                             *style,
                             relation->roles[m].GetName(),
                             polygon);

            relWaysLabelDrawn++;
          }
        }

        if (!relation->roles[m].GetRefName().empty()) {
          const LabelStyle *style=styleConfig.GetWayRefLabelStyle(relation->roles[m].GetType());

          if (style!=NULL &&
              style->IsContourStyle() &&
              projection.GetMagnification()>=style->GetMinMag() &&
              projection.GetMagnification()<=style->GetMaxMag()) {

            if (!IsVisible(projection,
                           relation->roles[m].nodes,
                           style->GetSize())) {
              continue;
            }

            polygon.TransformWay(projection,parameter.GetOptimizeWayNodes(),relation->roles[m].nodes);

            DrawContourLabel(projection,
                             parameter,
                             *style,
                             relation->roles[m].GetRefName(),
                             polygon);

            relWaysLabelDrawn++;
          }
        }
      }
    }
  }

  void MapPainter::DrawPOIWays(const StyleConfig& styleConfig,
                               const Projection& projection,
                               const MapParameter& parameter,
                               const MapData& data)
  {
    for (std::list<WayRef>::const_iterator w=data.poiWays.begin();
         w!=data.poiWays.end();
         ++w) {
      const WayRef& way=*w;

      if (way->IsArea()) {
        std::cerr << "POI way is area, skipping..." << std::endl;
        continue;
      }

      const LineStyle *style=styleConfig.GetWayLineStyle(way->GetType());

      if (style==NULL) {
        continue;
      }

      if (IsVisible(projection,
                    way->nodes,
                    style->GetWidth())) {
        polygon.TransformWay(projection,parameter.GetOptimizeWayNodes(),way->nodes);

        DrawWay(projection,
                parameter,
                *style,
                way->GetAttributes(),
                polygon);
      }

      //waysDrawnCount++;

      //nodesAllCount+=way->nodes.size();
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

  void MapPainter::Draw(const StyleConfig& styleConfig,
                        const Projection& projection,
                        const MapParameter& parameter,
                        const MapData& data)
  {
    waysCount=data.ways.size();
    waysOutlineDrawn=0;
    waysDrawn=0;
    waysLabelDrawn=0;

    relWaysCount=data.relationWays.size();
    relWaysOutlineDrawn=0;
    relWaysDrawn=0;
    relWaysLabelDrawn=0;

    areasCount=data.areas.size();
    areasDrawn=0;
    areasLabelDrawn=0;

    relAreasCount=data.relationAreas.size();
    relAreasDrawn=0;
    relAreasLabelDrawn=0;

    cellWidth=parameter.GetFontSize()*4;
    cellHeight=parameter.GetFontSize()*4;
    xCellCount=(int)ceil(projection.GetWidth()/cellWidth);
    yCellCount=(int)ceil(projection.GetHeight()/cellHeight);

    labels.clear();
    labelRefs.clear();
    labelRefs.resize(xCellCount*yCellCount);

    if (parameter.IsDebugPerformance()) {
      std::cout << "Draw ";
      std::cout << projection.GetLat() <<", ";
      std::cout << projection.GetLon() << " with mag. ";
      std::cout << projection.GetMagnification() << "x" << "/" << log(projection.GetMagnification())/log(2);
      std::cout << " area " << projection.GetWidth() << "x" << projection.GetHeight() << std::endl;
    }

    //
    // Setup and Precalculation
    //

    PrecalculateStyleData(styleConfig,
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
    // POI ways (aka routes)
    //


    StopClock routesTimer;

    DrawPOIWays(styleConfig,
                projection,
                parameter,
                data);

    routesTimer.Stop();

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
      std::cout << "Paths: " << pathsTimer << "/" << pathLabelsTimer << " ";
      std::cout << "Areas: " << areasTimer << "/" << areaLabelsTimer << " ";
      std::cout << "Nodes: " << nodesTimer << " ";
      std::cout << "POIs: " << poisTimer << "/" << routesTimer << " ";
      std::cout << "Labels: " << labelsTimer << std::endl;

      std::cout << "Path ways: " << waysCount << "/" << waysDrawn << "/" << waysOutlineDrawn << "/" << waysLabelDrawn << " (pcs)" << std::endl;
      std::cout << "Path rels: " << relWaysCount << "/" << relWaysDrawn << "/" << relWaysOutlineDrawn << "/" << relWaysLabelDrawn << " (pcs)" << std::endl;
      std::cout << "Area ways: " << areasCount << "/" << areasDrawn << "/" << areasLabelDrawn << " (pcs)" << std::endl;
      std::cout << "Area rels: " << relAreasCount << "/" << relAreasDrawn << "/" << relAreasLabelDrawn << " (pcs)" << std::endl;
    }
  }
}

