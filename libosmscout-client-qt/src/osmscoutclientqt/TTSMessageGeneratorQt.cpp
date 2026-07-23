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

QString TTSMessageGeneratorQt::Phrase(const VoiceMessageStruct &message) const
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

    case Type::LeaveRbExit1:
      return Translate(QT_TRANSLATE_NOOP("TTSMessageGeneratorQt", "At the roundabout, take the first exit"));
    case Type::LeaveRbExit2:
      return Translate(QT_TRANSLATE_NOOP("TTSMessageGeneratorQt", "At the roundabout, take the second exit"));
    case Type::LeaveRbExit3:
      return Translate(QT_TRANSLATE_NOOP("TTSMessageGeneratorQt", "At the roundabout, take the third exit"));
    case Type::LeaveRbExit4:
      return Translate(QT_TRANSLATE_NOOP("TTSMessageGeneratorQt", "At the roundabout, take the fourth exit"));
    case Type::LeaveRbExit5:
      return Translate(QT_TRANSLATE_NOOP("TTSMessageGeneratorQt", "At the roundabout, take the fifth exit"));
    case Type::LeaveRbExit6:
      return Translate(QT_TRANSLATE_NOOP("TTSMessageGeneratorQt", "At the roundabout, take the sixth exit"));

    case Type::NoMessage:
    case Type::Silent:
    default:
      return {};
  }
}

std::optional<std::string> TTSMessageGeneratorQt::GenerateMessage(const VoiceMessageStruct &message,
                                                                  const VoiceMessageStruct &then)
{
  // TODO: distance in phrase...
  QString phrase = Phrase(message);
  if (phrase.isEmpty()) {
    // Nothing to say (NoMessage / Silent).
    return std::nullopt;
  }

  if (then) {
    // TODO: ignore when "then" phrase is too far...
    QString thenPhrase = Phrase(then);
    if (!thenPhrase.isEmpty()) {
      // %1 is the following maneuver, e.g. "Turn left, then turn right".
      phrase += Translate(QT_TRANSLATE_NOOP("TTSMessageGeneratorQt", ", then %1"))
                    .arg(thenPhrase);
    }
  }

  return phrase.toStdString();
}

}

