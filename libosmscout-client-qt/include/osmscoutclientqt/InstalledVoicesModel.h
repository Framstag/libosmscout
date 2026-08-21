#ifndef OSMSCOUT_CLIENT_QT_INSTALLEDVOICESMODEL_H
#define OSMSCOUT_CLIENT_QT_INSTALLEDVOICESMODEL_H

/*
  OSMScout - a Qt backend for libosmscout and libosmscout-map
  Copyright (C) 2020 Lukas Karas

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

#include <osmscoutclient/Settings.h>

#include <osmscoutclientqt/VoiceManager.h>
#include <osmscoutclientqt/Voice.h>
#include <osmscoutclientqt/TTSEngine.h>
#include <osmscoutclientqt/ClientQtImportExport.h>

#include <QAbstractListModel>
#include <QList>
#include <QUrl>

#include <memory>

namespace osmscout {

class VoiceCorePlayer;
class TTSMessageGeneratorQt;

/**
 * Model providing access to currently installed voices on device
 * + entry for configuration without voice commands.
 *
 * This model suppose to be used in combo box.
 *
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API InstalledVoicesModel : public QAbstractListModel {
  Q_OBJECT

  //!< localized, human readable description of the current TTS engine state
  //!< (e.g. "Synthesizing voice sample…"), useful for showing synthesis
  //!< progress in the UI as the initial synthesis may take some time.
  Q_PROPERTY(QString ttsStateText READ getTTSStateText NOTIFY ttsStateChanged)

private:
  Slot<std::string> voiceDirSlot{
    [this](const std::string &dir){ onVoiceChanged(QString::fromStdString(dir)); }
  };

signals:
  void voiceChanged(const QString);

  // Piper TTS engine runs asynchronously on its own thread, these signals are
  // used to invoke it (via queued connections) from playTTSSample().
  void initTTSVoiceRequested(const Voice &voice);
  void playTTSMessageRequested(QString message);

  //!< emitted whenever getTTSStateText() changes
  void ttsStateChanged();

public slots:
  void update();
  void onVoiceChanged(const QString&);
  void playTTSAudio(const QList<QUrl> &audioFiles);

private slots:
  void onTTSStateChange(osmscout::TTSEngineState state);
  void onTTSError(const QString &message);

public:
  InstalledVoicesModel();

  ~InstalledVoicesModel() override;

  enum Roles {
    NameRole = Qt::UserRole, // name
    LangRole = Qt::UserRole + 1, //
    GenderRole = Qt::UserRole + 2, // male or female (for now :-))
    ValidRole = Qt::UserRole + 3, // true if it real voice, false when placeholder for no-voice configuration
    LicenseRole = Qt::UserRole + 4,
    AuthorRole = Qt::UserRole + 5,
    DescriptionRole = Qt::UserRole + 6,
    SelectedRole = Qt::UserRole + 7, // true when this voice is selected
    TypeRole = Qt::UserRole + 8, // VoiceOfMarble or Piper
  };
  Q_ENUM(Roles)

  Q_INVOKABLE int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  Q_INVOKABLE QVariant data(const QModelIndex &index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;
  Q_INVOKABLE Qt::ItemFlags flags(const QModelIndex &index) const override;

  Q_INVOKABLE void select(const QModelIndex &index);
  Q_INVOKABLE void playSample(const QModelIndex &index, const QStringList &sample);

  /**
   * Play a synthesized voice sample for a Piper TTS voice at the given index.
   * Unlike playSample() (which plays pre-recorded "Voice of Marble" ogg
   * files), this synthesizes a short, representative navigation message using
   * TTSMessageGeneratorQt and PiperTTSEngine. It is a no-op for non-Piper (or
   * invalid) voices, or when the library was built without libpiper support.
   */
  Q_INVOKABLE void playTTSSample(const QModelIndex &index);

  /**
   * Localized, human readable description of the current TTS engine state
   * (see osmscout::TTSEngineState). Reads as "Ready" before any Piper sample
   * was requested (or when the library was built without libpiper support).
   */
  QString getTTSStateText() const;

private:
  void EnsureTTSEngine();

private:
  QString voiceDir;
  QList<Voice> voices;
  VoiceManagerRef voiceManager;
  SettingsRef settings;

  // we setup QObject parents, objects are cleaned after Module destruction
  VoiceCorePlayer *mediaPlayer{nullptr};

  // Piper TTS sample playback: the engine lives in its own background
  // thread (see TTSEngine), created lazily on first use.
  TTSEngine *ttsEngine{nullptr};
  std::shared_ptr<TTSMessageGeneratorQt> ttsMessageGenerator;
  QString ttsMessageLanguage; // language currently loaded into ttsMessageGenerator

  // state of ttsEngine, mirrored here to expose it as a Qt property
  TTSEngineState ttsState{TTSEngineState::Idle};
  QString ttsErrorMessage; // last error reported by ttsEngine (not localized)
};
}
#endif //OSMSCOUT_CLIENT_QT_INSTALLEDVOICESMODEL_H
