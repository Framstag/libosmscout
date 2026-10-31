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

#include <algorithm>
#include <optional>

namespace osmscout {

namespace {

/**
 * The data format version the given database directory's types.dat names, or no value when that file is
 * missing, shorter than the version field, or unreadable. A version this helper cannot read decides nothing:
 * the reader stays the authority on such a directory.
 */
std::optional<uint32_t> ReadDatabaseFileFormatVersion(const std::filesystem::path &databaseDirectory)
{
  try {
    return TypeConfig::GetDatabaseFileFormatVersion(databaseDirectory.string());
  }
  catch (const std::exception &e) {
    osmscout::log.Debug() << "  data format version of " << databaseDirectory.string()
                          << " cannot be read: " << e.what();
    return std::nullopt;
  }
}

}

MapManager::MapManager(const std::vector<std::filesystem::path> &databaseLookupDirs):
  AsyncWorker("MapManager"),
  lookupDatabases(std::bind(&MapManager::LookupDatabases, this)),
  databaseLookupDirs(databaseLookupDirs)
{

}

MapManager::~MapManager()
{
  // Stop the scan before the members it reads (the registered directories and the
  // published databases) are destroyed.
  Stop();
}

CancelableFuture<bool> MapManager::LookupDatabases()
{
  return Async<bool>([this](Breaker &breaker) -> bool{

    osmscout::log.Info() << "Lookup databases";
    std::unique_lock<std::mutex> lock(lookupMutex);

    // Work on a snapshot of the registered directories. The scan must not read the
    // owner's list while it runs, because the owner's state is released when it is
    // destroyed, and a stopped scan must not publish a partially updated set.
    std::vector<std::filesystem::path> lookupDirs=databaseLookupDirs;

    std::vector<MapDirectory> foundDirectories;
    std::set<std::filesystem::path> uniqPaths;
    std::vector<std::filesystem::path> databaseFsDirectories;

    for (const auto &lookupDir:lookupDirs){
      if (breaker.IsAborted()) {
        return false;
      }

      osmscout::log.Info() << "Scanning maps lookup directory: " << lookupDir.string();

      if (!std::filesystem::exists(lookupDir) || !std::filesystem::is_directory(lookupDir)) {
        osmscout::log.Warn() << "Lookup dir " << lookupDir.string() << " doesn't exist or isn't a directory";
        continue;
      }

      size_t candidateCount = 0;
      size_t validCount = 0;
      size_t invalidCount = 0;

      try {
        for (const auto & fInfo : std::filesystem::recursive_directory_iterator(lookupDir)) {
          if (breaker.IsAborted()) {
            return false;
          }

          auto entryPath = fInfo.path();
          if (fInfo.is_regular_file() && entryPath.has_filename() && entryPath.has_parent_path() && entryPath.filename() == TypeConfig::FILE_TYPES_DAT){
            candidateCount++;
            try {
              // A directory whose data format this build does not read stays out of the database list:
              // the reader would refuse it later, with its own error, while everything registered in
              // between holds a database that never opens. One line names the skip instead.
              auto fileFormatVersion=ReadDatabaseFileFormatVersion(entryPath.parent_path());

              if (fileFormatVersion.has_value() &&
                  *fileFormatVersion!=FILE_FORMAT_VERSION) {
                invalidCount++;
                osmscout::log.Warn() << "  skipping map directory whose data format version is "
                                     << *fileFormatVersion << ", this build reads "
                                     << FILE_FORMAT_VERSION << ": "
                                     << entryPath.parent_path().string();
                continue;
              }

              MapDirectory mapDir(entryPath.parent_path());
              if (mapDir.IsValid()) {
                validCount++;
                std::string displayName = mapDir.GetName();
                if (displayName.empty()) {
                  displayName = mapDir.GetDir().filename().string();
                }
                osmscout::log.Info() << "  found valid map '" << displayName
                                     << "' at " << mapDir.GetDirStr()
                                     << (mapDir.HasMetadata() ? " (with metadata)" : " (no metadata)");
                if (uniqPaths.find(mapDir.GetDir()) == uniqPaths.end()) {
                  foundDirectories.push_back(mapDir);
                  databaseFsDirectories.push_back(mapDir.GetDir());
                  uniqPaths.insert(mapDir.GetDir());
                }
              } else {
                invalidCount++;
                osmscout::log.Info() << "  skipping invalid map directory: " << entryPath.parent_path().string();
              }
            } catch (const std::exception &e) {
              invalidCount++;
              osmscout::log.Warn() << "  error checking database at " << entryPath.parent_path().string() << ": " << e.what();
            }
          }
        }
      } catch (const std::exception &e) {
        osmscout::log.Warn() << "Error iterating directory " << lookupDir.string() << ": " << e.what();
      }

      osmscout::log.Info() << "Lookup directory " << lookupDir.string()
                           << " scan complete: " << candidateCount << " candidates, "
                           << validCount << " valid, " << invalidCount << " invalid";
    }

    if (breaker.IsAborted()) {
      return false;
    }

    databaseDirectories=foundDirectories;
    osmscout::log.Info() << "Total installed maps found: " << databaseDirectories.size();
    databaseListChanged.Emit(databaseFsDirectories);

    return true;
  });
}

void MapManager::AddLookupDirectory(const std::filesystem::path &dir)
{
  bool alreadyRegistered = false;
  {
    std::unique_lock<std::mutex> lock(lookupMutex);
    // Avoid duplicates
    for (const auto &existing : databaseLookupDirs) {
      if (std::filesystem::equivalent(existing, dir)) {
        alreadyRegistered = true;
        break;
      }
    }
    if (!alreadyRegistered) {
      databaseLookupDirs.push_back(dir);
    }
  }
  // Always re-scan, even when the directory was already registered:
  // a re-downloaded map in an existing directory must become visible
  // (fix-download).
  LookupDatabases();
}

void MapManager::RemoveLookupDirectory(const std::filesystem::path &dir)
{
  {
    std::unique_lock<std::mutex> lock(lookupMutex);
    databaseLookupDirs.erase(
        std::remove_if(databaseLookupDirs.begin(), databaseLookupDirs.end(),
                       [&dir](const std::filesystem::path &existing) {
                         std::error_code ec;
                         return std::filesystem::equivalent(existing, dir, ec) ||
                                existing == dir;
                       }),
        databaseLookupDirs.end());
  }
  // Always re-scan, so that maps from the removed directory disappear from
  // the database list and the databaseListChanged signal is emitted.
  LookupDatabases();
}

CancelableFuture<bool> MapManager::DeleteOther(const std::vector<std::string> &mapPath, const std::filesystem::path &fsPath)
{
  return Async<bool>([this, mapPath, fsPath](Breaker&) -> bool{
    std::unique_lock<std::mutex> lock(lookupMutex);

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

