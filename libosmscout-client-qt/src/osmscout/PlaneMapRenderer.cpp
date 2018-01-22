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

#include <osmscout/PlaneMapRenderer.h>
#include <osmscout/OSMTile.h>

#include <osmscout/system/Math.h>

// uncomment or define by compiler parameter to render various debug marks
// #define DRAW_DEBUG

// Timeout for the first rendering after rerendering was triggered (render what ever data is available)
static int INITIAL_DATA_RENDERING_TIMEOUT = 10;

// Timeout for the updated rendering after rerendering was triggered (more rendering data is available)
static int UPDATED_DATA_RENDERING_TIMEOUT = 200;

PlaneMapRenderer::PlaneMapRenderer(QThread *thread,
                                   SettingsRef settings,
                                   DBThreadRef dbThread,
                                   QString iconDirectory):
  MapRenderer(thread,settings,dbThread,iconDirectory),
  canvasOverrun(1.5),
  loadJob(NULL),
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

  //
  // Make sure that we decouple caller and receiver if SLOT method acquire locks,
  // even if they are running in the same thread
  // else we might get into a dead lock
  //

  connect(this,SIGNAL(TriggerMapRenderingSignal(const MapViewStruct&)),
          this,SLOT(TriggerMapRendering(const MapViewStruct&)),
          Qt::QueuedConnection);

  connect(this,SIGNAL(TriggerInitialRendering()),
          this,SLOT(HandleInitialRenderingRequest()));

  connect(&pendingRenderingTimer,SIGNAL(timeout()),
          this,SLOT(DrawMap()));

  connect(this,SIGNAL(TriggerDrawMap()),
          this,SLOT(DrawMap()),
          Qt::QueuedConnection);
}

PlaneMapRenderer::~PlaneMapRenderer()
{
  qDebug() << "~PlaneMapRenderer";
  if (currentImage!=NULL)
    delete currentImage;
  if (finishedImage!=NULL)
    delete finishedImage;
  if (loadJob!=NULL)
    delete loadJob;
}

void PlaneMapRenderer::InvalidateVisualCache()
{
  QMutexLocker finishedLocker(&finishedMutex);
  osmscout::log.Debug() << "Invalidate finished image";
  if (finishedImage)
    delete finishedImage;
  finishedImage=NULL;
}

/**
 * Render map defined by request to painter
 * @param painter
 * @param request
 * @return true if rendered map is complete
 */
bool PlaneMapRenderer::RenderMap(QPainter& painter,
                                 const MapViewStruct& request)
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

  if (!requestProjection.Set(request.coord,
                             request.angle,
                             request.magnification,
                             mapDpi,
                             request.width,
                             request.height)) {
    osmscout::log.Warn() << "Invalid request projection!";
    return false;
  }

  osmscout::MercatorProjection finalImgProjection;

  if (!finalImgProjection.Set(finishedCoord,
                              finishedAngle,
                              finishedMagnification,
                              mapDpi,
                              finishedImage->width(),
                              finishedImage->height())) {
    osmscout::log.Warn() << "Invalid finished projection!";
    return false;
  }

  // projection bounding box may be smaller than projection dimensions...
  double scale=computeScale(finalImgProjection,requestProjection);

  QRectF sourceRectangle(0,
                         0,
                         finalImgProjection.GetWidth(),
                         finalImgProjection.GetHeight());

  double targetCenterX;
  double targetCenterY;

  osmscout::GeoCoord srcImageCenterCoord;
  finalImgProjection.PixelToGeo(finalImgProjection.GetWidth()/2,
                                finalImgProjection.GetHeight()/2,
                                srcImageCenterCoord);

  requestProjection.GeoToPixel(srcImageCenterCoord,targetCenterX,targetCenterY);
  double targetTopLeftX=targetCenterX - finalImgProjection.GetWidth()*scale*0.5;
  double targetTopLeftY=targetCenterY - finalImgProjection.GetHeight()*scale*0.5;

  QRectF targetRectangle(targetTopLeftX,
                         targetTopLeftY,
                         finalImgProjection.GetWidth()*scale,
                         finalImgProjection.GetHeight()*scale);


  // check if transformed final img cover current canvas...
  if (finalImgProjection.GetAngle()!=requestProjection.GetAngle() ||
      targetRectangle.top()>0 || targetRectangle.left()>0 ||
      targetRectangle.bottom()<requestProjection.GetHeight() || targetRectangle.right()<requestProjection.GetWidth()) {
    // ...if not, there is necessary to draw some background
    painter.fillRect(0,
                     0,
                     request.width,
                     request.height,
                     QColor::fromRgbF(backgroundColor.GetR(),
                                      backgroundColor.GetG(),
                                      backgroundColor.GetB(),
                                      backgroundColor.GetA()));
  }

  painter.save();
  if (finalImgProjection.GetAngle()!=requestProjection.GetAngle()){
    // rotate final image
    QPointF rotationCenter(targetRectangle.x()+targetRectangle.width()/2.0,
                           targetRectangle.y()+targetRectangle.height()/2.0);
    painter.translate(rotationCenter);
    painter.rotate(qRadiansToDegrees(requestProjection.GetAngle()-finalImgProjection.GetAngle()));
    painter.translate(rotationCenter*-1.0);
  }

  // After our computations with float numbers, target rectangle is not aligned to pixel.
  // It leads to additional anti-aliasing that blurs output...
  double absDiff=std::abs((targetRectangle.x()-targetTopLeftX) - sourceRectangle.x()) +
                 std::abs((targetRectangle.y()-targetTopLeftY) - sourceRectangle.y()) +
                 std::abs(targetRectangle.width()              - sourceRectangle.width()) +
                 std::abs(targetRectangle.height()             - sourceRectangle.height());

  // ...for that reason, when rectangles are (almost) the same,
  // round target position to get better output
  if (absDiff < 1e-3 && finalImgProjection.GetAngle()==requestProjection.GetAngle()){
    targetRectangle.setX(sourceRectangle.x() + round(targetTopLeftX));
    targetRectangle.setY(sourceRectangle.y() + round(targetTopLeftY));
    targetRectangle.setSize(sourceRectangle.size());
  }

  painter.drawImage(targetRectangle,
                    *finishedImage,
                    sourceRectangle);

#ifdef DRAW_DEBUG
  painter.resetTransform();
  double lon,lat;
  finalImgProjection.PixelToGeo(0,0,lon,lat);
  osmscout::GeoCoord topLeft(lat,lon);
  finalImgProjection.PixelToGeo(finalImgProjection.GetWidth(),finalImgProjection.GetHeight(),lon,lat);
  osmscout::GeoCoord bottomRight(lat,lon);
  finalImgProjection.PixelToGeo(finalImgProjection.GetWidth(),0,lon,lat);
  osmscout::GeoCoord topRight(lat,lon);
  finalImgProjection.PixelToGeo(0,finalImgProjection.GetHeight(),lon,lat);
  osmscout::GeoCoord bottomLeft(lat,lon);

  painter.setPen(QColor::fromRgbF(0,0,1));
  double x1,y1,x2,y2;
  requestProjection.GeoToPixel(topLeft,x1,y1);
  requestProjection.GeoToPixel(bottomRight,x2,y2);
  painter.drawLine(x1,y1,x2,y2);
  requestProjection.GeoToPixel(topRight,x1,y1);
  requestProjection.GeoToPixel(bottomLeft,x2,y2);
  painter.drawLine(x1,y1,x2,y2);
#endif

  MapViewStruct extendedRequest=request;
  extendedRequest.width*=canvasOverrun;
  extendedRequest.height*=canvasOverrun;
  bool needsNoRepaint=finishedImage->width()==(int) extendedRequest.width &&
                      finishedImage->height()==(int) extendedRequest.height &&
                      finishedCoord==request.coord &&
                      finishedAngle==request.angle &&
                      finishedMagnification==request.magnification;

  if (!needsNoRepaint){
    {
      QMutexLocker reqLocker(&lastRequestMutex);
      lastRequest=extendedRequest;
    }
    emit TriggerMapRenderingSignal(extendedRequest);
  }

  painter.restore();
  return needsNoRepaint;
}

double PlaneMapRenderer::computeScale(const osmscout::MercatorProjection &previousProjection,
                                      const osmscout::MercatorProjection &currentProjection)
{
  double currentDiagonal=sqrt(pow(currentProjection.GetWidth(),2) + pow(currentProjection.GetHeight(),2));

  double topLeftLon;
  double topLeftLat;
  currentProjection.PixelToGeo(0,0,topLeftLon,topLeftLat);
  double bottomRightLon;
  double bottomRightLat;
  currentProjection.PixelToGeo(currentProjection.GetWidth(),currentProjection.GetHeight(),
                               bottomRightLon,bottomRightLat);

  double x1;
  double y1;
  previousProjection.GeoToPixel(osmscout::GeoCoord(topLeftLat,topLeftLon),x1,y1);
  double x2;
  double y2;
  previousProjection.GeoToPixel(osmscout::GeoCoord(bottomRightLat,bottomRightLon),x2,y2);

  double previousDiagonal=sqrt(pow(x2-x1,2) + pow(y2-y1,2));
  return currentDiagonal / previousDiagonal;
}

void PlaneMapRenderer::Initialize()
{
}

/**
 * Actual map drawing into the back buffer
 */
void PlaneMapRenderer::DrawMap()
{
  {
    QMutexLocker locker(&lock);
    if (loadJob==NULL){
      return;
    }
    osmscout::log.Debug() << "DrawMap()";
    if (thread!=QThread::currentThread()){
      osmscout::log.Warn() << "Incorrect thread!";
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
    // We want to get notified, if we have more than 1000 objects from a certain type (=> move type rendering to a higher zoom level?)
    drawParameter.SetWarningObjectCountLimit(1000);
    // We want to get notified, if we have more than 20000 coords from a certain type (=> move type rendering to a higher zoom level?)
    drawParameter.SetWarningCoordCountLimit(20000);

    // optimize process can reduce number of nodes before rendering
    // it helps for slow renderer backend, but it cost some cpu
    // it seems that it is ok to disable it for Qt
    drawParameter.SetOptimizeWayNodes(osmscout::TransPolygon::none);
    drawParameter.SetOptimizeAreaNodes(osmscout::TransPolygon::none);

    drawParameter.SetRenderBackground(false); // we draw background before MapPainter
    drawParameter.SetRenderUnknowns(false); // it is necessary to disable it with multiple databases
    drawParameter.SetRenderSeaLand(renderSea);

    drawParameter.SetFontName(fontName.toStdString());
    drawParameter.SetFontSize(fontSize);

    drawParameter.SetLabelLineMinCharCount(15);
    drawParameter.SetLabelLineMaxCharCount(30);
    drawParameter.SetLabelLineFitToArea(true);
    drawParameter.SetLabelLineFitToWidth(std::min(projection.GetWidth(), projection.GetHeight())/canvasOverrun);

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

    // overlay objects
    std::vector<OverlayObjectRef> overlayObjects;
    osmscout::GeoBox renderBox;
    projection.GetDimensions(renderBox);
    getOverlayObjects(overlayObjects, renderBox);

    bool success;
    {
      DBRenderJob job(renderProjection,
                      loadJob->GetAllTiles(),
                      &drawParameter,
                      &p,
                      overlayObjects,
                      /*drawCanvasBackground*/ true);
      dbThread->RunJob(&job);
      success=job.IsSuccess();
    }

#ifdef DRAW_DEBUG
    p.setPen(QColor::fromRgbF(1,0,0));
    p.drawLine(0,0, currentImage->width(), currentImage->height());
    p.drawLine(currentImage->width(),0, 0, currentImage->height());
#endif

    p.end();

    if (loadJob->IsFinished()){
      // this slot is may be called from DBLoadJob, we can't delete it now
      loadJob->deleteLater();
      loadJob=NULL;
    }

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

void PlaneMapRenderer::HandleTileStatusChanged(QString /*dbPath*/,const osmscout::TileRef /*changedTile*/)
{
  QMutexLocker locker(&lock);
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

void PlaneMapRenderer::onLoadJobFinished(QMap<QString,QMap<osmscout::TileId,osmscout::TileRef>>)
{
  emit TriggerDrawMap();
}

void PlaneMapRenderer::TriggerMapRendering(const MapViewStruct& request)
{
  {
    QMutexLocker reqLocker(&lastRequestMutex);
    if (request!=lastRequest){
      return;
    }
  }

  osmscout::log.Debug() << "Start data loading...";
  {
    QMutexLocker locker(&lock);
    if (loadJob!=NULL){
      // TODO: check if job contains same tiles...
      loadJob->deleteLater();
      loadJob=NULL;
    }
    if (thread!=QThread::currentThread()){
      osmscout::log.Warn() << "Incorrect thread!";
    }

    currentWidth=request.width;
    currentHeight=request.height;
    currentCoord=request.coord;
    currentAngle=request.angle;
    currentMagnification=request.magnification;

    projection.Set(currentCoord,
                   currentAngle,
                   currentMagnification,
                   mapDpi,
                   currentWidth,
                   currentHeight);

    unsigned long maximumAreaLevel=4;
    if (currentMagnification.GetLevel() >= 15) {
      maximumAreaLevel=6;
    }

    loadJob=new DBLoadJob(projection,
                          maximumAreaLevel,
                          /* lowZoomOptimization */ true,
                          /* closeOnFinish */ false);

    connect(loadJob, SIGNAL(tileStateChanged(QString,const osmscout::TileRef)),
            this, SLOT(HandleTileStatusChanged(QString,const osmscout::TileRef)),
            Qt::QueuedConnection);
    connect(loadJob, SIGNAL(finished(QMap<QString,QMap<osmscout::TileId,osmscout::TileRef>>)),
            this, SLOT(onLoadJobFinished(QMap<QString,QMap<osmscout::TileId,osmscout::TileRef>>)));

    dbThread->RunJob(loadJob);
  }
  emit TriggerInitialRendering();
}

void PlaneMapRenderer::HandleInitialRenderingRequest()
{
  if (pendingRenderingTimer.isActive())
    return; // avoid repeated draw postpone (data loading may be called very fast)

  osmscout::log.Debug() << "Start rendering timer:" << INITIAL_DATA_RENDERING_TIMEOUT << "ms";
  pendingRenderingTimer.stop();
  pendingRenderingTimer.start(INITIAL_DATA_RENDERING_TIMEOUT);
}

void PlaneMapRenderer::onStylesheetFilenameChanged()
{
  {
    QMutexLocker locker(&lock);
    QMutexLocker finishedLocker(&finishedMutex);

    dbThread->RunSynchronousJob(
      [this](const std::list<DBInstanceRef>& databases) {
        for (auto &db:databases){
          if (db->styleConfig){
            finishedUnknownFillStyle=db->styleConfig->GetUnknownFillStyle(projection);
            if (finishedUnknownFillStyle){
              break;
            }
          }
        }
      }
    );
  }

  MapRenderer::onStylesheetFilenameChanged();
  emit Redraw();
}
