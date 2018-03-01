/*
  This source is part of the libosmscout-map library
  Copyright (C) 2018  Lukáš Karas

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

#include <osmscout/InstalledMapsModel.h>
#include <osmscout/OSMScoutQt.h>

InstalledMapsModel::InstalledMapsModel()
{
  mapManager=OSMScoutQt::GetInstance().GetMapManager();
  connect(mapManager.get(), SIGNAL(databaseListChanged(QList<QDir>)),
          this, SLOT(onDatabaseListChagned()));
  onDatabaseListChagned();
}

InstalledMapsModel::~InstalledMapsModel()
{
}

void InstalledMapsModel::onDatabaseListChagned()
{
  beginResetModel();
  endResetModel();
}

int InstalledMapsModel::rowCount(const QModelIndex &/*parent*/) const
{
  return mapManager->getDatabaseDirectories().size();
}

QVariant InstalledMapsModel::data(const QModelIndex &index, int role) const
{
  QList<MapDirectory> dirs=mapManager->getDatabaseDirectories();
  if (index.row() >= dirs.size()){
    return QVariant();
  }
  auto dir=dirs.at(index.row());
  // qDebug() << "dir path:" << dir.getPath();
  switch (role) {
    case Qt::DisplayRole:
    case NameRole:
      return dir.hasMetadata() ? dir.getName() : dir.getDir().dirName();
    case PathRole:
      return dir.hasMetadata() ? dir.getPath() : QStringList();
    case DirectoryRole:
      return dir.getDir().canonicalPath();
    case TimeRole:
      return dir.hasMetadata() ? dir.getCreation() : QVariant();
    default:
      break;
  }
  return QVariant();
}

QHash<int, QByteArray> InstalledMapsModel::roleNames() const
{
  QHash<int, QByteArray> roles=QAbstractListModel::roleNames();

  roles[NameRole]="name";
  roles[PathRole]="path";
  roles[DirectoryRole]="directory";
  roles[TimeRole]="time";

  return roles;
}

Qt::ItemFlags InstalledMapsModel::flags(const QModelIndex &index) const
{
  if(!index.isValid()) {
    return Qt::ItemIsEnabled;
  }

  return QAbstractListModel::flags(index) | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool InstalledMapsModel::deleteMap(int row)
{
  QList<MapDirectory> dirs=mapManager->getDatabaseDirectories();
  if (row < 0 || row >= dirs.size()){
    return false;
  }
  auto dir=dirs.at(row);
  bool result = dir.deleteDatabase();
  mapManager->lookupDatabases();
  return result;
}

QVariant InstalledMapsModel::timeOfMap(QStringList path)
{
  if (path.empty()){
    return QVariant();
  }
  QList<MapDirectory> dirs=mapManager->getDatabaseDirectories();
  for (const auto &dir:dirs){
    if (dir.hasMetadata() && dir.getPath()==path){
      return dir.getCreation();
    }
  }
  return QVariant();
}
