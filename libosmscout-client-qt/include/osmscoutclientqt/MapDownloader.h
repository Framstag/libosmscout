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

#include <osmscoutclientqt/ClientQtImportExport.h>

#include <osmscoutclient/MapProvider.h>
#include <osmscoutclient/Settings.h>
#include <osmscoutclient/MapDirectory.h>
#include <osmscoutclient/MapManager.h>

#include <osmscoutclientqt/AvailableMapsModel.h>
#include <osmscoutclientqt/FileDownloader.h>

#include <QObject>
#include <QStringList>
#include <QList>
#include <QDir>
#include <QTimer>
#include <QtGlobal>
#include <QStorageInfo>

namespace osmscout {

/**
 * Utility class for downloading map db described by AvailableMapsModelMap
 * over http.
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API MapDownloadJob: public DownloadJob
{
  Q_OBJECT

  AvailableMapsModelMap   map;

public:

  MapDownloadJob(QNetworkAccessManager *webCtrl, AvailableMapsModelMap map, QDir target, bool replaceExisting);

  /**
   * Cancel downloading,
   * when db is not downloaded successfully, remove it from disk
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
 * Manager of map databases. It provide db lookup
 * (in databaseDirectories) and simple scheduler for downloading maps.
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API MapDownloader: public QObject
{
  Q_OBJECT

private:
  QList<MapDownloadJob*> downloadJobs;
  QNetworkAccessManager webCtrl;
  MapManagerRef mapManager;

public slots:
  void onJobFinished();
  void onJobFailed(QString errorMessage);

signals:
  void mapDownloadFails(QString message);
  void downloadJobsChanged();

public:
  MapDownloader(MapManagerRef mapManager, SettingsRef settings);

  ~MapDownloader() override;

  /**
   * Start map downloading into local dir.
   *
   * @param map
   * @param dir
   * @param replaceExisting - when true, manager will delete existing db with same path (MapDirectory::getPath)
   */
  void downloadMap(AvailableMapsModelMap map, QDir dir, bool replaceExisting = true);
  void downloadNext();

  QList<MapDownloadJob*> getDownloadJobs() const {
    return downloadJobs;
  }
};

/**
 * \ingroup QtAPI
 */
using MapDownloaderRef = std::shared_ptr<MapDownloader>;

}

#endif /* OSMSCOUT_CLIENT_QT_MAPMANAGER_H */
