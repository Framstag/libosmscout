#ifndef OSMSCOUT_CLIENT_QT_SEARCHLOCATIONMODEL_H
#define OSMSCOUT_CLIENT_QT_SEARCHLOCATIONMODEL_H

/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2014  Tim Teulings
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

#include <QObject>
#include <QAbstractListModel>

#include <osmscout/GeoCoord.h>
#include <osmscout/LocationEntry.h>
#include <osmscout/LocationService.h>
#include <osmscout/SearchModule.h>

#include <osmscout/private/ClientQtImportExport.h>

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API LocationListModel : public QAbstractListModel
{
  Q_OBJECT
  Q_PROPERTY(int     count       READ rowCount    NOTIFY countChanged)
  Q_PROPERTY(bool    searching   READ isSearching NOTIFY SearchingChanged)
  Q_PROPERTY(double  lat         READ GetLat      WRITE SetLat)
  Q_PROPERTY(double  lon         READ GetLon      WRITE SetLon)
  Q_PROPERTY(int     resultLimit READ GetResultLimit WRITE SetResultLimit)

signals:
  void SearchRequested(const QString searchPattern,
                       int limit,
                       osmscout::GeoCoord searchCenter,
                       AdminRegionInfoRef defaultRegion,
                       osmscout::BreakerRef breaker);
  void SearchingChanged(bool);
  void countChanged(int);
  void regionLookupRequested(osmscout::GeoCoord);

public slots:
  void setPattern(const QString& pattern);
  void onSearchResult(const QString searchPattern,
                      const QList<LocationEntry>);
  void onSearchFinished(const QString searchPattern, bool error);

  void onLocationAdminRegions(const osmscout::GeoCoord,QList<AdminRegionInfoRef>);
  void onLocationAdminRegionFinished(const osmscout::GeoCoord);

private:
  QString pattern;
  QString lastRequestPattern;
  QList<LocationEntry*> locations;
  bool searching;
  SearchModule* searchModule;
  LookupModule* lookupModule;
  osmscout::GeoCoord searchCenter;
  int resultLimit;
  osmscout::BreakerRef breaker;
  AdminRegionInfoRef defaultRegion;
  AdminRegionInfoRef lastRequestDefaultRegion;

public:
  enum Roles {
    LabelRole = Qt::UserRole,
    TypeRole = Qt::UserRole +1,
    RegionRole = Qt::UserRole +2,
    LatRole = Qt::UserRole +3,
    LonRole = Qt::UserRole +4
  };

public:
  LocationListModel(QObject* parent = 0);
  virtual ~LocationListModel();

  Q_INVOKABLE virtual QVariant data(const QModelIndex &index, int role) const;

  Q_INVOKABLE virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;

  Q_INVOKABLE virtual Qt::ItemFlags flags(const QModelIndex &index) const;

  virtual QHash<int, QByteArray> roleNames() const;

  Q_INVOKABLE LocationEntry* get(int row) const;

  inline bool isSearching() const
  {
    return searching;
  }

  inline double GetLat() const {
    return searchCenter.GetLat();
  }

  inline double GetLon() const {
    return searchCenter.GetLon();
  }

  inline void SetLat(double lat){
    if (lat!=searchCenter.GetLat()) {
      searchCenter.Set(lat, searchCenter.GetLon());
      lookupRegion();
    }
  }

  inline void SetLon(double lon){
    if (lon!=searchCenter.GetLon()) {
      searchCenter.Set(searchCenter.GetLat(), lon);
      lookupRegion();
    }
  }

  inline int GetResultLimit() const {
    return resultLimit;
  }

  inline void SetResultLimit(int limit){
    resultLimit=limit;
  }

private:
  void lookupRegion();

};

#endif
