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

#include "MapWidget.h"

#include <iostream>

MapWidget::MapWidget(QWidget *parent) :
  QWidget(parent),
  requestNewMap(true)
{
  setFocusPolicy(Qt::StrongFocus);

  connect(&dbThread,SIGNAL(InitialisationFinished(DatabaseLoadedResponse)),this,SLOT(InitialisationFinished(DatabaseLoadedResponse)));
  connect(this,SIGNAL(TriggerMapRenderingSignal()),&dbThread,SLOT(TriggerMapRendering()));
  connect(&dbThread,SIGNAL(HandleMapRenderingResult()),this,SLOT(DrawRenderResult()));
  connect(&dbThread,SIGNAL(Redraw()),this,SLOT(Redraw()));

  lon=0;
  lat=0;
  magnification=64;
}

MapWidget::~MapWidget()
{
  // no code
}

void MapWidget::Redraw()
{
  update();
}

void MapWidget::InitialisationFinished(const DatabaseLoadedResponse& response)
{
  size_t zoom=1;
  double dlat=360;
  double dlon=180;

  std::cout << "Initial bounding box [";
  std::cout << response.minLat <<"," << response.minLon << " - " << response.maxLat << "," << response.maxLon << "]" << std::endl;

  lat=response.minLat+(response.maxLat-response.minLat)/2;
  lon=response.minLon+(response.maxLon-response.minLon)/2;

  while (dlat>response.maxLat-response.minLat &&
         dlon>response.maxLon-response.minLon) {
    zoom=zoom*2;
    dlat=dlat/2;
    dlon=dlon/2;
  }

  magnification=zoom;

  std::cout << "Magnification: " << magnification.GetMagnification() << "x" << std::endl;

  TriggerMapRendering();
}

void MapWidget::DrawRenderResult()
{
  update();
}

void MapWidget::TriggerMapRendering()
{
  RenderMapRequest request;

  request.lat=lat;
  request.lon=lon;
  request.magnification=magnification;
  request.width=width();
  request.height=height();

  dbThread.UpdateRenderRequest(request);
  emit TriggerMapRenderingSignal();
}

void MapWidget::keyPressEvent(QKeyEvent* event)
{
  if (event->key()==Qt::Key_Plus) {
    ZoomIn(2.0);
    event->accept();
  }
  else if (event->key()==Qt::Key_Minus) {
    ZoomOut(2.0);
    event->accept();
  }
  if (event->key()==Qt::Key_Left) {
    osmscout::MercatorProjection projection;
    double                       lonMin,latMin,lonMax,latMax;

    projection.Set(lon,lat,
                   magnification,
                   width(),height());

    projection.GetDimensions(lonMin,latMin,lonMax,latMax);

    lon-=(lonMax-lonMin)*0.3;

    TriggerMapRendering();
    event->accept();
  }
  else if (event->key()==Qt::Key_Right) {
    osmscout::MercatorProjection projection;
    double                       lonMin,latMin,lonMax,latMax;

    projection.Set(lon,lat,
                   magnification,
                   width(),height());

    projection.GetDimensions(lonMin,latMin,lonMax,latMax);

    lon+=(lonMax-lonMin)*0.3;

    TriggerMapRendering();
    event->accept();
  }
  else if (event->key()==Qt::Key_Up) {
    osmscout::MercatorProjection projection;
    double                       lonMin,latMin,lonMax,latMax;

    projection.Set(lon,lat,
                   magnification,
                   width(),height());

    projection.GetDimensions(lonMin,latMin,lonMax,latMax);

    lat+=(latMax-latMin)*0.3;

    TriggerMapRendering();
    event->accept();
  }
  else if (event->key()==Qt::Key_Down) {
    osmscout::MercatorProjection projection;
    double                       lonMin,latMin,lonMax,latMax;

    projection.Set(lon,lat,
                   magnification,
                   width(),height());

    projection.GetDimensions(lonMin,latMin,lonMax,latMax);

    lat-=(latMax-latMin)*0.3;

    TriggerMapRendering();
    event->accept();
  }
}

void MapWidget::HandleMouseMove(QMouseEvent* event)
{
  double                       olon, olat;
  double                       tlon, tlat;
  osmscout::MercatorProjection projection;

  projection.Set(lon,lat,
                 magnification,
                 width(),height());

  // Get origin coordinates
  projection.PixelToGeo(0,0,
                        olon,olat);

  // Get current mouse pos coordinates (relative to drag start)
  projection.PixelToGeo(event->x()-startX,
                        event->y()-startY,
                        tlon,tlat);

  lon=startLon-(tlon-olon);
  lat=startLat-(tlat-olat);
}

void MapWidget::mousePressEvent(QMouseEvent* event)
{
  if (event->button()==1) {
    startLon=lon;
    startLat=lat;
    startX=event->x();
    startY=event->y();
  }
}

void MapWidget::mouseMoveEvent(QMouseEvent* event)
{
  HandleMouseMove(event);
  requestNewMap=false;
  update();
}

void MapWidget::mouseReleaseEvent(QMouseEvent* event)
{
  if (event->button()==1) {
    HandleMouseMove(event);
    requestNewMap=true;
    update();
  }
}

void MapWidget::wheelEvent(QWheelEvent* event)
{
  int numDegrees=event->delta()/8;
  int numSteps=numDegrees/15;

    if (numSteps>=0) {
    ZoomIn(numSteps*1.35);
  }
  else {
    ZoomOut(-numSteps*1.35);
  }

  event->accept();
}

void MapWidget::paintEvent(QPaintEvent* event)
{
  RenderMapRequest request;

  request.lat=lat;
  request.lon=lon;
  request.magnification=magnification;
  request.width=width();
  request.height=height();

  QPainter painter(this);

  if (!dbThread.RenderMap(painter,request) &&
      requestNewMap) {
    TriggerMapRendering();
  }

  requestNewMap=true;
}

void MapWidget::resizeEvent(QResizeEvent* event)
{
  // no code
}

void MapWidget::ZoomIn(double zoomFactor)
{
  if (magnification.GetMagnification()*zoomFactor>200000) {
    magnification.SetMagnification(200000);
  }
  else {
    magnification.SetMagnification(magnification.GetMagnification()*zoomFactor);
  }

  TriggerMapRendering();
}

void MapWidget::ZoomOut(double zoomFactor)
{
  if (magnification.GetMagnification()/zoomFactor<1) {
    magnification.SetMagnification(1);
  }
  else {
    magnification.SetMagnification(magnification.GetMagnification()/zoomFactor);
  }

  TriggerMapRendering();
}

void MapWidget::ShowReference(const osmscout::ObjectFileRef& reference,
                              const osmscout::Magnification& magnification)
{
  if (reference.GetType()==osmscout::refNode) {
    osmscout::NodeRef node;

    if (dbThread.GetNodeByOffset(reference.GetFileOffset(),node)) {
      lon=node->GetLon();
      lat=node->GetLat();
      this->magnification=magnification;

      TriggerMapRendering();
    }
  }
  else if (reference.GetType()==osmscout::refArea) {
    osmscout::AreaRef area;

    if (dbThread.GetAreaByOffset(reference.GetFileOffset(),area)) {
      if (area->GetCenter(lat,lon)) {
        this->magnification=magnification;

        TriggerMapRendering();
      }
    }
  }
  else if (reference.GetType()==osmscout::refWay) {
    osmscout::WayRef way;

    if (dbThread.GetWayByOffset(reference.GetFileOffset(),way)) {
      if (way->GetCenter(lat,lon)) {
        this->magnification=magnification;

        TriggerMapRendering();
      }
    }
  }
  else {
    assert(false);
  }
}

#include "moc_MapWidget.cpp"
