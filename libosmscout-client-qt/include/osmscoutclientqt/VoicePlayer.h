#ifndef OSMSCOUT_CLIENT_QT_VOICE_PLAYER_H
#define OSMSCOUT_CLIENT_QT_VOICE_PLAYER_H

/*
  OSMScout - a Qt backend for libosmscout and libosmscout-map
  Copyright (C) 2024 Jean-Luc Barriere

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

#include <QObject>
#include <QUrl>

namespace osmscout {

/**
 * \ingroup QtAPI
 *
 * Defines the abstract interface for the voice player
 */
class OSMSCOUT_CLIENT_QT_API VoicePlayer : public QObject {
  Q_OBJECT

public:
  enum PlaybackState {
    StoppedState = 0,
    PlayingState,
  };

  explicit VoicePlayer(QObject *parent);
  /**
   * @brief clear the playlist
   */
  virtual void clearQueue() = 0;
  /**
   * @brief add a track to the playlist
   * @param source url of the audio track
   */
  virtual void addToQueue(const QUrl &source) = 0;
  /**
   * @brief set the track index to play
   * @param index position in playlist
   */
  virtual void setCurrentIndex(int index) = 0;
  /**
   * @brief start playing from current index
   */
  virtual void play() = 0;
  /**
   * @brief stop playing
   */
  virtual void stop() = 0;
  /**
   * @brief returns the current index
   * @return position in playlist
   */
  virtual int index() const = 0;
  /**
   * @brief returns the track count in the playlist
   * @return count
   */
  virtual int queueCount() const = 0;

signals:
  void playbackStateChanged(VoicePlayer::PlaybackState state);
};

/**
 * \ingroup QtAPI
 *
 * Provides the default voice player
 */
class OSMSCOUT_CLIENT_QT_API VoiceCorePlayer : public VoicePlayer {
  Q_OBJECT

public:
  VoiceCorePlayer& operator=(const VoiceCorePlayer&) = delete;
  VoiceCorePlayer& operator=(VoiceCorePlayer&&) = delete;

  VoiceCorePlayer(const VoiceCorePlayer&) = delete;
  VoiceCorePlayer(VoiceCorePlayer&&) = delete;
  explicit VoiceCorePlayer(QObject *parent);
  ~VoiceCorePlayer() override = default;

  void clearQueue() override;
  void addToQueue(const QUrl &source) override;
  void setCurrentIndex(int index) override;

  void play() override;
  void stop() override;

  int index() const override;
  int queueCount() const override;

  /**
   * @brief returns the current state of playback
   * @return playback state
   */
  PlaybackState playbackState() const;

private slots:
  void onStateChanged(VoicePlayer::PlaybackState newState);

private:
  VoicePlayer *player;
  PlaybackState state{StoppedState};
};

}

#endif /* OSMSCOUT_CLIENT_QT_VOICE_PLAYER_H */
