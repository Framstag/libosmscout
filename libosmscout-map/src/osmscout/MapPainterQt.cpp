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
#include <limits>

#include <osmscout/Util.h>

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

  QFont MapPainterQt::GetFont(const MapParameter& parameter,
                              double fontSize)
  {
    std::map<size_t,QFont>::const_iterator f;

    f=fonts.find(fontSize);

    if (f!=fonts.end()) {
      return f->second;
    }

    QFont font(parameter.GetFontName().c_str(),QFont::Normal,false);

    font.setPixelSize(parameter.GetFontSize()*fontSize);
    font.setStyleStrategy(QFont::PreferAntialias);
    font.setStyleStrategy(QFont::PreferMatch);

    return fonts.insert(std::pair<size_t,QFont>(fontSize,font)).first->second;
  }

  bool MapPainterQt::HasIcon(const StyleConfig& styleConfig,
                             IconStyle& style)
  {
    if (style.GetId()==std::numeric_limits<size_t>::max()) {
      return false;
    }

    if (style.GetId()!=0) {
      return true;
    }

    std::string filename=std::string("../libosmscout/data/icons/14x14/standard/")+
                         style.GetIconName()+".png";

    QImage image;

    if (image.load(filename.c_str())) {
      images.resize(images.size()+1,image);
      style.SetId(images.size());
      std::cout << "Loaded image " << filename << " => id " << style.GetId() << std::endl;

      return true;
    }
    else {
      std::cerr << "ERROR while loading icon file '" << filename << "'" << std::endl;
      style.SetId(std::numeric_limits<size_t>::max());

      return false;
    }
  }

  bool MapPainterQt::HasPattern(const StyleConfig& styleConfig,
                                PatternStyle& style)
  {
    if (style.GetId()==std::numeric_limits<size_t>::max()) {
      return false;
    }

    if (style.GetId()!=0) {
      return true;
    }

    std::string filename=std::string("../libosmscout/data/icons/14x14/standard/")+
                         style.GetPatternName()+".png";

    QImage image;

    if (image.load(filename.c_str())) {
      images.resize(images.size()+1,image);
      style.SetId(images.size());
      patterns.resize(images.size());

      patterns[patterns.size()-1].setTextureImage(image);

      std::cout << "Loaded image " << filename << " => id " << style.GetId() << std::endl;

      return true;
    }
    else {
      std::cerr << "ERROR while loading icon file '" << filename << "'" << std::endl;
      style.SetId(std::numeric_limits<size_t>::max());

      return false;
    }
  }

  void MapPainterQt::DrawLabel(const Projection& projection,
                               const MapParameter& parameter,
                               const LabelStyle& style,
                               const std::string& text,
                               double x, double y)
  {
    if (style.GetStyle()==LabelStyle::normal) {
      double fontSize=style.GetSize();
      double r=style.GetTextR();
      double g=style.GetTextG();
      double b=style.GetTextB();
      double a=style.GetTextA();

      if (projection.GetMagnification()>style.GetScaleAndFadeMag()) {
        double factor=Log2(projection.GetMagnification())-Log2(style.GetScaleAndFadeMag());
        fontSize=fontSize*pow(2,factor);
        a=a/factor;
      }

      QFont        font(GetFont(parameter,fontSize));
      QFontMetrics metrics=QFontMetrics(font);
      QString      string=QString::fromUtf8(text.c_str());
      QRect        extents=metrics.boundingRect(string);

      painter->setPen(QColor::fromRgbF(r,g,b,a));
      painter->setBrush(Qt::NoBrush);
      painter->setFont(font);
      painter->drawText(QPointF(x-extents.width()/2,
                                y+metrics.ascent()-extents.height()/2),
                        string);
    }
    else if (style.GetStyle()==LabelStyle::plate) {
      static const double outerWidth = 4;
      static const double innerWidth = 2;

      double       fontSize=style.GetSize();
      QFont        font(GetFont(parameter,fontSize));
      QFontMetrics metrics=QFontMetrics(font);
      QString      string=QString::fromUtf8(text.c_str());
      QRect        extents=metrics.boundingRect(string);

      if (x-extents.width()/2-outerWidth>=projection.GetWidth() ||
          x+extents.width()/2-outerWidth<0 ||
          y-extents.height()/2+outerWidth>=projection.GetHeight() ||
          y+extents.width()/2+outerWidth<0) {
        return;
      }

      painter->fillRect(QRectF(x-extents.width()/2-outerWidth,
                               y-metrics.height()/2-outerWidth,
                               extents.width()+2*outerWidth,
                               metrics.height()+2*outerWidth),
                              QColor::fromRgbF(style.GetBgR(),
                                               style.GetBgG(),
                                               style.GetBgB(),
                                               style.GetBgA()));

      painter->setPen(QColor::fromRgbF(style.GetBorderR(),
                                       style.GetBorderG(),
                                       style.GetBorderB(),
                                       style.GetBorderA()));

      painter->drawRect(QRect(x-extents.width()/2-innerWidth,
                              y-metrics.height()/2-innerWidth,
                              extents.width()+2*innerWidth,
                              metrics.height()+2*innerWidth));

      painter->setPen(QColor::fromRgbF(style.GetTextR(),
                                       style.GetTextG(),
                                       style.GetTextB(),
                                       style.GetTextA()));

      painter->drawText(QPointF(x-extents.width()/2-metrics.leftBearing(string[0]),
                                y-metrics.height()/2+metrics.ascent()),
                                string);
    }
    else if (style.GetStyle()==LabelStyle::emphasize) {
      double fontSize=style.GetSize();
      double r=style.GetTextR();
      double g=style.GetTextG();
      double b=style.GetTextB();
      double a=style.GetTextA();

      if (projection.GetMagnification()>style.GetScaleAndFadeMag()) {
        double factor=Log2(projection.GetMagnification())-Log2(style.GetScaleAndFadeMag());

        fontSize=fontSize*pow(2,factor);
        if (factor>=1) {
          a=a/factor;
        }
      }

      QFont        font(GetFont(parameter,fontSize));
      QFontMetrics metrics=QFontMetrics(font);
      QString      string=QString::fromUtf8(text.c_str());
      QRect        extents=metrics.boundingRect(string);
      QPainterPath path;
      QPen         pen;

      pen.setColor(QColor::fromRgbF(1.0,1.0,1.0,a));
      pen.setWidthF(2.0);
      painter->setPen(pen);

      path.addText(QPointF(x-extents.width()/2,
                           y+metrics.ascent()-extents.height()/2),
                   font,
                   string);

      painter->drawPath(path);
      painter->fillPath(path,QBrush(QColor::fromRgbF(r,g,b,a)));
    }
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
    QFont        font(GetFont(parameter,fontSize));
    QFontMetrics metrics=QFontMetrics(font);
    QString      string=QString::fromUtf8(text.c_str());
    QRect        extents=metrics.boundingRect(string);

    pen.setColor(QColor::fromRgbF(r,g,b,a));
    painter->setPen(pen);
    painter->setFont(font);

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
    assert(style->GetId()>0);
    assert(style->GetId()!=std::numeric_limits<size_t>::max());
    assert(style->GetId()<=images.size());
    assert(!images[style->GetId()-1].isNull());

    painter->drawImage(QPointF(x-images[style->GetId()-1].width()/2,
                               y-images[style->GetId()-1].height()/2),
                       images[style->GetId()-1]);
  }

  void MapPainterQt::DrawSymbol(const SymbolStyle* style, double x, double y)
  {
    QPainterPath path;

    switch (style->GetStyle()) {
    case SymbolStyle::none:
      break;
    case SymbolStyle::box:
      path.addRect(x-style->GetSize()/2,y-style->GetSize()/2,
                   style->GetSize(),style->GetSize());
      painter->fillPath(path,QBrush(QColor::fromRgbF(style->GetFillR(),
                                                     style->GetFillG(),
                                                     style->GetFillB(),
                                                     style->GetFillA())));
      break;
    case SymbolStyle::circle:
      path.addEllipse(QPointF(x,y),
                      (double)style->GetSize(),
                      (double)style->GetSize());
      painter->fillPath(path,QBrush(QColor::fromRgbF(style->GetFillR(),
                                                     style->GetFillG(),
                                                     style->GetFillB(),
                                                     style->GetFillA())));
      break;
    case SymbolStyle::triangle: {
      path.moveTo(x-style->GetSize()/2,y+style->GetSize()/2);
      path.lineTo(x,y-style->GetSize()/2);
      path.lineTo(x+style->GetSize()/2,y+style->GetSize()/2);
      path.lineTo(x-style->GetSize()/2,y+style->GetSize()/2);
      painter->fillPath(path,QBrush(QColor::fromRgbF(style->GetFillR(),
                                                     style->GetFillG(),
                                                     style->GetFillB(),
                                                     style->GetFillA())));
    }
      break;
    }
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
/*
    painter->setPen(pen);
    size_t last=0;
    bool start=true;
    for (size_t i=0; i<nodes.size(); i++) {
      if (drawNode[i]) {
        if (start) {
          start=false;
        }
        else {
          painter->drawLine(QPointF(nodeX[last],nodeY[last]),QPointF(nodeX[i],nodeY[i]));
        }

        last=i;
      }
    }*/
/*
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

    painter->strokePath(path,pen);*/

    QPolygonF polygon;
    for (size_t i=0; i<nodes.size(); i++) {
      if (drawNode[i]) {
        polygon << QPointF(nodeX[i],nodeY[i]);
      }
    }

    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawPolyline(polygon);
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
/*
    size_t last=0;
    bool start=true;
    for (size_t i=0; i<nodes.size(); i++) {
      if (drawNode[i]) {
        if (start) {
          start=false;
        }
        else {
          painter->drawLine(QPointF(nodeX[last],nodeY[last]),QPointF(nodeX[i],nodeY[i]));
        }

        last=i;
      }
    }*/
/*
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

    painter->strokePath(path,pen);*/
    QPolygonF polygon;
    for (size_t i=0; i<nodes.size(); i++) {
      if (drawNode[i]) {
        polygon << QPointF(nodeX[i],nodeY[i]);
      }
    }

    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawPolyline(polygon);

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

    QPolygonF polygon;

    for (size_t i=0; i<nodes.size(); i++) {
      if (drawNode[i]) {
        polygon << QPointF(nodeX[i],nodeY[i]);
      }
    }

    SetPen(styleConfig.GetAreaBorderStyle(type),
           borderWidth[(size_t)type]);

    if (hasPattern) {
      hasPattern=HasPattern(styleConfig,*patternStyle);
    }

    if (hasPattern) {
      painter->setBrush(patterns[patternStyle->GetId()-1]);
    }
    else if (hasFill) {
      SetBrush(fillStyle);
    }
    else {
      SetBrush();
    }

    painter->drawPolygon(polygon);
  }

  void MapPainterQt::DrawArea(const FillStyle& style,
                              const MapParameter& parameter,
                              double x,
                              double y,
                              double width,
                              double height)
  {
    painter->fillRect(QRectF(x,y,width,height),
                      QBrush(QColor::fromRgbF(style.GetFillR(),
                                              style.GetFillG(),
                                              style.GetFillB(),
                                              1)));
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

