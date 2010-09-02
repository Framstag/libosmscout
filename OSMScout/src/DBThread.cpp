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

#include <QMutexLocker>

#include <osmscout/StyleConfigLoader.h>

DBThread dbThread;

DBThread::DBThread()
 : currentPixmap(NULL),
   finishedPixmap(NULL)
{
  // no code
}

void DBThread::run()
{
  if (database.Open("../TravelJinni")) {
    if (database.GetTypeConfig()!=NULL) {
      styleConfig=new osmscout::StyleConfig(database.GetTypeConfig());

      if (!osmscout::LoadStyleConfig("../TravelJinni/standard.oss.xml",
                                     *styleConfig)) {
        delete styleConfig;
        styleConfig=NULL;
      }
    }
    else {
      styleConfig=NULL;
    }
  }
  else {
    std::cerr<<"Cannot open database!"<<std::endl;
  }

  emit InitialisationFinished();

  exec();

  if (database.IsOpen()) {
    database.Close();
  }
}

void DBThread::TriggerMapRendering(const RenderMapRequest& request)
{
  if (currentPixmap==NULL ||
      currentPixmap->width()!=(int)request.width ||
      currentPixmap->height()!=(int)request.height) {
    delete currentPixmap;

    //currentPixmap=new QGLPixelBuffer(QSize(request.width,request.height));
    currentPixmap=new QPixmap(QSize(request.width,request.height));
  }

  currentLon=request.lon;
  currentLat=request.lat;
  currentMagnification=request.magnification;

  if (database.IsOpen() &&
      styleConfig!=NULL) {
    osmscout::MercatorProjection projection;
    osmscout::MapParameter       parameter;

    std::cout << std::endl;

    osmscout::StopClock overallTimer;

    projection.Set(currentLon,
                   currentLat,
                   currentMagnification,
                   request.width,
                   request.height);

    osmscout::StopClock dataRetrievalTimer;

    database.GetObjects(*styleConfig,
                        projection.GetLonMin(),
                        projection.GetLatMin(),
                        projection.GetLonMax(),
                        projection.GetLatMax(),
                        projection.GetMagnification(),
                        ((size_t)ceil(osmscout::Log2(projection.GetMagnification())))+6,
                        2000,
                        2000,
                        std::numeric_limits<size_t>::max(),
                        data.nodes,
                        data.ways,
                        data.areas,
                        data.relationWays,
                        data.relationAreas);

    database.GetGroundTiles(projection.GetLonMin(),
                            projection.GetLatMin(),
                            projection.GetLonMax(),
                            projection.GetLatMax(),
                            data.groundTiles);

    dataRetrievalTimer.Stop();

    osmscout::StopClock drawTimer;

    QPainter *p=new QPainter(currentPixmap);

    painter.DrawMap(*styleConfig,
                    projection,
                    parameter,
                    data,
                    p);

    delete p;

    drawTimer.Stop();
    overallTimer.Stop();

    std::cout << "All: " << overallTimer << " Data: " << dataRetrievalTimer << " Draw: " << drawTimer << std::endl;
  }
  else {
    std::cout << "Cannot draw map: " << database.IsOpen() << " " << (styleConfig!=NULL) << std::endl;

    QPainter *p=new QPainter(currentPixmap);

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

  std::swap(currentPixmap,finishedPixmap);
  std::swap(currentLon,finishedLon);
  std::swap(currentLat,finishedLat);
  std::swap(currentMagnification,finishedMagnification);

  emit HandleMapRenderingResult();
}

bool DBThread::RenderMap(QPainter& painter,
                         const RenderMapRequest& request)
{
  QMutexLocker locker(&mutex);

  if (finishedPixmap==NULL) {
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

  projection.Set(finishedLon,finishedLat,
                 finishedMagnification,
                 finishedPixmap->width(),finishedPixmap->height());

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

  painter.drawPixmap(dx,dy,*finishedPixmap);

/*
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
*/

  return finishedPixmap->width()==(int)request.width &&
         finishedPixmap->height()==(int)request.height &&
         finishedLon==request.lon &&
         finishedLat==request.lat &&
         finishedMagnification==request.magnification;
}

#include "moc_DBThread.cpp"
