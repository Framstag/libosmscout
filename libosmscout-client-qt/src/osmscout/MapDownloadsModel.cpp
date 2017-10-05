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

#include <osmscout/MapDownloadsModel.h>
#include <osmscout/util/Logger.h>
#include <osmscout/OSMScoutQt.h>

MapDownloadsModel::MapDownloadsModel(QObject *parent):
  QAbstractListModel(parent){

  mapManager=OSMScoutQt::GetInstance().GetMapManager();
  connect(mapManager.get(), SIGNAL(downloadJobsChanged()), this, SLOT(onDownloadJobsChanged()));
  connect(mapManager.get(), SIGNAL(mapDownloadFails(QString)), this, SIGNAL(mapDownloadFails(QString)));
  onDownloadJobsChanged();
}

QString MapDownloadsModel::suggestedDirectory(QVariant mapVar, QString rootDirectory)
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

  if (mapVar.canConvert<AvailableMapsModelMap>()){
    AvailableMapsModelMap map=mapVar.value<AvailableMapsModelMap>();
    path+=QDir::separator();
    for (auto part:map.getPath()){
      path+=part+"-";
    }
    path+=map.getCreation().toString("yyyyMMdd-HHmmss");
  }
  return path;
}

void MapDownloadsModel::downloadMap(QVariant mapVar, QString dir)
{
  if (mapVar.canConvert<AvailableMapsModelMap>()){
    AvailableMapsModelMap map=mapVar.value<AvailableMapsModelMap>();
    
    mapManager->downloadMap(map, QDir(dir));
  }
}

QStringList MapDownloadsModel::getLookupDirectories()
{
  return mapManager->getLookupDirectories();
}

double MapDownloadsModel::getFreeSpace(QString dir)
{
#ifdef HAS_QSTORAGE
  QStorageInfo storage=QStorageInfo(QDir(dir));
  return storage.bytesAvailable();
#else
  return -1;
#endif
}

void MapDownloadsModel::onDownloadJobsChanged()
{
  beginResetModel();
  for (auto job:mapManager->getDownloadJobs()){
    connect(job, SIGNAL(downloadProgress()), this, SLOT(onDownloadProgress()));
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
  emit dataChanged(createIndex(0,0), createIndex(count-1,0),roles);
}

int MapDownloadsModel::rowCount(const QModelIndex &/*parent*/) const
{
  return mapManager->getDownloadJobs().size();
}

QVariant MapDownloadsModel::data(const QModelIndex &index, int role) const
{
  auto jobs=mapManager->getDownloadJobs();
  if (index.row()>=jobs.size()){
    return QVariant();
  }

  auto job=jobs.at(index.row());
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
    default:
      break;
  }
  return QVariant();
}

QHash<int, QByteArray> MapDownloadsModel::roleNames() const
{
  QHash<int, QByteArray> roles=QAbstractListModel::roleNames();

  roles[MapNameRole]="mapName";
  roles[TargetDirectoryRole]="targetDirectory";
  roles[ProgressRole]="progressRole";
  roles[ProgressDescriptionRole]="progressDescription";

  return roles;
}

Qt::ItemFlags MapDownloadsModel::flags(const QModelIndex &index) const
{
  if(!index.isValid()) {
    return Qt::ItemIsEnabled;
  }

  return QAbstractListModel::flags(index) | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}
