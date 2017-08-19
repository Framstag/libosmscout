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

#include <map>

#include <QObject>
#include <QAbstractListModel>

#include <osmscout/Location.h>
#include <osmscout/util/Breaker.h>
#include <osmscout/routing/Route.h>

#include <osmscout/private/ClientQtImportExport.h>

#include <osmscout/SearchLocationModel.h>
#include <osmscout/DBThread.h>
#include <osmscout/Router.h>
#include <osmscout/OverlayWay.h>

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API RoutingListModel : public QAbstractListModel
{
  Q_OBJECT
  Q_PROPERTY(int count            READ rowCount         NOTIFY computingChanged)
  Q_PROPERTY(bool ready           READ isReady          NOTIFY computingChanged)
  Q_PROPERTY(QObject *routeWay    READ getRouteWay      NOTIFY computingChanged)
  Q_PROPERTY(double length        READ getRouteLength   NOTIFY computingChanged)
  Q_PROPERTY(double duration      READ getRouteDuration NOTIFY computingChanged)

signals:
  void routeRequest(LocationEntryRef start,
                    LocationEntryRef target,
                    osmscout::Vehicle vehicle,
                    int requestId,
                    osmscout::BreakerRef breaker);

  void computingChanged();

  void routeFailed(QString reason);

  void routingProgress(int percent);

public slots:
  void setStartAndTarget(LocationEntry* start,
                         LocationEntry* target,
                         QString vehicleStr="car");

  void clear();

  void cancel();

  void onRouteComputed(RouteSelectionRef route,
                       int requestId);

  void onRouteFailed(QString reason,
                     int requestId);

  void onRoutingProgress(int percent,
                         int requestId);

private:
  Router                *router;
  RouteSelectionRef     route;
  int                   requestId;
  bool                  computing;
  osmscout::BreakerRef  breaker;

public:
  enum Roles {
      LabelRole = Qt::UserRole
  };

public:
  RoutingListModel(QObject* parent = 0);
  virtual ~RoutingListModel();

  QVariant data(const QModelIndex &index, int role) const;

  int rowCount(const QModelIndex &parent = QModelIndex()) const;

  double getRouteLength() const;

  double getRouteDuration() const;

  Qt::ItemFlags flags(const QModelIndex &index) const;

  QHash<int, QByteArray> roleNames() const;

  Q_INVOKABLE RouteStep* get(int row) const;

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
  inline Q_INVOKABLE LocationEntry* locationEntryFromPosition(double lat, double lon, QString label="")
  {
    return new LocationEntry(label,osmscout::GeoCoord(lat,lon));
  }

  inline OverlayWay* getRouteWay()
  {
    if (!route){
      return NULL;
    }
    return new OverlayWay(route->routeWay.nodes);
  }
};

#endif
