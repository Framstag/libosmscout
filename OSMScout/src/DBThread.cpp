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

#include <cmath>
#include <iostream>

#include <QMutexLocker>

#include <osmscout/StyleConfigLoader.h>

#include <osmscout/util/StopClock.h>

DBThread dbThread;

QBreaker::QBreaker()
  :osmscout::Breaker()
  ,aborted(false)
{
}

bool QBreaker::Break(){
  QMutexLocker locker(&mutex);
  aborted = true;
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
  aborted = false;
}


DBThread::DBThread()
 : database(databaseParameter),
   styleConfig(NULL),
   router(routerParameter),
   iconDirectory(),
   currentImage(NULL)
#if defined(HAVE_LIB_QTOPENGL)
   ,currentGLPixmap(NULL)
#endif
   ,currentLat(0.0)
   ,currentLon(0.0)
   ,currentMagnification(0)
   ,finishedImage(NULL)
#if defined(HAVE_LIB_QTOPENGL)
   ,finishedGLPixmap(NULL)
#endif
   ,finishedLat(0.0)
   ,finishedLon(0.0)
   ,finishedMagnification(0)
   ,currentRenderRequest()
   ,doRender(false)
   , renderBreaker(new QBreaker())
   , renderBreakerRef(renderBreaker)
{
}

void DBThread::FreeMaps()
{
#if defined(HAVE_LIB_QTOPENGL)
  delete currentGLPixmap;
  currentGLPixmap=NULL;

  delete finishedGLPixmap;
  finishedGLPixmap=NULL;
#endif

  delete currentImage;
  currentImage=NULL;

  delete finishedImage;
  finishedImage=NULL;
}

void DBThread::Initialize()
{
  QStringList cmdLineArgs = QApplication::arguments();
  QString databaseDirectory = cmdLineArgs.size() > 1 ? cmdLineArgs.at(1) : QDir::currentPath();
  QString stylesheetFilename = cmdLineArgs.size() > 2 ? cmdLineArgs.at(2) : databaseDirectory + QDir::separator() + "standard.oss";
  iconDirectory = cmdLineArgs.size() > 3 ? cmdLineArgs.at(3) : databaseDirectory + QDir::separator() + "icons";

  if (database.Open(databaseDirectory.toLocal8Bit().data()) &&
      router.Open(databaseDirectory.toLocal8Bit().data())) {
    if (database.GetTypeConfig()!=NULL) {
      styleConfig=new osmscout::StyleConfig(database.GetTypeConfig());

	  if (!osmscout::LoadStyleConfig(stylesheetFilename.toLocal8Bit().data(),
                                   *styleConfig)) {
        delete styleConfig;
        styleConfig=NULL;
      }
    }
    else {
      styleConfig=NULL;
    }

    osmscout::TypeId     type;
    osmscout::TypeConfig *typeConfig=router.GetTypeConfig();

    type=typeConfig->GetWayTypeId("highway_motorway");
    assert(type!=osmscout::typeIgnore);
    routingProfile.AddType(type,110.0);

    type=typeConfig->GetWayTypeId("highway_motorway_link");
    assert(type!=osmscout::typeIgnore);
    routingProfile.AddType(type,60.0);

    type=typeConfig->GetWayTypeId("highway_trunk");
    assert(type!=osmscout::typeIgnore);
    routingProfile.AddType(type,100.0);

    type=typeConfig->GetWayTypeId("highway_trunk_link");
    assert(type!=osmscout::typeIgnore);
    routingProfile.AddType(type,60.0);

    type=typeConfig->GetWayTypeId("highway_primary");
    assert(type!=osmscout::typeIgnore);
    routingProfile.AddType(type,70.0);

    type=typeConfig->GetWayTypeId("highway_primary_link");
    assert(type!=osmscout::typeIgnore);
    routingProfile.AddType(type,60.0);

    type=typeConfig->GetWayTypeId("highway_secondary");
    assert(type!=osmscout::typeIgnore);
    routingProfile.AddType(type,60.0);

    type=typeConfig->GetWayTypeId("highway_secondary_link");
    assert(type!=osmscout::typeIgnore);
    routingProfile.AddType(type,50.0);

    type=typeConfig->GetWayTypeId("highway_tertiary");
    assert(type!=osmscout::typeIgnore);
    routingProfile.AddType(type,55.0);

    type=typeConfig->GetWayTypeId("highway_unclassified");
    assert(type!=osmscout::typeIgnore);
    routingProfile.AddType(type,50.0);

    type=typeConfig->GetWayTypeId("highway_road");
    assert(type!=osmscout::typeIgnore);
    routingProfile.AddType(type,50.0);

    type=typeConfig->GetWayTypeId("highway_residential");
    assert(type!=osmscout::typeIgnore);
    routingProfile.AddType(type,40.0);

    type=typeConfig->GetWayTypeId("highway_living_street");
    assert(type!=osmscout::typeIgnore);
    routingProfile.AddType(type,10.0);

    type=typeConfig->GetWayTypeId("highway_service");
    assert(type!=osmscout::typeIgnore);
    routingProfile.AddType(type,30.0);
  }
  else {
    std::cerr << "Cannot open database!" << std::endl;
    return;
  }


  DatabaseLoadedResponse response;

  if (!database.GetBoundingBox(response.minLat,
                               response.minLon,
                               response.maxLat,
                               response.maxLon)) {
    std::cerr << "Cannot read initial bounding box" << std::endl;
    return;
  }

  std::cout << "Initial bounding box [";
  std::cout << response.minLat <<"," << response.minLon << " - " << response.maxLat << "," << response.maxLon << "]" << std::endl;

  emit InitialisationFinished(response);
}

void DBThread::Finalize(){
  FreeMaps();

  if (router.IsOpen()) {
    router.Close();
  }

  if (database.IsOpen()) {
    database.Close();
  }
}

void DBThread::UpdateRenderRequest(const RenderMapRequest& request)
{
  QMutexLocker locker(&mutex);

  currentRenderRequest=request;
  doRender=true;

  renderBreaker->Break();
}

void DBThread::TriggerMapRendering()
{
  RenderMapRequest request;
  {
    QMutexLocker locker(&mutex);

    request=currentRenderRequest;
    if (!doRender) {
      return;
    }

    doRender=false;

    renderBreaker->Reset();
  }

#if defined(HAVE_LIB_QTOPENGL)
  if (QGLFormat::hasOpenGL()) {
    if (currentGLPixmap==NULL ||
        currentGLPixmapSize.width()!=(int)request.width ||
        currentGLPixmapSize.height()!=(int)request.height) {

      QGLFormat format;
      format.setAlpha(true);
      format.setDepth(false);
      format.setDoubleBuffer(false);
      format.setSampleBuffers(true);

      delete currentGLPixmap;

      currentGLPixmapSize=QSize(request.width,request.height);
      currentGLPixmap=new QGLPixelBuffer(currentGLPixmapSize,format);
    }
  }

  if (currentGLPixmap==NULL ||
      !currentGLPixmap->isValid()) {
    if (currentImage==NULL ||
        currentImage->width()!=(int)request.width ||
        currentImage->height()!=(int)request.height) {
      delete currentImage;

      currentImage=new QImage(QSize(request.width,request.height),QImage::Format_RGB32);
    }
  }
#else
  if (currentImage==NULL ||
      currentImage->width()!=(int)request.width ||
      currentImage->height()!=(int)request.height) {
    delete currentImage;

    currentImage=new QImage(QSize(request.width,request.height),QImage::Format_RGB32);
  }
#endif

  currentLon=request.lon;
  currentLat=request.lat;
  currentMagnification=request.magnification;

  if (database.IsOpen() &&
      styleConfig!=NULL) {
    osmscout::MercatorProjection  projection;
    osmscout::MapParameter        drawParameter;
    osmscout::AreaSearchParameter searchParameter;

    searchParameter.SetBreaker(renderBreakerRef);

    std::list<std::string>        paths;

    paths.push_back(iconDirectory.toLocal8Bit().data());

    //drawParameter.SetDPI(QApplication::desktop()->physicalDpiX());
    drawParameter.SetIconPaths(paths);
    drawParameter.SetPatternPaths(paths);
    drawParameter.SetDebugPerformance(true);
    drawParameter.SetOptimizeWayNodes(osmscout::TransPolygon::quality);
    drawParameter.SetOptimizeAreaNodes(osmscout::TransPolygon::quality);
    drawParameter.SetBreaker(renderBreakerRef);

    std::cout << std::endl;

    osmscout::StopClock overallTimer;

    projection.Set(currentLon,
                   currentLat,
                   currentMagnification,
                   request.width,
                   request.height);

    osmscout::TypeSet              nodeTypes;
    std::vector<osmscout::TypeSet> wayTypes;
    osmscout::TypeSet              areaTypes;

    styleConfig->GetNodeTypesWithMaxMag(projection.GetMagnification(),
                                        nodeTypes);

    styleConfig->GetWayTypesByPrioWithMaxMag(projection.GetMagnification(),
                                             wayTypes);

    styleConfig->GetAreaTypesWithMaxMag(projection.GetMagnification(),
                                        areaTypes);

    osmscout::StopClock dataRetrievalTimer;

    database.GetObjects(nodeTypes,
                        wayTypes,
                        areaTypes,
                        projection.GetLonMin(),
                        projection.GetLatMin(),
                        projection.GetLonMax(),
                        projection.GetLatMax(),
                        projection.GetMagnification(),
                        searchParameter,
                        data.nodes,
                        data.ways,
                        data.areas,
                        data.relationWays,
                        data.relationAreas);
    /*
    database.GetGroundTiles(projection.GetLonMin(),
                            projection.GetLatMin(),
                            projection.GetLonMax(),
                            projection.GetLatMax(),
                            data.groundTiles);*/

    dataRetrievalTimer.Stop();

    osmscout::StopClock drawTimer;

    QPainter *p=NULL;

#if defined(HAVE_LIB_QTOPENGL)
    if (currentGLPixmap!=NULL &&
        currentGLPixmap->isValid()) {
      p=new QPainter(currentGLPixmap);
    }
    else {
      p=new QPainter(currentImage);
    }
#else
    p=new QPainter(currentImage);
#endif

    p->setRenderHint(QPainter::Antialiasing);
    p->setRenderHint(QPainter::TextAntialiasing);
    p->setRenderHint(QPainter::SmoothPixmapTransform);

    painter.DrawMap(*styleConfig,
                    projection,
                    drawParameter,
                    data,
                    p);

    delete p;

    drawTimer.Stop();
    overallTimer.Stop();

    std::cout << "All: " << overallTimer << " Data: " << dataRetrievalTimer << " Draw: " << drawTimer << std::endl;
  }
  else {
    std::cout << "Cannot draw map: " << database.IsOpen() << " " << (styleConfig!=NULL) << std::endl;

    QPainter *p=NULL;

#if defined(HAVE_LIB_QTOPENGL)
    if (currentGLPixmap!=NULL &&
        currentGLPixmap->isValid()) {
      p=new QPainter(currentGLPixmap);
    }
    else {
      p=new QPainter(currentImage);
    }
#else
    p=new QPainter(currentImage);
#endif

    p->setRenderHint(QPainter::Antialiasing);
    p->setRenderHint(QPainter::TextAntialiasing);
    p->setRenderHint(QPainter::SmoothPixmapTransform);

    p->fillRect(0,0,request.width,request.height,
                QColor::fromRgbF(0.0,0.0,0.0,1.0));

    p->setPen(QColor::fromRgbF(1.0,1.0,1.0,1.0));

    QString text("not initialized (yet)");

    p->drawText(QRect(0,0,request.width,request.height),
                Qt::AlignCenter|Qt::AlignVCenter,
                text,
                NULL);

    delete p;
  }

  QMutexLocker locker(&mutex);
  if (renderBreaker->IsAborted()) return;

  std::swap(currentImage,finishedImage);
#if defined(HAVE_LIB_QTOPENGL)
  std::swap(currentGLPixmap,finishedGLPixmap);
  std::swap(currentGLPixmapSize,finishedGLPixmapSize);
#endif
  std::swap(currentLon,finishedLon);
  std::swap(currentLat,finishedLat);
  std::swap(currentMagnification,finishedMagnification);

  emit HandleMapRenderingResult();
}

bool DBThread::RenderMap(QPainter& painter,
                         const RenderMapRequest& request)
{
  QMutexLocker locker(&mutex);

#if defined(HAVE_LIB_QTOPENGL)
  if (finishedImage==NULL &&
      finishedGLPixmap==NULL) {
#else
  if (finishedImage==NULL) {
#endif
    painter.fillRect(0,0,request.width,request.height,
                     QColor::fromRgbF(0.0,0.0,0.0,1.0));

    painter.setPen(QColor::fromRgbF(1.0,1.0,1.0,1.0));

    QString text("no map available");

    painter.drawText(QRectF(0,0,request.width,request.height),
                     text,
                     QTextOption(Qt::AlignCenter));

    return false;
  }

  osmscout::MercatorProjection projection;

#if defined(HAVE_LIB_QTOPENGL)
  if (finishedGLPixmap!=NULL) {
    projection.Set(finishedLon,finishedLat,
                   finishedMagnification,
                   finishedGLPixmapSize.width(),
                   finishedGLPixmapSize.height());
  }
  else {
    projection.Set(finishedLon,finishedLat,
                   finishedMagnification,
                   finishedImage->width(),
                   finishedImage->height());
  }
#else
  projection.Set(finishedLon,finishedLat,
                 finishedMagnification,
                 finishedImage->width(),
                 finishedImage->height());
#endif

  double lonMin,lonMax,latMin,latMax;

  projection.GetDimensions(lonMin,latMin,lonMax,latMax);

  double d=(lonMax-lonMin)*2*M_PI/360;
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
  if (request.lon!=finishedLon || request.lat!=finishedLat) {
    dx-=(request.lon-finishedLon)*request.width/(lonMax-lonMin);
    dy+=(request.lat-finishedLat)*request.height/(latMax-latMin);
  }

#if defined(HAVE_LIB_QTOPENGL)
  if (finishedGLPixmap!=NULL && finishedGLPixmap->isValid()) {
    QImage image=finishedGLPixmap->toImage();
    painter.drawImage(dx,dy,image);
  }
  else {
    painter.drawImage(dx,dy,*finishedImage);
  }
#else
  painter.drawImage(dx,dy,*finishedImage);
#endif

#if defined(HAVE_LIB_QTOPENGL)
  if (finishedGLPixmap!=NULL &&
      finishedGLPixmap->isValid()) {
    return finishedGLPixmapSize.width()==(int)request.width &&
           finishedGLPixmapSize.height()==(int)request.height &&
           finishedLon==request.lon &&
           finishedLat==request.lat &&
           finishedMagnification==request.magnification;
  }
  else {
    return finishedImage->width()==(int)request.width &&
           finishedImage->height()==(int)request.height &&
           finishedLon==request.lon &&
           finishedLat==request.lat &&
           finishedMagnification==request.magnification;
  }
#else
    return finishedImage->width()==(int)request.width &&
           finishedImage->height()==(int)request.height &&
           finishedLon==request.lon &&
           finishedLat==request.lat &&
           finishedMagnification==request.magnification;
#endif
}

bool DBThread::GetNode(osmscout::Id id, osmscout::NodeRef& node) const
{
  QMutexLocker locker(&mutex);

  return database.GetNode(id,node);
}

bool DBThread::GetWay(osmscout::Id id, osmscout::WayRef& way) const
{
  QMutexLocker locker(&mutex);

  return database.GetWay(id,way);
}

bool DBThread::GetRelation(osmscout::Id id, osmscout::RelationRef& relation) const
{
  QMutexLocker locker(&mutex);

  return database.GetRelation(id,relation);
}

bool DBThread::GetMatchingAdminRegions(const QString& name,
                                       std::list<osmscout::AdminRegion>& regions,
                                       size_t limit,
                                       bool& limitReached) const
{
  QMutexLocker locker(&mutex);

  return database.GetMatchingAdminRegions(name.toUtf8().data(),
                                          regions,
                                          limit,limitReached, false);
}

bool DBThread::GetMatchingLocations(const osmscout::AdminRegion& region,
                                    const QString& name,
                                    std::list<osmscout::Location>& locations,
                                    size_t limit,
                                    bool& limitReached) const
{
  QMutexLocker locker(&mutex);

  return database.GetMatchingLocations(region,
                                       name.toUtf8().data(),
                                       locations,
                                       limit,
                                       limitReached,
                                       false);
}

bool DBThread::CalculateRoute(osmscout::Id startWayId,
                              osmscout::Id startNodeId,
                              osmscout::Id targetWayId,
                              osmscout::Id targetNodeId,
                              osmscout::RouteData& route)
{
  QMutexLocker locker(&mutex);

  return router.CalculateRoute(routingProfile,
                               startWayId,
                               startNodeId,
                               targetWayId,
                               targetNodeId,
                               route);
}

bool DBThread::TransformRouteDataToRouteDescription(const osmscout::RouteData& data,
                                                    osmscout::RouteDescription& description,
                                                    const std::string& start,
                                                    const std::string& target)
{
  QMutexLocker locker(&mutex);

  if (!router.TransformRouteDataToRouteDescription(data,description)) {
    return false;
  }

  osmscout::TypeConfig *typeConfig=router.GetTypeConfig();

  std::list<osmscout::RoutePostprocessor::PostprocessorRef> postprocessors;

  postprocessors.push_back(new osmscout::RoutePostprocessor::DistanceAndTimePostprocessor());
  postprocessors.push_back(new osmscout::RoutePostprocessor::StartPostprocessor(start));
  postprocessors.push_back(new osmscout::RoutePostprocessor::TargetPostprocessor(target));
  postprocessors.push_back(new osmscout::RoutePostprocessor::WayNamePostprocessor());
  postprocessors.push_back(new osmscout::RoutePostprocessor::CrossingWaysPostprocessor());
  postprocessors.push_back(new osmscout::RoutePostprocessor::DirectionPostprocessor());

  osmscout::RoutePostprocessor::InstructionPostprocessor *instructionProcessor=new osmscout::RoutePostprocessor::InstructionPostprocessor();

  instructionProcessor->AddMotorwayType(typeConfig->GetWayTypeId("highway_motorway"));
  instructionProcessor->AddMotorwayLinkType(typeConfig->GetWayTypeId("highway_motorway_link"));
  instructionProcessor->AddMotorwayType(typeConfig->GetWayTypeId("highway_trunk"));
  instructionProcessor->AddMotorwayLinkType(typeConfig->GetWayTypeId("highway_trunk_link"));
  postprocessors.push_back(instructionProcessor);

  if (!routePostprocessor.PostprocessRouteDescription(description,
                                                      routingProfile,
                                                      database,
                                                      postprocessors)) {
    return false;
  }

  return true;
}

bool DBThread::TransformRouteDataToWay(const osmscout::RouteData& data,
                                       osmscout::Way& way)
{
  QMutexLocker locker(&mutex);

  return router.TransformRouteDataToWay(data,way);
}


void DBThread::ClearRoute()
{
  QMutexLocker locker(&mutex);

  data.poiWays.clear();

  FreeMaps();

  emit Redraw();
}

void DBThread::AddRoute(const osmscout::Way& way)
{
  QMutexLocker locker(&mutex);

  data.poiWays.push_back(way);

  FreeMaps();

  emit Redraw();
}

#include "moc_DBThread.cpp"
