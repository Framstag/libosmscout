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
  Q_PROPERTY(TTSEngineState state READ getState NOTIFY stateChange)

public:
  /**
   * State of a TTSEngine, useful for showing synthesis progress in the UI
   * (initial synthesis of a message may take a noticeable amount of time).
   */
  enum class TTSEngineState {
    Idle,         //!< engine is idle, not synthesizing anything at the moment
    Initializing, //!< engine is initializing the voice (e.g. loading a model)
    Synthesizing, //!< engine is currently synthesizing a message
    Error,        //!< the last operation failed (see TTSEngine::error signal for details)
  };
  Q_ENUM(TTSEngineState);

protected:
  QThread         *thread; //!< engine background thread (owns itself, deleted on finish)
  TTSEngineState   state{TTSEngineState::Idle};  //!< current state of the engine

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

signals:
  void playAudioFilesRequest(const QList<QUrl> &audioFiles);

  /**
   * Emitted when the engine encounters an error (e.g. synthesis or voice
   * loading failure). @p message is a localized, human readable
   * description suitable for direct display in the UI.
   */
  void error(const QString &message);

  /**
   * Emitted whenever the engine's state changes, e.g. when it starts or
   * finishes synthesizing a message. Useful for showing synthesis progress
   * in the UI, as the initial synthesis of a message may take a while.
   */
  void stateChange(TTSEngineState state);

public:
  TTSEngine();

  TTSEngine(const TTSEngine&) = delete;
  TTSEngine(TTSEngine&&) = delete;
  TTSEngine& operator=(const TTSEngine&) = delete;
  TTSEngine& operator=(TTSEngine&&) = delete;

  ~TTSEngine() override;

  TTSEngineState getState() const
  {
    return state;
  }

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



