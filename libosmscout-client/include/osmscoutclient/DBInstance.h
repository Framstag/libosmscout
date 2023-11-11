#ifndef OSMSCOUT_CLIENT_DBINSTANCE_H
#define OSMSCOUT_CLIENT_DBINSTANCE_H

/*
  This source is part of the libosmscout library
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
#include <osmscout/async/Thread.h>
#include <osmscout/async/Signal.h>
#include <osmscout/async/Breaker.h>

#include <osmscoutmap/MapService.h>
#include <osmscoutmap/MapPainter.h>

#include <osmscoutclient/ClientImportExport.h>

#include <chrono>
#include <thread>

namespace osmscout {

/**
 * \ingroup ClientAPI
 *
 * Instance of one osmscout db and db specific objects.
 *
 * It is thread safe
 */
class OSMSCOUT_CLIENT_API DBInstance
{
public:
  const std::string                       path;

  using MapPainterRef=std::shared_ptr<MapPainter>;

private:
  mutable std::mutex                      mutex;
  std::map<std::thread::id,MapPainterRef> painterHolder;  ///< thread-local cache of map painters, guarded by mutex
  std::chrono::steady_clock::time_point   lastUsage;      ///< last time when db was used, guarded by mutex

  osmscout::GeoBox                        dbBox;          ///< cached db GeoBox, may be accessed without lock and lastUsage update
  osmscout::DatabaseRef                   database;

  osmscout::LocationServiceRef            locationService;
  osmscout::LocationDescriptionServiceRef locationDescriptionService;
  osmscout::MapServiceRef                 mapService;

  osmscout::StyleConfigRef                styleConfig;

  Slot<std::thread::id>                   threadFinishSlot;

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
    styleConfig(styleConfig),
    threadFinishSlot(std::bind(&DBInstance::OnThreadFinished, this, std::placeholders::_1))
  {
    if (!database->GetBoundingBox(dbBox)){
      osmscout::log.Error() << "Failed to get db GeoBox: " << path;
    }
    lastUsage=std::chrono::steady_clock::now();
  };

  DBInstance(const DBInstance&) = delete;
  DBInstance(DBInstance&&) = delete;

  DBInstance& operator=(const DBInstance&) = delete;
  DBInstance& operator=(DBInstance&&) = delete;

  virtual ~DBInstance()
  {
    Close();
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
    std::scoped_lock lock(mutex);
    lastUsage=std::chrono::steady_clock::now();
    return database;
  }

  osmscout::MapServiceRef GetMapService()
  {
    std::scoped_lock lock(mutex);
    lastUsage=std::chrono::steady_clock::now();
    return mapService;
  }

  osmscout::LocationDescriptionServiceRef GetLocationDescriptionService()
  {
    std::scoped_lock lock(mutex);
    lastUsage=std::chrono::steady_clock::now();
    return locationDescriptionService;
  }

  osmscout::LocationServiceRef GetLocationService()
  {
    std::scoped_lock lock(mutex);
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
    std::scoped_lock lock(mutex);
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-lastUsage);
  }

  bool LoadStyle(const std::string &stylesheetFilename,
                 std::unordered_map<std::string,bool> stylesheetFlags,
                 std::list<StyleError> &errors);

  /**
   * Get or create thread local MapPainter instance for this map
   * \note To make sure that painter will not be destroyed during usage,
   * read-lock for databases should be held.
   * \warning It may be null when styleConfig is not loaded!
   * @return pointer to thread-local painter
   */
  template<typename PainterType,
           typename Requires = std::enable_if_t<std::is_base_of_v<MapPainter, PainterType>>>
  std::shared_ptr<PainterType> GetPainter()
  {
    std::scoped_lock lock(mutex);
    if (!styleConfig) {
      return nullptr;
    }

    if (auto it=painterHolder.find(std::this_thread::get_id());
        it!=painterHolder.end()){
      if (std::shared_ptr<PainterType> res=std::dynamic_pointer_cast<PainterType>(it->second);
          res!=nullptr) {
        return res;
      } else {
        log.Warn() << "Thread changed painter type";
      }
    }

    std::shared_ptr<PainterType> res=std::make_shared<PainterType>(styleConfig);
    painterHolder[std::this_thread::get_id()]=res;

    ThreadExitSignal().Connect(threadFinishSlot);

    return res;
  }

  void Close();

private:
  void OnThreadFinished(const std::thread::id &id);
};

using DBInstanceRef = std::shared_ptr<DBInstance>;

}

#endif /* OSMSCOUT_CLIENT_DBINSTANCE_H */
