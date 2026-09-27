#ifndef OSMSCOUT_CLIENT_FAVORITESTORE_H
#define OSMSCOUT_CLIENT_FAVORITESTORE_H

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
#include <osmscoutclient/FavoriteLocationService.h>

#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace osmscout {

/**
 * \ingroup ClientAPI
 *
 * Owns the favorite location service and serialises the operations that
 * replace the store as a whole.
 *
 * FavoriteLocationService is thread-safe per call, but two of its uses replace
 * the store wholesale rather than mutating it: loading a file builds a fresh
 * service, and saving rebuilds the in-memory state from caller-supplied data
 * (ClearAll() followed by one Add per group and favorite). Those sequences are
 * not atomic in the service, so a concurrent reader can observe a half-rebuilt
 * store, and a caller holding the service pointer cannot destroy it safely
 * while a call is in flight.
 *
 * This class closes both gaps by keeping the service instance behind a single
 * mutex: every operation runs under it, and the wholesale replacements
 * (ReplaceByPath(), ReplaceAndSave()) run as one critical section. Callers
 * therefore never observe a partial store, never operate on a destroyed
 * instance, and never have to reason about the service's lifetime.
 *
 * All methods are safe to call from any thread and at any time; when no store
 * is loaded they return false (or an empty result) instead of faulting.
 */
class OSMSCOUT_CLIENT_API FavoriteStore
{
public:
  FavoriteStore() = default;
  ~FavoriteStore();

  FavoriteStore(const FavoriteStore &) = delete;
  FavoriteStore &operator=(const FavoriteStore &) = delete;

  /**
   * Replace the store with a new service backed by the given file.
   * The file is created if it does not exist and loaded if it does.
   *
   * The replaced content is discarded, even when the file cannot be read: the
   * service constructor does not report a load failure, so this operation
   * reports "a store is installed", not "the file was read". Use HasStore()
   * followed by a read operation to see what the store holds.
   *
   * @param filePath  path to the JSON file for persistence
   * @return true
   */
  bool ReplaceByPath(const std::string &filePath);

  /**
   * Replace the store with the given groups and persist it to the given file.
   *
   * The caller's snapshot is applied as one atomic replacement: no reader can
   * observe the store while it is being rebuilt. The group order and the starred
   * order of the snapshot are kept: the groups are applied in the order given,
   * and the starred favorites carry their place with them. Group attributes are
   * applied
   * through the service (currently the "color" attribute); favorite attributes
   * are stored with the favorite.
   *
   * When the content cannot be written, the operation reports false and the
   * store keeps the supplied content: the previous content is not restored, so
   * a caller that wants to keep it has to supply it again.
   *
   * @param filePath  path to the JSON file for persistence
   * @param groups    the complete new content of the store, in the order it should have
   * @return true on success, false on write error or when the file carries a version this client
   *         does not understand (then the supplied content is kept in memory and the file is left
   *         unchanged)
   */
  bool ReplaceAndSave(const std::string &filePath,
                      const std::vector<FavLocationGroup> &groups);

  /**
   * Whether a store is currently loaded.
   */
  bool HasStore() const;

  /**
   * Destroy the store. Safe to call more than once; a later call that needs a
   * store returns false (or an empty result) until one is loaded again.
   */
  void Shutdown();

  /**
   * Return all groups, in the user-defined group order. Empty when no store is
   * loaded.
   */
  std::vector<FavLocationGroup> GetGroups() const;

  /**
   * Return the starred favorites in their user-defined order, each entry naming
   * the group that holds it. Empty when no store is loaded.
   */
  std::vector<FavLocationStarredEntry> GetStarred() const;

  /**
   * The format version of the file the loaded store is backed by, or
   * FavoriteLocationService::UnknownFileFormatVersion when no store is loaded.
   */
  int GetFileFormatVersion() const;

  /**
   * Whether the loaded store can be read and written. False when no store is
   * loaded, and false when the file carries a version this client does not
   * understand: in that case the store reports no groups and refuses to persist
   * over the file, so content written by a newer client survives.
   */
  bool IsFileFormatSupported() const;

  bool AddGroup(const std::string &name);

  bool DeleteGroup(const std::string &name);

  bool RenameGroup(const std::string &oldName,
                   const std::string &newName);

  bool MoveGroup(const std::string &name,
                 size_t newIndex);

  bool AddFavorite(const std::string &groupName,
                   const FavLocation &fav);

  bool DeleteFavorite(const std::string &groupName,
                      const std::string &favName);

  bool RenameFavorite(const std::string &groupName,
                      const std::string &oldName,
                      const std::string &newName);

  bool MoveFavorite(const std::string &groupName,
                    const std::string &favName,
                    size_t newIndex);

  bool MoveFavoriteToGroup(const std::string &srcGroup,
                           const std::string &favName,
                           const std::string &dstGroup,
                           size_t newIndex);

  bool MoveStarred(const std::string &groupName,
                   const std::string &favName,
                   size_t newIndex);

  bool SetStarred(const std::string &groupName,
                  const std::string &favName,
                  bool starred);

  bool IsStarred(const std::string &groupName,
                 const std::string &favName) const;

  bool SetGroupColor(const std::string &groupName,
                     const std::string &color);

  std::string GetGroupColor(const std::string &groupName) const;

private:
  mutable std::mutex mutex_;

  /**
   * The owned service instance. Every access happens under mutex_, which is why
   * the lock order is always store-then-service and can never be the reverse.
   */
  std::unique_ptr<FavoriteLocationService> service_;
};

}

#endif
