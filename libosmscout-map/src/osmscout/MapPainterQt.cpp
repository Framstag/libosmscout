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

#include <osmscout/util/Geometry.h>

namespace osmscout {

  MapPainterQt::MapPainterQt()
  {
    sin.resize(360*10);

    for (size_t i=0; i<sin.size(); i++) {
      sin[i]=std::sin(M_PI/180*i/(sin.size()/360));
    }
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
                             const MapParameter& parameter,
                             IconStyle& style)
  {
    if (style.GetId()==std::numeric_limits<size_t>::max()) {
      return false;
    }

    if (style.GetId()!=0) {
      return true;
    }

    for (std::list<std::string>::const_iterator path=parameter.GetIconPaths().begin();
         path!=parameter.GetIconPaths().end();
         ++path) {

      std::string filename=*path+style.GetIconName()+".png";

      QImage image;

      if (image.load(filename.c_str())) {
        images.resize(images.size()+1,image);
        style.SetId(images.size());
        std::cout << "Loaded image " << filename << " => id " << style.GetId() << std::endl;

        return true;
      }
    }

    std::cerr << "ERROR while loading icon file '" << style.GetIconName() << "'" << std::endl;
    style.SetId(std::numeric_limits<size_t>::max());

    return false;
  }

  bool MapPainterQt::HasPattern(const StyleConfig& styleConfig,
                                const MapParameter& parameter,
                                PatternStyle& style)
  {
    if (style.GetId()==std::numeric_limits<size_t>::max()) {
      return false;
    }

    if (style.GetId()!=0) {
      return true;
    }

    for (std::list<std::string>::const_iterator path=parameter.GetPatternPaths().begin();
         path!=parameter.GetPatternPaths().end();
         ++path) {
      std::string filename=*path+style.GetPatternName()+".png";

      QImage image;

      if (image.load(filename.c_str())) {
        images.resize(images.size()+1,image);
        style.SetId(images.size());
        patterns.resize(images.size());

        patterns[patterns.size()-1].setTextureImage(image);

        std::cout << "Loaded image " << filename << " => id " << style.GetId() << std::endl;

        return true;
      }
    }

    std::cerr << "ERROR while loading icon file '" << style.GetPatternName() << "'" << std::endl;
    style.SetId(std::numeric_limits<size_t>::max());

    return false;
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

    QPen          pen;
    QFont         font(GetFont(parameter,fontSize));
    QFontMetricsF metrics=QFontMetricsF(font,painter->device());
    QString       string=QString::fromUtf8(text.c_str());
    double        stringLength=metrics.width(string);

    pen.setColor(QColor::fromRgbF(r,g,b,a));
    painter->setPen(pen);
    painter->setFont(font);

    QPainterPath path;

    TransformWay(projection,parameter,nodes);

    if (nodes[0].lon<nodes[nodes.size()-1].lon) {
      bool start=true;

      for (size_t j=0; j<nodes.size(); j++) {
        if (drawNode[j]) {
          if (start) {
            path.moveTo(nodeX[j],nodeY[j]);
            start=false;
          }
          else {
            path.lineTo(nodeX[j],nodeY[j]);
          }
        }
      }
    }
    else {
      bool start=true;

      for (size_t j=0; j<nodes.size(); j++) {
        if (drawNode[nodes.size()-j-1]) {
          if (start) {
            path.moveTo(nodeX[nodes.size()-j-1],
                        nodeY[nodes.size()-j-1]);
            start=false;
          }
          else {
            path.lineTo(nodeX[nodes.size()-j-1],
                        nodeY[nodes.size()-j-1]);
          }
        }
      }
    }

    if (path.length()<stringLength) {
      // Text is longer than path to draw on
      return;
    }

    qreal offset=(path.length()-stringLength)/2;

    QTransform tran;

    for (int i=0; i<string.size(); i++) {
      QPointF point=path.pointAtPercent(path.percentAtLength(offset));
      qreal angle=path.angleAtPercent(path.percentAtLength(offset));

      // rotation matrix components

      qreal sina=sin[lround((360-angle)*10)%sin.size()];
      qreal cosa=sin[lround((360-angle+90)*10)%sin.size()];

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
      tran.setMatrix(cosa,sina,0.0,
                     -sina,cosa,0.0,
                     -deltaX+deltaPenX,-deltaY-deltaPenY,1.0);

      painter->setTransform(tran);

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

  void MapPainterQt::DrawPath(const LineStyle::Style& style,
                              const Projection& projection,
                              const MapParameter& parameter,
                              double r,
                              double g,
                              double b,
                              double a,
                              double width,
                              CapStyle startCap,
                              CapStyle endCap,
                              const std::vector<Point>& nodes)
  {
    QPen pen;

    pen.setColor(QColor::fromRgbF(r,g,b,a));
    pen.setWidthF(width);
    pen.setJoinStyle(Qt::RoundJoin);

    if (startCap==capRound &&
        endCap==capRound &&
        style==LineStyle::normal) {
      pen.setCapStyle(Qt::RoundCap);
    }
    else {
      pen.setCapStyle(Qt::FlatCap);
    }

    switch (style) {
    case LineStyle::none:
      // way should not be visible in this case!
      assert(false);
      break;
    case LineStyle::normal:
      pen.setStyle(Qt::SolidLine);
      break;
    case LineStyle::longDash:
      pen.setStyle(Qt::DashLine);
      break;
    case LineStyle::dotted:
      pen.setStyle(Qt::DotLine);
      break;
    case LineStyle::lineDot:
      pen.setStyle(Qt::DashDotLine);
      break;
    }

    TransformWay(projection,parameter,nodes);

    size_t firstNode=0;
    size_t lastNode=0;

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

    QPainterPath path;

    bool start=true;
    for (size_t i=0; i<nodes.size(); i++) {
      if (drawNode[i]) {
        if (start) {
          path.moveTo(nodeX[i],nodeY[i]);
          start=false;
          firstNode=i;
          lastNode=i;
        }
        else {
          path.lineTo(nodeX[i],nodeY[i]);
          lastNode=i;
        }
      }
    }

    painter->strokePath(path,pen);
/*
    QPolygonF polygon;
    for (size_t i=0; i<nodes.size(); i++) {
      if (drawNode[i]) {
        polygon << QPointF(nodeX[i],nodeY[i]);
      }
    }

    painter->setPen(pen);
    painter->drawPolyline(polygon);*/

    if (style==LineStyle::normal &&
      startCap==capRound &&
      endCap!=capRound) {
      painter->setBrush(QBrush(QColor::fromRgbF(r,g,b,a)));

      painter->drawEllipse(QPointF(nodeX[firstNode],
                                   nodeY[firstNode]),
                                   width/2,width/2);
    }

    if (style==LineStyle::normal &&
      endCap==capRound &&
      startCap!=capRound) {
      painter->setBrush(QBrush(QColor::fromRgbF(r,g,b,a)));

      painter->drawEllipse(QPointF(nodeX[lastNode],
                                   nodeY[lastNode]),
                                   width/2,width/2);
    }
  }

  void MapPainterQt::DrawArea(const Projection& projection,
                              const MapParameter& parameter,
                              TypeId type,
                              const FillStyle& fillStyle,
                              const LineStyle* lineStyle,
                              const std::vector<Point>& nodes)
  {
    TransformArea(projection,parameter,nodes);

    QPolygonF polygon;

    for (size_t i=0; i<nodes.size(); i++) {
      if (drawNode[i]) {
        polygon << QPointF(nodeX[i],nodeY[i]);
      }
    }

    if (lineStyle!=NULL) {
      SetPen(*lineStyle,
             borderWidth[(size_t)type]);
    }
    else {
      painter->setPen(Qt::NoPen);
    }

    SetBrush(fillStyle);

    painter->drawPolygon(polygon);
  }

  void MapPainterQt::DrawArea(const Projection& projection,
                              const MapParameter& parameter,
                              TypeId type,
                              const PatternStyle& patternStyle,
                              const LineStyle* lineStyle,
                              const std::vector<Point>& nodes)
  {
    TransformArea(projection,parameter,nodes);

    QPolygonF polygon;

    for (size_t i=0; i<nodes.size(); i++) {
      if (drawNode[i]) {
        polygon << QPointF(nodeX[i],nodeY[i]);
      }
    }

    if (lineStyle!=NULL) {
      SetPen(*lineStyle,
             borderWidth[(size_t)type]);
    }
    else {
      painter->setPen(Qt::NoPen);
    }

    painter->setBrush(patterns[patternStyle.GetId()-1]);

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

  void MapPainterQt::SetPen(const LineStyle& style,
                            double lineWidth)
  {
    QPen pen;

    pen.setColor(QColor::fromRgbF(style.GetLineR(),
                                  style.GetLineG(),
                                  style.GetLineB(),
                                  style.GetLineA()));
    pen.setWidthF(lineWidth);

    switch (style.GetStyle()) {
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

  void MapPainterQt::SetBrush()
  {
    painter->setBrush(Qt::NoBrush);
  }

  void MapPainterQt::SetBrush(const FillStyle& fillStyle)
  {
    painter->setBrush(QBrush(QColor::fromRgbF(fillStyle.GetFillR(),
                                              fillStyle.GetFillG(),
                                              fillStyle.GetFillB(),
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

