/*
  This source is part of the libosmscout-map library
  Copyright (C) 2010  Tim Teulings
  Copyright (C) 2016  Lukáš Karas

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

#include <cmath>
#include <iostream>

#include <osmscout/MapWidget.h>
#include <osmscout/InputHandler.h>
#include <osmscout/OSMScoutQt.h>

#define TMP_SUFFIX ".tmp"

//! We rotate in 16 steps
static double DELTA_ANGLE=2*M_PI/16.0;

MapWidget::MapWidget(QQuickItem* parent)
    : QQuickPaintedItem(parent),
      inputHandler(NULL), 
      showCurrentPosition(false),
      finished(false)
{
    setOpaquePainting(true);
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptHoverEvents(true);
    
    DBThreadRef dbThread = OSMScoutQt::GetInstance().GetDBThread();
    
    mapDpi = dbThread->GetSettings()->GetMapDPI();

    connect(dbThread->GetSettings().get(), SIGNAL(MapDPIChange(double)),
            this, SLOT(onMapDPIChange(double)));
  
    tapRecognizer.setPhysicalDpi(dbThread->GetPhysicalDpi());

    //setFocusPolicy(Qt::StrongFocus);

    connect(dbThread.get(),SIGNAL(Redraw()),
            this,SLOT(redraw()));    
    connect(dbThread.get(),SIGNAL(stylesheetFilenameChanged()),
            this,SIGNAL(stylesheetFilenameChanged()));
    connect(dbThread.get(),SIGNAL(styleErrorsChanged()),
            this,SIGNAL(styleErrorsChanged()));
    connect(dbThread.get(),SIGNAL(databaseLoadFinished(osmscout::GeoBox)),
            this,SIGNAL(databaseLoaded(osmscout::GeoBox)));
        
    connect(&tapRecognizer, SIGNAL(tap(const QPoint)),        this, SLOT(onTap(const QPoint)));
    connect(&tapRecognizer, SIGNAL(doubleTap(const QPoint)),  this, SLOT(onDoubleTap(const QPoint)));
    connect(&tapRecognizer, SIGNAL(longTap(const QPoint)),    this, SLOT(onLongTap(const QPoint)));
    connect(&tapRecognizer, SIGNAL(tapLongTap(const QPoint)), this, SLOT(onTapLongTap(const QPoint)));
    
    // TODO, open last position, move to current position or get as constructor argument...
    view = new MapView(this, osmscout::GeoCoord(0.0, 0.0), /*angle*/ 0, osmscout::Magnification::magContinent);
    setupInputHandler(new InputHandler(*view));
    setKeepTouchGrab(true);

    setRenderTarget(RenderTarget::FramebufferObject);
    setPerformanceHints(PerformanceHint::FastFBOResizing);
}

MapWidget::~MapWidget()
{
    delete inputHandler;
    delete view;
}

void MapWidget::translateToTouch(QMouseEvent* event, Qt::TouchPointStates states)
{
    QMouseEvent *mEvent = static_cast<QMouseEvent *>(event);

    QTouchEvent::TouchPoint touchPoint;
    touchPoint.setPressure(1);
    touchPoint.setPos(mEvent->pos());
    touchPoint.setState(states);
    
    QList<QTouchEvent::TouchPoint> points;
    points << touchPoint;
    QTouchEvent *touchEvnt = new QTouchEvent(QEvent::TouchBegin,0, Qt::NoModifier, 0, points);
    //qDebug() << "translate mouse event to touch event: "<< touchEvnt;
    touchEvent(touchEvnt);
    delete touchEvnt;
}
void MapWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button()==1) {
        translateToTouch(event, Qt::TouchPointPressed);
    }
}
void MapWidget::mouseMoveEvent(QMouseEvent* event)
{
    translateToTouch(event, Qt::TouchPointMoved);
}
void MapWidget::hoverMoveEvent(QHoverEvent* event) {
    QQuickPaintedItem::hoverMoveEvent(event);

    double lat;
    double lon;
    getProjection().PixelToGeo(event->pos().x(), event->pos().y(), lon, lat);
    emit mouseMove(event->pos().x(), event->pos().y(), lat, lon, event->modifiers());
}
void MapWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button()==1) {
        translateToTouch(event, Qt::TouchPointReleased);
    }
}
 
void MapWidget::setupInputHandler(InputHandler *newGesture)
{
    bool locked = false; 
    if (inputHandler != NULL){
        locked = inputHandler->isLockedToPosition();
        delete inputHandler;
    }
    inputHandler = newGesture;
    
    connect(inputHandler, SIGNAL(viewChanged(const MapView&)), 
            this, SLOT(changeView(const MapView&)));

    if (locked != inputHandler->isLockedToPosition()){
        emit lockToPossitionChanged();
    }
    //qDebug() << "Input handler changed (" << (newGesture->animationInProgress()? "animation": "stationary") << ")";
}

void MapWidget::redraw()
{
    update();
}

void MapWidget::changeView(const MapView &updated)
{
    //qDebug() << "viewChanged: " << QString::fromStdString(updated.center.GetDisplayText()) << "   level: " << updated.magnification.GetLevel();
    //qDebug() << "viewChanged (" << (inputHandler->animationInProgress()? "animation": "stationary") << ")";
    bool changed = *view != updated;
    view->operator =( updated );
    // make sure that we render map with antialiasing. TODO: do it better
    if (changed || (!inputHandler->animationInProgress())){
        redraw();
    }
    if (changed){
        emit viewChanged();
    }
}

void MapWidget::touchEvent(QTouchEvent *event)
{
  
    if (!inputHandler->touch(event)){
        if (event->touchPoints().size() == 1){
            QTouchEvent::TouchPoint tp = event->touchPoints()[0];
            setupInputHandler(new DragHandler(*view, mapDpi));
        }else{
            setupInputHandler(new MultitouchHandler(*view, mapDpi));
        }
        inputHandler->touch(event);
    }

    tapRecognizer.touch(event);
    
    event->accept();
  
    /*
    qDebug() << "touchEvent:";
    QList<QTouchEvent::TouchPoint> relevantTouchPoints;
    for (QTouchEvent::TouchPoint tp: event->touchPoints()){
      Qt::TouchPointStates state(tp.state());
      qDebug() << "  " << state <<" " << tp.id() << 
              " pos " << tp.pos().x() << "x" << tp.pos().y() << 
              " @ " << tp.pressure();    
    }
    */
 }

void MapWidget::focusOutEvent(QFocusEvent *event)
{
    qDebug() << "focus-out event";
    if (!inputHandler->focusOutEvent(event)){
        setupInputHandler(new InputHandler(*view));
    }
    QQuickPaintedItem::focusOutEvent(event);
}

void MapWidget::wheelEvent(QWheelEvent* event)
{
    static int cumulNumDegrees = 0;
    cumulNumDegrees += event->angleDelta().y();
    if(abs(cumulNumDegrees) < 120){
        return;
    }

    int numDegrees =  cumulNumDegrees / 8;
    int numSteps = numDegrees / 15;

    if (numSteps>=0) {
        zoomIn(numSteps*1.35, event->pos());
    }
    else {
        zoomOut(-numSteps*1.35, event->pos());
    }
    cumulNumDegrees %= 120;

    event->accept();
}

void MapWidget::paint(QPainter *painter)
{
    DBThreadRef dbThread=OSMScoutQt::GetInstance().GetDBThread();

    bool animationInProgress = inputHandler->animationInProgress();
    
    painter->setRenderHint(QPainter::Antialiasing, !animationInProgress);
    painter->setRenderHint(QPainter::TextAntialiasing, !animationInProgress);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, !animationInProgress);
    painter->setRenderHint(QPainter::HighQualityAntialiasing, !animationInProgress);
    
    RenderMapRequest request;
    QRectF           boundingBox = contentsBoundingRect();

    request.coord = view->center;
    request.angle = view->angle;
    request.magnification = view->magnification;
    request.width = boundingBox.width();
    request.height = boundingBox.height();

    bool oldFinished = finished;
    finished = dbThread->RenderMap(*painter,request);
    if (oldFinished != finished){
        emit finishedChanged(finished);
    }

    // render current position spot
    if (showCurrentPosition && locationValid){
        osmscout::MercatorProjection projection = getProjection();
        
        double x;
        double y;
        projection.GeoToPixel(osmscout::GeoCoord(currentPosition.GetLat(), currentPosition.GetLon()), x, y);
        if (boundingBox.contains(x, y)){
            
            if (horizontalAccuracyValid){
                double diameter = horizontalAccuracy * projection.GetMeterInPixel();
                if (diameter > 25.0 && diameter < std::max(request.width, request.height)){
                    painter->setBrush(QBrush(QColor::fromRgbF(1.0, 1.0, 1.0, 0.4)));
                    painter->setPen(QColor::fromRgbF(1.0, 1.0, 1.0, 0.7));
                    painter->drawEllipse(x - (diameter /2.0), y - (diameter /2.0), diameter, diameter);
                }
            }
            
            // TODO: take DPI into account
            painter->setBrush(QBrush(QColor::fromRgbF(0,1,0, .6)));
            painter->setPen(QColor::fromRgbF(0.0, 0.5, 0.0, 0.9));
            painter->drawEllipse(x - 10, y - 10, 20, 20);
        }
    }
    
    // render marks
    if (!marks.isEmpty()){
        osmscout::MercatorProjection projection = getProjection();
        
        double x;
        double y;
        painter->setBrush(QBrush());
        QPen pen;
        pen.setColor(QColor::fromRgbF(0.8, 0.0, 0.0, 0.9));
        pen.setWidth(6);
        painter->setPen(pen);
        
        for (auto &entry: marks){
            projection.GeoToPixel(osmscout::GeoCoord(entry.GetLat(), entry.GetLon()), x, y);
            if (boundingBox.contains(x, y)){
                // TODO: take DPI into account
                painter->drawEllipse(x - 20, y - 20, 40, 40);
            }
        }
    }
}

void MapWidget::recenter()
{
  DBThreadRef dbThread = OSMScoutQt::GetInstance().GetDBThread();
  DatabaseLoadedResponse resp = dbThread->loadedResponse();
  if (!resp.boundingBox.IsValid()){
    return;
  }
  double dimension = osmscout::GetEllipsoidalDistance(resp.boundingBox.GetMinCoord(),
                                                      resp.boundingBox.GetMaxCoord());
  
  showCoordinates(resp.boundingBox.GetCenter(), magnificationByDimension(dimension));
}

bool MapWidget::isDatabaseLoaded()
{
  DBThreadRef dbThread = OSMScoutQt::GetInstance().GetDBThread();
  DatabaseLoadedResponse resp = dbThread->loadedResponse();
  return resp.boundingBox.IsValid();
}

bool MapWidget::isInDatabaseBoundingBox(double lat, double lon)
{
  DBThreadRef dbThread = OSMScoutQt::GetInstance().GetDBThread();
  DatabaseLoadedResponse resp = dbThread->loadedResponse();
  if (!resp.boundingBox.IsValid()){
    return false;
  }
  osmscout::GeoCoord coord(lat, lon);
  return resp.boundingBox.Includes(coord);
}

QPointF MapWidget::screenPosition(double lat, double lon)
{
    double x;
    double y;
    getProjection().GeoToPixel(osmscout::GeoCoord(lat, lon), x, y);
    return QPointF(x, y);
}

void MapWidget::zoom(double zoomFactor)
{
    zoom(zoomFactor, QPoint(width()/2, height()/2));
}

void MapWidget::zoomIn(double zoomFactor)
{
    zoomIn(zoomFactor, QPoint(width()/2, height()/2));
}

void MapWidget::zoomOut(double zoomFactor)
{
    zoomOut(zoomFactor, QPoint(width()/2, height()/2));
}

void MapWidget::zoom(double zoomFactor, const QPoint widgetPosition)
{
  if (zoomFactor == 1)
    return;
  
  if (!inputHandler->zoom(zoomFactor, widgetPosition, QRect(0, 0, width(), height()))){
    setupInputHandler(new MoveHandler(*view, mapDpi));
    inputHandler->zoom(zoomFactor, widgetPosition, QRect(0, 0, width(), height()));
  }
}

void MapWidget::zoomIn(double zoomFactor, const QPoint widgetPosition)
{
    zoom(zoomFactor, widgetPosition);
}

void MapWidget::zoomOut(double zoomFactor, const QPoint widgetPosition)
{
    zoom( 1.0/zoomFactor, widgetPosition);
}

void MapWidget::move(QVector2D vector)
{
    if (!inputHandler->move(vector)){
        setupInputHandler(new MoveHandler(*view, mapDpi));
        inputHandler->move(vector);
    }
}

void MapWidget::left()
{
    move(QVector2D( width()/-3, 0 ));
}

void MapWidget::right()
{
    move(QVector2D( width()/3, 0 ));
}

void MapWidget::up()
{
    move(QVector2D( 0, height()/-3 ));
}

void MapWidget::down()
{
    move(QVector2D( 0, height()/+3 ));
}

void MapWidget::rotateLeft()
{
    if (!inputHandler->rotateBy(DELTA_ANGLE, -DELTA_ANGLE)){
        setupInputHandler(new MoveHandler(*view, mapDpi));
        inputHandler->rotateBy(DELTA_ANGLE, -DELTA_ANGLE);
    }
}

void MapWidget::rotateRight()
{
    if (!inputHandler->rotateBy(DELTA_ANGLE, DELTA_ANGLE)){
        setupInputHandler(new MoveHandler(*view, mapDpi));
        inputHandler->rotateBy(DELTA_ANGLE, DELTA_ANGLE);
    }
}

void MapWidget::toggleDaylight()
{
    DBThreadRef dbThread=OSMScoutQt::GetInstance().GetDBThread();

    dbThread->ToggleDaylight();
    redraw();
}

void MapWidget::reloadStyle()
{
    DBThreadRef dbThread=OSMScoutQt::GetInstance().GetDBThread();

    dbThread->ReloadStyle();
    redraw();
}

void MapWidget::reloadTmpStyle() {
    DBThreadRef dbThread=OSMScoutQt::GetInstance().GetDBThread();
    dbThread->ReloadStyle(TMP_SUFFIX);
    redraw();    
}

void MapWidget::setLockToPosition(bool lock){
    if (lock){
        if (!inputHandler->currentPosition(locationValid, currentPosition)){
            setupInputHandler(new LockHandler(*view, mapDpi, std::min(width(), height()) / 3));
            inputHandler->currentPosition(locationValid, currentPosition);
        }
    }else{
        setupInputHandler(new InputHandler(*view));
    }
}

void MapWidget::showCoordinates(osmscout::GeoCoord coord, osmscout::Magnification magnification)
{
    if (!inputHandler->showCoordinates(coord, magnification)){
        setupInputHandler(new JumpHandler(*view));
        inputHandler->showCoordinates(coord, magnification);
    }
}

void MapWidget::showCoordinates(double lat, double lon)
{
    showCoordinates(osmscout::GeoCoord(lat,lon), osmscout::Magnification::magVeryClose);
}

void MapWidget::showCoordinatesInstantly(osmscout::GeoCoord coord, osmscout::Magnification magnification)
{
    
    MapView newView = *view;
    newView.magnification = magnification;
    newView.center = coord;
    setupInputHandler(new InputHandler(newView));
    changeView(newView);
}

void MapWidget::showCoordinatesInstantly(double lat, double lon)
{
    showCoordinatesInstantly(osmscout::GeoCoord(lat,lon), osmscout::Magnification::magVeryClose);    
}

osmscout::Magnification MapWidget::magnificationByDimension(double dimension)
{
  osmscout::Magnification::Mag mag = osmscout::Magnification::magBlock;
  if (dimension > 0.1)
    mag = osmscout::Magnification::magVeryClose;
  if (dimension > 0.2)
    mag = osmscout::Magnification::magCloser;
  if (dimension > 0.5)
    mag = osmscout::Magnification::magClose;
  if (dimension > 1)
    mag = osmscout::Magnification::magDetail;
  if (dimension > 3)
    mag = osmscout::Magnification::magSuburb;
  if (dimension > 7)
    mag = osmscout::Magnification::magCity;
  if (dimension > 15)
    mag = osmscout::Magnification::magCityOver;
  if (dimension > 30)
    mag = osmscout::Magnification::magProximity;
  if (dimension > 60)
    mag = osmscout::Magnification::magRegion;
  if (dimension > 120)
    mag = osmscout::Magnification::magCounty;
  if (dimension > 240)
    mag = osmscout::Magnification::magStateOver;
  if (dimension > 500)
    mag = osmscout::Magnification::magState;
  return osmscout::Magnification(mag);
}

void MapWidget::showLocation(LocationEntry* location)
{
  if (!location){
    qWarning() << "Invalid location" << location;
    return;
  }
  qDebug() << "Show location: " << location;
    
  osmscout::GeoCoord center;
  double dimension = 0.01; // km
  if (location->getBBox().IsValid()){
    center = location->getBBox().GetCenter();
    dimension = osmscout::GetEllipsoidalDistance(location->getBBox().GetMinCoord(),
                                                 location->getBBox().GetMaxCoord());
  }else{
    center = location->getCoord();
  }
  
       
  showCoordinates(center, magnificationByDimension(dimension));
}

void MapWidget::locationChanged(bool locationValid, double lat, double lon, bool horizontalAccuracyValid, double horizontalAccuracy)
{
    // location
    lastUpdate.restart();
    this->locationValid = locationValid;
    this->currentPosition.Set(lat, lon);
    this->horizontalAccuracyValid = horizontalAccuracyValid;
    this->horizontalAccuracy = horizontalAccuracy;

    inputHandler->currentPosition(locationValid, currentPosition);

    redraw();
}

void MapWidget::addPositionMark(int id, double lat, double lon)
{
    marks.insert(id, osmscout::GeoCoord(lat, lon));
    update();
}

void MapWidget::removePositionMark(int id)
{
    marks.remove(id);
    update();
}

void MapWidget::onTap(const QPoint p)
{
    qDebug() << "tap " << p;
    double lat;
    double lon;
    getProjection().PixelToGeo(p.x(), p.y(), lon, lat);
    emit tap(p.x(), p.y(), lat, lon);
}

void MapWidget::onDoubleTap(const QPoint p)
{
    qDebug() << "double tap " << p;
    zoomIn(2.0, p);
    double lat;
    double lon;
    getProjection().PixelToGeo(p.x(), p.y(), lon, lat);
    emit doubleTap(p.x(), p.y(), lat, lon);
}

void MapWidget::onLongTap(const QPoint p)
{
    qDebug() << "long tap " << p;
    double lat;
    double lon;
    getProjection().PixelToGeo(p.x(), p.y(), lon, lat);
    emit longTap(p.x(), p.y(), lat, lon);
}

void MapWidget::onTapLongTap(const QPoint p)
{
    qDebug() << "tap, long tap " << p;
    zoomOut(2.0, p);
    double lat;
    double lon;
    getProjection().PixelToGeo(p.x(), p.y(), lon, lat);
    emit tapLongTap(p.x(), p.y(), lat, lon);
}

void MapWidget::onMapDPIChange(double dpi)
{
    mapDpi = dpi;
    
    // discard current input handler
    setupInputHandler(new InputHandler(*view));
}

QString MapWidget::GetStylesheetFilename() const
{
    DBThreadRef dbThread=OSMScoutQt::GetInstance().GetDBThread();
    return dbThread->GetStylesheetFilename();
}

QString MapWidget::GetZoomLevelName() const
{
    double level = view->magnification.GetMagnification();
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
bool MapWidget::stylesheetHasErrors() const
{
    DBThreadRef dbThread=OSMScoutQt::GetInstance().GetDBThread();
    return !dbThread->GetStyleErrors().isEmpty();
}
int MapWidget::firstStylesheetErrorLine() const
{
    DBThreadRef dbThread=OSMScoutQt::GetInstance().GetDBThread();
    QList<StyleError> errors=dbThread->GetStyleErrors();
    if (errors.isEmpty())
      return -1;
    return errors.first().GetLine();
}
int MapWidget::firstStylesheetErrorColumn() const
{
    DBThreadRef dbThread=OSMScoutQt::GetInstance().GetDBThread();
    QList<StyleError> errors=dbThread->GetStyleErrors();
    if (errors.isEmpty())
      return -1;
    return errors.first().GetColumn();  
}
QString MapWidget::firstStylesheetErrorDescription() const
{
    DBThreadRef dbThread=OSMScoutQt::GetInstance().GetDBThread();
    QList<StyleError> errors=dbThread->GetStyleErrors();
    if (errors.isEmpty())
      return "";
    return errors.first().GetDescription();
}

bool MapWidget::toggleDebug()
{
    osmscout::log.Debug(!osmscout::log.IsDebug());

    return osmscout::log.IsDebug();
}

bool MapWidget::toggleInfo()
{
    osmscout::log.Info(!osmscout::log.IsInfo());

    return osmscout::log.IsInfo();
}
