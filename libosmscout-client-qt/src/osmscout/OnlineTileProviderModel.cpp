/*
  This source is part of the libosmscout-map library
  Copyright (C) 2016  Lukáš Karas

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

#include <osmscout/OnlineTileProviderModel.h>

QVariant OnlineTileProviderModel::data(const QModelIndex &index, int role) const
{
  OnlineTileProvider provider = onlineProviders.at(index.row());
  switch (role) {
    case Qt::DisplayRole:
    case NameRole:
        return provider.getName();
    case IdRole:
        return provider.getId();
    default:
        break;
  }
  return QVariant();
}

QHash<int, QByteArray> OnlineTileProviderModel::roleNames() const
{
    QHash<int, QByteArray> roles=QAbstractListModel::roleNames();

    roles[NameRole]="name";
    roles[IdRole]="id";

    return roles;
}

Qt::ItemFlags OnlineTileProviderModel::flags(const QModelIndex &index) const
{
  if(!index.isValid()) {
    return Qt::ItemIsEnabled;
  }

  return QAbstractListModel::flags(index) | Qt::ItemIsEnabled | Qt::ItemIsSelectable;    
}

int OnlineTileProviderModel::count() const
{
    return onlineProviders.size();
}

QString OnlineTileProviderModel::getId(int row) const
{
    return onlineProviders.at(row).getId();
}

QString OnlineTileProviderModel::getName(int row) const
{
    return onlineProviders.at(row).getName();    
}

  