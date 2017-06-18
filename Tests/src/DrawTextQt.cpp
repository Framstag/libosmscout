/*
  DrawTextQt - a test program for libosmscout
  Copyright (C) 2017  Lukas Karas

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <iostream>
#include <iomanip>

#include <QApplication>
#include <QDesktopWidget>
#include <QPixmap>
#include <QScreen>
#include <QtGui>
#include <QDebug>

#include <osmscout/Database.h>
#include <osmscout/MapService.h>

#include <osmscout/SimplifiedPath.h>

#include <DrawWindow.h>

DrawWindow::DrawWindow(QString variant, int sinCount, QWidget *parent)
   : QWidget(parent), variant(variant), sinCount(sinCount), cnt(0), startOffset(0), moveOffset(0)
{
  create();
  setMinimumSize(QSize(300,100));

  timer.restart();
  animTimer.restart();

  sin.resize(360*10);

  for (size_t i=0; i<sin.size(); i++) {
    sin[i]=std::sin(M_PI/180*i/(sin.size()/360));
  }
}

DrawWindow::~DrawWindow()
{

}

void DrawWindow::setupTransformation(QPainter *painter, const osmscout::SimplifiedPath &p,
                                     const qreal offset, const qreal baseline) const
{
  QTransform tran;
  QPointF point=p.PointAtLength(offset);
  qreal   angle=p.AngleAtLengthDeg(offset);
  qreal penWidth = painter->pen().widthF();

  // rotation matrix components
  qreal sina=sin[lround((360-angle)*10)%sin.size()];
  qreal cosa=sin[lround((360-angle+90)*10)%sin.size()];

  // Rotation
  qreal newX=(cosa*point.x())-(sina*(point.y()-baseline));
  qreal newY=(cosa*(point.y()-baseline))+(sina*point.x());

  // Aditional offseting
  qreal deltaPenX=cosa*penWidth;
  qreal deltaPenY=sina*penWidth;

  // Getting the delta distance for the translation part of the transformation
  qreal deltaX=newX-point.x();
  qreal deltaY=newY-point.y();

  // Applying rotation and translation.
  tran.setMatrix(cosa,sina,0.0,
                 -sina,cosa,0.0,
                 -deltaX+deltaPenX,-deltaY-deltaPenY,1.0);
  painter->setTransform(tran);
}

void DrawWindow::drawText1(QPainter *painter, QString string, const osmscout::SimplifiedPath &p)
{
  QPen          pen;
  QFont         font;
  QFontMetricsF metrics=QFontMetricsF(font,painter->device());
  double        fontHeight=12;

  font.setPixelSize(fontHeight);
  font.setStyleStrategy(QFont::PreferAntialias);
  font.setStyleStrategy(QFont::PreferMatch);

  pen.setColor(QColor::fromRgbF(0,0,0));
  painter->setPen(pen);
  painter->setFont(font);

  qreal offset=0;
  while (offset<p.GetLength()){
    for (int i=0; i<string.size() && offset<p.GetLength(); i++) {
      QPointF point=p.PointAtLength(offset);

      setupTransformation(painter, p, offset, fontHeight/4);

      painter->drawText(point,QString(string[i]));

      offset+=metrics.width(string[i]);
    }

    offset+=3*fontHeight;
  }
  painter->resetTransform();
}

void DrawWindow::drawText2(QPainter *painter, QString string, const osmscout::SimplifiedPath &p)
{
  QPen          pen;
  QFont         font;
  double        fontHeight=12;
  double        pLength=p.GetLength();

  font.setPixelSize(fontHeight);
  font.setStyleStrategy(QFont::PreferAntialias);
  font.setStyleStrategy(QFont::PreferMatch);

  pen.setColor(QColor::fromRgbF(0,0,0));
  painter->setPen(pen);
  painter->setFont(font);

  QTextLayout textLayout(string,font,painter->device());
  // evaluate layout
  textLayout.beginLayout();
  while (textLayout.createLine().isValid()){};
  textLayout.endLayout();

  QList<QGlyphRun> glyphs=textLayout.glyphRuns();
  double stringWidth=textLayout.boundingRect().width();

  //qDebug() << "lines"<<textLayout.lineCount()<<string;
  //qDebug() << "generated"<<glyphs.size()<<"glyphs";

  double offset=0;
  QVector<quint32> indexes(1);
  QVector<QPointF> positions(1);
  while (offset<pLength){
    for (const QGlyphRun &glypRun: glyphs){
      for (int g=0; g<glypRun.glyphIndexes().size(); g++){
        auto index=glypRun.glyphIndexes().at(g);
        auto pos=glypRun.positions().at(g);
        indexes[0]=index;
        positions[0]=QPointF(0,pos.y());

        qreal glyphOffset=offset+pos.x();
        if (glyphOffset>pLength)
          continue;

        //QPointF point=p.pointAtPercent(p.percentAtLength(glyphOffset));
        QPointF point=p.PointAtLength(glyphOffset);

        // check if current glyph can be visible
        QRectF boundingRect=glypRun.rawFont().boundingRect(index);
        qreal diagonal=boundingRect.width()+boundingRect.height(); // it is little bit longer than correct sqrt(w^2+h^2)
        if (!painter->viewport().intersects(QRect(QPoint(point.x()-diagonal, point.y()-diagonal),
                                                  QPoint(point.x()+diagonal, point.y()+diagonal)))){
          continue;
        }
        setupTransformation(painter, p, glyphOffset, fontHeight*-1);

        QGlyphRun orphanGlyph;
        //orphanGlyph.setBoundingRect();
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

    offset+=stringWidth+3*fontHeight;
  }
  painter->resetTransform();
}

void DrawWindow::drawLine(QPainter *painter, const osmscout::SimplifiedPath &p)
{
  QPen pen;
  pen.setColor(QColor::fromRgbF(0,0,1));
  painter->setPen(pen);
  for (double d=0;d<p.GetLength();d+=0.5){
    QPointF point=p.PointAtLength(d);
    painter->drawPoint(point);
  }

  /*
  pen.setColor(QColor::fromRgbF(1,0,0));
  painter->setPen(pen);
  for (double d=0;d<p.GetLength();d+=20){
    QPointF point=p.PointAtLength(d);
    qreal angle=p.AngleAtLength(d);
    QPointF add(std::cos(angle), -1*std::sin(angle));
    //qDebug() << d << p.AngleAtLengthDeg(d) << add;
    painter->drawLine(point, point + add*10);
  }
  */
}

void DrawWindow::paintEvent(QPaintEvent */* event */)
{
  QPainter painter(this);
  painter.fillRect(0,0, width(), height(), QBrush(QColor::fromRgbF(1,1,1)));

  QString string="Hebrew Sarah (שרה) is spelled: sin (ש), resh (ר) and heh (ה)";

  int sinStart=0;
  for (int k=0;k<sinCount;k++){
    osmscout::SimplifiedPath p;
    // fill path with sinus
    for (int x=startOffset;(x+startOffset)<width();x++){
      //int y=std::cos(((double)(x+sinStart)/(double)width()) *3*M_PI) * (height()/2-44) + height()/2;
      int y=std::sin(((double)(x+sinStart+moveOffset)/(double)width()) *2*M_PI) * (height()/2-44) + height()/2;
      p.AddPoint(x,y);
    }
    sinStart+=30;

    if (variant=="simple")
      drawText1(&painter,string,p);
    if (variant=="bidirectional")
      drawText2(&painter,string,p);
    if (variant=="line")
      drawLine(&painter,p);
  }

  cnt++;
  if (timer.elapsed()>2000){
    qDebug() << "" << ((double)cnt/(double)timer.elapsed())*1000 << " fps";
    cnt=0;
    timer.restart();
  }
  if (animTimer.elapsed()>100){
    startOffset=(startOffset-1)%width();
    moveOffset=startOffset-0.2;
    animTimer.restart();
  }

  painter.end();
  update();
}

int main(int argc, char** argv)
{
  QApplication app(argc, argv);

  if (app.arguments().size()<3){
    qWarning() << "No enough arguments!";
    std::cout << "Usage:" << std::endl;
    std::cout << app.arguments().at(0).toStdString() << " countOfCurves variant" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "Variant can be:" << std::endl;
    std::cout << "  noop" << std::endl;
    std::cout << "  line" << std::endl;
    std::cout << "  simple" << std::endl;
    std::cout << "  bidirectional" << std::endl;
    return 1;
  }

  int sinCount=app.arguments().at(1).toInt();
  QString variant=app.arguments().at(2);

  qDebug() << "use" << variant << "variant, render" << sinCount << "paths";

  DrawWindow window(variant,sinCount);
  window.show();

  return app.exec();
}

