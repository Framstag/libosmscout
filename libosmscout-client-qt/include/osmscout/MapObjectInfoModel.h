#ifndef OSMSCOUT_CLIENT_QT_MAPOBJECTINFOMODEL_H
#define OSMSCOUT_CLIENT_QT_MAPOBJECTINFOMODEL_H

/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2016  Lukas Karas

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

#include <osmscout/OverlayObject.h>

#include <osmscout/GeoCoord.h>
#include <osmscout/util/GeoBox.h>

#include <osmscout/DBThread.h>
#include <osmscout/LookupModule.h>

#include <osmscout/LocationInfoModel.h>
#include <osmscout/ClientQtImportExport.h>

#include <QObject>
#include <QAbstractListModel>
#include <iostream>

namespace osmscout {

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API MapObjectInfoModel: public QAbstractListModel
{
  Q_OBJECT
  Q_PROPERTY(bool ready READ isReady NOTIFY readyChange)


public:
  enum Roles {
      LabelRole           = Qt::UserRole,
      TypeRole            = Qt::UserRole+1,
      IdRole              = Qt::UserRole+2,
      NameRole            = Qt::UserRole+3,
      ObjectRole          = Qt::UserRole+4,
      PhoneRole           = Qt::UserRole+5,
      WebsiteRole         = Qt::UserRole+6,
      AddressLocationRole = Qt::UserRole+7,
      AddressNumberRole   = Qt::UserRole+8,
      PostalCodeRole      = Qt::UserRole+9,
      RegionRole          = Qt::UserRole+10,
      LatRole             = Qt::UserRole+11,
      LonRole             = Qt::UserRole+12
  };
  Q_ENUM(Roles)

signals:
  void readyChange(bool ready);
  void objectsOnViewRequested(const MapViewStruct &view, const QRectF &filterRectangle);
  void objectsRequested(const LocationEntry &entry, bool reverseLookupAddresses);

public slots:
  void dbInitialized(const DatabaseLoadedResponse&);
  void setPosition(QObject *mapView,
                   const int width, const int height,
                   const int screenX, const int screenY);

  void onViewObjectsLoaded(const MapViewStruct&,
                           const QList<LookupModule::ObjectInfo> &objects);

  void setLocationEntry(QObject *o);

  void onObjectsLoaded(const LocationEntry &entry,
                       const QList<LookupModule::ObjectInfo> &objects);

private:
  void addToModel(const QList<LookupModule::ObjectInfo> &objects);

public:
  MapObjectInfoModel();
  virtual ~MapObjectInfoModel();

  Q_INVOKABLE virtual int inline rowCount(const QModelIndex &/*parent = QModelIndex()*/) const
  {
      return model.size();
  };

  bool inline isReady() const
  {
      return ready;
  };

  Q_INVOKABLE QObject* createOverlayObject(int row) const;

  Q_INVOKABLE virtual QVariant data(const QModelIndex &index, int role) const;
  virtual QHash<int, QByteArray> roleNames() const;
  Q_INVOKABLE virtual Qt::ItemFlags flags(const QModelIndex &index) const;

private:
  bool ready;
  bool setup;
  QList<ObjectKey> objectSet; // set of objects already inserted to model
  QList<LookupModule::ObjectInfo> model;
  MapViewStruct view;
  QRectF filterRectangle;
  LocationEntry locationEntry;

  QList<osmscout::MapData> mapData;
  double mapDpi;
  LookupModule* lookupModule;
};

}

#endif /* OSMSCOUT_CLIENT_QT_MAPOBJECTINFOMODEL_H */
