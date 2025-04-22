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

#include <osmscoutclientqt/MapWidget.h>
#include <osmscoutclientqt/InputHandler.h>
#include <osmscoutclientqt/OSMScoutQt.h>
#include <QtSvg/QSvgRenderer>
#include <QtGlobal>
#include <QQuickWindow>

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
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    setAcceptTouchEvents(true);
#endif

    setupRenderer();

    auto settings=OSMScoutQt::GetInstance().GetSettings();

    DBThreadRef dbThread = OSMScoutQt::GetInstance().GetDBThread();

    settings->mapDPIChange.Connect(mapDpiSlot);
    dbThread->stylesheetFilenameChanged.Connect(stylesheetFilenameChangedSlot);
    dbThread->styleErrorsChanged.Connect(styleErrorsChangedSlot);
    dbThread->databaseLoadFinished.Connect(databaseLoadedSlot);
    dbThread->flushCachesSignal.Connect(flushCachesSlot);

    // DBThread::flushCachesSignal can be emitted from arbitrary thread
    connect(this, &MapWidget::flushCachesRequest, this, &MapWidget::FlushCaches, Qt::QueuedConnection);

    tapRecognizer.setPhysicalDpi(dbThread->GetPhysicalDpi());

    connect(&tapRecognizer, &TapRecognizer::tap,        this, &MapWidget::onTap);
    connect(&tapRecognizer, &TapRecognizer::doubleTap,  this, &MapWidget::onDoubleTap);
    connect(&tapRecognizer, &TapRecognizer::longTap,    this, &MapWidget::onLongTap);
    connect(&tapRecognizer, &TapRecognizer::tapLongTap, this, &MapWidget::onTapLongTap);
    connect(&tapRecognizer, &TapRecognizer::tapAndDrag, this, &MapWidget::onTapAndDrag);

    connect(this, &QQuickItem::widthChanged, this, &MapWidget::onResize);
    connect(this, &QQuickItem::heightChanged, this, &MapWidget::onResize);

    connect(&iconAnimation, &IconAnimation::update, this, &MapWidget::redraw);

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
    if (iconLookup!=nullptr) {
      iconLookup->deleteLater();
      iconLookup = nullptr;
    }
}

void MapWidget::setupRenderer()
{
    if (renderer) {
      renderer->deleteLater();
    }

    renderer = OSMScoutQt::GetInstance().MakeMapRenderer(renderingType);

    connect(renderer, &MapRenderer::Redraw,
            this, &MapWidget::redraw);
}

void MapWidget::translateToTouch(QMouseEvent* event, Qt::TouchPointStates states)
{
    assert(event);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // translate the mouse event for desktop device without touch pointer
    QTouchEvent touchEvnt(QEvent::TouchBegin, event->pointingDevice(), Qt::NoModifier, event->points());
#else
    QTouchEvent::TouchPoint touchPoint;
    touchPoint.setPressure(1);
    touchPoint.setPos(event->pos());
    touchPoint.setState(states);
    QList<QTouchEvent::TouchPoint> points;
    points << touchPoint;
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QTouchEvent touchEvnt(QEvent::TouchBegin, 0, Qt::NoModifier, Qt::TouchPointStates(), points);
#else
    QTouchEvent touchEvnt(QEvent::TouchBegin, 0, Qt::NoModifier, 0, points);
#endif
#endif

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

    osmscout::GeoCoord coord;
    getProjection().PixelToGeo(event->pos().x(), event->pos().y(), coord);
    emit mouseMove(event->pos().x(), event->pos().y(), coord.GetLat(), coord.GetLon(), event->modifiers());
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
        emit lockToPositionChanged();
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
    assert(view);
    bool changed = *view != updated;
    bool latChangedFlag = floor(view->GetLat()*10000) != floor(updated.GetLat()*10000);
    bool lonChangedFlag = floor(view->GetLon()*10000) != floor(updated.GetLon()*10000);
    bool angleChangedFlag = floor(view->angle.AsRadians()*10000) != floor(updated.angle.AsRadians()*10000);
    bool magLevelChangedFlag = view->magnification.GetLevel() != updated.magnification.GetLevel();

    double oldPixelSize = floor(GetPixelSize()*1000);
    *view = updated;
    bool pixelSizeChangedFlag = oldPixelSize != floor(GetPixelSize()*1000);

    // make sure that we render map with antialiasing. TODO: do it better
    if (changed || (!inputHandler->animationInProgress())){
        redraw();
    }
    if (latChangedFlag) {
        emit latChanged();
    }
    if (lonChangedFlag) {
        emit lonChanged();
    }
    if (angleChangedFlag) {
        emit angleChanged();
    }
    if (magLevelChangedFlag) {
        emit magLevelChanged();
    }
    if (pixelSizeChangedFlag) {
        emit pixelSizeChanged();
    }
    if (changed){
        emit viewChanged();
    }
}

void MapWidget::touchEvent(QTouchEvent *event)
{
    assert(event);
    vehicle.lastGesture.restart();

    tapRecognizer.touch(*event);

    if (!inputHandler->touch(*event)){
        if (event->touchPoints().size() == 1){
            setupInputHandler(new DragHandler(*view));
        }else{
            setupInputHandler(new MultitouchHandler(*view));
        }
        inputHandler->touch(*event);
    }

    if (preventMouseStealing) {
       int activePoints = std::count_if(event->touchPoints().begin(), event->touchPoints().end(),
                                        [](const auto &tp) { return tp.state() != Qt::TouchPointReleased; });

       if (activePoints == 0) {
         setKeepMouseGrab(false);
         ungrabMouse();
       } else if (!keepMouseGrab()) {
         grabMouse();
         setKeepMouseGrab(true);
       }
    }

    event->accept();
 }

void MapWidget::focusOutEvent(QFocusEvent *event)
{
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

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    QPoint pos = event->pos();
#else
    QPoint pos = QPoint((int)event->position().x(), (int)event->position().y());
#endif

    if (numSteps>=0) {
        zoomIn(numSteps*1.35, pos);
    }
    else {
        zoomOut(-numSteps*1.35, pos);
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

    MapViewStruct request=GetViewStruct();
    QRectF boundingBox = contentsBoundingRect();

    bool oldFinished = finished;
    assert(renderer);
    finished = renderer->RenderMap(*painter,request);
    if (oldFinished != finished){
        emit finishedChanged(finished);
    }

    iconAnimation.paint(painter, projection);

    // render vehicle
    if (vehicle.position && !vehicle.getIcon().isNull()){
      QImage vehicleIcon=vehicle.getIcon();
      osmscout::Vertex2D screenPos;
      projection.GeoToPixel(vehicle.position->getCoord(),
                            screenPos);

      Bearing iconAngle;
      if (vehicle.position->getBearing()) {
        Bearing vehicleBearing = *(vehicle.position->getBearing());
        Bearing projectionBearing = Bearing::Radians(projection.GetAngle());
        iconAngle = vehicleBearing + projectionBearing;
      }

      painter->save();
      painter->translate(screenPos.GetX(),
                         screenPos.GetY());
      painter->rotate(iconAngle.AsDegrees());
      // draw vehicleIcon center on coordinate 0x0
      painter->drawImage(QPointF(vehicleIcon.width()/-2,
                                 vehicleIcon.height()/-2),
                         vehicleIcon);
      painter->restore();
    }

    // render current position spot
    if (showCurrentPosition && currentPosition.valid){
        osmscout::Vertex2D screenPos;
        projection.GeoToPixel(currentPosition.coord,
                              screenPos);
        if (boundingBox.contains(screenPos.GetX(), screenPos.GetY())){

            if (currentPosition.horizontalAccuracyValid){
                double diameter = currentPosition.horizontalAccuracy * projection.GetMeterInPixel();
                if (diameter > 25.0 && diameter < std::max(request.width, request.height)){
                    painter->setBrush(QBrush(QColor::fromRgbF(1.0, 1.0, 1.0, 0.4)));
                    painter->setPen(QColor::fromRgbF(1.0, 1.0, 1.0, 0.7));
                    painter->drawEllipse(screenPos.GetX() - (diameter /2.0),
                                         screenPos.GetY() - (diameter /2.0),
                                         diameter,
                                         diameter);
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
            painter->drawEllipse(screenPos.GetX() - dimension/2,
                                 screenPos.GetY() - dimension/2,
                                 dimension,
                                 dimension);
        }
    }

    // render marks
    if (!marks.isEmpty()){
        painter->setBrush(QBrush());
        QPen pen;
        pen.setColor(QColor::fromRgbF(0.8, 0.0, 0.0, 0.9));
        pen.setWidth(6);
        painter->setPen(pen);

        for (auto &entry: marks){
            osmscout::Vertex2D screenPos;
            projection.GeoToPixel(osmscout::GeoCoord(entry.GetLat(), entry.GetLon()),
                                  screenPos);
            if (boundingBox.contains(screenPos.GetX(), screenPos.GetY())){
                double dimension = projection.ConvertWidthToPixel(6);
                painter->drawEllipse(screenPos.GetX() - dimension/2,
                                     screenPos.GetY() - dimension/2,
                                     dimension, dimension);
            }
        }
    }
}

void MapWidget::recenter()
{
  DBThreadRef dbThread = OSMScoutQt::GetInstance().GetDBThread();
  GeoBox boundingBox = dbThread->databaseBoundingBox();
  if (!boundingBox.IsValid()){
    return;
  }
  Distance dimension = osmscout::GetEllipsoidalDistance(boundingBox.GetMinCoord(),
                                                        boundingBox.GetMaxCoord());

  showCoordinates(boundingBox.GetCenter(), magnificationByDimension(dimension));
}

bool MapWidget::isDatabaseLoaded()
{
  DBThreadRef dbThread = OSMScoutQt::GetInstance().GetDBThread();
  GeoBox boundingBox = dbThread->databaseBoundingBox();
  return boundingBox.IsValid();
}

bool MapWidget::isInDatabaseBoundingBox(double lat, double lon)
{
  DBThreadRef dbThread = OSMScoutQt::GetInstance().GetDBThread();
  GeoBox boundingBox = dbThread->databaseBoundingBox();
  if (!boundingBox.IsValid()){
    return false;
  }
  osmscout::GeoCoord coord(lat, lon);
  return boundingBox.Includes(coord);
}

QPointF MapWidget::screenPosition(double lat, double lon)
{
    osmscout::Vertex2D screenPos;
    getProjection().GeoToPixel(osmscout::GeoCoord(lat, lon),
                               screenPos);
    return QPointF(screenPos.GetX(), screenPos.GetY());
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

void MapWidget::moveLeft()
{
    move(QVector2D( width()/-3, 0 ));
}

void MapWidget::moveRight()
{
    move(QVector2D( width()/3, 0 ));
}

void MapWidget::moveUp()
{
    move(QVector2D( 0, height()/-3 ));
}

void MapWidget::moveDown()
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

void MapWidget::pivotBy(double angleChange)
{
  vehicle.lastGesture.restart();
  if (!inputHandler->pivotBy(angleChange)){
    setupInputHandler(new MoveHandler(*view));
    inputHandler->pivotBy(angleChange);
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
  if (vehicle.follow == follow) {
    return;
  }
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

void MapWidget::setVehicleAutoRotateMap(bool b)
{
  if (vehicle.autoRotateMap != b) {
    vehicle.autoRotateMap = b;
    if (inputHandler->isFollowVehicle() && vehicle.position) {
      inputHandler->vehiclePosition(*vehicle.position, vehicle.autoRotateMap);
    }
    emit vehicleAutoRotateMapChanged();
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

void MapWidget::deactivateIcons()
{
  iconAnimation.deactivateAll();
}

void MapWidget::onTap(const QPoint p)
{
  osmscout::GeoCoord coord;
  getProjection().PixelToGeo(p.x(), p.y(),
                             coord);
  emit tap(p.x(), p.y(), coord.GetLat(), coord.GetLon());
  iconAnimation.deactivateAll();
  if (iconLookup!=nullptr) {
    iconLookup->RequestIcon(GetViewStruct(), p, renderer->getOverlayObjects());
  }
}

void MapWidget::onIconFound(QPoint /*lookupCoord*/, MapIcon icon)
{
  if (icon.iconStyle->IsVisible()) {
    if (!icon.iconStyle->GetIconName().empty()) {
      qDebug() << "Object:" << QString::fromStdString(icon.objectRef.GetName())
               << "icon:" << QString::fromStdString(icon.iconStyle->GetIconName())
               << "name:" << icon.name;
    } else {
      assert(icon.iconStyle->GetSymbol());
      qDebug() << "Object:" << QString::fromStdString(icon.objectRef.GetName())
               << "symbol:" << QString::fromStdString(icon.iconStyle->GetSymbol()->GetName())
               << "name:" << icon.name;
    }
  }

  iconAnimation.activate(icon);

  emit iconTapped(icon.screenCoord, icon.coord.GetLat(), icon.coord.GetLon(), icon.databasePath,
                  QString(icon.objectRef.GetTypeName()), icon.objectRef.GetFileOffset(), icon.poiId,
                  icon.type, icon.name, icon.altName, icon.ref, icon.operatorName, icon.phone, icon.website,
                  icon.openingHours);
}

void MapWidget::onDoubleTap(const QPoint p)
{
    zoomIn(2.0, p);
    osmscout::GeoCoord coord;
    getProjection().PixelToGeo(p.x(), p.y(),
                               coord);
    emit doubleTap(p.x(), p.y(), coord.GetLat(), coord.GetLon());
}

void MapWidget::onLongTap(const QPoint p)
{
  osmscout::GeoCoord coord;
    getProjection().PixelToGeo(p.x(), p.y(), coord);
    emit longTap(p.x(), p.y(), coord.GetLat(), coord.GetLon());
}

void MapWidget::onTapAndDrag(const QPoint p)
{
    // discard current input handler
    DBThreadRef dbThread = OSMScoutQt::GetInstance().GetDBThread();
    setupInputHandler(new ZoomGestureHandler(*view, p, dbThread->GetPhysicalDpi()));
}

void MapWidget::onTapLongTap(const QPoint p)
{
    // discard current input handler
    DBThreadRef dbThread = OSMScoutQt::GetInstance().GetDBThread();
    setupInputHandler(new ZoomGestureHandler(*view, p, dbThread->GetPhysicalDpi()));

    osmscout::GeoCoord coord;
    getProjection().PixelToGeo(p.x(), p.y(),
                               coord);
    emit tapLongTap(p.x(), p.y(), coord.GetLat(), coord.GetLon());
}

void MapWidget::onMapDPIChange(double dpi)
{
    assert(QThread::currentThread() == this->thread());

    MapView v = *view;
    v.mapDpi = dpi;
    changeView(v);

    loadVehicleIcons();

    // discard current input handler
    setupInputHandler(new InputHandler(*view));
}

void MapWidget::onResize()
{
    inputHandler->widgetResized(QSizeF(width(), height()));
}

MapViewStruct MapWidget::GetViewStruct() const
{
  MapViewStruct result;
  QRectF boundingBox = contentsBoundingRect();

  result.coord = view->center;
  result.angle = view->angle;
  result.magnification = view->magnification;
  result.width = boundingBox.width();
  result.height = boundingBox.height();
  result.dpi = view->mapDpi;
  return result;
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

    inputHandler->vehiclePosition(*vehicle.position, vehicle.autoRotateMap);
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
  }else{
    qWarning() << "Cannot load icon" << iconPath;
  }
  return image;
}


void MapWidget::loadVehicleIcons()
{
  double iconPixelSize=getProjection().ConvertWidthToPixel(vehicle.iconSize * vehicleScaleFactor);
  QString iconDirectory=OSMScoutQt::GetInstance().GetIconDirectory();

  vehicle.standardIcon=loadSVGIcon(iconDirectory, vehicle.standardIconFile, iconPixelSize);
  vehicle.noGpsSignalIcon=loadSVGIcon(iconDirectory, vehicle.noGpsSignalIconFile, iconPixelSize);
  vehicle.inTunnelIcon=loadSVGIcon(iconDirectory, vehicle.inTunnelIconFile, iconPixelSize);
}

void MapWidget::setInteractiveIcons(bool b)
{
  if (iconLookup!=nullptr && !b) {
    iconLookup->deleteLater();
    iconLookup = nullptr;
    return;
  }
  if (iconLookup==nullptr && b) {
    iconLookup=OSMScoutQt::GetInstance().MakeIconLookup();
    connect(iconLookup, &IconLookup::iconFound,
            this, &MapWidget::onIconFound,
            Qt::QueuedConnection);
  }
}

QString MapWidget::GetStylesheetFilename() const
{
    DBThreadRef dbThread=OSMScoutQt::GetInstance().GetDBThread();
    return QString::fromStdString(dbThread->GetStylesheetFilename());
}

QString MapWidget::GetZoomLevelName() const
{
    osmscout::MagnificationConverter converter;
    std::string                      name;

    if (converter.Convert(osmscout::MagnificationLevel(view->magnification.GetLevel()),
                          name)) {
      return QString::fromStdString(name);
    }

    return "";
}
bool MapWidget::stylesheetHasErrors() const
{
    DBThreadRef dbThread=OSMScoutQt::GetInstance().GetDBThread();
    return !dbThread->GetStyleErrors().empty();
}
int MapWidget::firstStylesheetErrorLine() const
{
    DBThreadRef dbThread=OSMScoutQt::GetInstance().GetDBThread();
    std::list<StyleError> errors=dbThread->GetStyleErrors();
    if (errors.empty()) {
      return -1;
    }
    return errors.front().GetLine();
}
int MapWidget::firstStylesheetErrorColumn() const
{
    DBThreadRef dbThread=OSMScoutQt::GetInstance().GetDBThread();
    std::list<StyleError> errors=dbThread->GetStyleErrors();
    if (errors.empty()) {
      return -1;
    }
    return errors.front().GetColumn();
}
QString MapWidget::firstStylesheetErrorDescription() const
{
    DBThreadRef dbThread=OSMScoutQt::GetInstance().GetDBThread();
    std::list<StyleError> errors=dbThread->GetStyleErrors();
    if (errors.empty()) {
      return "";
    }
    return QString::fromStdString(errors.front().GetDescription());
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

void MapWidget::setVehicleScaleFactor(float factor)
{
  vehicleScaleFactor = factor;
  loadVehicleIcons();
  redraw();
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

    setupRenderer();

    for (auto &p:overlayWays){
      renderer->addOverlayObject(p.first, p.second);
    }

    emit renderingTypeChanged(GetRenderingType());
  }
}

void MapWidget::FlushCaches(const std::chrono::milliseconds &idleMs)
{
  assert(QThread::currentThread() == this->thread());
  renderer->FlushVisualCaches(idleMs);
}
}
