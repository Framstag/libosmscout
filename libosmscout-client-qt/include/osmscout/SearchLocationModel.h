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
#include <QJSValue>

#include <osmscout/GeoCoord.h>
#include <osmscout/LocationEntry.h>
#include <osmscout/LocationService.h>
#include <osmscout/SearchModule.h>

#include <osmscout/private/ClientQtImportExport.h>

/**
 * Model for searching objects in osmscout databases by pattern written by human.
 *
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API LocationListModel : public QAbstractListModel
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
   * Limit of results for each database.
   */
  Q_PROPERTY(int      resultLimit READ GetResultLimit WRITE SetResultLimit)

  /**
   * Searched pattern
   */
  Q_PROPERTY(QString  pattern     READ getPattern  WRITE setPattern)

  /**
   * JavaScript function used for sorting search results.
   * Compare function will receive two locations arguments,
   * A and B, where A < B (A should be shown before B),
   * compare function should return number < 0.
   *
   * For sorting results alphabetically:
   * ```
   * LocationListModel{
   *   compare: function(a, b){
   *     if (a.label < b.label){
   *        return -1;
   *     }
   *     return +1;
   *   }
   * }
   * ```
   */
  Q_PROPERTY(QJSValue compare     READ getCompare  WRITE setCompare)

  /**
   * JavaScript function used for check location equality.
   * It is possible that model will returns one physical location multiple
   * times, for example one from location index and second from fulltext
   * search, or two database segments for one real street...
   *
   * Such results may be merged in model.
   * Function will receive two locations (only locations with same label
   * will be checked) and should return boolean result.
   *
   * For example, merge near objects with same label and type:
   * ```
   * LocationListModel{
   *   equals: function(a, b){
   *     if (a.objectType == b.objectType &&
   *         a.distanceTo(b.lat, b.lon) < 300 &&
   *         a.distanceTo(searchCenterLat, searchCenterLon) > 3000 ){
   *           return true;
   *     }
   *     return false;
   *   }
   * }
   * ```
   */
  Q_PROPERTY(QJSValue equals      READ getEquals   WRITE setEquals)

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
  QJSValue compareFn;
  QJSValue equalsFn;

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

public:
  LocationListModel(QObject* parent = 0);
  virtual ~LocationListModel();

  QJSValue getCompare() const {
    return compareFn;
  }

  void setCompare(const QJSValue &fn){
    compareFn=fn;
  }

  QJSValue getEquals() const{
    return equalsFn;
  }

  void setEquals(const QJSValue &fn){
    equalsFn=fn;
  }

  Q_INVOKABLE virtual QVariant data(const QModelIndex &index, int role) const;

  Q_INVOKABLE virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;

  Q_INVOKABLE virtual Qt::ItemFlags flags(const QModelIndex &index) const;

  virtual QHash<int, QByteArray> roleNames() const;

  Q_INVOKABLE LocationEntry* get(int row) const;

  inline bool isSearching() const {
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

  inline QString getPattern() const {
    return pattern;
  }

private:
  void lookupRegion();

};

#endif
