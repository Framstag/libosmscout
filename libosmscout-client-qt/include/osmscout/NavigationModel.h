#ifndef OSMSCOUT_CLIENT_QT_NAVIGATIONMODEL_H
#define OSMSCOUT_CLIENT_QT_NAVIGATIONMODEL_H

/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2018 Lukas Karas

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

#include <osmscout/NavigationModule.h>
#include <osmscout/OverlayObject.h>

#include <osmscout/ClientQtImportExport.h>

#include <QObject>

namespace osmscout {

/**
 * Model providing navigation functionality to QML.
 * Main logic sits in osmscout::Navigation class and its Qt wrapper NavigationModule.
 *
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API NavigationModel : public QAbstractListModel
{
  Q_OBJECT
  Q_PROPERTY(QObject *route         READ getRoute          WRITE setRoute NOTIFY routeChanged)
  Q_PROPERTY(QObject *routeWay      READ getRouteWay       NOTIFY routeChanged)
  Q_PROPERTY(QObject *nextRouteStep READ getNextRoutStep   NOTIFY update)

  Q_PROPERTY(QDateTime arrivalEstimate READ getArrivalEstimate    NOTIFY arrivalUpdate)
  Q_PROPERTY(double remainingDistance  READ getRemainingDinstance NOTIFY arrivalUpdate)

  // km/h, <0 when unkwnown
  Q_PROPERTY(double currentSpeed    READ getCurrentSpeed    NOTIFY currentSpeedUpdate)
  // km/h <0 when unknown
  Q_PROPERTY(double maxAllowedSpeed READ getMaxAllowedSpeed NOTIFY maxAllowedSpeedUpdate)

signals:
  void update();

  void arrivalUpdate();

  void routeChanged(QtRouteData route,
                    osmscout::Vehicle vehicle);

  void positionChange(osmscout::GeoCoord coord,
                      bool horizontalAccuracyValid, double horizontalAccuracy);

  void rerouteRequest(double fromLat, double fromLon,
                      const QString bearing,
                      double bearingAngle,
                      double toLat, double toLon);

  void targetReached(QString targetBearing, double targetDistance);

  void positionEstimate(osmscout::PositionAgent::PositionState state, double lat, double lon, QString bearing);

  void currentSpeedUpdate(double currentSpeed);
  void maxAllowedSpeedUpdate(double maxAllowedSpeed);

public slots:
  void locationChanged(bool locationValid,
                       double lat, double lon,
                       bool horizontalAccuracyValid, double horizontalAccuracy);

  void onUpdate(std::list<RouteStep> instructions);

  void onUpdateNext(RouteStep nextRouteInstruction);

  void onPositionEstimate(const PositionAgent::PositionState state,
                          const GeoCoord coord,
                          const std::shared_ptr<osmscout::Bearing> bearing);

  void onTargetReached(const osmscout::Bearing targetBearing,
                       const osmscout::Distance targetDistance);

  void onRerouteRequest(const GeoCoord from,
                        const std::shared_ptr<osmscout::Bearing> initialBearing,
                        const GeoCoord to);

  void onArrivalEstimate(QDateTime arrivalEstimate, osmscout::Distance remainingDistance);

  void onCurrentSpeed(double currentSpeed);
  void onMaxAllowedSpeed(double maxAllowedSpeed);

public:
  enum Roles {
    ShortDescriptionRole = Qt::UserRole + 1,
    DescriptionRole = Qt::UserRole + 2,
    TypeRole = Qt::UserRole + 3,
    RoundaboutExitRole = Qt::UserRole + 4
  };
  Q_ENUM(Roles)

public:
  NavigationModel();

  virtual ~NavigationModel();

  bool isPositionOnRoute();

  QObject *getRoute() const;
  void setRoute(QObject *route);

  QObject *getNextRoutStep();

  QVariant data(const QModelIndex &index, int role) const;

  int rowCount(const QModelIndex &parent = QModelIndex()) const;

  Qt::ItemFlags flags(const QModelIndex &index) const;

  QHash<int, QByteArray> roleNames() const;

  inline OverlayWay* getRouteWay()
  {
    if (!route){
      return nullptr;
    }
    return new OverlayWay(route.routeWay().nodes);
  }

  inline QDateTime getArrivalEstimate() const
  {
    return arrivalEstimate;
  }

  inline double getRemainingDinstance() const
  {
    return remainingDistance.AsMeter();
  }

  inline double getCurrentSpeed() const
  {
    return currentSpeed;
  }

  inline double getMaxAllowedSpeed() const
  {
    return maxAllowedSpeed;
  }

private:
  NavigationModule* navigationModule;
  QtRouteData       route;
  osmscout::Vehicle vehicle;

  std::vector<RouteStep> routeSteps;
  RouteStep nextRouteStep;

  QDateTime arrivalEstimate;
  osmscout::Distance remainingDistance;

  double currentSpeed{-1};
  double maxAllowedSpeed{-1};
};

}

#endif //OSMSCOUT_CLIENT_QT_NAVIGATIONMODEL_H
