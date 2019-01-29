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

namespace osmscout {

/*
NextStepDescriptionBuilder::NextStepDescriptionBuilder():
    roundaboutCrossingCounter(0),
    index(0)
{
}

void NextStepDescriptionBuilder::NextDescription(const Distance &distance,
                                                 std::list<osmscout::RouteDescription::Node>::const_iterator& waypoint,
                                                 std::list<osmscout::RouteDescription::Node>::const_iterator end)
{
  if (waypoint==end || (distance.AsMeter()>=0 && previousDistance>distance)) {
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
    description.distance=waypoint->GetDistance();
    description.time    =waypoint->GetTime();

    if (waypoint->GetDistance() > distance){
      description.distanceTo=(waypoint->GetDistance()-distance);
      break;
    }
  }
}
*/

NavigationModule::NavigationModule(QThread *thread,
                                   SettingsRef settings,
                                   DBThreadRef dbThread):
  thread(thread), settings(settings), dbThread(dbThread)
{
  timer.moveToThread(thread); // constructor is called from different thread!
  connect(&timer, SIGNAL(timeout()), this, SLOT(onTimeout()));
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

void NavigationModule::ProcessMessages(const std::list<osmscout::NavigationMessageRef>& messages)
{
  for (const auto &message : messages) {
    if (dynamic_cast<PositionAgent::PositionMessage *>(message.get()) != nullptr) {
      auto positionMessage=dynamic_cast<PositionAgent::PositionMessage*>(message.get());
      auto &position=positionMessage->position;
      emit positionEstimate(position.state, position.coord);
    }
    else if (dynamic_cast<osmscout::TargetReachedMessage*>(message.get())!=nullptr) {
      auto targetReachedMessage = dynamic_cast<osmscout::TargetReachedMessage *>(message.get());
      emit targetReached(targetReachedMessage->targetBearing,targetReachedMessage->targetDistance);
    }
    else if (dynamic_cast<RerouteRequestMessage*>(message.get())!=nullptr) {
      auto req = dynamic_cast<RerouteRequestMessage *>(message.get());
      emit rerouteRequest(req->from, req->initialBearing, req->to);
    }
  }
}

bool NavigationModule::loadRoutableObjects(const GeoBox &box,
                                           const Vehicle &vehicle,
                                           const std::map<std::string,DatabaseId> &databaseMapping,
                                           RoutableObjectsRef &data)
{
  StopClock stopClock;

  assert(data);
  data->bbox=box;

  dbThread->RunSynchronousJob([&](const std::list<DBInstanceRef> &databases){
    Magnification magnification(Magnification::magClose);
    for (auto &db:databases) {
      auto dbIdIt=databaseMapping.find(db->database->GetPath());
      if (dbIdIt==databaseMapping.end()){
        continue; // this database was not used for routing
      }
      DatabaseId databaseId=dbIdIt->second;

      MapService::TypeDefinition routableTypes;
      for (auto &type:db->database->GetTypeConfig()->GetTypes()){
        if (type->CanRoute(vehicle)){
          if (type->CanBeArea()){
            routableTypes.areaTypes.Set(type);
          }
          if (type->CanBeWay()){
            routableTypes.wayTypes.Set(type);
          }
          if (type->CanBeNode()){ // can be node routable? :-)
            routableTypes.nodeTypes.Set(type);
          }
        }
      }

      std::list<TileRef> tiles;
      db->mapService->LookupTiles(magnification,box,tiles);
      db->mapService->LoadMissingTileData(AreaSearchParameter{},
                                          magnification,
                                          routableTypes,
                                          tiles);

      RoutableDBObjects &objects=data->dbMap[databaseId];
      objects.typeConfig=db->database->GetTypeConfig();
      for (auto &tile:tiles){
        tile->GetWayData().CopyData([&](const WayRef &way){objects.ways[way->GetFileOffset()]=way;});
        tile->GetAreaData().CopyData([&](const AreaRef &area){objects.areas[area->GetFileOffset()]=area;});
      }
    }
  });

  stopClock.Stop();
  if (stopClock.GetMilliseconds() > 50){
    log.Warn() << "Loading of routable objects took " << stopClock.ResultString();
  }

  return true;
}

void NavigationModule::setupRoute(LocationEntryRef /*target*/,
                                  QtRouteData route,
                                  osmscout::Vehicle vehicle)
{
  if (thread!=QThread::currentThread()){
    qWarning() << "setupRoute" << this << "from incorrect thread;" << thread << "!=" << QThread::currentThread();
  }
  if (!route){
    routeDescription = nullptr;
    if (timer.isActive()) {
      timer.stop();
    }
    return;
  }
  // create own copy of route description
  routeDescription=std::make_shared<RouteDescription>(route.routeDescription());

  auto now = std::chrono::system_clock::now();
  auto initializeMessage=std::make_shared<osmscout::InitializeMessage>(now);
  ProcessMessages(engine.Process(initializeMessage));

  auto routeUpdateMessage=std::make_shared<osmscout::RouteUpdateMessage>(now,routeDescription,vehicle);
  ProcessMessages(engine.Process(routeUpdateMessage));

  timer.start(1000);
}

void NavigationModule::onTimeout()
{
  auto now = std::chrono::system_clock::now();
  auto routeUpdateMessage=std::make_shared<osmscout::TimeTickMessage>(now);
  ProcessMessages(engine.Process(routeUpdateMessage));
}

void NavigationModule::locationChanged(osmscout::GeoCoord coord,
                                       bool horizontalAccuracyValid,
                                       double horizontalAccuracy) {
  if (thread!=QThread::currentThread()){
    qWarning() << "locationChanged" << this << "from incorrect thread;" << thread << "!=" << QThread::currentThread();
  }

  auto now = std::chrono::system_clock::now();
  auto gpsUpdateMessage=std::make_shared<osmscout::GPSUpdateMessage>(
      now,
      coord,
      /*speed is not known*/-1,
      Meters(horizontalAccuracyValid ? horizontalAccuracy: -1));

  ProcessMessages(engine.Process(gpsUpdateMessage));

  auto timeTickMessage=std::make_shared<osmscout::TimeTickMessage>(now);
  ProcessMessages(engine.Process(timeTickMessage));
}
}
