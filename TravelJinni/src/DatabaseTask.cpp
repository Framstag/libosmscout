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
#include <iostream>

#if defined(__WIN32__) || defined(WIN32) || defined(__APPLE__)
  #include <cairo.h>
#else
  #include <cairo/cairo.h>
#endif

#include <osmscout/StyleConfigLoader.h>

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

DatabaseTask::DatabaseTask(osmscout::Database* database,
                           Lum::Model::Action* jobFinishedAction)
 : database(database),
   styleConfig(NULL),
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
          styleConfig!=NULL) {
        osmscout::MercatorProjection  projection;
        osmscout::MapParameter        drawParameter;
        osmscout::AreaSearchParameter searchParameter;

        std::list<std::string>        paths;

        paths.push_back("../libosmscout/data/icons/14x14/standard/");

        drawParameter.SetIconPaths(paths);
        drawParameter.SetPatternPaths(paths);

        drawParameter.SetOptimizeWayNodes(true);
        drawParameter.SetOptimizeAreaNodes(true);
        drawParameter.SetDebugPerformance(true);

        std::cout << std::endl;

        osmscout::StopClock overallTimer;

        projection.Set(currentLon,
                       currentLat,
                       currentMagnification,
                       currentWidth,
                       currentHeight);

        osmscout::StopClock dataRetrievalTimer;

        database->GetObjects(*styleConfig,
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
        database->GetGroundTiles(projection.GetLonMin(),
                                 projection.GetLatMin(),
                                 projection.GetLonMax(),
                                 projection.GetLatMax(),
                                 data.groundTiles);*/

        dataRetrievalTimer.Stop();

        osmscout::StopClock drawTimer;

        painter.DrawMap(*styleConfig,
                        projection,
                        drawParameter,
                        data,
                        currentCairo);

        drawTimer.Stop();
        overallTimer.Stop();

        std::cout << "All: " << overallTimer << " Data: " << dataRetrievalTimer << " Draw: " << drawTimer << std::endl;
      }
      else {
        std::cout << "Cannot draw map: " << database->IsOpen() << " " << (styleConfig!=NULL) << std::endl;
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

  Lum::Base::Path p;

  p.SetNativeDir(path);

  return database->Open(Lum::Base::WStringToString(p.GetPath()).c_str());
}

bool DatabaseTask::IsOpen() const
{
  Lum::OS::Guard<Lum::OS::Mutex> guard(mutex);

  return database->IsOpen();
}

void DatabaseTask::Close()
{
  Lum::OS::Guard<Lum::OS::Mutex> guard(mutex);

  database->Close();
}


void DatabaseTask::FlushCache()
{
  Lum::OS::Guard<Lum::OS::Mutex> guard(mutex);

  database->FlushCache();
}

bool DatabaseTask::LoadStyleConfig(const std::wstring& filename,
                                   osmscout::StyleConfig*& styleConfig)
{
  Lum::OS::Guard<Lum::OS::Mutex> guard(mutex);

  Lum::Base::Path f;

  f.SetNativePath(filename);

  if (database->GetTypeConfig()!=NULL) {
    styleConfig=new osmscout::StyleConfig(database->GetTypeConfig());

    if (osmscout::LoadStyleConfig(Lum::Base::WStringToString(f.GetPath()).c_str(),
                                  *styleConfig)) {
      return true;
    }
    else {
      delete styleConfig;
      styleConfig=NULL;

      return false;
    }
  }
  else {
    styleConfig=NULL;
    return false;
  }
}

void DatabaseTask::SetStyle(osmscout::StyleConfig* styleConfig)
{
  Lum::OS::Guard<Lum::OS::Mutex> guard(mutex);

  this->styleConfig=styleConfig;
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

bool DatabaseTask::GetWay(osmscout::Id id,
                          osmscout::WayRef& way) const
{
  Lum::OS::Guard<Lum::OS::Mutex> guard(mutex);

  if (!database->IsOpen()) {
    return false;
  }

  return database->GetWay(id,way);
}

bool DatabaseTask::GetMatchingAdminRegions(const std::wstring& name,
                                           std::list<osmscout::AdminRegion>& regions,
                                           size_t limit,
                                           bool& limitReached) const
{
  Lum::OS::Guard<Lum::OS::Mutex> guard(mutex);

  if (!database->IsOpen()) {
    return false;
  }

  return database->GetMatchingAdminRegions(Lum::Base::WStringToUTF8(name),
                                           regions,
                                           limit,limitReached, false);
}

bool DatabaseTask::GetMatchingLocations(const osmscout::AdminRegion& region,
                                        const std::wstring& name,
                                        std::list<osmscout::Location>& locations,
                                        size_t limit,
                                        bool& limitReached) const
{
  Lum::OS::Guard<Lum::OS::Mutex> guard(mutex);

  if (!database->IsOpen()) {
    return false;
  }

  return database->GetMatchingLocations(region,
                                        Lum::Base::WStringToUTF8(name),
                                        locations,
                                        limit,
                                        limitReached,
                                        false);
}

bool DatabaseTask::CalculateRoute(osmscout::Id startWayId, osmscout::Id startNodeId,
                                  osmscout::Id targetWayId, osmscout::Id targetNodeId,
                                  osmscout::RouteData& route)
{
  Lum::OS::Guard<Lum::OS::Mutex> guard(mutex);

  if (!database->IsOpen()) {
    return false;
  }

  return database->CalculateRoute(startWayId,
                                  startNodeId,
                                  targetWayId,
                                  targetNodeId,
                                  route);
}

bool DatabaseTask::TransformRouteDataToRouteDescription(const osmscout::RouteData& data,
                                                        osmscout::RouteDescription& description)
{
  Lum::OS::Guard<Lum::OS::Mutex> guard(mutex);

  if (!database->IsOpen()) {
    return false;
  }

  return database->TransformRouteDataToRouteDescription(data,description);
}

bool DatabaseTask::TransformRouteDataToWay(const osmscout::RouteData& data,
                                           osmscout::Way& way)
{
  Lum::OS::Guard<Lum::OS::Mutex> guard(mutex);

  if (!database->IsOpen()) {
    return false;
  }

  return database->TransformRouteDataToWay(data,way);
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

  data.poiWays.push_back(osmscout::WayRef(way));

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
                              double magnification)
{
  Lum::OS::Guard<Lum::OS::Mutex> guard(mutex);

  if (finishedCairo==NULL) {
    std::cout << "No map available!" << std::endl;
    return false;
  }

  osmscout::MercatorProjection projection;

  projection.Set(finishedLon,finishedLat,
                 finishedMagnification,
                 finishedWidth,finishedHeight);

  double lonMin,lonMax,latMin,latMax;

  projection.GetDimensions(lonMin,latMin,lonMax,latMax);

  double d=(lonMax-lonMin)*2*M_PI/360;
  double scaleSize;
  size_t minScaleWidth=width/20;
  size_t maxScaleWidth=width/10;
  double scaleValue=d*180*60/M_PI*1852.216/(width/minScaleWidth);

  //std::cout << "1/10 screen (" << width/10 << " pixels) are: " << scaleValue << " meters" << std::endl;

  scaleValue=pow(10,floor(log10(scaleValue))+1);
  scaleSize=scaleValue/(d*180*60/M_PI*1852.216/width);

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

    // Scale

    cairo_save(cairo);

    cairo_set_source_rgb(cairo,0,0,0);
    cairo_set_line_width(cairo,2);
    cairo_move_to(cairo,x+width/20,y+height*19/20);
    cairo_line_to(cairo,x+width/20+scaleSize-1,y+height*19/20);
    cairo_stroke(cairo);
    cairo_move_to(cairo,x+width/20,y+height*19/20);
    cairo_line_to(cairo,x+width/20,y+height*19/20-height/40);
    cairo_stroke(cairo);
    cairo_move_to(cairo,x+width/20+scaleSize-1,y+height*19/20);
    cairo_line_to(cairo,x+width/20+scaleSize-1,y+height*19/20-height/40);
    cairo_stroke(cairo);

    cairo_move_to(cairo,x+width/20+scaleSize-1+10,y+height*19/20);
    cairo_show_text(cairo,Lum::Base::NumberToString((size_t)scaleValue).c_str());
    cairo_stroke(cairo);

    cairo_restore(cairo);
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
