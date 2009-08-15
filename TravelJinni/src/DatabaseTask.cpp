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

#include <cairo/cairo.h>

#include <osmscout/StyleConfigLoader.h>

#include <Lum/Features.h>

#include <Lum/Base/String.h>

#if defined(HAVE_LIB_CAIRO)
#include <Lum/OS/Cairo/Bitmap.h>
#include <Lum/OS/Cairo/Display.h>
#include <Lum/OS/Cairo/DrawInfo.h>
#endif

#include <Lum/OS/X11/DrawInfo.h>
#include <Lum/OS/X11/Display.h>

#include <Lum/OS/Bitmap.h>
#include <Lum/OS/Display.h>
#include <Lum/OS/Driver.h>
#include <Lum/OS/Thread.h>

// Cairo includes X11, defines Status
#if defined(Status)
  #undef Status
#endif
#include <Lum/Base/Path.h>

#include "MapPainter.h"

DatabaseTask::DatabaseTask(Database* database,
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
   finishedCairo(NULL),
   painter(*database)
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

        currentSurface=cairo_image_surface_create(CAIRO_FORMAT_RGB24,
                                                  currentJob->width,currentJob->height);
        currentCairo=cairo_create(currentSurface);

        currentWidth=currentJob->width;
        currentHeight=currentJob->height;
      }

      currentLon=currentJob->lon;
      currentLat=currentJob->lat;
      currentMagnification=currentJob->magnification;

      if (database->IsOpen() && styleConfig!=NULL) {
        painter.DrawMap(*styleConfig,
                      currentLon,currentLat,currentMagnification,
                      currentWidth,currentHeight,
                      currentSurface,currentCairo);
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
      std::swap(finishedWidth,currentWidth);
      std::swap(finishedHeight,currentHeight);

      finishedLon=currentLon;
      finishedLat=currentLat;
      finishedMagnification=currentMagnification;
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

bool DatabaseTask::LoadStyleConfig(const std::wstring& filename,
                                   StyleConfig*& styleConfig)
{
  Lum::OS::Guard<Lum::OS::Mutex> guard(mutex);

  Lum::Base::Path f;

  f.SetNativePath(filename);

  if (database->GetTypeConfig()!=NULL) {
    styleConfig=new StyleConfig(database->GetTypeConfig());

    if (::LoadStyleConfig(Lum::Base::WStringToString(f.GetPath()).c_str(),
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

void DatabaseTask::SetStyle(StyleConfig* styleConfig)
{
  Lum::OS::Guard<Lum::OS::Mutex> guard(mutex);

  this->styleConfig=styleConfig;
}

bool DatabaseTask::GetWay(Id id, Way& way) const
{
  Lum::OS::Guard<Lum::OS::Mutex> guard(mutex);

  if (!database->IsOpen()) {
    return false;
  }

  return database->GetWay(id,way);
}

bool DatabaseTask::GetMatchingCities(const std::wstring& name,
                                     std::list<City>& cities,
                                     size_t limit,
                                     bool& limitReached) const
{
  Lum::OS::Guard<Lum::OS::Mutex> guard(mutex);

  if (!database->IsOpen()) {
    return false;
  }

  return database->GetMatchingCities(Lum::Base::WStringToUTF8(name),
                                     cities,
                                     limit,limitReached);
}

bool DatabaseTask::GetMatchingStreets(Id urbanId, const std::wstring& name,
                                      std::list<Street>& streets,
                                      size_t limit, bool& limitReached) const
{
  Lum::OS::Guard<Lum::OS::Mutex> guard(mutex);

  if (!database->IsOpen()) {
    return false;
  }

  return database->GetMatchingStreets(urbanId,
                                      Lum::Base::WStringToUTF8(name),
                                      streets,
                                      limit,limitReached);
}

bool DatabaseTask::CalculateRoute(Id startWayId, Id startNodeId,
                                  Id targetWayId, Id targetNodeId,
                                  RouteData& route)
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

bool DatabaseTask::TransformRouteDataToRouteDescription(const RouteData& data,
                                                        RouteDescription& description)
{
  Lum::OS::Guard<Lum::OS::Mutex> guard(mutex);

  if (!database->IsOpen()) {
    return false;
  }

  return database->TransformRouteDataToRouteDescription(data,description);
}

bool DatabaseTask::TransformRouteDataToWay(const RouteData& data,
                                           Way& way)
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

  painter.poiWays.clear();

  SignalRedraw();
}

void DatabaseTask::AddRoute(const Way& way)
{
  Lum::OS::Guard<Lum::OS::Mutex> guard(mutex);

  painter.poiWays.push_back(way);

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

    double dx,dy;

    double lonMin,lonMax,latMin,latMax;

    painter.GetDimensions(finishedLon,finishedLat,
                          finishedMagnification,
                          finishedWidth,finishedHeight,
                          lonMin,latMin,lonMax,latMax);

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

    dx=0;
    dy=0;
    if (lon!=finishedLon || lat!=finishedLat) {
      dx+=(lon-finishedLon)*width/(lonMax-lonMin);
      dy+=(lat-finishedLat)*height/(latMax-latMin);
    }

    draw->PushClip(x,y,width,height);

#if defined(LUM_HAVE_LIB_CAIRO)
    if (dynamic_cast<Lum::OS::Cairo::DrawInfo*>(draw)!=NULL) {
      cairo_t* cairo=dynamic_cast<Lum::OS::Cairo::DrawInfo*>(draw)->cairo;

      cairo_save(cairo);
      cairo_set_source_surface(cairo,finishedSurface,x-dx,y+dy);
      cairo_rectangle(cairo,x,y,finishedWidth,finishedHeight);
      cairo_fill(cairo);
      cairo_restore(cairo);

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
#if defined(LUM_HAVE_LIB_X)
    if (dynamic_cast<Lum::OS::X11::DrawInfo*>(draw)!=NULL) {
      Lum::OS::X11::DrawInfo *x11Draw=dynamic_cast<Lum::OS::X11::DrawInfo*>(draw);

      cairo_surface_t *surface=cairo_xlib_surface_create(x11Draw->display,
                                                         x11Draw->drawable,
                                                         dynamic_cast<Lum::OS::X11::Display*>(Lum::OS::display)->visual,
                                                         window->GetWidth(),window->GetHeight());

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


