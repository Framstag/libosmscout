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

#ifndef ONLINETILEPROVIDERMODEL_H
#define	ONLINETILEPROVIDERMODEL_H

#include <QAbstractListModel>

#include <osmscout/OnlineTileProvider.h>
#include <osmscout/Settings.h>


class OnlineTileProviderModel : public QAbstractListModel {
  Q_OBJECT
  
public:
  inline OnlineTileProviderModel()
  {
      onlineProviders = Settings::GetInstance()->GetOnlineProviders();
  };
  
  virtual inline ~OnlineTileProviderModel(){};
  
  enum Roles {
    NameRole = Qt::UserRole,
    IdRole = Qt::UserRole+1,
  };

  int inline rowCount(const QModelIndex &/*parent = QModelIndex()*/) const
  {
    return onlineProviders.size();
  };

  QVariant data(const QModelIndex &index, int role) const;
  QHash<int, QByteArray> roleNames() const;
  Qt::ItemFlags flags(const QModelIndex &index) const;

  Q_INVOKABLE int count() const;
  Q_INVOKABLE QString getId(int row) const;
  Q_INVOKABLE QString getName(int row) const;
  
private:
    QList<OnlineTileProvider> onlineProviders;
};

#endif	/* ONLINETILEPROVIDERMODEL_H */

