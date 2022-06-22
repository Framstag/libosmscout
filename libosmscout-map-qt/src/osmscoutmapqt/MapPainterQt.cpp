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

#include <osmscoutmapqt/MapPainterQt.h>
#include <osmscoutmap/LabelPath.h>

#include <iostream>
#include <limits>

#include <QPainterPath>
#include <QTextLayout>
#include <QDebug>
#include <QtSvg/QSvgRenderer>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

#include <osmscout/util/File.h>
#include <osmscout/util/Geometry.h>
#include <osmscout/util/Logger.h>
#include <osmscoutmapqt/SymbolRendererQt.h>

namespace osmscout {

  MapPainterQt::MapPainterQt(const StyleConfigRef& styleConfig)
  : MapPainter(styleConfig),
    painter(nullptr),
    labelLayouter(this)
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
    FontDescriptor descriptor;
    descriptor.fontName=QString::fromStdString(parameter.GetFontName());
    descriptor.fontSize=fontSize*projection.ConvertWidthToPixel(parameter.GetFontSize());
    descriptor.weight=QFont::Normal;
    descriptor.italic=false;

    if (fonts.contains(descriptor)) {
      return fonts.value(descriptor);
    }

    QFont font(descriptor.fontName.toStdString().c_str(),
               descriptor.weight,
               descriptor.italic);

    font.setPixelSize(descriptor.fontSize);
    font.setStyleStrategy(static_cast<QFont::StyleStrategy>(QFont::PreferAntialias | QFont::PreferMatch));

    fonts[descriptor]=font;
    return font;
  }

  bool MapPainterQt::HasIcon(const StyleConfig& /*styleConfig*/,
                             const Projection& projection,
                             const MapParameter& parameter,
                             IconStyle& style)
  {
    if (style.GetIconId()==0) {
      return false;
    }

    const auto &it=images.find(style.GetIconName());

    // there is possible that exists multiple IconStyle instances with same iconId (point and area icon with same icon name)
    // setup dimensions for all of them
    // std::cout << style.GetIconId() << ": " << style.GetIconName() << " @ " << &style << std::endl;
    if (parameter.GetIconMode()==MapParameter::IconMode::Scalable ||
        parameter.GetIconMode()==MapParameter::IconMode::ScaledPixmap){

      style.SetWidth(std::round(projection.ConvertWidthToPixel(parameter.GetIconSize())));
      style.SetHeight(style.GetWidth());
    }else{
      style.SetWidth(std::round(parameter.GetIconPixelSize()));
      style.SetHeight(style.GetWidth());
    }

    if (it!=images.end() &&
        !it->second.isNull()) {

      if (parameter.GetIconMode()==MapParameter::IconMode::OriginalPixmap){
        style.SetWidth(it->second.width());
        style.SetHeight(it->second.height());
      }
      return true;
    }

    std::list<std::string> erronousPaths;

    for (const auto& path : parameter.GetIconPaths()) {
      QImage image;
      std::string filename;
      bool success=false;
      if (parameter.GetIconMode()==MapParameter::IconMode::Scalable){
        filename=AppendFileToDir(path,style.GetIconName()+".svg");

        // Load SVG
        QSvgRenderer renderer(QString::fromStdString(filename));
        if (renderer.isValid()) {
          image = QImage(style.GetWidth(), style.GetHeight(), QImage::Format_ARGB32);
          image.fill(Qt::transparent);

          QPainter painter(&image);
          renderer.render(&painter);
          painter.end();
          success = !image.isNull();
        }
      }else{
        filename=AppendFileToDir(path,style.GetIconName()+".png");
        if (image.load(filename.c_str())) {
          if (parameter.GetIconMode()==MapParameter::IconMode::OriginalPixmap){
            style.SetWidth(image.width());
            style.SetHeight(image.height());
          }
          success = true;
        }
      }

      if (success) {
        images[style.GetIconName()] = image;
        log.Info() << "Loaded icon '" << style.GetIconName() << "' from \"" << filename << "\"";
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

  bool MapPainterQt::HasPattern(const Projection& projection,
                                const MapParameter& parameter,
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
      bool success = false;
      std::string filename;
      QImage image;
      if (parameter.GetPatternMode()==MapParameter::PatternMode::Scalable){
        filename=AppendFileToDir(path,style.GetPatternName()+".svg");

        // Load SVG
        QSvgRenderer renderer(QString::fromStdString(filename));
        if (renderer.isValid()) {
          int dimension = std::round(projection.ConvertWidthToPixel(parameter.GetPatternSize()));
          image = QImage(dimension, dimension, QImage::Format_ARGB32);

          // Qt svg don't support page background from Inkscape SVGs, use fill color as workaround for it
          image.fill(QColor::fromRgbF(style.GetFillColor().GetR(),
                                      style.GetFillColor().GetG(),
                                      style.GetFillColor().GetB(),
                                      style.GetFillColor().GetA()));

          QPainter painter(&image);
          renderer.render(&painter);
          painter.end();
          success = !image.isNull();
        }
      }else {
        filename = AppendFileToDir(path, style.GetPatternName() + ".png");
        success = image.load(filename.c_str());
      }

      if (success) {
        if (idx>=patternImages.size()) {
          patternImages.resize(idx+1);
        }

        patternImages[idx]=image;

        if (idx>=patterns.size()) {
          patterns.resize(idx+1);
        }

        patterns[idx].setTextureImage(image);

        log.Info() << "Loaded pattern '" << style.GetPatternName() << "' from \"" << filename << "\"";

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

  double MapPainterQt::GetFontHeight(const Projection& projection,
                                     const MapParameter& parameter,
                                     double fontSize)
  {
    QFont        font(GetFont(projection,
                              parameter,
                              fontSize));
    QFontMetrics metrics=QFontMetrics(font);

    return metrics.height();
  }

  void LayoutTextLayout(const QFontMetrics& fontMetrics,
                        qreal proposedWidth,
                        QTextLayout& layout,
                        QRectF& boundingBox)
  {
    qreal width=0;
    qreal height=0;
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

  void MapPainterQt::DrawLabel(const Projection& /*projection*/,
                               const MapParameter& /*parameter*/,
                               const DoubleRectangle& labelRect,
                               const LabelData& label,
                               const QTextLayout& textLayout)
  {
    QRectF rect(labelRect.x, labelRect.y, labelRect.width, labelRect.height);
    if (!QRectF(painter->viewport()).intersects(rect)){
      return;
    }

    if (const auto *style = dynamic_cast<const TextStyle*>(label.style.get());
        style != nullptr) {

      double r=style->GetTextColor().GetR();
      double g=style->GetTextColor().GetG();
      double b=style->GetTextColor().GetB();

      if (style->GetStyle()==TextStyle::normal) {

        QColor textColor=QColor::fromRgbF(r,g,b,label.alpha);

        QList<QTextLayout::FormatRange> formatList;
        QTextLayout::FormatRange        range;

        painter->setPen(textColor);

        textLayout.draw(painter,rect.topLeft());
      }
      else if (style->GetStyle()==TextStyle::emphasize) {

        QColor textColor=QColor::fromRgbF(r,g,b,label.alpha);

        r=style->GetEmphasizeColor().GetR();
        g=style->GetEmphasizeColor().GetG();
        b=style->GetEmphasizeColor().GetB();

        QColor outlineColor=QColor::fromRgbF(r,g,b,label.alpha);

        /**
         * Use text outline for emphasize is better
         * (QTextLayout::FormatRange::setTextOutline),
         * but it has terribly performance.
         * So we draw text multiple times with move,
         * it will create similar effect.
         */
        painter->setPen(outlineColor);
        textLayout.draw(painter, rect.topLeft()-QPointF(1,0));
        textLayout.draw(painter, rect.topLeft()+QPointF(1,0));
        textLayout.draw(painter, rect.topLeft()-QPointF(0,1));
        textLayout.draw(painter, rect.topLeft()+QPointF(0,1));

        painter->setPen(textColor);
        textLayout.draw(painter, rect.topLeft());
      }
    }
    else if (const auto *style = dynamic_cast<const ShieldStyle*>(label.style.get());
             style != nullptr) {

      QPointF marginMove(-5,-5);
      QSizeF marginResize(10,10);

      QColor     textColor=QColor::fromRgbF(style->GetTextColor().GetR(),
                                            style->GetTextColor().GetG(),
                                            style->GetTextColor().GetB(),
                                            style->GetTextColor().GetA());

      // Shield background
      painter->fillRect(QRectF(rect.topLeft() + marginMove,
                               rect.size() + QSizeF(1,1) + marginResize),
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

      painter->drawRect(QRectF(rect.topLeft() + QPointF(2,2) + marginMove,
                               rect.size() + QSizeF(1-4,1-4) + marginResize));

      painter->setPen(QPen(textColor,1.0));
      textLayout.draw(painter,
                      rect.topLeft());
    } else {
      log.Warn() << "Label style not recognised: " << label.style.get();
    }
  }

  std::shared_ptr<QtLabel> MapPainterQt::Layout(const Projection& projection,
                                                const MapParameter& parameter,
                                                const std::string& text,
                                                double fontSize,
                                                double objectWidth,
                                                bool enableWrapping,
                                                bool /*contourLabel*/)
  {
    // TODO: cache labels
    QFont font(GetFont(projection,
                       parameter,
                       fontSize));
    qreal width=0;
    qreal height=0;

    painter->setFont(font);

    QFontMetrics fontMetrics=QFontMetrics(font, painter->device());
    qreal leading=fontMetrics.leading();

    std::shared_ptr<QtLabel> label=std::make_shared<QtLabel>(
        QString::fromUtf8(text.c_str()), font, painter->device());

    label->label.setCacheEnabled(true);

    double proposedWidth = -1;
    if (enableWrapping) {
      proposedWidth = GetProposedLabelWidth(parameter,
                                            fontMetrics.averageCharWidth(),
                                            objectWidth,
                                            text.length());
    }

    // evaluate layout
    label->label.beginLayout();
    while (true) {
      QTextLine line = label->label.createLine();
      if (!line.isValid())
        break;

      if (proposedWidth > 0) {
        line.setLineWidth(proposedWidth);
      } else {
        // it is necessary to setup some width to get usable dimension in QTextLine::naturalTextWidth()
        line.setLineWidth(std::numeric_limits<qreal>::max());
      }

      if (leading > 0) {
        height += leading;
      }
      line.setPosition(QPointF(0.0,height));
      width=std::max(width,line.naturalTextWidth());
      height+=line.height();
    }
    label->label.endLayout();

    // Center all lines horizontally, after we know the actual width

    for (int i=0; i<label->label.lineCount(); i++) {
      QTextLine line = label->label.lineAt(i);

      line.setPosition(QPointF((width-line.naturalTextWidth())/2,line.position().y()));
    }

    label->width=width;
    label->height=height;
    label->fontSize=fontSize;
    label->text=text;

    return label;
  }

  void MapPainterQt::SetupTransformation(QPainter* painter,
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

    // Additional offseting
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

  void MapPainterQt::FollowPathInit(FollowPathHandle &hnd,
                                    Vertex2D &origin,
                                    size_t transStart,
                                    size_t transEnd,
                                    bool isClosed,
                                    bool keepOrientation)
  {
    hnd.i=0;
    hnd.nVertex=transEnd >= transStart ? transEnd - transStart : transStart-transEnd;
    bool isReallyClosed=(coordBuffer.buffer[transStart]==coordBuffer.buffer[transEnd]);

    if (isClosed && !isReallyClosed) {
      hnd.nVertex++;
      hnd.closeWay=true;
    }
    else {
      hnd.closeWay=false;
    }

    if (keepOrientation ||
        coordBuffer.buffer[transStart].GetX()<coordBuffer.buffer[transEnd].GetX()) {
      hnd.transStart=transStart;
      hnd.transEnd=transEnd;
    }
    else {
      hnd.transStart=transEnd;
      hnd.transEnd=transStart;
    }

    hnd.direction=(hnd.transStart < hnd.transEnd) ? 1 : -1;
    origin.Set(coordBuffer.buffer[hnd.transStart].GetX(), coordBuffer.buffer[hnd.transStart].GetY());
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
        x2=coordBuffer.buffer[hnd.transStart].GetX();
        y2=coordBuffer.buffer[hnd.transStart].GetY();
      }
      else {
        x2=coordBuffer.buffer[hnd.transStart+(hnd.i+1)*hnd.direction].GetX();
        y2=coordBuffer.buffer[hnd.transStart+(hnd.i+1)*hnd.direction].GetY();
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

    symbol.GetBoundingBox(projection,minX,minY,maxX,maxY);

    double           widthPx=maxX-minX;
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
          DrawSymbol(projection, parameter, symbol, 0, 0);
          loop=FollowPath(followPathHnd, space, origin);
        }
      }
    }
    painter->setTransform(savedTransform);
  }

  void MapPainterQt::DrawIcon(const IconStyle* style,
                              double x, double y,
                              double width, double height)
  {
    const auto &it=images.find(style->GetIconName());

    assert(it!=images.end());
    assert(!it->second.isNull());

    painter->drawImage(QRectF(x-width/2, y-height/2, width, height),
                       it->second,
                       QRectF(0, 0, it->second.width(), it->second.height()));
  }

  void MapPainterQt::DrawSymbol(const Projection& projection,
                                const MapParameter& /*parameter*/,
                                const Symbol& symbol,
                                double x, double y)
  {
    SymbolRendererQt renderer(painter);
    renderer.Render(symbol, Vertex2D(x, y), projection);
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
    QPen pen(QColor::fromRgbF(color.GetR(),
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

      dashes << 0 << 0; // skip butt?
      for (size_t i=0; i<dash.size(); i++) {
        dashes << dash[i];
      }

      pen.setDashPattern(dashes);
    }

    QPainterPath p;

    p.moveTo(coordBuffer.buffer[transStart].GetX(),
             coordBuffer.buffer[transStart].GetY());
    for (size_t i=transStart+1; i<=transEnd; i++) {
      p.lineTo(coordBuffer.buffer[i].GetX(),
               coordBuffer.buffer[i].GetY());
    }

    painter->strokePath(p,pen);

    if (dash.empty() &&
        startCap==LineStyle::capRound &&
        endCap!=LineStyle::capRound) {
      painter->setPen(QColor(Qt::transparent));
      painter->setBrush(QBrush(QColor::fromRgbF(color.GetR(),
                                                color.GetG(),
                                                color.GetB(),
                                                color.GetA())));

      painter->drawEllipse(QPointF(coordBuffer.buffer[transStart].GetX(),
                                   coordBuffer.buffer[transStart].GetY()),
                                   width/2,width/2);
    }

    if (dash.empty() &&
        endCap==LineStyle::capRound &&
        startCap!=LineStyle::capRound) {
      painter->setPen(QColor(Qt::transparent));
      painter->setBrush(QBrush(QColor::fromRgbF(color.GetR(),
                                                color.GetG(),
                                                color.GetB(),
                                                color.GetA())));

      painter->drawEllipse(QPointF(coordBuffer.buffer[transEnd].GetX(),
                                   coordBuffer.buffer[transEnd].GetY()),
                                   width/2,width/2);
    }
  }

  void MapPainterQt::DrawArea(const Projection& projection,
                              const MapParameter& parameter,
                              const MapPainter::AreaData& area)
  {
    QPainterPath path;

    path.moveTo(coordBuffer.buffer[area.coordRange.GetStart()].GetX(),
                coordBuffer.buffer[area.coordRange.GetStart()].GetY());
    for (size_t i=area.coordRange.GetStart()+1; i<=area.coordRange.GetEnd(); i++) {
      path.lineTo(coordBuffer.buffer[i].GetX(),
                  coordBuffer.buffer[i].GetY());
    }
    path.closeSubpath();

    if (!area.clippings.empty()) {
      for (const auto& data : area.clippings) {
        path.moveTo(coordBuffer.buffer[data.GetStart()].GetX(),
                    coordBuffer.buffer[data.GetStart()].GetY());

        for (size_t i=data.GetStart()+1; i<=data.GetEnd(); i++) {
          path.lineTo(coordBuffer.buffer[i].GetX(),
                      coordBuffer.buffer[i].GetY());
        }

        path.closeSubpath();
      }
    }

    if (area.fillStyle) {
      SetFill(projection,
              parameter,
              *area.fillStyle);
    }
    else {
      painter->setBrush(Qt::NoBrush);
    }

    if (area.borderStyle) {
      SetBorder(projection,
                parameter,
                *area.borderStyle);
    }
    else {
      painter->setPen(Qt::NoPen);
    }

    bool   restoreTransform=false;
    size_t idx=-1;

    if (area.fillStyle &&
        area.fillStyle->HasPattern()) {
      idx=area.fillStyle->GetPatternId()-1;

      if (idx<patterns.size() && !patterns[idx].textureImage().isNull()) {
        patterns[idx].setTransform(QTransform::fromTranslate(
                                          remainder(coordBuffer.buffer[area.coordRange.GetStart()].GetX(),patterns[idx].textureImage().width()),
                                          remainder(coordBuffer.buffer[area.coordRange.GetStart()].GetY(),patterns[idx].textureImage().height())));
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

  void MapPainterQt::BeforeDrawing(const StyleConfig& /*styleConfig*/,
                                   const Projection& projection,
                                   const MapParameter& parameter,
                                   const MapData& /*data*/)
  {
    labelLayouter.SetViewport(DoubleRectangle(0, 0, painter->window().width(), painter->window().height()));
    labelLayouter.SetLayoutOverlap(projection.ConvertWidthToPixel(parameter.GetLabelLayouterOverlap()));
  }

  MapPainterQt::QtLabelLayouter& MapPainterQt::GetLayouter()
  {
    if (delegateLabelLayouter){
      return *delegateLabelLayouter;
    }
    return labelLayouter;
  }

  void MapPainterQt::DrawRectangle(int x, int y,
                                   int width, int height,
                                   const Color &color)
  {
    QPen pen;
    pen.setColor(QColor::fromRgbF(color.GetR(),color.GetG(),color.GetB(),color.GetA()));
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);

    painter->drawRect(x,y,width,height);
  }


  void MapPainterQt::RegisterRegularLabel(const Projection &projection,
                                          const MapParameter &parameter,
                                          const std::vector<LabelData> &labels,
                                          const Vertex2D &position,
                                          double objectWidth)
  {
    GetLayouter().RegisterLabel(projection, parameter, position, labels, objectWidth);
  }

  void MapPainterQt::RegisterContourLabel(const Projection &projection,
                                          const MapParameter &parameter,
                                          const PathLabelData &label,
                                          const LabelPath &labelPath)
  {
    GetLayouter().RegisterContourLabel(projection, parameter, label, labelPath);
  }

  void MapPainterQt::DrawLabels(const Projection& projection,
                                const MapParameter& parameter,
                                const MapData& /*data*/)
  {
    if (delegateLabelLayouter !=nullptr){
      return;
    }

    labelLayouter.Layout(projection, parameter);

    labelLayouter.DrawLabels(projection,
                             parameter,
                             this);

    labelLayouter.Reset();
  }

  DoubleRectangle MapPainterQt::GlyphBoundingBox(const QGlyphRun &glyph) const
  {
    auto bbox=glyph.boundingRect();
    auto tl=bbox.topLeft();
    return DoubleRectangle(tl.x(), tl.y(), bbox.width(), bbox.height());
  }

  void MapPainterQt::DrawGlyph(QPainter *painter, const Glyph<QGlyphRun> &glyph) const
  {
    QTransform tran;
    const QTransform originalTran=painter->transform();

    tran.translate(glyph.position.GetX(), glyph.position.GetY());
    tran.rotateRadians(glyph.angle);

    painter->setTransform(tran);

    painter->drawGlyphRun(QPointF(0,0), glyph.glyph);

    painter->setTransform(originalTran);
  }

  void MapPainterQt::DrawGlyphs(const Projection &/*projection*/,
                                const MapParameter &/*parameter*/,
                                const osmscout::PathTextStyleRef style,
                                const std::vector<Glyph<QGlyphRun>> &glyphs)
  {
    const Color &color = style->GetTextColor();
    QPen pen;
    pen.setColor(QColor::fromRgbF(color.GetR(),color.GetG(),color.GetB(),color.GetA()));
    painter->setPen(pen);

    for (const Glyph<QGlyphRun> &glyph:glyphs) {

      DrawGlyph(painter, glyph);
    }
  }

  void MapPainterQt::SetFill(const Projection& projection,
                             const MapParameter& parameter,
                             const FillStyle& fillStyle)
  {
    if (fillStyle.HasPattern() &&
        projection.GetMagnification()>=fillStyle.GetPatternMinMag() &&
        HasPattern(projection, parameter, fillStyle)) {
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
  }

  void MapPainterQt::SetBorder(const Projection& projection,
                               const MapParameter& parameter,
                               const BorderStyle& borderStyle)
  {
    double borderWidth=projection.ConvertWidthToPixel(borderStyle.GetWidth());

    if (borderWidth>=parameter.GetLineMinWidthPixel()) {
      QPen pen;

      pen.setColor(QColor::fromRgbF(borderStyle.GetColor().GetR(),
                                    borderStyle.GetColor().GetG(),
                                    borderStyle.GetColor().GetB(),
                                    borderStyle.GetColor().GetA()));
      pen.setWidthF(borderWidth);

      if (borderStyle.GetDash().empty()) {
        pen.setStyle(Qt::SolidLine);
        pen.setCapStyle(Qt::RoundCap);
      }
      else {
        QVector<qreal> dashes;

        for (double i : borderStyle.GetDash()) {
          dashes << i;
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

  void MapPainterQt::DrawGroundTiles(const Projection& projection,
                                     const MapParameter& parameter,
                                     const std::list<GroundTile>& groundTiles,
                                     QPainter* painter)
  {
    std::lock_guard<std::mutex> guard(mutex);

    this->painter=painter;

    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::TextAntialiasing);

    // TODO: remove this method and use standard Draw
    MapData data;
    data.baseMapTiles=groundTiles;

    Draw(projection,
         parameter,
         data,
         RenderSteps::DrawBaseMapTiles,
         RenderSteps::DrawBaseMapTiles);
  }

  bool MapPainterQt::DrawMap(const Projection& projection,
                             const MapParameter& parameter,
                             const MapData& data,
                             QPainter* painter,
                             RenderSteps startStep,
                             RenderSteps endStep)
  {
    std::lock_guard<std::mutex> guard(mutex);

    this->painter=painter;

    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::TextAntialiasing);

    return Draw(projection,
                parameter,
                data,
                startStep,
                endStep);
  }

  template<> std::vector<QtGlyph> QtLabel::ToGlyphs() const
  {
    std::vector<QtGlyph> result;
    QVector<quint32> indexes(1);
    QVector<QPointF> positions(1);

    positions[0] = QPointF(0, 0);

    QList<QGlyphRun> glyphs=label.glyphRuns();
    for (const QGlyphRun &glyphRun: glyphs){
      for (int g=0; g<glyphRun.glyphIndexes().size(); g++) {

        qint32 index = glyphRun.glyphIndexes().at(g);
        QPointF pos = glyphRun.positions().at(g);
        QRectF bbox = glyphRun.rawFont().boundingRect(index);

        indexes[0] = index;

        QGlyphRun orphanGlyph;
        orphanGlyph.setBoundingRect(bbox);
        orphanGlyph.setFlags(glyphRun.flags());
        orphanGlyph.setGlyphIndexes(indexes);
        orphanGlyph.setOverline(glyphRun.overline());
        orphanGlyph.setPositions(positions);
        orphanGlyph.setRawFont(glyphRun.rawFont());
        orphanGlyph.setRightToLeft(glyphRun.isRightToLeft());
        orphanGlyph.setStrikeOut(glyphRun.strikeOut());
        orphanGlyph.setUnderline(glyphRun.underline());

        QtGlyph glyph;
        glyph.glyph=std::move(orphanGlyph);
        glyph.position.Set(pos.x(), pos.y());
        result.push_back(std::move(glyph));
      }
    }
    return result;
  }

  MapPainterBatchQt::MapPainterBatchQt(size_t expectedCount):
    MapPainterBatch(expectedCount) {}

  MapPainterBatchQt::~MapPainterBatchQt(){}

  bool MapPainterBatchQt::paint(const Projection& projection,
                                const MapParameter& parameter,
                                QPainter* qPainter)
  {
    assert(data.size() == painters.size());
    if (painters.empty()){
      return true;
    }
    qPainter->setRenderHint(QPainter::Antialiasing);
    qPainter->setRenderHint(QPainter::TextAntialiasing);

    // prepare map painters:
    // - acquire locks
    // - setup QPainter instance
    // - delegate all label layout operations to the last painter
    std::vector<std::unique_lock<std::mutex>> locks;
    locks.reserve(painters.size());
    MapPainterQt* lastPainter = painters.back();
    for (MapPainterQt* painter: painters){
      locks.emplace_back(painter->mutex);
      painter->painter = qPainter;

      if (painter != lastPainter){
        painter->delegateLabelLayouter = &(lastPainter->labelLayouter);
      }
    }

    bool success=true;
    for (size_t step=osmscout::RenderSteps::FirstStep;
         step<=osmscout::RenderSteps::LastStep;
         step++){

      for (size_t i=0;i<data.size(); i++){
        const MapData &d=*(data[i]);
        MapPainterQt *painter = painters[i];

        // copy missing icons to last painter cache
        // because last painter do the real label and icon rendering
        if (step==osmscout::RenderSteps::DrawLabels &&
            painter->delegateLabelLayouter == &(lastPainter->labelLayouter) &&
            painter!=lastPainter){

          for (auto im=painter->images.begin(); im != painter->images.end(); im++){
            if (lastPainter->images.find(im->first) == lastPainter->images.end()) {
              lastPainter->images[im->first] = im->second;
            }
          }
        }
        success &= painter->Draw(projection,
                                 parameter,
                                 d,
                                 (RenderSteps)step,
                                 (RenderSteps)step);
      }
    }

    // cleanup
    for (MapPainterQt* painter: painters){
      painter->painter = nullptr;
      painter->delegateLabelLayouter = nullptr;
    }
    return success;
  }
}

