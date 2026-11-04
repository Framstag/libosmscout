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
#include <osmscout/util/Distance.h>
#include <osmscout/util/Time.h>
#include <osmscout/system/Compiler.h>
#include <osmscout/OSMScoutTypes.h>

#include <list>
#include <optional>
#include <algorithm>
#include <cstddef>

namespace osmscout {

struct PostprocessorCallback : public RouteDescriptionPostprocessor::Callback
{
private:
  Distance distanceFromStart;
  Distance stopAfter;
  Distance distance;
  std::optional<Distance> roundaboutEnter; //<! distance of roundabout enter before nextMessage
  RouteDescription::SuggestedLaneDescriptionRef suggestedLaneDescription;
  RouteDescription::LaneDescriptionRef laneDescription;

public:
  VoiceMessageStruct nextMessage;
  VoiceMessageStruct thenMessage;

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
    suggestedLaneDescription=node.GetDescription<RouteDescription::SuggestedLaneDescription>();
    laneDescription=node.GetDescription<RouteDescription::LaneDescription>();
  }

  void OnMotorwayLeave(VoiceMessageStruct::Type type,
                       const RouteDescription::MotorwayJunctionDescriptionRef& motorwayJunctionDescription,
                       const RouteDescription::DestinationDescriptionRef& destinationDescription,
                       const RouteDescription::NameDescriptionRef& fromDescription,
                       const RouteDescription::NameDescriptionRef& toDescription)
  {
    std::optional<std::string> exitRef = std::nullopt;
    if (motorwayJunctionDescription &&
        motorwayJunctionDescription->GetJunctionDescription() &&
        !motorwayJunctionDescription->GetJunctionDescription()->GetRef().empty()) {
        exitRef = motorwayJunctionDescription->GetJunctionDescription()->GetRef();
    }

    std::optional<std::string> destinationName;
    if (destinationDescription && !destinationDescription->GetDescription().empty()) {
      destinationName = destinationDescription->GetDescription();
    }

    auto GetRefOrName = [](const RouteDescription::NameDescriptionRef& nameDescription) -> std::optional<std::string> {
      if (!nameDescription) {
        return std::nullopt;
      }
      if (!nameDescription->GetRef().empty()) {
        return nameDescription->GetRef();
      }
      if (!nameDescription->GetName().empty()) {
        return nameDescription->GetName();
      }
      return std::nullopt;
    };

    auto ConstructMessage = [this, &type, &exitRef, &destinationName, &fromDescription, &toDescription, &GetRefOrName] {
      VoiceMessageStruct message{type, distance};
      message.exitRef = std::move(exitRef);
      message.destinationName = std::move(destinationName);
      message.fromName = GetRefOrName(fromDescription);;
      message.toName = GetRefOrName(toDescription);

      if (suggestedLaneDescription && laneDescription) {
        message.laneSuggestion =  VoiceMessageStruct::LaneSuggestion(
          laneDescription->GetLaneCount(),
          suggestedLaneDescription->GetFrom(),
          suggestedLaneDescription->GetTo()
        );
      }
      return message;
    };

    if (!nextMessage){
      nextMessage = ConstructMessage();
    } else if (!thenMessage){
      thenMessage = ConstructMessage();
    }
  }

  void OnMotorwayChange(const RouteDescription::MotorwayChangeDescriptionRef& motorwayChangeDescription,
                        const RouteDescription::MotorwayJunctionDescriptionRef& motorwayJunctionDescription,
                        [[maybe_unused]] const RouteDescription::DirectionDescriptionRef& directionDescription,
                        const RouteDescription::DestinationDescriptionRef& crossingDestinationDescription) override
  {
    assert(motorwayChangeDescription);
    if (motorwayChangeDescription->GetFromDescription() &&
        motorwayChangeDescription->GetToDescription() &&
        !motorwayChangeDescription->GetFromDescription()->GetRef().empty() &&
        motorwayChangeDescription->GetFromDescription()->GetRef() == motorwayChangeDescription->GetToDescription()->GetRef()) {
      // ignore motorway change with same ref, it usually indicates changes like: A7 "Bridge XYZ" -> A7 "Autobahn"
      // it is not useful information for the driver
      return;
    }
    OnMotorwayLeave(VoiceMessageStruct::Type::MotorwayChange, motorwayJunctionDescription, crossingDestinationDescription, motorwayChangeDescription->GetFromDescription(), motorwayChangeDescription->GetToDescription());
  }

  void OnMotorwayLeave(const RouteDescription::MotorwayLeaveDescriptionRef& motorwayLeaveDescription,
                       const RouteDescription::MotorwayJunctionDescriptionRef& motorwayJunctionDescription,
                       [[maybe_unused]] const RouteDescription::DirectionDescriptionRef& directionDescription,
                       [[maybe_unused]] const RouteDescription::NameDescriptionRef& nameDescription,
                       const RouteDescription::DestinationDescriptionRef& destinationDescription) override
  {
    assert(motorwayLeaveDescription);
    VoiceMessageStruct::Type type;
    auto move=motorwayLeaveDescription->GetDirection();
    if (move==RouteDescription::DirectionDescription::sharpLeft ||
        move==RouteDescription::DirectionDescription::left ||
        move==RouteDescription::DirectionDescription::slightlyLeft) {
      type=VoiceMessageStruct::Type::LeaveMotorwayLeft;
    } else if (move==RouteDescription::DirectionDescription::sharpRight ||
               move==RouteDescription::DirectionDescription::right ||
               move==RouteDescription::DirectionDescription::slightlyRight) {
      type=VoiceMessageStruct::Type::LeaveMotorwayRight;
    } else {
      type=VoiceMessageStruct::Type::LeaveMotorway;
    }

    OnMotorwayLeave(type, motorwayJunctionDescription, destinationDescription, motorwayLeaveDescription->GetFromDescription(), nullptr);
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
    using MessageType = VoiceMessageStruct::Type;

    if (!roundaboutEnter) {
      if (!nextMessage) {
        // we are on the roundabout right now, be silent until we leave it
        nextMessage = VoiceMessageStruct{MessageType::Silent, distance};
      }
      return;
    }

    // say the message before entering the roundabout
    if (!nextMessage){
      nextMessage = VoiceMessageStruct{MessageType::LeaveRoundabout, *roundaboutEnter};
      nextMessage.exitNumber = roundaboutLeaveDescription->GetExitCount();
    } else if (!thenMessage){
      thenMessage = VoiceMessageStruct{MessageType::LeaveRoundabout, *roundaboutEnter};
      thenMessage.exitNumber = roundaboutLeaveDescription->GetExitCount();
    }
    roundaboutEnter = std::nullopt;
  }

  void OnTargetReached(const osmscout::RouteDescription::TargetDescriptionRef& /*targetDescription*/) override
  {
    if (!nextMessage){
      using MessageType = VoiceMessageStruct::Type;
      if (distance - distanceFromStart < Meters(40)) {
        nextMessage = VoiceMessageStruct{MessageType::TargetReached, distance};
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

    using MessageType = VoiceMessageStruct::Type;
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
        nextMessage = VoiceMessageStruct{type, distance};
      } else if (!thenMessage) {
        thenMessage = VoiceMessageStruct{type, distance};
      }
    }
  }

  bool Continue() const override
  {
    return (stopAfter < Distance::Zero() || distance <= stopAfter) && (!thenMessage);
  }
};

void VoiceInstructionAgent::toSamples(std::vector<SampleVoiceInstructionMessage::VoiceSample> &samples, const VoiceMessageStruct &message, bool shortRoundaboutMessage)
{
  using VoiceSample = SampleVoiceInstructionMessage::VoiceSample;

  if (!shortRoundaboutMessage && message.type==VoiceMessageStruct::Type::LeaveRoundabout) {
    samples.push_back(VoiceSample::RbCross);
  }

  switch (message.type) {

    case VoiceMessageStruct::Type::LeaveRoundabout:
      switch (message.exitNumber) {
        case 1:
          samples.push_back(VoiceSample::RbExit1);
          break;
        case 2:
          samples.push_back(VoiceSample::RbExit2);
          break;
        case 3:
          samples.push_back(VoiceSample::RbExit3);
          break;
        case 4:
          samples.push_back(VoiceSample::RbExit4);
          break;
        case 5:
          samples.push_back(VoiceSample::RbExit5);
          break;
        case 6:
          samples.push_back(VoiceSample::RbExit6);
          break;
        default:
          // it is not correct, but what else we may say? (VoiceOfMarble project does not have samples for more than 6 exits)
          samples.push_back(VoiceSample::RbExit6);
          log.Warn() << "Roundabout exit number " << message.exitNumber << " is not handled!";
      }
      break;

    case VoiceMessageStruct::Type::TargetReached:
      samples.push_back(VoiceSample::Arrive);
      break;

    case VoiceMessageStruct::Type::SharpLeft:
      samples.push_back(VoiceSample::SharpLeft);
      break;
    case VoiceMessageStruct::Type::TurnLeft:
      samples.push_back(VoiceSample::TurnLeft);
      break;
    case VoiceMessageStruct::Type::StraightOn:
      samples.push_back(VoiceSample::Straight);
      break;
    case VoiceMessageStruct::Type::TurnRight:
      samples.push_back(VoiceSample::TurnRight);
      break;
    case VoiceMessageStruct::Type::SharpRight:
      samples.push_back(VoiceSample::SharpRight);
      break;

    case VoiceMessageStruct::Type::LeaveMotorway:
      samples.push_back(VoiceSample::MwExit);
      break;
    case VoiceMessageStruct::Type::LeaveMotorwayRight:
      samples.push_back(VoiceSample::MwExitRight);
      break;
    case VoiceMessageStruct::Type::LeaveMotorwayLeft:
      samples.push_back(VoiceSample::MwExitLeft);
      break;

    default:
      log.Error() << "Message type " << static_cast<int>(message.type) << " is not handled!";
      assert(false);
  }
}

std::vector<SampleVoiceInstructionMessage::VoiceSample> VoiceInstructionAgent::toSamples(const Distance &distanceFromStart,
                                                                                         const VoiceMessageStruct &message,
                                                                                         const VoiceMessageStruct &then)
{
  using VoiceSample = SampleVoiceInstructionMessage::VoiceSample;
  std::vector<SampleVoiceInstructionMessage::VoiceSample> samples;

  assert(message);

  if (message.type == VoiceMessageStruct::Type::Silent) {
    return samples;
  }

  // distance from our position to next message
  Distance nextMessageDistance = message.distance - distanceFromStart;
  double distanceInUnits = (units == DistanceUnitSystem::Metrics) ? nextMessageDistance.AsMeter() : nextMessageDistance.As<Yard>();

  if (distanceInUnits > 900){
    return samples;
  }

  // We are close to roundabout, we will use short roundabout message in such case.
  bool shortRoundaboutMessage = message.type == VoiceMessageStruct::Type::LeaveRoundabout &&
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

  toSamples(samples, message, shortRoundaboutMessage);
  if (then){
    auto thenDistance = then.distance - message.distance;
    if (thenDistance <= Meters(200)) { // ignore then message otherwise
      samples.push_back(VoiceSample::Then);
      toSamples(samples, then, true);
    }
  }
  return samples;
}

std::list<NavigationMessageRef> VoiceInstructionAgent::Process(const NavigationMessageRef& message)
{
  std::list<NavigationMessageRef> result;

  if (auto routeUpdateMessage = dynamic_cast<RouteUpdateMessage*>(message.get());
      routeUpdateMessage != nullptr){
    // reset state
    lastMessage.type=VoiceMessageStruct::Type::NoMessage;
    lastMessagePosition=Distance::Zero();
    announcedTriggers=0;
    vehicle=routeUpdateMessage->vehicle;
    if (ttsMessageGenerator) {
      ttsMessageGenerator->SetVehicle(vehicle);
    }
    return result;
  }

  if (auto voiceSetup = dynamic_cast<VoiceSetupMessage*>(message.get());
      voiceSetup != nullptr){
    voiceType=voiceSetup->type;
    if (voiceType==NavigationVoiceType::TextToSpeech) {
      if (!ttsMessageGenerator) {
        voiceType=NavigationVoiceType::None; // no generator set, do not generate voice instructions
      }
      if (!ttsMessageGenerator->SetLanguage(voiceSetup->languageCode)) {
        voiceType=NavigationVoiceType::None; // language is not supported, do not generate voice instructions
      }
    }
    return result;
  }

  auto positionMessage = dynamic_cast<PositionAgent::PositionMessage*>(message.get());
  if (positionMessage==nullptr || voiceType==NavigationVoiceType::None) {
    return result;
  }

  using VoiceSample = SampleVoiceInstructionMessage::VoiceSample;
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
    if (voiceType==NavigationVoiceType::VoiceOfMarble) {
      result.push_back(std::make_shared<SampleVoiceInstructionMessage>(
          positionMessage->timestamp,
          std::vector<VoiceSample>{VoiceSample::GpsFound}));
    } else {
      assert(voiceType==NavigationVoiceType::TextToSpeech);
      if (auto ttsMessage=ttsMessageGenerator->GenerateMessage(Distance::Zero(), VoiceMessageStruct(VoiceMessageStruct::Type::GpsFound, Distance::Zero()),VoiceMessageStruct())) {
        result.push_back(std::make_shared<TTSVoiceInstructionMessage>(
          positionMessage->timestamp, std::move(*ttsMessage)));
      }
    }
    prevGpsSignal = gpsSignal;
  } else if (prevGpsSignal && !gpsSignal && (now - lastSeenGpsSignal) > seconds(10)){
    // GpsLost
    if (voiceType==NavigationVoiceType::VoiceOfMarble) {
      result.push_back(std::make_shared<SampleVoiceInstructionMessage>(
          positionMessage->timestamp,
          std::vector<VoiceSample>{VoiceSample::GpsLost}));
    } else {
      assert(voiceType==NavigationVoiceType::TextToSpeech);
      if (auto ttsMessage=ttsMessageGenerator->GenerateMessage(Distance::Zero(), VoiceMessageStruct(VoiceMessageStruct::Type::GpsLost, Distance::Zero()),VoiceMessageStruct())) {
        result.push_back(std::make_shared<TTSVoiceInstructionMessage>(
          positionMessage->timestamp, std::move(*ttsMessage)));
      }
    }
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
  Distance nextMessageDistance = callback.nextMessage.distance - distanceFromStart;
  double distanceInUnits = (units == DistanceUnitSystem::Metrics) ? nextMessageDistance.AsMeter() : nextMessageDistance.As<Yard>();

  if (callback.nextMessage) {
    const std::vector<double> triggers = TriggerDistances(callback.nextMessage.type, voiceType, vehicle);

    // number of trigger distances already passed (triggers are sorted descending)
    size_t active = 0;
    while (active < triggers.size() && distanceInUnits <= triggers[active]) {
      ++active;
    }

    bool sameMessage = callback.nextMessage == lastMessage;
    if (!sameMessage) {
      announcedTriggers = 0;
    } else if ((distanceFromStart - lastMessagePosition) < Meters(-50)) {
      // position jumped back, announce again
      announcedTriggers = 0;
    }

    if (active > announcedTriggers) {
      if (voiceType==NavigationVoiceType::VoiceOfMarble) {
        result.push_back(std::make_shared<SampleVoiceInstructionMessage>(
          positionMessage->timestamp,
          toSamples(distanceFromStart, callback.nextMessage, callback.thenMessage)));
      } else {
        assert(voiceType==NavigationVoiceType::TextToSpeech);
        if (auto ttsMessage=ttsMessageGenerator->GenerateMessage(distanceFromStart, callback.nextMessage, callback.thenMessage)) {
          result.push_back(std::make_shared<TTSVoiceInstructionMessage>(
            positionMessage->timestamp, std::move(*ttsMessage)));
        }
      }
      announcedTriggers = active;
    }
    lastMessage=callback.nextMessage;
    lastMessagePosition=distanceFromStart;
  }

  return result;
}

std::vector<double> VoiceInstructionAgent::TriggerDistances(VoiceMessageStruct::Type type,
                                                            NavigationVoiceType voiceType,
                                                            Vehicle vehicle)
{
  using Type = VoiceMessageStruct::Type;
  // Distances (in meters or yards, depending on units) in descending order,
  // when the instruction should be voiced.
  // VoiceOfMarble has only limited set of recorded distances (up to 800),
  // TTS may use arbitrary distances.
  if (vehicle==vehicleBicycle) {
    switch (type) {
      case Type::TargetReached:
        return {40};
      default:
        return std::vector<double>{50, 10};
    }
  }
  if (vehicle==vehicleFoot) {
    switch (type) {
      case Type::TargetReached:
        return {40};
      default:
        return std::vector<double>{50, 10};
    }
  }
  // vehicleCar
  const bool tts = voiceType == NavigationVoiceType::TextToSpeech;
  switch (type) {
    case Type::LeaveMotorway:
    case Type::LeaveMotorwayLeft:
    case Type::LeaveMotorwayRight:
      return tts ? std::vector<double>{1500, 1000, 500} : std::vector<double>{800, 500, 200};
    case Type::MotorwayChange:
      // there is no recorded sample for it
      return tts ? std::vector<double>{500} : std::vector<double>{};
    case Type::TargetReached:
      return {40};
    case Type::LeaveRoundabout:
    case Type::SharpLeft:
    case Type::TurnLeft:
    case Type::StraightOn:
    case Type::TurnRight:
    case Type::SharpRight:
      return tts ? std::vector<double>{1000, 500, 150, 60} : std::vector<double>{800, 500, 150, 60};
    default:
      return {};
  }

}

}
