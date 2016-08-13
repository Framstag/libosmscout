#ifndef OSMSCOUT_CLIENT_QT_MAPWIDGET_H
#define OSMSCOUT_CLIENT_QT_MAPWIDGET_H

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
#include <osmscout/util/GeoBox.h>

#include <osmscout/DBThread.h>

#include <osmscout/private/ClientQtImportExport.h>

#include <osmscout/SearchLocationModel.h>

class OSMSCOUT_CLIENT_QT_API MapWidget : public QQuickPaintedItem
{
  Q_OBJECT
  Q_PROPERTY(double lat READ GetLat)
  Q_PROPERTY(double lon READ GetLon)
  Q_PROPERTY(int zoomLevel READ zoomLevel NOTIFY zoomLevelChanged)
  Q_PROPERTY(QString zoomLevelName READ zoomLevelName NOTIFY zoomLevelNameChanged)
  Q_PROPERTY(QString stylesheetFilename READ stylesheetFilename NOTIFY stylesheetFilenameChanged)
  Q_PROPERTY(bool stylesheetHasErrors READ stylesheetHasErrors WRITE setStylesheetHasErrors NOTIFY stylesheetHasErrorsChanged)
  Q_PROPERTY(int stylesheetErrorLine READ stylesheetErrorLine CONSTANT)
  Q_PROPERTY(int stylesheetErrorColumn READ stylesheetErrorColumn CONSTANT)
  Q_PROPERTY(QString stylesheetErrorDescription READ stylesheetErrorDescription CONSTANT)

private:
  osmscout::GeoCoord           center;
  double                       angle;
  osmscout::Magnification      magnification;

  // Drag and drop
  int                          startX;
  int                          startY;
  osmscout::MercatorProjection startProjection;

  // Controlling rerendering...
  bool                         mouseDragging;
  bool                         dbInitialized;
  bool                         hasBeenPainted;

  // Errors in stylesheet
  int                           errorLine;
  int                           errorColumn;
  bool                          hasErrors;
  QString                       errorDescription;

signals:
  void TriggerMapRenderingSignal(const RenderMapRequest& request);
  void latChanged();
  void lonChanged();
  void zoomLevelChanged();
  void zoomLevelNameChanged();
  void stylesheetFilenameChanged();
  void stylesheetHasErrorsChanged();

public slots:
  void initialisationFinished(const DatabaseLoadedResponse& response);
  void redraw();
  void zoomIn(double zoomFactor);
  void zoomOut(double zoomFactor);
  void left();
  void right();
  void up();
  void down();
  void rotateLeft();
  void rotateRight();

  void toggleDaylight();
  void reloadStyle();
  void reloadTmpStyle();

  void showCoordinates(double lat, double lon);
  void showLocation(Location* location);

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

  inline bool stylesheetHasErrors() const
  {
      return hasErrors;
  }
  void setStylesheetHasErrors(bool value) {
    if(value != hasErrors){
        hasErrors = value;
        emit stylesheetHasErrorsChanged();
    }
  }
  inline int stylesheetErrorLine() const
  {
      return errorLine;
  }
  inline int stylesheetErrorColumn() const
  {
      return errorColumn;
  }
  inline const QString &stylesheetErrorDescription() const
  {
      return errorDescription;
  }

  void mousePressEvent(QMouseEvent* event);
  void mouseMoveEvent(QMouseEvent* event);
  void mouseReleaseEvent(QMouseEvent* event);
  void wheelEvent(QWheelEvent* event);

  void paint(QPainter *painter);
};

#endif
