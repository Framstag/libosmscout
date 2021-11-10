#ifndef OSMSCOUT_CLIENT_QT_INSTALLEDMAPSMODEL_H
#define OSMSCOUT_CLIENT_QT_INSTALLEDMAPSMODEL_H

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


#include <osmscoutclientqt/ClientQtImportExport.h>

#include <QAbstractListModel>

#include <osmscoutclientqt/MapManager.h>

namespace osmscout {

/**
 * Model providing access to currently installed maps on device
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API InstalledMapsModel : public QAbstractListModel {
Q_OBJECT

signals:
  void databaseListChanged();

public slots:
  void onDatabaseListChanged();

public:
  InstalledMapsModel();

  virtual ~InstalledMapsModel();

  enum Roles {
    NameRole = Qt::UserRole, // localized name
    PathRole = Qt::UserRole + 1, // logical path of map
    DirectoryRole = Qt::UserRole + 2, // directory
    TimeRole = Qt::UserRole + 3, // generating time of map
    ByteSizeRole = Qt::UserRole + 4, // size on the disk in bytes
    SizeRole = Qt::UserRole + 5, // human readable size on the disk
    VersionRole = Qt::UserRole + 6,
  };
  Q_ENUM(Roles)

  Q_INVOKABLE virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
  Q_INVOKABLE virtual QVariant data(const QModelIndex &index, int role) const;
  virtual QHash<int, QByteArray> roleNames() const;
  Q_INVOKABLE virtual Qt::ItemFlags flags(const QModelIndex &index) const;

  /**
   * Delete installed map represented this model on given row
   * @param row
   * @return true on success
   */
  Q_INVOKABLE bool deleteMap(int row);
  Q_INVOKABLE virtual bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());

  /**
   * Generation time of map with given path. Null if don't exists
   * It may be used for detection if there is some update available.
   *
   * @param path
   * @return
   */
  Q_INVOKABLE QVariant timeOfMap(const QStringList& path);

private:
  QList<MapDirectory> dirs;
  MapManagerRef mapManager;
};

}

#endif //OSMSCOUT_CLIENT_QT_INSTALLEDMAPSMODEL_H
