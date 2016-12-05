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

#ifndef AVAILABLEMAPSMODEL_H
#define	AVAILABLEMAPSMODEL_H

#include <QAbstractListModel>

#include <osmscout/MapProvider.h>
#include <osmscout/Settings.h>

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API AvailableMapsModel : public QAbstractListModel {
  Q_OBJECT
  
public:
  inline AvailableMapsModel()
  {
    mapProviders = Settings::GetInstance()->GetMapProviders();
  }

  inline ~AvailableMapsModel()
  {
  }

  enum Roles {
    NameRole = Qt::UserRole, // localized name
    PathRole = Qt::UserRole+1, // path in tree
    DirRole = Qt::UserRole+2, // isDir? true: false
    ServerDirectoryRole = Qt::UserRole+3, // server path for this map
    TimeRole = Qt::UserRole+4, // QTime of map creation 
    VersionRole = Qt::UserRole+5,
    SizeRole = Qt::UserRole+6,
  };

  int rowCount(const QModelIndex &parent = QModelIndex()) const;

  QVariant data(const QModelIndex &index, int role) const;
  QHash<int, QByteArray> roleNames() const;
  Qt::ItemFlags flags(const QModelIndex &index) const;
  
private:
  QList<MapProvider> mapProviders;
};

#endif	/* AVAILABLEMAPMODEL_H */
