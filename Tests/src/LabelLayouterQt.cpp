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
#include <memory>

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

class IntRectangle {
public:
  int x;
  int y;
  int width;
  int height;
};

//typedef std::list<LabelData>::iterator LabelDataRef;
template<class NativeGlyph>
class Glyph {
public:
  NativeGlyph glyph;
  osmscout::Vertex2D position;
  double angle{0}; //!< clock-wise rotation in radians

  osmscout::Vertex2D tl{0,0};
  osmscout::Vertex2D tr{0,0};
  osmscout::Vertex2D br{0,0};
  osmscout::Vertex2D bl{0,0};

  osmscout::Vertex2D trPosition{0,0}; //!< top-left position after rotation
  double trWidth{0};                  //!< width after rotation
  double trHeight{0};                 //!< height after rotation
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

};

template<class NativeGlyph>
class ContourLabel
{
public:
  size_t priority;
  std::vector<Glyph<NativeGlyph>> glyphs;
};

namespace {
  class Mask
  {
  public:
    Mask(size_t rowSize) : d(rowSize)
    {
      //std::cout << "create " << this << std::endl;
    };

    Mask(const Mask &m) :
        d(m.d), cellFrom(m.cellFrom), cellTo(m.cellTo), rowFrom(m.rowFrom), rowTo(m.rowTo)
    {
      //std::cout << "create(2) " << this << std::endl;
    };

    ~Mask()
    {
      //std::cout << "delete " << this << std::endl;
    }

    Mask(Mask &&m) = delete;
    Mask &operator=(const Mask &m) = delete;
    Mask &operator=(Mask &&m) = delete;

    void prepare(const IntRectangle &rect);

    inline int64_t size() const
    { return d.size(); };

    std::vector<uint64_t> d;

    int cellFrom{0};
    int cellTo{0};
    int rowFrom{0};
    int rowTo{0};
  };

  void Mask::prepare(const IntRectangle &rect)
  {
    cellFrom = rect.x / 64;
    int cellFromBit = rect.x % 64;
    cellTo = (rect.x + rect.width) / 64;
    int cellToBit = (rect.x + rect.width) % 64;
    rowFrom = rect.y;
    rowTo = rect.y + rect.height;

    if (cellFromBit<0){
      cellFrom--;
      cellFromBit=64+cellFromBit;
    }
    if (cellToBit<0){
      cellTo--;
      cellToBit=64+cellToBit;
    }

    uint64_t mask = ~0;
    for (int c = std::max(0, cellFrom); c <= std::min((int) d.size() - 1, cellTo); c++) {
      d[c] = mask;
    }
    if (cellFrom >= 0 && cellFrom < size()) {
      d[cellFrom] = d[cellFrom] >> cellFromBit;
    }
    if (cellTo >= 0 && cellTo < size()) {
      d[cellTo] = d[cellTo] << (64 - cellToBit);
    }
  }
}

template <class NativeGlyph>
osmscout::Vertex2D glyphTopLeft(const NativeGlyph &glyph);

template <class NativeGlyph>
double glyphWidth(const NativeGlyph &glyph);

template <class NativeGlyph>
double glyphHeight(const NativeGlyph &glyph);

template <class NativeGlyph, class NativeLabel, class TextLayouter>
class OSMSCOUT_MAP_API LabelLayouter
{

public:
  using ContourLabelType = ContourLabel<NativeGlyph>;
  using LabelType = Label<NativeGlyph, NativeLabel>;
  using LabelInstanceType = LabelInstance<NativeGlyph, NativeLabel>;

public:
  LabelLayouter(TextLayouter *textLayouter):
      textLayouter(textLayouter)
  {};

  void reset()
  {
    contourLabelInstances.clear();
    labelInstances.clear();
  }

  inline bool checkLabelCollision(const std::vector<uint64_t> &canvas,
                           const Mask &mask,
                           int64_t viewportHeight
  )
  {
    bool collision=false;
    for (int r=std::max(0,mask.rowFrom); !collision && r<=std::min((int)viewportHeight-1, mask.rowTo); r++){
      for (int c=std::max(0,mask.cellFrom); !collision && c<=std::min((int)mask.size()-1,mask.cellTo); c++){
        collision |= (mask.d[c] & canvas[r*mask.size() + c]) != 0;
      }
    }
    return collision;
  }

  inline void markLabelPlace(std::vector<uint64_t> &canvas,
                             const Mask &mask,
                             int viewportHeight
  )
  {
    for (int r=std::max(0,mask.rowFrom); r<=std::min((int)viewportHeight-1, mask.rowTo); r++){
      for (int c=std::max(0,mask.cellFrom); c<=std::min((int)mask.size()-1, mask.cellTo); c++){
        canvas[r*mask.size() + c] = mask.d[c] | canvas[r*mask.size() + c];
      }
    }
  }

  void layout(int viewportWidth, int viewportHeight)
  {
    std::vector<ContourLabelType> allSortedContourLabels;
    std::vector<LabelInstanceType> allSortedLabels;

    std::swap(allSortedLabels, labelInstances);
    std::swap(allSortedContourLabels, contourLabelInstances);

    // TODO: sort labels by priority and position (to be deterministic)

    // compute collisions, hide some labels
    int64_t rowSize = (viewportWidth / 64)+1;
    //int64_t binaryWidth = rowSize * 8;
    //size_t binaryHeight = (viewportHeight / 8)+1;
    std::vector<uint64_t> canvas((size_t)(rowSize*viewportHeight));
    //canvas.data()

    auto labelIter = allSortedLabels.begin();
    auto contourLabelIter = allSortedContourLabels.begin();
    while (labelIter != allSortedLabels.end()
        || contourLabelIter != allSortedContourLabels.end()) {

      auto currentLabel = labelIter;
      auto currentContourLabel = contourLabelIter;
      if (currentLabel != allSortedLabels.end()
       && currentContourLabel != allSortedContourLabels.end()) {
        if (currentLabel->priority != currentContourLabel->priority) {
          if (currentLabel->priority < currentContourLabel->priority) {
            currentContourLabel = allSortedContourLabels.end();
          } else {
            currentLabel = allSortedLabels.end();
          }
        }
      }

      if (currentLabel != allSortedLabels.end()){

        IntRectangle rectangle{
          (int)currentLabel->x,
          (int)currentLabel->y,
          (int)currentLabel->label.width,
          (int)currentLabel->label.height,
        };
        Mask row(rowSize);

        row.prepare(rectangle);

        bool collision=checkLabelCollision(canvas, row, viewportHeight);
        if (!collision) {
          markLabelPlace(canvas, row, viewportHeight);
          labelInstances.push_back(*currentLabel);
        }

        labelIter++;
      }

      if (currentContourLabel != allSortedContourLabels.end()){
        int glyphCnt=currentContourLabel->glyphs.size();
        //std::vector<uint64_t> rowBuff((size_t)(rowSize * glyphCnt));
        Mask m(rowSize);
        std::vector<Mask> masks(glyphCnt, m);
        bool collision=false;
        for (int gi=0; !collision && gi<glyphCnt; gi++) {
          //uint64_t *row=rowBuff.data() + (gi*rowSize);

          auto glyph=currentContourLabel->glyphs[gi];
          IntRectangle rect{
            (int)glyph.trPosition.GetX(),
            (int)glyph.trPosition.GetY(),
            (int)glyph.trWidth,
            (int)glyph.trHeight
          };
          masks[gi].prepare(rect);
          collision |= checkLabelCollision(canvas, masks[gi], viewportHeight);
        }
        if (!collision) {
          for (int gi=0; gi<glyphCnt; gi++) {
            markLabelPlace(canvas, masks[gi], viewportHeight);
          }
          contourLabelInstances.push_back(*currentContourLabel);
        }
        contourLabelIter++;
      }
    }
  }

  void registerLabel(QPointF point,
                     std::string string,
                     double proposedWidth = 5000.0)
  {
    int fontHeight=18;
    LabelInstanceType instance;

    instance.label = textLayouter->layout(string, fontHeight, proposedWidth);

    instance.id = 0;
    instance.priority = 0;

    instance.x = point.x() - instance.label.width/2;
    instance.y = point.y() - instance.label.height/2;
    instance.alpha = 1.0;

    labelInstances.push_back(instance);
  }

  void registerContourLabel(std::vector<QPointF> way,
                            std::string string)
  {
    // TODO: parameters
    int fontHeight=24;
    int textOffset=fontHeight / 3;

    // TODO: cache simplified path for way id
    osmscout::SimplifiedPath p;
    for (auto const &point:way){
      p.AddPoint(point.x(), point.y());
    }

    // TODO: cache label for string and font parameters
    LabelType label = textLayouter->layout(string, fontHeight, /* proposed width */ 5000.0);
    std::vector<Glyph<NativeGlyph>> glyphs = label.toGlyphs();

    double pLength=p.GetLength();
    double offset=0;
    while (offset<pLength){
      ContourLabelType cLabel;
      cLabel.priority = 1;
      for (Glyph<NativeGlyph> glyphCopy:glyphs){
        double glyphOffset=offset+glyphCopy.position.GetX();
        osmscout::Vertex2D point=p.PointAtLength(glyphOffset);
        qreal   angle=p.AngleAtLength(glyphOffset)*-1;
        double  sinA=std::sin(angle);
        double  cosA=std::cos(angle);

        glyphCopy.position=osmscout::Vertex2D(point.GetX() - textOffset * sinA,
                                              point.GetY() + textOffset * cosA);

        glyphCopy.angle=angle;

        double w=glyphWidth(glyphCopy.glyph);
        double h=glyphHeight(glyphCopy.glyph);
        auto tl=glyphTopLeft(glyphCopy.glyph);

        // four coordinates of glyph bounding box; x,y of top-left, top-right, bottom-right, bottom-left
        std::array<double, 4> x{tl.GetX(), tl.GetX()+w, tl.GetX()+w, tl.GetX()};
        std::array<double, 4> y{tl.GetY(), tl.GetY(), tl.GetY()+h, tl.GetY()+h};

        // rotate
        for (int i=0; i<4; i++){
          double tmp;
          tmp  = x[i] * cosA - y[i] * sinA;
          y[i] = x[i] * sinA + y[i] * cosA;
          x[i] = tmp;
        }
        glyphCopy.tl.Set(x[0]+glyphCopy.position.GetX(), y[0]+glyphCopy.position.GetY());
        glyphCopy.tr.Set(x[1]+glyphCopy.position.GetX(), y[1]+glyphCopy.position.GetY());
        glyphCopy.br.Set(x[2]+glyphCopy.position.GetX(), y[2]+glyphCopy.position.GetY());
        glyphCopy.bl.Set(x[3]+glyphCopy.position.GetX(), y[3]+glyphCopy.position.GetY());

        // bounding box
        double minX=x[0];
        double maxX=x[0];
        double minY=y[0];
        double maxY=y[0];
        for (int i=1; i<4; i++){
          minX = std::min(minX, x[i]);
          maxX = std::max(maxX, x[i]);
          minY = std::min(minY, y[i]);
          maxY = std::max(maxY, y[i]);
        }
        // setup glyph top-left position and dimension after rotation
        glyphCopy.trPosition.Set(minX+glyphCopy.position.GetX(), minY+glyphCopy.position.GetY());
        glyphCopy.trWidth  = maxX - minX;
        glyphCopy.trHeight = maxY - minY;

        cLabel.glyphs.push_back(glyphCopy);
      }
      contourLabelInstances.push_back(cLabel);
      offset+=label.width + 3*fontHeight;
    }
  }

  std::vector<LabelInstanceType> labels() const
  {
    return labelInstances;
  }

  std::vector<ContourLabelType> contourLabels() const
  {
    return contourLabelInstances;
  }

private:
  TextLayouter *textLayouter;
  std::vector<ContourLabelType> contourLabelInstances;
  std::vector<LabelInstanceType> labelInstances;
};


using QtGlyph = Glyph<QGlyphRun>;
using QtLabel = Label<QGlyphRun, std::shared_ptr<QTextLayout>>;
using QtLabelInstance = LabelInstance<QGlyphRun, std::shared_ptr<QTextLayout>>;

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

    /*
    QList<QTextLayout::FormatRange> formatList;
    QTextLayout::FormatRange        range;

    range.start=0;
    range.length=text.length();
    range.format.setForeground(QBrush(QColor(0,0,0)));
    formatList.append(range);

    label.label->setAdditionalFormats(formatList);
     */

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

double glyphWidth(const QGlyphRun &glyph)
{
  return glyph.boundingRect().width();
}

double glyphHeight(const QGlyphRun &glyph)
{
  return glyph.boundingRect().height();
}

osmscout::Vertex2D glyphTopLeft(const QGlyphRun &glyph)
{
  auto tl=glyph.boundingRect().topLeft();
  return osmscout::Vertex2D(tl.x(),tl.y());
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
    : QWidget(parent), variant(variant), sinCount(sinCount), cnt(0), startOffset(0), moveOffset()
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
  const QTransform originalTran=painter->transform();
  QPointF point=QPointF(glyph.position.GetX(), glyph.position.GetY());
  //qreal   angle=(glyph.angle * 180) / M_PI;
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
  tran.rotateRadians(glyph.angle);


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

  /*
  std::vector<QPointF> way;
  QPointF center(width()/2,height()/2);
  way.push_back(center);
  way.push_back(center + 100*QPointF(std::cos((double)startOffset/10.0), std::sin((double)startOffset/10.0)));
  layouter->registerContourLabel(way, "A");
  drawLine(&painter, way);
  */

  layouter->registerLabel(QPointF(60, 30), "Test label");

  layouter->registerLabel(QPointF(300, 100),
                          "Lorem ipsum dolor sit amet, consectetuer adipiscing elit.",
                          250);
  layouter->registerLabel(QPointF(200, 80), "Collision test");

  layouter->layout(width(), height());

  for (const ContourLabel<QGlyphRun> &label:layouter->contourLabels()){
    for (const Glyph<QGlyphRun> &glyph:label.glyphs){
      drawGlyph(&painter, glyph);

      QPen pen(QColor::fromRgbF(0,1,0));
      pen.setWidthF(0.8);
      painter.setPen(pen);
      painter.drawRect(glyph.trPosition.GetX(), glyph.trPosition.GetY(), glyph.trWidth, glyph.trHeight);

      pen.setColor(QColor::fromRgbF(1,0,0));
      painter.setPen(pen);
      painter.drawLine(glyph.tl.GetX(), glyph.tl.GetY(), glyph.tr.GetX(), glyph.tr.GetY());
      painter.drawLine(glyph.tr.GetX(), glyph.tr.GetY(), glyph.br.GetX(), glyph.br.GetY());
      painter.drawLine(glyph.br.GetX(), glyph.br.GetY(), glyph.bl.GetX(), glyph.bl.GetY());
      painter.drawLine(glyph.bl.GetX(), glyph.bl.GetY(), glyph.tl.GetX(), glyph.tl.GetY());
    }
  }

  for (const QtLabelInstance inst : layouter->labels()){
    painter.setPen(QColor::fromRgbF(0,0,0));
    QTextLayout *tl = inst.label.label.get();
    tl->draw(&painter, QPointF(inst.x, inst.y));

    painter.setPen(QColor::fromRgbF(0,1,0));
    painter.drawRect(QRectF(QPointF(inst.x, inst.y), QSizeF(inst.label.width, inst.label.height)));
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

