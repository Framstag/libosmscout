/*
  OSMScout - a Qt backend for libosmscout and libosmscout-map
  Copyright (C) 2016 Lukas Karas

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

#include <osmscoutclientqt/MapDownloadsModel.h>
#include <osmscout/log/Logger.h>
#include <osmscoutclientqt/OSMScoutQt.h>

namespace osmscout {

MapDownloadsModel::MapDownloadsModel(QObject *parent):
  QAbstractListModel(parent){

  mapManager=OSMScoutQt::GetInstance().GetMapManager();
  connect(mapManager.get(), &MapManager::downloadJobsChanged, this, &MapDownloadsModel::onDownloadJobsChanged);
  connect(mapManager.get(), &MapManager::mapDownloadFails, this, &MapDownloadsModel::mapDownloadFails);
  onDownloadJobsChanged();
}

QString MapDownloadsModel::suggestedDirectory(QObject *obj, QString rootDirectory)
{
  auto mapManager=OSMScoutQt::GetInstance().GetMapManager();
  auto directories=mapManager->getLookupDirectories();
  auto it=directories.begin();
  QString path=rootDirectory;
  if (path==""){
    path=".";
    if (it!=directories.end()){
      path=*it;
    }
  }

  const AvailableMapsModelMap *map=dynamic_cast<const AvailableMapsModelMap*>(obj);
  if (map!=nullptr){
    path+=QDir::separator();
    for (const auto& part:map->getPath()){
      path+=part+"-";
    }
    path+=map->getCreation().toString("yyyyMMdd-HHmmss");
    return path;
  }

  qWarning() << obj << "can't be converted to AvailableMapsModelMap";
  return path;
}

void MapDownloadsModel::downloadMap(QObject *obj, QString dir)
{
  qDebug() << "request to download map:" << obj << "to" << dir;
  const AvailableMapsModelMap *map=dynamic_cast<const AvailableMapsModelMap*>(obj);
  if (map!=nullptr){
    mapManager->downloadMap(*map, QDir(dir));
  }else{
    qWarning() << obj << "can't be converted to AvailableMapsModelMap";
  }
}

QStringList MapDownloadsModel::getLookupDirectories()
{
  return mapManager->getLookupDirectories();
}

double MapDownloadsModel::getFreeSpace(QString dir)
{
  QStorageInfo storage=QStorageInfo(QDir(dir));
  return storage.bytesAvailable();
}

void MapDownloadsModel::onDownloadJobsChanged()
{
  beginResetModel();
  for (auto *job:mapManager->getDownloadJobs()){
    connect(job, &MapDownloadJob::downloadProgress, this, &MapDownloadsModel::onDownloadProgress);
  }
  endResetModel();
}

void MapDownloadsModel::onDownloadProgress()
{
  int count=mapManager->getDownloadJobs().size();
  if (count==0)
    return;
  QVector<int> roles;
  roles<<ProgressRole;
  roles<<ProgressDescriptionRole;
  roles<<ErrorStringRole;
  emit dataChanged(createIndex(0,0), createIndex(count-1,0),roles);
}

int MapDownloadsModel::rowCount(const QModelIndex &/*parent*/) const
{
  return mapManager->getDownloadJobs().size();
}

QVariant MapDownloadsModel::data(const QModelIndex &index, int role) const
{
  auto jobs=mapManager->getDownloadJobs();
  if (index.row() < 0 || index.row()>=jobs.size()){
    return QVariant();
  }

  auto *job=jobs.at(index.row());
  switch (role) {
    case Qt::DisplayRole:
    case MapNameRole:
      return job->getMapName();
    case TargetDirectoryRole:
      return job->getDestinationDirectory().path();
    case ProgressRole:
      return job->getProgress();
    case ProgressDescriptionRole:
      return job->getDownloadingFile();
    case ErrorStringRole:
      return job->getError();
    default:
      break;
  }
  return QVariant();
}

void MapDownloadsModel::cancel(int row)
{
  auto jobs=mapManager->getDownloadJobs();
  if (row < 0 || row >= jobs.size()){
    return;
  }

  auto *job = jobs.at(row);
  qDebug() << "Cancel downloading:" << job->getMapName();
  job->cancel();
}

QHash<int, QByteArray> MapDownloadsModel::roleNames() const
{
  QHash<int, QByteArray> roles=QAbstractListModel::roleNames();

  roles[MapNameRole]="mapName";
  roles[TargetDirectoryRole]="targetDirectory";
  roles[ProgressRole]="progressRole";
  roles[ProgressDescriptionRole]="progressDescription";
  roles[ErrorStringRole]="errorString";

  return roles;
}

Qt::ItemFlags MapDownloadsModel::flags(const QModelIndex &index) const
{
  if(!index.isValid()) {
    return Qt::ItemIsEnabled;
  }

  return QAbstractListModel::flags(index) | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}
}
