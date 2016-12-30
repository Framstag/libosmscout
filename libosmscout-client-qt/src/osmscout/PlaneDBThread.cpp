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

#include <iostream>

#include <QMutexLocker>
#include <QDebug>
#include <QDir>
#include <QRegExp>

#include <osmscout/util/Logger.h>
#include <osmscout/util/StopClock.h>
#include <osmscout/system/Math.h>

#include <osmscout/DBThread.h>
#include <osmscout/PlaneDBThread.h>

// Timeout for the first rendering after rerendering was triggered (render what ever data is available)
static int INITIAL_DATA_RENDERING_TIMEOUT = 10;

// Timeout for the updated rendering after rerendering was triggered (more rendering data is available)
static int UPDATED_DATA_RENDERING_TIMEOUT = 200;

PlaneDBThread::PlaneDBThread(QStringList databaseLookupDirs,
                             QString stylesheetFilename,
                             QString iconDirectory)
 : DBThread(databaseLookupDirs, stylesheetFilename, iconDirectory),
   pendingRenderingTimer(this),
   currentImage(NULL),
   currentCoord(0.0,0.0),
   currentAngle(0.0),
   currentMagnification(0),
   finishedImage(NULL),
   finishedCoord(0.0,0.0),
   finishedMagnification(0)
{
  pendingRenderingTimer.setSingleShot(true);

  connect(this,SIGNAL(TriggerMapRenderingSignal(const RenderMapRequest&)),
          this,SLOT(TriggerMapRendering(const RenderMapRequest&)),
          Qt::QueuedConnection);

  connect(this,SIGNAL(TriggerInitialRendering()),
          this,SLOT(HandleInitialRenderingRequest()));

  connect(&pendingRenderingTimer,SIGNAL(timeout()),
          this,SLOT(DrawMap()));

  connect(this,SIGNAL(TileStatusChanged(const osmscout::TileRef&)),
          this,SLOT(HandleTileStatusChanged(const osmscout::TileRef&)),
          Qt::QueuedConnection);

  //
  // Make sure that we always decouple caller and receiver even if they are running in the same thread
  // else we might get into a dead lock
  //

  connect(this,SIGNAL(TriggerDrawMap()),
          this,SLOT(DrawMap()),
          Qt::QueuedConnection);
}

PlaneDBThread::~PlaneDBThread()
{

}

void PlaneDBThread::Initialize()
{

  osmscout::log.Debug() << "Initialize";
  // invalidate tile cache and init base
  DBThread::InitializeDatabases();

  {
    QMutexLocker locker(&mutex);
    QMutexLocker finishedLocker(&finishedMutex);
    for (auto db:databases){
      if (db->styleConfig){
        db->styleConfig->GetUnknownFillStyle(projection, finishedUnknownFillStyle);
        if (finishedUnknownFillStyle){
          break;
        }
      }
    }
  }

  emit Redraw();
}

bool PlaneDBThread::RenderMap(QPainter& painter,
                              const RenderMapRequest& request)
{
  //qDebug() << "RenderMap()";

  QMutexLocker locker(&finishedMutex);

  osmscout::Color backgroundColor;
  if (finishedUnknownFillStyle) {
    backgroundColor=finishedUnknownFillStyle->GetFillColor();
  } else {
    backgroundColor=osmscout::Color(0,0,0);
  }

  if (finishedImage==NULL) {
    painter.fillRect(0,
                     0,
                     request.width,
                     request.height,
                     QColor::fromRgbF(backgroundColor.GetR(),
                                      backgroundColor.GetG(),
                                      backgroundColor.GetB(),
                                      backgroundColor.GetA()));
    //RenderMessage(painter,request.width,request.height,"no image rendered (internal error?)");

    // Since we assume that this is just a temporary problem, or we just were not instructed to render
    // a map yet, we trigger rendering an image...
    {
      QMutexLocker reqLocker(&lastRequestMutex);
      lastRequest=request;
    }
    emit TriggerMapRenderingSignal(request);
    return false;
  }

  osmscout::MercatorProjection requestProjection;
  requestProjection.Set(request.coord,
                 request.angle,
                 request.magnification,
                 mapDpi,
                 request.width,
                 request.height);

  osmscout::MercatorProjection finalImgProjection;
  finalImgProjection.Set(finishedCoord,
                 finishedAngle,
                 finishedMagnification,
                 mapDpi,
                 finishedImage->width(),
                 finishedImage->height());

  osmscout::GeoBox finalImgBoundingBox;
  finalImgProjection.GetDimensions(finalImgBoundingBox);


  double x1;
  double y1;
  double x2;
  double y2;

  requestProjection.GeoToPixel(finalImgBoundingBox.GetMaxCoord(),x2,y1); // max coord => right top
  requestProjection.GeoToPixel(finalImgBoundingBox.GetMinCoord(),x1,y2); // min coord => left bottom

  if (x1>0 || y1>0 || x2<request.width || y2<request.height) {
    painter.fillRect(0,
                     0,
                     request.width,
                     request.height,
                     QColor::fromRgbF(backgroundColor.GetR(),
                                      backgroundColor.GetG(),
                                      backgroundColor.GetB(),
                                      backgroundColor.GetA()));
  }

  // TODO: handle angle
  //qDebug() << "Draw final image to canvas:" << QRectF(x1,y1,x2-x1,y2-y1);
  painter.drawImage(QRectF(x1,y1,x2-x1,y2-y1),*finishedImage);

  bool needsNoRepaint=finishedImage->width()==(int) request.width &&
                      finishedImage->height()==(int) request.height &&
                      finishedCoord==request.coord &&
                      finishedAngle==request.angle &&
                      finishedMagnification==request.magnification;

  if (!needsNoRepaint){
    {
      QMutexLocker reqLocker(&lastRequestMutex);
      lastRequest=request;
    }
    emit TriggerMapRenderingSignal(request);
  }

  return needsNoRepaint;
}

void PlaneDBThread::TriggerMapRendering(const RenderMapRequest& request)
{
  {
    QMutexLocker reqLocker(&lastRequestMutex);
    if (request!=lastRequest){
      return;
    }
  }

  osmscout::log.Debug() << "Start data loading...";
  {
    QMutexLocker locker(&mutex);
    // CancelCurrentDataLoading();

    currentWidth=request.width;
    currentHeight=request.height;
    currentCoord=request.coord;
    currentAngle=request.angle;
    currentMagnification=request.magnification;

    for (auto db:databases){
      if (db->database->IsOpen() &&
          db->styleConfig) {
        osmscout::AreaSearchParameter searchParameter;

        db->dataLoadingBreaker->Reset();
        searchParameter.SetBreaker(db->dataLoadingBreaker);

        if (currentMagnification.GetLevel()>=15) {
          searchParameter.SetMaximumAreaLevel(6);
        }
        else {
          searchParameter.SetMaximumAreaLevel(4);
        }

        searchParameter.SetUseMultithreading(true);
        searchParameter.SetUseLowZoomOptimization(true);

        projection.Set(currentCoord,
                       currentAngle,
                       currentMagnification,
                       mapDpi,
                       currentWidth,
                       currentHeight);

        std::list<osmscout::TileRef> tiles;

        db->mapService->LookupTiles(projection,tiles);
        if (!db->mapService->LoadMissingTileDataAsync(searchParameter,*(db->styleConfig),tiles)) {
          osmscout::log.Error() << "*** Loading of data has error or was interrupted";
          continue;
        }
      }
      else {
        osmscout::log.Error() << "Cannot draw map: " << db->database->IsOpen() << " " << db->styleConfig.get();
        //QPainter p;
        //RenderMessage(p,request.width,request.height,"Database not open");
      }
    }
  }
  emit TriggerInitialRendering();
}

void PlaneDBThread::HandleInitialRenderingRequest()
{
  if (pendingRenderingTimer.isActive())
    return; // avoid repeated draw postpone (data loading may be called very fast)

  osmscout::log.Debug() << "Start rendering timer:" << INITIAL_DATA_RENDERING_TIMEOUT << "ms";
  pendingRenderingTimer.stop();
  pendingRenderingTimer.start(INITIAL_DATA_RENDERING_TIMEOUT);
}

void PlaneDBThread::InvalidateVisualCache()
{
  QMutexLocker finishedLocker(&finishedMutex);
  osmscout::log.Debug() << "Invalidate finished image";
  if (finishedImage)
    delete finishedImage;
  finishedImage=NULL;
}

void PlaneDBThread::HandleTileStatusChanged(const osmscout::TileRef& changedTile)
{
  //return; // FIXME: remove this return, make loading asynchronous
  QMutexLocker locker(&mutex);

  bool relevant=false;
  for (auto &db: databases){
    std::list<osmscout::TileRef> tiles;

    db->mapService->LookupTiles(projection,tiles);

    for (const auto tile : tiles) {
      if (tile==changedTile) {
        relevant|=true;
        break;
      }
    }
  }

  if (!relevant) {
    return;
  }

  int elapsedTime=lastRendering.elapsed();

  //qDebug() << "Relevant tile changed, elapsed:" << elapsedTime;

  if (pendingRenderingTimer.isActive()) {
    //qDebug() << "Waiting for timer in" << pendingRenderingTimer.remainingTime() ;
  }
  else if (elapsedTime>UPDATED_DATA_RENDERING_TIMEOUT) {
    osmscout::log.Debug() << "TriggerDrawMap, last rendering" << elapsedTime << "ms before";
    emit TriggerDrawMap();
  }
  else {
    osmscout::log.Debug() << "Start rendering timer:" << UPDATED_DATA_RENDERING_TIMEOUT-elapsedTime << "ms";
    pendingRenderingTimer.start(UPDATED_DATA_RENDERING_TIMEOUT-elapsedTime);
  }
}

void PlaneDBThread::TileStateCallback(const osmscout::TileRef& changedTile)
{
  // parent
  DBThread::TileStateCallback(changedTile);

  //printTileInfo(changedTile);

  // We are in the context of one of the libosmscout worker threads
  emit TileStatusChanged(changedTile);
}

/**
 * Actual map drawing into the back buffer
 */
void PlaneDBThread::DrawMap()
{
  osmscout::log.Debug() << "DrawMap()";
  {
    QMutexLocker locker(&mutex);
    for (auto db:databases){
      if (!db->database->IsOpen() || (!db->styleConfig)) {
          osmscout::log.Warn() << " Not initialized! " << db->path.toLocal8Bit().data();
          return;
      }
    }

    if (currentImage==NULL ||
        currentImage->width()!=(int)currentWidth ||
        currentImage->height()!=(int)currentHeight) {
      delete currentImage;

      currentImage=new QImage(QSize(currentWidth,
                                    currentHeight),
                              QImage::Format_RGB32);
    }

    osmscout::MapParameter       drawParameter;
    std::list<std::string>       paths;

    paths.push_back(iconDirectory.toLocal8Bit().data());

    drawParameter.SetIconPaths(paths);
    drawParameter.SetPatternPaths(paths);
    drawParameter.SetDebugData(false);
    drawParameter.SetDebugPerformance(true);
    drawParameter.SetOptimizeWayNodes(osmscout::TransPolygon::quality);
    drawParameter.SetOptimizeAreaNodes(osmscout::TransPolygon::quality);
    drawParameter.SetRenderBackground(true); // we always render background in PlaneDBThread
    drawParameter.SetRenderSeaLand(renderSea);

    // create copy of projection
    osmscout::MercatorProjection renderProjection;

    renderProjection.Set(projection.GetCenter(),
                         projection.GetAngle(),
                         projection.GetMagnification(),
                         projection.GetDPI(),
                         projection.GetWidth(),
                         projection.GetHeight());

    renderProjection.SetLinearInterpolationUsage(renderProjection.GetMagnification().GetLevel() >= 10);

    QPainter p;
    p.begin(currentImage);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    bool success=true;
    for (auto &db:databases){
      std::list<osmscout::TileRef> tiles;
      osmscout::MapData            data; // TODO: make sence cache these data?

      db->mapService->LookupTiles(renderProjection,tiles);
      db->mapService->AddTileDataToMapData(tiles,data);

      if (drawParameter.GetRenderSeaLand()) {
        db->mapService->GetGroundTiles(renderProjection,
                                       data.groundTiles);
      }

      success&=db->painter->DrawMap(renderProjection,
                                    drawParameter,
                                    data,
                                    &p);

    }
    p.end();

    if (!success)  {
      osmscout::log.Error() << "*** Rendering of data has error or was interrupted";
      return;
    }
    {
      QMutexLocker finishedLocker(&finishedMutex);
      std::swap(currentImage,finishedImage);

      finishedCoord=currentCoord;
      finishedAngle=currentAngle;
      finishedMagnification=currentMagnification;

      lastRendering=QTime::currentTime();
    }
  }

  emit Redraw();
}
