#ifndef OSMSCOUT_CLIENT_QT_TILED_DBTHREAD_H
#define OSMSCOUT_CLIENT_QT_TILED_DBTHREAD_H

/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2010  Tim Teulings
 Copyright (C) 2016  Lukáš Karas

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

#include <QtGui>
#include <QThread>
#include <QMetaType>
#include <QMutex>
#include <QTime>
#include <QTimer>

#include <osmscout/Database.h>
#include <osmscout/LocationService.h>
#include <osmscout/MapService.h>
#include <osmscout/RoutingService.h>
#include <osmscout/RoutePostprocessor.h>

#include <osmscout/MapPainterQt.h>

#include <osmscout/util/Breaker.h>

#include <osmscout/Settings.h>
#include <osmscout/TileCache.h>
#include <osmscout/OsmTileDownloader.h>
#include <osmscout/DBThread.h>

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API TiledDBThread : public DBThread
{
  Q_OBJECT

public slots:
  void DrawMap(QPainter &p, const osmscout::GeoCoord center, uint32_t z,
        size_t width, size_t height, size_t lookupWidth, size_t lookupHeight);
  void onlineTileRequest(uint32_t zoomLevel, uint32_t xtile, uint32_t ytile);
  void offlineTileRequest(uint32_t zoomLevel, uint32_t xtile, uint32_t ytile);
  void tileDownloaded(uint32_t zoomLevel, uint32_t x, uint32_t y, QImage image, QByteArray downloadedData);
  void tileDownloadFailed(uint32_t zoomLevel, uint32_t x, uint32_t y, bool zoomLevelOutOfRange);  
  void onDatabaseLoaded(osmscout::GeoBox boundingBox);

  void onStylesheetFilenameChanged();

  void onlineTileProviderChanged();
  void onlineTilesEnabledChanged(bool);

  void onOfflineMapChanged(bool);
  virtual void Initialize();

private:
  QString                       tileCacheDirectory;

  // tile caches
  // Rendered tile is combined from both sources.
  //
  // Online cache may contain NULL images (QImage::isNull() is true) for areas
  // covered by offline database and offline cache can contain NULL images 
  // for areas not coverred by database.
  // 
  // Offline tiles should be in ARGB format on database area interface.
  mutable QMutex                tileCacheMutex;
  TileCache                     onlineTileCache;
  TileCache                     offlineTileCache;
  
  OsmTileDownloader             *tileDownloader;

  bool                          onlineTilesEnabled;
  bool                          offlineTilesEnabled;

  int                           screenWidth;
  int                           screenHeight;

public:
  TiledDBThread(QStringList databaseLookupDirectories, 
                QString iconDirectory,
                SettingsRef renderingSettings,
                QString tileCacheDirectory,
                size_t onlineTileCacheSize = 20, 
                size_t offlineTileCacheSize = 50);

  virtual ~TiledDBThread();
  
  /**
   * 
   * @param painter
   * @param request
   * @return true if rendered map is complete (queue of tile render requests is empty)
   */
  virtual bool RenderMap(QPainter& painter,
                         const RenderMapRequest& request);
  
  virtual void InvalidateVisualCache();

private:

  /**
   * lookup tile in cache, if not found, try upper zoom level for substitute. 
   * (It is better upscaled tile than empty space)
   * Is is repeated up to zoomLevel - upLimit
   */
  bool lookupAndDrawTile(TileCache& tileCache, QPainter& painter, 
        double x, double y, double renderTileWidth, double renderTileHeight, 
        uint32_t zoomLevel, uint32_t xtile, uint32_t ytile, 
        uint32_t upLimit, uint32_t downLimit);

  void lookupAndDrawBottomTileRecursive(TileCache& tileCache, QPainter& painter, 
        double x, double y, double renderTileWidth, double renderTileHeight, double overlap,
        uint32_t zoomLevel, uint32_t xtile, uint32_t ytile, 
        uint32_t downLimit);

  DatabaseCoverage databaseCoverageOfTile(uint32_t zoomLevel, uint32_t xtile, uint32_t ytile);
};

#endif
