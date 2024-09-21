/*
  This source is part of the libosmscout-map library
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

#include <cstdlib>

#include <QDebug>
#include <QPoint>
#include <QTime>
#include <QElapsedTimer>

#include <osmscoutclientqt/InputHandler.h>
#include <osmscoutclientqt/OSMTile.h>

#include <osmscout/projection/MercatorProjection.h>

#include <osmscout/system/Math.h>

namespace osmscout {

void TapRecognizer::onTimeout()
{
    switch(state){
    case PRESSED:
        state = INACTIVE;
        emit longTap(QPoint(startX, startY));
        break;
    case RELEASED:
        state = INACTIVE;
        emit tap(QPoint(startX, startY));
        break;
    case PRESSED2:
        state = INACTIVE;
        emit tapLongTap(QPoint(startX, startY));
        break;
    case INACTIVE:
    default:
        state = INACTIVE;
        break;
    }
}

void TapRecognizer::touch(const QTouchEvent &event)
{
    // discard on multi touch
    if (event.touchPoints().size() != 1){
        state = INACTIVE;
        return;
    }

    QTouchEvent::TouchPoint finger = event.touchPoints()[0];
    Qt::TouchPointStates fingerState(finger.state());
    bool released = fingerState.testFlag(Qt::TouchPointReleased);
    bool pressed = fingerState.testFlag(Qt::TouchPointPressed);
    int fingerId = finger.id();
    int x = finger.pos().x();
    int y = finger.pos().y();

    // discard when PRESSED and registered another finger
    if ((state == PRESSED || state == PRESSED2) && (fingerId != startFingerId)){
        state = INACTIVE;
        return;
    }
    // where there is significant movement...
    if (std::max(std::abs(x - startX), std::abs(y - startY)) > moveTolerance) {
        // discard during first press
        if (state == PRESSED) {
            state = INACTIVE;
            return;
        }
        // emit tap and drag immediately
        if (state == PRESSED2) {
            state = INACTIVE;
            emit tapAndDrag(QPoint(startX, startY));
            return;
        }
    }
    // second touch with too big distance
    if (state == RELEASED && std::max(std::abs(x - startX), std::abs(y - startY)) > moveTolerance){
        state = INACTIVE;
        return;
    }
    if ((!pressed) && (!released))
        return;

    timer.stop();

    switch(state){
    case INACTIVE:
        if (pressed){
            startFingerId = fingerId;
            startX = x;
            startY = y;
            state = PRESSED;
            timer.setInterval(holdIntervalMs);
            timer.start();
        }
        break;
    case PRESSED:
        if (released){
            state = RELEASED;
            timer.setInterval(tapIntervalMs);
            timer.start();
        }
        break;
    case RELEASED:
        if (pressed){
            startFingerId = fingerId;
            startX = x;
            startY = y;
            state = PRESSED2;
            timer.setInterval(hold2IntervalMs);
            timer.start();
        }
        break;
    case PRESSED2:
        if (released){
            state = INACTIVE;
            emit doubleTap(QPoint(startX, startY));
        }
        break;
    default:
        state = INACTIVE;
        break;
    }
}

MoveAccumulator& MoveAccumulator::operator+=(const QPointF p)
{
    AccumulatorEvent ev = {p, QElapsedTimer()};
    ev.timer.start();
    events.push_back(ev);
    // flush old events
    while ((!events.isEmpty()) && events.first().timer.elapsed() > memory){
        events.pop_front();
    }
    return *this;
}

QVector2D MoveAccumulator::collect()
{
    QVector2D vector;
    double distance = 0;
    // flush old events
    while ((!events.isEmpty()) && events.first().timer.elapsed() > memory){
        events.pop_front();
    }
    if (!events.isEmpty()){
        AccumulatorEvent lastEv = events.first();
        events.pop_front();
        while (!events.isEmpty()){
            AccumulatorEvent currentEv = events.first();
            events.pop_front();
            QVector2D lastVect(
                        currentEv.pos.x() - lastEv.pos.x(),
                        currentEv.pos.y() - lastEv.pos.y()
                    );
            double len = lastVect.length();
            if (len > vectorLengthTreshold || vector.isNull()){
                vector = lastVect;
            }
            distance += len;
            lastEv = currentEv;
        }
    }
    vector.normalize();
    return QVector2D(
            vector.x() * -1 * distance * factor,
            vector.y() * -1 * distance * factor
            );
}

InputHandler::InputHandler(const MapView &view): view(view)
{
}

void InputHandler::painted()
{
    // noop
}
bool InputHandler::animationInProgress()
{
    return false;
}
bool InputHandler::showCoordinates(const osmscout::GeoCoord &/*coord*/, const osmscout::Magnification &/*magnification*/, const osmscout::Bearing &/*bearing*/)
{
    return false;
}
bool InputHandler::zoom(double /*zoomFactor*/, const QPoint &/*widgetPosition*/, const QRect &/*widgetDimension*/)
{
    return false;
}

bool InputHandler::move(const QVector2D &/*move*/)
{
    return false;
}
bool InputHandler::rotateTo(double /*angle*/)
{
    return false;
}
bool InputHandler::rotateBy(double /*angleChange*/)
{
    return false;
}
bool InputHandler::pivotBy(double /*angleChange*/)
{
  return false;
}
bool InputHandler::touch(const QTouchEvent &/*event*/)
{
    return false;
}
bool InputHandler::currentPosition(bool /*locationValid*/, osmscout::GeoCoord /*currentPosition*/)
{
    return false;
}
bool InputHandler::vehiclePosition(const VehiclePosition& /*vehiclePosition*/, bool /*autoRotateMap*/)
{
    return false;
}
bool InputHandler::isLockedToPosition()
{
    return false;
}
bool InputHandler::isFollowVehicle()
{
    return false;
}
bool InputHandler::focusOutEvent(QFocusEvent* /*event*/)
{
    return false;
}
void InputHandler::widgetResized(const QSizeF &/*widgetSize*/)
{
  // no code
}

MoveHandler::MoveHandler(const MapView &view): InputHandler(view)
{
    connect(&timer, &QTimer::timeout, this, &MoveHandler::onTimeout);
    timer.setSingleShot(false);
}

bool MoveHandler::touch(const QTouchEvent &event)
{
    // move handler consumes finger release
    if (event.touchPoints().size() == 1){
        QTouchEvent::TouchPoint finger = event.touchPoints()[0];
        Qt::TouchPointStates state(finger.state());
        return state.testFlag(Qt::TouchPointReleased);
    }
    return false;
}

void MoveHandler::onTimeout()
{
    double progress = (double)(animationStart.elapsed() + ANIMATION_TICK) / (double)animationDuration;
    if (progress >= 1){
        progress = 1.0;
        timer.stop();
    }
    double scale = progress;
    if (!linearProgression) {
      //scale = std::log( progress * (M_E - 1) + 1);
      scale = std::log10( progress * (10 - 1) + 1);
    }

    osmscout::MercatorProjection projection;

    //qDebug() << "move: " << QString::fromStdString(view.center.GetDisplayText()) << "   by: " << move;
    double startMag = startMapView.magnification.GetMagnification();
    double targetMag = targetMagnification.GetMagnification();

    double startAngle = startMapView.angle.AsRadians();
    double finalAngle = startAngle + ((targetAngle-startAngle) * scale);

    if (finalAngle > 2*M_PI) {
        finalAngle = fmod(finalAngle, 2*M_PI);
    }

    if (finalAngle < 0) {
        finalAngle = 2*M_PI + fmod(finalAngle, 2*M_PI);
    }

    if (!projection.Set(startMapView.center,
                        finalAngle,
                        osmscout::Magnification(startMag + ((targetMag - startMag) * scale) ),
                        startMapView.mapDpi, 1000, 1000)) {
        return;
    }

    if (!projection.Move(_move.x() * scale, _move.y() * scale * -1.0)) {
        return;
    }

    view.magnification = projection.GetMagnification();
    view.center=projection.GetCenter();
    view.angle=Bearing::Radians(projection.GetAngle());
    if (view.center.GetLon() < OSMTile::minLon()){
        view.center.Set(view.center.GetLat(),OSMTile::minLon());
    }else if (view.center.GetLon() > OSMTile::maxLon()){
        view.center.Set(view.center.GetLat(),OSMTile::maxLon());
    }
    if (view.center.GetLat() > OSMTile::maxLat()){
        view.center.Set(OSMTile::maxLat(),view.center.GetLon());
    }else if (view.center.GetLat() < OSMTile::minLat()){
        view.center.Set(OSMTile::minLat(),view.center.GetLon());
    }

    emit viewChanged(view);
}

bool MoveHandler::animationInProgress()
{
    return timer.isActive();
}

bool MoveHandler::zoom(double zoomFactor, const QPoint &widgetPosition, const QRect &widgetDimension)
{
    startMapView = view;
    targetAngle = view.GetAngle();
    // compute event distance from center
    QPoint distance = widgetPosition;
    distance -= QPoint(widgetDimension.width() / 2, widgetDimension.height() / 2);
    if (zoomFactor > 1){
        _move.setX(distance.x() * (zoomFactor -1));
        _move.setY(distance.y() * (zoomFactor -1));
    }else{
        // 0.75 is magic constant from experiments when map coordinate has same screen position while zoom-out
        _move.setX(distance.x() * -0.75 * ((1/zoomFactor) -1));
        _move.setY(distance.y() * -0.75 * ((1/zoomFactor) -1));
    }

    targetMagnification = view.magnification;
    osmscout::Magnification maxMag(osmscout::MagnificationLevel(20));

    if (zoomFactor > 1){ // zoom in
        if (targetMagnification.GetMagnification()*zoomFactor>maxMag.GetMagnification()) {
            targetMagnification.SetMagnification(maxMag.GetMagnification());
        }
        else {
            targetMagnification.SetMagnification(targetMagnification.GetMagnification()*zoomFactor);
        }
    }else{ // zoom out
        if (targetMagnification.GetMagnification()*zoomFactor<1) {
            targetMagnification.SetMagnification(1);
        }
        else {
            targetMagnification.SetMagnification(targetMagnification.GetMagnification()*zoomFactor);
        }
    }
    //emit viewChanged(view);
    linearProgression = false;
    animationDuration = ZOOM_ANIMATION_DURATION;
    animationStart.restart();
    timer.setInterval(ANIMATION_TICK);
    timer.start();
    onTimeout();

    return true;
}

bool MoveHandler::move(const QVector2D &move)
{
    startMapView = view;
    targetMagnification = view.magnification;
    targetAngle = view.GetAngle();

    _move.setX(move.x());
    _move.setY(move.y());

    linearProgression = false;
    animationDuration = MOVE_ANIMATION_DURATION;
    animationStart.restart();
    timer.setInterval(ANIMATION_TICK);
    timer.start();
    onTimeout();

    return true;
}

bool MoveHandler::moveNow(const QVector2D &move)
{
    osmscout::MercatorProjection projection;

    //qDebug() << "move: " << QString::fromStdString(view.center.GetDisplayText()) << "   by: " << move;

    if (!projection.Set(view.center, view.angle.AsRadians(), view.magnification, view.mapDpi, 1000, 1000)) {
        return false;
    }

    if (!projection.Move(move.x(), move.y() * -1.0)) {
        return false;
    }

    view.center=projection.GetCenter();
    if (view.center.GetLon() < OSMTile::minLon()){
        view.center.Set(view.center.GetLat(),OSMTile::minLon());
    }else if (view.center.GetLon() > OSMTile::maxLon()){
        view.center.Set(view.center.GetLat(),OSMTile::maxLon());
    }
    if (view.center.GetLat() > OSMTile::maxLat()){
        view.center.Set(OSMTile::maxLat(),view.center.GetLon());
    }else if (view.center.GetLat() < OSMTile::minLat()){
        view.center.Set(OSMTile::minLat(),view.center.GetLon());
    }

    emit viewChanged(view);
    return true;
}

bool MoveHandler::rotateTo(double angle)
{
    startMapView = view;
    targetMagnification = view.magnification;

    targetAngle = angle;
    if (std::abs(targetAngle-view.angle.AsRadians())>M_PI){
        targetAngle+=2*M_PI;
    }

    _move.setX(0);
    _move.setY(0);

    linearProgression = false;
    animationDuration = ROTATE_ANIMATION_DURATION;
    animationStart.restart();
    timer.setInterval(ANIMATION_TICK);
    timer.start();
    onTimeout();

    return true;
}

bool MoveHandler::rotateBy(double angleChange)
{

    startMapView = view;
    targetMagnification = view.magnification;

    targetAngle = view.angle.AsRadians()+angleChange;

    _move.setX(0);
    _move.setY(0);

    linearProgression = false;
    animationDuration = ROTATE_ANIMATION_DURATION;
    animationStart.restart();
    timer.setInterval(ANIMATION_TICK);
    timer.start();
    onTimeout();

    return true;
}

bool MoveHandler::pivotBy(double angleChange)
{
  startMapView = view;
  targetMagnification = view.magnification;

  targetAngle = view.angle.AsRadians()+angleChange;

  _move.setX(0);
  _move.setY(0);

  linearProgression = true;
  animationDuration = ROTATE_ANIMATION_DURATION;
  animationStart.restart();
  timer.setInterval(ANIMATION_TICK);
  timer.start();
  onTimeout();

  return true;
}

ZoomGestureHandler::ZoomGestureHandler(const MapView &view, const QPoint &p, double zoomDistance):
    InputHandler(view),
  startMag(view.magnification),
  gestureStart(p),
  zoomDistance(zoomDistance)
{
  // qDebug() << "zoom gesture init:" << startMag.GetMagnification();
}

bool ZoomGestureHandler::touch(const QTouchEvent &event)
{
  if (event.touchPoints().size() != 1) {
    return false;
  }

  QTouchEvent::TouchPoint finger = event.touchPoints()[0];
  Qt::TouchPointStates state(finger.state());
  if (state.testFlag(Qt::TouchPointReleased)){
    return false;
  }

  double zoom = (finger.pos().y() - double(gestureStart.y())) / zoomDistance;
  if (zoom < 0){
    zoom = 1.0/(1.0+abs(zoom));
  } else {
    zoom = 1.0+zoom;
  }

  Magnification maxMag(osmscout::MagnificationLevel(20));
  if (startMag.GetMagnification() * zoom > maxMag.GetMagnification()) {
    view.magnification = maxMag;
  } else if (startMag.GetMagnification() * zoom < 1) {
    view.magnification.SetMagnification(1);
  } else {
    view.magnification.SetMagnification(startMag.GetMagnification() * zoom);
  }
  // qDebug() << "zoom gesture:" << zoom << "(" << double(gestureStart.y()) << finger.pos().y() << ")"
  //          << startMag.GetMagnification() << "->" << view.magnification.GetMagnification();
  emit viewChanged(view);
  return true;
}

JumpHandler::JumpHandler(const MapView &view,
    double moveAnimationDuration,
    double zoomAnimationDuration):
    InputHandler(view),
    moveAnimationDuration(moveAnimationDuration),
    zoomAnimationDuration(zoomAnimationDuration)
{
    connect(&timer, &QTimer::timeout, this, &JumpHandler::onTimeout);
    timer.setSingleShot(false);
}

void JumpHandler::onTimeout()
{
    double progress = (double)(animationStart.elapsed() + ANIMATION_TICK) / moveAnimationDuration;
    double zoomProgress = (double)(animationStart.elapsed() + ANIMATION_TICK) / zoomAnimationDuration;
    if (progress >= 1){
      progress = 1;
    }
    if (zoomProgress >= 1){
      zoomProgress = 1;
    }
    if (progress >= 1 && zoomProgress >= 1){
      timer.stop();
    }
    double magScale = zoomProgress; // std::log10( progress * (10 - 1) + 1);
    double angleScale = progress;
    double positionScale = std::log( progress * (M_E - 1) + 1);

    double startMag = startMapView.magnification.GetMagnification();
    double targetMag = targetMapView.magnification.GetMagnification();
    view.magnification = osmscout::Magnification(startMag + ((targetMag - startMag) * magScale));

    double startLat = startMapView.center.GetLat();
    double targetLat = targetMapView.center.GetLat();
    double lat = startLat + ((targetLat - startLat) * positionScale);

    double startLon = startMapView.center.GetLon();
    double targetLon = targetMapView.center.GetLon();
    double lon = startLon + ((targetLon - startLon) * positionScale);

    view.center.Set(lat, lon);

    double startAngle = startMapView.angle.AsRadians();
    view.angle = Bearing::Radians(startAngle + (angleDiff * angleScale));

    emit viewChanged(view);
}
bool JumpHandler::animationInProgress()
{
    return timer.isActive();
}

bool JumpHandler::showCoordinates(const osmscout::GeoCoord &coord, const osmscout::Magnification &magnification, const osmscout::Bearing &bearing)
{
    startMapView = view;
    targetMapView = MapView(coord, bearing, magnification, view.mapDpi);
    angleDiff = (bearing - startMapView.angle).AsRadians();
    if (angleDiff > M_PI){
      angleDiff -= 2*M_PI;
    }

    animationStart.restart();
    timer.setInterval(ANIMATION_TICK);
    timer.start();
    onTimeout();

    return true;
}

DragHandler::DragHandler(const MapView &view):
        MoveHandler(view), moving(true), startView(view), fingerId(-1),
        startX(-1), startY(-1), ended(false)
{
}

bool DragHandler::touch(const QTouchEvent &event)
{
    if (ended)
        return false;

    if (event.touchPoints().size() != 1)
        return false;

    QTouchEvent::TouchPoint finger = event.touchPoints()[0];
    Qt::TouchPointStates state(finger.state());

    moving = !state.testFlag(Qt::TouchPointReleased);

    if (startX < 0){ // first touch by this point
        if (!state.testFlag(Qt::TouchPointReleased)){
            startX = finger.pos().x();
            startY = finger.pos().y();
            fingerId = finger.id();
        }
    }else{
        if (fingerId != finger.id())
            return false; // should not happen

        view = startView;
        MoveHandler::moveNow(QVector2D(
            startX - finger.pos().x(),
            startY - finger.pos().y()
        ));
    }
    moveAccumulator += finger.pos();
    if (state.testFlag(Qt::TouchPointReleased)){
        MoveHandler::move(moveAccumulator.collect());
        ended = true;
    }
    return true;
}

bool DragHandler::zoom(double /*zoomFactor*/, const QPoint &/*widgetPosition*/, const QRect &/*widgetDimension*/)
{
    return false;
        // TODO: finger on screen and zoom
        // => compute geo point under finger, change magnification and then update startView
}

bool DragHandler::move(const QVector2D &/*move*/)
{
    return false; // finger on screen discard move
}
bool DragHandler::rotateBy(double /*angleChange*/)
{
    return false; // finger on screen discard rotation ... TODO like zoom
}
bool DragHandler::animationInProgress()
{
    return moving || MoveHandler::animationInProgress();
}


MultitouchHandler::MultitouchHandler(const MapView &view):
    MoveHandler(view), moving(true), startView(view), initialized(false), ended(false)
{
}

bool MultitouchHandler::animationInProgress()
{
    return moving || MoveHandler::animationInProgress();
}
bool MultitouchHandler::zoom(double /*zoomFactor*/, const QPoint &/*widgetPosition*/, const QRect &/*widgetDimension*/)
{
    return false;
}
bool MultitouchHandler::move(const QVector2D &/*vector*/)
{
    return false;
}
bool MultitouchHandler::rotateBy(double /*angleChange*/)
{
    return false;
}
bool MultitouchHandler::touch(const QTouchEvent &event)
{
    if (ended)
        return false;

    if (!initialized){
        QList<QTouchEvent::TouchPoint> valid;
        QListIterator<QTouchEvent::TouchPoint> it(event.touchPoints());
        while (it.hasNext() && (valid.size() < 2)){
            QTouchEvent::TouchPoint tp = it.next();
            Qt::TouchPointStates state(tp.state());
            if (!state.testFlag(Qt::TouchPointReleased)){
                valid << tp;
            }
        }
        if (valid.size() >= 2){
            startPointA = valid[0];
            startPointB = valid[1];
            initialized = true;
            return true;
        }
    }else{
        QTouchEvent::TouchPoint currentA;
        QTouchEvent::TouchPoint currentB;
        int assigned = 0;

        QListIterator<QTouchEvent::TouchPoint> it(event.touchPoints());
        while (it.hasNext() && (assigned < 2)){
            QTouchEvent::TouchPoint tp = it.next();
            if (tp.id() == startPointA.id()){
                currentA = tp;
                assigned++;
            }
            if (tp.id() == startPointB.id()){
                currentB = tp;
                assigned++;
            }
        }
        if (assigned == 2){
            view = startView;

            // move
            QPointF startCenter(
                (startPointA.pos().x() + startPointB.pos().x()) / 2,
                (startPointA.pos().y() + startPointB.pos().y()) / 2
            );
            QPointF currentCenter(
                (currentA.pos().x() + currentB.pos().x()) / 2,
                (currentA.pos().y() + currentB.pos().y()) / 2
            );

            QVector2D move(
                startCenter.x() - currentCenter.x(),
                startCenter.y() - currentCenter.y()
            );
            MoveHandler::moveNow(move);
            moveAccumulator += currentCenter;

            // zoom
            QVector2D startVector(startPointA.pos() - startPointB.pos());
            QVector2D currentVector(currentA.pos() - currentB.pos());
            double scale = 1;
            if (startVector.length() > 0){

                scale = currentVector.length() / startVector.length();

              osmscout::Magnification minMag(MagnificationLevel(0));
                osmscout::Magnification maxMag(MagnificationLevel(20));

                if (view.magnification.GetMagnification()*scale>maxMag.GetMagnification()) {
                    view.magnification.SetMagnification(maxMag.GetMagnification());
                }else {
                    if (view.magnification.GetMagnification()*scale<minMag.GetMagnification()) {
                        view.magnification.SetMagnification(minMag.GetMagnification());
                    }else {
                        view.magnification.SetMagnification(view.magnification.GetMagnification()*scale);
                    }
                }
            }

            if (move.length() > 10 || scale > 1.1 || scale < 0.9){
                startPointA = currentA;
                startPointB = currentB;
                startView = view;
            }

            emit viewChanged(view);
            return true;
        }
    }
    moving = false;
    ended = true;
    MoveHandler::move(moveAccumulator.collect());
    return true;
}

bool LockHandler::currentPosition(bool locationValid, osmscout::GeoCoord currentPosition)
{
    if (locationValid){
        osmscout::MercatorProjection projection;

        if (!projection.Set(view.center, view.magnification, view.mapDpi, window.width(), window.height())) {
            return false;
        }

        Vertex2D screenPos;
        projection.GeoToPixel(currentPosition,
                              screenPos);
        double distanceFromCenter = sqrt(pow(std::abs(projection.GetWidth()/2 - screenPos.GetX()), 2) +
                                         pow(std::abs(projection.GetHeight()/2 - screenPos.GetY()), 2));

        double moveTolerance = std::min(window.width(), window.height()) / 4;
        if (distanceFromCenter > moveTolerance){
            JumpHandler::showCoordinates(currentPosition, view.magnification, view.angle);
        }
    }
    return true;
}

bool LockHandler::showCoordinates(const osmscout::GeoCoord &/*coord*/, const osmscout::Magnification &/*magnification*/, const osmscout::Bearing &/*bearing*/){
    return false; // lock handler can't handle it, we are locked on "currentPosition"
}

bool LockHandler::isLockedToPosition()
{
    return true;
}
bool LockHandler::focusOutEvent(QFocusEvent* /*event*/)
{
    return true;
}
void LockHandler::widgetResized(const QSizeF &widgetSize)
{
    window=widgetSize;
}

VehicleFollowHandler::VehicleFollowHandler(const MapView &view, const QSizeF &widgetSize):
    JumpHandler(view), window(widgetSize)
{}

bool VehicleFollowHandler::vehiclePosition(const VehiclePosition &vehiclePosition, bool autoRotateMap)
{
  Bearing mapBearing = vehiclePosition.getBearing() && autoRotateMap ? *(vehiclePosition.getBearing()) : osmscout::Bearing();
  log.Debug() << "map bearing: " << mapBearing.LongDisplayString();
  // clockwise to counterclockwise (car bearing to canvas rotation)
  mapBearing = Bearing::Radians(2 * M_PI - mapBearing.AsRadians());

  Magnification magnification = view.magnification;
  if(vehiclePosition.getNextStepCoord()) {
    Distance nextStepDistance = GetSphericalDistance(vehiclePosition.getCoord(), *vehiclePosition.getNextStepCoord());

    double mag;
    if (nextStepDistance > Kilometers(4)) {
      mag = pow(2.0, 14); // Magnification::magClose;
    } else if (nextStepDistance > Meters(2000)) {
      mag = pow(2.0, 14.25);
    } else if (nextStepDistance > Meters(1000)) {
      mag = pow(2.0, 14.5);
    } else if (nextStepDistance > Meters(750)) {
      mag = pow(2.0, 14.75);
    } else if (nextStepDistance > Meters(500)) {
      mag = pow(2.0, 15); // Magnification::magCloser;
    } else if (nextStepDistance > Meters(300)) {
      mag = pow(2.0, 15.5);
    } else if (nextStepDistance > Meters(200)) {
      mag = pow(2.0, 16); // Magnification::magVeryClose;
    } else if (nextStepDistance > Meters(100)) {
      mag = pow(2.0, 17); // Magnification::magVeryClose;
    } else {
      mag = pow(2.0,18); // Magnification::magBlock
    }

    magnification.SetMagnification(mag);
    double factor = magnification.GetMagnification() / view.magnification.GetMagnification();
    if (factor > 1.05) {
      magnification.SetMagnification(view.magnification.GetMagnification() * 1.05);
    } else if (factor < 0.95) {
      magnification.SetMagnification(view.magnification.GetMagnification() * 0.95);
    }
  }

  osmscout::MercatorProjection projection;

  if (!projection.Set(vehiclePosition.getCoord(),
                      mapBearing.AsRadians(),
                      magnification,
                      view.mapDpi,
                      window.width(), window.height()
  )) {
    return false;
  }

  osmscout::GeoCoord coord;
  if (!projection.PixelToGeo(window.width()/2,
                             autoRotateMap ? window.height()/4 : window.height()/2,
                             coord)){
    return false;
  }

  return JumpHandler::showCoordinates(coord, magnification, mapBearing);
}
bool VehicleFollowHandler::isLockedToPosition()
{
  return true;
}
bool VehicleFollowHandler::isFollowVehicle()
{
  return true;
}
void VehicleFollowHandler::widgetResized(const QSizeF &widgetSize)
{
  window=widgetSize;
}
}
