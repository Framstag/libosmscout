  #ifndef OSMSCOUT_CLIENT_QT_DBINSTANCE_H
#define OSMSCOUT_CLIENT_QT_DBINSTANCE_H

/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2017 Lukáš Karas

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

#include <osmscout/location/LocationService.h>
#include <osmscout/location/LocationDescriptionService.h>

#include <osmscout/db/Database.h>

#include <osmscoutmap/MapService.h>
#include <osmscoutmap/MapPainter.h>

#include <osmscout/async/Breaker.h>

#include <osmscoutclientqt/ClientQtImportExport.h>

#include <QObject>
#include <QMap>
#include <QThread>

#include <chrono>

namespace osmscout {

/**
 * \ingroup QtAPI
 *
 * Instance of one osmscout db and db specific objects.
 *
 * It is thread safe
 */
class OSMSCOUT_CLIENT_QT_API DBInstance : public QObject
{
  Q_OBJECT

public:
  const std::string                       path;

private:
  mutable std::mutex                      mutex;
  QMap<QThread*,MapPainter*>              painterHolder;  ///< thread-local cache of map painters, guarded by mutex
  std::chrono::steady_clock::time_point   lastUsage;      ///< last time when db was used, guarded by mutex

  osmscout::GeoBox                        dbBox;          ///< cached db GeoBox, may be accessed without lock and lastUsage update
  osmscout::DatabaseRef                   database;

  osmscout::LocationServiceRef            locationService;
  osmscout::LocationDescriptionServiceRef locationDescriptionService;
  osmscout::MapServiceRef                 mapService;

  osmscout::StyleConfigRef                styleConfig;

public slots:
  void onThreadFinished();

public:
  DBInstance(const std::string &path,
             const osmscout::DatabaseRef& database,
             const osmscout::LocationServiceRef& locationService,
             const osmscout::LocationDescriptionServiceRef& locationDescriptionService,
             const osmscout::MapServiceRef& mapService,
             const osmscout::StyleConfigRef& styleConfig):
    path(path),
    database(database),
    locationService(locationService),
    locationDescriptionService(locationDescriptionService),
    mapService(mapService),
    styleConfig(styleConfig)
  {
    if (!database->GetBoundingBox(dbBox)){
      osmscout::log.Error() << "Failed to get db GeoBox: " << path;
    }
    lastUsage=std::chrono::steady_clock::now();
  };

  ~DBInstance() override
  {
    close();
  };

  osmscout::GeoBox GetDBGeoBox() const
  {
    return dbBox;
  }

  /**
   * return true if db is open
   * lastUsage is not updated
   */
  bool IsOpen() const
  {
    return database->IsOpen();
  }

  osmscout::DatabaseRef GetDatabase()
  {
    std::lock_guard<std::mutex> lock(mutex);
    lastUsage=std::chrono::steady_clock::now();
    return database;
  }

  osmscout::MapServiceRef GetMapService()
  {
    std::lock_guard<std::mutex> lock(mutex);
    lastUsage=std::chrono::steady_clock::now();
    return mapService;
  }

  osmscout::LocationDescriptionServiceRef GetLocationDescriptionService()
  {
    std::lock_guard<std::mutex> lock(mutex);
    lastUsage=std::chrono::steady_clock::now();
    return locationDescriptionService;
  }

  osmscout::LocationServiceRef GetLocationService()
  {
    std::lock_guard<std::mutex> lock(mutex);
    lastUsage=std::chrono::steady_clock::now();
    return locationService;
  }

  osmscout::StyleConfigRef GetStyleConfig() const
  {
    return styleConfig;
  }

  /**
   * Returns the number of milliseconds since last db usage
   */
  std::chrono::milliseconds LastUsageMs() const
  {
    std::lock_guard<std::mutex> lock(mutex);
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-lastUsage);
  }

  bool LoadStyle(QString stylesheetFilename,
                 std::unordered_map<std::string,bool> stylesheetFlags,
                 QList<StyleError> &errors);

  /**
   * Get or create thread local MapPainter instance for this map
   * \note To make sure that painter will not be destroyed during usage,
   * read-lock for databases should be held.
   * \warning It may be null when styleConfig is not loaded!
   * @return pointer to thread-local painter
   */
  template<typename PainterType,
           typename Requires = std::enable_if_t<std::is_base_of<MapPainter, PainterType>::value>>
  PainterType* GetPainter()
  {
    std::lock_guard<std::mutex> lock(mutex);
    if (!styleConfig) {
      return nullptr;
    }

    if (painterHolder.contains(QThread::currentThread())){
      MapPainter *p=painterHolder[QThread::currentThread()];
      if (PainterType* res=dynamic_cast<PainterType*>(p);
          res!=nullptr) {
        return res;
      } else {
        painterHolder.remove(QThread::currentThread());
        log.Warn() << "Thread changed painter type";
        delete p;
      }
    }

    PainterType* res=new PainterType(styleConfig);
    painterHolder[QThread::currentThread()]=res;
    connect(QThread::currentThread(), &QThread::finished,
            this, &DBInstance::onThreadFinished);
    return res;
  }

  void close();
};

using DBInstanceRef = std::shared_ptr<DBInstance>;

}

#endif /* OSMSCOUT_CLIENT_QT_DBINSTANCE_H */
