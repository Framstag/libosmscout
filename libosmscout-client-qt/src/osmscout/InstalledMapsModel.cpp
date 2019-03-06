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

namespace osmscout {

InstalledMapsModel::InstalledMapsModel()
{
  mapManager=OSMScoutQt::GetInstance().GetMapManager();
  connect(mapManager.get(), &MapManager::databaseListChanged,
          this, &InstalledMapsModel::onDatabaseListChanged);
  connect(mapManager.get(), &MapManager::databaseListChanged,
          this, &InstalledMapsModel::databaseListChanged);
  onDatabaseListChanged();
}

InstalledMapsModel::~InstalledMapsModel()
{
}

void InstalledMapsModel::onDatabaseListChanged()
{
  QList<MapDirectory> currentDirs=mapManager->getDatabaseDirectories();

  qStableSort(currentDirs);

  // following process is little bit complicated, but we don't want to call
  // model reset - it breaks UI animations for changes

  // process removals
  QMap<QString, MapDirectory> currentDirMap;
  for (auto dir: currentDirs){
    currentDirMap[dir.getDir().absolutePath()] = dir;
  }

  bool deleteDone=false;
  while (!deleteDone){
    deleteDone=true;
    for (int row=0;row<dirs.size(); row++){
      if (!currentDirMap.contains(dirs.at(row).getDir().absolutePath())){
        beginRemoveRows(QModelIndex(), row, row);
        dirs.removeAt(row);
        endRemoveRows();
        deleteDone = false;
        break;
      }
    }
  }

  // process adds
  QMap<QString, MapDirectory> oldDirMap;
  for (auto dir: dirs){
    oldDirMap[dir.getDir().absolutePath()] = dir;
  }

  for (int row = 0; row < currentDirs.size(); row++) {
    auto dir = currentDirs.at(row);
    if (!oldDirMap.contains(dir.getDir().absolutePath())){
      beginInsertRows(QModelIndex(), row, row);
      dirs.insert(row, dir);
      endInsertRows();
      oldDirMap[dir.getDir().absolutePath()] = dir;
    }
  }
}

int InstalledMapsModel::rowCount(const QModelIndex &/*parent*/) const
{
  return dirs.size();
}

QVariant InstalledMapsModel::data(const QModelIndex &index, int role) const
{
  if (index.row() < 0 || index.row() >= dirs.size()){
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

Q_INVOKABLE bool InstalledMapsModel::removeRows(int fromRow, int count, const QModelIndex &/*parent*/)
{
  if (fromRow < 0 || count <= 0 || fromRow + count > dirs.size()){
    return false;
  }

  for (int row = fromRow; row < (fromRow + count); row++){
      auto dir=dirs.at(row);
      if (!dir.deleteDatabase()){
        qWarning() << "Failed to remove " << dir.getDir().absolutePath();
        break;
      }
  }
  mapManager->lookupDatabases();
  return true;
}

bool InstalledMapsModel::deleteMap(int row)
{
  return removeRows(row, 1);
}

QVariant InstalledMapsModel::timeOfMap(QStringList path)
{
  if (path.empty()){
    return QVariant();
  }
  for (const auto &dir:dirs){
    if (dir.hasMetadata() && dir.getPath()==path){
      return dir.getCreation();
    }
  }
  return QVariant();
}
}
