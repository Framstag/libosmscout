/*
  This source is part of the libosmscout library
  Copyright (C) 2017  Lukáš Karas

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

#include <osmscoutclient/MapManager.h>

#include <osmscout/TypeConfig.h>

namespace osmscout {

MapManager::MapManager(const std::vector<std::filesystem::path> &databaseLookupDirs):
  AsyncWorker("MapManager"),
  lookupDatabases(std::bind(&MapManager::LookupDatabases, this)),
  databaseLookupDirs(databaseLookupDirs)
{

}

CancelableFuture<bool> MapManager::LookupDatabases()
{
  return Async<bool>([this](Breaker&) -> bool{

    osmscout::log.Info() << "Lookup databases";
    std::unique_lock<std::mutex> lock;

    databaseDirectories.clear();
    std::set<std::filesystem::path> uniqPaths;
    std::vector<std::filesystem::path> databaseFsDirectories;

    for (const auto &lookupDir:databaseLookupDirs){
      // Symlinks are automatically resolved when using exists() and is_directory().
      // https://en.cppreference.com/w/cpp/filesystem/exists
      // https://en.cppreference.com/w/cpp/filesystem/is_directory
      // https://en.cppreference.com/w/cpp/filesystem/status
      if (!std::filesystem::exists(lookupDir) || !std::filesystem::is_directory(lookupDir)) {
        osmscout::log.Warn() << "Lookup dir " << lookupDir.string() << " doesn't exist or isn't a directory";
        continue;
      }

      for (const auto & fInfo : std::filesystem::recursive_directory_iterator(lookupDir)) {
        auto entryPath = fInfo.path();
        if (fInfo.is_regular_file() && entryPath.has_filename() && entryPath.has_parent_path() && entryPath.filename() == TypeConfig::FILE_TYPES_DAT){
          MapDirectory mapDir(entryPath.parent_path());
          if (mapDir.IsValid()) {
            osmscout::log.Info() << "found db " << mapDir.GetName() << ": " << mapDir.GetDirStr();
            if (uniqPaths.find(mapDir.GetDir()) == uniqPaths.end()) {
              databaseDirectories.push_back(mapDir);
              databaseFsDirectories.push_back(mapDir.GetDir());
              uniqPaths.insert(mapDir.GetDir());
            }
          }
        }
      }
    }
    databaseListChanged.Emit(databaseFsDirectories);

    return true;
  });
}

CancelableFuture<bool> MapManager::DeleteOther(const std::vector<std::string> &mapPath, const std::filesystem::path &fsPath)
{
  return Async<bool>([this, mapPath, fsPath](Breaker&) -> bool{
    std::unique_lock<std::mutex> lock;

    bool result = false;
    for (auto &mapDir:databaseDirectories) {
      if (mapDir.HasMetadata() &&
          mapDir.GetPath() == mapPath &&
        mapDir.GetDir() != fsPath) {

        osmscout::log.Debug() << "deleting map db " << mapDir.GetName() << ": " << mapDir.GetDirStr();
        mapDir.DeleteDatabase();
        result = true;
      }
    }

    if (result) {
      LookupDatabases();
    }
    return result;
  });
}

}

