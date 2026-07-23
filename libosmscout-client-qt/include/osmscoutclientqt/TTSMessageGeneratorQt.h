#ifndef OSMSCOUT_CLIENT_QT_TTSMESSAGEGENERATORQT_H
#define OSMSCOUT_CLIENT_QT_TTSMESSAGEGENERATORQT_H

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

#include <osmscout/navigation/VoiceInstructionAgent.h>

#include <osmscoutclientqt/ClientQtImportExport.h>

#include <QString>
#include <QTranslator>

#include <memory>
#include <optional>
#include <string>

namespace osmscout {

/**
 * \ingroup QtAPI
 *
 * Qt based implementation of TTSMessageGenerator producing
 * template-based messages for a TTS engine.
 *
 * Translations are resolved through a dedicated QTranslator instance
 * (see SetLanguage) instead of the globally installed application
 * translators. This makes it possible to generate voice instructions
 * in a language that is different from the language used by the rest of
 * the application. When no translation is available for a given source
 * string, the (English) source text is used as a fallback.
 */
class OSMSCOUT_CLIENT_QT_API TTSMessageGeneratorQt : public TTSMessageGenerator {
public:
  //!< Translation context used for all source strings (must match lupdate context).
  static constexpr const char *TranslationContext = "TTSMessageGeneratorQt";

private:
  QString translationDir;
  //!< Translator dedicated to voice instructions, independent of the app UI language.
  std::unique_ptr<QTranslator> translator;

public:
  explicit TTSMessageGeneratorQt(const QString &translationDir);
  ~TTSMessageGeneratorQt() override = default;

  bool SetLanguage(const std::string &language) override;

  std::optional<std::string> GenerateMessage(const VoiceMessageStruct &message,
                                             const VoiceMessageStruct &then) override;

private:
  /**
   * Translate a source string using the dedicated translator, falling back
   * to the source text when no translation is available.
   */
  QString Translate(const char *sourceText) const;

  /**
   * Build a translated phrase for a single maneuver. Returns an empty string
   * for messages that should not be spoken (NoMessage, Silent).
   */
  QString Phrase(const VoiceMessageStruct &message) const;
};

}

#endif //OSMSCOUT_CLIENT_QT_TTSMESSAGEGENERATORQT_H

