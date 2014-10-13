/*
  TravelJinni - Openstreetmap offline viewer
  Copyright (C) 2009  Tim Teulings

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

#include "DatabaseTask.h"

#include <cmath>
#include <exception>
#include <iostream>

#if defined(__WIN32__) || defined(WIN32) || defined(__APPLE__)
  #include <cairo.h>
#else
  #include <cairo/cairo.h>
#endif

#include <osmscout/util/StopClock.h>

#include <Lum/Base/String.h>

#if defined(LUM_HAVE_LIB_CAIRO)
  #include <Lum/OS/Cairo/Driver.h>
#endif

#if defined(LUM_HAVE_LIB_WIN32)
  #include <Lum/OS/Win32/DrawInfo.h>
  #include <Lum/OS/Win32/Display.h>
#endif

#include <Lum/OS/Display.h>
#include <Lum/OS/Driver.h>

// Cairo includes X11, defines Status
#if defined(Status)
  #undef Status
#endif
#include <Lum/Base/Path.h>

#include "Configuration.h"

DatabaseTask::DatabaseTask(const osmscout::DatabaseRef& database,
                           const osmscout::LocationServiceRef& locationService,
                           const osmscout::MapServiceRef& mapService,
                           Lum::Model::Action* jobFinishedAction)
 : database(database),
   locationService(locationService),
   mapService(mapService),
   painter(NULL),
   finish(false),
   newJob(NULL),
   currentJob(NULL),
   finishedJob(NULL),
   jobFinishedAction(jobFinishedAction),
   currentSurface(NULL),
   currentCairo(NULL),
   currentWidth(0),
   currentHeight(0),
   finishedSurface(NULL),
   finishedCairo(NULL)
{
  // no code
}

void DatabaseTask::GetCarSpeedTable(std::map<std::string,double>& map)
{
  map["highway_motorway"]=110.0;
  map["highway_motorway_trunk"]=100.0;
  map["highway_motorway_primary"]=70.0;
  map["highway_motorway_link"]=60.0;
  map["highway_motorway_junction"]=60.0;
  map["highway_trunk"]=100.0;
  map["highway_trunk_link"]=60.0;
  map["highway_primary"]=70.0;
  map["highway_primary_link"]=60.0;
  map["highway_secondary"]=60.0;
  map["highway_secondary_link"]=50.0;
  map["highway_tertiary"]=55.0;
  map["highway_unclassified"]=50.0;
  map["highway_road"]=50.0;
  map["highway_residential"]=40.0;
  map["highway_roundabout"]=40.0;
  map["highway_living_street"]=10.0;
  map["highway_service"]=30.0;
}


void DatabaseTask::Run()
{
  while (true) {
    mutex.Lock();

    while (newJob==NULL && !finish) {
      condition.Wait(mutex);
    }

    if (finish) {
      mutex.Unlock();
      return;
    }

    if (newJob!=NULL) {
      currentJob=newJob;
      newJob=NULL;
    }

    mutex.Unlock();

    if (currentJob!=NULL) {
      if (currentSurface==NULL ||
          currentWidth!=currentJob->width ||
          currentHeight!=currentJob->height) {
        cairo_destroy(currentCairo);
        cairo_surface_destroy(currentSurface);

        currentWidth=currentJob->width;
        currentHeight=currentJob->height;

        currentSurface=cairo_image_surface_create(CAIRO_FORMAT_RGB24,
                                                  currentWidth,currentHeight);
        currentCairo=cairo_create(currentSurface);
      }

      currentLon=currentJob->lon;
      currentLat=currentJob->lat;
      currentMagnification=currentJob->magnification;

      if (database->IsOpen() &&
          styleConfig.Valid()) {
        osmscout::MercatorProjection  projection;
        osmscout::MapParameter        drawParameter;
        osmscout::AreaSearchParameter searchParameter;

        std::list<std::string>        paths;

        paths.push_back("../libosmscout/data/icons/14x14/standard/");

        searchParameter.SetMaximumNodes(maxNodes->Get());
        searchParameter.SetMaximumWays(maxWays->Get());
        searchParameter.SetMaximumAreas(maxAreas->Get());

        searchParameter.SetUseLowZoomOptimization(true);
        searchParameter.SetUseMultithreading(currentMagnification.GetMagnification()<=osmscout::Magnification::magCity);

        drawParameter.SetIconPaths(paths);
        drawParameter.SetPatternPaths(paths);

        drawParameter.SetOptimizeWayNodes(optimizeWays->Get() ? osmscout::TransPolygon::quality : osmscout::TransPolygon::none);
        drawParameter.SetOptimizeAreaNodes(optimizeAreas->Get() ? osmscout::TransPolygon::quality : osmscout::TransPolygon::none);

        drawParameter.SetDrawWaysWithFixedWidth(false);

        drawParameter.SetRenderSeaLand(true);

        drawParameter.SetDebugPerformance(true);

        if (!dpi->IsNull()) {
          drawParameter.SetDPI(dpi->GetDouble());
        }
        else {
          drawParameter.SetDPI(Lum::OS::display->GetDPI());
        }

        std::cout << std::endl;

        osmscout::StopClock overallTimer;

        projection.Set(currentLon,
                       currentLat,
                       currentMagnification,
                       currentWidth,
                       currentHeight);
/*
        double width=10*drawParameter.GetDPI()/25.4;
        std::cout << "10mm => " << width << std::endl;
        double areaMinDegree=width/currentWidth*(projection.GetLonMax()-projection.GetLonMin());

        size_t level=0;
        double levelDegree=360.0;

        std::cout << "Minimum area degree: " << areaMinDegree << std::endl;
        while (areaMinDegree<levelDegree) {
          levelDegree=levelDegree/2;
          level++;
        }

        level=level-1;


        //searchParameter.SetMaximumAreaLevel(level-log2(currentMagnification));

        std::cout << "Resulting max area level: " << level-log2(currentMagnification) << " <=> " << searchParameter.GetMaximumAreaLevel() << std::endl;
*/

        osmscout::TypeSet              nodeTypes;
        std::vector<osmscout::TypeSet> wayTypes;
        osmscout::TypeSet              areaTypes;

        try {
          styleConfig->GetNodeTypesWithMaxMag(projection.GetMagnification(),
                                              nodeTypes);

          styleConfig->GetWayTypesByPrioWithMaxMag(projection.GetMagnification(),
                                                   wayTypes);
        }
        catch (std::exception& e) {
          std::cerr << "Exception while fetching style data: " << e.what() << std::endl;
        }

          styleConfig->GetAreaTypesWithMaxMag(projection.GetMagnification(),
                                              areaTypes);

        osmscout::StopClock dataRetrievalTimer;

        mapService->GetObjects(nodeTypes,
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
                               data.areas);

        if (drawParameter.GetRenderSeaLand()) {
          mapService->GetGroundTiles(projection.GetLonMin(),
                                     projection.GetLatMin(),
                                     projection.GetLonMax(),
                                     projection.GetLatMax(),
                                     projection.GetMagnification(),
                                     data.groundTiles);
        }

        dataRetrievalTimer.Stop();

        osmscout::StopClock drawTimer;

        cairo_set_tolerance(currentCairo,0.7);

        try {
          painter->DrawMap(projection,
                           drawParameter,
                           data,
                           currentCairo);

          drawTimer.Stop();
          overallTimer.Stop();

          std::cout << "All: " << overallTimer << " Data: " << dataRetrievalTimer << " Draw: " << drawTimer << std::endl;
        }
        catch (std::exception& e) {
          std::cerr << "Exception while rendering: " << e.what() << std::endl;
        }
      }
      else {
        std::cout << "Cannot draw map: " << database->IsOpen() << " " << (styleConfig.Valid()) << std::endl;
        cairo_save(currentCairo);
        cairo_set_source_rgb(currentCairo,0,0,0);
        cairo_paint(currentCairo);
        cairo_restore(currentCairo);
      }

      mutex.Lock();

      if (finishedJob!=NULL) {
        delete finishedJob;
      }

      finishedJob=currentJob;
      currentJob=NULL;

      std::swap(currentSurface,finishedSurface);
      std::swap(currentCairo,finishedCairo);
      std::swap(currentWidth,finishedWidth);
      std::swap(currentHeight,finishedHeight);
      std::swap(currentLon,finishedLon);
      std::swap(currentLat,finishedLat);
      std::swap(currentMagnification,finishedMagnification);

      Lum::OS::display->QueueActionForAsyncNotification(jobFinishedAction);

      mutex.Unlock();
    }
  }
}

void DatabaseTask::Finish()
{
  Lum::OS::Guard<Lum::OS::Mutex> guard(mutex);

  finish=true;
}

bool DatabaseTask::Open(const std::wstring& path)
{
  Lum::OS::Guard<Lum::OS::Mutex> guard(mutex);

  Lum::Base::Path  p;

  p.SetNativeDir(path);

  if (!database->Open(Lum::Base::WStringToString(p.GetPath()).c_str())) {
    return false;
  }

  osmscout::RouterParameter routerParameter;

  routerParameter.SetDebugPerformance(true);

  router=new osmscout::RoutingService(database,
                                      routerParameter,
                                      osmscout::vehicleCar);

  if (!router->Open()) {
    return false;
  }

  std::map<std::string,double> speedMap;

  GetCarSpeedTable(speedMap);

  routingProfile=new osmscout::FastestPathRoutingProfile(database->GetTypeConfig());
  routingProfile->ParametrizeForCar(*router->GetTypeConfig(),
                                   speedMap,
                                   160.0);

  return true;
}

bool DatabaseTask::IsOpen() const
{
  Lum::OS::Guard<Lum::OS::Mutex> guard(mutex);

  return database->IsOpen() && router.Valid() && router->IsOpen();
}

void DatabaseTask::Close()
{
  Lum::OS::Guard<Lum::OS::Mutex> guard(mutex);

  if (router.Valid()) {
    router->Close();
  }

  database->Close();
  routingProfile=NULL;
}


void DatabaseTask::FlushCache()
{
  Lum::OS::Guard<Lum::OS::Mutex> guard(mutex);

  database->FlushCache();
}

bool DatabaseTask::LoadStyleConfig(const std::wstring& filename)
{
  Lum::OS::Guard<Lum::OS::Mutex> guard(mutex);

  Lum::Base::Path f;

  f.SetNativePath(filename);

  osmscout::TypeConfigRef typeConfig=database->GetTypeConfig();

  if (typeConfig.Valid()) {
    styleConfig=new osmscout::StyleConfig(typeConfig);

    delete painter;
    painter=NULL;

    if (styleConfig->Load(Lum::Base::WStringToString(f.GetPath()).c_str())) {
      painter=new osmscout::MapPainterCairo(styleConfig);

      return true;
    }
    else {
      styleConfig=NULL;

      return false;
    }
  }
  else {
    styleConfig=NULL;
    return false;
  }
}

bool DatabaseTask::GetBoundingBox(double& minLat,double& minLon,
                                  double& maxLat,double& maxLon) const
{
  Lum::OS::Guard<Lum::OS::Mutex> guard(mutex);

  if (!database->IsOpen()) {
    return false;
  }

  return database->GetBoundingBox(minLat,minLon,maxLat,maxLon);
}

bool DatabaseTask::GetWayByOffset(osmscout::FileOffset offset,
                                  osmscout::WayRef& way) const
{
  Lum::OS::Guard<Lum::OS::Mutex> guard(mutex);

  if (!database->IsOpen()) {
    return false;
  }

  return database->GetWayByOffset(offset,way);
}

bool DatabaseTask::SearchForLocations(const osmscout::LocationSearch& search,
                                      osmscout::LocationSearchResult& result) const
{
  Lum::OS::Guard<Lum::OS::Mutex> guard(mutex);

  if (!database->IsOpen()) {
    return false;
  }

  return locationService->SearchForLocations(search,
                                             result);
}

bool DatabaseTask::CalculateRoute(const osmscout::ObjectFileRef& startObject,
                                  size_t startNodeIndex,
                                  const osmscout::ObjectFileRef& targetObject,
                                  size_t targetNodeIndex,
                                  osmscout::RouteData& route)
{
  Lum::OS::Guard<Lum::OS::Mutex> guard(mutex);

  if (!router.Valid() ||
      !router->IsOpen()) {
    return false;
  }

  return router->CalculateRoute(routingProfile,
                                startObject,
                                startNodeIndex,
                                targetObject,
                                targetNodeIndex,
                                route);
}

bool DatabaseTask::TransformRouteDataToRouteDescription(const osmscout::RouteData& data,
                                                        osmscout::RouteDescription& description,
                                                        const std::string& start,
                                                        const std::string& target)
{
  Lum::OS::Guard<Lum::OS::Mutex> guard(mutex);

  if (!database->IsOpen()) {
    return false;
  }

  if (!router.Valid() ||
      !router->IsOpen()) {
    return false;
  }

  if (!router->TransformRouteDataToRouteDescription(data,description)) {
    return false;
  }

  osmscout::TypeConfig *typeConfig=router->GetTypeConfig();

  std::list<osmscout::RoutePostprocessor::PostprocessorRef> postprocessors;

  postprocessors.push_back(new osmscout::RoutePostprocessor::DistanceAndTimePostprocessor());
  postprocessors.push_back(new osmscout::RoutePostprocessor::StartPostprocessor(start));
  postprocessors.push_back(new osmscout::RoutePostprocessor::TargetPostprocessor(target));
  postprocessors.push_back(new osmscout::RoutePostprocessor::WayNamePostprocessor());
  postprocessors.push_back(new osmscout::RoutePostprocessor::CrossingWaysPostprocessor());
  postprocessors.push_back(new osmscout::RoutePostprocessor::DirectionPostprocessor());

  osmscout::RoutePostprocessor::InstructionPostprocessor *instructionProcessor=new osmscout::RoutePostprocessor::InstructionPostprocessor();

  instructionProcessor->AddMotorwayType(typeConfig->GetTypeInfo("highway_motorway"));
  instructionProcessor->AddMotorwayLinkType(typeConfig->GetTypeInfo("highway_motorway_link"));
  instructionProcessor->AddMotorwayType(typeConfig->GetTypeInfo("highway_motorway_trunk"));
  instructionProcessor->AddMotorwayType(typeConfig->GetTypeInfo("highway_motorway_primary"));
  instructionProcessor->AddMotorwayType(typeConfig->GetTypeInfo("highway_trunk"));
  instructionProcessor->AddMotorwayLinkType(typeConfig->GetTypeInfo("highway_trunk_link"));
  postprocessors.push_back(instructionProcessor);

  if (!postprocessor.PostprocessRouteDescription(description,
                                                 routingProfile,
                                                 *database,
                                                 postprocessors)) {
    return false;
  }

  return true;
}

bool DatabaseTask::TransformRouteDataToWay(const osmscout::RouteData& data,
                                           osmscout::Way& way)
{
  Lum::OS::Guard<Lum::OS::Mutex> guard(mutex);

  if (!router.Valid() ||
      !router->IsOpen()) {
    return false;
  }

  return router->TransformRouteDataToWay(data,way);
}

void DatabaseTask::ClearRoute()
{
  Lum::OS::Guard<Lum::OS::Mutex> guard(mutex);

  data.poiWays.clear();

  SignalRedraw();
}

void DatabaseTask::AddRoute(const osmscout::Way& way)
{
  Lum::OS::Guard<Lum::OS::Mutex> guard(mutex);

  data.poiWays.push_back(new osmscout::Way(way));

  SignalRedraw();
}

void DatabaseTask::PostJob(Job *job)
{
  Lum::OS::Guard<Lum::OS::Mutex> guard(mutex);

  delete newJob;
  newJob=job;

  condition.Signal();
}

void DatabaseTask::SignalRedraw()
{
  delete newJob;
  newJob=new Job();

  newJob->lon=currentLon;
  newJob->lat=currentLat;
  newJob->magnification=currentMagnification;
  newJob->width=currentWidth;
  newJob->height=currentHeight;

  condition.Signal();
}

bool DatabaseTask::DrawResult(Lum::OS::Window* window,
                              Lum::OS::DrawInfo* draw,
                              int x, int y,
                              size_t width, size_t height,
                              double lon, double lat,
                              const osmscout::Magnification& magnification,
                              osmscout::Projection& projection)
{
  Lum::OS::Guard<Lum::OS::Mutex> guard(mutex);

  if (finishedCairo==NULL) {
    std::cout << "No map available!" << std::endl;
    return false;
  }

  projection.Set(finishedLon,finishedLat,
                 finishedMagnification,
                 finishedWidth,finishedHeight);

  double lonMin,lonMax,latMin,latMax;

  projection.GetDimensions(lonMin,latMin,lonMax,latMax);

  double dx=0;
  double dy=0;
  if (lon!=finishedLon || lat!=finishedLat) {
    dx+=(lon-finishedLon)*width/(lonMax-lonMin);
    dy+=(lat-finishedLat)*height/(latMax-latMin);
  }

  draw->PushClip(x,y,width,height);


#if defined(LUM_HAVE_LIB_CAIRO)
  if (dynamic_cast<Lum::OS::Cairo::DrawInfo*>(draw)!=NULL) {
    cairo_t* cairo=dynamic_cast<Lum::OS::Cairo::DrawInfo*>(draw)->cairo;

    cairo_set_source_surface(cairo,finishedSurface,x-dx,y+dy);
    cairo_rectangle(cairo,x,y,finishedWidth,finishedHeight);
    cairo_fill(cairo);
  }
#endif

#if defined(LUM_HAVE_LIB_WIN32)
  if (dynamic_cast<Lum::OS::Win32::DrawInfo*>(draw)!=NULL) {
    Lum::OS::Win32::DrawInfo *win32Draw=dynamic_cast<Lum::OS::Win32::DrawInfo*>(draw);

    cairo_surface_t *surface=cairo_win32_surface_create(win32Draw->dc);

    cairo_t* cairo=cairo_create(surface);

    cairo_set_source_surface(cairo,finishedSurface,x-dx,y+dy);
    cairo_rectangle(cairo,x,y,finishedWidth,finishedHeight);
    cairo_fill(cairo);

    cairo_destroy(cairo),
    cairo_surface_destroy(surface);
  }
#endif

  draw->PopClip();

  return finishedWidth==width &&
         finishedHeight==height &&
         finishedLon==lon &&
         finishedLat==lat &&
         finishedMagnification==magnification;
}
