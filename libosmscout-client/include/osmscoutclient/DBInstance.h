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

private:
  mutable std::mutex                      mutex;
  std::chrono::steady_clock::time_point   lastUsage;      ///< last time when db was used, guarded by mutex

  GeoBox                        dbBox;          ///< cached db GeoBox, may be accessed without lock and lastUsage update
  DatabaseRef                   database;

  LocationServiceRef            locationService;
  LocationDescriptionServiceRef locationDescriptionService;
  MapServiceRef                 mapService;

  StyleConfigRef                styleConfig;

public:
  DBInstance(const std::string &path,
             const DatabaseRef& database,
             const LocationServiceRef& locationService,
             const LocationDescriptionServiceRef& locationDescriptionService,
             const MapServiceRef& mapService,
             const StyleConfigRef& styleConfig):
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

  void Close();
};

using DBInstanceRef = std::shared_ptr<DBInstance>;

}

#endif /* OSMSCOUT_CLIENT_DBINSTANCE_H */
