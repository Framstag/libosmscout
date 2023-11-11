#ifndef OSMSCOUT_CLIENT_MAPMANAGER_H
#define OSMSCOUT_CLIENT_MAPMANAGER_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2023 Lukáš Karas

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

#include <osmscoutclient/ClientImportExport.h>

#include <osmscoutclient/MapDirectory.h>

#include <osmscout/async/Signal.h>
#include <osmscout/async/AsyncWorker.h>

#include <filesystem>

namespace osmscout {

/**
 * \ingroup ClientAPI
 *
 * Manager of map databases. It provide db lookup (in databaseDirectories).
 */
class OSMSCOUT_CLIENT_API MapManager: public AsyncWorker
{
public:
  Slot<> lookupDatabases;
  Signal<std::vector<std::filesystem::path>> databaseListChanged;

private:
  const std::vector<std::filesystem::path> databaseLookupDirs;
  std::vector<MapDirectory> databaseDirectories;
  std::mutex mutex;

public:
  explicit MapManager(const std::vector<std::filesystem::path> &databaseLookupDirs);
  MapManager(const MapManager&) = delete;
  MapManager(MapManager&&) = delete;

  MapManager& operator=(const MapManager&) = delete;
  MapManager& operator=(MapManager&&) = delete;

  ~MapManager() override = default;

  std::vector<std::filesystem::path> GetLookupDirectories() const
  {
    return databaseLookupDirs;
  }

  std::vector<MapDirectory> GetDatabaseDirectories() const
  {
    std::unique_lock<std::mutex> lock;
    return databaseDirectories;
  }

  /** Lookup map databases
   *
   * @return future, always true
   */
  CancelableFuture<bool> LookupDatabases();

  /** Deletes databases with logical map path other than specified by filesystem path
   *
   * @param mapPath
   * @param fsPath
   * @return future boolean, true when some database was deleted, false otherwise
   */
  CancelableFuture<bool> DeleteOther(const std::vector<std::string> &mapPath, const std::filesystem::path &fsPath);
};

using MapManagerRef = std::shared_ptr<MapManager>;

}

#endif //OSMSCOUT_CLIENT_MAPMANAGER_H
