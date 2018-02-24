#ifndef OSMSCOUT_CLIENT_QT_NEARPOIMODEL_H
#define OSMSCOUT_CLIENT_QT_NEARPOIMODEL_H

/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2018 Lukas Karas

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

#include <osmscout/LocationEntry.h>
#include <osmscout/POILookupModule.h>

#include <osmscout/private/ClientQtImportExport.h>

#include <QObject>
#include <QAbstractListModel>

#define INVALID_COORD -1000.0

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API NearPOIModel: public QAbstractListModel
{
Q_OBJECT
  /**
   * Count of rows in model - count of search results
   */
  Q_PROPERTY(int      count       READ rowCount    NOTIFY countChanged)

  /**
   * True if searching is in progress
   */
  Q_PROPERTY(bool     searching   READ isSearching NOTIFY SearchingChanged)

  /**
   * Lat and lon properties control where is logical search center.
   * Local admin region is used as default region,
   * databases used for search are sorted by distance from this point
   * (local results should be available faster).
   */
  Q_PROPERTY(double   lat         READ GetLat      WRITE SetLat)

  /**
   * \see lat property
   */
  Q_PROPERTY(double   lon         READ GetLon      WRITE SetLon)

  /**
   * Maximal distance of searched objects
   */
  Q_PROPERTY(double   maxDistance READ GetMaxDistance WRITE SetMaxDistance)

  /**
   * Limit of results for each database.
   */
  Q_PROPERTY(int      resultLimit READ GetResultLimit WRITE SetResultLimit)

  /**
   * Limit of results for each database.
   */
  Q_PROPERTY(QStringList types READ GetTypes WRITE SetTypes)

public:
  enum Roles {
    LabelRole = Qt::UserRole,
    TypeRole = Qt::UserRole +1,
    RegionRole = Qt::UserRole +2,
    LatRole = Qt::UserRole +3,
    LonRole = Qt::UserRole +4,
    DistanceRole = Qt::UserRole +5,
    BearingRole = Qt::UserRole +6,
    LocationObjectRole = Qt::UserRole +7
  };

signals:
  void countChanged(int);

  void SearchingChanged(bool);

  void lookupPOI(int requestId,
                 osmscout::BreakerRef breaker,
                 osmscout::GeoCoord searchCenter,
                 QStringList types,
                 double maxDistance);

public slots:
  void onLookupFinished(int requestId);
  void onLookupResult(int requestId, QList<LocationEntry> locations);

private:
  bool searching{false};
  int currentRequest{0};
  QList<LocationEntry*> locations;
  osmscout::GeoCoord searchCenter{INVALID_COORD,INVALID_COORD};
  int resultLimit{100};
  osmscout::BreakerRef breaker;
  double maxDistance{1000};
  QStringList types;

  POILookupModule *poiModule{nullptr};

public:
  NearPOIModel();
  virtual ~NearPOIModel();

  Q_INVOKABLE virtual QVariant data(const QModelIndex &index, int role) const;

  Q_INVOKABLE virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;

  Q_INVOKABLE virtual Qt::ItemFlags flags(const QModelIndex &index) const;

  Q_INVOKABLE LocationEntry* get(int row) const;

  virtual QHash<int, QByteArray> roleNames() const;

  inline bool isSearching() const
  {
    return searching;
  }

  inline double GetLat() const
  {
    return searchCenter.GetLat();
  }

  void SetLat(double lat)
  {
    if (lat!=searchCenter.GetLat()) {
      searchCenter.Set(lat, searchCenter.GetLon());
      lookupPOI();
    }
  }

  inline double GetLon() const
  {
    return searchCenter.GetLon();
  }

  void SetLon(double lon)
  {
    if (lon!=searchCenter.GetLon()){
      searchCenter.Set(searchCenter.GetLat(), lon);
      lookupPOI();
    }
  }

  inline double GetMaxDistance() const
  {
    return maxDistance;
  }

  void SetMaxDistance(double d)
  {
    if (maxDistance!=d){
      maxDistance=d;
      lookupPOI();
    }
  }

  inline int GetResultLimit() const
  {
    return resultLimit;
  }

  inline void SetResultLimit(int limit)
  {
    if (resultLimit!=limit){
      resultLimit=limit;
      lookupPOI();
    }
  }

  inline QStringList GetTypes() const
  {
    return types;
  }

  inline void SetTypes(QStringList t)
  {
    if (types!=t){
      types=t;
      lookupPOI();
    }
  }

private:
  void lookupPOI();
};

#endif //OSMSCOUT_CLIENT_QT_NEARPOIMODEL_H
