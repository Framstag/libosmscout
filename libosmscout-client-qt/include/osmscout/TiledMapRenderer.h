#ifndef OSMSCOUT_CLIENT_QT_TILEDMAPRENDERER_H
#define OSMSCOUT_CLIENT_QT_TILEDMAPRENDERER_H

/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2010  Tim Teulings
 Copyright (C) 2017 Lukas Karas

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

#include <QObject>
#include <QSettings>

#include <osmscout/DataTileCache.h>
#include <osmscout/DBThread.h>
#include <osmscout/MapRenderer.h>

#include <osmscout/ClientQtImportExport.h>

#include <atomic>

namespace osmscout {

class OSMSCOUT_CLIENT_QT_API TiledMapRenderer : public MapRenderer {
  Q_OBJECT

private:
  QString                       tileCacheDirectory;

  // tile caches
  // Rendered tile is combined from both sources.
  //
  // Online cache may contain NULL images (QImage::isNull() is true) for areas
  // covered by offline database and offline cache can contain NULL images
  // for areas not covered by database.
  //
  // When offlineTileCache is invalidated, cache keeps unchanged,
  // just its epoch is increased. When there is retrieved pixmap with
  // old epoch from cache, it is used, but rendering request is triggered.
  //
  // Offline tiles should be in ARGB format on database area interface.
  mutable QMutex                tileCacheMutex;
  TileCache                     onlineTileCache;
  TileCache                     offlineTileCache;

  OsmTileDownloader             *tileDownloader;

  std::atomic_bool              onlineTilesEnabled;
  std::atomic_bool              offlineTilesEnabled;

  int                           screenWidth;
  int                           screenHeight;

  // data loading request
  DBLoadJob                     *loadJob;
  uint32_t                      loadXFrom;
  uint32_t                      loadXTo;
  uint32_t                      loadYFrom;
  uint32_t                      loadYTo;
  MagnificationLevel            loadZ;
  size_t                        loadEpoch; // guarded by lock

  QColor                        unknownColor;

public slots:
  virtual void Initialize();
  virtual void InvalidateVisualCache();
  virtual void onStylesheetFilenameChanged();

  void onlineTileRequest(uint32_t zoomLevel, uint32_t xtile, uint32_t ytile);
  void offlineTileRequest(uint32_t zoomLevel, uint32_t xtile, uint32_t ytile);
  void tileDownloaded(uint32_t zoomLevel, uint32_t x, uint32_t y, QImage image, QByteArray downloadedData);
  void tileDownloadFailed(uint32_t zoomLevel, uint32_t x, uint32_t y, bool zoomLevelOutOfRange);
  void onDatabaseLoaded(osmscout::GeoBox boundingBox);
  void onLoadJobFinished(QMap<QString,QMap<osmscout::TileKey,osmscout::TileRef>>);

  void onlineTileProviderChanged();
  void onlineTilesEnabledChanged(bool);

  void onOfflineMapChanged(bool);

private:

  DatabaseCoverage databaseCoverageOfTile(uint32_t zoomLevel, uint32_t xtile, uint32_t ytile);

public:
  TiledMapRenderer(QThread *thread,
                   SettingsRef settings,
                   DBThreadRef dbThread,
                   QString iconDirectory,
                   QString tileCacheDirectory,
                   size_t onlineTileCacheSize,
                   size_t offlineTileCacheSize);

  virtual ~TiledMapRenderer();

  /**
   * Render map defined by request to painter
   * @param painter
   * @param request
   * @return true if rendered map is complete
   */
  virtual bool RenderMap(QPainter& painter,
                         const MapViewStruct& request);
};

}

#endif /* OSMSCOUT_CLIENT_QT_TILEDMAPRENDERER_H */
