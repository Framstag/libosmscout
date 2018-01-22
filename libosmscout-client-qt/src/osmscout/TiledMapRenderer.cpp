/*
  OSMScout - a Qt backend for libosmscout and libosmscout-map
  Copyright (C) 2010  Tim Teulings
  Copyright (C) 2017  Lukáš Karas

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

#include <osmscout/TiledMapRenderer.h>

#include <osmscout/OSMTile.h>
#include <osmscout/TiledRenderingHelper.h>

#include <osmscout/system/Math.h>
#include <osmscout/util/Logger.h>

TiledMapRenderer::TiledMapRenderer(QThread *thread,
                                   SettingsRef settings,
                                   DBThreadRef dbThread,
                                   QString iconDirectory,
                                   QString tileCacheDirectory,
                                   size_t onlineTileCacheSize,
                                   size_t offlineTileCacheSize):
  MapRenderer(thread,settings,dbThread,iconDirectory),
  tileCacheDirectory(tileCacheDirectory),
  onlineTileCache(onlineTileCacheSize), // online tiles can be loaded from disk cache easily
  offlineTileCache(offlineTileCacheSize), // render offline tile is expensive
  tileDownloader(NULL), // it will be created in different thread
  loadJob(NULL),
  unknownColor(QColor::fromRgbF(1.0,1.0,1.0)) // white
{
  QScreen *srn=QGuiApplication::primaryScreen();
  screenWidth=srn->availableSize().width();
  screenHeight=srn->availableSize().height();


  onlineTilesEnabled = settings->GetOnlineTilesEnabled();
  offlineTilesEnabled = settings->GetOfflineMap();

  connect(settings.get(), SIGNAL(OnlineTileProviderIdChanged(const QString)),
          this, SLOT(onlineTileProviderChanged()),
          Qt::QueuedConnection);
  connect(settings.get(), SIGNAL(OnlineTilesEnabledChanged(bool)),
          this, SLOT(onlineTilesEnabledChanged(bool)),
          Qt::QueuedConnection);
  connect(settings.get(), SIGNAL(OfflineMapChanged(bool)),
          this, SLOT(onOfflineMapChanged(bool)),
          Qt::QueuedConnection);

  connect(dbThread.get(), SIGNAL(databaseLoadFinished(osmscout::GeoBox)),
          this, SLOT(onDatabaseLoaded(osmscout::GeoBox)),
          Qt::QueuedConnection);
  //
  // Make sure that we always decouple caller and receiver even if they are running in the same thread
  // else we might get into a dead lock
  //

  connect(&onlineTileCache,SIGNAL(tileRequested(uint32_t, uint32_t, uint32_t)),
          this,SLOT(onlineTileRequest(uint32_t, uint32_t, uint32_t)),
          Qt::QueuedConnection);

  connect(&offlineTileCache,SIGNAL(tileRequested(uint32_t, uint32_t, uint32_t)),
          this,SLOT(offlineTileRequest(uint32_t, uint32_t, uint32_t)),
          Qt::QueuedConnection);
}

TiledMapRenderer::~TiledMapRenderer()
{
  qDebug() << "~TiledMapRenderer";
  if (tileDownloader != NULL){
    delete tileDownloader;
  }
  if (loadJob!=NULL){
    delete loadJob;
  }
}

void TiledMapRenderer::Initialize()
{
  {
    QMutexLocker locker(&lock);
    qDebug() << "Initialize";

    // create tile downloader in correct thread
    tileDownloader = new OsmTileDownloader(tileCacheDirectory,settings->GetOnlineTileProvider());

    connect(settings.get(), SIGNAL(OnlineTileProviderChanged(const OnlineTileProvider &)),
            tileDownloader, SLOT(onlineTileProviderChanged(const OnlineTileProvider &)),
            Qt::QueuedConnection);

    connect(tileDownloader, SIGNAL(downloaded(uint32_t, uint32_t, uint32_t, QImage, QByteArray)),
            this, SLOT(tileDownloaded(uint32_t, uint32_t, uint32_t, QImage, QByteArray)),
            Qt::QueuedConnection);

    connect(tileDownloader, SIGNAL(failed(uint32_t, uint32_t, uint32_t, bool)),
            this, SLOT(tileDownloadFailed(uint32_t, uint32_t, uint32_t, bool)),
            Qt::QueuedConnection);
  }

  // it is possible that databases are loaded already,
  // call style change callback as part of our initialisation
  onStylesheetFilenameChanged();

  // invalidate tile cache and Redraw()
  InvalidateVisualCache();
}

void TiledMapRenderer::onStylesheetFilenameChanged()
{
  {
    QMutexLocker locker(&tileCacheMutex);

    osmscout::FillStyleRef        unknownFillStyle;
    osmscout::MercatorProjection  projection;

    dbThread->RunSynchronousJob(
      [this,&unknownFillStyle,&projection](const std::list<DBInstanceRef>& databases) {
        for (auto &db:databases){
          if (db->styleConfig) {
            unknownFillStyle=db->styleConfig->GetUnknownFillStyle(projection);
            if (unknownFillStyle) {
              osmscout::Color fillColor = unknownFillStyle->GetFillColor();
              unknownColor.setRgbF(fillColor.GetR(),
                                   fillColor.GetG(),
                                   fillColor.GetB(),
                                   fillColor.GetA());
              break;
            }
          }
        }
      }
    );
  }

  MapRenderer::onStylesheetFilenameChanged();
}

void TiledMapRenderer::InvalidateVisualCache()
{
  // invalidate tile cache and emit Redraw
  {
      QMutexLocker locker(&tileCacheMutex);
      offlineTileCache.invalidate();
      offlineTileCache.clearPendingRequests();
  }
  emit Redraw();
}

/**
 * Render map defined by request to painter
 * @param painter
 * @param request
 * @return true if rendered map is complete
 */
bool TiledMapRenderer::RenderMap(QPainter& painter,
                                 const MapViewStruct& request)
{
  QTime start;
  QMutexLocker locker(&tileCacheMutex);
  int elapsed = start.elapsed();
  if (elapsed > 1){
    osmscout::log.Warn() << "Mutex acquiere took " << elapsed << " ms";
  }

  QList<TileCache*> layerCaches;
  if (onlineTilesEnabled){
    layerCaches << &onlineTileCache;
  }
  if (offlineTilesEnabled){
    layerCaches << &offlineTileCache;
  }
  onlineTileCache.clearPendingRequests();
  offlineTileCache.clearPendingRequests();

  if (!TiledRenderingHelper::RenderTiles(painter,request,layerCaches,mapDpi,unknownColor)){
    return false;
  }

  return onlineTileCache.isRequestQueueEmpty() && offlineTileCache.isRequestQueueEmpty();
}

DatabaseCoverage TiledMapRenderer::databaseCoverageOfTile(uint32_t zoomLevel, uint32_t xtile, uint32_t ytile)
{
  osmscout::GeoBox tileBoundingBox = OSMTile::tileBoundingBox(zoomLevel, xtile, ytile);
  osmscout::Magnification magnification;
  magnification.SetLevel(zoomLevel);
  DatabaseCoverage state=dbThread->databaseCoverage(magnification,tileBoundingBox);
  if (state==DatabaseCoverage::Outside &&
      overlayObjectsBox().Intersects(tileBoundingBox)){
    return DatabaseCoverage::Intersects;
  }
  return state;
}

void TiledMapRenderer::onDatabaseLoaded(osmscout::GeoBox boundingBox)
{
  {
    QMutexLocker locker(&tileCacheMutex);
    onlineTileCache.invalidate(boundingBox);
    offlineTileCache.invalidate(boundingBox);
  }

  emit Redraw();
}


void TiledMapRenderer::onlineTileRequest(uint32_t zoomLevel, uint32_t xtile, uint32_t ytile)
{
    {
        QMutexLocker locker(&tileCacheMutex);
        if (!onlineTileCache.startRequestProcess(zoomLevel, xtile, ytile)) // request was canceled or started already
            return;
    }

    bool requestedFromWeb = onlineTilesEnabled && (!(offlineTilesEnabled &&
                                                  databaseCoverageOfTile(zoomLevel, xtile, ytile) ==
                                                  DatabaseCoverage::Covered));

    if (requestedFromWeb){
        QMutexLocker locker(&lock);
        if (tileDownloader == NULL){
            qWarning() << "tile requested but downloader is not initialized yet";
            emit tileDownloadFailed(zoomLevel, xtile, ytile, false);
        }else{
            emit tileDownloader->download(zoomLevel, xtile, ytile);
        }
    } else{
        // put Null image
        {
            QMutexLocker locker(&tileCacheMutex);
            onlineTileCache.put(zoomLevel, xtile, ytile, QImage());
        }
    }
}

void TiledMapRenderer::offlineTileRequest(uint32_t zoomLevel, uint32_t xtile, uint32_t ytile)
{
    // just start loading
    QMutexLocker locker(&lock);
    if (loadJob!=NULL){
        // wait until previous loading is not finished
        return;
    }
    {
        QMutexLocker locker(&tileCacheMutex);
        if (!offlineTileCache.startRequestProcess(zoomLevel, xtile, ytile)) // request was canceled or started already
            return;
    }

    DatabaseCoverage state = databaseCoverageOfTile(zoomLevel, xtile, ytile);
    // render offline map when area is fully covered by database or online tiles are disabled -> render basemap
    bool render = (state != DatabaseCoverage::Outside) || (!onlineTilesEnabled);
    if (render) {
        // tile rendering have sub-linear complexity with area size
        // it means that it is advatage to merge more tile requests with same zoom
        // and render bigger area
        {
            QMutexLocker locker(&tileCacheMutex);
            offlineTileCache.mergeAndStartRequests(zoomLevel, xtile, ytile,
                                                   loadXFrom, loadXTo, loadYFrom, loadYTo,
                                                   /*maxWidth*/ 5, /*maxHeight*/ 5);
        }
        uint32_t width = (loadXTo - loadXFrom + 1);
        uint32_t height = (loadYTo - loadYFrom + 1);
        loadZ=zoomLevel;


        //osmscout::GeoBox tileBoundingBox = OSMTile::tileBoundingBox(zoomLevel, xtile, ytile);
        osmscout::GeoCoord tileVisualCenter = OSMTile::tileRelativeCoord(zoomLevel,
                (double)loadXFrom + (double)width/2.0,
                (double)loadYFrom + (double)height/2.0);

        double osmTileDimension = (double)OSMTile::osmTileOriginalWidth() * (mapDpi / OSMTile::tileDPI() ); // pixels

        osmscout::Magnification magnification;
        magnification.SetLevel(zoomLevel);

        osmscout::MercatorProjection projection;
        projection.Set(tileVisualCenter,
                       /*currentAngle*/ 0.0,
                       magnification,
                       mapDpi,
                       ((double)width * osmTileDimension)+osmTileDimension,
                       ((double)height * osmTileDimension)+osmTileDimension);

        unsigned long maximumAreaLevel=4;
        if (magnification.GetLevel() >= 15) {
          maximumAreaLevel=6;
        }

        loadJob=new DBLoadJob(projection,
                              maximumAreaLevel,
                              /* lowZoomOptimization */ true,
                              /* closeOnFinish */ false);

        connect(loadJob, SIGNAL(finished(QMap<QString,QMap<osmscout::TileId,osmscout::TileRef>>)),
                this, SLOT(onLoadJobFinished(QMap<QString,QMap<osmscout::TileId,osmscout::TileRef>>)));

        dbThread->RunJob(loadJob);

    }else{
        // put Null image
        {
            QMutexLocker locker(&tileCacheMutex);
            offlineTileCache.put(zoomLevel, xtile, ytile, QImage());
        }
    }
}

void TiledMapRenderer::tileDownloaded(uint32_t zoomLevel, uint32_t x, uint32_t y, QImage image, QByteArray /*downloadedData*/)
{
    {
        QMutexLocker locker(&tileCacheMutex);
        onlineTileCache.put(zoomLevel, x, y, image);
    }
    //std::cout << "  put: " << zoomLevel << " xtile: " << x << " ytile: " << y << std::endl;
    emit Redraw();
}

void TiledMapRenderer::tileDownloadFailed(uint32_t zoomLevel, uint32_t x, uint32_t y, bool zoomLevelOutOfRange)
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

void TiledMapRenderer::onlineTileProviderChanged()
{
    {
        QMutexLocker locker(&tileCacheMutex);
        onlineTileCache.invalidate();
    }
    emit Redraw();
}

void TiledMapRenderer::onlineTilesEnabledChanged(bool b)
{
    {
        onlineTilesEnabled = b;

        QMutexLocker cacheLocker(&tileCacheMutex);
        onlineTileCache.invalidate();
        onlineTileCache.clearPendingRequests();
    }

    // when online tiles are disabled, basemap is rendered
    // we need to invalidate offline tiles on this change
    InvalidateVisualCache();
}

void TiledMapRenderer::onOfflineMapChanged(bool b)
{
    {
        QMutexLocker locker(&lock);
        offlineTilesEnabled = b;

        QMutexLocker cacheLocker(&tileCacheMutex);
        onlineTileCache.invalidate(); // overlapp areas will change
        offlineTileCache.invalidate();
        offlineTileCache.clearPendingRequests();
    }
    emit Redraw();
}

void TiledMapRenderer::onLoadJobFinished(QMap<QString,QMap<osmscout::TileId,osmscout::TileRef>> tiles)
{
    // just start loading
    QMutexLocker locker(&lock);
    if (loadJob==NULL){
        // no running load job
        return;
    }

    uint32_t width = (loadXTo - loadXFrom + 1);
    uint32_t height = (loadYTo - loadYFrom + 1);

    osmscout::GeoCoord tileVisualCenter = OSMTile::tileRelativeCoord(loadZ,
            (double)loadXFrom + (double)width/2.0,
            (double)loadYFrom + (double)height/2.0);

    double osmTileDimension = (double)OSMTile::osmTileOriginalWidth() * (mapDpi / OSMTile::tileDPI() ); // pixels

    QImage canvas((double)width * osmTileDimension,
                  (double)height * osmTileDimension,
                  QImage::Format_ARGB32_Premultiplied); // TODO: verify best format with profiler (callgrind)

    QColor transparent = QColor::fromRgbF(1, 1, 1, 0.0);
    canvas.fill(transparent);

    QPainter p;
    p.begin(&canvas);

    //loadJob->AddTileDataToMapData()
    osmscout::MapParameter        drawParameter;
    std::list<std::string>        paths;

    paths.push_back(iconDirectory.toLocal8Bit().data());

    drawParameter.SetIconPaths(paths);
    drawParameter.SetPatternPaths(paths);
    drawParameter.SetDebugData(false);
    drawParameter.SetDebugPerformance(true);

    // optimize process can reduce number of nodes before rendering
    // it helps for slow renderer backend, but it cost some cpu
    // it seems that it is ok disable it for Qt
    drawParameter.SetOptimizeWayNodes(osmscout::TransPolygon::none);
    drawParameter.SetOptimizeAreaNodes(osmscout::TransPolygon::none);

    drawParameter.SetRenderBackground(false);
    drawParameter.SetRenderUnknowns(false); // it is necessary to disable it with multiple sources
    drawParameter.SetRenderSeaLand(renderSea);

    drawParameter.SetFontName(fontName.toStdString());
    drawParameter.SetFontSize(fontSize);

    drawParameter.SetLabelLineMinCharCount(15);
    drawParameter.SetLabelLineMaxCharCount(30);
    drawParameter.SetLabelLineFitToArea(true);
    drawParameter.SetLabelLineFitToWidth(std::min(screenWidth, screenHeight));

    // see Tiler.cpp example...

    // To get accurate label drawing at tile borders, we take into account labels
    // of other than the current tile, too.
    if (loadZ >= 14) {
        // but not for high zoom levels, it is too expensive
        drawParameter.SetDropNotVisiblePointLabels(true);
    }else{
        drawParameter.SetDropNotVisiblePointLabels(false);
    }

    // setup projection for these tiles
    osmscout::MercatorProjection projection;
    osmscout::Magnification magnification;
    magnification.SetLevel(loadZ);
    projection.Set(tileVisualCenter, /* angle */ 0, magnification, mapDpi,
                   canvas.width(), canvas.height());
    projection.SetLinearInterpolationUsage(loadZ >= 10);

    // overlay ways
    std::vector<OverlayObjectRef> overlayObjects;
    osmscout::GeoBox renderBox;
    projection.GetDimensions(renderBox);
  getOverlayObjects(overlayObjects, renderBox);

    //DrawMap(p, tileVisualCenter, loadZ, canvas.width(), canvas.height());
    bool success;
    {
      DBRenderJob job(projection,
                      tiles,
                      &drawParameter,
                      &p,
                      overlayObjects,
                      /*drawCanvasBackground*/ false,
                      /*renderBasemap*/ !onlineTilesEnabled);
      dbThread->RunJob(&job);
      success=job.IsSuccess();
    }

    // this slot is called from DBLoadJob, we can't delete it now
    loadJob->deleteLater();
    loadJob=NULL;

    if (!success)  {
      osmscout::log.Error() << "*** Rendering of data has error or was interrupted";
      return;
    }

    p.end();

    {
        QMutexLocker locker(&tileCacheMutex);
        if (width == 1 && height == 1){
            offlineTileCache.put(loadZ, loadXFrom, loadYFrom, canvas);
        }else{
            for (uint32_t y = loadYFrom; y <= loadYTo; ++y){
                for (uint32_t x = loadXFrom; x <= loadXTo; ++x){

                    QImage tile = canvas.copy(
                            (double)(x - loadXFrom) * osmTileDimension,
                            (double)(y - loadYFrom) * osmTileDimension,
                            osmTileDimension, osmTileDimension
                            );

                    offlineTileCache.put(loadZ, x, y, tile);
                }
            }
        }

        // we don't process offline tile requests while there is active
        // loading job, so we reemit requests again...
        offlineTileCache.reemitRequests();
    }

    emit Redraw();
    //std::cout << "  put offline: " << loadZ << " xtile: " << xtile << " ytile: " << ytile << std::endl;
}
