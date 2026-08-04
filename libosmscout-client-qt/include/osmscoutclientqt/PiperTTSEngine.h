#ifndef OSMSCOUT_CLIENT_QT_PIPERTTSENGINE_H
#define OSMSCOUT_CLIENT_QT_PIPERTTSENGINE_H

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

#include <osmscoutclientqt/ClientQtImportExport.h>
#include <osmscoutclientqt/TTSEngine.h>

#include <QDir>
#include <QHash>
#include <QString>

// forward declaration of the opaque Piper synthesizer, so that piper.h is not
// required by users of this header
struct piper_synthesizer;

namespace osmscout {

/**
 * \ingroup QtAPI
 *
 * Piper (libpiper) based text-to-speech engine.
 *
 * Synthesis is performed with the (synchronous) Piper API on the engine
 * background thread. Each message is rendered into a WAV file placed in the
 * per-application cache directory (e.g. ~/.cache/... on Unix) and cached, so
 * repeated messages are synthesized only once. Playback is delegated to the
 * VoiceCorePlayer.
 *
 * Support for Piper is optional; this class is only compiled and available
 * when the library was built with libpiper (see the CMake / Meson build
 * files). When libpiper is missing, NavigationModule simply keeps its TTS
 * engine unset.
 */
class OSMSCOUT_CLIENT_QT_API PiperTTSEngine : public TTSEngine {
  Q_OBJECT

public:
  /**
   * @param player audio player used to play synthesized messages.
   * @param espeakDataDir path to the espeak-ng data directory required by Piper.
   */
  explicit PiperTTSEngine(const QString &espeakDataDir);
  ~PiperTTSEngine() override;

  void initVoice(const Voice &voice) override;
  Voice getVoice() override;
  void prepareMessage(QString message) override;
  void playMessage(QString message) override;

protected:
  /**
   * Remove the oldest synthesized WAV files when the cache directory exceeds
   * the size limit. Called on the engine thread when the engine thread starts.
   */
  void cleanCache() override;

private:
  /**
   * Synthesize the message to a cached WAV file and return its path.
   * Returns an empty string on failure. Has to be called on the engine thread.
   */
  QString prepare(const QString &message);

  /**
   * Local path of the WAV file used to cache the given message. The file name
   * is derived from a hash of both the current voice and the message, so that
   * caches of different voices do not collide.
   */
  QString wavFilePath(const QString &message) const;

  /**
   * Release the Piper synthesizer (if any).
   */
  void closeSynthesizer();

private:
  QDir    cacheDir;       //!< cache directory holding synthesized WAV files
  QString espeakDataPath; //!< path to the espeak-ng data directory required by Piper
  Voice voice;
  QString voiceId;        //!< identifier of the loaded voice (name + directory), part of the sample hash

  piper_synthesizer *synthesizer{nullptr}; //!< active Piper synthesizer, nullptr when no voice is loaded

  QHash<QString, QString> cache; //!< message -> WAV file path
};

}

#endif // OSMSCOUT_CLIENT_QT_PIPERTTSENGINE_H








