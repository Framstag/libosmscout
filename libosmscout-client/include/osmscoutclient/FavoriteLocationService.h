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
 * A starred favorite together with the group that holds it. The starred order
 * spans groups, so a caller presenting it needs to know for each entry which
 * group the favorite lives in (for example to select that group, or to show the
 * owning group next to the entry).
 */
struct OSMSCOUT_CLIENT_API FavLocationStarredEntry
{
  std::string groupName;
  FavLocation favorite;
};

/**
 * \ingroup ClientAPI
 *
 * Service for managing favorite locations persisted to a JSON file.
 *
 * Provides CRUD operations on groups and favorites within groups. The groups
 * have a user-defined order: it is the order a reader observes, the order a
 * positional operation changes and the order that is saved.
 *
 * Thread-safe: read operations use shared locks, write operations
 * use exclusive locks.
 *
 * The JSON file is created on first construction if it does not exist.
 * Call Save() to persist in-memory state to disk.
 *
 * The layout of that file, every version it can carry and the compatibility
 * policy are documented in `Documentation/FavoritesFileFormat.md`.
 */
class OSMSCOUT_CLIENT_API FavoriteLocationService
{
public:
  /**
   * The format version this client writes, and the highest version it can read.
   */
  static constexpr int CurrentFileFormatVersion = 1;

  /**
   * The format version of a favorites file written before the file carried a
   * version: the groups are keyed by name and there is no stored group order.
   * Such a file is still read, and is written back in the current form on the
   * next save. Support for reading it is a compatibility path with a planned
   * end (see the file format document).
   */
  static constexpr int LegacyFileFormatVersion = 0;

  /**
   * The version of the format of the file this service is opened on, or -1 when
   * no version is known because nothing could be read.
   */
  static constexpr int UnknownFileFormatVersion = -1;

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
   * A file without a version is read as the pre-version form. A file whose
   * version is newer than CurrentFileFormatVersion is not read as groups: the
   * state reports that version and that it is unsupported, and Save() refuses to
   * overwrite that file. Use GetFileFormatVersion() and IsFileFormatSupported()
   * to tell an unsupported file apart from an empty one.
   *
   * @return true on success, false on parse error or unsupported version
   */
  bool Load();

  /**
   * Save current in-memory state to the JSON file.
   * Writes to a temp file first, then atomically renames. The written document
   * carries CurrentFileFormatVersion and the groups in their stored order.
   *
   * Refuses to write when the file this service was opened on carries a version
   * newer than CurrentFileFormatVersion, so content this client does not
   * understand is never overwritten.
   *
   * @return true on success, false on write error or unsupported file version
   */
  bool Save();

  /**
   * The format version of the file this service is opened on, or
   * UnknownFileFormatVersion when no version is known.
   */
  int GetFileFormatVersion() const;

  /**
   * Whether the file this service is opened on carries a version this client can
   * read and write. False for an unsupported (newer) version and when no version
   * is known.
   */
  bool IsFileFormatSupported() const;

  /**
   * Return all groups, in the user-defined group order.
   *
   * The order is the stored order: it is what this reader reports, what
   * MoveGroup() changes and what a save persists. A group that is added is
   * appended at the end.
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
   * Move a group to another position in the group order.
   *
   * The target index is 0-based and refers to the group order after the group
   * has been removed from its current position. An index outside the order
   * bounds is clamped to the first/last position, so moving a group to the
   * front or to the end does not depend on the caller knowing how many groups
   * there are. Moving a group to the position it already occupies succeeds
   * without changing anything.
   *
   * Only the position changes: the group keeps its name, its attributes and its
   * favorites, and the relative order of the other groups is unchanged.
   *
   * @param name      group name
   * @param newIndex  0-based target position in the group order
   * @return true if moved (or already at that position), false if the group is not found
   */
  bool MoveGroup(const std::string &name,
                 size_t newIndex);

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
   * Move a favorite to another position within its group.
   *
   * The target index is 0-based and refers to the favorite list after the
   * favorite has been removed from its current position. An index outside the
   * list bounds is clamped to the first/last position, so moving a favorite to
   * the front or to the end does not depend on the caller knowing the size.
   * Moving a favorite to the position it already occupies succeeds without
   * changing anything.
   *
   * @param groupName  group name
   * @param favName    favorite name to move
   * @param newIndex   0-based target position within the group
   * @return true if moved (or already at that position), false if group or fav not found
   */
  bool MoveFavorite(const std::string &groupName,
                    const std::string &favName,
                    size_t newIndex);

  /**
   * Set or clear the starred flag on a favorite.
   * Star is stored as attributes["starred"] = "true".
   * Unsetting removes the key entirely.
   *
   * @param groupName  group name
   * @param favName    favorite name
   * @param starred    true to star, false to unstar
   * @return true if updated, false if group or fav not found
   */
  /**
   * Move a favorite from one group into another group, at a target position.
   *
   * The target index is 0-based and refers to the destination group's favorite
   * list after the favorite has been taken out of its own group. An index
   * outside that list bounds is clamped to the first/last position. Moving a
   * favorite into the group it already belongs to succeeds and leaves both
   * positions unchanged.
   *
   * The favorite keeps its data: name, coordinates and all attributes, including
   * a star and its place in the starred order. A favorite name is unique inside a
   * group, so when the destination group already holds a favorite of that name
   * the move fails and both groups are left unchanged: the destination favorite
   * is not replaced, removed or renamed.
   *
   * @param srcGroup      group the favorite currently belongs to
   * @param favName       favorite name to move
   * @param dstGroup      group to move the favorite into
   * @param newIndex      0-based target position in the destination group
   * @return true if moved (or already in that group), false if either group or the favorite is not
   *         found, or if the destination group already holds a favorite of that name
   */
  bool MoveFavoriteToGroup(const std::string &srcGroup,
                           const std::string &favName,
                           const std::string &dstGroup,
                           size_t newIndex);

  /**
   * Return the starred favorites in their user-defined order.
   *
   * The order spans groups: it is not derived from the group order, from the
   * position of a favorite inside its group, or from any name. Only starred
   * favorites appear, and each entry names the group that holds it.
   *
   * The values that hold the order are owned by the service and are not part of
   * this contract; a caller arranges stars with MoveStarred().
   */
  std::vector<FavLocationStarredEntry> GetStarred() const;

  /**
   * Move a starred favorite to another position in the starred order.
   *
   * The target index is 0-based and refers to the starred order after the
   * favorite has been removed from its current position. An index outside the
   * order bounds is clamped to the first/last position. Moving a starred
   * favorite to the position it already occupies succeeds without changing
   * anything.
   *
   * Only the position in the starred order changes: the favorite's data, its
   * group and its position inside that group are untouched.
   *
   * @param groupName  group the favorite belongs to
   * @param favName    favorite name
   * @param newIndex   0-based target position in the starred order
   * @return true if moved (or already at that position), false if the group or the favorite is not
   *         found, or if the favorite is not starred
   */
  bool MoveStarred(const std::string &groupName,
                   const std::string &favName,
                   size_t newIndex);

  bool SetStarred(const std::string &groupName,
                  const std::string &favName,
                  bool starred);

  /**
   * Check if a favorite is starred.
   *
   * @param groupName  group name
   * @param favName    favorite name
   * @return true if starred, false if not found or not starred
   */
  bool IsStarred(const std::string &groupName,
                 const std::string &favName) const;

  /**
   * Set or clear the color of a group.
   * Color is stored as attributes["color"] = "RRGGBB" (6 hex chars).
   * Passing empty string clears the color.
   *
   * @param groupName  group name
   * @param color      6-char hex RGB string, or empty to clear
   * @return true if set, false if group not found or invalid color
   */
  bool SetGroupColor(const std::string &groupName,
                     const std::string &color);

  /**
   * Get the color of a group.
   *
   * @param groupName  group name
   * @return color string (6 hex chars) or empty if no color or group not found
   */
  std::string GetGroupColor(const std::string &groupName) const;

  /**
   * Remove all groups and favorites from memory.
   * Used when rebuilding state from an external source.
   */
  void ClearAll();

private:
  /**
   * Find a group by name. A scan over the ordered group collection, because the
   * collection's sequence is the user-defined group order and a map keyed by
   * name cannot express that order. Group counts are small, so the scan costs
   * nothing next to keeping one order instead of two structures.
   */
  FavLocationGroup *FindGroup(const std::string &name);

  const FavLocationGroup *FindGroup(const std::string &name) const;

  mutable std::shared_mutex mutex_;
  std::string filePath_;

  /**
   * The format version of the file this service is opened on, and whether that
   * version may be read and written. Both are updated by Load() and Save().
   *
   * The supported flag starts as true because nothing has been read yet that
   * could refuse a write: a file that does not exist is created by the
   * constructor through Save().
   */
  int fileVersion_ = UnknownFileFormatVersion;
  bool fileVersionSupported_ = true;

  /**
   * The groups, in the user-defined order. The sequence is the order: adding a
   * group appends at the end, renaming a group keeps its position, and the
   * positional operation moves an entry within this sequence.
   */
  std::vector<FavLocationGroup> groups_;
};

}

#endif
