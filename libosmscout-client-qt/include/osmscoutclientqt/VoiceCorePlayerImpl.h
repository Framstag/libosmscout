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

#include <osmscoutclientqt/VoicePlayer.h>

#include <QtGlobal>
#if QT_VERSION_MAJOR == 6

#include <mutex>
#include <QObject>
#include <QAudioOutput>
#include <QMediaPlayer>
#include <QList>

namespace osmscout {

class OSMSCOUT_LOCAL VoiceCorePlayerImpl : public VoicePlayer {
  Q_OBJECT

public:
  VoiceCorePlayerImpl& operator=(const VoiceCorePlayerImpl&) = delete;
  VoiceCorePlayerImpl& operator=(VoiceCorePlayerImpl&&) = delete;

  VoiceCorePlayerImpl(const VoiceCorePlayerImpl&) = delete;
  VoiceCorePlayerImpl(VoiceCorePlayerImpl&&) = delete;
  explicit VoiceCorePlayerImpl(QObject *parent) : VoicePlayer(parent) {
    output = new QAudioOutput();
    instance = new QMediaPlayer();
    instance->setAudioOutput(output);
    connect(instance, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)),
            this, SLOT(onMediaStatusChanged(QMediaPlayer::MediaStatus)));
  }

  ~VoiceCorePlayerImpl() override {
    if (instance)
      delete instance;
    if (output)
      delete output;
  }

  void clearQueue() override {
    std::lock_guard<std::recursive_mutex> g(mutex);
    if (playing) {
      stop();
    }
    currentIndex = 0;
    queue.clear();
  }

  void addToQueue(const QUrl &source) override {
    std::lock_guard<std::recursive_mutex> g(mutex);
    queue << source;
  }

  void setCurrentIndex(int index) override {
    std::lock_guard<std::recursive_mutex> g(mutex);
    if (index != currentIndex && index >= 0 && index < queue.size()) {
      currentIndex = index;
      if (playing) {
        instance->setSource(queue[index]);
        instance->play();
      }
    }
  }

  void play() override {
    std::lock_guard<std::recursive_mutex> g(mutex);
    if (!playing && currentIndex < queue.size()) {
      playing = true;
      emit playbackStateChanged(VoicePlayer::PlayingState);
      instance->setSource(queue[currentIndex]);
      instance->play();
    }
  }

  void stop() override {
    std::lock_guard<std::recursive_mutex> g(mutex);
    playing = false;
    instance->stop();
    emit playbackStateChanged(VoicePlayer::StoppedState);
  }

  int index() const override {
    std::lock_guard<std::recursive_mutex> g(mutex);
    return currentIndex;
  }
  int queueCount() const override {
    std::lock_guard<std::recursive_mutex> g(mutex);
    return queue.size();
  }

private slots:
  void onMediaStatusChanged(QMediaPlayer::MediaStatus status) {
    bool stopped = false;

    // start critical section
    {
      std::lock_guard<std::recursive_mutex> g(mutex);
      if (!playing) {
        return;
      }

      switch (status) {
      // on the end of media, play the next queued track or
      // set the playback state to stopped
      case QMediaPlayer::MediaStatus::EndOfMedia:
      {
        currentIndex += 1;
        if (currentIndex < queue.size()) {
          instance->setSource(queue[currentIndex]);
          instance->play();
        } else {
          stopped = true;
        }
        break;
      }
      // on failure, set the playback state to stopped
      case QMediaPlayer::MediaStatus::InvalidMedia:
      case QMediaPlayer::MediaStatus::NoMedia:
        stopped = true;
        break;
      default:
        break;
      }

      playing = !stopped;
    }
    // end critical section

    if (stopped) {
      emit playbackStateChanged(VoicePlayer::StoppedState);
    }
  }

private:
  mutable std::recursive_mutex mutex;
  QAudioOutput *output = nullptr;
  QMediaPlayer *instance = nullptr;
  QList<QUrl> queue;
  int currentIndex = 0;
  bool playing = false;
};

}

#elif QT_VERSION_MAJOR == 5

#include <QObject>
#include <QMediaPlayer>
#include <QMediaPlaylist>

namespace osmscout {

class OSMSCOUT_LOCAL VoiceCorePlayerImpl : public VoicePlayer {
  Q_OBJECT

public:
  VoiceCorePlayerImpl& operator=(const VoiceCorePlayerImpl&) = delete;
  VoiceCorePlayerImpl& operator=(VoiceCorePlayerImpl&&) = delete;

  VoiceCorePlayerImpl(const VoiceCorePlayerImpl&) = delete;
  VoiceCorePlayerImpl(VoiceCorePlayerImpl&&) = delete;
  explicit VoiceCorePlayerImpl(QObject *parent) : VoicePlayer(parent) {
    playlist = new QMediaPlaylist();
    instance = new QMediaPlayer();
    instance->setPlaylist(playlist);
    connect(instance, SIGNAL(stateChanged(QMediaPlayer::State)),
            this, SLOT(onStateChanged(QMediaPlayer::State)));
  }

  ~VoiceCorePlayerImpl() override {
    if (instance)
      delete instance;
    if (playlist)
      delete playlist;
  }

  void clearQueue() override { playlist->clear(); }
  void addToQueue(const QUrl &source) override { playlist->addMedia(source); }
  void setCurrentIndex(int index) override { playlist->setCurrentIndex(index); }

  void play() override { instance->play(); }
  void stop() override { instance->stop(); }

  int index() const override { return playlist->currentIndex(); }
  int queueCount() const override { return playlist->mediaCount(); }

private slots:
  void onStateChanged(QMediaPlayer::State newState) {
    switch (newState) {
    case QMediaPlayer::State::PlayingState:
      emit playbackStateChanged(VoicePlayer::PlayingState);
      break;
    case QMediaPlayer::State::StoppedState:
    case QMediaPlayer::State::PausedState:
      emit playbackStateChanged(VoicePlayer::StoppedState);
      break;
    default:
      break;
    }
  }

private:
  QMediaPlaylist *playlist = nullptr;
  QMediaPlayer *instance = nullptr;
};

}

#endif
