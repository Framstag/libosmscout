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

#include <osmscoutclientqt/InstalledMapsModel.h>
#include <osmscoutclientqt/OSMScoutQt.h>
#include <osmscoutclientqt/QtStdConverters.h>

#include <algorithm>

namespace osmscout {

InstalledMapsModel::InstalledMapsModel()
{
  mapManager=OSMScoutQt::GetInstance().GetMapManager();
  connect(this, &InstalledMapsModel::databaseListChanged,
          this, &InstalledMapsModel::onDatabaseListChanged);

  mapManager->databaseListChanged.Connect(databaseListChangedSlot);
  onDatabaseListChanged();
}

InstalledMapsModel::~InstalledMapsModel()
{
}

void InstalledMapsModel::onDatabaseListChanged()
{
  std::vector<MapDirectory> currentDirs=mapManager->GetDatabaseDirectories();

  std::stable_sort(currentDirs.begin(), currentDirs.end());

  // following process is little bit complicated, but we don't want to call
  // model reset - it breaks UI animations for changes

  // process removals
  QMap<QString, MapDirectory> currentDirMap;
  for (const auto& dir: currentDirs){
    currentDirMap[QString::fromStdString(dir.GetDirStr())] = dir;
  }

  bool deleteDone=false;
  while (!deleteDone){
    deleteDone=true;
    for (int row=0;row<dirs.size(); row++){
      if (!currentDirMap.contains(QString::fromStdString(dirs.at(row).GetDirStr()))){
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
  for (const auto& dir: dirs){
    oldDirMap[QString::fromStdString(dir.GetDirStr())] = dir;
  }

  for (size_t row = 0; row < currentDirs.size(); row++) {
    const auto& dir = currentDirs.at(row);
    if (!oldDirMap.contains(QString::fromStdString(dir.GetDirStr()))){
      beginInsertRows(QModelIndex(), row, row);
      dirs.insert(row, dir);
      endInsertRows();
      oldDirMap[QString::fromStdString(dir.GetDirStr())] = dir;
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
      return dir.HasMetadata() ? QString::fromStdString(dir.GetName()) : QString::fromStdString(dir.GetDirStr());
    case PathRole:
      return dir.HasMetadata() ? StringVectorToQStringList(dir.GetPath()) : QStringList();
    case DirectoryRole:
      return QString::fromStdString(dir.GetDirStr());
    case TimeRole:
      return dir.HasMetadata() ? TimestampToQDateTime(dir.GetCreation()) : QVariant();
    case ByteSizeRole:
      return qint64(dir.ByteSize());
    case SizeRole:
      return QString::fromStdString(osmscout::ByteSizeToString(double(dir.ByteSize())));
    case VersionRole:
      return dir.HasMetadata() ? dir.GetVersion() : 0;
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
  roles[ByteSizeRole]="byteSize";
  roles[SizeRole]="size";
  roles[VersionRole]="version";

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
      if (!dir.DeleteDatabase()){
        qWarning() << "Failed to remove " << QString::fromStdString(dir.GetDirStr());
        break;
      }
  }
  mapManager->LookupDatabases();
  return true;
}

bool InstalledMapsModel::deleteMap(int row)
{
  return removeRows(row, 1);
}

QVariant InstalledMapsModel::timeOfMap(const QStringList& path)
{
  if (path.empty()){
    return QVariant();
  }
  for (const auto &dir:dirs){
    if (dir.HasMetadata() && StringVectorToQStringList(dir.GetPath())==path){
      return TimestampToQDateTime(dir.GetCreation());
    }
  }
  return QVariant();
}
}
