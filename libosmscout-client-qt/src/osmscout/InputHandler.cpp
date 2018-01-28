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
#include <QVector>

#include <osmscout/InputHandler.h>
#include <osmscout/OSMTile.h>

#include <osmscout/util/Projection.h>

#include <osmscout/system/Math.h>

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

void TapRecognizer::touch(QTouchEvent *event)
{
    // discard on multi touch
    if (event->touchPoints().size() != 1){
        state = INACTIVE;
        return;
    }

    QTouchEvent::TouchPoint finger = event->touchPoints()[0];
    Qt::TouchPointStates fingerState(finger.state());
    bool released = fingerState.testFlag(Qt::TouchPointReleased);
    bool pressed = fingerState.testFlag(Qt::TouchPointPressed);
    int fingerId = finger.id();
    int x = finger.pos().x();
    int y = finger.pos().y();

    // discard when PRESSED and registered another finger or some bigger movement
    if ((state == PRESSED || state == PRESSED2) &&
            (fingerId != startFingerId || std::max(std::abs(x - startX), std::abs(y - startY)) > moveTolerance)){

        state = INACTIVE;
        return;
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
    AccumulatorEvent ev = {p, QTime()};
    ev.time.start();
    events.push_back(ev);
    // flush old events
    while ((!events.isEmpty()) && events.first().time.elapsed() > memory){
        events.pop_front();
    }
    return *this;
}

QVector2D MoveAccumulator::collect()
{
    QVector2D vector;
    double distance = 0;
    // flush old events
    while ((!events.isEmpty()) && events.first().time.elapsed() > memory){
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

InputHandler::InputHandler(MapView view): view(view)
{
}
InputHandler::~InputHandler()
{
    // noop
}
void InputHandler::painted()
{
    // noop
}
bool InputHandler::animationInProgress()
{
    return false;
}
bool InputHandler::showCoordinates(osmscout::GeoCoord /*coord*/, osmscout::Magnification /*magnification*/)
{
    return false;
}
bool InputHandler::zoom(double /*zoomFactor*/, const QPoint /*widgetPosition*/, const QRect /*widgetDimension*/)
{
    return false;
}

bool InputHandler::move(QVector2D /*move*/)
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
bool InputHandler::touch(QTouchEvent* /*event*/)
{
    return false;
}
bool InputHandler::currentPosition(bool /*locationValid*/, osmscout::GeoCoord /*currentPosition*/, double /*moveTolerance*/)
{
    return false;
}
bool InputHandler::isLockedToPosition()
{
    return false;
}
bool InputHandler::focusOutEvent(QFocusEvent* /*event*/)
{
    return false;
}

MoveHandler::MoveHandler(MapView view): InputHandler(view)
{
    connect(&timer, SIGNAL(timeout()), this, SLOT(onTimeout()));
    timer.setSingleShot(false);
}

MoveHandler::~MoveHandler()
{
    // noop
}
bool MoveHandler::touch(QTouchEvent *event)
{
    // move handler consumes finger release
    if (event->touchPoints().size() == 1){
        QTouchEvent::TouchPoint finger = event->touchPoints()[0];
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
    //double scale = std::log( progress * (M_E - 1) + 1);
    double scale = std::log10( progress * (10 - 1) + 1);

    osmscout::MercatorProjection projection;

    //qDebug() << "move: " << QString::fromStdString(view.center.GetDisplayText()) << "   by: " << move;
    double startMag = startMapView.magnification.GetMagnification();
    double targetMag = targetMagnification.GetMagnification();

    double startAngle = startMapView.angle;
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
    view.angle=projection.GetAngle();
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

bool MoveHandler::zoom(double zoomFactor, const QPoint widgetPosition, const QRect widgetDimension)
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
    osmscout::Magnification maxMag;
    maxMag.SetLevel(20);
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
    animationDuration = ZOOM_ANIMATION_DURATION;
    animationStart.restart();
    timer.setInterval(ANIMATION_TICK);
    timer.start();
    onTimeout();

    return true;
}

bool MoveHandler::move(QVector2D move)
{
    startMapView = view;
    targetMagnification = view.magnification;
    targetAngle = view.GetAngle();

    _move.setX(move.x());
    _move.setY(move.y());

    animationDuration = MOVE_ANIMATION_DURATION;
    animationStart.restart();
    timer.setInterval(ANIMATION_TICK);
    timer.start();
    onTimeout();

    return true;
}

bool MoveHandler::moveNow(QVector2D move)
{
    osmscout::MercatorProjection projection;

    //qDebug() << "move: " << QString::fromStdString(view.center.GetDisplayText()) << "   by: " << move;

    if (!projection.Set(view.center, view.angle, view.magnification, view.mapDpi, 1000, 1000)) {
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
    if (std::abs(targetAngle-view.angle)>M_PI){
        targetAngle+=2*M_PI;
    }

    _move.setX(0);
    _move.setY(0);

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

    targetAngle = view.angle+angleChange;

    _move.setX(0);
    _move.setY(0);

    animationDuration = ROTATE_ANIMATION_DURATION;
    animationStart.restart();
    timer.setInterval(ANIMATION_TICK);
    timer.start();
    onTimeout();

    return true;
}


JumpHandler::JumpHandler(MapView view):
    InputHandler(view)
{
    connect(&timer, SIGNAL(timeout()), this, SLOT(onTimeout()));
    timer.setSingleShot(false);
}

JumpHandler::~JumpHandler()
{
    // noop
}
void JumpHandler::onTimeout()
{
    double progress = (double)(animationStart.elapsed() + ANIMATION_TICK) / (double)ANIMATION_DURATION;
    if (progress >= 1){
        progress = 1.0;
        timer.stop();
    }
    double magScale = progress; // std::log10( progress * (10 - 1) + 1);
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

    emit viewChanged(view);
}
bool JumpHandler::animationInProgress()
{
    return timer.isActive();
}

bool JumpHandler::showCoordinates(osmscout::GeoCoord coord, osmscout::Magnification magnification)
{
    startMapView = view;
    targetMapView = MapView(coord, view.angle, magnification, view.mapDpi);

    animationStart.restart();
    timer.setInterval(ANIMATION_TICK);
    timer.start();
    onTimeout();

    return true;
}

DragHandler::DragHandler(MapView view):
        MoveHandler(view), moving(true), startView(view), fingerId(-1),
        startX(-1), startY(-1), ended(false)
{
}
DragHandler::~DragHandler()
{
}
bool DragHandler::touch(QTouchEvent *event)
{
    if (ended)
        return false;

    if (event->touchPoints().size() != 1)
        return false;

    QTouchEvent::TouchPoint finger = event->touchPoints()[0];
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

bool DragHandler::zoom(double /*zoomFactor*/, const QPoint /*widgetPosition*/, const QRect /*widgetDimension*/)
{
    return false;
        // TODO: finger on screen and zoom
        // => compute geo point under finger, change magnification and then update startView
}

bool DragHandler::move(QVector2D /*move*/)
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


MultitouchHandler::MultitouchHandler(MapView view):
    MoveHandler(view), moving(true), startView(view), initialized(false), ended(false)
{
}

MultitouchHandler::~MultitouchHandler()
{

}
bool MultitouchHandler::animationInProgress()
{
    return moving || MoveHandler::animationInProgress();
}
bool MultitouchHandler::zoom(double /*zoomFactor*/, const QPoint /*widgetPosition*/, const QRect /*widgetDimension*/)
{
    return false;
}
bool MultitouchHandler::move(QVector2D /*vector*/)
{
    return false;
}
bool MultitouchHandler::rotateBy(double /*angleChange*/)
{
    return false;
}
bool MultitouchHandler::touch(QTouchEvent *event)
{
    if (ended)
        return false;

    if (!initialized){
        QList<QTouchEvent::TouchPoint> valid;
        QListIterator<QTouchEvent::TouchPoint> it(event->touchPoints());
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

        QListIterator<QTouchEvent::TouchPoint> it(event->touchPoints());
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

                osmscout::Magnification maxMag;
                osmscout::Magnification minMag;
                maxMag.SetLevel(20);
                minMag.SetLevel(0);

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

bool LockHandler::currentPosition(bool locationValid, osmscout::GeoCoord currentPosition, double moveTolerance)
{
    if (locationValid){
        osmscout::MercatorProjection projection;

        if (!projection.Set(view.center, view.magnification, view.mapDpi, 1000, 1000)) {
            return false;
        }

        double x;
        double y;
        projection.GeoToPixel(currentPosition, x, y);
        double distanceFromCenter = sqrt(pow(std::abs(500.0 - x), 2) + pow(std::abs(500.0 - y), 2));
        if (distanceFromCenter > moveTolerance){
            JumpHandler::showCoordinates(currentPosition, view.magnification);
        }
    }
    return true;
}

bool LockHandler::showCoordinates(osmscout::GeoCoord /*coord*/, osmscout::Magnification /*magnification*/){
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
