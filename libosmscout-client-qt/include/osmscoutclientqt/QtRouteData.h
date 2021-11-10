#ifndef OSMSCOUT_CLIENT_QT_QTROUTEDATA_H
#define OSMSCOUT_CLIENT_QT_QTROUTEDATA_H

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

#include <osmscout/Way.h>

#include <osmscoutclientqt/RouteStep.h>
#include <osmscout/routing/RouteDescription.h>

#include <osmscoutclientqt/ClientQtImportExport.h>

#include <QObject>

namespace osmscout {

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
    osmscout::RouteDescription routeDescription;
    QList<RouteStep>           routeSteps;
    osmscout::Way              routeWay;
  };
  using PrivateDataRef = std::shared_ptr<PrivateData>;

  PrivateDataRef data;

public:
  explicit QtRouteData(QObject* parent=nullptr): QObject(parent) {};
  //! copy constructor, Qt ownership is copied
  QtRouteData(const QtRouteData &other);

  QtRouteData(osmscout::RouteDescription &&routeDescription,
              QList<RouteStep> &&routeSteps,
              osmscout::Way &&routeWay,
              QObject* parent=nullptr);

  QtRouteData(QtRouteData &&) = delete;
  ~QtRouteData() override = default;

  QtRouteData& operator=(const QtRouteData&);
  QtRouteData& operator=(const QtRouteData&&) = delete;

  inline operator bool() const {
    return (bool)data;
  };

  void clear();

  osmscout::Way routeWayCopy() const;
  const osmscout::Way& routeWay() const;

  const QList<RouteStep>& routeSteps() const;
  const osmscout::RouteDescription& routeDescription() const;
};

}

#endif //OSMSCOUT_CLIENT_QT_QTROUTEDATA_H
