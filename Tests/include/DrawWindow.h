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

#ifndef TEST_DRAWWINDOW_H
#define TEST_DRAWWINDOW_H

#include <QApplication>
#include <QDesktopWidget>
#include <QPixmap>
#include <QScreen>

#include <osmscout/Database.h>
#include <osmscout/MapService.h>

#include <osmscout/MapPainterQt.h>

#include <QtGui>

class DrawWindow : public QWidget
{
  Q_OBJECT
public:
  explicit DrawWindow(QString variant, int sinCount, QWidget *parent = 0);

  virtual ~DrawWindow();

protected:
  virtual void paintEvent(QPaintEvent *event);

  void setupTransformation(QPainter *painter, const QPainterPath &p, const qreal offset) const;

  void drawText1(QPainter *painter, QString string, QPainterPath p);
  void drawText2(QPainter *painter, QString string, QPainterPath p);

  QString                   variant;
  std::vector<double>       sin;           //! Lookup table for sin calculation
  int                       sinCount;
  int                       cnt;
  QTime                     timer;
};

#endif /* TEST_DRAWWINDOW_H */

