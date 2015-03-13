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

#include <QQuickPaintedItem>

#include <osmscout/GeoCoord.h>

#include "DBThread.h"
#include "SearchLocationModel.h"

class MapWidget : public QQuickPaintedItem
{
  Q_OBJECT
  Q_PROPERTY(double lat READ GetLat)
  Q_PROPERTY(double lon READ GetLon)
  Q_PROPERTY(int zoomLevel READ zoomLevel NOTIFY zoomLevelChanged)
  Q_PROPERTY(QString zoomLevelName READ zoomLevelName NOTIFY zoomLevelNameChanged)
  Q_PROPERTY(QString stylesheetFilename READ stylesheetFilename NOTIFY stylesheetFilenameChanged)

private:
  osmscout::GeoCoord      center;
  osmscout::Magnification magnification;

  // Drag and drop
  double                  startLon,startLat;
  int                     startX,startY;

  // Controlling rerendering...
  bool                    requestNewMap;

signals:
  void TriggerMapRenderingSignal();
  void latChanged();
  void lonChanged();
  void zoomLevelChanged();
  void zoomLevelNameChanged();
  void stylesheetFilenameChanged();

public slots:
  void initialisationFinished(const DatabaseLoadedResponse& response);
  void redraw();
  void zoomIn(double zoomFactor);
  void zoomOut(double zoomFactor);
  void left();
  void right();
  void up();
  void down();
  void showCoordinates(double lat, double lon);
  void showLocation(Location* location);
  void reloadStyle();
  void reloadTmpStyle();

private:
  void TriggerMapRendering();

  void HandleMouseMove(QMouseEvent* event);

public:
  MapWidget(QQuickItem* parent = 0);
  virtual ~MapWidget();

  inline double GetLat() const
  {
      return center.GetLat();
  }

  inline double GetLon() const
  {
      return center.GetLon();
  }

  inline double zoomLevel() const
  {
      return magnification.GetLevel();
  }

  QString zoomLevelName();

  QString stylesheetFilename();

  void mousePressEvent(QMouseEvent* event);
  void mouseMoveEvent(QMouseEvent* event);
  void mouseReleaseEvent(QMouseEvent* event);
  void wheelEvent(QWheelEvent* event);

  void paint(QPainter *painter);
};

#endif
