#ifndef OSMSCOUT_CLIENT_QT_ROUTINGMODEL_H
#define OSMSCOUT_CLIENT_QT_ROUTINGMODEL_H

/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2014  Tim Teulings

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

#include <osmscout/Location.h>
#include <osmscout/util/Breaker.h>
#include <osmscout/routing/RouteDescription.h>

#include <osmscoutclientqt/ClientQtImportExport.h>

#include <osmscoutclientqt/SearchLocationModel.h>
#include <osmscoutclientqt/DBThread.h>
#include <osmscoutclientqt/Router.h>
#include <osmscoutclientqt/OverlayObject.h>
#include <osmscoutclientqt/QmlRoutingProfile.h>

#include <QObject>
#include <QAbstractListModel>

#include <map>

namespace osmscout {

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API RoutingListModel : public QAbstractListModel
{
  Q_OBJECT
  Q_PROPERTY(int count            READ rowCount         NOTIFY computingChanged)
  Q_PROPERTY(bool ready           READ isReady          NOTIFY computingChanged)
  Q_PROPERTY(QObject *routeWay    READ getRouteWay      NOTIFY computingChanged)
  Q_PROPERTY(QObject *route       READ getRoute         NOTIFY computingChanged)
  Q_PROPERTY(double length        READ getRouteLength   NOTIFY computingChanged)
  Q_PROPERTY(double duration      READ getRouteDuration NOTIFY computingChanged)

signals:
  void routeRequest(LocationEntryRef start,
                    LocationEntryRef target,
                    QmlRoutingProfileRef profile,
                    int requestId,
                    osmscout::BreakerRef breaker);

  void computingChanged();

  void routeFailed(QString reason);

  void routingProgress(int percent);

public slots:
  void setStartAndTarget(LocationEntry* start,
                         LocationEntry* target,
                         QString vehicleStr="car");

  void setStartAndTarget(LocationEntry* start,
                         LocationEntry* target,
                         QmlRoutingProfile *routingProfile);

  void clear();

  void cancel();

  void onRouteComputed(QtRouteData route,
                       int requestId);

  void onRouteFailed(QString reason,
                     int requestId);

  void onRoutingProgress(int percent,
                         int requestId);

private:
  Router                *router;
  QtRouteData           route;
  int                   requestId;
  bool                  computing;
  osmscout::BreakerRef  breaker;

public:
  using Roles = RouteStep::Roles;

public:
  explicit RoutingListModel(QObject* parent = nullptr);
  RoutingListModel(const RoutingListModel&) = delete;
  RoutingListModel(RoutingListModel&&) = delete;
  virtual ~RoutingListModel();

  RoutingListModel& operator=(const RoutingListModel&) = delete;
  RoutingListModel& operator=(RoutingListModel&&) = delete;

  QVariant data(const QModelIndex &index, int role) const;

  int rowCount(const QModelIndex &parent = QModelIndex()) const;

  /**
   * Route length in meters
   * @return
   */
  double getRouteLength() const;

  /**
   * Route duration in seconds
   * @return
   */
  double getRouteDuration() const;

  Qt::ItemFlags flags(const QModelIndex &index) const;

  QHash<int, QByteArray> roleNames() const;

  Q_INVOKABLE QObject* get(int row) const;

  inline bool isReady()
  {
    return !computing;
  }

  inline Q_INVOKABLE QStringList availableVehicles()
  {
    QStringList vehicles;
    vehicles << "car";
    vehicles << "bicycle";
    vehicles << "foot";
    return vehicles;
  }

  /**
   * Create LocationEntry from geographic coordinate with optional label.
   * It may be used from QML when selecting route start/end via point on map.
   */
  inline Q_INVOKABLE QObject* locationEntryFromPosition(double lat, double lon, QString label="")
  {
    return new LocationEntry(label,osmscout::GeoCoord(lat,lon));
  }

  inline QObject *getRoute() const
  {
    assert(route.parent()==nullptr); // Ownership is copied. To transfer ownership to QML, parent have to be null.
    return new QtRouteData(route);
  }

  inline OverlayWay* getRouteWay()
  {
    if (!route){
      return nullptr;
    }
    return new OverlayWay(route.routeWay().nodes);
  }
};

}

#endif
