/*
 This source is part of the libosmscout library
 Copyright (C) 2019  Lukas Karas

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

#include <osmscout/navigation/VoiceInstructionAgent.h>
#include <osmscout/navigation/PositionAgent.h>
#include <osmscout/routing/RouteDescription.h>
#include <osmscout/routing/RouteDescriptionPostprocessor.h>
#include <osmscout/log/Logger.h>
#include <osmscout/system/Compiler.h>

namespace osmscout {

struct PostprocessorCallback : public RouteDescriptionPostprocessor::Callback
{
private:
  Distance distanceFromStart;
  Distance stopAfter;
  Distance distance;
  std::optional<Distance> roundaboutEnter; //<! distance of roundabout enter before nextMessage

public:
  VoiceInstructionAgent::MessageStruct nextMessage;
  VoiceInstructionAgent::MessageStruct thenMessage;

public:
  PostprocessorCallback(const Distance &distanceFromStart, const Distance &stopAfter):
      distanceFromStart{distanceFromStart},
      stopAfter{stopAfter},
      distance{distanceFromStart}
  {}

  ~PostprocessorCallback() override = default;

  void BeforeNode(const RouteDescription::Node& node) override
  {
    distance=node.GetDistance();
  }

  void OnMotorwayLeave(const RouteDescription::DirectionDescription::Move& move)
  {
    VoiceInstructionAgent::MessageType type;
    if (move==RouteDescription::DirectionDescription::sharpLeft ||
        move==RouteDescription::DirectionDescription::left ||
        move==RouteDescription::DirectionDescription::slightlyLeft) {
      type=VoiceInstructionAgent::MessageType::LeaveMotorwayLeft;
    } else if (move==RouteDescription::DirectionDescription::sharpRight ||
               move==RouteDescription::DirectionDescription::right ||
               move==RouteDescription::DirectionDescription::slightlyRight) {
      type=VoiceInstructionAgent::MessageType::LeaveMotorwayRight;
    } else {
      type=VoiceInstructionAgent::MessageType::LeaveMotorway;
    }

    if (!nextMessage){
      nextMessage = VoiceInstructionAgent::MessageStruct{type, distance};
    } else if (!thenMessage){
      thenMessage = VoiceInstructionAgent::MessageStruct{type, distance};
    }
  }

  void OnMotorwayChange(const RouteDescription::MotorwayChangeDescriptionRef& motorwayChangeDescription,
                        [[maybe_unused]] const RouteDescription::MotorwayJunctionDescriptionRef& motorwayJunctionDescription,
                        [[maybe_unused]] const RouteDescription::DirectionDescriptionRef& directionDescription,
                        [[maybe_unused]] const RouteDescription::DestinationDescriptionRef& crossingDestinationDescription) override
  {
    assert(motorwayChangeDescription);
    OnMotorwayLeave(motorwayChangeDescription->GetDirection());
  }

  void OnMotorwayLeave(const RouteDescription::MotorwayLeaveDescriptionRef& motorwayLeaveDescription,
                       [[maybe_unused]] const RouteDescription::MotorwayJunctionDescriptionRef& motorwayJunctionDescription,
                       [[maybe_unused]] const RouteDescription::DirectionDescriptionRef& directionDescription,
                       [[maybe_unused]] const RouteDescription::NameDescriptionRef& nameDescription,
                       [[maybe_unused]] const RouteDescription::DestinationDescriptionRef& destinationDescription) override
  {
    assert(motorwayLeaveDescription);
    OnMotorwayLeave(motorwayLeaveDescription->GetDirection());
  }

  void OnRoundaboutEnter([[maybe_unused]] const RouteDescription::RoundaboutEnterDescriptionRef &roundaboutEnterDescription,
                         [[maybe_unused]] const RouteDescription::CrossingWaysDescriptionRef &crossingWaysDescription) override
  {
    if (!roundaboutEnter) {
      roundaboutEnter = distance;
    }
  }

  void OnRoundaboutLeave(const osmscout::RouteDescription::RoundaboutLeaveDescriptionRef& roundaboutLeaveDescription,
                         [[maybe_unused]] const osmscout::RouteDescription::NameDescriptionRef& nameDescription) override
  {
    assert(roundaboutLeaveDescription);
    using MessageType = VoiceInstructionAgent::MessageType;

    if (!roundaboutEnter) {
      if (!nextMessage) {
        // we are on the roundabout right now, be silent until we leave it
        nextMessage = VoiceInstructionAgent::MessageStruct{MessageType::Silent, distance};
      }
      return;
    }

    MessageType type = MessageType::NoMessage;
    switch (roundaboutLeaveDescription->GetExitCount()){
      case 1:
        type = MessageType::LeaveRbExit1;
        break;
      case 2:
        type = MessageType::LeaveRbExit2;
        break;
      case 3:
        type = MessageType::LeaveRbExit3;
        break;
      case 4:
        type = MessageType::LeaveRbExit4;
        break;
      case 5:
        type = MessageType::LeaveRbExit5;
        break;
      case 6:
        type = MessageType::LeaveRbExit6;
        break;
      default:
        // it is not correct, but what else we may say?
        type = MessageType::LeaveRbExit6;
    }
    // say the message before entering the roundabout
    if (!nextMessage){
      nextMessage = VoiceInstructionAgent::MessageStruct{type, *roundaboutEnter};
    } else if (!thenMessage){
      thenMessage = VoiceInstructionAgent::MessageStruct{type, *roundaboutEnter};
    }
    roundaboutEnter = std::nullopt;
  }

  void OnTargetReached(const osmscout::RouteDescription::TargetDescriptionRef& /*targetDescription*/) override
  {
    if (!nextMessage){
      using MessageType = VoiceInstructionAgent::MessageType;
      if (distance - distanceFromStart < Meters(40)) {
        nextMessage = VoiceInstructionAgent::MessageStruct{MessageType::TargetReached, distance};
      }
    }
  }

  void OnTurn(const osmscout::RouteDescription::TurnDescriptionRef& turnDescription,
              [[maybe_unused]] const osmscout::RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription,
              [[maybe_unused]] const osmscout::RouteDescription::DirectionDescriptionRef& directionDescription,
              [[maybe_unused]] const RouteDescription::TypeNameDescriptionRef& typeNameDescription,
              [[maybe_unused]] const osmscout::RouteDescription::NameDescriptionRef& nameDescription) override
  {
    assert(directionDescription);

    using MessageType = VoiceInstructionAgent::MessageType;
    using DirectionDescription = RouteDescription::DirectionDescription;
    MessageType type = MessageType::NoMessage;
    switch (turnDescription->GetDirection()) {
      case DirectionDescription::sharpLeft:
        type = MessageType::SharpLeft;
        break;
      case DirectionDescription::left:
        type = MessageType::TurnLeft;
        break;
      case DirectionDescription::slightlyLeft:
        type = MessageType::TurnLeft;
        break;
      case DirectionDescription::straightOn:
        type = MessageType::StraightOn;
        break;
      case DirectionDescription::slightlyRight:
        type = MessageType::TurnRight;
        break;
      case DirectionDescription::right:
        type = MessageType::TurnRight;
        break;
      case DirectionDescription::sharpRight:
        type = MessageType::SharpRight;
        break;
      default:
        assert(false);
    }
    if (type != MessageType::NoMessage){
      if (!nextMessage) {
        nextMessage = VoiceInstructionAgent::MessageStruct{type, distance};
      } else if (!thenMessage) {
        thenMessage = VoiceInstructionAgent::MessageStruct{type, distance};
      }
    }
  }

  bool Continue() const override
  {
    return (stopAfter < Distance::Zero() || distance <= stopAfter) && (!thenMessage);
  }
};

void VoiceInstructionAgent::toSamples(std::vector<VoiceInstructionMessage::VoiceSample> &samples, const MessageType &type, bool shortRoundaboutMessage)
{
  using VoiceSample = VoiceInstructionMessage::VoiceSample;

  if (!shortRoundaboutMessage && type>=MessageType::LeaveRbExit1 && type<=MessageType::LeaveRbExit6) {
    samples.push_back(VoiceSample::RbCross);
  }

  switch (type) {

    case MessageType::LeaveRbExit1:
      samples.push_back(VoiceSample::RbExit1);
      break;
    case MessageType::LeaveRbExit2:
      samples.push_back(VoiceSample::RbExit2);
      break;
    case MessageType::LeaveRbExit3:
      samples.push_back(VoiceSample::RbExit3);
      break;
    case MessageType::LeaveRbExit4:
      samples.push_back(VoiceSample::RbExit4);
      break;
    case MessageType::LeaveRbExit5:
      samples.push_back(VoiceSample::RbExit5);
      break;
    case MessageType::LeaveRbExit6:
      samples.push_back(VoiceSample::RbExit6);
      break;

    case MessageType::TargetReached:
      samples.push_back(VoiceSample::Arrive);
      break;

    case MessageType::SharpLeft:
      samples.push_back(VoiceSample::SharpLeft);
      break;
    case MessageType::TurnLeft:
      samples.push_back(VoiceSample::TurnLeft);
      break;
    case MessageType::StraightOn:
      samples.push_back(VoiceSample::Straight);
      break;
    case MessageType::TurnRight:
      samples.push_back(VoiceSample::TurnRight);
      break;
    case MessageType::SharpRight:
      samples.push_back(VoiceSample::SharpRight);
      break;

    case MessageType::LeaveMotorway:
      samples.push_back(VoiceSample::MwExit);
      break;
    case MessageType::LeaveMotorwayRight:
      samples.push_back(VoiceSample::MwExitRight);
      break;
    case MessageType::LeaveMotorwayLeft:
      samples.push_back(VoiceSample::MwExitLeft);
      break;

    default:
      log.Error() << "Message type " << static_cast<int>(type) << " is not handled!";
      assert(false);
  }
}

std::vector<VoiceInstructionMessage::VoiceSample> VoiceInstructionAgent::toSamples(const Distance &distanceFromStart,
                                                                                   const MessageStruct &message,
                                                                                   const MessageStruct &then)
{
  using VoiceSample = VoiceInstructionMessage::VoiceSample;
  std::vector<VoiceInstructionMessage::VoiceSample> samples;

  assert(message);

  if (message.type == MessageType::Silent) {
    return samples;
  }

  // distance from our position to next message
  Distance nextMessageDistance = (message.distance - distanceFromStart);
  double distanceInUnits = (units == DistanceUnitSystem::Metrics) ? nextMessageDistance.AsMeter() : nextMessageDistance.As<Yard>();

  if (distanceInUnits > 900){
    return samples;
  }

  // We are close to roundabout, we will use short roundabout message in such case.
  bool shortRoundaboutMessage = message.type >= MessageType::LeaveRbExit1 && message.type <= MessageType::LeaveRbExit6 &&
    distanceInUnits < 120;

  if (bool skipDistanceInformation = (distanceInUnits < 80 && vehicle == vehicleCar);
      !skipDistanceInformation){

    samples.push_back(VoiceSample::After);
    if (distanceInUnits > 800){
      samples.push_back(VoiceSample::Distance800);
    } else if (distanceInUnits > 700) {
      samples.push_back(VoiceSample::Distance700);
    } else if (distanceInUnits > 600) {
      samples.push_back(VoiceSample::Distance600);
    } else if (distanceInUnits > 500) {
      samples.push_back(VoiceSample::Distance500);
    } else if (distanceInUnits > 400) {
      samples.push_back(VoiceSample::Distance400);
    } else if (distanceInUnits > 300) {
      samples.push_back(VoiceSample::Distance300);
    } else if (distanceInUnits > 200) {
      samples.push_back(VoiceSample::Distance200);
    } else if (distanceInUnits > 100) {
      samples.push_back(VoiceSample::Distance100);
    } else if (distanceInUnits > 80) {
      samples.push_back(VoiceSample::Distance80);
    } else {
      samples.push_back(VoiceSample::Distance50);
    }
    samples.push_back(units == DistanceUnitSystem::Metrics ? VoiceSample::Meters : VoiceSample::Yards);
  }

  toSamples(samples, message.type, shortRoundaboutMessage);
  if (then){
    auto thenDistance = then.distance - message.distance;
    if (thenDistance <= Meters(200)) { // ignore then message otherwise
      samples.push_back(VoiceSample::Then);
      toSamples(samples, then.type, true);
    }
  }
  return samples;
}

std::list<NavigationMessageRef> VoiceInstructionAgent::Process(const NavigationMessageRef& message)
{
  std::list<NavigationMessageRef> result;

  auto routeUpdateMessage = dynamic_cast<RouteUpdateMessage*>(message.get());
  if (routeUpdateMessage != nullptr){
    // reset state
    lastMessage.type=MessageType::NoMessage;
    lastMessagePosition=Distance::Zero();
    vehicle=routeUpdateMessage->vehicle;
    return result;
  }

  auto positionMessage = dynamic_cast<PositionAgent::PositionMessage*>(message.get());
  if (positionMessage==nullptr) {
    return result;
  }

  using VoiceSample = VoiceInstructionMessage::VoiceSample;
  using namespace std::chrono;
  Timestamp now = positionMessage->timestamp;

  // triggering GpsFound / GpsLost messages
  bool gpsSignal = positionMessage->position.state != PositionAgent::PositionState::NoGpsSignal;

  // PositionAgent reports NoGpsSignal when there is no update for longer than 2 seconds
  // or accuracy is lower than 100 meters. It is fine for UI but too strict for voice
  // notification. For that reason we are using lastSeenGpsSignal time
  // and triggers GpsLost message after longer time.
  if (!prevGpsSignal && gpsSignal){
    // GpsFound
    result.push_back(std::make_shared<VoiceInstructionMessage>(
        positionMessage->timestamp,
        std::vector<VoiceSample>{VoiceSample::GpsFound}));
    prevGpsSignal = gpsSignal;
  } else if (prevGpsSignal && !gpsSignal && (now - lastSeenGpsSignal) > seconds(10)){
    // GpsLost
    result.push_back(std::make_shared<VoiceInstructionMessage>(
        positionMessage->timestamp,
        std::vector<VoiceSample>{VoiceSample::GpsLost}));
    prevGpsSignal = gpsSignal;
  }
  if (gpsSignal){
    lastSeenGpsSignal = now;
  }

  if (!positionMessage->route){
    // we don't have route description yet
    return result;
  }

  if (positionMessage->position.state != PositionAgent::PositionState::OnRoute &&
      positionMessage->position.state != PositionAgent::PositionState::EstimateInTunnel) {
    // what route instruction we should show?
    return result;
  }

  RouteDescriptionPostprocessor postprocessor;
  auto prevRouteNode = positionMessage->position.routeNode;
  auto coord = positionMessage->position.coord;
  // our current distance from route start
  Distance distanceFromStart = prevRouteNode->GetDistance() + GetEllipsoidalDistance(coord, prevRouteNode->GetLocation());
  PostprocessorCallback callback(distanceFromStart, distanceFromStart + Kilometers(2));

  // start postprocessor with the first node before us (positionMessage->position.routeNode is behind us)
  RouteDescription::NodeIterator nodeBefore=positionMessage->position.routeNode;
  while (nodeBefore != positionMessage->route->Nodes().end() &&
      nodeBefore->GetDistance() > Distance::Zero() &&
      nodeBefore->GetDistance() < distanceFromStart) {
    ++nodeBefore;
  }
  postprocessor.GenerateDescription(nodeBefore,
                                    positionMessage->route->Nodes().end(),
                                    callback);

  // distance from our position to next message
  Distance nextMessageDistance = (callback.nextMessage.distance - distanceFromStart);
  double distanceInUnits = (units == DistanceUnitSystem::Metrics) ? nextMessageDistance.AsMeter() : nextMessageDistance.As<Yard>();

  if (callback.nextMessage && distanceInUnits < 900){

    if (callback.nextMessage != lastMessage){
      result.push_back(std::make_shared<VoiceInstructionMessage>(
        positionMessage->timestamp,
        toSamples(distanceFromStart, callback.nextMessage, callback.thenMessage)));
      lastMessage=callback.nextMessage;
      lastMessagePosition=distanceFromStart;
    } else {
      Distance distFromLast = distanceFromStart - lastMessagePosition;
      if (!lastMessage ||
          distFromLast.AsMeter() < -50 ||
          (distanceInUnits < 550 && distFromLast > Meters(300)) ||
          (distanceInUnits < 150 && distFromLast > Meters(200)) ||
          (distanceInUnits < 60 && distFromLast > Meters(100))) {

        result.push_back(std::make_shared<VoiceInstructionMessage>(
            positionMessage->timestamp,
            toSamples(distanceFromStart, callback.nextMessage, callback.thenMessage)));
        lastMessage=callback.nextMessage;
        lastMessagePosition=distanceFromStart;
      }
    }
  }

  return result;
}

}
