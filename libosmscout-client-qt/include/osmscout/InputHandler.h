#ifndef OSMSCOUT_CLIENT_QT_INPUTHANDLER_H
#define OSMSCOUT_CLIENT_QT_INPUTHANDLER_H

/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2016  Lukas Karas

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

#include <QObject>
#include <QVector2D>
#include <QTouchEvent>
#include <QTimer>
#include <QTime>
#include <QQueue>

#include <osmscout/util/Bearing.h>
#include <osmscout/util/GeoBox.h>
#include <osmscout/util/Magnification.h>

#include <osmscout/ClientQtImportExport.h>
#include <osmscout/VehiclePosition.h>

namespace osmscout {

/**
 * \ingroup QtAPI
 *
 * Simple class for recognizing some basic gestures: tap, double tap, long-tap and tap-and-hold.
 *
 * Widget should send own \ref QTouchEvent to this object, when some tap gesture
 * is recognized it emits one of its signals.
 *
 *  - **Tap**: touch shorter than holdInterval (1000 ms) followed with pause longer than tapInterval (200 ms)
 *  - **Long tap**: touch longer than holdInterval (1000 ms)
 *  - **Double tap**: tap followed by second one within tapInterval (200 ms)
 *  - **Tap, long tap**: tap followed by long tap within tapInterval (200 ms)
 *
 * Physical DPI of display should be setup before first use. TapRecognizer use some small move
 * tolerance (~ 2.5 mm), it means that events are emited even that touch point is moving within
 * this tolerance. For double tap, when second tap is farther than this tolerance,
 * double-tap event is not emited.
 */
class OSMSCOUT_CLIENT_QT_API TapRecognizer : public QObject{
  Q_OBJECT

private:
  enum TapRecState{
    INACTIVE = 0,
    PRESSED = 1, // timer started with hold interval, if expired - long-tap is emited
    RELEASED = 2, // timer started with tap interval, if expired - tap is emited
    PRESSED2 = 3, // timer started with hold interval, if expired - tap-long-tap
  };

  int startFingerId;
  int startX;
  int startY;

  QTimer timer;
  TapRecState state;
  int holdIntervalMs;
  int hold2IntervalMs;
  int tapIntervalMs;
  int moveTolerance;

private slots:
  void onTimeout();

public:
  inline TapRecognizer():
          state(INACTIVE),
          holdIntervalMs(1000),
          hold2IntervalMs(500),
          tapIntervalMs(200),
          moveTolerance(15)
  {
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, this, &TapRecognizer::onTimeout);
  }

  virtual inline ~TapRecognizer()
  {
  }

  void touch(const QTouchEvent &event);

  inline void setPhysicalDpi(double physicalDpi)
  {
    moveTolerance = physicalDpi / 10.0; // ~ 2.5 mm
  }

signals:
  void tap(const QPoint p);
  void doubleTap(const QPoint p);
  void longTap(const QPoint p);
  void tapLongTap(const QPoint p);
};

/**
 * \ingroup QtAPI
 */
struct AccumulatorEvent
{
  QPointF pos;
  QTime time;
};

/**
 * \ingroup QtAPI
 *
 * Helper class that accumulates move (touch events) within some time period
 * (time defined FIFO queue). It helps to \ref MoveHandler determine of move
 * vector when drag gesture ends. It is used for animate move momentum.
 */
class MoveAccumulator : public QObject{
  Q_OBJECT

private:
  int memory; // ms
  QQueue<AccumulatorEvent> events;
  double factor;
  double vectorLengthTreshold;

public:

  /**
   *
   * @param memory - in milliseconds for finger position points
   * @param factor - momentum movement length (vector returned from collect method) will be equal to recorded length * factor
   * @param vectorLengthTreshold - movement (between two points) have to be longer than treshold (in pixels) for change move vector
   */
  inline MoveAccumulator(int memory = 100, double factor = 4, double vectorLengthTreshold = 5):
    memory(memory), factor(factor), vectorLengthTreshold(vectorLengthTreshold)
  {
  }
  virtual inline ~MoveAccumulator(){}

  MoveAccumulator& operator+=(const QPointF p);
  QVector2D collect();
};

/**
 * \ingroup QtAPI
 *
 * Object thats carry information about view center, angle and magnification.
 */
class OSMSCOUT_CLIENT_QT_API MapView: public QObject
{
  Q_OBJECT

  Q_PROPERTY(double   lat       READ GetLat       CONSTANT)
  Q_PROPERTY(double   lon       READ GetLon       CONSTANT)
  Q_PROPERTY(double   angle     READ GetAngle     CONSTANT)
  Q_PROPERTY(double   mag       READ GetMag       CONSTANT)
  Q_PROPERTY(uint32_t magLevel  READ GetMagLevel  CONSTANT)
  Q_PROPERTY(double   mapDpi    READ GetMapDpi    CONSTANT)

public:
  inline MapView(QObject *parent=0): QObject(parent) {}

  inline MapView(QObject *parent,
                 const osmscout::GeoCoord &center,
                 const Bearing &angle,
                 const osmscout::Magnification &magnification,
                 double mapDpi):
    QObject(parent), center(center), angle(angle), magnification(magnification), mapDpi(mapDpi) {}

  inline MapView(const osmscout::GeoCoord &center,
                 const Bearing &angle,
                 const osmscout::Magnification &magnification,
                 double mapDpi):
    center(center), angle(angle), magnification(magnification), mapDpi(mapDpi) {}

  /**
   * This copy constructor don't transfer ownership
   * in Qt hierarchy - it may cause troubles.
   * @param mv
   */
  inline MapView(const MapView &mv):
    QObject(), center(mv.center), angle(mv.angle), magnification(mv.magnification), mapDpi(mv.mapDpi) {}

  virtual inline ~MapView(){}

  inline double GetLat(){ return center.GetLat(); }
  inline double GetLon(){ return center.GetLon(); }
  inline double GetAngle(){ return angle.AsRadians(); }
  inline double GetMag(){ return magnification.GetMagnification(); }
  inline double GetMagLevel(){ return magnification.GetLevel(); }
  inline double GetMapDpi(){ return mapDpi; }

  inline bool IsValid(){ return mapDpi > 0; }

  void inline operator=(const MapView &mv)
  {
    center = mv.center;
    angle = mv.angle;
    magnification = mv.magnification;
    mapDpi = mv.mapDpi;
  }

  osmscout::GeoCoord           center;
  Bearing                      angle; // canvas clockwise
  osmscout::Magnification      magnification;
  double                       mapDpi{0};
};

inline bool operator==(const MapView& a, const MapView& b)
{
  return a.center == b.center && a.angle == b.angle && a.magnification == b.magnification && a.mapDpi == b.mapDpi;
}
inline bool operator!=(const MapView& a, const MapView& b)
{
  return ! (a == b);
}

/**
 * \ingroup QtAPI
 *
 * Input handler retrieve all inputs from user and may change MapView (emits viewChange signal).
 * If handler don't accept specific action, returns false. In such case,
 * default handler for this action should be activated.
 *
 * Input handlers is application of behaviour pattern. It solves problems like:
 *
 *  - what should happen when finger is on the screen and plus button is pressed
 *  - recognising multitouch gestures
 *
 * Qt provides api for register custom gesture recognizers, but it is not
 * available in QML world and its api don't fit to Map application requierements.
 *
 * Handler also controls map animations.
 */
class OSMSCOUT_CLIENT_QT_API InputHandler : public QObject{
    Q_OBJECT
public:
    InputHandler(const MapView &view);
    virtual ~InputHandler();

    virtual void painted();
    virtual bool animationInProgress();

    virtual bool showCoordinates(const osmscout::GeoCoord &coord, const osmscout::Magnification &magnification, const osmscout::Bearing &bearing);
    virtual bool zoom(double zoomFactor, const QPoint &widgetPosition, const QRect &widgetDimension);
    virtual bool move(const QVector2D &vector); // move vector in pixels
    virtual bool rotateTo(double angle);
    virtual bool rotateBy(double angleChange);
    virtual bool touch(const QTouchEvent &event);
    virtual bool currentPosition(bool locationValid, osmscout::GeoCoord currentPosition);
    virtual bool vehiclePosition(const VehiclePosition &vehiclePosition);
    virtual bool isLockedToPosition();
    virtual bool isFollowVehicle();
    virtual bool focusOutEvent(QFocusEvent *event);
    virtual void widgetResized(const QSizeF &widgetSize);

signals:
    void viewChanged(const MapView &view);

protected:
    MapView view;
};

/**
 * \ingroup QtAPI
 *
 * Handler with support of simple moves and zoom.
 * View changes are animated, so one action may emits many of viewChange signals.
 */
class OSMSCOUT_CLIENT_QT_API MoveHandler : public InputHandler {
    Q_OBJECT

private:
    QTime animationStart;
    QTimer timer;
    MapView startMapView;
    QVector2D _move;
    osmscout::Magnification targetMagnification;
    double targetAngle;
    int animationDuration;

    const int MOVE_ANIMATION_DURATION = 1000; // ms
    const int ZOOM_ANIMATION_DURATION = 500; // ms
    const int ROTATE_ANIMATION_DURATION = 1000; //ms
    const int ANIMATION_TICK = 16;

private slots:
    void onTimeout();

public:
    MoveHandler(const MapView &view);
    virtual ~MoveHandler();

    virtual bool animationInProgress() override;

    /**
     * Called from DragHandler or MultitouchHandler when gesture moves with map
     *
     * @param vector
     * @return
     */
    bool moveNow(const QVector2D &vector); // move vector in pixels, without animation

    virtual bool zoom(double zoomFactor, const QPoint &widgetPosition, const QRect &widgetDimension) override;
    virtual bool move(const QVector2D &vector) override; // move vector in pixels
    virtual bool rotateTo(double angle) override;
    virtual bool rotateBy(double angleChange) override;
    virtual bool touch(const QTouchEvent &event) override;
};

/**
 * \ingroup QtAPI
 *
 * Input handler that animates jumps to target map view.
 */
class OSMSCOUT_CLIENT_QT_API JumpHandler : public InputHandler {
    Q_OBJECT

private:
    QTime animationStart;
    QTimer timer;
    MapView startMapView;
    MapView targetMapView;
    double angleDiff; // radians

    double moveAnimationDuration;
    double zoomAnimationDuration;

    static constexpr int ANIMATION_DURATION = 1000; // ms
    static constexpr int ANIMATION_TICK = 16; // ms

private slots:
    void onTimeout();

public:
    JumpHandler(const MapView &view,
                double moveAnimationDuration = (double)ANIMATION_DURATION,
                double zoomAnimationDuration = (double)ANIMATION_DURATION);

    virtual ~JumpHandler();

    virtual bool animationInProgress() override;
    virtual bool showCoordinates(const osmscout::GeoCoord &coord, const osmscout::Magnification &magnification, const osmscout::Bearing &bearing) override;
};

/**
 * \ingroup QtAPI
 *
 * InputHandler with support of dragg gesture.
 */
class OSMSCOUT_CLIENT_QT_API DragHandler : public MoveHandler {
    Q_OBJECT
public:
    DragHandler(const MapView &view);
    virtual ~DragHandler();

    virtual bool animationInProgress() override;

    virtual bool zoom(double zoomFactor, const QPoint &widgetPosition, const QRect &widgetDimension) override;
    virtual bool move(const QVector2D &vector) override; // move vector in pixels
    virtual bool rotateBy(double angleChange) override;

    virtual bool touch(const QTouchEvent &event) override;

private:
    bool moving;
    MapView startView;
    int fingerId;
    int startX;
    int startY;
    bool ended;
    MoveAccumulator moveAccumulator;
};

/**
 * \ingroup QtAPI
 *
 * InputHandler with support of multitouch input. It use just first two
 * touch points from touch events.
 */
class OSMSCOUT_CLIENT_QT_API MultitouchHandler : public MoveHandler {
    Q_OBJECT
public:
    MultitouchHandler(const MapView &view);
    virtual ~MultitouchHandler();

    virtual bool animationInProgress() override;

    virtual bool zoom(double zoomFactor, const QPoint &widgetPosition, const QRect &widgetDimension) override;
    virtual bool move(const QVector2D &vector) override; // move vector in pixels
    virtual bool rotateBy(double angleChange) override;

    virtual bool touch(const QTouchEvent &event) override;

private:
    bool moving;
    MapView startView;
    bool initialized;
    bool ended;
    MoveAccumulator moveAccumulator;

    // we take only first two touch points into account
    QTouchEvent::TouchPoint startPointA;
    QTouchEvent::TouchPoint startPointB;
};

/**
 * \ingroup QtAPI
 *
 * Input handler that locks map view to current position.
 */
class OSMSCOUT_CLIENT_QT_API LockHandler : public JumpHandler {
    Q_OBJECT
public:
    inline LockHandler(const MapView &view, const QSizeF &widgetSize):
      JumpHandler(view), window(widgetSize)
    {};

    virtual bool currentPosition(bool locationValid, osmscout::GeoCoord currentPosition) override;
    virtual bool showCoordinates(const osmscout::GeoCoord &coord, const osmscout::Magnification &magnification, const osmscout::Bearing &bearing) override;
    virtual bool isLockedToPosition() override;
    virtual bool focusOutEvent(QFocusEvent *event) override;
    virtual void widgetResized(const QSizeF &widgetSize) override;
private:
    QSizeF window;
};

/**
 * \ingroup QtAPI
 *
 * Input handler that follow vehicle.
 */
class OSMSCOUT_CLIENT_QT_API VehicleFollowHandler : public JumpHandler {
Q_OBJECT
public:
  VehicleFollowHandler(const MapView &view, const QSizeF &widgetSize);

  virtual bool vehiclePosition(const VehiclePosition &vehiclePosition) override;
  virtual bool isLockedToPosition() override;
  virtual bool isFollowVehicle() override;
  virtual void widgetResized(const QSizeF &widgetSize) override;

private:
  QSizeF window;
};

}

Q_DECLARE_METATYPE(osmscout::AccumulatorEvent)
Q_DECLARE_METATYPE(osmscout::MapView)

#endif	/* OSMSCOUT_CLIENT_QT_INPUTHANDLER_H */
