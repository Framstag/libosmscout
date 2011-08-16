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

  void MapPainterQt::GetTextDimension(const MapParameter& parameter,
                                      double fontSize,
                                      const std::string& text,
                                      double& xOff,
                                      double& yOff,
                                      double& width,
                                      double& height)
  {
    QFont        font(GetFont(parameter,fontSize));
    QFontMetrics metrics=QFontMetrics(font);
    QString      string=QString::fromUtf8(text.c_str());
    QRect        extents=metrics.boundingRect(string);

    xOff=extents.x();
    yOff=extents.y();
    width=extents.width();
    height=extents.height();
  }

  void MapPainterQt::DrawLabel(const Projection& projection,
                               const MapParameter& parameter,
                               const Label& label)
  {
    double r=label.style->GetTextR();
    double g=label.style->GetTextG();
    double b=label.style->GetTextB();

    QFont        font(GetFont(parameter,label.fontSize));
    QString      string=QString::fromUtf8(label.text.c_str());
    QFontMetrics metrics=QFontMetrics(font);

    if (label.style->GetStyle()==LabelStyle::normal) {
      painter->setPen(QColor::fromRgbF(r,g,b,label.alpha));
      painter->setBrush(Qt::NoBrush);
      painter->setFont(font);
      painter->drawText(QPointF(label.x,
                                label.y+metrics.ascent()),
                        string);
    }
    else if (label.style->GetStyle()==LabelStyle::emphasize) {
      QPainterPath path;
      QPen         pen;

      pen.setColor(QColor::fromRgbF(1.0,1.0,1.0,label.alpha));
      pen.setWidthF(2.0);
      painter->setPen(pen);

      path.addText(QPointF(label.x,
                           label.y+metrics.ascent()),
                           font,
                   string);

      painter->drawPath(path);
      painter->fillPath(path,QBrush(QColor::fromRgbF(r,g,b,label.alpha)));
    }
  }

  void MapPainterQt::DrawPlateLabel(const Projection& projection,
                                    const MapParameter& parameter,
                                    const Label& label)
  {
    QFont        font(GetFont(parameter,label.fontSize));
    QFontMetrics metrics=QFontMetrics(font);
    QString      string=QString::fromUtf8(label.text.c_str());

    painter->fillRect(QRectF(label.bx,
                             label.by,
                             label.bwidth,
                             label.bheight),
                      QBrush(QColor::fromRgbF(label.style->GetBgR(),
                                              label.style->GetBgG(),
                                              label.style->GetBgB(),
                                              1)));

    painter->setPen(QColor::fromRgbF(label.style->GetBorderR(),
                                     label.style->GetBorderG(),
                                     label.style->GetBorderB(),
                                     label.style->GetBorderA()));
    painter->setBrush(Qt::NoBrush);

    painter->drawRect(QRectF(label.bx+2,
                             label.by+2,
                             label.bwidth-4,
                             label.bheight-4));

    painter->setPen(QColor::fromRgbF(label.style->GetTextR(),
                                     label.style->GetTextG(),
                                     label.style->GetTextB(),
                                     label.style->GetTextA()));
    painter->setBrush(Qt::NoBrush);
    painter->setFont(font);
    painter->drawText(QPointF(label.x,
                              label.y+metrics.ascent()),
                              string);
  }

  void MapPainterQt::DrawContourLabel(const Projection& projection,
                                      const MapParameter& parameter,
                                      const LabelStyle& style,
                                      const std::string& text,
                                      const TransPolygon& path)
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

    QPainterPath p;

    if (path.points[path.GetStart()].x<path.points[path.GetEnd()].x) {
      bool start=true;

      for (size_t j=path.GetStart(); j<=path.GetEnd(); j++) {
        if (path.points[j].draw) {
          if (start) {
            p.moveTo(path.points[j].x,path.points[j].y);
            start=false;
          }
          else {
            p.lineTo(path.points[j].x,path.points[j].y);
          }
        }
      }
    }
    else {
      bool start=true;

      for (size_t j=0; j<=path.GetEnd()-path.GetStart(); j++) {
        size_t idx=path.GetEnd()-j;

        if (path.points[idx].draw) {
          if (start) {
            p.moveTo(path.points[idx].x,
                     path.points[idx].y);
            start=false;
          }
          else {
            p.lineTo(path.points[idx].x,
                     path.points[idx].y);
          }
        }
      }
    }

    if (p.length()<stringLength) {
      // Text is longer than path to draw on
      return;
    }

    qreal offset=(p.length()-stringLength)/2;

    QTransform tran;

    for (int i=0; i<string.size(); i++) {
      QPointF point=p.pointAtPercent(p.percentAtLength(offset));
      qreal angle=p.angleAtPercent(p.percentAtLength(offset));

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

  void MapPainterQt::DrawPath(const Projection& projection,
                              const MapParameter& parameter,
                              double r,
                              double g,
                              double b,
                              double a,
                              double width,
                              const std::vector<double>& dash,
                              CapStyle startCap,
                              CapStyle endCap,
                              const TransPolygon& path)
  {
    QPen pen;

    pen.setColor(QColor::fromRgbF(r,g,b,a));
    pen.setWidthF(width);
    pen.setJoinStyle(Qt::RoundJoin);

    if (startCap==capRound &&
        endCap==capRound &&
        dash.empty()) {
      pen.setCapStyle(Qt::RoundCap);
    }
    else {
      pen.setCapStyle(Qt::FlatCap);
    }

    if (dash.empty()) {
      pen.setStyle(Qt::SolidLine);
    }
    else {
      QVector<qreal> dashes;

      for (size_t i=0; i<dash.size(); i++) {
        dashes << dash[i];
      }

      pen.setDashPattern(dashes);
    }

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

    QPainterPath p;

    bool start=true;
    for (size_t i=path.GetStart(); i<=path.GetEnd(); i++) {
      if (path.points[i].draw) {
        if (start) {
          p.moveTo(path.points[i].x,path.points[i].y);
          start=false;
        }
        else {
          p.lineTo(path.points[i].x,path.points[i].y);
        }
      }
    }

    painter->strokePath(p,pen);
/*
    QPolygonF polygon;
    for (size_t i=0; i<nodes.size(); i++) {
      if (drawNode[i]) {
        polygon << QPointF(nodeX[i],nodeY[i]);
      }
    }

    painter->setPen(pen);
    painter->drawPolyline(polygon);*/

    if (dash.empty() &&
        startCap==capRound &&
        endCap!=capRound) {
      painter->setBrush(QBrush(QColor::fromRgbF(r,g,b,a)));

      painter->drawEllipse(QPointF(path.points[path.GetStart()].x,
                                   path.points[path.GetStart()].y),
                                   width/2,width/2);
    }

    if (dash.empty() &&
      endCap==capRound &&
      startCap!=capRound) {
      painter->setBrush(QBrush(QColor::fromRgbF(r,g,b,a)));

      painter->drawEllipse(QPointF(path.points[path.GetEnd()].x,
                                   path.points[path.GetEnd()].y),
                                   width/2,width/2);
    }
  }

  void MapPainterQt::DrawArea(const Projection& projection,
                              const MapParameter& parameter,
                              TypeId type,
                              const FillStyle& fillStyle,
                              const LineStyle* lineStyle,
                              const TransPolygon& area)
  {
    QPolygonF polygon;

    for (size_t i=area.GetStart(); i<=area.GetEnd(); i++) {
      if (area.points[i].draw) {
        polygon << QPointF(area.points[i].x,area.points[i].y);
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
                              const TransPolygon& area)
  {
    QPolygonF polygon;

    for (size_t i=area.GetStart(); i<=area.GetEnd(); i++) {
      if (area.points[i].draw) {
        polygon << QPointF(area.points[i].x,area.points[i].y);
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

    if (style.GetDash().empty()) {
      pen.setStyle(Qt::SolidLine);
      pen.setCapStyle(Qt::RoundCap);
    }
    else {
      QVector<qreal> dashes;

      for (size_t i=0; i<style.GetDash().size(); i++) {
        dashes << style.GetDash()[i];
      }

      pen.setDashPattern(dashes);
      pen.setCapStyle(Qt::FlatCap);
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

