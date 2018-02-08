/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2010 Tim Teulings
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


#ifndef OSMSCOUT_CLIENT_QT_QTROUTEDATA_H
#define OSMSCOUT_CLIENT_QT_QTROUTEDATA_H

#include <osmscout/Way.h>

#include <osmscout/RouteStep.h>
#include <osmscout/routing/Route.h>

#include <osmscout/private/ClientQtImportExport.h>

#include <QObject>

/**
 * Representation of computed route
 *
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API QtRouteData : public QObject {
  Q_OBJECT

private:
  // Main route data. It may be shared from multiple QtRouteData instances, we have to keep it immutable
  struct PrivateData{
    // TODO: add DatabaseId mapping or make it unique and stable
    osmscout::RouteDescription routeDescription;
    QList<RouteStep>           routeSteps;
    osmscout::Way              routeWay;
  };
  typedef std::shared_ptr<PrivateData> PrivateDataRef;

  PrivateDataRef data;

public:
  QtRouteData(QObject* parent=nullptr): QObject(parent) {};

  QtRouteData(const QtRouteData &other, QObject* parent=nullptr);

  QtRouteData(osmscout::RouteDescription &&routeDescription,
              QList<RouteStep> &&routeSteps,
              osmscout::Way &&routeWay,
              QObject* parent=nullptr);

  ~QtRouteData() {};

  QtRouteData& operator=(const QtRouteData&);

  inline operator bool() const {
    return (bool)data;
  };

  void clear();

  osmscout::Way routeWayCopy() const;
  const osmscout::Way& routeWay() const;

  const QList<RouteStep>& routeSteps() const;
  const osmscout::RouteDescription& routeDescription() const;
};

#endif //OSMSCOUT_CLIENT_QT_QTROUTEDATA_H
