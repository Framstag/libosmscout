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
#include <osmscout/SimplifiedPath.h>

#include <iostream>
#include <limits>

#include <QPainterPath>
#include <QTextLayout>
#include <QDebug>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

#include <osmscout/util/File.h>
#include <osmscout/util/Geometry.h>
#include <osmscout/util/Logger.h>

namespace osmscout {

  MapPainterQt::MapPainterQt(const StyleConfigRef& styleConfig)
  : MapPainter(styleConfig,
               new CoordBufferImpl<Vertex2D>()),
    coordBuffer((CoordBufferImpl<Vertex2D>*)transBuffer.buffer),
    painter(NULL)
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

  QFont MapPainterQt::GetFont(const Projection& projection,
                              const MapParameter& parameter,
                              double fontSize)
  {
    std::map<size_t,QFont>::const_iterator f;

    fontSize=fontSize*projection.ConvertWidthToPixel(parameter.GetFontSize());

    f=fonts.find(fontSize);

    if (f!=fonts.end()) {
      return f->second;
    }

    QFont font(parameter.GetFontName().c_str(),QFont::Normal,false);

    font.setPixelSize(fontSize);
    font.setStyleStrategy(QFont::PreferAntialias);
    font.setStyleStrategy(QFont::PreferMatch);

    return fonts.insert(std::pair<size_t,QFont>(fontSize,font)).first->second;
  }

  bool MapPainterQt::HasIcon(const StyleConfig& /*styleConfig*/,
                             const MapParameter& parameter,
                             IconStyle& style)
  {
    if (style.GetIconId()==0) {
      return false;
    }

    size_t idx=style.GetIconId()-1;

    if (idx<images.size() &&
        !images[idx].isNull()) {
      return true;
    }

    std::list<std::string> erronousPaths;

    for (const auto& path : parameter.GetIconPaths()) {
      std::string filename=AppendFileToDir(path,style.GetIconName()+".png");

      QImage image;

      if (image.load(filename.c_str())) {
        if (idx>=images.size()) {
          images.resize(idx+1);
        }

        images[idx]=image;

        //std::cout << "Loaded image '" << filename << "'" << std::endl;

        return true;
      }

      erronousPaths.push_back(filename);
    }

    log.Warn() << "Cannot find icon '" << style.GetIconName() << "'";

    for (const auto& path : erronousPaths) {
      log.Warn() <<  "Search path '" << path << "'";
    }

    style.SetIconId(0);

    return false;
  }

  bool MapPainterQt::HasPattern(const MapParameter& parameter,
                                const FillStyle& style)
  {
    assert(style.HasPattern());

    // Was not able to load pattern
    if (style.GetPatternId()==0) {
      return false;
    }

    size_t idx=style.GetPatternId()-1;

    if (idx<patterns.size() &&
        !patternImages[idx].isNull()) {
      return true;
    }

    std::list<std::string> erronousPaths;

    for (const auto& path : parameter.GetPatternPaths()) {
      std::string filename=AppendFileToDir(path,style.GetPatternName()+".png");

      QImage image;

      if (image.load(filename.c_str())) {
        if (idx>=patternImages.size()) {
          patternImages.resize(idx+1);
        }

        patternImages[idx]=image;

        if (idx>=patterns.size()) {
          patterns.resize(idx+1);
        }

        patterns[idx].setTextureImage(image);

        //std::cout << "Loaded image '" << filename << "'" << std::endl;

        return true;
      }

      erronousPaths.push_back(filename);
    }

    log.Warn() << "Cannot find pattern '" << style.GetPatternName() << "'";

    for (const auto& path : erronousPaths) {
      log.Warn() <<  "Search path '" << path << "'";
    }

    style.SetPatternId(0);

    return false;
  }

  void MapPainterQt::GetFontHeight(const Projection& projection,
                                   const MapParameter& parameter,
                                   double fontSize,
                                   double& height)
  {
    QFont        font(GetFont(projection,
                              parameter,
                              fontSize));
    QFontMetrics metrics=QFontMetrics(font);

    height=metrics.height();
  }

  void MapPainterQt::GetTextDimension(const Projection& projection,
                                      const MapParameter& parameter,
                                      double fontSize,
                                      const std::string& text,
                                      double& xOff,
                                      double& yOff,
                                      double& width,
                                      double& height)
  {
    QFont        font(GetFont(projection,
                              parameter,
                              fontSize));
    QFontMetrics fontMetrics=QFontMetrics(font);
    QString      string=QString::fromUtf8(text.c_str());
    QTextLayout  textLayout(string,font);
    qreal        proposedWidth=parameter.GetLabelLineCharCount()*fontMetrics.averageCharWidth();
    qreal        leading=fontMetrics.leading() ;

    width=0;
    height=0;

    textLayout.beginLayout();
    while (true) {
      QTextLine line=textLayout.createLine();
      if (!line.isValid())
        break;

      line.setLineWidth(proposedWidth);
      height+=leading;
      line.setPosition(QPointF(0.0,height));
      width=std::max(width,(double)line.naturalTextWidth());
      height+=line.height();
    }
    textLayout.endLayout();

    QRectF boundingBox=textLayout.boundingRect();

    xOff=boundingBox.x();
    yOff=boundingBox.y();
  }

  void LayoutTextLayout(const MapParameter& parameter,
                        const QFontMetrics& fontMetrics,
                        QTextLayout& layout,
                        QRectF& boundingBox)
  {
    qreal width=0;
    qreal height=0;
    qreal proposedWidth=parameter.GetLabelLineCharCount()*fontMetrics.averageCharWidth();
    qreal leading=fontMetrics.leading();

    // Calculate and layout all lines initial left aligned

    layout.beginLayout();
    while (true) {
      QTextLine line = layout.createLine();
      if (!line.isValid())
        break;

      line.setLineWidth(proposedWidth);
      height+=leading;
      line.setPosition(QPointF(0.0,height));
      width=std::max(width,line.naturalTextWidth());
      height+=line.height();
    }
    layout.endLayout();

    boundingBox=layout.boundingRect();

    boundingBox.setWidth(width);
    boundingBox.setHeight(height);

    // Center all lines horizontally, after we know the actual width

    for (int i=0; i<layout.lineCount(); i++) {
      QTextLine line = layout.lineAt(i);

      line.setPosition(QPointF((width-line.naturalTextWidth())/2,line.position().y()));
    }
  }

  void MapPainterQt::DrawLabel(const Projection& projection,
                               const MapParameter& parameter,
                               const LabelData& label)
  {
    QFont        font(GetFont(projection,
                              parameter,
                              label.fontSize));
    QString      string=QString::fromUtf8(label.text.c_str());
    QFontMetrics fontMetrics=QFontMetrics(font);
    QTextLayout  textLayout(string,font);

    textLayout.setCacheEnabled(true);

    if (dynamic_cast<const TextStyle*>(label.style.get())!=NULL) {
      const TextStyle* style=dynamic_cast<const TextStyle*>(label.style.get());
      double           r=style->GetTextColor().GetR();
      double           g=style->GetTextColor().GetG();
      double           b=style->GetTextColor().GetB();

      if (style->GetStyle()==TextStyle::normal) {
        QColor                          textColor=QColor::fromRgbF(r,g,b,label.alpha);
        QRectF                          boundingBox;
        QList<QTextLayout::FormatRange> formatList;
        QTextLayout::FormatRange        range;

        range.start=0;
        range.length=string.length();
        range.format.setForeground(QBrush(textColor));
        formatList.append(range);

        textLayout.setAdditionalFormats(formatList);

        LayoutTextLayout(parameter,
                         fontMetrics,
                         textLayout,
                         boundingBox);

        textLayout.draw(painter,QPointF(label.x+boundingBox.x(),
                                        label.y+boundingBox.y()));
      }
      else if (style->GetStyle()==TextStyle::emphasize) {
        QRectF                          boundingBox;
        QColor                          textColor=QColor::fromRgbF(r,g,b,label.alpha);
        QColor                          outlineColor=QColor::fromRgbF(1.0,1.0,1.0,label.alpha);
        QPen                            outlinePen(outlineColor,2.0,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin);
        QList<QTextLayout::FormatRange> formatList;
        QTextLayout::FormatRange        range;

        range.start=0;
        range.length=string.length();
        range.format.setForeground(QBrush(outlineColor));
        range.format.setTextOutline(outlinePen);
        formatList.append(range);

        textLayout.setAdditionalFormats(formatList);

        LayoutTextLayout(parameter,
                         fontMetrics,
                         textLayout,
                         boundingBox);

        textLayout.draw(painter,
                        QPointF(label.x+boundingBox.x(),
                                label.y+boundingBox.y()));

        range.start=0;
        range.length=string.length();
        range.format.setForeground(QBrush(textColor));
        range.format.setTextOutline(QPen(Qt::transparent));
        formatList.clear();
        formatList.append(range);

        textLayout.setAdditionalFormats(formatList);

        LayoutTextLayout(parameter,
                         fontMetrics,
                         textLayout,
                         boundingBox);

        textLayout.draw(painter,
                        QPointF(label.x+boundingBox.x(),
                                label.y+boundingBox.y()));
      }
    }
    else if (dynamic_cast<const ShieldStyle*>(label.style.get())!=NULL) {
      const ShieldStyle *style=dynamic_cast<const ShieldStyle*>(label.style.get());
      QColor             textColor=QColor::fromRgbF(style->GetTextColor().GetR(),
                                                    style->GetTextColor().GetG(),
                                                    style->GetTextColor().GetB(),
                                                    style->GetTextColor().GetA());
      QRectF             boundingBox;

      // Shield background
      painter->fillRect(QRectF(label.bx1,
                               label.by1,
                               label.bx2-label.bx1+1,
                               label.by2-label.by1+1),
                        QBrush(QColor::fromRgbF(style->GetBgColor().GetR(),
                                                style->GetBgColor().GetG(),
                                                style->GetBgColor().GetB(),
                                                1)));

      // Shield border
      painter->setPen(QColor::fromRgbF(style->GetBorderColor().GetR(),
                                       style->GetBorderColor().GetG(),
                                       style->GetBorderColor().GetB(),
                                       style->GetBorderColor().GetA()));
      painter->setBrush(Qt::NoBrush);

      painter->drawRect(QRectF(label.bx1+2,
                               label.by1+2,
                               label.bx2-label.bx1+1-4,
                               label.by2-label.by1+1-4));

      QList<QTextLayout::FormatRange> formatList;
      QTextLayout::FormatRange range;

      range.start=0;
      range.length=string.length();
      range.format.setForeground(QBrush(textColor));
      range.format.setTextOutline(Qt::NoPen);
      formatList.clear();
      formatList.append(range);

      textLayout.setAdditionalFormats(formatList);

      LayoutTextLayout(parameter,
                       fontMetrics,
                       textLayout,
                       boundingBox);

      textLayout.draw(painter,
                      QPointF(label.x+boundingBox.x(),
                              label.y+boundingBox.y()));
    }
  }

  void MapPainterQt::setupTransformation(QPainter *painter,
                                         const QPointF center,
                                         const qreal angle,
                                         const qreal baseline) const
  {
    QTransform tran;
    qreal penWidth = painter->pen().widthF();

    // rotation matrix components
    qreal sina=sin[lround((360-angle)*10)%sin.size()];
    qreal cosa=sin[lround((360-angle+90)*10)%sin.size()];

    // Rotation
    qreal newX=(cosa*center.x())-(sina*(center.y()-baseline));
    qreal newY=(cosa*(center.y()-baseline))+(sina*center.x());

    // Aditional offseting
    qreal deltaPenX=cosa*penWidth;
    qreal deltaPenY=sina*penWidth;

    // Getting the delta distance for the translation part of the transformation
    qreal deltaX=newX-center.x();
    qreal deltaY=newY-center.y();

    // Applying rotation and translation.
    tran.setMatrix(cosa,sina,0.0,
                   -sina,cosa,0.0,
                   -deltaX+deltaPenX,-deltaY-deltaPenY,1.0);
    painter->setTransform(tran);
  }

  void MapPainterQt::DrawContourLabel(const Projection& projection,
                                      const MapParameter& parameter,
                                      const PathTextStyle& style,
                                      const std::string& text,
                                      size_t transStart, size_t transEnd)
  {
    double fontSize=style.GetSize();
    double r=style.GetTextColor().GetR();
    double g=style.GetTextColor().GetG();
    double b=style.GetTextColor().GetB();
    double a=style.GetTextColor().GetA();

    QPen          pen;
    QFont         font(GetFont(projection,
                               parameter,
                               fontSize));
    QString       string=QString::fromUtf8(text.c_str());

    QTextLayout textLayout(string,font,painter->device());
    // evaluate layout
    textLayout.beginLayout();
    while (textLayout.createLine().isValid()){};
    textLayout.endLayout();
    int fontPixelSize=font.pixelSize();

    QList<QGlyphRun> glyphs=textLayout.glyphRuns();
    double stringWidth=textLayout.boundingRect().width();

    pen.setColor(QColor::fromRgbF(r,g,b,a));
    painter->setPen(pen);
    painter->setFont(font);

    // build path
    SimplifiedPath p;
    if (coordBuffer->buffer[transStart].GetX()<coordBuffer->buffer[transEnd].GetX()) {
      for (size_t j=transStart; j<=transEnd; j++) {
        p.AddPoint(coordBuffer->buffer[j].GetX(),
                   coordBuffer->buffer[j].GetY());
      }
    }
    else {
      for (size_t j=0; j<=transEnd-transStart; j++) {
        size_t idx=transEnd-j;
        p.AddPoint(coordBuffer->buffer[idx].GetX(),
                   coordBuffer->buffer[idx].GetY());
      }
    }

    qreal pLength=p.GetLength();
    qreal spaceLeft=pLength-stringWidth-2*contourLabelOffset;

    // If space around labels left is < offset on both sides, do not render at all
    if (spaceLeft<0.0) {
      return;
    }

    spaceLeft=fmod(spaceLeft,stringWidth+contourLabelSpace);

    qreal labelInstanceOffset=spaceLeft/2+contourLabelOffset;
    qreal offset=labelInstanceOffset;

    QVector<quint32> indexes(1);
    QVector<QPointF> positions(1);
    while (offset<pLength) {

      // skip string rendering when path is too much squiggly at this offset
      if (!p.TestAngleVariance(offset,offset+stringWidth,M_PI/4)){
        offset+=stringWidth+contourLabelSpace;
        continue;
      }

      qreal initialAngle=std::abs(p.AngleAtLengthDeg(offset));
      bool upwards=initialAngle>90 && initialAngle<270;

      // draw glyphs
      for (const QGlyphRun &glypRun: glyphs){
        for (int g=0; g<glypRun.glyphIndexes().size(); g++){
          auto index=glypRun.glyphIndexes().at(g);
          auto pos=glypRun.positions().at(g);
          indexes[0]=index;
          positions[0]=QPointF(0,pos.y());
          QRectF boundingRect=glypRun.rawFont().boundingRect(index);

          qreal glyphOffset=upwards? (offset+stringWidth-pos.x()) : offset+pos.x();
          if (glyphOffset>pLength)
            continue;

          QPointF point=p.PointAtLength(glyphOffset);
          // check if current glyph can be visible
          qreal diagonal=boundingRect.width()+boundingRect.height(); // it is little bit longer than correct sqrt(w^2+h^2)
          if (!painter->viewport().intersects(QRect(point.x()-diagonal, point.y()-diagonal,
                                                    point.x()+diagonal, point.y()+diagonal))){
            continue;
          }
          qreal angle=p.AngleAtLengthDeg(glyphOffset);
          if (upwards){
            angle-=180;
          }

          setupTransformation(painter, point, angle, fontPixelSize*-0.7);

          QGlyphRun orphanGlyph;
          orphanGlyph.setBoundingRect(boundingRect);
          orphanGlyph.setFlags(glypRun.flags());
          orphanGlyph.setGlyphIndexes(indexes);
          orphanGlyph.setOverline(glypRun.overline());
          orphanGlyph.setPositions(positions);
          orphanGlyph.setRawFont(glypRun.rawFont());
          orphanGlyph.setRightToLeft(glypRun.isRightToLeft());
          orphanGlyph.setStrikeOut(glypRun.strikeOut());
          orphanGlyph.setUnderline(glypRun.underline());

          painter->drawGlyphRun(point, orphanGlyph);
        }
      }

      offset+=stringWidth+contourLabelSpace;
    }

    painter->resetTransform();
  }


  void MapPainterQt::FollowPathInit(FollowPathHandle &hnd,
                                    Vertex2D &origin,
                                    size_t transStart,
                                    size_t transEnd,
                                    bool isClosed,
                                    bool keepOrientation)
  {
    hnd.i=0;
    hnd.nVertex=transEnd >= transStart ? transEnd - transStart : transStart-transEnd;
    bool isReallyClosed=(coordBuffer->buffer[transStart]==coordBuffer->buffer[transEnd]);

    if (isClosed && !isReallyClosed) {
      hnd.nVertex++;
      hnd.closeWay=true;
    }
    else {
      hnd.closeWay=false;
    }

    if (keepOrientation ||
        coordBuffer->buffer[transStart].GetX()<coordBuffer->buffer[transEnd].GetX()) {
      hnd.transStart=transStart;
      hnd.transEnd=transEnd;
    }
    else {
      hnd.transStart=transEnd;
      hnd.transEnd=transStart;
    }

    hnd.direction=(hnd.transStart < hnd.transEnd) ? 1 : -1;
    origin.Set(coordBuffer->buffer[hnd.transStart].GetX(), coordBuffer->buffer[hnd.transStart].GetY());
  }

  bool MapPainterQt::FollowPath(FollowPathHandle &hnd,
                                double l,
                                Vertex2D &origin)
  {
    double x=origin.GetX();
    double y=origin.GetY();
    double x2,y2;
    double deltaX,deltaY,len,fracToGo;

    while (hnd.i<hnd.nVertex) {
      if (hnd.closeWay && hnd.nVertex-hnd.i==1) {
        x2=coordBuffer->buffer[hnd.transStart].GetX();
        y2=coordBuffer->buffer[hnd.transStart].GetY();
      }
      else {
        x2=coordBuffer->buffer[hnd.transStart+(hnd.i+1)*hnd.direction].GetX();
        y2=coordBuffer->buffer[hnd.transStart+(hnd.i+1)*hnd.direction].GetY();
      }
      deltaX=(x2-x);
      deltaY=(y2-y);
      len=sqrt(deltaX*deltaX + deltaY*deltaY);

      fracToGo=l/len;
      if (fracToGo<=1.0) {
        origin.Set(x + deltaX*fracToGo,y + deltaY*fracToGo);
        return true;
      }

      //advance to next point on the path
      l-=len;
      x=x2;
      y=y2;
      hnd.i++;
    }

    return false;
  }

  void MapPainterQt::DrawContourSymbol(const Projection& projection,
                                       const MapParameter& parameter,
                                       const Symbol& symbol,
                                       double space,
                                       size_t transStart,
                                       size_t transEnd)
  {
    double           minX;
    double           minY;
    double           maxX;
    double           maxY;

    symbol.GetBoundingBox(minX,minY,maxX,maxY);

    double           widthPx=projection.ConvertWidthToPixel(maxX-minX);
    double           height=maxY-minY;
    bool             isClosed=false;
    Vertex2D         origin;
    double           x1,y1,x2,y2,x3,y3,slope;
    FollowPathHandle followPathHnd;

    FollowPathInit(followPathHnd,
                   origin,
                   transStart,
                   transEnd,
                   isClosed,
                   true);

    if (!isClosed &&
        !FollowPath(followPathHnd,space/2,origin)) {
      return;
    }

    QTransform savedTransform=painter->transform();
    QTransform t;
    bool       loop=true;

    while (loop) {
      x1=origin.GetX();
      y1=origin.GetY();
      loop=FollowPath(followPathHnd, widthPx/2, origin);

      if (loop) {
        x2=origin.GetX();
        y2=origin.GetY();
        loop=FollowPath(followPathHnd, widthPx/2, origin);

        if (loop) {
          x3=origin.GetX();
          y3=origin.GetY();
          slope=atan2(y3-y1,x3-x1);
          t=QTransform::fromTranslate(x2, y2);
          t.rotateRadians(slope);
          painter->setTransform(t);
          DrawSymbol(projection, parameter, symbol, 0, -height*2);
          loop=FollowPath(followPathHnd, space, origin);
        }
      }
    }
    painter->setTransform(savedTransform);
  }

  void MapPainterQt::DrawIcon(const IconStyle* style,
                              double x, double y)
  {
    size_t idx=style->GetIconId()-1;

    assert(idx<images.size());
    assert(!images[idx].isNull());

    painter->drawImage(QPointF(x-images[idx].width()/2,
                               y-images[idx].height()/2),
                       images[idx]);
  }

  void MapPainterQt::DrawSymbol(const Projection& projection,
                                const MapParameter& parameter,
                                const Symbol& symbol,
                                double x, double y)
  {
    double minX;
    double minY;
    double maxX;
    double maxY;
    double centerX;
    double centerY;

    symbol.GetBoundingBox(minX,minY,maxX,maxY);

    centerX=(minX+maxX)/2;
    centerY=(minY+maxY)/2;

    for (const auto& primitive : symbol.GetPrimitives()) {
      const DrawPrimitive *primitivePtr=primitive.get();

      if (dynamic_cast<const PolygonPrimitive*>(primitivePtr)!=NULL) {
        const PolygonPrimitive *polygon=dynamic_cast<const PolygonPrimitive*>(primitivePtr);
        FillStyleRef           style=polygon->GetFillStyle();

        SetFill(projection,
                parameter,
                *style);

        QPainterPath path;

        for (std::list<Vertex2D>::const_iterator pixel=polygon->GetCoords().begin();
             pixel!=polygon->GetCoords().end();
             ++pixel) {
          if (pixel==polygon->GetCoords().begin()) {
            path.moveTo(x+projection.ConvertWidthToPixel(pixel->GetX()-centerX),
                        y+projection.ConvertWidthToPixel(maxY-pixel->GetY()-centerY));
          }
          else {
            path.lineTo(x+projection.ConvertWidthToPixel(pixel->GetX()-centerX),
                        y+projection.ConvertWidthToPixel(maxY-pixel->GetY()-centerY));
          }
        }

        painter->drawPath(path);
      }
      else if (dynamic_cast<const RectanglePrimitive*>(primitivePtr)!=NULL) {
        const RectanglePrimitive *rectangle=dynamic_cast<const RectanglePrimitive*>(primitivePtr);
        FillStyleRef             style=rectangle->GetFillStyle();

        SetFill(projection,
                parameter,
                *style);

        QPainterPath path;

        path.addRect(x+projection.ConvertWidthToPixel(rectangle->GetTopLeft().GetX()-centerX),
                     y+projection.ConvertWidthToPixel(maxY-rectangle->GetTopLeft().GetY()-centerY),
                     projection.ConvertWidthToPixel(rectangle->GetWidth()),
                     projection.ConvertWidthToPixel(rectangle->GetHeight()));

        painter->drawPath(path);
      }
      else if (dynamic_cast<const CirclePrimitive*>(primitivePtr)!=NULL) {
        const CirclePrimitive *circle=dynamic_cast<const CirclePrimitive*>(primitivePtr);
        FillStyleRef          style=circle->GetFillStyle();

        QPointF center(x+projection.ConvertWidthToPixel(circle->GetCenter().GetX()-centerX),
                       y+projection.ConvertWidthToPixel(maxY-circle->GetCenter().GetY()-centerY));

        double  radius=projection.ConvertWidthToPixel(circle->GetRadius());

        /*
        std::cout << "Circle: " << x << "," << y << " " << circle->GetCenter().x << "," << circle->GetCenter().y << " " << centerX << "," << centerY << " " << center.x() << "," << center.y() << std::endl;

        radius=circle->GetRadius()*projection.GetMeterInPixel();*/

        SetFill(projection,
                parameter,
                *style);

        /*
        QRadialGradient grad(center, radius);
        grad.setColorAt(0, QColor::fromRgbF(style->GetFillColor().GetR(),
                                            style->GetFillColor().GetG(),
                                            style->GetFillColor().GetB(),
                                            style->GetFillColor().GetA()));
        grad.setColorAt(1, QColor(0, 0, 0, 0));
        QBrush g_brush(grad); // Gradient QBrush
        painter->setBrush(g_brush);*/

        QPainterPath path;

        path.addEllipse(center,
                        radius,
                        radius);

        painter->drawPath(path);
      }
    }
  }

  void MapPainterQt::DrawPath(const Projection& /*projection*/,
                              const MapParameter& /*parameter*/,
                              const Color& color,
                              double width,
                              const std::vector<double>& dash,
                              LineStyle::CapStyle startCap,
                              LineStyle::CapStyle endCap,
                              size_t transStart, size_t transEnd)
  {
    QPen pen;

    pen.setColor(QColor::fromRgbF(color.GetR(),
                                  color.GetG(),
                                  color.GetB(),
                                  color.GetA()));
    pen.setWidthF(width);
    pen.setJoinStyle(Qt::RoundJoin);

   if (startCap==LineStyle::capButt ||
       endCap==LineStyle::capButt) {
      pen.setCapStyle(Qt::FlatCap);
    }
    else if (startCap==LineStyle::capSquare ||
             endCap==LineStyle::capSquare) {
      pen.setCapStyle(Qt::SquareCap);
    }
    else {
      pen.setCapStyle(Qt::RoundCap);
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

    p.moveTo(coordBuffer->buffer[transStart].GetX(),
             coordBuffer->buffer[transStart].GetY());
    for (size_t i=transStart+1; i<=transEnd; i++) {
      p.lineTo(coordBuffer->buffer[i].GetX(),
               coordBuffer->buffer[i].GetY());
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
        startCap==LineStyle::capRound &&
        endCap!=LineStyle::capRound) {
      painter->setBrush(QBrush(QColor::fromRgbF(color.GetR(),
                                                color.GetG(),
                                                color.GetB(),
                                                color.GetA())));

      painter->drawEllipse(QPointF(coordBuffer->buffer[transStart].GetX(),
                                   coordBuffer->buffer[transStart].GetY()),
                                   width/2,width/2);
    }

    if (dash.empty() &&
      endCap==LineStyle::capRound &&
      startCap!=LineStyle::capRound) {
      painter->setBrush(QBrush(QColor::fromRgbF(color.GetR(),
                                                color.GetG(),
                                                color.GetB(),
                                                color.GetA())));

      painter->drawEllipse(QPointF(coordBuffer->buffer[transEnd].GetX(),
                                   coordBuffer->buffer[transEnd].GetY()),
                                   width/2,width/2);
    }
  }

  void MapPainterQt::DrawArea(const Projection& projection,
                              const MapParameter& parameter,
                              const MapPainter::AreaData& area)
  {
    QPainterPath path;

    path.moveTo(coordBuffer->buffer[area.transStart].GetX(),
                coordBuffer->buffer[area.transStart].GetY());
    for (size_t i=area.transStart+1; i<=area.transEnd; i++) {
      path.lineTo(coordBuffer->buffer[i].GetX(),
                  coordBuffer->buffer[i].GetY());
    }
    path.closeSubpath();

    if (!area.clippings.empty()) {
      for (std::list<PolyData>::const_iterator c=area.clippings.begin();
          c!=area.clippings.end();
          c++) {
        const PolyData& data=*c;

        path.moveTo(coordBuffer->buffer[data.transStart].GetX(),
                    coordBuffer->buffer[data.transStart].GetY());

        for (size_t i=data.transStart+1; i<=data.transEnd; i++) {
          path.lineTo(coordBuffer->buffer[i].GetX(),
                      coordBuffer->buffer[i].GetY());
        }

        path.closeSubpath();
      }
    }

    SetFill(projection,
            parameter,
            *area.fillStyle);
    bool restoreTransform = false;
    size_t idx = -1;
    if (area.fillStyle->HasPattern()) {
      idx=area.fillStyle->GetPatternId()-1;

      if (idx<patterns.size() && !patterns[idx].textureImage().isNull()) {
        patterns[idx].setTransform(QTransform::fromTranslate(
                                          remainder(coordBuffer->buffer[area.transStart].GetX(),patterns[idx].textureImage().width()),
                                          remainder(coordBuffer->buffer[area.transStart].GetY(),patterns[idx].textureImage().height())));
        painter->setBrush(patterns[idx]);
        restoreTransform = true;
      }
    }

    painter->drawPath(path);

    if (restoreTransform) {
      patterns[idx].setTransform(QTransform(1.0,0.0,1.0,0.0,0.0,0.0));
    }
  }

  void MapPainterQt::DrawGround(const Projection& projection,
                                const MapParameter& /*parameter*/,
                                const FillStyle& style)
  {
    painter->fillRect(QRectF(0,0,projection.GetWidth(),projection.GetHeight()),
                      QBrush(QColor::fromRgbF(style.GetFillColor().GetR(),
                                              style.GetFillColor().GetG(),
                                              style.GetFillColor().GetB(),
                                              1)));
  }

  void MapPainterQt::SetPen(const LineStyle& style,
                            double lineWidth)
  {
    QPen pen;

    pen.setColor(QColor::fromRgbF(style.GetLineColor().GetR(),
                                  style.GetLineColor().GetG(),
                                  style.GetLineColor().GetB(),
                                  style.GetLineColor().GetA()));
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

  void MapPainterQt::SetFill(const Projection& projection,
                             const MapParameter& parameter,
                             const FillStyle& fillStyle)
  {
    double borderWidth=projection.ConvertWidthToPixel(fillStyle.GetBorderWidth());

    if (fillStyle.HasPattern() &&
        projection.GetMagnification()>=fillStyle.GetPatternMinMag() &&
        HasPattern(parameter,fillStyle)) {
      size_t idx=fillStyle.GetPatternId()-1;

      painter->setBrush(patterns[idx]);
    }
    else if (fillStyle.GetFillColor().IsVisible()) {
      painter->setBrush(QBrush(QColor::fromRgbF(fillStyle.GetFillColor().GetR(),
                                                fillStyle.GetFillColor().GetG(),
                                                fillStyle.GetFillColor().GetB(),
                                                fillStyle.GetFillColor().GetA())));
    }
    else {
      painter->setBrush(Qt::NoBrush);
    }

    if (borderWidth>=parameter.GetLineMinWidthPixel()) {
      QPen pen;

      pen.setColor(QColor::fromRgbF(fillStyle.GetBorderColor().GetR(),
                                    fillStyle.GetBorderColor().GetG(),
                                    fillStyle.GetBorderColor().GetB(),
                                    fillStyle.GetBorderColor().GetA()));
      pen.setWidthF(borderWidth);

      if (fillStyle.GetBorderDash().empty()) {
        pen.setStyle(Qt::SolidLine);
        pen.setCapStyle(Qt::RoundCap);
      }
      else {
        QVector<qreal> dashes;

        for (size_t i=0; i<fillStyle.GetBorderDash().size(); i++) {
          dashes << fillStyle.GetBorderDash()[i];
        }

        pen.setDashPattern(dashes);
        pen.setCapStyle(Qt::FlatCap);
      }

      painter->setPen(pen);
    }
    else {
      painter->setPen(Qt::NoPen);
    }

  }

  bool MapPainterQt::DrawMap(const Projection& projection,
                             const MapParameter& parameter,
                             const MapData& data,
                             QPainter* painter)
  {
    std::lock_guard<std::mutex> guard(mutex);

    this->painter=painter;

    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::TextAntialiasing);

    return Draw(projection,
                parameter,
                data);
  }
}

