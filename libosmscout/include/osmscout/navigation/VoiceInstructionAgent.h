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
#include <osmscout/navigation/PositionAgent.h>

namespace osmscout {

enum class NavigationVoiceType: int
{
  None = 0,
  VoiceOfMarble = 1,
  TextToSpeech = 2
};

struct OSMSCOUT_API VoiceSetupMessage CLASS_FINAL : public NavigationMessage
{
  NavigationVoiceType type;
  std::string languageCode;

  VoiceSetupMessage(const Timestamp& timestamp, const NavigationVoiceType &type, const std::string &languageCode):
    NavigationMessage(timestamp), type(type), languageCode(languageCode)
  {}
};

struct OSMSCOUT_API SampleVoiceInstructionMessage CLASS_FINAL : public NavigationMessage
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

  inline SampleVoiceInstructionMessage(const Timestamp& timestamp, std::vector<VoiceSample> &&message):
    NavigationMessage(timestamp), message(message)
  {}
};

/** Some TTS engines may have high latency, this message signalize that message may be prepared, but not played yet. */
struct OSMSCOUT_API TTSVoiceInstructionPrepareMessage CLASS_FINAL : public NavigationMessage
{
  std::string message;

  inline TTSVoiceInstructionPrepareMessage(const Timestamp& timestamp, std::string &&message):
    NavigationMessage(timestamp), message(message)
  {}
};

/** This message should be played right away. */
struct OSMSCOUT_API TTSVoiceInstructionMessage CLASS_FINAL : public NavigationMessage
{
  std::string message;

  inline TTSVoiceInstructionMessage(const Timestamp& timestamp, std::string &&message):
    NavigationMessage(timestamp), message(message)
  {}
};

struct VoiceMessageStruct {
  enum class OSMSCOUT_API Type: int {
    NoMessage = 0,

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
    SharpRight,

    LeaveMotorway,
    LeaveMotorwayRight,
    LeaveMotorwayLeft,

    GpsLost,
    GpsFound,

    Silent
  };

  Type type{Type::NoMessage};
  Distance distance;

  VoiceMessageStruct() = default;
  VoiceMessageStruct(const VoiceMessageStruct&) = default;
  VoiceMessageStruct(VoiceMessageStruct &&) = default;

  VoiceMessageStruct(Type type, const Distance &distance):
    type{type}, distance{distance} {}

  ~VoiceMessageStruct() = default;

  VoiceMessageStruct &operator=(const VoiceMessageStruct&) = default;
  VoiceMessageStruct &operator=(VoiceMessageStruct&&) = default;

  explicit operator bool() const
  {
    return type != Type::NoMessage;
  }

  bool operator==(const VoiceMessageStruct &other) const
  {
    return type==other.type && distance==other.distance;
  }

  bool operator!=(const VoiceMessageStruct &other) const
  {
    return !(*this==other);
  }
};

/** Abstract class for generating template-based messages for TTS engine
 */
class OSMSCOUT_API TTSMessageGenerator
{
public:
  virtual ~TTSMessageGenerator() = default;

  /**
   * Set the language of the generated messages
   * @param languageCode
   * @return true if language is supported
   */
  virtual bool SetLanguage(const std::string &languageCode) = 0;

  virtual std::optional<std::string> GenerateMessage(const VoiceMessageStruct &message, const VoiceMessageStruct &then) = 0;
};

using TTSMessageGeneratorRef = std::shared_ptr<TTSMessageGenerator>;

class OSMSCOUT_API NoOpTTSMessageGenerator : public TTSMessageGenerator
{
public:
  ~NoOpTTSMessageGenerator() override = default;

  bool SetLanguage([[maybe_unused]] const std::string &languageCode) override
  {
    return false;
  };

  std::optional<std::string> GenerateMessage([[maybe_unused]] const VoiceMessageStruct &message, [[maybe_unused]] const VoiceMessageStruct &then) override
  {
    return std::nullopt;
  }
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
private:
  DistanceUnitSystem units{DistanceUnitSystem::Metrics};
  Vehicle vehicle{vehicleCar};
  NavigationVoiceType voiceType{NavigationVoiceType::None};
  TTSMessageGeneratorRef ttsMessageGenerator;

  // state used for triggering GpsFound / GpsLost messages
  bool prevGpsSignal{true};
  Timestamp lastSeenGpsSignal{Timestamp::min()};

  VoiceMessageStruct lastMessage;
  Distance lastMessagePosition; // where we trigger last message (it is before lastMessage.distance usually)

public:
  VoiceInstructionAgent(DistanceUnitSystem units, TTSMessageGeneratorRef ttsMessageGenerator):
    units{units}, ttsMessageGenerator(ttsMessageGenerator)
  {};

  ~VoiceInstructionAgent() override = default;

  std::list<NavigationMessageRef> Process(const NavigationMessageRef& message) override;

private:
  void toSamples(std::vector<SampleVoiceInstructionMessage::VoiceSample> &samples,
                 const VoiceMessageStruct::Type &messageType,
                 bool shortRoundaboutMessage);

  std::vector<SampleVoiceInstructionMessage::VoiceSample> toSamples(const Distance &distanceFromStart,
                                                                    const VoiceMessageStruct &message,
                                                                    const VoiceMessageStruct &then);
};
}

#endif //LIBOSMSCOUT_VOICEINSTRUCTIONAGENT_H
