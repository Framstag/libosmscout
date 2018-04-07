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
  osmscout::Vertex2D position;
  double angle{0}; // radians
};

template<class NativeGlyph, class NativeLabel>
class Label
{
public:
  NativeLabel             label;

  double                  width;
  double                  height;

  double                  fontSize; //!< Font size to be used
  osmscout::LabelStyleRef style;    //!< Style for drawing
  std::string             text;     //!< The label text

  std::vector<Glyph<NativeGlyph>> toGlyphs() const;
};

template<class NativeGlyph, class NativeLabel>
class LabelInstance
{
public:
  size_t                  id;       //!< Id of this label, multiple labels with the same id do not intersect with each other
  size_t                  priority; //!< Priority of the entry

  double                  x;        //!< Coordinate of the left, top edge of the text
  double                  y;        //!< Coordinate of the left, top edge of the text
  double                  alpha;    //!< Alpha value of the label

  Label<NativeGlyph, NativeLabel>
                          label;

public:
  LabelInstance();
  virtual ~LabelInstance();
};

template <class NativeGlyph, class NativeLabel, class TextLayouter>
class OSMSCOUT_MAP_API LabelLayouter
{
public:
  using ContourLabel = std::vector<Glyph<NativeGlyph>>;
  using LabelType = Label<NativeGlyph, NativeLabel>;
  using LabelInstanceType = LabelInstance<NativeGlyph, NativeLabel>;

public:
  LabelLayouter(TextLayouter *textLayouter):
      textLayouter(textLayouter)
  {};

  void registerContourLabel(std::vector<QPointF> way,
                            std::string string)
  {
    // TODO: parameters
    int fontHeight=24;
    int textOffset=6;

    // TODO: cache simplified path for way id
    osmscout::SimplifiedPath p;
    for (auto const &point:way){
      p.AddPoint(point.x(), point.y());
    }

    // TODO: cache label for string and font parameters
    LabelType label = textLayouter->layout(string, fontHeight, /* proposed width */ 5000.0);
    ContourLabel cLabel = label.toGlyphs();

    double pLength=p.GetLength();
    double offset=0;
    while (offset<pLength){
      ContourLabel copy;
      for (Glyph<NativeGlyph> glyphCopy:cLabel){
        double glyphOffset=offset+glyphCopy.position.GetX();
        QPointF point=p.PointAtLength(glyphOffset);
        qreal   angle=p.AngleAtLength(glyphOffset);
        // TODO: take y into account
        glyphCopy.position=osmscout::Vertex2D(point.x() + textOffset * std::sin(angle),
                                              point.y() + textOffset * std::cos(angle));
        glyphCopy.angle=angle;

        copy.push_back(glyphCopy);
      }
      contourLabelsVect.push_back(copy);
      offset+=label.width + 3*fontHeight;
    }
    // TODO
  }

  std::vector<LabelInstanceType> labels() const
  {
    // TODO
    return std::move(std::vector<LabelInstanceType>());
  }

  std::vector<ContourLabel> contourLabels() const
  {
    return contourLabelsVect;
  }

private:
  TextLayouter *textLayouter;
  std::vector<ContourLabel> contourLabelsVect;
};


using QtGlyph = Glyph<QGlyphRun>;
using QtLabel = Label<QGlyphRun, std::shared_ptr<QTextLayout>>;

class QTextLayouter
{
public:
  QPainter *painter;
  LabelLayouter<QGlyphRun, std::shared_ptr<QTextLayout>, QTextLayouter> labelLayouter;

public:
  QTextLayouter(QPainter *painter):
    painter(painter),
    labelLayouter(this)
  {
  }

  QtLabel layout(std::string text, int fontSize, double proposedWidth)
  {
    QFont         font;
    qreal width=0;
    qreal height=0;

    font.setPixelSize(fontSize);
    font.setStyleStrategy(QFont::PreferAntialias);
    font.setStyleStrategy(QFont::PreferMatch);

    painter->setFont(font);

    QFontMetrics fontMetrics=QFontMetrics(font, painter->device());
    qreal leading=fontMetrics.leading();

    QtLabel label;

    label.label = std::make_shared<QTextLayout>(QString::fromUtf8(text.c_str()),font,painter->device());
    // evaluate layout
    label.label->beginLayout();
    while (true) {
      QTextLine line = label.label->createLine();
      if (!line.isValid())
        break;

      line.setLineWidth(proposedWidth);

      height+=leading;
      line.setPosition(QPointF(0.0,height));
      width=std::max(width,line.naturalTextWidth());
      height+=line.height();
    }
    label.label->endLayout();

    // Center all lines horizontally, after we know the actual width

    for (int i=0; i<label.label->lineCount(); i++) {
      QTextLine line = label.label->lineAt(i);

      line.setPosition(QPointF((width-line.naturalTextWidth())/2,line.position().y()));
    }

    label.width=width;
    label.height=height;
    label.fontSize=fontSize;
    //label.style;
    label.text=text;

    return label;
  }
};

template<> std::vector<QtGlyph> QtLabel::toGlyphs() const
{
  std::vector<QtGlyph> result;
  QVector<quint32> indexes(1);
  QVector<QPointF> positions(1);

  positions[0] = QPointF(0, 0);

  QList<QGlyphRun> glyphs=label->glyphRuns();
  for (const QGlyphRun &glyphRun: glyphs){
    for (int g=0; g<glyphRun.glyphIndexes().size(); g++) {

      qint32 index = glyphRun.glyphIndexes().at(g);
      QPointF pos = glyphRun.positions().at(g);
      QRectF bbox = glyphRun.boundingRect();

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
                 const QtGlyph &glyph) const;

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
  QPen          pen;
  pen.setColor(QColor::fromRgbF(0,0,0));
  painter->setPen(pen);

  QTransform tran;
  const QTransform &originalTran=painter->transform();
  QPointF point=QPointF(glyph.position.GetX(), glyph.position.GetY());
  qreal   angle=(glyph.angle * 180) / M_PI;
  //qreal penWidth = painter->pen().widthF();

  // rotation matrix components
  /*
  qreal sina=sin[lround((360-angle)*10)%sin.size()];
  qreal cosa=sin[lround((360-angle+90)*10)%sin.size()];

  // Applying rotation and translation.
  tran.setMatrix(cosa, sina, 0.0,
                 -sina, cosa, 0.0,
                 point.x(), point.y(), 1.0);

  */
  tran.translate(point.x(), point.y());
  tran.rotateRadians(glyph.angle*-1);


  painter->setTransform(tran);

  painter->drawGlyphRun(QPointF(0,0), glyph.glyph);

  painter->setTransform(originalTran);

}

void DrawWindow::paintEvent(QPaintEvent* /* event */)
{
  QPainter painter(this);
  painter.fillRect(0,0, width(), height(), QBrush(QColor::fromRgbF(1,1,1)));

  QTextLayouter textLayouter(&painter);
  LabelLayouter<QGlyphRun,std::shared_ptr<QTextLayout>,QTextLayouter> *layouter=&textLayouter.labelLayouter;

  QString string="Hebrew Sarah (שרה) is spelled: sin (ש), resh (ר) and heh (ה)";
  //QString string="A";

  int sinStart=0;
  for (int k=0;k<sinCount;k++){
    std::vector<QPointF> way;
    for (int x=startOffset;(x+startOffset)<width();x++){
      //int y=std::cos(((double)(x+sinStart)/(double)width()) *3*M_PI) * (height()/2-44) + height()/2;
      int y=std::sin(((double)(x+sinStart+moveOffset)/(double)width()) *2*M_PI) * (height()/2-44) + height()/2;
      way.push_back(QPointF(x,y));
    }
    layouter->registerContourLabel(way, string.toStdString());
    drawLine(&painter, way);
  }

  for (const std::vector<Glyph<QGlyphRun>> &label:layouter->contourLabels()){
    for (const Glyph<QGlyphRun> &glyph:label){
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
  DrawWindow window("",1);
  window.show();

  return app.exec();
}

