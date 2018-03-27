/*
  DrawTextQt - a test program for libosmscout
  Copyright (C) 2018  Lukas Karas

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

class OSMSCOUT_MAP_API LabelLayoutKey
{
public:
  std::string text;
  double fontSize;
  bool multiline;
  double maxWidth;

public:
  LabelLayoutKey();
  virtual ~LabelLayoutKey();
};

//typedef std::list<LabelData>::iterator LabelDataRef;
template<class NativeGlyph>
class Glyph {
public:
  NativeGlyph glyph;
  osmscout::Vertex2D center;
  double angle;
};

template<class NativeGlyph>
class Label
{
public:
  double                   width;
  double                   height;

  double                   fontSize; //!< Font size to be used
  osmscout::LabelStyleRef  style;    //!< Style for drawing
  std::string              text;     //!< The label text

  std::vector<Glyph<NativeGlyph>> toGlyphs() const;
};

template<class NativeGlyph>
class LabelInstance
{
public:
  size_t                   id;       //!< Id of this label, multiple labels with the same id do not intersect with each other
  size_t                   priority; //!< Priority of the entry

  double                   x;        //!< Coordinate of the left, top edge of the text
  double                   y;        //!< Coordinate of the left, top edge of the text
  double                   alpha;    //!< Alpha value of the label

  Label<NativeGlyph>       label;

public:
  LabelInstance();
  virtual ~LabelInstance();
};

template <class NativeGlyph, class TextLayouter>
class OSMSCOUT_MAP_API LabelLayouter
{
public:
  using ContourLabel = std::vector<Glyph<NativeGlyph>>;

public:
  void registerContourLabel(std::vector<QPointF> way,
                            std::string string)
  {

  }

  std::vector<LabelInstance<NativeGlyph>> labels() const
  {
    return std::move(std::vector<LabelInstance<NativeGlyph>>());
  }

  std::vector<ContourLabel> contourLabels() const
  {
    return std::move(std::vector<ContourLabel>());
  }
};

class QTextLayouter
{

};

Label<QGlyphRun>::toGlyphs() const
{

}

class DrawWindow : public QWidget
{
Q_OBJECT
public:
  explicit DrawWindow(QString variant, int sinCount, QWidget *parent = 0);

  virtual ~DrawWindow();

protected:
  virtual void paintEvent(QPaintEvent *event);

  void drawLine(QPainter *painter, const std::vector<QPointF> &p);
  void drawGlyph(QPainter *painter,
                 const Glyph<QGlyphRun> &glyph) const;

  QString                   variant;
  std::vector<double>       sin;           //! Lookup table for sin calculation
  int                       sinCount;
  int                       cnt;
  QTime                     timer;
  QTime                     animTimer;
  int                       startOffset;
  double                    moveOffset;
};

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

void DrawWindow::drawLine(QPainter *painter, const std::vector<QPointF> &way)
{
  QPen pen;
  pen.setColor(QColor::fromRgbF(0,0,1));
  painter->setPen(pen);
  for (const auto &point:way){
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


void DrawWindow::drawGlyph(QPainter *painter,
                           const Glyph<QGlyphRun> &glyph) const
{
  QTransform tran;
  osmscout::Vertex2D point=glyph.center;
  qreal   angle=glyph.angle;
  qreal penWidth = painter->pen().widthF();

  // rotation matrix components
  qreal sina=sin[lround((360-angle)*10)%sin.size()];
  qreal cosa=sin[lround((360-angle+90)*10)%sin.size()];

  // Rotation
  qreal newX=(cosa*point.GetX())-(sina*(point.GetY()));
  qreal newY=(cosa*(point.GetY()))+(sina*point.GetX());

  // Aditional offseting
  qreal deltaPenX=cosa*penWidth;
  qreal deltaPenY=sina*penWidth;

  // Getting the delta distance for the translation part of the transformation
  qreal deltaX=newX-point.GetX();
  qreal deltaY=newY-point.GetY();

  // Applying rotation and translation.
  tran.setMatrix(cosa,sina,0.0,
                 -sina,cosa,0.0,
                 -deltaX+deltaPenX,-deltaY-deltaPenY,1.0);
  painter->setTransform(tran);

  /*
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
  */

  painter->drawGlyphRun(QPointF(point.GetX(), point.GetY()), glyph.glyph);

}

void DrawWindow::paintEvent(QPaintEvent* /* event */)
{
  QPainter painter(this);
  painter.fillRect(0,0, width(), height(), QBrush(QColor::fromRgbF(1,1,1)));

  LabelLayouter<QGlyphRun,QTextLayouter> layouter;

  QString string="Hebrew Sarah (שרה) is spelled: sin (ש), resh (ר) and heh (ה)";

  int sinStart=0;
  for (int k=0;k<sinCount;k++){
    std::vector<QPointF> way;
    for (int x=startOffset;(x+startOffset)<width();x++){
      //int y=std::cos(((double)(x+sinStart)/(double)width()) *3*M_PI) * (height()/2-44) + height()/2;
      int y=std::sin(((double)(x+sinStart+moveOffset)/(double)width()) *2*M_PI) * (height()/2-44) + height()/2;
      way.push_back(QPointF(x,y));
    }
    layouter.registerContourLabel(way, string.toStdString());
  }

  for (const LabelInstance<QGlyphRun> &label:layouter.labels()){
    for (const Glyph<QGlyphRun> &glyph:label.label.toGlyphs()){
      drawGlyph(&painter, glyph);
    }
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
  DrawWindow window("",10);
  window.show();

  return app.exec();
}

