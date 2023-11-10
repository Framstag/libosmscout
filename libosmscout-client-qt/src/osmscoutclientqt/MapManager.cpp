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

#include <osmscoutclientqt/MapManager.h>
#include <osmscoutclientqt/PersistentCookieJar.h>

#include <osmscout/TypeConfig.h>
#include <osmscout/log/Logger.h>

namespace osmscout {

const char* MapDownloadJob::FILE_METADATA = "metadata.json";

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
    MapDirectory dir(target);
    dir.deleteDatabase();
  }
}

void MapDownloadJob::start()
{
  if (target.exists()){
    MapDirectory mapDir(target);
    if (mapDir.hasMetadata() &&
        !mapDir.isValid() &&
        mapDir.getPath() == map.getPath() &&
        mapDir.getCreation() == map.getCreation()) {
      // directory contains partial download
      // (contains downloader metadata, but not all required files)
      if (!mapDir.deleteDatabase()) {
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
  QFile metadataFile(target.filePath(FILE_METADATA));
  metadataFile.open(QFile::OpenModeFlag::WriteOnly);
  metadataFile.write(doc.toJson());
  metadataFile.close();
  if (metadataFile.error() != QFile::FileError::NoError){
    done = true;
    error = metadataFile.errorString();
    emit failed(metadataFile.errorString());
    return;
  }

  QStringList fileNames = MapDirectory::optionalFiles() + MapDirectory::mandatoryFiles();

  DownloadJob::start(QString::fromStdString(map.getProvider().getUri())+"/"+map.getServerDirectory(), fileNames);
}

MapDirectory::MapDirectory(QDir dir):
    dir(dir)
{
  QStringList fileNames = mandatoryFiles();
  // coverage.idx is optional, introduced after db version 16
  // text*.dat files are optional, these files are missing
  // when db is build without Marisa support

  osmscout::log.Debug() << "Checking db files in directory " << dir.absolutePath().toStdString();
  valid=true;
  for (const auto &fileName: fileNames) {
    bool exists=dir.exists(fileName);
    if (!exists){
      osmscout::log.Debug() << "Missing mandatory file: " << fileName.toStdString();
    }
    valid &= exists;
  }
  if (!valid){
    osmscout::log.Warn() << "Can't use db " << dir.absolutePath().toStdString() << ", some mandatory files are missing.";
  }

  // metadata
  if (dir.exists(MapDownloadJob::FILE_METADATA)){
    QFile jsonFile(dir.filePath(MapDownloadJob::FILE_METADATA));
    jsonFile.open(QFile::OpenModeFlag::ReadOnly);
    QJsonDocument doc = QJsonDocument::fromJson(jsonFile.readAll());
    QJsonObject metadataObject = doc.object();
    if (metadataObject.contains("name") &&
        metadataObject.contains("map") &&
        metadataObject.contains("creation") &&
        metadataObject.contains("version")) {
      name = metadataObject["name"].toString();
      path = metadataObject["map"].toString().split("/");
#if QT_VERSION > QT_VERSION_CHECK(5, 8, 0) /* For compatibility with QT 5.6 */
      creation.setSecsSinceEpoch(metadataObject["creation"].toDouble());
#else
      creation.setTime_t(metadataObject["creation"].toDouble());
#endif
      version = metadataObject["version"].toInt();
      metadata = true;
    }
  }
}

QStringList MapDirectory::mandatoryFiles()
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
            << "route.dat"
            << "arearoute.idx"
            << "router.dat"
            << "router2.dat"
            // types.dat should be last, when download is interrupted,
            // directory is not recognized as valid map
            << "types.dat";

  return fileNames;
}

QStringList MapDirectory::optionalFiles(){
  QStringList fileNames;
  fileNames << "textloc.dat"
            << "textother.dat"
            << "textpoi.dat"
            << "textregion.dat"
            << "coverage.idx";
  return fileNames;
}

QStringList MapDirectory::metadataFiles()
{
  QStringList fileNames;
  fileNames << MapDownloadJob::FILE_METADATA;
  return fileNames;
}

qint64 MapDirectory::byteSize() const
{
  qint64 size = 0;
  QStringList fileNames = mandatoryFiles() + optionalFiles() + metadataFiles();

  for (const auto &fileName: fileNames) {
    if(dir.exists(fileName)){
      size+=QFileInfo(dir, fileName).size();
    }
  }
  return size;
}

bool MapDirectory::deleteDatabase()
{
  valid=false;

  QStringList fileNames = mandatoryFiles() + optionalFiles() + metadataFiles();

  bool result=true;
  for (const auto &fileName: fileNames) {
    if(dir.exists(fileName)){
      result&=dir.remove(fileName);
    } else {
      // check for partial download
      QString tempfileName = fileName + FileDownloaderConfig::TemporaryFileSuffix;
      if (dir.exists(tempfileName)){
        result&=dir.remove(tempfileName);
      }
    }
  }
  QDir parent=dir;
  parent.cdUp();
  result&=parent.rmdir(dir.dirName());
  if (result){
    qDebug() << "Removed db" << dir.path();
  }else{
    qWarning() << "Failed to remove db directory completely" << dir.path();
  }
  return result;
}

MapManager::MapManager(QStringList databaseLookupDirs, SettingsRef settings):
  databaseLookupDirs(databaseLookupDirs)
{
  osmscout::log.Debug() << "MapManager ctor";
  webCtrl.setCookieJar(new PersistentCookieJar(settings));
  // we don't use disk cache here
}

void MapManager::lookupDatabases()
{
  osmscout::log.Info() << "Lookup databases";

  databaseDirectories.clear();
  QSet<QString> uniqPaths;
  QList<QDir> databaseFsDirectories;

  for (QString lookupDir:databaseLookupDirs){
    QDirIterator dirIt(lookupDir, QDir::Files, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
    while (dirIt.hasNext()) {
      dirIt.next();
      QFileInfo fInfo(dirIt.filePath());
      if (fInfo.isFile() && fInfo.fileName() == osmscout::TypeConfig::FILE_TYPES_DAT){
        MapDirectory mapDir(fInfo.dir());
        if (mapDir.isValid()) {
          osmscout::log.Info() << "found db " << mapDir.getName().toStdString() << ": " << fInfo.dir().absolutePath().toStdString();
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
  for (auto& job:downloadJobs){
    delete job;
  }
  downloadJobs.clear();
}

void MapManager::downloadMap(AvailableMapsModelMap map, QDir dir, bool replaceExisting)
{
  auto* job=new MapDownloadJob(&webCtrl, map, dir, replaceExisting);
  connect(job, &MapDownloadJob::finished, this, &MapManager::onJobFinished);
  connect(job, &MapDownloadJob::canceled, this, &MapManager::onJobFinished);
  connect(job, &MapDownloadJob::failed, this, &MapManager::onJobFailed);
  downloadJobs<<job;
  emit downloadJobsChanged();
  downloadNext();
}

void MapManager::downloadNext()
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

void MapManager::onJobFailed(QString errorMessage)
{
  onJobFinished();
  emit mapDownloadFails(errorMessage);
}

void MapManager::onJobFinished()
{
  QList<MapDownloadJob*> finished;
  for (auto *job:downloadJobs){
    if (job->isDone()){
      finished << job;

      if (job->isReplaceExisting() && job->isSuccessful()){
        // if there is upgrade requested, delete old db with same (logical) path
        for (auto &mapDir:databaseDirectories) {
          if (mapDir.hasMetadata() &&
              mapDir.getPath() == job->getMapPath() &&
              mapDir.getDir().canonicalPath() != job->getDestinationDirectory().canonicalPath()) {

            osmscout::log.Debug() << "deleting map db " << mapDir.getName().toStdString() << " after upgrade: "
                                  << mapDir.getDir().canonicalPath().toStdString();
            mapDir.deleteDatabase();
          }
        }
      }
    }
  }
  if (!finished.isEmpty()){
    lookupDatabases();
  }
  for (auto *job:finished){
    downloadJobs.removeOne(job);
    emit downloadJobsChanged();
    job->deleteLater();
  }
  downloadNext();
}
}
