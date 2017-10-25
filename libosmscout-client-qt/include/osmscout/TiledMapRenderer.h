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


#ifndef TILEDMAPRENDERER_H
#define TILEDMAPRENDERER_H

#include <QObject>
#include <QSettings>

#include <osmscout/DataTileCache.h>
#include <osmscout/DBThread.h>
#include <osmscout/MapRenderer.h>

#include <osmscout/private/ClientQtImportExport.h>

#include <atomic>

class OSMSCOUT_CLIENT_QT_API TiledMapRenderer : public MapRenderer {
  Q_OBJECT

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
  uint32_t                      loadZ;

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
  void onLoadJobFinished(QMap<QString,QMap<osmscout::TileId,osmscout::TileRef>>);

  void onlineTileProviderChanged();
  void onlineTilesEnabledChanged(bool);

  void onOfflineMapChanged(bool);

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

#endif /* TILEDMAPRENDERER_H */
