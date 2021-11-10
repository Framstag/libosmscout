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

#include <QObject>
#include <QThread>
#include <osmscoutclientqt/DBThread.h>
#include <osmscoutclientqt/LookupModule.h>

#ifdef OSMSCOUT_HAVE_LIB_MARISA
#include <osmscout/TextSearchIndex.h>
#endif

#include <osmscoutclientqt/ClientQtImportExport.h>

namespace osmscout {

class SearchModule;

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API SearchRunnable : public QRunnable {

protected:
  SearchModule *searchModule;
  DBInstanceRef db;
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
  bool BuildLocationEntry(const osmscout::ObjectFileRef& object,
                          const QString &title,
                          DBInstanceRef &db,
                          std::map<osmscout::FileOffset,osmscout::AdminRegionRef> &adminRegionMap,
                          QList<LocationEntry> &locations);

  bool BuildLocationEntry(const osmscout::LocationSearchResult::Entry &entry,
                          DBInstanceRef &db,
                          std::map<osmscout::FileOffset,osmscout::AdminRegionRef> &adminRegionMap,
                          QList<LocationEntry> &locations);

  bool GetObjectDetails(DBInstanceRef &db,
                        const osmscout::ObjectFileRef& object,
                        QString &typeName,
                        osmscout::GeoCoord& coordinates,
                        osmscout::GeoBox& bbox);

  bool GetObjectDetails(DBInstanceRef &db,
                        const std::vector<osmscout::ObjectFileRef>& objects,
                        QString &typeName,
                        osmscout::GeoCoord& coordinates,
                        osmscout::GeoBox& bbox);
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
   * @param limit - suggested limit for count of retrieved entries from one database
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
