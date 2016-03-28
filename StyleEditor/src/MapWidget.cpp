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

#define TMP_SUFFIX ".tmp"

MapWidget::MapWidget(QQuickItem* parent)
    : QQuickPaintedItem(parent),
      center(0.0,0.0),
      angle(0.0),
      magnification(64),
      requestNewMap(true)
{
    setOpaquePainting(true);
    setAcceptedMouseButtons(Qt::LeftButton);

    DBThread *dbThread=DBThread::GetInstance();
    //setFocusPolicy(Qt::StrongFocus);

    connect(dbThread,SIGNAL(InitialisationFinished(DatabaseLoadedResponse)),
            this,SLOT(initialisationFinished(DatabaseLoadedResponse)));

    connect(this,SIGNAL(TriggerMapRenderingSignal()),
            dbThread,SLOT(TriggerMapRendering()));

    connect(dbThread,SIGNAL(HandleMapRenderingResult()),
            this,SLOT(redraw()));

    connect(dbThread,SIGNAL(Redraw()),
            this,SLOT(redraw()));

    connect(dbThread,SIGNAL(stylesheetFilenameChanged()),
            this,SIGNAL(stylesheetFilenameChanged()));
}

MapWidget::~MapWidget()
{
    // no code
}

QString MapWidget::stylesheetFilename() {
    DBThread *dbThread=DBThread::GetInstance();
    return dbThread->GetStylesheetFilename();
}

void MapWidget::redraw()
{
    if(!hasErrors){
        update();
    }
}

void MapWidget::initialisationFinished(const DatabaseLoadedResponse& response)
{
    size_t zoom=1;
    double dlat=360;
    double dlon=180;

    std::cout << "Initial bounding box " << response.boundingBox.GetDisplayText() << std::endl;

    center=response.boundingBox.GetCenter();

    while (dlat>response.boundingBox.GetHeight() &&
           dlon>response.boundingBox.GetWidth()) {
        zoom=zoom*2;
        dlat=dlat/2;
        dlon=dlon/2;
    }

    magnification=zoom;

    std::cout << "Magnification: " << magnification.GetMagnification() << "x" << std::endl;

    TriggerMapRendering();
}

void MapWidget::TriggerMapRendering()
{
    DBThread         *dbThread=DBThread::GetInstance();
    RenderMapRequest request;

    request.lat=center.GetLat();
    request.lon=center.GetLon();
    request.angle=angle;
    request.magnification=magnification;
    request.width=width();
    request.height=height();

    dbThread->UpdateRenderRequest(request);

    emit TriggerMapRenderingSignal();
}


void MapWidget::HandleMouseMove(QMouseEvent* event)
{
    osmscout::MercatorProjection projection=startProjection;

    if (!projection.Move(startX-event->x(),
                         event->y()-startY)) {
        return;
    }

    center=projection.GetCenter();
}

void MapWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button()==1) {
        DBThread *dbThread=DBThread::GetInstance();

        dbThread->GetProjection(startProjection);

        startX=event->x();
        startY=event->y();

        setFocus(true);
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

    QPoint numPixels = event->pixelDelta();
    QPoint numDegrees = event->angleDelta() / 8;
    int steps = 0;

   if (!numPixels.isNull()) {
        steps = numPixels.y()>0 ? numPixels.manhattanLength() : -numPixels.manhattanLength();

    } else if (!numDegrees.isNull()) {
        QPoint numSteps = numDegrees / 15;
        steps = numSteps.y()>0 ? numSteps.manhattanLength() : -numSteps.manhattanLength();
    }

    if(steps==0){
        return;
    }
    if (steps>=0) {
        zoomIn(std::max(1.01,0.2*steps));
    }
    else {
        zoomOut(std::max(1.01,0.2*steps));
    }

    event->accept();
}

void MapWidget::paint(QPainter *painter)
{
    RenderMapRequest request;
    DBThread         *dbThread=DBThread::GetInstance();
    QRectF           boundingBox=contentsBoundingRect();

    request.lat=center.GetLat();
    request.lon=center.GetLon();
    request.magnification=magnification;
    request.width=boundingBox.width();
    request.height=boundingBox.height();

    if (!dbThread->RenderMap(*painter,request) &&
            requestNewMap) {
        TriggerMapRendering();
    }

    requestNewMap=true;
}

void MapWidget::zoomIn(double zoomFactor)
{
    if (magnification.GetMagnification()*zoomFactor>800000) {
        magnification.SetMagnification(800000);
    }
    else {
        magnification.SetMagnification(magnification.GetMagnification()*zoomFactor);
    }

    TriggerMapRendering();

    emit zoomLevelChanged();
    emit zoomLevelNameChanged();
}

void MapWidget::zoomOut(double zoomFactor)
{
    if (magnification.GetMagnification()/zoomFactor<1) {
        magnification.SetMagnification(1);
    }
    else {
        magnification.SetMagnification(magnification.GetMagnification()/zoomFactor);
    }

    TriggerMapRendering();
    emit zoomLevelChanged();
    emit zoomLevelNameChanged();
}

QString MapWidget::zoomLevelName() {
    double level = magnification.GetMagnification();
    if(level>=osmscout::Magnification::magWorld && level < osmscout::Magnification::magContinent){
        return "World";
    } else if(level>=osmscout::Magnification::magContinent && level < osmscout::Magnification::magState){
        return "Continent";
    } else if(level>=osmscout::Magnification::magState && level < osmscout::Magnification::magStateOver){
        return "State";
    } else if(level>=osmscout::Magnification::magStateOver && level < osmscout::Magnification::magCounty){
        return "StateOver";
    } else if(level>=osmscout::Magnification::magCounty && level < osmscout::Magnification::magRegion){
        return "County";
    } else if(level>=osmscout::Magnification::magRegion && level < osmscout::Magnification::magProximity){
        return "Region";
    } else if(level>=osmscout::Magnification::magProximity && level < osmscout::Magnification::magCityOver){
        return "Proximity";
    } else if(level>=osmscout::Magnification::magCityOver && level < osmscout::Magnification::magCity){
        return "CityOver";
    } else if(level>=osmscout::Magnification::magCity && level < osmscout::Magnification::magSuburb){
        return "City";
    } else if(level>=osmscout::Magnification::magSuburb && level < osmscout::Magnification::magDetail){
        return "Suburb";
    } else if(level>=osmscout::Magnification::magDetail && level < osmscout::Magnification::magClose){
        return "Detail";
    } else if(level>=osmscout::Magnification::magClose && level < osmscout::Magnification::magVeryClose){
        return "Close";
    } else if(level>=osmscout::Magnification::magVeryClose && level < osmscout::Magnification::magBlock){
        return "VeryClose";
    } else if(level>=osmscout::Magnification::magBlock && level < osmscout::Magnification::magStreet){
        return "Block";
    } else if(level>=osmscout::Magnification::magStreet && level < osmscout::Magnification::magHouse){
        return "Street";
    } else if(level>=osmscout::Magnification::magHouse){
        return "House";
    }

    assert(false);

    return "";
}

void MapWidget::left()
{
    osmscout::MercatorProjection projection;
    osmscout::GeoBox             boundingBox;

    projection.Set(center.GetLon(),
                   center.GetLat(),
                   magnification,
                   width(),height());

    projection.GetDimensions(boundingBox);

    center.Set(center.GetLat(),
               center.GetLon()-boundingBox.GetWidth()*0.3);

    TriggerMapRendering();
}

void MapWidget::right()
{
    osmscout::MercatorProjection projection;
    osmscout::GeoBox             boundingBox;

    projection.Set(center.GetLon(),
                   center.GetLat(),
                   magnification,
                   width(),height());

    projection.GetDimensions(boundingBox);

    center.Set(center.GetLat(),
               center.GetLon()+boundingBox.GetWidth()*0.3);

    TriggerMapRendering();
}

void MapWidget::up()
{
    osmscout::MercatorProjection projection;
    osmscout::GeoBox             boundingBox;

    projection.Set(center.GetLon(),
                   center.GetLat(),
                   magnification,
                   width(),height());

    projection.GetDimensions(boundingBox);

    center.Set(center.GetLat()+boundingBox.GetHeight()*0.3,
               center.GetLon());

    TriggerMapRendering();
}

void MapWidget::down()
{
    osmscout::MercatorProjection projection;
    osmscout::GeoBox             boundingBox;

    projection.Set(center.GetLon(),
                   center.GetLat(),
                   magnification,
                   width(),height());

    projection.GetDimensions(boundingBox);

    center.Set(center.GetLat()-boundingBox.GetHeight()*0.3,
               center.GetLon());

    TriggerMapRendering();
}

void MapWidget::showCoordinates(double lat, double lon)
{
    center.Set(lat,lon);
    this->magnification=osmscout::Magnification::magVeryClose;

    TriggerMapRendering();
}

void MapWidget::showLocation(Location* location)
{
    if (location==NULL) {
        std::cout << "MapWidget::showLocation(): no location passed!" << std::endl;

        return;
    }

    std::cout << "MapWidget::showLocation(\"" << location->getName().toLocal8Bit().constData() << "\")" << std::endl;

    osmscout::ObjectFileRef reference=location->getReferences().front();

    DBThread* dbThread=DBThread::GetInstance();

    if (reference.GetType()==osmscout::refNode) {
        osmscout::NodeRef node;

        if (dbThread->GetNodeByOffset(reference.GetFileOffset(),node)) {
            center=node->GetCoords();
            this->magnification=osmscout::Magnification::magVeryClose;

            TriggerMapRendering();
        }
    }
    else if (reference.GetType()==osmscout::refArea) {
        osmscout::AreaRef area;

        if (dbThread->GetAreaByOffset(reference.GetFileOffset(),area)) {
            if (area->GetCenter(center)) {
                this->magnification=osmscout::Magnification::magVeryClose;

                TriggerMapRendering();
            }
        }
    }
    else if (reference.GetType()==osmscout::refWay) {
        osmscout::WayRef way;

        if (dbThread->GetWayByOffset(reference.GetFileOffset(),way)) {
            if (way->GetCenter(center)) {
                this->magnification=osmscout::Magnification::magVeryClose;

                TriggerMapRendering();
            }
        }
    }
    else {
        assert(false);
    }
}

void MapWidget::reloadStyle() {
    DBThread* dbThread=DBThread::GetInstance();
    if(dbThread->ReloadStyle()){
        TriggerMapRendering();
        hasErrors = false;
    } else {
        StyleError err = dbThread->GetStyleErrors().at(0);
        errorLine = err.GetLine();
        errorColumn = err.GetColumn();
        errorDescription = err.GetTypeName()+": "+err.GetText();
        hasErrors = true;
    }
}

void MapWidget::reloadTmpStyle() {
    DBThread* dbThread=DBThread::GetInstance();
    if(dbThread->ReloadStyle(TMP_SUFFIX)){
        TriggerMapRendering();
        hasErrors = false;
    } else {
        StyleError err = dbThread->GetStyleErrors().at(0);
        errorLine = err.GetLine();
        errorColumn = err.GetColumn();
        errorDescription = err.GetTypeName()+": "+err.GetText();
        hasErrors = true;
    }
}
