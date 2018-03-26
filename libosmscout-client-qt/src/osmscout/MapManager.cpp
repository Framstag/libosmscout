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

#include <osmscout/MapManager.h>
#include <osmscout/TypeConfig.h>
#include <osmscout/PersistentCookieJar.h>

#include <osmscout/util/Logger.h>
#include <osmscout/DBThread.h>

const char* MapDownloadJob::FILE_METADATA = "metadata.json";

MapDownloadJob::MapDownloadJob(QNetworkAccessManager *webCtrl,
                               AvailableMapsModelMap map,
                               QDir target,
                               bool replaceExisting):
  webCtrl(webCtrl), map(map), target(target),
  done(false), started(false), downloadedBytes(0),
  replaceExisting(replaceExisting)
{
}

MapDownloadJob::~MapDownloadJob()
{
  for (auto job:jobs){
    delete job;
  }
  jobs.clear();
}

void MapDownloadJob::start()
{
  QJsonObject mapMetadata;
  mapMetadata["name"] = map.getName();
  mapMetadata["map"] = map.getPath().join("/");
  mapMetadata["version"] = map.getVersion();
  mapMetadata["creation"] = (double)map.getCreation().toTime_t();

  QJsonDocument doc(mapMetadata);
  QFile metadataFile(target.filePath(FILE_METADATA));
  metadataFile.open(QFile::OpenModeFlag::WriteOnly);
  metadataFile.write(doc.toJson());
  metadataFile.close();
  if (metadataFile.error() != QFile::FileError::NoError){
    started = true;
    done = true;
    error = metadataFile.errorString();
    emit failed(metadataFile.errorString());
    return;
  }

  QStringList fileNames;
  fileNames << "bounding.dat"
            << "nodes.dat"
            << "areas.dat"
            << "ways.dat" 
            << "areanode.idx"
            << "areaarea.idx"
            << "areaway.idx" 
            << "areasopt.dat"
            << "waysopt.dat" 
            << "location.idx"
            << "water.idx"
            << "intersections.dat" 
            << "intersections.idx"
            << "router.dat"
            << "router2.dat"
            << "textloc.dat"
            << "textother.dat"
            << "textpoi.dat"
            << "textregion.dat"
            << "coverage.idx";

  // types.dat should be last, when download is interrupted,
  // directory is not recognized as valid map
  fileNames << "types.dat";

  for (auto fileName:fileNames){
    auto job=new FileDownloader(webCtrl, map.getProvider().getUri()+"/"+map.getServerDirectory()+"/"+fileName, target.filePath(fileName));
    connect(job, SIGNAL(finished(QString)), this, SLOT(onJobFinished()));
    connect(job, SIGNAL(error(QString, bool)), this, SLOT(onJobFailed(QString, bool)));
    connect(job, SIGNAL(writtenBytes(uint64_t)), this, SIGNAL(downloadProgress()));
    connect(job, SIGNAL(writtenBytes(uint64_t)), this, SLOT(onDownloadProgress(uint64_t)));
    jobs << job;
  }
  started=true;
  downloadNextFile();
}

void MapDownloadJob::onDownloadProgress(uint64_t)
{
  // reset error message
  error = "";
}

void MapDownloadJob::onJobFailed(QString errorMessage, bool recoverable){
  osmscout::log.Warn() << "Download failed with the error: "
                       << errorMessage.toStdString() << " "
                       << (recoverable? "(recoverable)": "(not recoverable)");

  if (recoverable){
    error = errorMessage;
    emit downloadProgress();
  }else{
    done = true;
    error = errorMessage;
    emit failed(errorMessage);
  }
}

void MapDownloadJob::onJobFinished()
{
  if (!jobs.isEmpty()) {
    jobs.first()->deleteLater();
    downloadedBytes += jobs.first()->getBytesDownloaded();
    jobs.pop_front();
  }
  
  downloadNextFile();
}

void MapDownloadJob::downloadNextFile()
{
  if (!jobs.isEmpty()) {
    jobs.first()->startDownload();
    emit downloadProgress();
  } else {
    done = true;
    emit finished();
  }
}

double MapDownloadJob::getProgress()
{
  double expected=expectedSize();
  uint64_t downloaded=downloadedBytes;
  for (auto job:jobs){
    downloaded+=job->getBytesDownloaded();
  }
  if (expected==0.0)
    return 0;
  return (double)downloaded/expected;
}

QString MapDownloadJob::getDownloadingFile()
{
  if (!jobs.isEmpty())
    return jobs.first()->getFileName();
  return "";
}

MapDirectory::MapDirectory(QDir dir):
    dir(dir)
{
  QStringList fileNames;
  fileNames << "bounding.dat"
            << "nodes.dat"
            << "areas.dat"
            << "ways.dat"
            << "areanode.idx"
            << "areaarea.idx"
            << "areaway.idx"
            << "areasopt.dat"
            << "waysopt.dat"
            << "location.idx"
            << "water.idx"
            << "intersections.dat"
            << "intersections.idx"
            << "router.dat"
            << "router2.dat"
            << "textloc.dat"
            << "textother.dat"
            << "textpoi.dat"
            << "textregion.dat"
            << "types.dat";
  // coverage.idx is optional, introduced after database version 16

  valid=true;
  for (const auto &fileName: fileNames) {
    valid &= dir.exists(fileName);
  }

  // metadata
  if (dir.exists(MapDownloadJob::FILE_METADATA)){
    QFile jsonFile(dir.filePath(MapDownloadJob::FILE_METADATA));
    jsonFile.open(QFile::OpenModeFlag::ReadOnly);
    QJsonDocument doc = QJsonDocument::fromJson(jsonFile.readAll());
    QJsonObject metadataObject = doc.object();
    if (metadataObject.contains("name") &&
                                    metadataObject.contains("map") &&
                                    metadataObject.contains("creation") ){
      name = metadataObject["name"].toString();
      path = metadataObject["map"].toString().split("/");
      creation.setTime_t(metadataObject["creation"].toDouble());
      metadata = true;
    }
  }
}

bool MapDirectory::deleteDatabase()
{
  valid=false;

  QStringList fileNames;
  fileNames << "bounding.dat"
            << "nodes.dat"
            << "areas.dat"
            << "ways.dat"
            << "areanode.idx"
            << "areaarea.idx"
            << "areaway.idx"
            << "areasopt.dat"
            << "waysopt.dat"
            << "location.idx"
            << "water.idx"
            << "intersections.dat"
            << "intersections.idx"
            << "router.dat"
            << "router2.dat"
            << "textloc.dat"
            << "textother.dat"
            << "textpoi.dat"
            << "textregion.dat"
            << "coverage.idx"
            << "types.dat"
            << MapDownloadJob::FILE_METADATA;

  bool result=true;
  for (const auto &fileName: fileNames) {
    if(dir.exists(fileName)){
      result&=dir.remove(fileName);
    }
  }
  QDir parent=dir;
  parent.cdUp();
  return result && parent.rmdir(dir.dirName());
}

MapManager::MapManager(QStringList databaseLookupDirs, SettingsRef settings):
  databaseLookupDirs(databaseLookupDirs)
{
  qDebug() << "MapManager ctor";
  webCtrl.setCookieJar(new PersistentCookieJar(settings));
  // we don't use disk cache here
}

void MapManager::lookupDatabases()
{
  databaseDirectories.clear();
  QSet<QString> uniqPaths;
  QList<QDir> databaseFsDirectories;

  for (QString lookupDir:databaseLookupDirs){
    QDirIterator dirIt(lookupDir, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
    while (dirIt.hasNext()) {
      dirIt.next();
      QFileInfo fInfo(dirIt.filePath());
      if (fInfo.isFile() && fInfo.fileName() == osmscout::TypeConfig::FILE_TYPES_DAT){
        MapDirectory mapDir(fInfo.dir());
        if (mapDir.isValid()) {
          qDebug() << "found database" << mapDir.getName() << ":" << fInfo.dir().absolutePath();
          if (!uniqPaths.contains(fInfo.canonicalFilePath())) {
            databaseDirectories << mapDir;
            databaseFsDirectories << mapDir.getDir();
            uniqPaths << fInfo.canonicalFilePath();
          }
        }
      }
    }
  }
  emit databaseListChanged(databaseFsDirectories);
}

MapManager::~MapManager(){
  for (auto job:downloadJobs){
    delete job;
  }
  downloadJobs.clear();
}

void MapManager::downloadMap(AvailableMapsModelMap map, QDir dir, bool replaceExisting)
{
  if (dir.exists()){
    MapDirectory mapDir(dir);
    if (mapDir.hasMetadata() &&
        !mapDir.isValid() &&
        mapDir.getPath() == map.getPath() &&
        mapDir.getCreation() == map.getCreation()) {
      // directory contains partial download
      // (contains downloader metadata, but not all required files)
      // TODO: continue partial download
    }
    qWarning() << "Directory already exists"<<dir.canonicalPath()<<"!";
    emit mapDownloadFails("Directory already exists");
    return;
  } else {
    if (!dir.mkpath(dir.path())) {
      qWarning() << "Can't create directory" << dir.canonicalPath() << "!";
      emit mapDownloadFails("Can't create directory");
      return;
    }
  }
#ifdef HAS_QSTORAGE
  QStorageInfo storage=QStorageInfo(dir);
  if (storage.bytesAvailable()<(double)map.getSize()){
    qWarning() << "Free space" << storage.bytesAvailable() << "bytes is less than map size ("<<map.getSize()<<")!";
  }
#endif
  
  auto job=new MapDownloadJob(&webCtrl, map, dir, replaceExisting);
  connect(job, SIGNAL(finished()), this, SLOT(onJobFinished()));
  connect(job, SIGNAL(failed(QString)), this, SLOT(onJobFailed(QString)));
  downloadJobs<<job;
  emit downloadJobsChanged();
  downloadNext();
}

void MapManager::downloadNext()
{
  for (auto job:downloadJobs){
    if (job->isDownloading()){
      return;
    }
  }  
  for (auto job:downloadJobs){
    job->start();
    break;
  }  
}

void MapManager::onJobFailed(QString /*errorMessage*/)
{
  onJobFinished();
}

void MapManager::onJobFinished()
{
  QList<MapDownloadJob*> finished;
  for (auto job:downloadJobs){
    if (job->isDone()){
      finished<<job;

      if (job->isReplaceExisting()){
        // if there is upgrade requested, delete old database with same (logical) path
        for (auto &mapDir:databaseDirectories) {
          if (mapDir.hasMetadata() &&
              mapDir.getPath() == job->getMapPath() &&
              mapDir.getDir().canonicalPath() != job->getDestinationDirectory().canonicalPath()) {

            qDebug() << "deleting map database" << mapDir.getName() << "after upgrade:"
                     << mapDir.getDir().canonicalPath();
            mapDir.deleteDatabase();
          }
        }
      }
    }
  }
  if (!finished.isEmpty()){
    lookupDatabases();
  }
  for (auto job:finished){
    downloadJobs.removeOne(job);
    emit downloadJobsChanged();
    delete job;
  }
  downloadNext();
}
