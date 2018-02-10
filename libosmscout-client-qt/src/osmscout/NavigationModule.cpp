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

#include <osmscout/NavigationModule.h>

NextStepDescriptionBuilder::NextStepDescriptionBuilder():
    roundaboutCrossingCounter(0),
    index(0),
    previousDistance(0.0)
{
}

void NextStepDescriptionBuilder::NextDescription(double distance,
                                                 std::list<osmscout::RouteDescription::Node>::const_iterator& waypoint,
                                                 std::list<osmscout::RouteDescription::Node>::const_iterator end)
{
  if (waypoint==end || (distance>=0 && previousDistance>distance)) {
    return;
  }
  RouteDescriptionBuilder builder;
  for (; waypoint!=end; waypoint++) {
    QList<RouteStep> routeSteps;
    if (!builder.GenerateRouteStep(*waypoint, routeSteps, roundaboutCrossingCounter)){
      continue;
    }
    if (routeSteps.isEmpty()){
      continue;
    }

    description=routeSteps.first();
    description.distance=waypoint->GetDistance()*1000;
    description.time    =waypoint->GetTime()*3600;

    if (waypoint->GetDistance() > distance){
      description.distanceTo=(waypoint->GetDistance()-distance)*1000;
      break;
    }
  }
}

NavigationModule::NavigationModule(QThread *thread,
                                   SettingsRef settings,
                                   DBThreadRef dbThread):
  thread(thread), settings(settings), dbThread(dbThread),
  navigation(&nextStepDescBuilder)
{
  navigation.SetSnapDistance(40.0);
}

NavigationModule::~NavigationModule()
{
  if (thread!=QThread::currentThread()){
    qWarning() << "Destroy" << this << "from incorrect thread;" << thread << "!=" << QThread::currentThread();
  }
  qDebug() << "~NavigationModule";
  if (thread!=NULL){
    thread->quit();
  }
}

void NavigationModule::setupRoute(LocationEntryRef /*target*/,
                                  QtRouteData route,
                                  osmscout::Vehicle /*vehicle*/)
{
  if (thread!=QThread::currentThread()){
    qWarning() << "setupRoute" << this << "from incorrect thread;" << thread << "!=" << QThread::currentThread();
  }
  if (!route){
    routeDescription.Clear();
    navigation.Clear();
    return;
  }
  // create own copy of route description
  routeDescription=route.routeDescription();
  navigation.SetRoute(&routeDescription);
}

void NavigationModule::locationChanged(osmscout::GeoCoord coord,
                                       bool /*horizontalAccuracyValid*/,
                                       double /*horizontalAccuracy*/) {
  if (thread!=QThread::currentThread()){
    qWarning() << "locationChanged" << this << "from incorrect thread;" << thread << "!=" << QThread::currentThread();
  }
  if (!navigation.HasRoute()) {
    return;
  }

  double minDistance = 0;
  bool onRoute = navigation.UpdateCurrentLocation(coord, minDistance);
  RouteStep routeStep = navigation.nextWaypointDescription();
  emit update(onRoute, routeStep);
}
