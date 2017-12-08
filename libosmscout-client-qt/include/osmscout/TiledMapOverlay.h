#ifndef LIBOSMSCOUT_TILEMAPOVERLAY_H
#define LIBOSMSCOUT_TILEMAPOVERLAY_H
/*
  OSMScout - a Qt backend for libosmscout and libosmscout-map
  Copyright (C) 2017 Lukáš Karas

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

#include <osmscout/MapOverlay.h>
#include <osmscout/TileCache.h>
#include <osmscout/OsmTileDownloader.h>

#include <osmscout/private/ClientQtImportExport.h>

#include <QImage>

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API TileLoaderThread: public QObject {
Q_OBJECT

private:
  QThread *thread;
  OsmTileDownloader *tileDownloader;
  OnlineTileProvider provider;

public slots:
  void init();
  void download(uint32_t, uint32_t, uint32_t);
  void onProviderChanged(const OnlineTileProvider &newProvider);

signals:
  void downloaded(uint32_t zoomLevel, uint32_t x, uint32_t y, QImage image, QByteArray downloadedData);
  void failed(uint32_t zoomLevel, uint32_t x, uint32_t y, bool zoomLevelOutOfRange);

public:
  TileLoaderThread(QThread *thread);

  virtual ~TileLoaderThread();
};

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API TiledMapOverlay : public MapOverlay
{
  Q_OBJECT
  Q_PROPERTY(QJsonValue provider READ getProvider WRITE setProvider)
  Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled)

private:
  mutable QMutex      tileCacheMutex;
  TileCache           onlineTileCache;
  QJsonValue          providerJson;
  TileLoaderThread    *loader;
  bool                enabled;
  QColor              transparentColor;

public slots:
  void tileDownloaded(uint32_t zoomLevel, uint32_t x, uint32_t y, QImage image, QByteArray downloadedData);
  void tileDownloadFailed(uint32_t zoomLevel, uint32_t x, uint32_t y, bool zoomLevelOutOfRange);
  void download(uint32_t zoomLevel, uint32_t xtile, uint32_t ytile);

signals:
  void providerChanged(const OnlineTileProvider &provider);

public:
  TiledMapOverlay(QQuickItem* parent = 0);
  virtual ~TiledMapOverlay();

  virtual void paint(QPainter *painter);

  QJsonValue getProvider();
  void setProvider(QJsonValue jv);

  bool isEnabled();
  void setEnabled(bool b);
};

#endif //LIBOSMSCOUT_TILEMAPOVERLAY_H
