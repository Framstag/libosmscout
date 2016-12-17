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

#ifndef MAPDOWNLOADSMODEL_H
#define	MAPDOWNLOADSMODEL_H

#include <QObject>
#include <QStringList>
#include <QList>
#include <QDir>
#include <QTimer>

#include <osmscout/private/ClientQtImportExport.h>

#include <osmscout/MapProvider.h>
#include <osmscout/MapManager.h>
#include <osmscout/DBThread.h>

/**
 * QML list model with currently downloaded maps. It provide methods
 * (invocable from QML) for starting new map download.
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API MapDownloadsModel: public QAbstractListModel
{
  Q_OBJECT
signals:
  void mapDownloadFails(QString message);

public slots:
  void onDownloadJobsChanged();
  void onDownloadProgress();

private:
  MapManagerRef mapManager;

public:
  MapDownloadsModel(QObject *parent=Q_NULLPTR);
  
  virtual inline ~MapDownloadsModel(){};
  
  enum Roles {
    MapNameRole = Qt::UserRole,
    TargetDirectoryRole = Qt::UserRole+1,
    ProgressRole = Qt::UserRole+2,
    ProgressDescriptionRole = Qt::UserRole+3,
  };

  Q_INVOKABLE virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
  Q_INVOKABLE virtual QVariant data(const QModelIndex &index, int role) const;
  virtual QHash<int, QByteArray> roleNames() const;
  Q_INVOKABLE virtual Qt::ItemFlags flags(const QModelIndex &index) const;

  Q_INVOKABLE QString suggestedDirectory(QVariant mapVar, QString rootDirectory = "");
  Q_INVOKABLE void downloadMap(QVariant map, QString dir);
  Q_INVOKABLE QStringList getLookupDirectories();
  Q_INVOKABLE double getFreeSpace(QString dir);
};

#endif	/* MAPDOWNLOADSMODEL_H */

