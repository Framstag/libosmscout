#ifndef OSMSCOUT_CLIENT_QT_TTSENGINE_H
#define OSMSCOUT_CLIENT_QT_TTSENGINE_H

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
#include <osmscoutclientqt/Voice.h>
#include <osmscoutclientqt/VoicePlayer.h>

#include <QObject>
#include <QString>

class QThread;

namespace osmscout {

/**
 * \ingroup QtAPI
 *
 * Abstract text-to-speech engine. It synthesizes spoken navigation
 * instructions and plays them through the provided VoiceCorePlayer.
 *
 * Speech synthesis may be a slow, synchronous operation (see PiperTTSEngine),
 * so the engine runs in its own background thread. All operations
 * (#initVoice, #prepareMessage, #playMessage) are executed asynchronously on
 * that thread, they should be invoked through a queued connection
 * (e.g. QMetaObject::invokeMethod with Qt::QueuedConnection).
 *
 * The background thread is created and started by the base constructor. When
 * it starts, #cleanCache() is invoked on the engine thread (default
 * implementation is empty, a derived class may override it to prune its cache).
 */
class OSMSCOUT_CLIENT_QT_API TTSEngine : public QObject {
  Q_OBJECT

protected:
  QThread         *thread; //!< engine background thread (owns itself, deleted on finish)
  VoiceCorePlayer *player; //!< audio player, not owned (lives in the creator thread)

public slots:
  /**
   * Initialize the engine for the given voice.
   * Runs asynchronously on the engine thread.
   */
  virtual void initVoice(const Voice &voice) = 0;

  /**
 * Synthesize the message to an audio file (cached in a temporary directory)
 * without playing it. Runs asynchronously on the engine thread.
 */
  virtual void prepareMessage(QString message) = 0;

  /**
   * Prepare the message (when it is not cached yet) and play it right away.
   * Runs asynchronously on the engine thread.
   */
  virtual void playMessage(QString message) = 0;

public:
  /**
   * @param player audio player used to play synthesized messages.
   *               It is not owned by the engine and has to outlive it.
   */
  explicit TTSEngine(VoiceCorePlayer *player);

  TTSEngine(const TTSEngine&) = delete;
  TTSEngine(TTSEngine&&) = delete;
  TTSEngine& operator=(const TTSEngine&) = delete;
  TTSEngine& operator=(TTSEngine&&) = delete;

  ~TTSEngine() override;

  virtual Voice getVoice() = 0;

protected:
  /**
   * Called on the engine thread when the background thread starts. The default
   * implementation does nothing; a derived class may override it to clean up
   * its on-disk cache. Runs on the engine thread.
   */
  virtual void cleanCache();
};

}

#endif // OSMSCOUT_CLIENT_QT_TTSENGINE_H



