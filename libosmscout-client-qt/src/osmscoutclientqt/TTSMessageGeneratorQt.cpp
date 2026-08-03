/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2026 Lukas Karas

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

#include <osmscoutclientqt/TTSMessageGeneratorQt.h>

#include <osmscout/log/Logger.h>
#include <osmscout/util/Distance.h>

#include <QCoreApplication>
#include <QLocale>
#include <QStandardPaths>

namespace osmscout {

TTSMessageGeneratorQt::TTSMessageGeneratorQt(const QString &translationDir):
  translationDir(translationDir)
{
}

bool TTSMessageGeneratorQt::SetLanguage(const std::string &language)
{
  // Use a fresh translator instance. QTranslator is not reusable in a way that
  // would let us cleanly "unload" a previous translation on every platform,
  // so we simply replace it.
  translator = std::make_unique<QTranslator>();

  QString lang = QString::fromStdString(language);

  if (lang.isEmpty()) {
    log.Warn() << "Instruction language is emtpy"; // should not happen
    return false;
  }

  if (!translationDir.isEmpty() &&
      translator->load(lang, translationDir)) {
    log.Debug() << "Loaded voice instruction translation for language " << language;
    return true;
  }

  log.Warn() << "No voice instruction translation for language \"" << language
             << "\" (looked in " << translationDir.toStdString() << ")";
  translator.reset();
  return false;
}

void TTSMessageGeneratorQt::SetUnits(DistanceUnitSystem units)
{
  this->units = units;
}

void TTSMessageGeneratorQt::SetVehicle(Vehicle vehicle)
{
  this->vehicle = vehicle;
}

QString TTSMessageGeneratorQt::Translate(const char *sourceText) const
{
  if (translator) {
    QString translated = translator->translate(TranslationContext, sourceText);
    if (!translated.isEmpty()) {
      return translated;
    }
  }
  // Fallback to the (English) source text. Unlike QObject::tr(), a bare
  // QTranslator returns an empty string when no translation is found.
  return QString::fromUtf8(sourceText);
}

QString TTSMessageGeneratorQt::Phrase(const VoiceMessageStruct &message, bool shortRoundaboutMessage) const
{
  using Type = VoiceMessageStruct::Type;

  switch (message.type) {
    case Type::GpsFound:
      return Translate(QT_TRANSLATE_NOOP("TTSMessageGeneratorQt", "GPS signal found"));
    case Type::GpsLost:
      return Translate(QT_TRANSLATE_NOOP("TTSMessageGeneratorQt", "GPS signal lost"));

    case Type::TargetReached:
      return Translate(QT_TRANSLATE_NOOP("TTSMessageGeneratorQt", "You have reached your destination"));

    case Type::SharpLeft:
      return Translate(QT_TRANSLATE_NOOP("TTSMessageGeneratorQt", "Turn sharply left"));
    case Type::TurnLeft:
      return Translate(QT_TRANSLATE_NOOP("TTSMessageGeneratorQt", "Turn left"));
    case Type::StraightOn:
      return Translate(QT_TRANSLATE_NOOP("TTSMessageGeneratorQt", "Continue straight ahead"));
    case Type::TurnRight:
      return Translate(QT_TRANSLATE_NOOP("TTSMessageGeneratorQt", "Turn right"));
    case Type::SharpRight:
      return Translate(QT_TRANSLATE_NOOP("TTSMessageGeneratorQt", "Turn sharply right"));

    case Type::LeaveMotorway:
      return Translate(QT_TRANSLATE_NOOP("TTSMessageGeneratorQt", "Leave the motorway"));
    case Type::LeaveMotorwayLeft:
      return Translate(QT_TRANSLATE_NOOP("TTSMessageGeneratorQt", "Leave the motorway on the left"));
    case Type::LeaveMotorwayRight:
      return Translate(QT_TRANSLATE_NOOP("TTSMessageGeneratorQt", "Leave the motorway on the right"));

    // Roundabout exits: when close (or as a "then" maneuver) the
    // "At the roundabout" part is omitted.
    case Type::LeaveRbExit1:
      return shortRoundaboutMessage
        ? Translate(QT_TRANSLATE_NOOP("TTSMessageGeneratorQt", "Take the first exit"))
        : Translate(QT_TRANSLATE_NOOP("TTSMessageGeneratorQt", "At the roundabout, take the first exit"));
    case Type::LeaveRbExit2:
      return shortRoundaboutMessage
        ? Translate(QT_TRANSLATE_NOOP("TTSMessageGeneratorQt", "Take the second exit"))
        : Translate(QT_TRANSLATE_NOOP("TTSMessageGeneratorQt", "At the roundabout, take the second exit"));
    case Type::LeaveRbExit3:
      return shortRoundaboutMessage
        ? Translate(QT_TRANSLATE_NOOP("TTSMessageGeneratorQt", "Take the third exit"))
        : Translate(QT_TRANSLATE_NOOP("TTSMessageGeneratorQt", "At the roundabout, take the third exit"));
    case Type::LeaveRbExit4:
      return shortRoundaboutMessage
        ? Translate(QT_TRANSLATE_NOOP("TTSMessageGeneratorQt", "Take the fourth exit"))
        : Translate(QT_TRANSLATE_NOOP("TTSMessageGeneratorQt", "At the roundabout, take the fourth exit"));
    case Type::LeaveRbExit5:
      return shortRoundaboutMessage
        ? Translate(QT_TRANSLATE_NOOP("TTSMessageGeneratorQt", "Take the fifth exit"))
        : Translate(QT_TRANSLATE_NOOP("TTSMessageGeneratorQt", "At the roundabout, take the fifth exit"));
    case Type::LeaveRbExit6:
      return shortRoundaboutMessage
        ? Translate(QT_TRANSLATE_NOOP("TTSMessageGeneratorQt", "Take the sixth exit"))
        : Translate(QT_TRANSLATE_NOOP("TTSMessageGeneratorQt", "At the roundabout, take the sixth exit"));

    case Type::NoMessage:
    case Type::Silent:
    default:
      return {};
  }
}

QString TTSMessageGeneratorQt::DistancePhrase(double distanceInUnits) const
{
  // Bucket the distance the same way the Voice of Marble messages do.
  int value;
  if (distanceInUnits > 800) {
    value = 800;
  } else if (distanceInUnits > 700) {
    value = 700;
  } else if (distanceInUnits > 600) {
    value = 600;
  } else if (distanceInUnits > 500) {
    value = 500;
  } else if (distanceInUnits > 400) {
    value = 400;
  } else if (distanceInUnits > 300) {
    value = 300;
  } else if (distanceInUnits > 200) {
    value = 200;
  } else if (distanceInUnits > 100) {
    value = 100;
  } else if (distanceInUnits > 80) {
    value = 80;
  } else {
    value = 50;
  }

  QString unit = (units == DistanceUnitSystem::Metrics)
      ? Translate(QT_TRANSLATE_NOOP("TTSMessageGeneratorQt", "meters"))
      : Translate(QT_TRANSLATE_NOOP("TTSMessageGeneratorQt", "yards"));

  //: e.g. "After 300 meters/yards"
  return Translate(QT_TRANSLATE_NOOP("TTSMessageGeneratorQt", "After %1 %2"))
      .arg(value)
      .arg(unit);
}

std::optional<std::string> TTSMessageGeneratorQt::GenerateMessage(const Distance &distanceFromStart,
                                                                  const VoiceMessageStruct &message,
                                                                  const VoiceMessageStruct &then)
{
  using Type = VoiceMessageStruct::Type;

  if (!message) {
    // Nothing to say (NoMessage).
    return std::nullopt;
  }

  // distance from the current position to the maneuver
  Distance nextMessageDistance = message.distance - distanceFromStart;
  double distanceInUnits = (units == DistanceUnitSystem::Metrics)
      ? nextMessageDistance.AsMeter()
      : nextMessageDistance.As<Yard>();

  if (distanceInUnits > 900) {
    // too far away, nothing to announce yet
    return std::nullopt;
  }

  // when the roundabout is close, omit the "At the roundabout" part
  bool shortRoundaboutMessage = message.type >= Type::LeaveRbExit1 &&
                                message.type <= Type::LeaveRbExit6 &&
                                distanceInUnits < 120;

  QString maneuver = Phrase(message, shortRoundaboutMessage);
  if (maneuver.isEmpty()) {
    // nothing to say (Silent)
    return std::nullopt;
  }

  QString phrase = maneuver;

  // announce the distance, unless the maneuver is very close (only for cars)
  bool skipDistanceInformation = (distanceInUnits < 80 && vehicle == vehicleCar);
  if (!skipDistanceInformation) {
    QString distancePhrase = DistancePhrase(distanceInUnits);
    //: combine distance and maneuver, e.g. "After 300 meters, turn left"
    phrase = Translate(QT_TRANSLATE_NOOP("TTSMessageGeneratorQt", "%1, %2"))
                 .arg(distancePhrase, maneuver);
  }

  if (then) {
    // only announce the following maneuver when it is close to this one
    Distance thenDistance = then.distance - message.distance;
    if (thenDistance <= Meters(200)) {
      QString thenPhrase = Phrase(then, /*shortRoundaboutMessage*/ true);
      if (!thenPhrase.isEmpty()) {
        //: %1 is the following maneuver, e.g. "Turn left, then turn right".
        phrase = Translate(QT_TRANSLATE_NOOP("TTSMessageGeneratorQt", "%1, then %2"))
                     .arg(phrase)
                     .arg(thenPhrase);
      }
    }
  }
  phrase += "."; // always add end dot for better TTS phrase

  return phrase.toStdString();
}

}

