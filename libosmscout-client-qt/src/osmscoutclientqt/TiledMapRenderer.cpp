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

#include <osmscoutclientqt/TiledMapRenderer.h>

#include <osmscoutclientqt/OSMTile.h>
#include <osmscoutclientqt/TiledRenderingHelper.h>

#include <osmscout/system/Math.h>
#include <osmscout/log/Logger.h>

#include <QGuiApplication>
#include <QScreen>
#include <QtCore>

namespace osmscout {
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
  tileDownloader(nullptr), // it will be created in different thread
  loadJob(nullptr),
  unknownColor(QColor::fromRgbF(1.0,1.0,1.0)) // white
{
  QScreen *srn=QGuiApplication::primaryScreen();
  screenWidth=srn->availableSize().width();
  screenHeight=srn->availableSize().height();


  onlineTilesEnabled = settings->GetOnlineTilesEnabled();
  offlineTilesEnabled = settings->GetOfflineMap();

  settings->onlineTileProviderChanged.Connect(onlineTileProviderSlot);
  settings->onlineTilesEnabledChanged.Connect(onlineTileEnabledSlot);
  settings->offlineMapChanged.Connect(offlineMapChangedSlot);

  connect(this, &TiledMapRenderer::onlineTileProviderSignal,
          this, &TiledMapRenderer::onlineTileProviderChanged,
          Qt::QueuedConnection);
  connect(this, &TiledMapRenderer::onlineTilesEnabledSignal,
          this, &TiledMapRenderer::onlineTilesEnabledChanged,
          Qt::QueuedConnection);
  connect(this, &TiledMapRenderer::offlineMapChangedSignal,
          this, &TiledMapRenderer::onOfflineMapChanged,
          Qt::QueuedConnection);

  //
  // Make sure that we always decouple caller and receiver even if they are running in the same thread
  // else we might get into a dead lock
  //

  connect(&onlineTileCache, &TileCache::tileRequested,
          this, &TiledMapRenderer::onlineTileRequest,
          Qt::QueuedConnection);

  connect(&offlineTileCache, &TileCache::tileRequested,
          this, &TiledMapRenderer::offlineTileRequest,
          Qt::QueuedConnection);
}

TiledMapRenderer::~TiledMapRenderer()
{
  qDebug() << "~TiledMapRenderer";
  delete tileDownloader;
  delete loadJob;
}

void TiledMapRenderer::Initialize()
{
  {
    QMutexLocker locker(&lock);
    osmscout::log.Debug() << "Initialize";

    // create tile downloader in correct thread
    tileDownloader = new OsmTileDownloader(tileCacheDirectory,settings->GetOnlineTileProvider());

    connect(this, &TiledMapRenderer::onlineTileProviderSignal,
            tileDownloader, &OsmTileDownloader::onlineTileProviderChanged,
            Qt::QueuedConnection);

    connect(tileDownloader, &OsmTileDownloader::downloaded,
            this, &TiledMapRenderer::tileDownloaded,
            Qt::QueuedConnection);

    connect(tileDownloader, &OsmTileDownloader::failed,
            this, &TiledMapRenderer::tileDownloadFailed,
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
        for (const auto &db:databases){
          auto styledConfig=db->GetStyleConfig();
          if (styledConfig) {
            unknownFillStyle=styledConfig->GetUnknownFillStyle(projection);
            if (unknownFillStyle) {
              osmscout::Color fillColor = unknownFillStyle->GetFillColor();
              unknownColor.setRgbF(fillColor.GetR(),
                                   fillColor.GetG(),
                                   fillColor.GetB(),
                                   fillColor.GetA());
            }

            tileGridColor=QColor::fromRgbF(0.8,0.8,0.8);
            if (styledConfig->HasFlag("daylight") && !styledConfig->GetFlagByName("daylight")) {
              tileGridColor=QColor::fromRgbF(0.2,0.2,0.2);
            }

            break;
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
    offlineTileCache.incEpoch();
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
  QElapsedTimer start;
  start.start();
  QMutexLocker locker(&tileCacheMutex);
  if (auto elapsed = start.elapsed(); elapsed > 1){
    osmscout::log.Warn() << "Mutex acquire took " << elapsed << " ms";
  }

  QList<TileCache*> layerCaches;
  if (onlineTilesEnabled){
    layerCaches << &onlineTileCache;
  }

  layerCaches << &offlineTileCache;

  onlineTileCache.clearPendingRequests();
  offlineTileCache.clearPendingRequests();

  if (!TiledRenderingHelper::RenderTiles(painter,request,layerCaches,unknownColor,-1,tileGridColor)){
    return false;
  }

  return onlineTileCache.isRequestQueueEmpty() && offlineTileCache.isRequestQueueEmpty();
}

DatabaseCoverage TiledMapRenderer::databaseCoverageOfTile(uint32_t zoomLevel, uint32_t xtile, uint32_t ytile)
{
  GeoBox tileBoundingBox = OSMTile::tileBoundingBox(zoomLevel, xtile, ytile);
  MagnificationLevel level(zoomLevel);
  Magnification magnification(level);
  DatabaseCoverage state = offlineTilesEnabled
                         ? dbThread->databaseCoverage(magnification,tileBoundingBox)
                         : DatabaseCoverage::Outside;

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
    offlineTileCache.incEpoch();
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
        if (tileDownloader == nullptr){
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
    if (loadJob!=nullptr){
        // wait until previous loading is not finished
        return;
    }
    {
        QMutexLocker tileCacheLocker(&tileCacheMutex);
        if (!offlineTileCache.startRequestProcess(zoomLevel, xtile, ytile)) // request was canceled or started already
            return;

        loadEpoch = offlineTileCache.getEpoch();
    }

    DatabaseCoverage state = databaseCoverageOfTile(zoomLevel, xtile, ytile);
    // render offline map when area is fully covered by db or online tiles are disabled -> render basemap
    bool render = (state != DatabaseCoverage::Outside) || (!onlineTilesEnabled);
    if (render) {
        // tile rendering have sub-linear complexity with area size
        // it means that it is advantage to merge more tile requests with same zoom
        // and render bigger area
        {
            QMutexLocker tileCacheLocker(&tileCacheMutex);
            offlineTileCache.mergeAndStartRequests(zoomLevel, xtile, ytile,
                                                   loadXFrom, loadXTo, loadYFrom, loadYTo,
                                                   /*maxWidth*/ 5, /*maxHeight*/ 5);
        }
        uint32_t width = (loadXTo - loadXFrom + 1);
        uint32_t height = (loadYTo - loadYFrom + 1);
        loadZ=MagnificationLevel(zoomLevel);


        //osmscout::GeoBox tileBoundingBox = OSMTile::tileBoundingBox(zoomLevel, xtile, ytile);
        osmscout::GeoCoord tileVisualCenter = OSMTile::tileRelativeCoord(zoomLevel,
                (double)loadXFrom + (double)width/2.0,
                (double)loadYFrom + (double)height/2.0);

        double osmTileDimension = (double)OSMTile::osmTileOriginalWidth() * (mapDpi / OSMTile::tileDPI() ); // pixels

        MagnificationLevel level(zoomLevel);
        Magnification magnification(level);

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

        connect(loadJob, &DBLoadJob::finished,
                this, &TiledMapRenderer::onLoadJobFinished);

        if (offlineTilesEnabled) {
          dbThread->RunJob(std::bind(&DBLoadJob::Run, loadJob, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        } else {
          // offline map rendering is disabled but there are some overlay objects intersecting with the tile...
          onLoadJobFinished(QMap<QString,QMap<osmscout::TileKey,osmscout::TileRef>>());
        }

    }else{
        // put Null image
        QMutexLocker tileCacheLocker(&tileCacheMutex);
        offlineTileCache.put(zoomLevel, xtile, ytile, QImage(), loadEpoch);
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

void TiledMapRenderer::onlineTileProviderChanged(const OnlineTileProvider&)
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
        onlineTileCache.invalidate(); // overlap areas will change
        offlineTileCache.invalidate();
        offlineTileCache.clearPendingRequests();
        offlineTileCache.incEpoch();
    }
    emit Redraw();
}

void TiledMapRenderer::onLoadJobFinished(QMap<QString,QMap<osmscout::TileKey,osmscout::TileRef>> tiles)
{
    QMutexLocker locker(&lock);
    if (loadJob==nullptr){
        // no running load job
        return;
    }

    uint32_t width = (loadXTo - loadXFrom + 1);
    uint32_t height = (loadYTo - loadYFrom + 1);

    osmscout::GeoCoord tileVisualCenter = OSMTile::tileRelativeCoord(loadZ.Get(),
            (double)loadXFrom + (double)width/2.0,
            (double)loadYFrom + (double)height/2.0);

    // For HiDPI screens (screenPixelRatio > 1) tiles as up-scaled before displaying. When there is ratio 2.0, 100px on Qt canvas
    // is displayed as 200px on the screen. To provide best results on HiDPI screen, we upscale tiles by this pixel ratio.
    double finalDpi = mapDpi * this->screenPixelRatio;

    uint32_t tileDimension = double(OSMTile::osmTileOriginalWidth()) * (finalDpi / OSMTile::tileDPI()); // pixels

    // older/mobile OpenGL (without GL_ARB_texture_non_power_of_two) requires textures with size of power of two
    // we should provide tiles with required size to avoid scaling in QOpenGLTextureCache::bindTexture
    tileDimension = qNextPowerOfTwo(tileDimension - 1);
    finalDpi = (double(tileDimension) / double(OSMTile::osmTileOriginalWidth())) * OSMTile::tileDPI();

    QImage canvas(width * tileDimension,
                  height * tileDimension,
                  QImage::Format_RGBA8888_Premultiplied);

    QColor transparent = QColor::fromRgbF(1, 1, 1, 0.0);
    canvas.fill(transparent);

    QPainter p;
    p.begin(&canvas);

    osmscout::MapParameter        drawParameter;
    std::list<std::string>        paths;

    paths.push_back(iconDirectory.toLocal8Bit().data());

    drawParameter.SetIconMode(osmscout::MapParameter::IconMode::Scalable);
    drawParameter.SetPatternMode(osmscout::MapParameter::PatternMode::Scalable);
    drawParameter.SetIconPaths(paths);
    drawParameter.SetPatternPaths(paths);
    drawParameter.SetDebugData(osmscout::log.IsDebug());
    drawParameter.SetDebugPerformance(osmscout::log.IsWarn());

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

    drawParameter.SetShowAltLanguage(showAltLanguage);

    drawParameter.SetLabelLineMinCharCount(15);
    drawParameter.SetLabelLineMaxCharCount(30);
    drawParameter.SetLabelLineFitToArea(true);
    drawParameter.SetLabelLineFitToWidth(std::min(screenWidth, screenHeight));

    drawParameter.GetLocaleRef().SetDistanceUnits(units == "imperial" ? osmscout::DistanceUnitSystem::Imperial : osmscout::DistanceUnitSystem::Metrics);

    // setup projection for these tiles
    osmscout::MercatorProjection projection;
    osmscout::Magnification magnification(loadZ);

    projection.Set(tileVisualCenter, /* angle */ 0, magnification, finalDpi,
                   canvas.width(), canvas.height());
    projection.SetLinearInterpolationUsage(loadZ.Get() >= 10);

    // overlay ways
    std::vector<OverlayObjectRef> overlayObjects;
    osmscout::GeoBox renderBox(projection.GetDimensions());
    getOverlayObjects(overlayObjects, renderBox);

    //DrawMap(p, tileVisualCenter, loadZ, canvas.width(), canvas.height());
    bool success;
    {
      DBRenderJob job(projection,
                      tiles,
                      &drawParameter,
                      &p,
                      overlayObjects,
                      dbThread->GetEmptyStyleConfig(),
                      /*drawCanvasBackground*/ false,
                      /*renderBasemap*/ !onlineTilesEnabled,
                      offlineTilesEnabled);
      dbThread->RunJob(std::bind(&DBRenderJob::Run, &job, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
      success=job.IsSuccess();
    }

    // this slot is called from DBLoadJob, we can't delete it now
    loadJob->Close();
    loadJob->deleteLater();
    loadJob=nullptr;

    if (!success)  {
      osmscout::log.Error() << "*** Rendering of data has error or was interrupted";
      return;
    }

    p.end();

    {
        QMutexLocker tileCacheLocker(&tileCacheMutex);

        if (loadEpoch != offlineTileCache.getEpoch()){
          osmscout::log.Warn() << "Rendered from outdated data" << loadEpoch << "!=" << offlineTileCache.getEpoch();
        }

        if (width == 1 && height == 1){
            offlineTileCache.put(loadZ.Get(), loadXFrom, loadYFrom, canvas, loadEpoch);
        }else{
            for (uint32_t y = loadYFrom; y <= loadYTo; ++y){
                for (uint32_t x = loadXFrom; x <= loadXTo; ++x){

                    QImage tile = canvas.copy(
                            (x - loadXFrom) * tileDimension,
                            (y - loadYFrom) * tileDimension,
                            tileDimension, tileDimension
                            );

                    offlineTileCache.put(loadZ.Get(), x, y, tile, loadEpoch);
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
}
