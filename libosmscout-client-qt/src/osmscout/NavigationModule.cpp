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
#include <osmscout/util/Logger.h>

namespace osmscout {

NavigationModule::NavigationModule(QThread *thread,
                                   SettingsRef settings,
                                   DBThreadRef dbThread):
  thread(thread), settings(settings), dbThread(dbThread)
{
  assert(settings);
  assert(dbThread);

  timer.moveToThread(thread); // constructor is called from different thread!
  connect(&timer, &QTimer::timeout, this, &NavigationModule::onTimeout);

  connect(settings.get(), &Settings::VoiceLookupDirectoryChanged, this, &NavigationModule::onVoiceChanged);
  connect(settings.get(), &Settings::VoiceDirChanged, this, &NavigationModule::onVoiceChanged);
  onVoiceChanged(settings->GetVoiceDir());
}

NavigationModule::~NavigationModule()
{
  if (thread!=QThread::currentThread()){
    qWarning() << "Destroy" << this << "from incorrect thread;" << thread << "!=" << QThread::currentThread();
  }
  qDebug() << "~NavigationModule";
  if (thread!=nullptr){
    thread->quit();
  }
}

void NavigationModule::InitPlayer()
{
  if (thread!=QThread::currentThread()){
    qWarning() << "Player initialised from incorrect thread;" << thread << "!=" << QThread::currentThread();
  }

  if (mediaPlayer==nullptr){
    assert(currentPlaylist==nullptr);
    mediaPlayer = new QMediaPlayer(this);
    currentPlaylist = new QMediaPlaylist(mediaPlayer);
    connect(mediaPlayer, &QMediaPlayer::stateChanged, this, &NavigationModule::playerStateChanged);
    mediaPlayer->setPlaylist(currentPlaylist);
  }
}

void NavigationModule::ProcessMessages(const std::list<osmscout::NavigationMessageRef>& messages)
{
  for (const auto &message : messages) {
    if (auto positionMessage=dynamic_cast<PositionAgent::PositionMessage*>(message.get());
        positionMessage != nullptr) {

      auto &position=positionMessage->position;
      assert(position.state!=PositionAgent::Uninitialised); // unitialised position newer should be used in UI
      emit positionEstimate(position.state, position.coord, lastBearing);
    }
    else if (auto bearingMessage = dynamic_cast<osmscout::BearingChangedMessage *>(message.get());
             bearingMessage != nullptr) {

      lastBearing=bearingMessage->bearing;
    }
    else if (auto targetReachedMessage = dynamic_cast<osmscout::TargetReachedMessage *>(message.get());
             targetReachedMessage != nullptr) {

      emit targetReached(targetReachedMessage->targetBearing,targetReachedMessage->targetDistance);
    }
    else if (auto req = dynamic_cast<RerouteRequestMessage *>(message.get());
             req != nullptr) {

      emit rerouteRequest(req->from,
                          req->initialBearing,
                          req->to);
    }
    else if (auto instructions = dynamic_cast<RouteInstructionsMessage<RouteStep> *>(message.get());
             instructions != nullptr) {

      emit update(instructions->instructions);
    }
    else if (auto nextInstruction = dynamic_cast<NextRouteInstructionsMessage<RouteStep> *>(message.get());
             nextInstruction != nullptr) {

      if (!nextInstruction->nextRouteInstruction.shortDescription.isEmpty()) {
        log.Debug() << "In " << nextInstruction->nextRouteInstruction.distanceTo.AsMeter() << " m: "
                    << nextInstruction->nextRouteInstruction.shortDescription.toStdString();
      }
      emit updateNext(nextInstruction->nextRouteInstruction);
    }
    else if (auto arrivalMessage = dynamic_cast<osmscout::ArrivalEstimateMessage *>(message.get());
             arrivalMessage != nullptr) {

      using namespace std::chrono;
      emit arrivalEstimate(QDateTime::fromMSecsSinceEpoch(duration_cast<milliseconds>(arrivalMessage->arrivalEstimate.time_since_epoch()).count()),
                           arrivalMessage->remainingDistance);
    }
    else if (auto currentSpeedMessage = dynamic_cast<osmscout::CurrentSpeedMessage *>(message.get());
             currentSpeedMessage != nullptr) {

      emit currentSpeed(currentSpeedMessage->speed);
    }
    else if (auto maxSpeedMessage = dynamic_cast<osmscout::MaxAllowedSpeedMessage *>(message.get());
             maxSpeedMessage != nullptr) {

      emit maxAllowedSpeed(maxSpeedMessage->maxAllowedSpeed);
    } else if (auto voiceInstructionMessage = dynamic_cast<osmscout::VoiceInstructionMessage*>(message.get());
               voiceInstructionMessage != nullptr) {

      if (!voiceDir.isEmpty()) {
        nextMessage = voiceInstructionMessage->message;
        InitPlayer();
        assert(mediaPlayer);
        playerStateChanged(mediaPlayer->state());
      }
    } else if (auto laneMessage = dynamic_cast<osmscout::LaneAgent::LaneMessage*>(message.get());
               laneMessage != nullptr) {

      emit laneUpdate(laneMessage->lane);
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
      auto database=db->GetDatabase();
      auto dbIdIt=databaseMapping.find(database->GetPath());
      if (dbIdIt==databaseMapping.end()){
        continue; // this database was not used for routing
      }
      DatabaseId databaseId=dbIdIt->second;

      MapService::TypeDefinition routableTypes;
      for (auto &type:database->GetTypeConfig()->GetTypes()){
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
      auto mapService=db->GetMapService();
      mapService->LookupTiles(magnification,box,tiles);
      mapService->LoadMissingTileData(AreaSearchParameter{},
                                          magnification,
                                          routableTypes,
                                          tiles);

      RoutableDBObjects &objects=data->dbMap[databaseId];
      objects.typeConfig=database->GetTypeConfig();
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

void NavigationModule::setupRoute(QtRouteData route,
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

void NavigationModule::onVoiceChanged(const QString dir)
{
  qDebug() << "Voice dir changed to:" << dir;
  voiceDir = dir;
  if (!QDir(voiceDir).exists()){
    voiceDir.clear(); // disable voice
  }
}

QString NavigationModule::sampleFile(osmscout::VoiceInstructionMessage::VoiceSample sample) const
{
  using VoiceSample = osmscout::VoiceInstructionMessage::VoiceSample;
  switch(sample){
    case VoiceSample::After: return "After.ogg";
    case VoiceSample::AhExitLeft: return "AhExitLeft.ogg";
    case VoiceSample::AhExit: return "AhExit.ogg";
    case VoiceSample::AhExitRight: return "AhExitRight.ogg";
    case VoiceSample::AhFerry: return "AhFerry.ogg";
    case VoiceSample::AhKeepLeft: return "AhKeepLeft.ogg";
    case VoiceSample::AhKeepRight: return "AhKeepRight.ogg";
    case VoiceSample::AhLeftTurn: return "AhLeftTurn.ogg";
    case VoiceSample::AhRightTurn: return "AhRightTurn.ogg";
    case VoiceSample::AhUTurn: return "AhUTurn.ogg";
    case VoiceSample::Arrive: return "Arrive.ogg";
    case VoiceSample::BearLeft: return "BearLeft.ogg";
    case VoiceSample::BearRight: return "BearRight.ogg";
    case VoiceSample::Depart: return "Depart.ogg";
    case VoiceSample::GpsFound: return "GpsFound.ogg";
    case VoiceSample::GpsLost: return "GpsLost.ogg";
    case VoiceSample::Charge: return "Charge.ogg";
    case VoiceSample::KeepLeft: return "KeepLeft.ogg";
    case VoiceSample::KeepRight: return "KeepRight.ogg";
    case VoiceSample::LnLeft: return "LnLeft.ogg";
    case VoiceSample::LnRight: return "LnRight.ogg";
    case VoiceSample::Marble: return "Marble.ogg";
    case VoiceSample::Meters: return "Meters.ogg";
    case VoiceSample::MwEnter: return "MwEnter.ogg";
    case VoiceSample::MwExitLeft: return "MwExitLeft.ogg";
    case VoiceSample::MwExit: return "MwExit.ogg";
    case VoiceSample::MwExitRight: return "MwExitRight.ogg";
    case VoiceSample::RbBack: return "RbBack.ogg";
    case VoiceSample::RbCross: return "RbCross.ogg";
    case VoiceSample::RbExit1: return "RbExit1.ogg";
    case VoiceSample::RbExit2: return "RbExit2.ogg";
    case VoiceSample::RbExit3: return "RbExit3.ogg";
    case VoiceSample::RbExit4: return "RbExit4.ogg";
    case VoiceSample::RbExit5: return "RbExit5.ogg";
    case VoiceSample::RbExit6: return "RbExit6.ogg";
    case VoiceSample::RbLeft: return "RbLeft.ogg";
    case VoiceSample::RbRight: return "RbRight.ogg";
    case VoiceSample::RoadEnd: return "RoadEnd.ogg";
    case VoiceSample::RouteCalculated: return "RouteCalculated.ogg";
    case VoiceSample::RouteDeviated: return "RouteDeviated.ogg";
    case VoiceSample::SharpLeft: return "SharpLeft.ogg";
    case VoiceSample::SharpRight: return "SharpRight.ogg";
    case VoiceSample::Straight: return "Straight.ogg";
    case VoiceSample::TakeFerry: return "TakeFerry.ogg";
    case VoiceSample::Then: return "Then.ogg";
    case VoiceSample::TryUTurn: return "TryUTurn.ogg";
    case VoiceSample::TurnLeft: return "TurnLeft.ogg";
    case VoiceSample::TurnRight: return "TurnRight.ogg";
    case VoiceSample::UTurn: return "UTurn.ogg";
    case VoiceSample::Yards: return "Yards.ogg";
    case VoiceSample::Take2ndLeft: return "Take2ndLeft.ogg";
    case VoiceSample::Take2ndRight: return "Take2ndRight.ogg";
    case VoiceSample::Take3rdLeft: return "Take3rdLeft.ogg";
    case VoiceSample::Take3rdRight: return "Take3rdRight.ogg";
    case VoiceSample::Distance50: return "50.ogg";
    case VoiceSample::Distance80: return "80.ogg";
    case VoiceSample::Distance100: return "100.ogg";
    case VoiceSample::Distance200: return "200.ogg";
    case VoiceSample::Distance300: return "300.ogg";
    case VoiceSample::Distance400: return "400.ogg";
    case VoiceSample::Distance500: return "500.ogg";
    case VoiceSample::Distance600: return "600.ogg";
    case VoiceSample::Distance700: return "700.ogg";
    case VoiceSample::Distance800: return "800.ogg";

    default:
      assert(false);
      return "";
  }
}

void NavigationModule::playerStateChanged(QMediaPlayer::State state)
{
  if (thread!=QThread::currentThread()){
    qWarning() << "Player state changed from incorrect thread;" << thread << "!=" << QThread::currentThread();
  }

  qDebug() << "Voice player state:" << mediaPlayer->state() << "(" << currentPlaylist->currentIndex() << "/" << currentPlaylist->mediaCount() << ")";
  if (!voiceDir.isEmpty() &&
      !nextMessage.empty() &&
      state == QMediaPlayer::StoppedState) {

    currentPlaylist->clear();

    for (const auto& sample : nextMessage){
      auto sampleUrl = QUrl::fromLocalFile(voiceDir + QDir::separator() + sampleFile(sample));
      qDebug() << "Adding to playlist:" << sampleUrl;
      currentPlaylist->addMedia(sampleUrl);
    }
    nextMessage.clear();
    currentPlaylist->setCurrentIndex(0);
    mediaPlayer->play();
  }
}

void NavigationModule::locationChanged(osmscout::GeoCoord coord,
                                       bool horizontalAccuracyValid,
                                       double horizontalAccuracy)
{
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
