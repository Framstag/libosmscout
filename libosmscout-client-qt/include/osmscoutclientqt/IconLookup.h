#ifndef OSMSCOUT_CLIENT_QT_ICONLOOKUP_H
#define OSMSCOUT_CLIENT_QT_ICONLOOKUP_H

/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2022 Lukas Karas

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

#include <osmscoutmap/DataTileCache.h>

#include <osmscoutclientqt/DBThread.h>

#include <osmscoutclientqt/ClientQtImportExport.h>
#include <osmscoutclientqt/OverlayObject.h>

#include <QObject>
#include <QSettings>
#include <QMutex>

namespace osmscout {

/**
 * \ingroup QtAPI
 */
struct OSMSCOUT_CLIENT_QT_API MapIcon {
  QPoint screenCoord;
  QRectF dimensions;
  GeoCoord coord;
  double distanceSquare;
  IconStyleRef iconStyle;
  QString databasePath;
  ObjectFileRef objectRef;
  int poiId;
  QString type;
  QString name;
  QString phone;
  QString website;
  QImage image;
};

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API IconLookup : public QObject {
  Q_OBJECT
private:
  static constexpr int iconImageUpscale=3;
  static constexpr double tapSize=4;
  QThread     *thread;
  DBThreadRef dbThread;
  DBLoadJob   *loadJob;
  osmscout::MercatorProjection projection;
  std::map<int,OverlayObjectRef> overlayObjects;
  MapParameter drawParameter;
  QPoint lookupCoord;
  std::vector<MapIcon> findIcons;

public slots:
  void onIconRequest(const MapViewStruct &view,
                     const QPoint &coord,
                     const std::map<int,OverlayObjectRef> &overlayObjects);
  void onDatabaseLoaded(QString dbPath,QList<osmscout::TileRef> tiles);
  void onLoadJobFinished(QMap<QString,QMap<osmscout::TileKey,osmscout::TileRef>> tiles);

public:
signals:
  void iconRequested(const MapViewStruct &view,
                     const QPoint &coord,
                     const std::map<int,OverlayObjectRef> &overlayObjects);

  void iconFound(QPoint lookupCoord, MapIcon icon);
  void iconNotFound(QPoint lookupCoord);

public:
  IconLookup(QThread *thread, DBThreadRef dbThread, QString iconDirectory);
  ~IconLookup() override;

  void RequestIcon(const MapViewStruct &view,
                   const QPoint &coord,
                   const std::map<int,OverlayObjectRef> &overlayObjects);

private:
  void lookupIcons(const QString &databasePath,
                   osmscout::MapData &data,
                   const TypeConfigRef &typeConfig,
                   const StyleConfigRef &styleConfig);
};

/**
 * \ingroup QtAPI
 */
typedef std::shared_ptr<IconLookup> IconLookupRef;

}
#endif //OSMSCOUT_CLIENT_QT_ICONLOOKUP_H
