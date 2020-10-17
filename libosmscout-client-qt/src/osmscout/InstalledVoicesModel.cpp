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

#include <osmscout/InstalledVoicesModel.h>
#include <osmscout/OSMScoutQt.h>

#include <algorithm>

namespace osmscout {

InstalledVoicesModel::InstalledVoicesModel()
{
  voiceManager=OSMScoutQt::GetInstance().GetVoiceManager();
  settings=OSMScoutQt::GetInstance().GetSettings();
  assert(voiceManager);
  assert(settings);

  connect(voiceManager.get(), &VoiceManager::reloaded,
      this, &InstalledVoicesModel::update);
  connect(settings.get(), &Settings::VoiceDirChanged,
          this, &InstalledVoicesModel::onVoiceChanged);
  connect(settings.get(), &Settings::VoiceDirChanged,
          this, &InstalledVoicesModel::voiceChanged);

  voiceDir = settings->GetVoiceDir();
  update();
}

InstalledVoicesModel::~InstalledVoicesModel()
{}

namespace {
bool voiceItemLessThan(const Voice &i1, const Voice &i2)
{
  if (i1.getLang() != i2.getLang()) {
    return i1.getLang().localeAwareCompare(i2.getLang()) < 0;
  }
  return i1.getName().localeAwareCompare(i2.getName()) < 0;
}
}

void InstalledVoicesModel::update()
{
  beginResetModel();
  voices.clear();
  voices<<Voice(); // no-voice placeholder
  QList<Voice> installedVoices=voiceManager->getInstalledVoices();
  std::sort(installedVoices.begin(), installedVoices.end(), voiceItemLessThan);
  voices+=installedVoices;
  endResetModel();
}

void InstalledVoicesModel::onVoiceChanged(const QString& dir)
{
  voiceDir = dir;
  if (!QDir(voiceDir).exists()){
    voiceDir.clear(); // disable voice
  }

  QVector<int> roles;
  roles<<SelectedRole;
  emit dataChanged(createIndex(0,0), createIndex(voices.size()-1,0),roles);
}

int InstalledVoicesModel::rowCount(const QModelIndex &/*parent*/) const
{
  return voices.size();
}

QVariant InstalledVoicesModel::data(const QModelIndex &index, int role) const
{
  if (index.row() < 0 || index.row() >= voices.size()){
    return QVariant();
  }
  auto voice=voices.at(index.row());
  switch (role) {
    case Qt::DisplayRole:
    case NameRole:
      return voice.getName();
    case LangRole:
      return voice.getLang();
    case GenderRole:
      return voice.getGender();
    case ValidRole:
      return voice.isValid();
    case LicenseRole:
      return voice.getLicense();
    case AuthorRole:
      return voice.getAuthor();
    case DescriptionRole:
      return voice.getDescription();
    case SelectedRole:
      return (voiceDir.isEmpty() && !voice.isValid()) ||
             (voiceDir == voice.getDir().absolutePath());
    default:
      break;
  }
  return QVariant();
}

void InstalledVoicesModel::select(const QModelIndex &index)
{
  if (index.row() < 0 || index.row() >= voices.size()){
    return;
  }
  auto voice=voices.at(index.row());
  settings->SetVoiceDir(voice.getDir().absolutePath());
}

void InstalledVoicesModel::playSample(const QModelIndex &index, const QStringList &sample)
{
  if (index.row() < 0 || index.row() >= voices.size()){
    return;
  }
  auto voice=voices.at(index.row());
  if (!voice.isValid() || !voice.getDir().exists()){
    return;
  }

  if (mediaPlayer==nullptr){
    assert(currentPlaylist==nullptr);
    mediaPlayer = new QMediaPlayer(this);
    currentPlaylist = new QMediaPlaylist(mediaPlayer);
    mediaPlayer->setPlaylist(currentPlaylist);
  }

  currentPlaylist->clear();

  for (const auto& file : sample){
    auto sampleUrl = QUrl::fromLocalFile(voice.getDir().path() + QDir::separator() + file);
    qDebug() << "Adding to playlist:" << sampleUrl;
    currentPlaylist->addMedia(sampleUrl);
  }

  currentPlaylist->setCurrentIndex(0);
  mediaPlayer->play();
}

QHash<int, QByteArray> InstalledVoicesModel::roleNames() const
{
  QHash<int, QByteArray> roles=QAbstractListModel::roleNames();

  roles[NameRole]="name";
  roles[LangRole]="lang";
  roles[GenderRole]="gender";
  roles[ValidRole]="valid";
  roles[SelectedRole]="selected";

  return roles;
}

Qt::ItemFlags InstalledVoicesModel::flags(const QModelIndex &index) const
{
  if(!index.isValid()) {
    return Qt::ItemIsEnabled;
  }

  return QAbstractListModel::flags(index) | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

}
