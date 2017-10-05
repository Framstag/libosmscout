#ifndef OSMSCOUT_CLIENT_QT_MAPSTYLEMODEL_H
#define	OSMSCOUT_CLIENT_QT_MAPSTYLEMODEL_H

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

#include <QObject>
#include <QAbstractListModel>
#include <QFileInfo>

#include <osmscout/StyleModule.h>
#include <osmscout/private/ClientQtImportExport.h>

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API MapStyleModel: public QAbstractListModel
{
  Q_OBJECT
  Q_PROPERTY(QString  style  READ getStyle  WRITE setStyle  NOTIFY styleChanged)

private:
  StyleModule *styleModule;

signals:
  void styleChanged();
  void loadStyleRequested(QString,std::unordered_map<std::string,bool>);
  
public:
  enum Roles {
      NameRole = Qt::UserRole,
      FileRole = Qt::UserRole+1,
      PathRole = Qt::UserRole+2,
  };

  MapStyleModel();
  virtual ~MapStyleModel();
 
  QString getStyle() const;
  void setStyle(const QString &style);

  Q_INVOKABLE virtual int inline rowCount(const QModelIndex &/*parent = QModelIndex()*/) const
  {
      return stylesheets.size();
  };

  Q_INVOKABLE virtual QVariant data(const QModelIndex &index, int role) const;
  virtual QHash<int, QByteArray> roleNames() const;
  Q_INVOKABLE virtual Qt::ItemFlags flags(const QModelIndex &index) const;

  Q_INVOKABLE int indexOf(const QString &style) const;
  Q_INVOKABLE QString file(int i) const;

private:
  QList<QFileInfo> stylesheets;
};

#endif	/* OSMSCOUT_CLIENT_QT_MAPSTYLEHELPER_H */

