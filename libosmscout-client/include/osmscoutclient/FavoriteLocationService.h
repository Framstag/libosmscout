#ifndef OSMSCOUT_CLIENT_FAVORITELOCATIONSERVICE_H
#define OSMSCOUT_CLIENT_FAVORITELOCATIONSERVICE_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2026  Tim Teulings

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

#include <map>
#include <shared_mutex>
#include <string>
#include <vector>

namespace osmscout {

/**
 * \ingroup ClientAPI
 *
 * A single favorite location with a name, geographic coordinate,
 * and an extensible attribute map for future fields.
 */
struct OSMSCOUT_CLIENT_API FavLocation
{
  std::string name;
  double lat = 0.0;
  double lon = 0.0;
  std::map<std::string, std::string> attributes;
};

/**
 * \ingroup ClientAPI
 *
 * A named group of favorite locations. Groups form a one-level
 * hierarchy. Each group has an extensible attribute map.
 */
struct OSMSCOUT_CLIENT_API FavLocationGroup
{
  std::string name;
  std::vector<FavLocation> favorites;
  std::map<std::string, std::string> attributes;
};

/**
 * \ingroup ClientAPI
 *
 * Service for managing favorite locations persisted to a JSON file.
 *
 * Provides CRUD operations on groups and favorites within groups.
 * Thread-safe: read operations use shared locks, write operations
 * use exclusive locks.
 *
 * The JSON file is created on first construction if it does not exist.
 * Call Save() to persist in-memory state to disk.
 */
class OSMSCOUT_CLIENT_API FavoriteLocationService
{
public:
  /**
   * Construct the service and load data from the given file path.
   * If the file does not exist, an empty store is initialised.
   *
   * @param filePath  path to the JSON file for persistence
   */
  explicit FavoriteLocationService(const std::string &filePath);

  /**
   * Load/reload data from the JSON file.
   *
   * @return true on success, false on parse error
   */
  bool Load();

  /**
   * Save current in-memory state to the JSON file.
   * Writes to a temp file first, then atomically renames.
   *
   * @return true on success, false on write error
   */
  bool Save();

  /**
   * Return all groups.
   */
  std::vector<FavLocationGroup> GetGroups() const;

  /**
   * Add a new empty group.
   *
   * @param name  group name (must be unique)
   * @return true if added, false if name already exists
   */
  bool AddGroup(const std::string &name);

  /**
   * Delete a group and all its favorites.
   *
   * @param name  group name
   * @return true if deleted, false if not found
   */
  bool DeleteGroup(const std::string &name);

  /**
   * Rename a group.
   *
   * @param oldName  current group name
   * @param newName  new group name (must be unique)
   * @return true if renamed, false if oldName not found or newName already exists
   */
  bool RenameGroup(const std::string &oldName,
                   const std::string &newName);

  /**
   * Return all favorites in a group.
   *
   * @param groupName  group name
   * @return list of favorites, empty if group not found
   */
  std::vector<FavLocation> GetFavorites(const std::string &groupName) const;

  /**
   * Add a favorite to a group.
   *
   * @param groupName  group name
   * @param fav        the favorite to add (name must be unique within group)
   * @return true if added, false if group not found or duplicate name
   */
  bool AddFavorite(const std::string &groupName, const FavLocation &fav);

  /**
   * Delete a favorite from a group.
   *
   * @param groupName  group name
   * @param favName    favorite name to delete
   * @return true if deleted, false if group or fav not found
   */
  bool DeleteFavorite(const std::string &groupName, const std::string &favName);

  /**
   * Rename a favorite within a group.
   *
   * @param groupName  group name
   * @param oldName    current favorite name
   * @param newName    new favorite name (must be unique within group)
   * @return true if renamed, false if old not found or new name exists
   */
  bool RenameFavorite(const std::string &groupName,
                      const std::string &oldName,
                      const std::string &newName);

  /**
   * Remove all groups and favorites from memory.
   * Used when rebuilding state from an external source.
   */
  void ClearAll();

private:
  mutable std::shared_mutex mutex_;
  std::string filePath_;
  std::map<std::string, FavLocationGroup> groups_;
};

}

#endif
