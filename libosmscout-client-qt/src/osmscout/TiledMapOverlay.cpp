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

#include <osmscout/TiledMapOverlay.h>
#include <osmscout/OsmTileDownloader.h>
#include <osmscout/OSMScoutQt.h>
#include <osmscout/TiledRenderingHelper.h>


TileLoaderThread::TileLoaderThread(QThread *thread): thread(thread), tileDownloader(NULL) {}

TileLoaderThread::~TileLoaderThread()
{
  if (thread!=NULL){
    thread->quit();
  }
  if (tileDownloader!=NULL){
    delete tileDownloader;
  }
}

void TileLoaderThread::init()
{
  // create tile downloader in correct thread
  SettingsRef settings=OSMScoutQt::GetInstance().GetSettings();
  QString tileCacheDirectory=OSMScoutQt::GetInstance().GetCacheLocation();
  tileDownloader = new OsmTileDownloader(tileCacheDirectory,provider);

  connect(tileDownloader, SIGNAL(failed(uint32_t, uint32_t, uint32_t, bool)),
          this, SIGNAL(failed(uint32_t, uint32_t, uint32_t, bool)));
  connect(tileDownloader, SIGNAL(downloaded(uint32_t, uint32_t, uint32_t, QImage, QByteArray)),
          this, SIGNAL(downloaded(uint32_t, uint32_t, uint32_t, QImage, QByteArray)));
}

void TileLoaderThread::download(uint32_t zoomLevel, uint32_t xtile, uint32_t ytile)
{
  if (tileDownloader == NULL){
    qWarning() << "tile requested but downloader is not initialized yet";
    emit failed(zoomLevel, xtile, ytile, false);
  }else{
    emit tileDownloader->download(zoomLevel, xtile, ytile);
  }
}

void TileLoaderThread::onProviderChanged(const OnlineTileProvider &newProvider)
{
  provider=newProvider;
  if (tileDownloader!=NULL){
    emit tileDownloader->onlineTileProviderChanged(provider);
  }
}

TiledMapOverlay::TiledMapOverlay(QQuickItem* parent):
    MapOverlay(parent),
    onlineTileCache(OSMScoutQt::GetInstance().GetOnlineTileCacheSize()),
    enabled(true),
    transparentColor(QColor::fromRgbF(0,0,0,0))
{
  QThread *thread=OSMScoutQt::GetInstance().makeThread("OverlayTileLoader");

  loader=new TileLoaderThread(thread);
  loader->moveToThread(thread);
  connect(thread, SIGNAL(started()),
          loader, SLOT(init()));
  thread->start();

  connect(this, SIGNAL(providerChanged(const OnlineTileProvider &)),
          loader, SLOT(onProviderChanged(const OnlineTileProvider &)),
          Qt::QueuedConnection);

  connect(loader, SIGNAL(downloaded(uint32_t, uint32_t, uint32_t, QImage, QByteArray)),
          this, SLOT(tileDownloaded(uint32_t, uint32_t, uint32_t, QImage, QByteArray)),
          Qt::QueuedConnection);

  connect(loader, SIGNAL(failed(uint32_t, uint32_t, uint32_t, bool)),
          this, SLOT(tileDownloadFailed(uint32_t, uint32_t, uint32_t, bool)),
          Qt::QueuedConnection);

  connect(&onlineTileCache,SIGNAL(tileRequested(uint32_t, uint32_t, uint32_t)),
          loader,SLOT(download(uint32_t, uint32_t, uint32_t)),
          Qt::QueuedConnection);
}

TiledMapOverlay::~TiledMapOverlay()
{
  if (loader!=NULL){
    delete loader;
  }
}

void TiledMapOverlay::paint(QPainter *painter)
{
  if (!enabled || !view->IsValid()){
    return;
  }
  QMutexLocker locker(&tileCacheMutex);
  QList<TileCache*> layerCaches;

  layerCaches << &onlineTileCache;
  onlineTileCache.clearPendingRequests();

  painter->setRenderHint(QPainter::Antialiasing, true);
  painter->setRenderHint(QPainter::TextAntialiasing, true);
  painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
  painter->setRenderHint(QPainter::HighQualityAntialiasing, true);

  MapViewStruct request;
  QRectF        boundingBox = contentsBoundingRect();

  request.coord = view->center;
  request.angle = view->angle;
  request.magnification = view->magnification;
  request.width = boundingBox.width();
  request.height = boundingBox.height();

  TiledRenderingHelper::RenderTiles(*painter,request,layerCaches,view->mapDpi,transparentColor, /*overlap*/ 0);
}

void TiledMapOverlay::download(uint32_t zoomLevel, uint32_t xtile, uint32_t ytile) {
  {
    QMutexLocker locker(&tileCacheMutex);
    if (!onlineTileCache.startRequestProcess(zoomLevel, xtile, ytile)) // request was canceled or started already
      return;
  }
}

void TiledMapOverlay::tileDownloaded(uint32_t zoomLevel, uint32_t x, uint32_t y, QImage image, QByteArray /*downloadedData*/)
{
  {
    QMutexLocker locker(&tileCacheMutex);
    onlineTileCache.put(zoomLevel, x, y, image);
  }
  //std::cout << "  put: " << zoomLevel << " xtile: " << x << " ytile: " << y << std::endl;
  emit redraw();
}

void TiledMapOverlay::tileDownloadFailed(uint32_t zoomLevel, uint32_t x, uint32_t y, bool zoomLevelOutOfRange)
{
  QMutexLocker locker(&tileCacheMutex);
  onlineTileCache.removeRequest(zoomLevel, x, y);

  if (zoomLevelOutOfRange && zoomLevel > 0){
    // hack: when zoom level is too high for online source,
    // we try to request tile with lower zoom level and put it to cache
    // as substitute
    uint32_t reqZoom = zoomLevel - 1;
    uint32_t reqX = x / 2;
    uint32_t reqY = y / 2;
    if ((!onlineTileCache.contains(reqZoom, reqX, reqY))
        && onlineTileCache.request(reqZoom, reqX, reqY)){
      qDebug() << "Tile download failed " << x << " " << y << " zoomLevel " << zoomLevel << " try lower zoom";
      //triggerTileRequest(reqZoom, reqX, reqY);
    }
  }
}

QJsonValue TiledMapOverlay::getProvider()
{
  return providerJson;
}

void TiledMapOverlay::setProvider(QJsonValue jv)
{
  OnlineTileProvider provider=OnlineTileProvider::fromJson(jv);
  if (!provider.isValid()){
    qWarning() << "Invalid provider:" << jv;
    return;
  }
  {
    QMutexLocker locker(&tileCacheMutex);
    // FIXME: there is possible race condition when provider is changed and there are pending downloads
    onlineTileCache.cleanupCache();
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
    QMutexLocker locker(&tileCacheMutex);
    onlineTileCache.cleanupCache();
  }
  redraw();
}
