#ifndef OSMSCOUT_CLIENT_QT_SEARCHMODULE_H
#define OSMSCOUT_CLIENT_QT_SEARCHMODULE_H

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

#include <osmscoutclientqt/DBThread.h>
#include <osmscoutclientqt/LookupModule.h>

#ifdef OSMSCOUT_HAVE_LIB_MARISA
#include <osmscout/db/TextSearchIndex.h>
#endif

#include <osmscoutclientqt/ClientQtImportExport.h>

#include <QObject>
#include <QThread>
#include <QRunnable>

namespace osmscout {

class SearchModule;

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API SearchRunnable : public QRunnable {

protected:
  SearchModule *searchModule;
  DBInstanceRef db;
  NameFeatureValueReader nameReader;
  NameAltFeatureValueReader altNameReader;
  QString searchPattern;
  int limit;
  osmscout::BreakerRef breaker;
  std::map<osmscout::FileOffset,osmscout::AdminRegionRef> adminRegionMap;
  std::promise<bool> promise;

public:
  SearchRunnable(SearchModule *searchModule,
                 DBInstanceRef &db,
                 const QString &searchPattern,
                 int limit,
                 osmscout::BreakerRef &breaker);

  std::future<bool> getFuture();

protected:
  bool GetObjectDetails(const osmscout::ObjectFileRef& object,
                        const std::string &searchKey,
                        QString &typeName,
                        QString &name,
                        QString &altName,
                        osmscout::GeoCoord& coordinates,
                        osmscout::GeoBox& bbox);

  bool GetObjectDetails(const std::vector<osmscout::ObjectFileRef>& objects,
                        const std::string &searchKey,
                        QString &typeName,
                        QString &name,
                        QString &altName,
                        osmscout::GeoCoord& coordinates,
                        osmscout::GeoBox& bbox);

    void GetObjectNames(const FeatureValueBuffer &features,
                        QString &typeName,
                        QString &name,
                        QString &altName);
};

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API SearchLocationsRunnable : public SearchRunnable {

private:
  AdminRegionInfoRef defaultRegionInfo;

public:
  SearchLocationsRunnable(SearchModule *searchModule,
                          DBInstanceRef &db,
                          const QString &searchPattern,
                          int limit,
                          osmscout::BreakerRef &breaker,
                          AdminRegionInfoRef &defaultRegion);

  void run() override;

private:
  bool SearchLocations(DBInstanceRef &db,
                       const QString &searchPattern,
                       const osmscout::AdminRegionRef &defaultRegion,
                       int limit,
                       osmscout::BreakerRef &breaker,
                       std::map<osmscout::FileOffset,osmscout::AdminRegionRef> &adminRegionMap);

  bool BuildLocationEntry(const osmscout::LocationSearchResult::Entry &entry,
                          std::map<osmscout::FileOffset,osmscout::AdminRegionRef> &adminRegionMap,
                          QList<LocationEntry> &locations);
};

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API FreeTextSearchRunnable : public SearchRunnable {

public:
  FreeTextSearchRunnable(SearchModule *searchModule,
                         DBInstanceRef &db,
                         const QString &searchPattern,
                         int limit,
                         osmscout::BreakerRef &breaker);

  void run() override;

private:
  bool FreeTextSearch(DBInstanceRef &db,
                      const QString &searchPattern,
                      int limit,
                      std::map<osmscout::FileOffset,osmscout::AdminRegionRef> &adminRegionMap);

  /**
   * @param object
   * @param searchKey index key matching search query
   * @param adminRegionMap cached map of administrative regions
   * @param locations list where new location is added
   * @return true on success
   */
  bool BuildLocationEntry(const osmscout::ObjectFileRef& object,
                          const std::string &searchKey,
                          std::map<osmscout::FileOffset,osmscout::AdminRegionRef> &adminRegionMap,
                          QList<LocationEntry> &locations);
};

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API SearchModule:public QObject{
  Q_OBJECT

private:
  QThread          *thread;
  DBThreadRef      dbThread;
  LookupModule     *lookupModule;

signals:
  void searchResult(const QString searchPattern, const QList<LocationEntry>);
  void searchFinished(const QString searchPattern, bool error);

public slots:

  /**
   * Start object search by some pattern.
   *
   * DBThread then emits searchResult signals followed by searchFinished
   * for this pattern.
   *
   * User of this function should use Qt::QueuedConnection for invoking
   * this slot, search may generate IO load and may tooks long time.
   *
   * Keep in mind that entries retrieved by searchResult signal can contains
   * duplicates, because search may use various databases and indexes.
   *
   * @param searchPattern
   * @param limit - suggested limit for count of retrieved entries from one db
   */
  void SearchForLocations(const QString searchPattern,
                          int limit,
                          osmscout::GeoCoord,
                          AdminRegionInfoRef defaultRegion,
                          osmscout::BreakerRef breaker);

public:
  SearchModule(QThread *thread,DBThreadRef dbThread,LookupModule *lookupModule);
  virtual ~SearchModule();

};

}

#endif /* OSMSCOUT_CLIENT_QT_SEARCHMODULE_H */
