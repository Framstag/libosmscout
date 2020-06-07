#ifndef LIBOSMSCOUT_VOICEINSTRUCTIONAGENT_H
#define LIBOSMSCOUT_VOICEINSTRUCTIONAGENT_H

/*
 This source is part of the libosmscout library
 Copyright (C) 2020  Lukas Karas

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

#include <osmscout/navigation/Engine.h>
#include <osmscout/navigation/Agents.h>
#include <osmscout/navigation/DataAgent.h>
#include "PositionAgent.h"

namespace osmscout {

struct OSMSCOUT_API VoiceInstructionMessage CLASS_FINAL : public NavigationMessage
{
  enum class VoiceSample {
    After,
    AhExitLeft,
    AhExit,
    AhExitRight,
    AhFerry,
    AhKeepLeft,
    AhKeepRight,
    AhLeftTurn,
    AhRightTurn,
    AhUTurn,
    Arrive,
    BearLeft,
    BearRight,
    Depart,
    GpsFound,
    GpsLost,
    Charge,
    KeepLeft,
    KeepRight,
    LnLeft,
    LnRight,
    Marble,
    Meters,
    MwEnter,
    MwExitLeft,
    MwExit,
    MwExitRight,
    RbBack,
    RbCross,
    RbExit1,
    RbExit2,
    RbExit3,
    RbExit4,
    RbExit5,
    RbExit6,
    RbLeft,
    RbRight,
    RoadEnd,
    RouteCalculated,
    RouteDeviated,
    SharpLeft,
    SharpRight,
    Straight,
    TakeFerry,
    Then,
    TryUTurn,
    TurnLeft,
    TurnRight,
    UTurn,
    Yards,
    Take2ndLeft,
    Take2ndRight,
    Take3rdLeft,
    Take3rdRight,
    Distance50,
    Distance80,
    Distance100,
    Distance200,
    Distance300,
    Distance400,
    Distance500,
    Distance600,
    Distance700,
    Distance800,
  };

  std::vector<VoiceSample> message;

  inline VoiceInstructionMessage(const Timestamp& timestamp, std::vector<VoiceSample> &&message):
    NavigationMessage(timestamp), message(message)
  {}
};

/**
 * This agent prepares voice messages for concatenation voice synthesis.
 * It follows simple pattern described on "Voice of Marble" project page:
 * https://community.kde.org/Marble/VoiceOfMarble/Translations
 *
 * Message pattern is same for all languages. There is no need for translations,
 * just concatenate samples recorded in required language.
 *
 * There are existing samples recorded by Marble community that can be used:
 * https://marble.kde.org/speakers.php
 */
class OSMSCOUT_API VoiceInstructionAgent CLASS_FINAL : public NavigationAgent
{
public:

  enum class MessageType {
    NoMessage,

    LeaveRbExit1,
    LeaveRbExit2,
    LeaveRbExit3,
    LeaveRbExit4,
    LeaveRbExit5,
    LeaveRbExit6,

    TargetReached,

    SharpLeft,
    TurnLeft,
    StraightOn,
    TurnRight,
    SharpRight

  };

  struct MessageStruct {
    MessageType type{MessageType::NoMessage};
    Distance distance;

    MessageStruct() = default;
    MessageStruct(const MessageStruct&) = default;
    MessageStruct(MessageStruct &&) = default;

    MessageStruct(MessageType type, const Distance &distance):
      type{type}, distance{distance} {}

    ~MessageStruct() = default;

    MessageStruct &operator=(const MessageStruct&) = default;
    MessageStruct &operator=(MessageStruct&&) = default;

    operator bool() const
    {
      return type != MessageType::NoMessage;
    }

    bool operator==(const MessageStruct &other) const
    {
      return type==other.type && distance==other.distance;
    }

    bool operator!=(const MessageStruct &other) const
    {
      return !(*this==other);
    }
  };

private:
  Units units{Units::Metrics};

  // state used for triggering GpsFound / GpsLost messages
  bool prevGpsSignal{true};
  Timestamp lastSeenGpsSignal{Timestamp::min()};

  MessageStruct lastMessage;
  Distance lastMessagePosition; // where we trigger last message (it is before lastMessage.disntace usually)

public:
  inline VoiceInstructionAgent(Units units):
    units{units}
  {};

  ~VoiceInstructionAgent() override = default;

  std::list<NavigationMessageRef> Process(const NavigationMessageRef& message) override;

private:
  void toSamples(std::vector<VoiceInstructionMessage::VoiceSample> &samples,
                 const MessageType &messageType);

  std::vector<VoiceInstructionMessage::VoiceSample> toSamples(const Distance &distanceFromStart,
                                                              const MessageStruct &message,
                                                              const MessageStruct &then);
};
}

#endif //LIBOSMSCOUT_VOICEINSTRUCTIONAGENT_H
