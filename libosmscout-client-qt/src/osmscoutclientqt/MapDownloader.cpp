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

#include <QDirIterator>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include <osmscoutclientqt/MapDownloader.h>
#include <osmscoutclientqt/PersistentCookieJar.h>
#include <osmscoutclientqt/QtStdConverters.h>

#include <osmscout/TypeConfig.h>
#include <osmscout/log/Logger.h>

namespace osmscout {

MapDownloadJob::MapDownloadJob(QNetworkAccessManager *webCtrl,
                               AvailableMapsModelMap map,
                               QDir target,
                               bool replaceExisting):
    DownloadJob(webCtrl, target, replaceExisting),
    map(map)
{
}

MapDownloadJob::~MapDownloadJob()
{
  if (started && !successful){
    // delete partial db
    clearJobs(); // need to remove temporary files before deleting db directory
    MapDirectory dir(target.absolutePath().toStdString());
    dir.DeleteDatabase();
  }
}

void MapDownloadJob::start()
{
  using namespace std::chrono;
  if (target.exists()){
    MapDirectory mapDir(target.absolutePath().toStdString());
    if (mapDir.HasMetadata() &&
        !mapDir.IsValid() &&
        StringVectorToQStringList(mapDir.GetPath()) == map.getPath() &&
        duration_cast<milliseconds>(mapDir.GetCreation().time_since_epoch()).count() == map.getCreation().toMSecsSinceEpoch()) {
      // directory contains partial download
      // (contains downloader metadata, but not all required files)
      if (!mapDir.DeleteDatabase()) {
        qWarning() << "Failed to clean up partial download" << target.canonicalPath()<<"!";
        onJobFailed("Directory already exists", false);
        return;
      }
    } else {
      qWarning() << "Directory already exists" << target.canonicalPath()<<"!";
      onJobFailed("Directory already exists", false);
      return;
    }
  }

  if (!target.mkpath(target.path())) {
    qWarning() << "Can't create directory" << target.canonicalPath() << "!";
    onJobFailed("Can't create directory", false);
    return;
  }

  started=true;
  QStorageInfo storage=QStorageInfo(target);
  if (storage.bytesAvailable() > 0 && (uint64_t)storage.bytesAvailable() < map.getSize()){
    qWarning() << "Free space" << storage.bytesAvailable() << "bytes is less than map size (" << map.getSize() << ")!";
    onJobFailed("Not enough space", false);
    return;
  }

  QJsonObject mapMetadata;
  mapMetadata["name"] = map.getName();
  mapMetadata["map"] = map.getPath().join("/");
  mapMetadata["version"] = map.getVersion();
#if QT_VERSION > QT_VERSION_CHECK(5, 8, 0) /* For compatibility with QT 5.6 */
  mapMetadata["creation"] = (double)map.getCreation().toSecsSinceEpoch();
#else
  mapMetadata["creation"] = (double)map.getCreation().toTime_t();
#endif

  QJsonDocument doc(mapMetadata);
  QFile metadataFile(target.filePath(MapDirectory::FileMetadata));
  metadataFile.open(QFile::OpenModeFlag::WriteOnly);
  metadataFile.write(doc.toJson());
  metadataFile.close();
  if (metadataFile.error() != QFile::FileError::NoError){
    done = true;
    error = metadataFile.errorString();
    emit failed(metadataFile.errorString());
    return;
  }

  QStringList fileNames = StringVectorToQStringList(MapDirectory::OptionalFiles()) + StringVectorToQStringList(MapDirectory::MandatoryFiles());

  DownloadJob::start(QString::fromStdString(map.getProvider().getUri())+"/"+map.getServerDirectory(), fileNames);
}

MapDownloader::MapDownloader(MapManagerRef mapManager, SettingsRef settings):
  mapManager(mapManager)
{
  osmscout::log.Debug() << "MapDownloader ctor";
  webCtrl.setCookieJar(new PersistentCookieJar(settings));
  // we don't use disk cache here
}

MapDownloader::~MapDownloader(){
  for (auto& job:downloadJobs){
    delete job;
  }
  downloadJobs.clear();
}

void MapDownloader::downloadMap(AvailableMapsModelMap map, QDir dir, bool replaceExisting)
{
  auto* job=new MapDownloadJob(&webCtrl, map, dir, replaceExisting);
  connect(job, &MapDownloadJob::finished, this, &MapDownloader::onJobFinished);
  connect(job, &MapDownloadJob::canceled, this, &MapDownloader::onJobFinished);
  connect(job, &MapDownloadJob::failed, this, &MapDownloader::onJobFailed);
  downloadJobs<<job;
  emit downloadJobsChanged();
  downloadNext();
}

void MapDownloader::downloadNext()
{
  for (const auto* job:downloadJobs){
    if (job->isDownloading()){
      return;
    }
  }
  for (const auto& job:downloadJobs){
    job->start();
    break;
  }
}

void MapDownloader::onJobFailed(QString errorMessage)
{
  onJobFinished();
  emit mapDownloadFails(errorMessage);
}

void MapDownloader::onJobFinished()
{
  QList<MapDownloadJob*> finished;
  for (auto *job:downloadJobs){
    if (job->isDone()){
      finished << job;

      if (job->isReplaceExisting() && job->isSuccessful()){
        // if there is upgrade requested, delete old db with same (logical) path
        mapManager->DeleteOther(QStringListToStringVector(job->getMapPath()), job->getDestinationDirectory().canonicalPath().toStdString());
      }
    }
  }
  if (!finished.isEmpty()){
    mapManager->LookupDatabases();
  }
  for (auto *job:finished){
    downloadJobs.removeOne(job);
    emit downloadJobsChanged();
    job->deleteLater();
  }
  downloadNext();
}
}
