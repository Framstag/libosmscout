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

#ifndef MAPMANAGER_H
#define	MAPMANAGER_H

#include <QObject>
#include <QStringList>
#include <QList>
#include <QDir>
#include <QTimer>

#include <osmscout/private/ClientQtImportExport.h>

#include <osmscout/MapProvider.h>
#include <osmscout/AvailableMapsModel.h>

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API FileDownloadJob: public QObject
{
  Q_OBJECT
  
private:
   QUrl url;
   QFile file;
   bool downloading;
   bool downloaded;
   QNetworkReply *reply;

signals:
  void finished();
  void failed();

public slots:
  void onFinished(QNetworkReply* reply);
  void onReadyRead();

public:
  FileDownloadJob(QUrl url, QFileInfo file);
  virtual inline ~FileDownloadJob(){};
  
  void start(QNetworkAccessManager *webCtrl);

  inline bool isDownloading()
  {
    return downloading;
  };

  inline bool isDownloaded()
  {
    return downloaded;
  };
};

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API MapDownloadJob: public QObject
{
  Q_OBJECT
  
  QList<FileDownloadJob*> jobs;
  QNetworkAccessManager   *webCtrl;
  
  AvailableMapsModelMap   map;
  QDir                    target;
  
  QTimer                  backoffTimer;
  int                     backoffInterval;
  bool                    done;
  bool                    started;

signals:
  void finished();

public slots:
  void onJobFailed();
  void onJobFinished();
  void downloadNextFile();

public:
  MapDownloadJob(QNetworkAccessManager *webCtrl, AvailableMapsModelMap map, QDir target);
  virtual ~MapDownloadJob();
  
  void start();

  inline bool isDone()
  {
    return done;
  }
  inline bool isDownloading()
  {
    return started && !done;
  }
};

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API MapManager: public QObject
{
  Q_OBJECT
  
private:
  QStringList databaseLookupDirs;
  QList<QDir> databaseDirectories;
  QList<MapDownloadJob*> downloadJobs;
  QNetworkAccessManager webCtrl;

public slots:
  void lookupDatabases();
  void onJobFinished();
  
signals:
  void databaseListChanged(QList<QDir> databaseDirectories);
  
public:
  MapManager(QStringList databaseLookupDirs);
  
  virtual ~MapManager();

  void downloadMap(AvailableMapsModelMap map, QDir dir);
  void downloadNext();
  
  inline QStringList getLookupDirectories()
  {
    return databaseLookupDirs;
  }
};

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API QmlMapManager: public QObject
{
  Q_OBJECT

public:
  inline QmlMapManager(QObject *parent=Q_NULLPTR):QObject(parent){}
  
  virtual inline ~QmlMapManager(){};
  
  Q_INVOKABLE QString suggestedDirectory(QVariant mapVar);
  Q_INVOKABLE void downloadMap(QVariant map, QString dir);
  Q_INVOKABLE QStringList getLookupDirectories();
  Q_INVOKABLE double getFreeSpace(QString dir);
};

#endif	/* MAPMANAGER_H */

