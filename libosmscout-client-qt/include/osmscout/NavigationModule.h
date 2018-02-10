/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2017 Lukas Karas

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

#ifndef LIBOSMSCOUT_NAVIGATIONMODULE_H
#define LIBOSMSCOUT_NAVIGATIONMODULE_H

#include <osmscout/DBThread.h>
#include <osmscout/Settings.h>
#include <osmscout/Router.h>
#include <osmscout/Navigation.h>

#include <osmscout/private/ClientQtImportExport.h>

#include <QObject>

class OSMSCOUT_CLIENT_QT_API NextStepDescriptionBuilder:
    public osmscout::OutputDescription<RouteStep> {

public:
  NextStepDescriptionBuilder();

  virtual ~NextStepDescriptionBuilder(){};

  virtual void NextDescription(double distance,
                               std::list<osmscout::RouteDescription::Node>::const_iterator& waypoint,
                               std::list<osmscout::RouteDescription::Node>::const_iterator end);

private:
  size_t          roundaboutCrossingCounter;
  size_t          index;
  double          previousDistance;
};

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API NavigationModule: public QObject {
  Q_OBJECT

signals:
  void update(bool onRoute, RouteStep routeStep);

public slots:
  void setupRoute(LocationEntryRef target,
                  QtRouteData route,
                  osmscout::Vehicle vehicle);

  void locationChanged(osmscout::GeoCoord coord,
                       bool /*horizontalAccuracyValid*/,
                       double /*horizontalAccuracy*/);

public:
  NavigationModule(QThread *thread,
                   SettingsRef settings,
                   DBThreadRef dbThread);

  virtual ~NavigationModule();

private:
  QThread     *thread;
  SettingsRef settings;
  DBThreadRef dbThread;

  NextStepDescriptionBuilder nextStepDescBuilder;
  osmscout::RouteDescription routeDescription;
  osmscout::Navigation<RouteStep> navigation;
};

#endif //LIBOSMSCOUT_NAVIGATIONMODULE_H
