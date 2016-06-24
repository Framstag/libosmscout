/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2010  Tim Teulings

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "DBThread.h"

#include <iostream>

#include <QApplication>
#include <QMutexLocker>
#include <QDebug>
#include <QDir>

#include <osmscout/util/Logger.h>
#include <osmscout/util/StopClock.h>

#include <osmscout/system/Math.h>

// Timeout for the first rendering after rerendering was triggered (render what ever data is available)
static int INITIAL_DATA_RENDERING_TIMEOUT = 10;

// Timeout for the updated rendering after rerendering was triggered (more rendering data is available)
static int UPDATED_DATA_RENDERING_TIMEOUT = 200;

QBreaker::QBreaker()
  : osmscout::Breaker(),
    aborted(false)
{
}

bool QBreaker::Break()
{
  QMutexLocker locker(&mutex);
  aborted=true;

  return true;
}

bool QBreaker::IsAborted() const
{
  QMutexLocker locker(&mutex);

  return aborted;
}

void QBreaker::Reset()
{
  QMutexLocker locker(&mutex);

  aborted=false;
}


DBThread::DBThread()
 : database(std::make_shared<osmscout::Database>(databaseParameter)),
   locationService(std::make_shared<osmscout::LocationService>(database)),
   mapService(std::make_shared<osmscout::MapService>(database)),
   daylight(true),
   painter(NULL),
   iconDirectory(),
   pendingRenderingTimer(this),
   currentImage(NULL),
   currentCoord(0.0,0.0),
   currentAngle(0.0),
   currentMagnification(0),
   finishedImage(NULL),
   finishedCoord(0.0,0.0),
   finishedMagnification(0),
   dataLoadingBreaker(std::make_shared<QBreaker>())
{
  osmscout::log.Debug() << "DBThread::DBThread()";

  QScreen *srn=QApplication::screens().at(0);

  dpi=(double)srn->physicalDotsPerInch();

  pendingRenderingTimer.setSingleShot(true);

  connect(this,SIGNAL(TriggerInitialRendering()),
          this,SLOT(HandleInitialRenderingRequest()));

  connect(&pendingRenderingTimer,SIGNAL(timeout()),
          this,SLOT(DrawMap()));

  connect(this,SIGNAL(TileStatusChanged(const osmscout::TileRef&)),
          this,SLOT(HandleTileStatusChanged(const osmscout::TileRef&)));

  //
  // Make sure that we always decouple caller and receiver even if they are running in the same thread
  // else we might get into a dead lock
  //

  connect(this,SIGNAL(TriggerDrawMap()),
          this,SLOT(DrawMap()),
          Qt::QueuedConnection);

  osmscout::MapService::TileStateCallback callback=[this](const osmscout::TileRef& tile) {TileStateCallback(tile);};

  callbackId=mapService->RegisterTileStateCallback(callback);
}

DBThread::~DBThread()
{
  osmscout::log.Debug() << "DBThread::~DBThread()";

  if (painter!=NULL) {
    delete painter;
  }

  mapService->DeregisterTileStateCallback(callbackId);
}

void DBThread::FreeMaps()
{
  delete currentImage;
  currentImage=NULL;

  delete finishedImage;
  finishedImage=NULL;
}

bool DBThread::AssureRouter(osmscout::Vehicle /*vehicle*/)
{
  if (!database->IsOpen()) {
    return false;
  }

  if (!router/* ||
      (router && router->GetVehicle()!=vehicle)*/) {
    if (router) {
      if (router->IsOpen()) {
        router->Close();
      }
      router=NULL;
    }

    router=std::make_shared<osmscout::RoutingService>(database,
                                                      routerParameter,
                                                      osmscout::RoutingService::DEFAULT_FILENAME_BASE);

    if (!router->Open()) {
      return false;
    }
  }

  return true;
}

void DBThread::HandleInitialRenderingRequest()
{
  //std::cout << "Triggering initial data rendering timer..." << std::endl;
  pendingRenderingTimer.stop();
  pendingRenderingTimer.start(INITIAL_DATA_RENDERING_TIMEOUT);
}

void DBThread::HandleTileStatusChanged(const osmscout::TileRef& changedTile)
{
  QMutexLocker locker(&mutex);

  std::list<osmscout::TileRef> tiles;

  mapService->LookupTiles(projection,tiles);

  bool relevant=false;

  for (const auto tile : tiles) {
    if (tile==changedTile) {
      relevant=true;
      break;
    }
  }

  if (!relevant) {
    return;
  }

  int elapsedTime=lastRendering.elapsed();

  //std::cout << "Relevant tile changed " << elapsedTime << std::endl;

  if (pendingRenderingTimer.isActive()) {
    //std::cout << "Waiting for timer in " << pendingRenderingTimer.remainingTime() << std::endl;
  }
  else if (elapsedTime>UPDATED_DATA_RENDERING_TIMEOUT) {
    emit TriggerDrawMap();
  }
  else {
    //std::cout << "Triggering updated data rendering timer..." << std::endl;
    pendingRenderingTimer.start(UPDATED_DATA_RENDERING_TIMEOUT-elapsedTime);
  }
}

void DBThread::TileStateCallback(const osmscout::TileRef& changedTile)
{
  // We are in the context of one of the libosmscout worker threads
  emit TileStatusChanged(changedTile);
}

void DBThread::Initialize()
{
#ifdef __ANDROID__
    QStringList docPaths=QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation);

    QString databaseDirectory;

    // look for standard.oss in each directory
    for(int i=0; i < docPaths.size(); i++) {
        QStringList list_filters;
        list_filters << "osmscout";

        QDir path(docPaths[i]);
        QStringList list_files = path.entryList(list_filters,QDir::NoDotAndDotDot | QDir::Dirs);

        if(!(list_files.size() == 1)) {
            continue;
        }

        databaseDirectory=path.canonicalPath()+"/osmscout";
    }

    if(databaseDirectory.length() == 0) {
        qDebug() << "ERROR: map database directory not found";
    }
    else {
        qDebug() << "Loading database from " << databaseDirectory;
    }

    stylesheetFilename=databaseDirectory+"/standard.oss";

    qDebug() << "Loading style sheet from " << stylesheetFilename;

#else
  QStringList cmdLineArgs = QApplication::arguments();
  QString databaseDirectory = cmdLineArgs.size() > 1 ? cmdLineArgs.at(1) : QDir::currentPath();

  stylesheetFilename = cmdLineArgs.size() > 2 ? cmdLineArgs.at(2) : databaseDirectory + QDir::separator() + "standard.oss";
  iconDirectory = cmdLineArgs.size() > 3 ? cmdLineArgs.at(3) : databaseDirectory + QDir::separator() + "icons";
#endif

  if (database->Open(databaseDirectory.toLocal8Bit().data())) {
    osmscout::TypeConfigRef typeConfig=database->GetTypeConfig();

    if (typeConfig) {
      styleConfig=std::make_shared<osmscout::StyleConfig>(typeConfig);

      delete painter;
      painter=NULL;

      if (styleConfig->Load(stylesheetFilename.toLocal8Bit().data())) {
          painter=new osmscout::MapPainterQt(styleConfig);
      }
      else {
        qDebug() << "Cannot load style sheet!";
        styleConfig=NULL;
      }
    }
    else {
      qDebug() << "TypeConfig invalid!";
      styleConfig=NULL;
    }
  }
  else {
    qDebug() << "Cannot open database!";
    return;
  }


  DatabaseLoadedResponse response;

  if (!database->GetBoundingBox(response.boundingBox)) {
    qDebug() << "Cannot read initial bounding box";
    return;
  }

  lastRendering=QTime::currentTime();

  emit InitialisationFinished(response);
}

void DBThread::Finalize()
{
  FreeMaps();

  if (router && router->IsOpen()) {
    router->Close();
  }

  if (database->IsOpen()) {
    database->Close();
  }
}

void DBThread::GetProjection(osmscout::MercatorProjection& projection)
{
    QMutexLocker locker(&mutex);

    projection.Set(this->projection.GetCenter(),
                   this->projection.GetAngle(),
                   this->projection.GetMagnification(),
                   this->projection.GetDPI(),
                   this->projection.GetWidth(),
                   this->projection.GetHeight());
}

void DBThread::CancelCurrentDataLoading()
{
  dataLoadingBreaker->Break();
}

void DBThread::ToggleDaylight()
{
  QMutexLocker locker(&mutex);

  qDebug() << "Toggling daylight from " << daylight << " to " << !daylight << "...";

  if (!database->IsOpen()) {
    return;
  }

  osmscout::TypeConfigRef typeConfig=database->GetTypeConfig();

  if (!typeConfig) {
    return;
  }

  osmscout::StyleConfigRef newStyleConfig=std::make_shared<osmscout::StyleConfig>(typeConfig);

  newStyleConfig->AddFlag("daylight",!daylight);

  qDebug() << "Loading new stylesheet with daylight = " << newStyleConfig->GetFlagByName("daylight");

  if (newStyleConfig->Load(stylesheetFilename.toLocal8Bit().data())) {
    // Tear down
    delete painter;
    painter=NULL;

    // Recreate
    styleConfig=newStyleConfig;
    painter=new osmscout::MapPainterQt(styleConfig);

    daylight=!daylight;

    qDebug() << "Toggling daylight done.";
  }
}

void DBThread::ReloadStyle()
{
  qDebug() << "Reloading style...";

  QMutexLocker locker(&mutex);

  if (!database->IsOpen()) {
    return;
  }

  osmscout::TypeConfigRef typeConfig=database->GetTypeConfig();

  if (!typeConfig) {
    return;
  }

  osmscout::StyleConfigRef newStyleConfig=std::make_shared<osmscout::StyleConfig>(typeConfig);

  newStyleConfig->AddFlag("daylight",daylight);

  if (newStyleConfig->Load(stylesheetFilename.toLocal8Bit().data())) {
    // Tear down
    delete painter;
    painter=NULL;

    // Recreate
    styleConfig=newStyleConfig;
    painter=new osmscout::MapPainterQt(styleConfig);

    mapService->FlushTileCache();

    qDebug() << "Reloading style done.";
  }
}

/**
 * Triggers the loading of data for the given area and also triggers rendering
 * of the data afterwards.
 */
void DBThread::TriggerMapRendering(const RenderMapRequest& request)
{
  //std::cout << ">>> User triggered rendering" << std::endl;
  dataLoadingBreaker->Reset();

  {
    QMutexLocker locker(&mutex);

    currentWidth=request.width;
    currentHeight=request.height;
    currentCoord=request.coord;
    currentAngle=request.angle;
    currentMagnification=request.magnification;

    if (database->IsOpen() &&
        styleConfig) {
      osmscout::MapParameter        drawParameter;
      osmscout::AreaSearchParameter searchParameter;

      searchParameter.SetBreaker(dataLoadingBreaker);

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
                     dpi,
                     currentWidth,
                     currentHeight);

      std::list<osmscout::TileRef> tiles;

      mapService->LookupTiles(projection,tiles);
      if (!mapService->LoadMissingTileDataAsync(searchParameter,*styleConfig,tiles)) {
        qDebug() << "*** Loading of data has error or was interrupted";
        return;
      }

      emit TriggerInitialRendering();
    }
    else {
      qDebug() << "Cannot draw map: " << database->IsOpen() << " " << styleConfig.get();

      QPainter p;

      RenderMessage(p,request.width,request.height,"Database not open");
    }
  }
}

/**
 * Actual map drawing into the back buffer
 */
void DBThread::DrawMap()
{
  //std::cout << "DrawMap()" << std::endl;
  {
    QMutexLocker locker(&mutex);

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
    std::list<osmscout::TileRef> tiles;

    paths.push_back(iconDirectory.toLocal8Bit().data());

    drawParameter.SetIconPaths(paths);
    drawParameter.SetPatternPaths(paths);
    drawParameter.SetDebugData(false);
    drawParameter.SetDebugPerformance(true);
    drawParameter.SetOptimizeWayNodes(osmscout::TransPolygon::quality);
    drawParameter.SetOptimizeAreaNodes(osmscout::TransPolygon::quality);
    drawParameter.SetRenderBackground(true);
    drawParameter.SetRenderSeaLand(true);

    mapService->LookupTiles(projection,tiles);

    mapService->ConvertTilesToMapData(tiles,data);

    if (drawParameter.GetRenderSeaLand()) {
      mapService->GetGroundTiles(projection,
                                 data.groundTiles);
    }

    QPainter p;

    p.begin(currentImage);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    bool success=painter->DrawMap(projection,
                                  drawParameter,
                                  data,
                                  &p);

    p.end();

    if (!success)  {
      qDebug() << "*** Rendering of data has error or was interrupted";
      return;
    }

    std::swap(currentImage,finishedImage);

    finishedCoord=currentCoord;
    finishedAngle=currentAngle;
    finishedMagnification=currentMagnification;

    lastRendering=QTime::currentTime();
  }

  emit HandleMapRenderingResult();
}

void DBThread::RenderMessage(QPainter& painter, qreal width, qreal height, const char* message)
{
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setRenderHint(QPainter::TextAntialiasing);
  painter.setRenderHint(QPainter::SmoothPixmapTransform);

  painter.fillRect(0,0,width,height,
                   QColor::fromRgbF(0.0,0.0,0.0,1.0));

  painter.setPen(QColor::fromRgbF(1.0,1.0,1.0,1.0));

  QString text(message);

  painter.drawText(QRectF(0.0,0.0,width,height),
                   Qt::AlignCenter|Qt::AlignVCenter,
                   text,
                   NULL);
}

/**
 * Copies the last rendered map in the backbuffer to the given painter
 */
bool DBThread::RenderMap(QPainter& painter,
                         const RenderMapRequest& request)
{
  //std::cout << "RenderMap()" << std::endl;

  QMutexLocker locker(&mutex);

  if (finishedImage==NULL) {
    RenderMessage(painter,request.width,request.height,"no image rendered (internal error?)");

    // Since we assume that this is just a temporary problem, or we just were not instructed to render
    // a map yet, we trigger rendering an image...
    return false;
  }

  if (!styleConfig) {
    RenderMessage(painter,request.width,request.height,"no valid style sheet loaded");

    return true;
  }

  osmscout::MercatorProjection projection;

  projection.Set(finishedCoord,
                 finishedAngle,
                 finishedMagnification,
                 dpi,
                 finishedImage->width(),
                 finishedImage->height());

  projection.SetLinearInterpolationUsage(finishedMagnification.GetLevel() >= 10);

  osmscout::GeoBox boundingBox;

  projection.GetDimensions(boundingBox);

  double d=boundingBox.GetWidth()*2*M_PI/360;
  double scaleSize;
  size_t minScaleWidth=request.width/20;
  size_t maxScaleWidth=request.width/10;
  double scaleValue=d*180*60/M_PI*1852.216/(request.width/minScaleWidth);

  //std::cout << "1/10 screen (" << width/10 << " pixels) are: " << scaleValue << " meters" << std::endl;

  scaleValue=pow(10,floor(log10(scaleValue))+1);
  scaleSize=scaleValue/(d*180*60/M_PI*1852.216/request.width);

  if (scaleSize>minScaleWidth && scaleSize/2>minScaleWidth && scaleSize/2<=maxScaleWidth) {
    scaleValue=scaleValue/2;
    scaleSize=scaleSize/2;
  }
  else if (scaleSize>minScaleWidth && scaleSize/5>minScaleWidth && scaleSize/5<=maxScaleWidth) {
    scaleValue=scaleValue/5;
    scaleSize=scaleSize/5;
  }
  else if (scaleSize>minScaleWidth && scaleSize/10>minScaleWidth && scaleSize/10<=maxScaleWidth) {
    scaleValue=scaleValue/10;
    scaleSize=scaleSize/10;
  }

  //std::cout << "VisualScale: value: " << scaleValue << " pixel: " << scaleSize << std::endl;

  double dx=0;
  double dy=0;
  if (request.coord!=finishedCoord) {
    double rx,ry,fx,fy;

    projection.GeoToPixel(request.coord,
                          rx,
                          ry);
    projection.GeoToPixel(finishedCoord,
                          fx,
                          fy);
    dx=fx-rx;
    dy=fy-ry;
  }

  if (dx!=0 ||
      dy!=0) {
    osmscout::FillStyleRef unknownFillStyle;
    osmscout::Color backgroundColor;

    styleConfig->GetUnknownFillStyle(projection,
                                     unknownFillStyle);

    if (unknownFillStyle) {
      backgroundColor=unknownFillStyle->GetFillColor();
    }
    else {
      backgroundColor=osmscout::Color(0,0,0);
    }

    painter.fillRect(0,
                     0,
                     projection.GetWidth(),
                     projection.GetHeight(),
                     QColor::fromRgbF(backgroundColor.GetR(),
                                      backgroundColor.GetG(),
                                      backgroundColor.GetB(),
                                      backgroundColor.GetA()));
  }

  painter.drawImage(dx,dy,*finishedImage);

  bool needsNoRepaint=finishedImage->width()==(int) request.width &&
                      finishedImage->height()==(int) request.height &&
                      finishedCoord==request.coord &&
                      finishedAngle==request.angle &&
                      finishedMagnification==request.magnification;

  return needsNoRepaint;
}

osmscout::TypeConfigRef DBThread::GetTypeConfig() const
{
  return database->GetTypeConfig();
}

bool DBThread::GetNodeByOffset(osmscout::FileOffset offset,
                               osmscout::NodeRef& node) const
{
  return database->GetNodeByOffset(offset,node);
}

bool DBThread::GetAreaByOffset(osmscout::FileOffset offset,
                               osmscout::AreaRef& area) const
{
  return database->GetAreaByOffset(offset,area);
}

bool DBThread::GetWayByOffset(osmscout::FileOffset offset,
                              osmscout::WayRef& way) const
{
  return database->GetWayByOffset(offset,way);
}

bool DBThread::ResolveAdminRegionHierachie(const osmscout::AdminRegionRef& adminRegion,
                                           std::map<osmscout::FileOffset,osmscout::AdminRegionRef >& refs) const
{
  QMutexLocker locker(&mutex);

  return locationService->ResolveAdminRegionHierachie(adminRegion,
                                                      refs);
}

bool DBThread::SearchForLocations(const std::string& searchPattern,
                                  size_t limit,
                                  osmscout::LocationSearchResult& result) const
{
  QMutexLocker locker(&mutex);


  osmscout::LocationSearch search;

  search.limit=limit;

  if (!locationService->InitializeLocationSearchEntries(searchPattern,
                                                        search)) {
      return false;
  }

  return locationService->SearchForLocations(search,
                                             result);
}

bool DBThread::CalculateRoute(osmscout::Vehicle vehicle,
                              const osmscout::RoutingProfile& routingProfile,
                              const osmscout::ObjectFileRef& startObject,
                              size_t startNodeIndex,
                              const osmscout::ObjectFileRef targetObject,
                              size_t targetNodeIndex,
                              osmscout::RouteData& route)
{
  QMutexLocker locker(&mutex);

  if (!AssureRouter(vehicle)) {
    return false;
  }

  return router->CalculateRoute(routingProfile,
                                startObject,
                                startNodeIndex,
                                targetObject,
                                targetNodeIndex,
                                route);
}

bool DBThread::TransformRouteDataToRouteDescription(osmscout::Vehicle vehicle,
                                                    const osmscout::RoutingProfile& routingProfile,
                                                    const osmscout::RouteData& data,
                                                    osmscout::RouteDescription& description,
                                                    const std::string& start,
                                                    const std::string& target)
{
  QMutexLocker locker(&mutex);

  if (!AssureRouter(vehicle)) {
    return false;
  }

  if (!router->TransformRouteDataToRouteDescription(data,description)) {
    return false;
  }

  osmscout::TypeConfigRef typeConfig=router->GetTypeConfig();

  std::list<osmscout::RoutePostprocessor::PostprocessorRef> postprocessors;

  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::DistanceAndTimePostprocessor>());
  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::StartPostprocessor>(start));
  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::TargetPostprocessor>(target));
  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::WayNamePostprocessor>());
  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::CrossingWaysPostprocessor>());
  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::DirectionPostprocessor>());

  osmscout::RoutePostprocessor::InstructionPostprocessorRef instructionProcessor=std::make_shared<osmscout::RoutePostprocessor::InstructionPostprocessor>();

  instructionProcessor->AddMotorwayType(typeConfig->GetTypeInfo("highway_motorway"));
  instructionProcessor->AddMotorwayLinkType(typeConfig->GetTypeInfo("highway_motorway_link"));
  instructionProcessor->AddMotorwayType(typeConfig->GetTypeInfo("highway_motorway_trunk"));
  instructionProcessor->AddMotorwayType(typeConfig->GetTypeInfo("highway_trunk"));
  instructionProcessor->AddMotorwayLinkType(typeConfig->GetTypeInfo("highway_trunk_link"));
  instructionProcessor->AddMotorwayType(typeConfig->GetTypeInfo("highway_motorway_primary"));
  postprocessors.push_back(instructionProcessor);

  if (!routePostprocessor.PostprocessRouteDescription(description,
                                                      routingProfile,
                                                      *database,
                                                      postprocessors)) {
    return false;
  }

  return true;
}

bool DBThread::TransformRouteDataToWay(osmscout::Vehicle vehicle,
                                       const osmscout::RouteData& data,
                                       osmscout::Way& way)
{
  QMutexLocker locker(&mutex);

  if (!AssureRouter(vehicle)) {
    return false;
  }

  return router->TransformRouteDataToWay(data,way);
}


void DBThread::ClearRoute()
{
  {
    QMutexLocker locker(&mutex);

    data.poiWays.clear();

    FreeMaps();
  }

  emit Redraw();
}

void DBThread::AddRoute(const osmscout::Way& way)
{
  {
    QMutexLocker locker(&mutex);

    data.poiWays.push_back(std::make_shared<osmscout::Way>(way));

    FreeMaps();
  }

  emit Redraw();
}

bool DBThread::GetClosestRoutableNode(const osmscout::ObjectFileRef& refObject,
                                      const osmscout::Vehicle& vehicle,
                                      double radius,
                                      osmscout::ObjectFileRef& object,
                                      size_t& nodeIndex)
{
  QMutexLocker locker(&mutex);

  if (!AssureRouter(vehicle)) {
    return false;
  }

  object.Invalidate();

  if (refObject.GetType()==osmscout::refNode) {
    osmscout::NodeRef node;

    if (!database->GetNodeByOffset(refObject.GetFileOffset(),
                                   node)) {
      return false;
    }

    return router->GetClosestRoutableNode(node->GetCoords().GetLat(),
                                          node->GetCoords().GetLon(),
                                          vehicle,
                                          radius,
                                          object,
                                          nodeIndex);
  }
  else if (refObject.GetType()==osmscout::refArea) {
    osmscout::AreaRef area;

    if (!database->GetAreaByOffset(refObject.GetFileOffset(),
                                   area)) {
      return false;
    }

    osmscout::GeoCoord center;

    area->GetCenter(center);

    return router->GetClosestRoutableNode(center.GetLat(),
                                          center.GetLon(),
                                          vehicle,
                                          radius,
                                          object,
                                          nodeIndex);
  }
  else if (refObject.GetType()==osmscout::refWay) {
    osmscout::WayRef way;

    if (!database->GetWayByOffset(refObject.GetFileOffset(),
                                  way)) {
      return false;
    }

    return router->GetClosestRoutableNode(way->nodes[0].GetLat(),
                                          way->nodes[0].GetLon(),
                                          vehicle,
                                          radius,
                                          object,
                                          nodeIndex);
  }
  else {
    return true;
  }
}

static DBThread* dbThreadInstance=NULL;

bool DBThread::InitializeInstance()
{
  if (dbThreadInstance!=NULL) {
    return false;
  }

  dbThreadInstance=new DBThread();

  return true;
}

DBThread* DBThread::GetInstance()
{
  return dbThreadInstance;
}

void DBThread::FreeInstance()
{
  delete dbThreadInstance;

  dbThreadInstance=NULL;
}
