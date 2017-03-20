
#ifndef STYLEFLAGSMODEL_H
#define STYLEFLAGSMODEL_H

/*
  OSMScout - a Qt backend for libosmscout and libosmscout-map
  Copyright (C) 2017 Lukas Karas

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

#include <unordered_map>

#include <QObject>
#include <QAbstractListModel>

#include <osmscout/private/ClientQtImportExport.h>

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API StyleFlagsModel: public QAbstractListModel
{
  Q_OBJECT

private slots:
  void onStyleChanged();

public:
  enum Roles {
      KeyRole   = Qt::UserRole,
      ValueRole = Qt::UserRole+1,
  };

  StyleFlagsModel();
  virtual ~StyleFlagsModel();

  Q_INVOKABLE virtual int inline rowCount(const QModelIndex &/*parent = QModelIndex()*/) const
  {
      return mapFlags.size();
  };

  Q_INVOKABLE virtual QVariant data(const QModelIndex &index, int role) const;
  virtual QHash<int, QByteArray> roleNames() const;
  Q_INVOKABLE virtual Qt::ItemFlags flags(const QModelIndex &index) const;

  Q_INVOKABLE void setFlag(const QString &key, bool value);
private:
  QMap<QString,bool> mapFlags;
};

#endif /* STYLEFLAGSMODEL_H */

