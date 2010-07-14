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

#include <osmscout/MapPainterQt.h>

#include <cassert>
#include <cmath>
#include <iostream>

namespace osmscout {

  MapPainterQt::MapPainterQt()
  {
    // no code
  }

  MapPainterQt::~MapPainterQt()
  {
    // no code
    // TODO: Clean up fonts
  }

  QFont* MapPainterQt::GetFont(const MapParameter& parameter,
                               double fontSize)
  {
    std::map<size_t,QFont*>::const_iterator f;

    f=fonts.find(fontSize);

    if (f!=fonts.end()) {
      return f->second;
    }

    QFont* font=new QFont(parameter.GetFontName().c_str(), QFont::Normal, false);
    
    font->setPixelSize(parameter.GetFontSize()*fontSize);
    font->setStyleStrategy(QFont::PreferAntialias);
    font->setStyleStrategy(QFont::PreferMatch);
    
    return fonts.insert(std::pair<size_t,QFont*>(fontSize,font)).first->second;
  }

  bool MapPainterQt::HasIcon(const StyleConfig& styleConfig,
                             IconStyle& style)
  {
    return false;
  }                           
  
  void MapPainterQt::ClearArea(const StyleConfig& styleConfig,
                               const Projection& projection,
                               const MapParameter& parameter,
                               const MapData& data)
  {
    painter->fillRect(0,
                      0,
                      projection.GetWidth(),
                      projection.GetHeight(),
                      QColor::fromRgbF(241.0/255,238.0/255,233.0/255,1.0));
  }                             
                                                    
  void MapPainterQt::DrawLabel(const Projection& projection,
                               const MapParameter& parameter,
                               const LabelStyle& style,
                               const std::string& text,
                               double x, double y)
  {
    double fontSize=style.GetSize();
    double r=style.GetTextR();
    double g=style.GetTextG();
    double b=style.GetTextB();
    double a=style.GetTextA();
  
    if (projection.GetMagnification()>style.GetScaleAndFadeMag()) {
      double factor=log2(projection.GetMagnification())-log2(style.GetScaleAndFadeMag());
      fontSize=fontSize*pow(2,factor);
      a=a/factor;
    }
  
    QPen pen;

    pen.setColor(QColor::fromRgbF(r,g,b,a));
    painter->setPen(pen);
    
    QFont* font=GetFont(parameter,fontSize);
    
    painter->setFont(*font);
  
    QFontMetrics metrics=QFontMetrics(*font);
  
    QString string=QString::fromUtf8(text.c_str());
  
    QRect extents=metrics.boundingRect(string);
                                  
    painter->drawText(QPointF(x-extents.width()/2,
                              y+metrics.ascent()-extents.height()/2),
                      string);
  }                             

  void MapPainterQt::DrawContourLabel(const Projection& projection,
                                      const MapParameter& parameter,
                                      const LabelStyle& style,
                                      const std::string& text,
                                      const std::vector<Point>& nodes)
  {
    double fontSize=style.GetSize();
    double r=style.GetTextR();
    double g=style.GetTextG();
    double b=style.GetTextB();
    double a=style.GetTextA();

    QPen         pen;
    QFont*       font=GetFont(parameter,fontSize);
    QFontMetrics metrics=QFontMetrics(*font);
    QString      string=QString::fromUtf8(text.c_str());
    QRect        extents=metrics.boundingRect(string);

    pen.setColor(QColor::fromRgbF(r,g,b,a));
    painter->setPen(pen);
    painter->setFont(*font);

    QPainterPath path;

    double x,y;

    if (nodes[0].lon<nodes[nodes.size()-1].lon) {
      for (size_t j=0; j<nodes.size(); j++) {
        projection.GeoToPixel(nodes[j].lon,nodes[j].lat,x,y);
        if (j==0) {
          path.moveTo(x,y);
        }
        else {
          path.lineTo(x,y);
        }
      }
    }
    else {
      for (size_t j=0; j<nodes.size(); j++) {
        projection.GeoToPixel(nodes[nodes.size()-j-1].lon,nodes[nodes.size()-j
            -1].lat,x,y);

        if (j==0) {
          path.moveTo(x,y);
        }
        else {
          path.lineTo(x,y);
        }
      }
    }

    if (path.length()<extents.width()) {
      // Text is longer than path to draw on
      return;
    }

    qreal offset=(path.length()-extents.width())/2;

    for (int i=0; i<string.size(); i++) {
      QPointF point=path.pointAtPercent(path.percentAtLength(offset));
      qreal angle=path.angleAtPercent(path.percentAtLength(offset));

      qreal rad=-qreal(0.017453292519943295769)*angle; // PI/180

      // rotation matrix components
      qreal sina=std::sin(rad);
      qreal cosa=std::cos(rad);

      // Rotation
      qreal newX=(cosa*point.x())-(sina*point.y());
      qreal newY=(cosa*point.y())+(sina*point.x());

      // Aditional offseting
      qreal deltaPenX=cosa*pen.width();
      qreal deltaPenY=sina*pen.width();

      // Getting the delta distance for the translation part of the transformation
      qreal deltaX=newX-point.x();
      qreal deltaY=newY-point.y();
      // Applying rotation and translation.
      QTransform tran(cosa,sina,-sina,cosa,-deltaX+deltaPenX,-deltaY-deltaPenY);

      painter->setWorldTransform(tran);
      painter->drawText(point,QString(string[i]));

      offset+=metrics.width(string[i]);
    }

    painter->resetTransform();
  }                             
                         
  void MapPainterQt::DrawIcon(const IconStyle* style,
                              double x, double y)
  {
  }                             
                        
  void MapPainterQt::DrawSymbol(const SymbolStyle* style,
                                double x, double y)
  {
  }                             

  void MapPainterQt::DrawPath(LineStyle::Style style,
                              const Projection& projection,
                              double r,
                              double g,
                              double b,
                              double a,
                              double width,
                              const std::vector<Point>& nodes)
  {
    QPen pen;

    pen.setColor(QColor::fromRgbF(r,g,b,a));
    pen.setWidthF(width);
    
    switch (style) {
    case LineStyle::none:
      // way should not be visible in this case!
      assert(false);
      break;
    case LineStyle::normal:
      pen.setStyle(Qt::SolidLine);
      pen.setCapStyle(Qt::RoundCap);
      break;
    case LineStyle::longDash:
      pen.setStyle(Qt::DashLine);
      pen.setCapStyle(Qt::FlatCap);
      break;
    case LineStyle::dotted:
      pen.setStyle(Qt::DotLine);
      pen.setCapStyle(Qt::FlatCap);
      break;
    case LineStyle::lineDot:
      pen.setStyle(Qt::DashDotLine);
      pen.setCapStyle(Qt::FlatCap);
      break;
    }

    TransformWay(projection,nodes);

    QPainterPath path;

    bool start=true;
    for (size_t i=0; i<nodes.size(); i++) {
      if (drawNode[i]) {
        if (start) {
          path.moveTo(nodeX[i],nodeY[i]);
          start=false;
        }
        else {
          path.lineTo(nodeX[i],nodeY[i]);
        }
      }
    }

    painter->strokePath(path,pen);
  }
                        
  void MapPainterQt::DrawWayOutline(const StyleConfig& styleConfig,
                                    const Projection& projection,
                                    const MapParameter& parameter,
                                    TypeId type,
                                    const SegmentAttributes& attributes,
                                    const std::vector<Point>& nodes)
  {
    const LineStyle *style=styleConfig.GetWayLineStyle(type);

    if (style==NULL) {
      return;
    }

    double lineWidth=attributes.GetWidth();

    if (lineWidth==0) {
      lineWidth=style->GetWidth();
    }

    lineWidth=lineWidth/projection.GetPixelSize();

    if (lineWidth<style->GetMinPixel()) {
      lineWidth=style->GetMinPixel();
    }

    bool outline=style->GetOutline()>0 &&
                 lineWidth-2*style->GetOutline()>=parameter.GetOutlineMinWidth();

    if (!(attributes.IsBridge() &&
          projection.GetMagnification()>=magCity) &&
        !(attributes.IsTunnel() &&
          projection.GetMagnification()>=magCity) &&
        !outline) {
      return;
    }

    QPen pen;

    if (attributes.IsBridge() &&
        projection.GetMagnification()>=magCity) {

      pen.setColor(QColor::fromRgbF(0.0,0.0,0.0,1.0));        
      pen.setStyle(Qt::SolidLine);
    }
    else if (attributes.IsTunnel() &&
             projection.GetMagnification()>=magCity) {
      double tunnel[2];

      tunnel[0]=7+lineWidth;
      tunnel[1]=7+lineWidth;

      if (projection.GetMagnification()>=10000) {
        pen.setColor(QColor::fromRgbF(0.75,0.75,0.75,1.0));        
      }
      else {
        pen.setColor(QColor::fromRgbF(0.5,0.5,0.5,1.0));        
      }
      pen.setStyle(Qt::DashLine);
    }
    else {
      pen.setStyle(Qt::SolidLine);
      pen.setColor(QColor::fromRgbF(style->GetOutlineR(),
                                    style->GetOutlineG(),
                                    style->GetOutlineB(),
                                    style->GetOutlineA()));
    }
    pen.setCapStyle(Qt::FlatCap);

    pen.setWidthF(lineWidth);

    TransformWay(projection,nodes);

    QPainterPath path;

    bool start=true;
    for (size_t i=0; i<nodes.size(); i++) {
      if (drawNode[i]) {
        if (start) {
          path.moveTo(nodeX[i],nodeY[i]);
          start=false;
        }
        else {
          path.lineTo(nodeX[i],nodeY[i]);
        }
      }
    }

    painter->strokePath(path,pen);

    /*
    if (!attributes.StartIsJoint()) {
      cairo_set_line_cap(draw,CAIRO_LINE_CAP_ROUND);
      cairo_set_dash(draw,NULL,0,0);
      cairo_set_source_rgba(draw,
                            style->GetOutlineR(),
                            style->GetOutlineG(),
                            style->GetOutlineB(),
                            style->GetOutlineA());
      cairo_set_line_width(draw,lineWidth);

      cairo_move_to(draw,nodeX[0],nodeY[0]);
      cairo_line_to(draw,nodeX[0],nodeY[0]);
      cairo_stroke(draw);
    }

    if (!attributes.EndIsJoint()) {
      cairo_set_line_cap(draw,CAIRO_LINE_CAP_ROUND);
      cairo_set_dash(draw,NULL,0,0);
      cairo_set_source_rgba(draw,
                            style->GetOutlineR(),
                            style->GetOutlineG(),
                            style->GetOutlineB(),
                            style->GetOutlineA());
      cairo_set_line_width(draw,lineWidth);

      cairo_move_to(draw,nodeX[nodes.size()-1],nodeY[nodes.size()-1]);
      cairo_line_to(draw,nodeX[nodes.size()-1],nodeY[nodes.size()-1]);
      cairo_stroke(draw);
    }*/
  }                             

  void MapPainterQt::DrawArea(const StyleConfig& styleConfig,
                              const Projection& projection,
                              TypeId type,
                              int layer,
                              const SegmentAttributes& attributes,
                              const std::vector<Point>& nodes)
  {
    PatternStyle    *patternStyle=styleConfig.GetAreaPatternStyle(type);
    const FillStyle *fillStyle=styleConfig.GetAreaFillStyle(type,
                                                            attributes.IsBuilding());

    bool               hasPattern=patternStyle!=NULL &&
                                  patternStyle->GetLayer()==layer &&
                                  projection.GetMagnification()>=patternStyle->GetMinMag();
    bool               hasFill=fillStyle!=NULL &&
                               fillStyle->GetLayer()==layer;

    TransformArea(projection,nodes);

    QPainterPath path;

    bool start=true;
    for (size_t i=0; i<nodes.size(); i++) {
      if (drawNode[i]) {
        if (start) {
          path.moveTo(nodeX[i],nodeY[i]);
          start=false;
        }
        else {
          path.lineTo(nodeX[i],nodeY[i]);
        }
      }
    }

    SetPen(styleConfig.GetAreaBorderStyle(type),
           borderWidth[(size_t)type]);

    /*
    if (hasPattern) {
      hasPattern=HasPattern(styleConfig,*patternStyle);
    }*/
    hasPattern=false;

    if (hasPattern) {
      painter->setBrush(Qt::NoBrush);
      //FillRegion(nodes,projection,*patternStyle);
    }
    else if (hasFill) {
      SetBrush(fillStyle);
    }
    else {
      SetBrush();
    }
    
    painter->drawPath(path);
  }                             

  void MapPainterQt::SetPen(const LineStyle* style, double lineWidth)
  {
    if (style==NULL) {
      painter->setPen(Qt::NoPen);
    }
    else {
      QPen pen;

      pen.setColor(QColor::fromRgbF(style->GetLineR(),
                                    style->GetLineG(),
                                    style->GetLineB(),
                                    style->GetLineA()));
      pen.setWidthF(lineWidth);
      
      switch (style->GetStyle()) {
      case LineStyle::none:
        // way should not be visible in this case!
        assert(false);
        break;
      case LineStyle::normal:
        pen.setStyle(Qt::SolidLine);
        pen.setCapStyle(Qt::RoundCap);
        break;
      case LineStyle::longDash:
        pen.setStyle(Qt::DashLine);
        pen.setCapStyle(Qt::FlatCap);
        break;
      case LineStyle::dotted:
        pen.setStyle(Qt::DotLine);
        pen.setCapStyle(Qt::FlatCap);
        break;
      case LineStyle::lineDot:
        pen.setStyle(Qt::DashDotLine);
        pen.setCapStyle(Qt::FlatCap);
        break;
      }
      
      painter->setPen(pen);
    }
  }

  void MapPainterQt::SetBrush()
  {
    painter->setBrush(Qt::NoBrush);
  }
  
  void MapPainterQt::SetBrush(const FillStyle* fillStyle)
  {
    painter->setBrush(QBrush(QColor::fromRgbF(fillStyle->GetFillR(),
                                              fillStyle->GetFillG(),
                                              fillStyle->GetFillB(),
                                              1)));
  }

  bool MapPainterQt::DrawMap(const StyleConfig& styleConfig,
                             const Projection& projection,
                             const MapParameter& parameter,
                             const MapData& data,
                             QPainter* painter)
  {
    this->painter=painter;

    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::TextAntialiasing);
  
    Draw(styleConfig,
         projection,
         parameter,
         data);
  
    return true;
  }
}

