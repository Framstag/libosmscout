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

namespace osmscout {

InstalledVoicesModel::InstalledVoicesModel()
{
  voiceManager=OSMScoutQt::GetInstance().GetVoiceManager();
  settings=OSMScoutQt::GetInstance().GetSettings();
  assert(voiceManager);
  assert(settings);

  connect(voiceManager.get(), &VoiceManager::reloaded,
      this, &InstalledVoicesModel::update);
  connect(settings.get(), &Settings::VoiceChanged,
      this, &InstalledVoicesModel::onVoiceChanged);

  update();
}

InstalledVoicesModel::~InstalledVoicesModel()
{}

void InstalledVoicesModel::update()
{
  beginResetModel();
  voices.clear();
  voices<<Voice(); // no-voice placeholder
  voices+=voiceManager->getInstalledVoices();
  endResetModel();
}

void InstalledVoicesModel::onVoiceChanged()
{
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
      return false; // TODO
    default:
      break;
  }
  return QVariant();
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
