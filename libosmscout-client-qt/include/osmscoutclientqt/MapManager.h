#ifndef OSMSCOUT_CLIENT_QT_MAPMANAGER_H
#define OSMSCOUT_CLIENT_QT_MAPMANAGER_H

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

#include <QObject>
#include <QStringList>
#include <QList>
#include <QDir>
#include <QTimer>

#include <osmscoutclientqt/ClientQtImportExport.h>

#include <osmscoutclientqt/MapProvider.h>
#include <osmscoutclientqt/Settings.h>
#include <osmscoutclientqt/AvailableMapsModel.h>
#include <osmscoutclientqt/FileDownloader.h>

#include <QtGlobal>
#include <QStorageInfo>

namespace osmscout {

/**
 * Utility class for downloading map database described by AvailableMapsModelMap
 * over http.
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API MapDownloadJob: public DownloadJob
{
  Q_OBJECT

  AvailableMapsModelMap   map;

public:
  static const char* FILE_METADATA;

  MapDownloadJob(QNetworkAccessManager *webCtrl, AvailableMapsModelMap map, QDir target, bool replaceExisting);

  /**
   * Cancel downloading,
   * when database is not downloaded successfully, remove it from disk
   * (even already downloaded files).
   */
  ~MapDownloadJob() override;

  void start();

  QString getMapName() const
  {
    return map.getName();
  }

  QStringList getMapPath() const
  {
    return map.getPath();
  }

  uint64_t expectedSize() const override
  {
    return map.getSize();
  }
};

/**
 * Holder for map database metadata
 *
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API MapDirectory
{
public:
  MapDirectory() = default;
  explicit MapDirectory(QDir dir);
  ~MapDirectory() = default;

  MapDirectory(const MapDirectory &other) = default;
  MapDirectory &operator=(const MapDirectory &other) = default;

  MapDirectory(MapDirectory &&other) = default;
  MapDirectory &operator=(MapDirectory &&other) = default;

  QDir getDir() const
  {
    return dir;
  }

  static QStringList mandatoryFiles();
  static QStringList optionalFiles();
  static QStringList metadataFiles();

  /**
   * byte size of all database files on disk
   * @return
   */
  qint64 byteSize() const;

  /**
   * Delete complete database
   */
  bool deleteDatabase();

  /**
   * Check if directory contains all required files for osmscout database
   * @return true if all requirements met and directory may be used as database
   */
  bool isValid() const
  {
    return valid;
  }

  /**
   * Check if map directory contains metadata created by downloader
   * @return true if map directory contains metadata
   */
  bool hasMetadata() const
  {
    return metadata;
  }

  /**
   * Human readable name of the map. It is name of geographical region usually (eg: Germany, Czech Republic...).
   * Name is localised by server when it is downloading. When locale is changed later, name will remain
   * in its original locale.
   * @return map name
   */
  QString getName() const
  {
    return name;
  }

  /**
   * Logical path of the map, eg: europe/gemany; europe/czech-republic
   * @return
   */
  QStringList getPath() const
  {
    return path;
  }

  /**
   * Time of map import
   * @return
   */
  QDateTime getCreation() const
  {
    return creation;
  }

  int getVersion() const
  {
    return version;
  }

  bool operator<(const MapDirectory &o) const
  {
    if (getName() == o.getName()){
      return getDir().absolutePath() < o.getDir().absolutePath();
    }
    return getName() < o.getName();
  }

private:
  QDir dir;
  bool valid{false};
  bool metadata{false};
  QString name;
  QStringList path;
  QDateTime creation;
  int version{0};
};

/**
 * Manager of map databases. It provide database lookup
 * (in databaseDirectories) and simple scheduler for downloading maps.
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API MapManager: public QObject
{
  Q_OBJECT

private:
  QStringList databaseLookupDirs;
  QList<MapDirectory> databaseDirectories;
  QList<MapDownloadJob*> downloadJobs;
  QNetworkAccessManager webCtrl;

public slots:
  void lookupDatabases();
  void onJobFinished();
  void onJobFailed(QString errorMessage);

signals:
  void mapDownloadFails(QString message);
  void databaseListChanged(QList<QDir> databaseDirectories);
  void downloadJobsChanged();

public:
  MapManager(QStringList databaseLookupDirs, SettingsRef settings);

  ~MapManager() override;

  /**
   * Start map downloading into local dir.
   *
   * @param map
   * @param dir
   * @param replaceExisting - when true, manager will delete existing database with same path (MapDirectory::getPath)
   */
  void downloadMap(AvailableMapsModelMap map, QDir dir, bool replaceExisting = true);
  void downloadNext();

  QList<MapDownloadJob*> getDownloadJobs() const {
    return downloadJobs;
  }

  QStringList getLookupDirectories() const
  {
    return databaseLookupDirs;
  }

  QList<MapDirectory> getDatabaseDirectories() const
  {
    return databaseDirectories;
  }
};

/**
 * \ingroup QtAPI
 */
using MapManagerRef = std::shared_ptr<MapManager>;

}

#endif /* OSMSCOUT_CLIENT_QT_MAPMANAGER_H */
