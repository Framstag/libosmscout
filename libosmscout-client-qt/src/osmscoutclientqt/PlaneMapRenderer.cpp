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

#include <osmscoutclientqt/PlaneMapRenderer.h>
#include <osmscoutclientqt/OSMTile.h>

#include <osmscout/system/Math.h>

#include <QtMath>
#include <QDebug>

namespace osmscout {

// uncomment or define by compiler parameter to render various debug marks
// #define DRAW_DEBUG

// Timeout [ms] for the first rendering after rerendering was triggered (render what ever data is available)
static int INITIAL_DATA_RENDERING_TIMEOUT = 10;

// Timeout [ms] for the updated rendering after rerendering was triggered (more rendering data is available)
static int UPDATED_DATA_RENDERING_TIMEOUT = 200;

PlaneMapRenderer::PlaneMapRenderer(QThread *thread,
                                   SettingsRef settings,
                                   DBThreadRef dbThread,
                                   QString iconDirectory):
  MapRenderer(thread,settings,dbThread,iconDirectory),
  canvasOverrun(1.5),
  loadJob(nullptr),
  pendingRenderingTimer(this),
  currentImage(nullptr),
  currentCoord(0.0,0.0),
  currentAngle(0.0),
  finishedImage(nullptr),
  finishedCoord(0.0,0.0)
{
  pendingRenderingTimer.setSingleShot(true);

  //
  // Make sure that we decouple caller and receiver if SLOT method acquire locks,
  // even if they are running in the same thread
  // else we might get into a dead lock
  //

  connect(this, &PlaneMapRenderer::TriggerMapRenderingSignal,
          this, &PlaneMapRenderer::TriggerMapRendering,
          Qt::QueuedConnection);

  connect(this, &PlaneMapRenderer::TriggerInitialRendering,
          this, &PlaneMapRenderer::HandleInitialRenderingRequest);

  connect(&pendingRenderingTimer, &QTimer::timeout,
          this, &PlaneMapRenderer::DrawMap);

  connect(this, &PlaneMapRenderer::TriggerDrawMap,
          this, &PlaneMapRenderer::DrawMap,
          Qt::QueuedConnection);
}

PlaneMapRenderer::~PlaneMapRenderer()
{
  qDebug() << "~PlaneMapRenderer";
  if (currentImage!=nullptr)
    delete currentImage;
  if (finishedImage!=nullptr)
    delete finishedImage;
  if (loadJob!=nullptr)
    delete loadJob;
}

void PlaneMapRenderer::onDatabaseLoaded([[maybe_unused]] osmscout::GeoBox boundingBox)
{
  InvalidateVisualCache();
}

void PlaneMapRenderer::InvalidateVisualCache()
{
  {
    QMutexLocker finishedLocker(&finishedMutex);
    osmscout::log.Debug() << "Invalidate finished image";
    epoch++;
  }
  emit Redraw();
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

  if (finishedImage==nullptr) {
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
    emit TriggerMapRenderingSignal(request, epoch);
    return false;
  }

  osmscout::MercatorProjection requestProjection;

  if (!requestProjection.Set(request.coord,
                             request.angle.AsRadians(),
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

  osmscout::Vertex2D targetCenter;

  osmscout::GeoCoord srcImageCenterCoord;
  finalImgProjection.PixelToGeo(finalImgProjection.GetWidth()/2,
                                finalImgProjection.GetHeight()/2,
                                srcImageCenterCoord);

  requestProjection.GeoToPixel(srcImageCenterCoord,
                               targetCenter);
  double targetTopLeftX=targetCenter.GetX() - finalImgProjection.GetWidth()*scale*0.5;
  double targetTopLeftY=targetCenter.GetY() - finalImgProjection.GetHeight()*scale*0.5;

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
                      finishedAngle==request.angle.AsRadians() &&
                      finishedMagnification==request.magnification &&
                      finishedEpoch==epoch;

  if (!needsNoRepaint){
    {
      QMutexLocker reqLocker(&lastRequestMutex);
      lastRequest=extendedRequest;
    }
    emit TriggerMapRenderingSignal(extendedRequest, epoch);
  }

  painter.restore();
  return needsNoRepaint;
}

double PlaneMapRenderer::computeScale(const osmscout::MercatorProjection &previousProjection,
                                      const osmscout::MercatorProjection &currentProjection)
{
  double currentDiagonal=sqrt(pow(currentProjection.GetWidth(),2) + pow(currentProjection.GetHeight(),2));

  osmscout::GeoCoord topLeft;
  currentProjection.PixelToGeo(0,0,topLeft);
  osmscout::GeoCoord bottomRight;
  currentProjection.PixelToGeo(currentProjection.GetWidth(),currentProjection.GetHeight(),
                               bottomRight);

  osmscout::Vertex2D pos1;
  osmscout::Vertex2D pos2;
  previousProjection.GeoToPixel(topLeft,
                                pos1);
  previousProjection.GeoToPixel(bottomRight,
                                pos2);

  double previousDiagonal=sqrt(pow(pos2.GetX()-pos1.GetX(),2) + pow(pos2.GetY()-pos1.GetY(),2));
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
    if (loadJob==nullptr){
      return;
    }
    osmscout::log.Debug() << "DrawMap()";
    if (thread!=QThread::currentThread()){
      osmscout::log.Warn() << "Incorrect thread!";
    }

    if (currentImage==nullptr ||
        currentImage->width()!=(int)currentWidth ||
        currentImage->height()!=(int)currentHeight) {
      delete currentImage;

      currentImage=new QImage(QSize(currentWidth,
                                    currentHeight),
                              QImage::Format_RGBA8888_Premultiplied);
    }

    osmscout::MapParameter       drawParameter;
    std::list<std::string>       paths;

    paths.push_back(iconDirectory.toLocal8Bit().data());

    drawParameter.SetIconMode(osmscout::MapParameter::IconMode::Scalable);
    drawParameter.SetPatternMode(osmscout::MapParameter::PatternMode::Scalable);
    drawParameter.SetIconPaths(paths);
    drawParameter.SetPatternPaths(paths);
    drawParameter.SetDebugData(osmscout::log.IsDebug());
    drawParameter.SetDebugPerformance(osmscout::log.IsWarn());
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

    drawParameter.SetShowAltLanguage(showAltLanguage);

    drawParameter.SetLabelLineMinCharCount(5);
    drawParameter.SetLabelLineMaxCharCount(15);
    drawParameter.SetLabelLineFitToArea(true);
    drawParameter.SetLabelLineFitToWidth(std::min(projection.GetWidth(), projection.GetHeight())/canvasOverrun);

    drawParameter.GetLocaleRef().SetDistanceUnits(units == "imperial" ? osmscout::DistanceUnitSystem::Imperial : osmscout::DistanceUnitSystem::Metrics);

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
    osmscout::GeoBox renderBox(projection.GetDimensions());
    getOverlayObjects(overlayObjects, renderBox);

    bool success;
    {
      DBRenderJob job(renderProjection,
                      loadJob->GetAllTiles(),
                      &drawParameter,
                      &p,
                      overlayObjects,
                      dbThread->GetEmptyStyleConfig(),
                      /*drawCanvasBackground*/ true);
      dbThread->RunJob(std::bind(&DBRenderJob::Run, &job, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
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
      loadJob->Close();
      loadJob->deleteLater();
      loadJob=nullptr;
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
      finishedEpoch=currentEpoch;

      lastRendering.restart();
    }
  }
  emit Redraw();
}

void PlaneMapRenderer::HandleTileStatusChanged(QString /*dbPath*/,const osmscout::TileRef /*changedTile*/)
{
  QMutexLocker locker(&lock);
  int elapsedTime=lastRendering.isValid() ? lastRendering.elapsed() : UPDATED_DATA_RENDERING_TIMEOUT;

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

void PlaneMapRenderer::onLoadJobFinished(QMap<QString,QMap<osmscout::TileKey,osmscout::TileRef>>)
{
  emit TriggerDrawMap();
}

void PlaneMapRenderer::TriggerMapRendering(const MapViewStruct& request, size_t requestEpoch)
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
    if (loadJob!=nullptr){
      // TODO: check if job contains the same tiles...
      loadJob->Close();
      loadJob->deleteLater();
      loadJob=nullptr;
    }
    if (thread!=QThread::currentThread()){
      osmscout::log.Warn() << "Incorrect thread!";
    }

    currentWidth=request.width;
    currentHeight=request.height;
    currentCoord=request.coord;
    currentAngle=request.angle.AsRadians();
    currentMagnification=request.magnification;
    currentEpoch=requestEpoch;

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

    connect(loadJob, &DBLoadJob::tileStateChanged,
            this, &PlaneMapRenderer::HandleTileStatusChanged,
            Qt::QueuedConnection);
    connect(loadJob, &DBLoadJob::finished,
            this, &PlaneMapRenderer::onLoadJobFinished);

    dbThread->RunJob(std::bind(&DBLoadJob::Run, loadJob, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
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
        for (const auto &db:databases){
          auto styleConfig=db->GetStyleConfig();
          if (styleConfig){
            finishedUnknownFillStyle=styleConfig->GetUnknownFillStyle(projection);
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
}
