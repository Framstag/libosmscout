#ifndef OSMSCOUT_CLIENT_QT_MAPWIDGET_H
#define OSMSCOUT_CLIENT_QT_MAPWIDGET_H

/*
  OSMScout - a Qt backend for libosmscout and libosmscout-map
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

#include <QQuickPaintedItem>

#include <osmscout/GeoCoord.h>
#include <osmscout/util/GeoBox.h>
#include <osmscoutclient/DBThread.h>

#include <osmscoutclientqt/ClientQtImportExport.h>

#include <osmscoutclientqt/MapRenderer.h>
#include <osmscoutclientqt/SearchLocationModel.h>
#include <osmscoutclientqt/InputHandler.h>
#include <osmscoutclientqt/OSMScoutQt.h>
#include <osmscoutclientqt/OverlayObject.h>
#include <osmscoutclientqt/VehiclePosition.h>
#include <osmscoutclientqt/IconAnimation.h>
#include <osmscoutclientqt/IconLookup.h>

namespace osmscout {

/**
 * \defgroup QtAPI Qt API
 *
 * Classes for integration osmscout library with Qt framework.
 */

/**
 * \ingroup QtAPI
 *
 * Qt Quick widget for displaying map.
 *
 * Type should to be registered by \ref qmlRegisterType method
 * and \ref DBThread instance should be initialized before first usage.
 *
 */
class OSMSCOUT_CLIENT_QT_API MapWidget : public QQuickPaintedItem
{
  Q_OBJECT
  Q_PROPERTY(QObject  *view    READ GetView     WRITE SetMapView  NOTIFY viewChanged)
  Q_PROPERTY(QObject  *vehiclePosition READ GetVehiclePosition WRITE SetVehiclePosition)
  Q_PROPERTY(double   lat      READ GetLat      NOTIFY latChanged)
  Q_PROPERTY(double   lon      READ GetLon      NOTIFY lonChanged)
  Q_PROPERTY(double   angle    READ GetAngle    NOTIFY angleChanged)
  Q_PROPERTY(int      zoomLevel READ GetMagLevel NOTIFY magLevelChanged)
  Q_PROPERTY(QString  zoomLevelName READ GetZoomLevelName NOTIFY magLevelChanged)
  Q_PROPERTY(double   pixelSize READ GetPixelSize NOTIFY pixelSizeChanged)
  Q_PROPERTY(bool     databaseLoaded READ isDatabaseLoaded NOTIFY databaseLoaded)
  Q_PROPERTY(bool     finished READ IsFinished  NOTIFY finishedChanged)
  Q_PROPERTY(bool     showCurrentPosition READ getShowCurrentPosition WRITE setShowCurrentPosition)
  Q_PROPERTY(bool     lockToPosition READ isLockedToPosition WRITE setLockToPosition NOTIFY lockToPositionChanged)
  Q_PROPERTY(bool     followVehicle READ isFollowVehicle WRITE setFollowVehicle NOTIFY followVehicleChanged)
  Q_PROPERTY(bool     vehicleAutoRotateMap READ isVehicleAutoRotateMap WRITE setVehicleAutoRotateMap NOTIFY vehicleAutoRotateMapChanged)
  Q_PROPERTY(QString  stylesheetFilename READ GetStylesheetFilename NOTIFY stylesheetFilenameChanged)
  Q_PROPERTY(QString  renderingType READ GetRenderingType WRITE SetRenderingType NOTIFY renderingTypeChanged)

  /**
   * This property holds whether the mouse events may be stolen from this Widget.
   * By default this property is false.
   * see https://doc.qt.io/qt-5/qml-qtquick-mousearea.html#preventStealing-prop
   */
  Q_PROPERTY(bool preventMouseStealing READ isPreventMouseStealing WRITE setPreventMouseStealing)

  Q_PROPERTY(bool stylesheetHasErrors           READ stylesheetHasErrors              NOTIFY styleErrorsChanged)
  Q_PROPERTY(int stylesheetErrorLine            READ firstStylesheetErrorLine         NOTIFY styleErrorsChanged)
  Q_PROPERTY(int stylesheetErrorColumn          READ firstStylesheetErrorColumn       NOTIFY styleErrorsChanged)
  Q_PROPERTY(QString stylesheetErrorDescription READ firstStylesheetErrorDescription  NOTIFY styleErrorsChanged)

  Q_PROPERTY(QString vehicleStandardIconFile    READ getVehicleStandardIconFile     WRITE setVehicleStandardIconFile)
  Q_PROPERTY(QString vehicleNoGpsSignalIconFile READ getVehicleNoGpsSignalIconFile  WRITE setVehicleNoGpsSignalIconFile)
  Q_PROPERTY(QString vehicleInTunnelIconFile    READ getVehicleInTunnelIconFile     WRITE setVehicleInTunnelIconFile)
  Q_PROPERTY(double vehicleIconSize             READ getVehicleIconSize             WRITE setVehicleIconSize)

  Q_PROPERTY(bool interactiveIcons READ hasInteractiveIcons WRITE setInteractiveIcons)

private:
  MapRenderer      *renderer{nullptr};

  MapView          *view{nullptr};

  InputHandler     *inputHandler{nullptr};
  TapRecognizer    tapRecognizer;

  IconLookup       *iconLookup{nullptr};
  IconAnimation    iconAnimation;

  bool preventMouseStealing{false};

  bool finished{false};

  struct CurrentLocation {
    QDateTime lastUpdate{QDateTime::currentDateTime()};
    bool valid{false};
    osmscout::GeoCoord coord;
    bool horizontalAccuracyValid{false};
    double horizontalAccuracy{0};
  };
  CurrentLocation currentPosition;
  bool showCurrentPosition{false};

  RenderingType renderingType{RenderingType::PlaneRendering};

  QMap<int, osmscout::GeoCoord> marks;

  // vehicle data
  struct Vehicle {
    VehiclePosition  *position{nullptr};

    double iconSize{16}; // icon size [mm]

    /// input handler control
    bool follow{false};
    bool autoRotateMap{true};
    QElapsedTimer lastGesture; // when there is some gesture, we will not follow vehicle for some short time

    QString standardIconFile{"vehicle.svg"}; // state == OnRoute | OffRoute
    QString noGpsSignalIconFile{"vehicle_not_fixed.svg"}; // state == NoGpsSignal
    QString inTunnelIconFile{"vehicle_tunnel.svg"}; // state == EstimateInTunnel

    QImage standardIcon;
    QImage noGpsSignalIcon;
    QImage inTunnelIcon;

    QImage getIcon() const
    {
      if (position==nullptr){
        return QImage();
      }
      switch(position->getState()){
        case PositionAgent::PositionState::EstimateInTunnel:
          return !inTunnelIcon.isNull() ? inTunnelIcon : standardIcon;
        case PositionAgent::PositionState::NoGpsSignal:
          return !noGpsSignalIcon.isNull() ? noGpsSignalIcon : standardIcon;
        default:
          return standardIcon;
      }
    }
  };
  Vehicle vehicle;

  float vehicleScaleFactor{1.0};

signals:
  void viewChanged();
  void latChanged();
  void lonChanged();
  void angleChanged();
  void magLevelChanged();
  void pixelSizeChanged();
  void lockToPositionChanged();
  void followVehicleChanged();
  void vehicleAutoRotateMapChanged();
  void finishedChanged(bool finished);

  void mouseMove(const int screenX, const int screenY, const double lat, const double lon, const Qt::KeyboardModifiers modifiers);
  void tap(const int screenX, const int screenY, const double lat, const double lon);
  void doubleTap(const int screenX, const int screenY, const double lat, const double lon);
  void longTap(const int screenX, const int screenY, const double lat, const double lon);
  void tapLongTap(const int screenX, const int screenY, const double lat, const double lon);

  void iconTapped(QPoint screenCoord, double lat, double lon, QString databasePath,
                  QString objectType, quint64 objectId, int poiId, QString type,
                  QString name, QString altName, QString ref, QString operatorName, QString phone, QString website,
                  QString openingHours);

  void stylesheetFilenameChanged();
  void styleErrorsChanged();
  void databaseLoaded(osmscout::GeoBox);
  void renderingTypeChanged(QString type);

  void objectPicked(const ObjectFileRef object);

  void flushCachesRequest(const std::chrono::milliseconds &idleMs);

public slots:
  void changeView(const MapView &view);
  void redraw();

  void recenter();

  void zoom(double zoomFactor);
  void zoomIn(double zoomFactor);
  void zoomOut(double zoomFactor);

  void zoom(double zoomFactor, const QPoint widgetPosition);
  void zoomIn(double zoomFactor, const QPoint widgetPosition);
  void zoomOut(double zoomFactor, const QPoint widgetPosition);

  void move(QVector2D vector);
  void moveLeft();
  void moveRight();
  void moveUp();
  void moveDown();

  /**
   * Rotate view to specified angle [radians; [0 ~ 2*PI) ]
   * @param angle
   */
  void rotateTo(double angle);
  void rotateLeft();
  void rotateRight();
  void pivotBy(double angleChange);

  void toggleDaylight();
  void reloadStyle();
  void reloadTmpStyle();

  void showCoordinates(osmscout::GeoCoord coord, osmscout::Magnification magnification);
  void showCoordinates(double lat, double lon);
  void showCoordinatesInstantly(osmscout::GeoCoord coord, osmscout::Magnification magnification);
  void showCoordinatesInstantly(double lat, double lon);
  void showLocation(LocationEntry* location);

  void locationChanged(bool locationValid,
                       double lat, double lon,
                       bool horizontalAccuracyValid = false,
                       double horizontalAccuracy = 0,
                       const QDateTime &lastUpdate = QDateTime::currentDateTime());

  /**
   * Add "mark" (small red circle) on top of map.
   * It will be rendered in UI thread, count of marks should be limited.
   */
  void addPositionMark(int id, double lat, double lon);
  void removePositionMark(int id);

  /**
   * Method for registering map overlay objects.
   * Usage from QML:
   *
   *    var way=map.createOverlayWay();
   *    way.addPoint(50.09180646851823, 14.498789861494872);
   *    way.addPoint(50.09180646851823, 14.60);
   *    map.addOverlayObject(0,way);
   *
   * @param id
   * @param o
   */
  void addOverlayObject(int id, QObject *o);
  void removeOverlayObject(int id);
  void removeAllOverlayObjects();

  OverlayWay *createOverlayWay(QString type="_route");
  OverlayArea *createOverlayArea(QString type="_highlighted");
  OverlayNode *createOverlayNode(QString type="_highlighted");

  void deactivateIcons();

  bool toggleDebug();
  bool toggleInfo();

  /**
   * Method for configuring the aspect ratio of painted items. Default value is 1.0.
   * Using HiDPI it is required to set the value according with the scale factor.
   *
   * @param ratio
   */
  void setVehicleScaleFactor(float factor);

  void onIconFound(QPoint lookupCoord, MapIcon icon);

  //void pick(double lat,double lon);

private:
  Slot<double> mapDpiSlot{ std::bind(&MapWidget::onMapDPIChange, this, std::placeholders::_1) };

  Slot<> stylesheetFilenameChangedSlot{
    [this](){
      emit stylesheetFilenameChanged();
    }
  };

  Slot<> styleErrorsChangedSlot{
    [this](){
      emit styleErrorsChanged();
    }
  };

  Slot<osmscout::GeoBox> databaseLoadedSlot{
    [this](const osmscout::GeoBox &geoBox){
      emit databaseLoaded(geoBox);
    }
  };

  Slot<std::chrono::milliseconds> flushCachesSlot {
    std::bind(&MapWidget::flushCachesRequest, this, std::placeholders::_1)
  };


private slots:

  virtual void onTap(const QPoint p);
  virtual void onDoubleTap(const QPoint p);
  virtual void onLongTap(const QPoint p);
  virtual void onTapLongTap(const QPoint p);
  virtual void onTapAndDrag(const QPoint p);

  void onMapDPIChange(double dpi);

  void onResize();

private:
  void setupInputHandler(InputHandler *newGesture);

  void loadVehicleIcons();

  /**
   * @param dimension in kilometers
   * @return approximated magnification by object dimension
   */
  osmscout::Magnification magnificationByDimension(const Distance &dimension);

  void setupRenderer();

public:
  MapWidget(QQuickItem* parent = nullptr);
  ~MapWidget() override;

  inline MapView* GetView() const
  {
      return view; // We should be owner, parent is set http://doc.qt.io/qt-5/qqmlengine.html#objectOwnership
  }

  inline void SetMapView(QObject *o)
  {
    MapView *updated = dynamic_cast<MapView*>(o);
    if (updated == nullptr){
        qWarning() << "Failed to cast " << o << " to MapView*.";
        return;
    }

    bool changed = *view != *updated;
    if (changed){
      setupInputHandler(new InputHandler(*updated));
      changeView(*updated);
    }
  }

  MapViewStruct GetViewStruct() const;

  inline VehiclePosition* GetVehiclePosition() const
  {
    return vehicle.position;
  }

  void SetVehiclePosition(QObject *o);

  inline QString getVehicleStandardIconFile() const
  {
    return vehicle.standardIconFile;
  }

  inline void setVehicleStandardIconFile(const QString &file)
  {
    vehicle.standardIconFile = file;
    loadVehicleIcons();
    redraw();
  }

  inline QString getVehicleNoGpsSignalIconFile() const
  {
    return vehicle.noGpsSignalIconFile;
  }

  inline void setVehicleNoGpsSignalIconFile(const QString &file)
  {
    vehicle.noGpsSignalIconFile = file;
    loadVehicleIcons();
    redraw();
  }

  inline QString getVehicleInTunnelIconFile() const
  {
    return vehicle.inTunnelIconFile;
  }

  inline void setVehicleInTunnelIconFile(const QString &file)
  {
    vehicle.inTunnelIconFile = file;
    loadVehicleIcons();
    redraw();
  }

  inline double getVehicleIconSize() const{
    return vehicle.iconSize;
  }

  inline void setVehicleIconSize(double value)
  {
    vehicle.iconSize = value;
    loadVehicleIcons();
    redraw();
  }

  bool hasInteractiveIcons() const
  {
    return iconLookup!=nullptr;
  }

  void setInteractiveIcons(bool b);

  inline double GetLat() const
  {
      return view->center.GetLat();
  }

  inline double GetLon() const
  {
      return view->center.GetLon();
  }

  inline double GetAngle() const
  {
      return view->angle.AsRadians();
  }

  inline osmscout::GeoCoord GetCenter() const
  {
      return view->center;
  }

  QString GetStylesheetFilename() const;

  QString GetZoomLevelName() const;

  inline int GetMagLevel() const
  {
      return view->magnification.GetLevel();
  }

  inline double GetPixelSize() const
  {
      return getProjection().GetPixelSize();
  }

  inline bool IsFinished() const
  {
      return finished;
  }

  inline bool getShowCurrentPosition() const
  {
      return showCurrentPosition;
  };

  inline void setShowCurrentPosition(bool b)
  {
      showCurrentPosition = b;
      redraw();
  };

  inline bool isLockedToPosition() const
  {
      return inputHandler->isLockedToPosition();
  };

  void setLockToPosition(bool);

  inline bool isFollowVehicle() const
  {
    return vehicle.follow;
  }

  void setFollowVehicle(bool);

  inline bool isVehicleAutoRotateMap() const
  {
    return vehicle.autoRotateMap;
  }

  void setVehicleAutoRotateMap(bool);

  inline osmscout::MercatorProjection getProjection() const
  {
    osmscout::MercatorProjection projection;

    size_t w=width();
    size_t h=height();
    projection.Set(GetCenter(),
                   view->angle.AsRadians(),
                   view->magnification,
                   view->mapDpi,
                   // to avoid invalid projection when scene is not finished yet
                   w==0? 100:w,
                   h==0? 100:h);
    return projection;
  }

  void wheelEvent(QWheelEvent* event) override;
  void touchEvent(QTouchEvent *event) override;

  void focusOutEvent(QFocusEvent *event) override;

  void translateToTouch(QMouseEvent* event, Qt::TouchPointStates states);

  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void hoverMoveEvent(QHoverEvent* event) override;

  void paint(QPainter *painter) override;

  bool stylesheetHasErrors() const;
  int firstStylesheetErrorLine() const;
  int firstStylesheetErrorColumn() const;
  QString firstStylesheetErrorDescription() const;

  bool isDatabaseLoaded();
  Q_INVOKABLE bool isInDatabaseBoundingBox(double lat, double lon);
  Q_INVOKABLE QPointF screenPosition(double lat, double lon);

  QString GetRenderingType() const;
  void SetRenderingType(QString type);

  bool isPreventMouseStealing() const
  {
    return preventMouseStealing;
  }

  void  setPreventMouseStealing(bool b)
  {
    preventMouseStealing = b;
  }

  void FlushCaches(const std::chrono::milliseconds &idleMs);

  /**
   * Helper for loading SVG graphics
   */
  static QImage loadSVGIcon(const QString &directory, const QString fileName, double iconPixelSize);
};

}

#endif // OSMSCOUT_CLIENT_QT_MAPWIDGET_H
