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
#include <QtSvg/QSvgRenderer>

namespace osmscout {

#define TMP_SUFFIX ".tmp"

//! We rotate in 16 steps
static double DELTA_ANGLE=2*M_PI/16.0;

MapWidget::MapWidget(QQuickItem* parent)
    : QQuickPaintedItem(parent)
{
    setOpaquePainting(true);
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptHoverEvents(true);

    renderer = OSMScoutQt::GetInstance().MakeMapRenderer(renderingType);
    auto settings=OSMScoutQt::GetInstance().GetSettings();

    DBThreadRef dbThread = OSMScoutQt::GetInstance().GetDBThread();

    connect(settings.get(), &Settings::MapDPIChange,
            this, &MapWidget::onMapDPIChange);

    tapRecognizer.setPhysicalDpi(dbThread->GetPhysicalDpi());

    connect(renderer, &MapRenderer::Redraw,
            this, &MapWidget::redraw);
    connect(dbThread.get(), &DBThread::stylesheetFilenameChanged,
            this, &MapWidget::stylesheetFilenameChanged);
    connect(dbThread.get(), &DBThread::styleErrorsChanged,
            this, &MapWidget::styleErrorsChanged);
    connect(dbThread.get(), &DBThread::databaseLoadFinished,
            this, &MapWidget::databaseLoaded);

    connect(&tapRecognizer, &TapRecognizer::tap,        this, &MapWidget::onTap);
    connect(&tapRecognizer, &TapRecognizer::doubleTap,  this, &MapWidget::onDoubleTap);
    connect(&tapRecognizer, &TapRecognizer::longTap,    this, &MapWidget::onLongTap);
    connect(&tapRecognizer, &TapRecognizer::tapLongTap, this, &MapWidget::onTapLongTap);

    connect(this, &QQuickItem::widthChanged, this, &MapWidget::onResize);
    connect(this, &QQuickItem::heightChanged, this, &MapWidget::onResize);

    // TODO, open last position, move to current position or get as constructor argument...
    view = new MapView(this,
                       osmscout::GeoCoord(0.0, 0.0),
                       /*angle*/ Bearing(),
                       Magnification(Magnification::magContinent),
                       settings->GetMapDPI());
    setupInputHandler(new InputHandler(*view));
    setKeepTouchGrab(true);

    setRenderTarget(RenderTarget::FramebufferObject);
    setPerformanceHints(PerformanceHint::FastFBOResizing);

    loadVehicleIcons();
}

MapWidget::~MapWidget()
{
    delete inputHandler;
    delete view;
    if (renderer!=nullptr){
      renderer->deleteLater();
      renderer=nullptr;
    }
}

void MapWidget::translateToTouch(QMouseEvent* event, Qt::TouchPointStates states)
{
    assert(event);
    QMouseEvent *mEvent = static_cast<QMouseEvent *>(event);

    QTouchEvent::TouchPoint touchPoint;
    touchPoint.setPressure(1);
    touchPoint.setPos(mEvent->pos());
    touchPoint.setState(states);

    QList<QTouchEvent::TouchPoint> points;
    points << touchPoint;
    QTouchEvent touchEvnt(QEvent::TouchBegin,0, Qt::NoModifier, 0, points);
    //qDebug() << "translate mouse event to touch event: "<< touchEvnt;
    touchEvent(&touchEvnt);
}
void MapWidget::mousePressEvent(QMouseEvent* event)
{
    assert(event);
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
    if (inputHandler != nullptr){
        locked = inputHandler->isLockedToPosition();
        inputHandler->deleteLater();
    }
    inputHandler = newGesture;

    connect(inputHandler, &InputHandler::viewChanged,
            this, &MapWidget::changeView);

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
    assert(event);
    vehicle.lastGesture.restart();
    if (!inputHandler->touch(*event)){
        if (event->touchPoints().size() == 1){
            QTouchEvent::TouchPoint tp = event->touchPoints()[0];
            setupInputHandler(new DragHandler(*view));
        }else{
            setupInputHandler(new MultitouchHandler(*view));
        }
        inputHandler->touch(*event);
    }

    tapRecognizer.touch(*event);

    event->accept();

    qDebug() << "touchEvent:";
    QList<QTouchEvent::TouchPoint> relevantTouchPoints;
    for (QTouchEvent::TouchPoint tp: event->touchPoints()){
      Qt::TouchPointStates state(tp.state());
      qDebug() << "  " << state <<" " << tp.id() <<
              " pos " << tp.pos().x() << "x" << tp.pos().y() <<
              " @ " << tp.pressure();
    }
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
    osmscout::MercatorProjection projection = getProjection();
    if (!projection.IsValid()){
      qWarning() << "Projection is not valid!";
      return;
    }
    bool animationInProgress = inputHandler->animationInProgress();

    painter->setRenderHint(QPainter::Antialiasing, !animationInProgress);
    painter->setRenderHint(QPainter::TextAntialiasing, !animationInProgress);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, !animationInProgress);

    MapViewStruct request;
    QRectF        boundingBox = contentsBoundingRect();

    request.coord = view->center;
    request.angle = view->angle;
    request.magnification = view->magnification;
    request.width = boundingBox.width();
    request.height = boundingBox.height();
    request.dpi = view->mapDpi;

    bool oldFinished = finished;
    assert(renderer);
    finished = renderer->RenderMap(*painter,request);
    if (oldFinished != finished){
        emit finishedChanged(finished);
    }

    // render vehicle
    if (vehicle.position && !vehicle.getIcon().isNull()){
      QImage vehicleIcon=vehicle.getIcon();
      double x;
      double y;
      projection.GeoToPixel(vehicle.position->getCoord(), x, y);

      Bearing iconAngle;
      if (vehicle.position->getBearing()) {
        Bearing vehicleBearing = *(vehicle.position->getBearing());
        Bearing projectionBearing = Bearing::Radians(projection.GetAngle());
        iconAngle = vehicleBearing + projectionBearing;
      }

      painter->save();
      QTransform t=QTransform::fromTranslate(x, y); // move to rotation center
      t.rotateRadians(iconAngle.AsRadians());
      painter->setTransform(t);
      // draw vehicleIcon center on coordinate 0x0
      painter->drawImage(QPointF(vehicleIcon.width()/-2, vehicleIcon.height()/-2), vehicleIcon);
      painter->restore();
    }

    // render current position spot
    if (showCurrentPosition && currentPosition.valid){
        double x;
        double y;
        projection.GeoToPixel(currentPosition.coord, x, y);
        if (boundingBox.contains(x, y)){

            if (currentPosition.horizontalAccuracyValid){
                double diameter = currentPosition.horizontalAccuracy * projection.GetMeterInPixel();
                if (diameter > 25.0 && diameter < std::max(request.width, request.height)){
                    painter->setBrush(QBrush(QColor::fromRgbF(1.0, 1.0, 1.0, 0.4)));
                    painter->setPen(QColor::fromRgbF(1.0, 1.0, 1.0, 0.7));
                    painter->drawEllipse(x - (diameter /2.0), y - (diameter /2.0), diameter, diameter);
                }
            }

            if (currentPosition.lastUpdate.secsTo(QDateTime::currentDateTime()) > 60) {
                // outdated, use greyed green
                painter->setBrush(QBrush(QColor::fromRgb(0x73, 0x8d, 0x73, 0x99)));
            }else{
                // updated, use green
                painter->setBrush(QBrush(QColor::fromRgb(0, 0xff, 0, 0x99)));
            }
            painter->setPen(QColor::fromRgbF(0.0, 0.5, 0.0, 0.9));
            double dimension = projection.ConvertWidthToPixel(2.8);
            painter->drawEllipse(x - dimension/2, y - dimension/2, dimension, dimension);
        }
    }

    // render marks
    if (!marks.isEmpty()){
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
                double dimension = projection.ConvertWidthToPixel(6);
                painter->drawEllipse(x - dimension/2, y - dimension/2, dimension, dimension);
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
  Distance dimension = osmscout::GetEllipsoidalDistance(resp.boundingBox.GetMinCoord(),
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

  vehicle.lastGesture.restart();
  if (!inputHandler->zoom(zoomFactor, widgetPosition, QRect(0, 0, width(), height()))){
    setupInputHandler(new MoveHandler(*view));
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
    vehicle.lastGesture.restart();
    if (!inputHandler->move(vector)){
        setupInputHandler(new MoveHandler(*view));
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

void MapWidget::rotateTo(double angle)
{
    vehicle.lastGesture.restart();
    if (!inputHandler->rotateTo(angle)){
        setupInputHandler(new MoveHandler(*view));
        inputHandler->rotateTo(angle);
    }
}

void MapWidget::rotateLeft()
{
    vehicle.lastGesture.restart();
    if (!inputHandler->rotateBy(-DELTA_ANGLE)){
        setupInputHandler(new MoveHandler(*view));
        inputHandler->rotateBy(-DELTA_ANGLE);
    }
}

void MapWidget::rotateRight()
{
    vehicle.lastGesture.restart();
    if (!inputHandler->rotateBy(DELTA_ANGLE)){
        setupInputHandler(new MoveHandler(*view));
        inputHandler->rotateBy(DELTA_ANGLE);
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
        if (!inputHandler->currentPosition(currentPosition.valid, currentPosition.coord)){
            setupInputHandler(new LockHandler(*view, QSizeF(width(), height())));
            inputHandler->currentPosition(currentPosition.valid, currentPosition.coord);
        }
    }else{
        setupInputHandler(new InputHandler(*view));
    }
}

void MapWidget::setFollowVehicle(bool follow){
  vehicle.follow = follow;
  vehicle.lastGesture.invalidate(); // set to invalid state
  if (follow){
    if (!inputHandler->isFollowVehicle()){
      setupInputHandler(new VehicleFollowHandler(*view, QSizeF(width(), height())));
    }
  }else{
    setupInputHandler(new InputHandler(*view));
  }
}

void MapWidget::showCoordinates(osmscout::GeoCoord coord, osmscout::Magnification magnification)
{
    assert(view);
    vehicle.lastGesture.restart();
    if (!inputHandler->showCoordinates(coord, magnification, view->angle)){
        setupInputHandler(new JumpHandler(*view));
        inputHandler->showCoordinates(coord, magnification, view->angle);
    }
}

void MapWidget::showCoordinates(double lat, double lon)
{
    showCoordinates(osmscout::GeoCoord(lat,lon), Magnification(Magnification::magVeryClose));
}

void MapWidget::showCoordinatesInstantly(osmscout::GeoCoord coord, osmscout::Magnification magnification)
{

    MapView newView = *view;
    newView.magnification = magnification;
    newView.center = coord;
    vehicle.lastGesture.restart();
    setupInputHandler(new InputHandler(newView));
    changeView(newView);
}

void MapWidget::showCoordinatesInstantly(double lat, double lon)
{
    showCoordinatesInstantly(osmscout::GeoCoord(lat,lon), Magnification(Magnification::magVeryClose));
}

osmscout::Magnification MapWidget::magnificationByDimension(const Distance &d)
{
  osmscout::MagnificationLevel mag = osmscout::Magnification::magBlock;
  double dimension = d.As<Kilometer>();
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
  Distance dimension = Meters(10);
  if (location->getBBox().IsValid()){
    center = location->getBBox().GetCenter();
    dimension = osmscout::GetEllipsoidalDistance(location->getBBox().GetMinCoord(),
                                                 location->getBBox().GetMaxCoord());
  }else{
    center = location->getCoord();
  }

  showCoordinates(center, magnificationByDimension(dimension));
}

void MapWidget::locationChanged(bool locationValid,
                                double lat, double lon,
                                bool horizontalAccuracyValid,
                                double horizontalAccuracy,
                                const QDateTime &lastUpdate)
{
    // location
    this->currentPosition.lastUpdate = lastUpdate;
    this->currentPosition.valid = locationValid;
    this->currentPosition.coord.Set(lat, lon);
    this->currentPosition.horizontalAccuracyValid = horizontalAccuracyValid;
    this->currentPosition.horizontalAccuracy = horizontalAccuracy;

    inputHandler->currentPosition(locationValid, currentPosition.coord);

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

void MapWidget::addOverlayObject(int id, QObject *o)
{
  OverlayObjectRef copy;
  const OverlayObject *obj = dynamic_cast<const OverlayObject*>(o);
  if (obj == nullptr){
      qWarning() << "Failed to cast " << o << " to OverlayObject.";
      return;
  }
  // create shared pointer copy
  if (obj->getObjectType()==osmscout::refWay){
    copy = std::make_shared<OverlayWay>(static_cast<const OverlayWay&>(*obj));
  }else if (obj->getObjectType()==osmscout::refArea){
    copy = std::make_shared<OverlayArea>(static_cast<const OverlayArea&>(*obj));
  }else if (obj->getObjectType()==osmscout::refNode){
    copy = std::make_shared<OverlayNode>(static_cast<const OverlayNode&>(*obj));
  }

  if (copy){
    renderer->addOverlayObject(id, copy);
  }
}

void MapWidget::removeOverlayObject(int id)
{
  renderer->removeOverlayObject(id);
}

void MapWidget::removeAllOverlayObjects()
{
  renderer->removeAllOverlayObjects();
}

OverlayWay *MapWidget::createOverlayWay(QString type)
{
  OverlayWay *result=new OverlayWay();
  result->setTypeName(type);
  return result;
}

OverlayArea *MapWidget::createOverlayArea(QString type)
{
  OverlayArea *result=new OverlayArea();
  result->setTypeName(type);
  return result;
}

OverlayNode *MapWidget::createOverlayNode(QString type)
{
  OverlayNode *result=new OverlayNode();
  result->setTypeName(type);
  return result;
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
    MapView v = *view;
    v.mapDpi = dpi;
    changeView(v);

    loadVehicleIcons();

    // discard current input handler
    setupInputHandler(new InputHandler(*view));
    emit viewChanged();
}

void MapWidget::onResize()
{
    inputHandler->widgetResized(QSizeF(width(), height()));
}

void MapWidget::SetVehiclePosition(QObject *o)
{
  VehiclePosition *updated = dynamic_cast<VehiclePosition*>(o);
  if (o != nullptr && updated == nullptr){
    qWarning() << "Failed to cast " << o << " to VehiclePosition*.";
    return;
  }
  if (updated == nullptr){
    if (vehicle.position != nullptr) {
      delete vehicle.position;
      vehicle.position = nullptr;
    }
  }else{
    if (vehicle.position==nullptr){
      vehicle.position = new VehiclePosition(this);
    }
    *vehicle.position = *updated;
  }

  if (vehicle.position != nullptr &&
      vehicle.follow &&
      ((!vehicle.lastGesture.isValid()) || vehicle.lastGesture.elapsed() > 4000) // there was no gesture event for 4s
      ) {

    if (!inputHandler->isFollowVehicle()){
      setupInputHandler(new VehicleFollowHandler(*view, QSizeF(width(), height())));
    }

    inputHandler->vehiclePosition(*vehicle.position);
  }
  redraw();
}

QImage MapWidget::loadSVGIcon(const QString &directory, const QString fileName, double iconPixelSize)
{
  QImage image(iconPixelSize, iconPixelSize, QImage::Format_ARGB32);
  image.fill(Qt::transparent);

  QString iconPath=directory + QDir::separator() + fileName;
  QSvgRenderer renderer(iconPath);
  if (renderer.isValid()) {
    QPainter painter(&image);
    renderer.render(&painter);
    painter.end();
    qDebug() << "Loaded icon" << iconPath;
  }else{
    qWarning() << "Cannot load icon" << iconPath;
  }
  return image;
}


void MapWidget::loadVehicleIcons()
{
  double iconPixelSize=getProjection().ConvertWidthToPixel(vehicle.iconSize);
  QString iconDirectory=OSMScoutQt::GetInstance().GetIconDirectory();

  vehicle.standardIcon=loadSVGIcon(iconDirectory, vehicle.standardIconFile, iconPixelSize);
  vehicle.noGpsSignalIcon=loadSVGIcon(iconDirectory, vehicle.noGpsSignalIconFile, iconPixelSize);
  vehicle.inTunnelIcon=loadSVGIcon(iconDirectory, vehicle.inTunnelIconFile, iconPixelSize);
}

QString MapWidget::GetStylesheetFilename() const
{
    DBThreadRef dbThread=OSMScoutQt::GetInstance().GetDBThread();
    return dbThread->GetStylesheetFilename();
}

QString MapWidget::GetZoomLevelName() const
{
    osmscout::MagnificationConverter converter;
    std::string                      name;

    if (converter.Convert(osmscout::MagnificationLevel(view->magnification.GetLevel()),
                          name)) {
      return name.c_str();
    }

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

QString MapWidget::GetRenderingType() const
{
  if (renderingType==RenderingType::TiledRendering)
    return "tiled";
  return "plane";
}

void MapWidget::SetRenderingType(QString strType)
{
  RenderingType type=RenderingType::PlaneRendering;
  if (strType=="tiled"){
    type=RenderingType::TiledRendering;
  }
  if (type!=renderingType){
    renderingType=type;

    std::map<int,OverlayObjectRef> overlayWays = renderer->getOverlayObjects();
    renderer->deleteLater();

    renderer = OSMScoutQt::GetInstance().MakeMapRenderer(renderingType);
    for (auto &p:overlayWays){
      renderer->addOverlayObject(p.first, p.second);
    }
    connect(renderer, &MapRenderer::Redraw,
            this, &MapWidget::redraw);
    emit renderingTypeChanged(GetRenderingType());
  }
}
}
