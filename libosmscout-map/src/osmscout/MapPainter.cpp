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

#include <osmscout/Util.h>

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
    drawNode.resize(100000); // TODO: Calculate matching size
    nodeX.resize(100000);
    nodeY.resize(100000);
  }

  MapPainter::~MapPainter()
  {
    // no code
  }

  bool MapPainter::IsVisible(const Projection& projection,
                             const std::vector<Point>& nodes) const
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

    // If bounding box is neither left or right nor above or below
    // it must somehow cover the map area.
    return projection.GeoIsIn(lonMin,latMin,lonMax,latMax);
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
                               latMin,
                               xMin,
                               yMin)) {
      return false;
    }

    if (!projection.GeoToPixel(lonMax,
                               latMax,
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

  void MapPainter::TransformArea(const Projection& projection,
                                 const MapParameter& parameter,
                                 const std::vector<Point>& nodes)
  {
    if (!parameter.GetOptimizeAreaNodes()) {
      for (size_t i=0; i<nodes.size(); i++) {
        drawNode[i]=true;
        projection.GeoToPixel(nodes[i].lon,nodes[i].lat,
                              nodeX[i],nodeY[i]);
      }
    }
    else {
      drawNode[0]=true;
      drawNode[nodes.size()-1]=true;

      // Drop every point that is on direct line between two points A and B
      for (size_t i=1; i+1<nodes.size(); i++) {
        drawNode[i]=std::abs((nodes[i].lon-nodes[i-1].lon)/
                             (nodes[i].lat-nodes[i-1].lat)-
                             (nodes[i+1].lon-nodes[i].lon)/
                             (nodes[i+1].lat-nodes[i].lat))>=relevantSlopeDeriviation;
      }

      // Calculate screen position
      for (size_t i=0; i<nodes.size(); i++) {
        if (drawNode[i]) {
          projection.GeoToPixel(nodes[i].lon,nodes[i].lat,
                                nodeX[i],nodeY[i]);
        }
      }

      // Drop all points that do not differ in position from the previous node
      for (size_t i=1; i<nodes.size()-1; i++) {
        if (drawNode[i]) {
          size_t j=i+1;
          while (!drawNode[j]) {
            j++;
          }

          if (std::fabs(nodeX[j]-nodeX[i])<=relevantPosDeriviation &&
              std::fabs(nodeY[j]-nodeY[i])<=relevantPosDeriviation) {
            drawNode[i]=false;
          }
        }
      }
    }
  }

  void MapPainter::TransformWay(const Projection& projection,
                                const MapParameter& parameter,
                                const std::vector<Point>& nodes)
  {
    if (!parameter.GetOptimizeWayNodes()) {
      for (size_t i=0; i<nodes.size(); i++) {
        drawNode[i]=true;
        projection.GeoToPixel(nodes[i].lon,nodes[i].lat,
                              nodeX[i],nodeY[i]);
      }
    }
    else {
      size_t a;

      for (size_t i=0; i<nodes.size(); i++) {
        drawNode[i]=true;
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
            drawNode[i]=false;
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
            drawNode[i]=false;
          }
        }
      }

      // Drop every point that is on direct line between two points A and B
      for (size_t i=0; i+2<nodes.size(); i++) {
        if (drawNode[i]) {
          size_t j=i+1;
          while (j<nodes.size() && !drawNode[j]) {
            j++;
          }

          size_t k=j+1;
          while (k<nodes.size() && !drawNode[k]) {
            k++;
          }

          if (j<nodes.size() && k<nodes.size()) {
            drawNode[j]=std::abs((nodes[j].lon-nodes[i].lon)/
                                 (nodes[j].lat-nodes[i].lat)-
                                 (nodes[k].lon-nodes[j].lon)/
                                 (nodes[k].lat-nodes[j].lat))>=relevantSlopeDeriviation;
          }
        }
      }

      // Calculate screen position
      for (size_t i=0; i<nodes.size(); i++) {
        if (drawNode[i]) {
          projection.GeoToPixel(nodes[i].lon,nodes[i].lat,
                                nodeX[i],nodeY[i]);
        }
      }

      // Drop all points that do not differ in position from the previous node
      if (nodes.size()>2) {
        for (size_t i=1; i<nodes.size()-1; i++) {
          if (drawNode[i]) {
            size_t j=i+1;

            while (j+1<nodes.size() &&
                   !drawNode[j]) {
              j++;
            }

            if (std::fabs(nodeX[j]-nodeX[i])<=relevantPosDeriviation &&
                std::fabs(nodeY[j]-nodeY[i])<=relevantPosDeriviation) {
              drawNode[i]=false;
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

    for (std::vector<Way>::const_iterator way=data.ways.begin();
         way!=data.ways.end();
         ++way) {
      if (way->GetLayer()>=-5 && way->GetLayer()<=5) {
        wayLayers[way->GetLayer()+5]=true;
      }
    }

    for (size_t i=0; i<11; i++) {
      relationWayLayers[i]=false;
    }

    for (std::vector<Relation>::const_iterator relation=data.relationWays.begin();
         relation!=data.relationWays.end();
         ++relation) {
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

    for (std::vector<Way>::const_iterator area=data.areas.begin();
         area!=data.areas.end();
         ++area) {
      const FillStyle *style=styleConfig.GetAreaFillStyle(area->GetType(),
                                                          area->IsBuilding());

      if (style!=NULL &&
          style->GetLayer()>=-5 &&
          style->GetLayer()<=5) {
        areaLayers[style->GetLayer()+5]=true;
      }
    }

    for (std::vector<Relation>::const_iterator relation=data.relationAreas.begin();
         relation!=data.relationAreas.end();
         ++relation) {
      const FillStyle *style=styleConfig.GetAreaFillStyle(relation->type,
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
    for (std::vector<Node>::const_iterator node=data.nodes.begin();
         node!=data.nodes.end();
         ++node) {
      const LabelStyle  *labelStyle=styleConfig.GetNodeLabelStyle(node->type);
      IconStyle         *iconStyle=styleConfig.GetNodeIconStyle(node->type);
      const SymbolStyle *symbolStyle=iconStyle!=NULL ? NULL : styleConfig.GetNodeSymbolStyle(node->type);

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
        for (size_t i=0; i<node->tags.size(); i++) {
          // TODO: We should make sure we prefer one over the other
          if (node->tags[i].key==tagName) {
            label=node->tags[i].value;
            break;
          }
          else if (node->tags[i].key==tagRef)  {
            label=node->tags[i].value;
          }
        }

        hasLabel=!label.empty();
      }

      if (hasIcon) {
        hasIcon=HasIcon(styleConfig,*iconStyle);
      }

      if (!hasSymbol && !hasLabel && !hasIcon) {
        continue;
      }

      double x,y;

      projection.GeoToPixel(node->lon,node->lat,x,y);

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
        for (std::vector<Way>::const_iterator area=data.areas.begin();
             area!=data.areas.end();
             ++area) {
          DrawArea(styleConfig,
                   projection,
                   parameter,
                   area->GetType(),
                   layer,
                   area->GetAttributes(),
                   area->nodes);
        }
      }

      if (relationAreaLayers[l]) {
        for (std::vector<Relation>::const_iterator relation=data.relationAreas.begin();
             relation!=data.relationAreas.end();
             ++relation) {
          bool drawn=false;
          for (size_t m=0; m<relation->roles.size(); m++) {
            if (relation->roles[m].role=="0") {
              drawn=true;

              DrawArea(styleConfig,
                       projection,
                       parameter,
                       relation->roles[m].GetType(),
                       layer,
                       relation->roles[m].GetAttributes(),
                       relation->roles[m].nodes);
            }

            if (!drawn) {
              std::cout << " Something is wrong with area relation " << relation->id << std::endl;
            }
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
    for (std::vector<Way>::const_iterator area=data.areas.begin();
         area!=data.areas.end();
         ++area) {
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
        hasIcon=HasIcon(styleConfig,*iconStyle);
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
      }

      if (hasIcon) {
        DrawIcon(iconStyle,x,y);
      }

      if (hasSymbol) {
        DrawSymbol(symbolStyle,x,y);
      }
    }

    for (std::vector<Relation>::const_iterator relation=data.relationAreas.begin();
         relation!=data.relationAreas.end();
         ++relation) {
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
            hasIcon=HasIcon(styleConfig,*iconStyle);
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

    if (lineWidth==0) {
      lineWidth=style.GetWidth();
    }

    lineWidth=std::max(style.GetMinPixel(),
                       lineWidth/projection.GetPixelSize());

    bool outline=style.GetOutline()>0 &&
                 lineWidth-2*style.GetOutline()>=parameter.GetOutlineMinWidth();

    if (style.GetOutline()>0 &&
        !outline &&
        !(attributes.IsBridge() &&
          projection.GetMagnification()>=magCity) &&
        !(attributes.IsTunnel() &&
          projection.GetMagnification()>=magCity)) {
      // Should draw outline, but resolution is too low
      DrawPath(style.GetStyle(),
               projection,
               parameter,
               style.GetAlternateR(),
               style.GetAlternateG(),
               style.GetAlternateB(),
               style.GetAlternateA(),
               lineWidth,
               nodes);
    }
    else if (outline) {
      // Draw outline
      DrawPath(style.GetStyle(),
               projection,
               parameter,
               style.GetLineR(),
               style.GetLineG(),
               style.GetLineB(),
               style.GetLineA(),
               lineWidth-2*style.GetOutline(),
               nodes);
    }
    else {
      // Draw without outline
      DrawPath(style.GetStyle(),
               projection,
               parameter,
               style.GetLineR(),
               style.GetLineG(),
               style.GetLineB(),
               style.GetLineA(),
               lineWidth,
               nodes);
    }
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
        for (std::vector<Way>::const_iterator way=data.ways.begin();
             way!=data. ways.end();
             ++way) {

          if (way->GetLayer()!=layer) {
            continue;
          }

          const LineStyle *style=styleConfig.GetWayLineStyle(way->GetType());

          if (style==NULL) {
            continue;
          }

          DrawWayOutline(projection,
                         parameter,
                         *style,
                         way->GetAttributes(),
                         way->nodes);
        }
      }

      if (relationWayLayers[l]) {
        for (std::vector<Relation>::const_iterator relation=data.relationWays.begin();
             relation!=data.relationWays.end();
             ++relation) {
          for (size_t m=0; m<relation->roles.size(); m++) {
            TypeId type=relation->roles[m].GetType()==typeIgnore ? relation->type : relation->roles[m].GetType();

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
              DrawWayOutline(projection,
                             parameter,
                             *style,
                             relation->roles[m].GetAttributes(),
                             relation->roles[m].nodes);
            }
          }
        }
      }

      if (wayLayers[l]) {
        for (std::vector<Way>::const_iterator way=data.ways.begin();
             way!=data.ways.end();
             ++way) {

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
        }
      }

      if (relationWayLayers[l]) {
        for (std::vector<Relation>::const_iterator relation=data.relationWays.begin();
             relation!=data.relationWays.end();
             ++relation) {
          //std::cout << "Draw way relation " << relation->id << std::endl;
          for (size_t m=0; m<relation->roles.size(); m++) {
            TypeId type=relation->roles[m].GetType()==typeIgnore ? relation->type : relation->roles[m].GetType();

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
            }
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

    for (std::vector<Way>::const_iterator way=data.ways.begin();
         way!=data.ways.end();
         ++way) {
      if (!way->GetName().empty()) {
        const LabelStyle *style=styleConfig.GetWayNameLabelStyle(way->GetType());

        if (style!=NULL &&
            projection.GetMagnification()>=style->GetMinMag() &&
            projection.GetMagnification()<=style->GetMaxMag()) {

          if (style->GetStyle()==LabelStyle::contour) {
            DrawContourLabel(projection,
                             parameter,
                             *style,
                             way->GetName(),
                             way->nodes);
          }
          else {
            DrawTiledLabel(projection,
                           parameter,
                           *style,
                           way->GetName(),
                           way->nodes,
                           tileBlacklist);
          }
        }
      }

      if (!way->GetRefName().empty()) {
        const LabelStyle *style=styleConfig.GetWayRefLabelStyle(way->GetType());

        if (style!=NULL &&
            projection.GetMagnification()>=style->GetMinMag() &&
            projection.GetMagnification()<=style->GetMaxMag()) {

          if (style->GetStyle()==LabelStyle::contour) {
            DrawContourLabel(projection,
                             parameter,
                             *style,
                             way->GetRefName(),
                             way->nodes);
          }
          else {
            DrawTiledLabel(projection,
                           parameter,
                           *style,
                           way->GetRefName(),
                           way->nodes,
                           tileBlacklist);
          }
        }
      }
    }

    for (std::vector<Relation>::const_iterator relation=data.relationWays.begin();
         relation!=data.relationWays.end();
         ++relation) {
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
                DrawContourLabel(projection,
                                 parameter,
                                 *style,
                                 relation->roles[m].GetName(),
                                 relation->roles[m].nodes);
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
                DrawContourLabel(projection,
                                 parameter,
                                 *style,
                                 relation->roles[m].GetRefName(),
                                 relation->roles[m].nodes);
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
    for (std::list<Way>::const_iterator way=data.poiWays.begin();
         way!=data.poiWays.end();
         ++way) {

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
    for (std::list<Node>::const_iterator node=data.poiNodes.begin();
         node!=data.poiNodes.end();
         ++node) {
      if (!projection.GeoIsIn(node->lon,node->lat)) {
        continue;
      }

      const SymbolStyle *style=styleConfig.GetNodeSymbolStyle(node->type);

      if (style==NULL ||
          projection.GetMagnification()<style->GetMinMag()) {
        continue;
      }

      double x,y;

      projection.GeoToPixel(node->lon,node->lat,x,y);

      DrawSymbol(style,x,y);

      //nodesDrawnCount++;
    }
  }

  void MapPainter::DrawPOINodeLabels(const StyleConfig& styleConfig,
                                     const Projection& projection,
                                     const MapParameter& parameter,
                                     const MapData& data)
  {
    for (std::list<Node>::const_iterator node=data.poiNodes.begin();
         node!=data.poiNodes.end();
         ++node) {
      if (!projection.GeoIsIn(node->lon,node->lat)) {
        continue;
      }

      for (size_t i=0; i<node->tags.size(); i++) {
        // TODO: We should make sure we prefer one over the other
        if (node->tags[i].key==tagName) {
          const LabelStyle *style=styleConfig.GetNodeLabelStyle(node->type);

          if (style==NULL ||
              projection.GetMagnification()<style->GetMinMag() ||
              projection.GetMagnification()>style->GetMaxMag()) {
            continue;
          }

          double x,y;

          projection.GeoToPixel(node->lon,node->lat,x,y);

          DrawLabel(projection,
                    parameter,
                    *style,
                    node->tags[i].value,
                    x,y);
        }
        else if (node->tags[i].key==tagRef)  {
          const LabelStyle *style=styleConfig.GetNodeRefLabelStyle(node->type);

          if (style==NULL ||
              projection.GetMagnification()<style->GetMinMag() ||
              projection.GetMagnification()>style->GetMaxMag()) {
            continue;
          }

          double x,y;

          projection.GeoToPixel(node->lon,node->lat,x,y);

          DrawLabel(projection,
                    parameter,
                    *style,
                    node->tags[i].value,
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

    std::cout << "Areas: " << areasTimer <<"/" << areaLabelsTimer;
    std::cout << " Paths: " << pathsTimer << "/" << pathLabelsTimer;
    std::cout << " Nodes: " << nodesTimer;
    std::cout << " POIs: " << poisTimer << "/" << routesTimer << std::endl;
  }
}

