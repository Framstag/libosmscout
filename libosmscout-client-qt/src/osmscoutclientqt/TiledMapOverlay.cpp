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

#include <osmscoutclientqt/TiledMapOverlay.h>
#include <osmscoutclientqt/OsmTileDownloader.h>
#include <osmscoutclientqt/OSMScoutQt.h>
#include <osmscoutclientqt/TiledRenderingHelper.h>

#include <osmscoutclient/json/json.hpp>

#include <QJsonDocument>

namespace osmscout {

TileLoaderThread::TileLoaderThread(QThread *thread):
  thread(thread),
  tileDownloader(nullptr),
  onlineTileCache(OSMScoutQt::GetInstance().GetOnlineTileCacheSize())
{
  qDebug() << "Overlay tile cache:" << &onlineTileCache;
}

TileLoaderThread::~TileLoaderThread()
{
  if (thread!=nullptr){
    thread->quit();
  }
  if (tileDownloader!=nullptr){
    delete tileDownloader;
  }
}

void TileLoaderThread::init()
{
  // create tile downloader in correct thread
  SettingsRef settings=OSMScoutQt::GetInstance().GetSettings();
  QString tileCacheDirectory=OSMScoutQt::GetInstance().GetCacheLocation();
  tileDownloader = new OsmTileDownloader(tileCacheDirectory,provider);

  connect(tileDownloader, &OsmTileDownloader::failed,
          this, &TileLoaderThread::tileDownloadFailed,
          Qt::QueuedConnection);
  connect(tileDownloader, &OsmTileDownloader::downloaded,
          this, &TileLoaderThread::tileDownloaded,
          Qt::QueuedConnection);

  connect(&onlineTileCache, &TileCache::tileRequested,
          this, &TileLoaderThread::download,
          Qt::QueuedConnection);
}

void TileLoaderThread::download(uint32_t zoomLevel, uint32_t xtile, uint32_t ytile)
{
  if (tileDownloader == nullptr){
    qWarning() << "tile requested but downloader is not initialized yet";
    emit failed(zoomLevel, xtile, ytile);
  }else{
    QMutexLocker locker(&tileCacheMutex);
    if (onlineTileCache.startRequestProcess(zoomLevel, xtile, ytile)) {
      emit tileDownloader->download(zoomLevel, xtile, ytile);
    }
  }
}

void TileLoaderThread::onProviderChanged(const OnlineTileProvider &newProvider)
{
  QMutexLocker locker(&tileCacheMutex);
  onlineTileCache.clearPendingRequests();
  onlineTileCache.cleanupCache();

  provider=newProvider;
  if (tileDownloader!=nullptr){
    emit tileDownloader->onlineTileProviderChanged(provider);
  }
}

void TileLoaderThread::accessCache(std::function<void(TileCache&)> fn)
{
  QMutexLocker locker(&tileCacheMutex);
  fn(onlineTileCache);
}


void TileLoaderThread::tileDownloaded(uint32_t zoomLevel, uint32_t x, uint32_t y, QImage image, QByteArray /*downloadedData*/)
{
  {
    QMutexLocker locker(&tileCacheMutex);
    onlineTileCache.put(zoomLevel, x, y, image);
  }

  emit downloaded(zoomLevel, x, y);
}

void TileLoaderThread::tileDownloadFailed(uint32_t zoomLevel, uint32_t x, uint32_t y, bool zoomLevelOutOfRange)
{
  {
    QMutexLocker locker(&tileCacheMutex);
    onlineTileCache.removeRequest(zoomLevel, x, y);

    if (zoomLevelOutOfRange && zoomLevel > 0) {
      // hack: when zoom level is too high for online source,
      // we try to request tile with lower zoom level and put it to cache
      // as substitute
      uint32_t reqZoom = zoomLevel - 1;
      uint32_t reqX = x / 2;
      uint32_t reqY = y / 2;
      if ((!onlineTileCache.contains(reqZoom, reqX, reqY))
          && onlineTileCache.request(reqZoom, reqX, reqY)) {
        qDebug() << "Tile download failed" << x << y << "zoomLevel" << zoomLevel << "try lower zoom";
        //triggerTileRequest(reqZoom, reqX, reqY);
      }
    }
  }
  emit failed(zoomLevel, x, y);
}

TiledMapOverlay::TiledMapOverlay(QQuickItem* parent):
    MapOverlay(parent),
    enabled(true),
    transparentColor(QColor::fromRgbF(0,0,0,0))
{
  QThread *thread=OSMScoutQt::GetInstance().makeThread("OverlayTileLoader");

  loader=new TileLoaderThread(thread);
  loader->moveToThread(thread);
  connect(thread, &QThread::started,
          loader, &TileLoaderThread::init);
  thread->start();

  connect(this, &TiledMapOverlay::providerChanged,
          loader, &TileLoaderThread::onProviderChanged,
          Qt::QueuedConnection);

  connect(loader, &TileLoaderThread::downloaded,
          this, &TiledMapOverlay::tileDownloaded,
          Qt::QueuedConnection);
}

TiledMapOverlay::~TiledMapOverlay()
{
  if (loader!=nullptr){
    loader->deleteLater();
    loader=nullptr;
  }
}

void TiledMapOverlay::paint(QPainter *painter)
{
  if (!enabled || !view->IsValid()){
    return;
  }
  loader->accessCache([&](TileCache& onlineTileCache){

    QList<TileCache*> layerCaches;

    layerCaches << &onlineTileCache;
    onlineTileCache.clearPendingRequests();

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::TextAntialiasing, true);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);

    MapViewStruct request;
    QRectF        boundingBox = contentsBoundingRect();

    request.coord = view->center;
    request.angle = view->angle;
    request.magnification = view->magnification;
    request.width = boundingBox.width();
    request.height = boundingBox.height();
    request.dpi = view->mapDpi;

    TiledRenderingHelper::RenderTiles(*painter,request,layerCaches,transparentColor, /*overlap*/ 0, QColor::fromRgbF(0,0,0,0));
  });
}

void TiledMapOverlay::tileDownloaded(uint32_t /*zoomLevel*/, uint32_t /*x*/, uint32_t /*y*/)
{
  emit redraw();
}

QJsonValue TiledMapOverlay::getProvider()
{
  return providerJson;
}

void TiledMapOverlay::setProvider(QJsonValue jv)
{
  if (!jv.isObject()) {
    qWarning() << "Failed to parse providers json:" << jv;
    return;
  }

  OnlineTileProvider provider;
  try{
    auto jsonStr = QJsonDocument(jv.toObject()).toJson().toStdString();
    provider = OnlineTileProvider::fromJson(nlohmann::json::parse(jsonStr));
  } catch (const nlohmann::json::exception &e) {
    qWarning() << "Failed to parse providers json:" << e.what();
    return;
  }
  if (!provider.isValid()){
    qWarning() << "Invalid provider:" << jv;
    return;
  }

  providerJson=jv;
  emit providerChanged(provider);
  redraw();
}

bool TiledMapOverlay::isEnabled()
{
  return enabled;
}

void TiledMapOverlay::setEnabled(bool b)
{
  if (b==enabled){
    return;
  }
  enabled=b;
  if (!enabled){
    // cleanup cache to release memory
    loader->accessCache([&](TileCache& onlineTileCache) {
      onlineTileCache.cleanupCache();
    });
  }
  redraw();
}
}
