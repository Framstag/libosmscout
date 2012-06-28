#ifndef MAPWIDGET_H
#define MAPWIDGET_H

/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2010  Tim Teulings

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

#include <QtGui>

#include "DBThread.h"

class MapWidget: public QWidget
{
  Q_OBJECT


private:
  double lon;
  double lat;
  double magnification;

  // Drag and drop
  double startLon,startLat;
  int    startX,startY;
  bool   requestNewMap;

signals:
  void TriggerMapRenderingSignal();

public slots:
  void InitialisationFinished(const DatabaseLoadedResponse& response);
  void DrawRenderResult();
  void Redraw();

private:
  void TriggerMapRendering();

  void HandleMouseMove(QMouseEvent* event);

public:
  MapWidget(QWidget *parent=NULL);
  virtual ~MapWidget();

  void keyPressEvent(QKeyEvent* event);
  void mousePressEvent(QMouseEvent* event);
  void mouseMoveEvent(QMouseEvent* event);
  void mouseReleaseEvent(QMouseEvent* event);
  void wheelEvent(QWheelEvent* event);
  void paintEvent(QPaintEvent* event);
  void resizeEvent(QResizeEvent* event);

  void ZoomIn(double zoomFactor);
  void ZoomOut(double zoomFactor);
  void ShowReference(const osmscout::ObjectRef& reference,
                     const osmscout::Mag& magnification);

};

#endif
