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

#include <osmscout/util/StopClock.h>

namespace osmscout {

  static const double gradtorad=2*M_PI/360;
  static double relevantPosDeriviation=2.0;
  static double relevantSlopeDeriviation=0.1;

  MapParameter::MapParameter()
  : fontName("sans-serif"),
    fontSize(9.0),
    outlineMinWidth(1.0),
    optimizeWayNodes(false),
    optimizeAreaNodes(false)
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

  MapPainter::MapPainter()
  {
    tunnelDash.push_back(3);
    tunnelDash.push_back(3);
  }

  MapPainter::~MapPainter()
  {
    // no code
  }

  bool MapPainter::IsVisible(const Projection& projection,
                             const std::vector<Point>& nodes,
                             double pixelOffset) const
  {
    if (nodes.size()==0) {
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
                             TransPoint& point)
  {
    point.draw=true;
    projection.GeoToPixel(lon,lat,
                          point.x,point.y);
  }

  void MapPainter::TransformArea(const Projection& projection,
                                 const MapParameter& parameter,
                                 const std::vector<Point>& nodes,
                                 std::vector<TransPoint>& points)
  {
    points.resize(nodes.size());

    if (parameter.GetOptimizeAreaNodes()) {
      points[0].draw=true;
      points[nodes.size()-1].draw=true;

      // Drop every point that is on direct line between two points A and B
      for (size_t i=1; i+1<nodes.size(); i++) {
        points[i].draw=std::abs((nodes[i].lon-nodes[i-1].lon)/
                                (nodes[i].lat-nodes[i-1].lat)-
                                (nodes[i+1].lon-nodes[i].lon)/
                                (nodes[i+1].lat-nodes[i].lat))>=relevantSlopeDeriviation;
      }

      // Calculate screen position
      for (size_t i=0; i<nodes.size(); i++) {
        if (points[i].draw) {
          projection.GeoToPixel(nodes[i].lon,nodes[i].lat,
                                points[i].x,points[i].y);
        }
      }

      // Drop all points that do not differ in position from the previous node
      for (size_t i=1; i<nodes.size()-1; i++) {
        if (points[i].draw) {
          size_t j=i+1;
          while (!points[j].draw) {
            j++;
          }

          if (std::fabs(points[j].x-points[i].x)<=relevantPosDeriviation &&
              std::fabs(points[j].y-points[i].y)<=relevantPosDeriviation) {
            points[i].draw=false;
          }
        }
      }
    }
    else {
      for (size_t i=0; i<nodes.size(); i++) {
        points[i].draw=true;
        projection.GeoToPixel(nodes[i].lon,nodes[i].lat,
                              points[i].x,points[i].y);
      }
    }
  }

  void MapPainter::TransformWay(const Projection& projection,
                                const MapParameter& parameter,
                                const std::vector<Point>& nodes,
                                std::vector<TransPoint>& points)
  {
    points.resize(nodes.size());

    if (parameter.GetOptimizeWayNodes()) {
      size_t a;

      for (size_t i=0; i<nodes.size(); i++) {
        points[i].draw=true;
      }

      if (nodes.size()>=3) {
        a=0;
        while (a+1<nodes.size()) {
          if (projection.GeoIsIn(nodes[a].lon,nodes[a].lat)) {
            break;
          }

          a++;
        }

        if (a>1) {
          for (size_t i=0; i<a-1; i++) {
            points[i].draw=false;
          }
        }
      }

      if (nodes.size()>=3) {
        a=nodes.size()-1;
        while (a>0) {
          if (projection.GeoIsIn(nodes[a].lon,nodes[a].lat)) {
            break;
          }

          a--;
        }

        if (a<nodes.size()-2) {
          for (size_t i=a+2; i<nodes.size(); i++) {
            points[i].draw=false;
          }
        }
      }

      // Drop every point that is on direct line between two points A and B
      for (size_t i=0; i+2<nodes.size(); i++) {
        if (points[i].draw) {
          size_t j=i+1;
          while (j<nodes.size() && !points[j].draw) {
            j++;
          }

          size_t k=j+1;
          while (k<nodes.size() && !points[k].draw) {
            k++;
          }

          if (j<nodes.size() && k<nodes.size()) {
            points[j].draw=std::abs((nodes[j].lon-nodes[i].lon)/
                                    (nodes[j].lat-nodes[i].lat)-
                                    (nodes[k].lon-nodes[j].lon)/
                                    (nodes[k].lat-nodes[j].lat))>=relevantSlopeDeriviation;
          }
        }
      }

      // Calculate screen position
      for (size_t i=0; i<nodes.size(); i++) {
        if (points[i].draw) {
          projection.GeoToPixel(nodes[i].lon,nodes[i].lat,
                                points[i].x,points[i].y);
        }
      }

      // Drop all points that do not differ in position from the previous node
      if (nodes.size()>2) {
        for (size_t i=1; i<nodes.size()-1; i++) {
          if (points[i].draw) {
            size_t j=i+1;

            while (j+1<nodes.size() &&
                   !points[j].draw) {
              j++;
            }

            if (std::fabs(points[j].x-points[i].x)<=relevantPosDeriviation &&
                std::fabs(points[j].y-points[i].y)<=relevantPosDeriviation) {
              points[i].y=false;
            }
          }
        }
      }

      /*
      // Check which nodes or not visible in the given way
      for (size_t i=0; i<way->nodes.size(); i++) {
        if (way->nodes[i].lon<lonMin || way->nodes[i].lon>lonMax ||
            way->nodes[i].lat<latMin || way->nodes[i].lat>latMax){
          outNode[i]=true;
        }
      }

      if (outNode[1]) {
        drawNode[0]=false;
      }

      for (size_t i=1; i<way->nodes.size()-1; i++) {
        if (outNode[i-1] && outNode[i+1]) {
          drawNode[i]=false;
        }
      }

      if (outNode[way->nodes.size()-2]) {
        drawNode[way->nodes.size()-1]=false;
      }*/
    }
    else {
      for (size_t i=0; i<nodes.size(); i++) {
        points[i].draw=true;
        projection.GeoToPixel(nodes[i].lon,
                              nodes[i].lat,
                              points[i].x,
                              points[i].y);
      }
    }
  }

  bool MapPainter::GetBoundingBox(const std::vector<Point>& nodes,
                                  double& xmin, double& ymin,
                                  double& xmax, double& ymax)
  {
    if (nodes.size()==0) {
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
                                  double& cy)
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

    landFill.SetColor(241.0/255,238.0/255,233.0/255,1.0);
    seeFill.SetColor(181.0/255,208.0/255,208.0/255,1.0);
    coastFill.SetColor(255.0/255,192.0/255,203.0/255,1.0);
    unknownFill.SetColor(255.0/255,255.0/255,173.0/255,1.0);

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

  void MapPainter::DrawTiledLabel(const Projection& projection,
                                  const MapParameter& parameter,
                                  const LabelStyle& style,
                                  const std::string& label,
                                  const std::vector<Point>& nodes,
                                  std::set<size_t>& tileBlacklist)
  {
    double x,y;
    double xmin;
    double xmax;
    double ymin;
    double ymax;

    if (!GetBoundingBox(nodes,xmin,ymin,xmax,ymax)) {
      return;
    }

    projection.GeoToPixel(xmin,ymin,xmin,ymin);
    projection.GeoToPixel(xmax,ymax,xmax,ymax);

    x=xmin+(xmax-xmin)/2;
    y=ymin+(ymax-ymin)/2;

    size_t tx,ty;

    tx=(x-xmin)*20/(xmax-xmin);
    ty=(y-ymin)*20/(ymax-ymin);

    size_t tile=20*ty+tx;

    if (tileBlacklist.find(tile)!=tileBlacklist.end()) {
      return;
    }

    DrawLabel(projection,
              parameter,
              style,
              label,
              x,y);

    tileBlacklist.insert(tile);
  }

  void MapPainter::PrecalculateStyleData(const StyleConfig& styleConfig,
                                         const Projection& projection,
                                         const MapParameter& parameter,
                                         const MapData& data)
  {
    //
    // Calculate border width for each way line style
    //

    borderWidth.resize(styleConfig.GetStyleCount(),0);
    for (size_t i=0; i<styleConfig.GetStyleCount(); i++) {
      const LineStyle *borderStyle=styleConfig.GetAreaBorderStyle(i);

      if (borderStyle!=NULL) {
        borderWidth[i]=borderStyle->GetWidth()/projection.GetPixelSize();
        if (borderWidth[i]<borderStyle->GetMinPixel()) {
          borderWidth[i]=borderStyle->GetMinPixel();
        }
      }
    }

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

      const FillStyle *style=styleConfig.GetAreaFillStyle(area->GetType(),
                                                          area->IsBuilding());

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

      const FillStyle *style=styleConfig.GetAreaFillStyle(relation->GetType(),
                                                          false/*relation->flags & Way::isBuilding*/);

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

      TransPoint point;

      Transform(projection,
                parameter,
                node->GetLon(),
                node->GetLat(),
                point);

      if (hasLabel) {
        if (hasSymbol) {
          DrawLabel(projection,
                    parameter,
                    *labelStyle,
                    label,
                    point.x,
                    point.y+symbolStyle->GetSize()+5); // TODO: Better layout to real size of symbol
        }
        else if (hasIcon) {
          DrawLabel(projection,
                    parameter,
                    *labelStyle,
                    label,
                    point.x,
                    point.y+14+5); // TODO: Better layout to real size of icon
        }
        else {
          DrawLabel(projection,
                    parameter,
                    *labelStyle,
                    label,
                    point.x,
                    point.y);
        }
      }

      if (hasIcon) {
        DrawIcon(iconStyle,
                 point.x,
                 point.y);
      }

      if (hasSymbol) {
        DrawSymbol(symbolStyle,
                   point.x,
                   point.y);
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
          const FillStyle *fillStyle=styleConfig.GetAreaFillStyle(area->GetType(),
                                                                  area->GetAttributes().IsBuilding());
          const LineStyle *lineStyle=styleConfig.GetAreaBorderStyle(area->GetType());

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

          TransformArea(projection,
                        parameter,
                        area->nodes,
                        points);

          if (hasPattern) {
            DrawArea(projection,
                     parameter,
                     area->GetType(),
                     *patternStyle,
                     lineStyle,
                     points);

            areasDrawn++;
          }
          else if (hasFill) {
            DrawArea(projection,
                     parameter,
                     area->GetType(),
                     *fillStyle,
                     lineStyle,
                     points);

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
              const FillStyle *fillStyle=styleConfig.GetAreaFillStyle(relation->roles[m].GetType(),
                                                                      relation->roles[m].GetAttributes().IsBuilding());
              const LineStyle *lineStyle=styleConfig.GetAreaBorderStyle(relation->roles[m].GetType());

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
                TransformArea(projection,
                              parameter,
                              relation->roles[m].nodes,
                              points);

                DrawArea(projection,
                         parameter,
                         relation->roles[m].GetType(),
                         *patternStyle,
                         lineStyle,
                         points);
                drawn=true;
              }
              else if (hasFill) {
                TransformArea(projection,
                              parameter,
                              relation->roles[m].nodes,
                              points);

                DrawArea(projection,
                         parameter,
                         relation->roles[m].GetType(),
                         *fillStyle,
                         lineStyle,
                         points);
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
          DrawLabel(projection,
                    parameter,
                    *labelStyle,
                    label,
                    x,y+symbolStyle->GetSize()+5); // TODO: Better layout to real size of symbol
        }
        else if (hasIcon) {
          DrawLabel(projection,
                    parameter,
                    *labelStyle,
                    label,
                    x,y+14+5); // TODO: Better layout to real size of icon
        }
        else {
          DrawLabel(projection,
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
              DrawLabel(projection,
                        parameter,
                        *labelStyle,
                        label,
                        x,y+symbolStyle->GetSize()+5); // TODO: Better layout to real size of symbol
            }
            else if (hasIcon) {
              DrawLabel(projection,
                        parameter,
                        *labelStyle,
                        label,
                        x,y+14+5); // TODO: Better layout to real size of icon
            }
            else {
              DrawLabel(projection,
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
                           const std::vector<Point>& nodes)
  {
    double lineWidth=attributes.GetWidth();

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

    if (style.GetOutline()>0 &&
        !outline &&
        !(attributes.IsBridge() &&
          projection.GetMagnification()>=magCity) &&
        !(attributes.IsTunnel() &&
          projection.GetMagnification()>=magCity)) {
      // Should draw outline, but resolution is too low
      // Draw line with alternate color
      TransformWay(projection,parameter,nodes,points);

      DrawPath(projection,
               parameter,
               style.GetAlternateR(),
               style.GetAlternateG(),
               style.GetAlternateB(),
               style.GetAlternateA(),
               lineWidth,
               style.GetDash(),
               capRound,
               capRound,
               points);
    }
    else if (outline) {
      // Draw outline
      // Draw line with normal color but reduced with
      TransformWay(projection,parameter,nodes,points);

      DrawPath(projection,
               parameter,
               style.GetLineR(),
               style.GetLineG(),
               style.GetLineB(),
               style.GetLineA(),
               lineWidth-2*style.GetOutline(),
               style.GetDash(),
               capRound,
               capRound,
               points);
    }
    else {
      // Draw without outline
      // Draw line with normal color and normal width

      TransformWay(projection,parameter,nodes,points);

      DrawPath(projection,
               parameter,
               style.GetLineR(),
               style.GetLineG(),
               style.GetLineB(),
               style.GetLineA(),
               lineWidth,
               style.GetDash(),
               capRound,
               capRound,
               points);
    }
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

    if (!(drawBridge ||
          drawTunnel ||
          outline)) {
      return false;
    }

    if (drawBridge) {
      // black outline for bridges

      TransformWay(projection,parameter,nodes,points);

      DrawPath(projection,
               parameter,
               0.0,
               0.0,
               0.0,
               1.0,
               lineWidth,
               emptyDash,
               attributes.StartIsJoint() ? capButt : capRound,
               attributes.EndIsJoint() ? capButt : capRound,
               points);
    }
    else if (drawTunnel) {
      if (projection.GetMagnification()>=10000) {
        // light grey dashes

        TransformWay(projection,parameter,nodes,points);

        DrawPath(projection,
                 parameter,
                 0.75,
                 0.75,
                 0.75,
                 1.0,
                 lineWidth,
                 tunnelDash,
                 attributes.StartIsJoint() ? capButt : capRound,
                 attributes.EndIsJoint() ? capButt : capRound,
                 points);
      }
      else {
        // dark grey dashes

        TransformWay(projection,parameter,nodes,points);

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
                 points);
      }
    }
    else {
      // normal path, normal outline color

      TransformWay(projection,parameter,nodes,points);

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
               points);
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

            if (IsVisible(projection,
                          relation->roles[m].nodes,
                          style->GetWidth())) {
              if (DrawWayOutline(projection,
                                 parameter,
                                 *style,
                                 relation->roles[m].GetAttributes(),
                                 relation->roles[m].nodes)) {
                drawn=true;
              }
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

          const LineStyle *style=styleConfig.GetWayLineStyle(way->GetType());

          if (style==NULL) {
            continue;
          }

          DrawWay(projection,
                  parameter,
                  *style,
                  way->GetAttributes(),
                  way->nodes);

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

            if (IsVisible(projection,
                          relation->roles[m].nodes,
                          style->GetWidth())) {
              DrawWay(projection,
                      parameter,
                      *style,
                      relation->roles[m].GetAttributes(),
                      relation->roles[m].nodes);

              drawn=true;
            }
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
            projection.GetMagnification()>=style->GetMinMag() &&
            projection.GetMagnification()<=style->GetMaxMag()) {

          if (style->GetStyle()==LabelStyle::contour) {
            TransformWay(projection,parameter,way->nodes,points);

            DrawContourLabel(projection,
                             parameter,
                             *style,
                             way->GetName(),
                             points);
          }
          else {
            DrawTiledLabel(projection,
                           parameter,
                           *style,
                           way->GetName(),
                           way->nodes,
                           tileBlacklist);
          }

          waysLabelDrawn++;
        }
      }

      if (!way->GetRefName().empty()) {
        const LabelStyle *style=styleConfig.GetWayRefLabelStyle(way->GetType());

        if (style!=NULL &&
            projection.GetMagnification()>=style->GetMinMag() &&
            projection.GetMagnification()<=style->GetMaxMag()) {

          if (style->GetStyle()==LabelStyle::contour) {
            TransformWay(projection,parameter,way->nodes,points);

            DrawContourLabel(projection,
                             parameter,
                             *style,
                             way->GetRefName(),
                             points);
          }
          else {
            DrawTiledLabel(projection,
                           parameter,
                           *style,
                           way->GetRefName(),
                           way->nodes,
                           tileBlacklist);
          }

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
              projection.GetMagnification()>=style->GetMinMag() &&
              projection.GetMagnification()<=style->GetMaxMag()) {

            if (style->GetStyle()==LabelStyle::contour) {
              if (IsVisible(projection,
                            relation->roles[m].nodes,
                            style->GetSize())) {
                TransformWay(projection,parameter,relation->roles[m].nodes,points);

                DrawContourLabel(projection,
                                 parameter,
                                 *style,
                                 relation->roles[m].GetName(),
                                 points);
              }
            }
            else {
              DrawTiledLabel(projection,
                             parameter,
                             *style,
                             relation->roles[m].GetName(),
                             relation->roles[m].nodes,
                             tileBlacklist);
            }

            relWaysLabelDrawn++;
          }
        }

        if (!relation->roles[m].GetRefName().empty()) {
          const LabelStyle *style=styleConfig.GetWayRefLabelStyle(relation->roles[m].GetType());

          if (style!=NULL &&
              projection.GetMagnification()>=style->GetMinMag() &&
              projection.GetMagnification()<=style->GetMaxMag()) {

            if (style->GetStyle()==LabelStyle::contour) {
              if (IsVisible(projection,
                            relation->roles[m].nodes,
                            style->GetSize())) {
                TransformWay(projection,parameter,relation->roles[m].nodes,points);

                DrawContourLabel(projection,
                                 parameter,
                                 *style,
                                 relation->roles[m].GetRefName(),
                                 points);
              }
            }
            else {
              DrawTiledLabel(projection,
                             parameter,
                             *style,
                             relation->roles[m].GetRefName(),
                             relation->roles[m].nodes,
                             tileBlacklist);
            }

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
        DrawWay(projection,
                parameter,
                *style,
                way->GetAttributes(),
                way->nodes);
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

      const SymbolStyle *style=styleConfig.GetNodeSymbolStyle(node->GetType());

      if (style==NULL ||
          projection.GetMagnification()<style->GetMinMag()) {
        continue;
      }

      double x,y;

      projection.GeoToPixel(node->GetLon(),node->GetLat(),
                            x,y);

      DrawSymbol(style,x,y);

      //nodesDrawnCount++;
    }
  }

  void MapPainter::DrawPOINodeLabels(const StyleConfig& styleConfig,
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

          DrawLabel(projection,
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

          DrawLabel(projection,
                    parameter,
                    *style,
                    node->GetTagValue(i),
                    x,y);
        }
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

    /*
    nodesDrawnCount=0;
    areasDrawnCount=0;
    waysDrawnCount=0;*/

    std::cout << "Draw ";
    std::cout << projection.GetLat() <<", ";
    std::cout << projection.GetLon() << " with mag. ";
    std::cout << projection.GetMagnification() << "x" << "/" << log(projection.GetMagnification())/log(2);
    std::cout << " area " << projection.GetWidth() << "x" << projection.GetHeight() << std::endl;

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

    //
    // POI Node labels
    //

    DrawPOINodeLabels(styleConfig,
                      projection,
                      parameter,
                      data);

    poisTimer.Stop();
/*
    std::cout << "Nodes: " << nodesDrawnCount << "/" << data.nodes.size()+data.poiNodes.size() << " ";
    if (data.nodes.size()+data.poiNodes.size()>0) {
      std::cout << "(" << nodesDrawnCount*100/(data.nodes.size()+data.poiNodes.size()) << "%) ";
    }

    std::cout << " ways: " << waysDrawnCount << "/" << data.ways.size()+data.poiWays.size() << " ";
    if (data.ways.size()+data.poiWays.size()>0) {
      std::cout << "(" << waysDrawnCount*100/(data.ways.size()+data.poiWays.size()) << "%) ";
    }

    std::cout << " areas: " << areasDrawnCount << "/" << data.areas.size() << " ";
    if (data.areas.size()>0) {
      std::cout << "(" << areasDrawnCount*100/data.areas.size() << "%) ";
    }
    std::cout << std::endl;*/

    std::cout << "Paths: " << pathsTimer << "/" << pathLabelsTimer << " ";
    std::cout << "Areas: " << areasTimer << "/" << areaLabelsTimer << " ";
    std::cout << "Nodes: " << nodesTimer << " ";
    std::cout << "POIs: " << poisTimer << "/" << routesTimer << std::endl;

    std::cout << "Path ways: " << waysCount << "/" << waysDrawn << "/" << waysOutlineDrawn << "/" << waysLabelDrawn << " (pcs)" << std::endl;
    std::cout << "Path rels: " << relWaysCount << "/" << relWaysDrawn << "/" << relWaysOutlineDrawn << "/" << relWaysLabelDrawn << " (pcs)" << std::endl;
    std::cout << "Area ways: " << areasCount << "/" << areasDrawn << "/" << areasLabelDrawn << " (pcs)" << std::endl;
    std::cout << "Area rels: " << relAreasCount << "/" << relAreasDrawn << "/" << relAreasLabelDrawn << " (pcs)" << std::endl;
  }
}

