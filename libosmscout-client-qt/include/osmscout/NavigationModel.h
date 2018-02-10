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

#ifndef OSMSCOUT_CLIENT_QT_NAVIGATIONMODEL_H
#define OSMSCOUT_CLIENT_QT_NAVIGATIONMODEL_H

#include <osmscout/NavigationModule.h>

#include <osmscout/private/ClientQtImportExport.h>

#include <QObject>

/**
 * Model providing navigation functionality to QML.
 * Main logic sits in osmscout::Navigation class and its Qt wrapper NavigationModule.
 *
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API NavigationModel : public QObject
{
  Q_OBJECT
  Q_PROPERTY(bool positionOnRoute   READ isPositionOnRoute NOTIFY positionOnRouteChanged)
  Q_PROPERTY(QObject *route         READ getRoute          WRITE setRoute NOTIFY routeChanged)
  Q_PROPERTY(QObject *nextRouteStep READ getNextRoutStep   NOTIFY update)

signals:
  void update();
  void positionOnRouteChanged();

  void routeChanged(LocationEntryRef target,
                    QtRouteData route,
                    osmscout::Vehicle vehicle);

  void positionChange(osmscout::GeoCoord coord,
                      bool horizontalAccuracyValid, double horizontalAccuracy);

public slots:
  void locationChanged(bool locationValid,
                       double lat, double lon,
                       bool horizontalAccuracyValid, double horizontalAccuracy);

  void onUpdated(bool onRoute, RouteStep routeStep);

public:
  NavigationModel();

  virtual ~NavigationModel();

  bool isPositionOnRoute();

  QObject *getRoute() const;
  void setRoute(QObject *route);

  QObject *getNextRoutStep();

private:
  NavigationModule* navigationModule;
  LocationEntryRef  target;
  QtRouteData       route;
  osmscout::Vehicle vehicle;

  bool              onRoute;
  RouteStep         nextRouteStep;
};

#endif //OSMSCOUT_CLIENT_QT_NAVIGATIONMODEL_H
