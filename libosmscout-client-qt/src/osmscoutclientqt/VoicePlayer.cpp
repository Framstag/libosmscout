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
#include <osmscoutclientqt/VoiceCorePlayerImpl.h>

namespace osmscout {

VoicePlayer::VoicePlayer(QObject *parent) : QObject(parent) { }

VoiceCorePlayer::VoiceCorePlayer(QObject *parent) : VoicePlayer(parent) {
  player = new VoiceCorePlayerImpl(this);
  connect(player, SIGNAL(playbackStateChanged(VoicePlayer::PlaybackState)),
          this, SLOT(onStateChanged(VoicePlayer::PlaybackState)));
}

void VoiceCorePlayer::clearQueue() {
  player->clearQueue();
}

void VoiceCorePlayer::addToQueue(const QUrl &source) {
  player->addToQueue(source);
}

int VoiceCorePlayer::queueCount() const {
  return player->queueCount();
}

void VoiceCorePlayer::setCurrentIndex(int index) {
  player->setCurrentIndex(index);
}

void VoiceCorePlayer::play() {
  player->play();
}

void VoiceCorePlayer::stop() {
  player->stop();
}

int VoiceCorePlayer::index() const {
  return player->index();
}

VoicePlayer::PlaybackState VoiceCorePlayer::playbackState() const {
  return state;
}

void VoiceCorePlayer::onStateChanged(VoicePlayer::PlaybackState newState) {
  if (state != newState) {
    state = newState;
    emit playbackStateChanged(state);
  }
}

}
