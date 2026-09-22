#ifndef OSMSCOUT_CLIENT_DATABASEPATHREGISTRY_H
#define OSMSCOUT_CLIENT_DATABASEPATHREGISTRY_H

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

#include <filesystem>
#include <mutex>
#include <vector>

namespace osmscout {

/**
 * \ingroup ClientAPI
 *
 * Result of one registration call: the complete set of registered directories
 * as it is after the call, plus the disposition of every input path.
 *
 * Both are read under the same lock, so the caller never observes a set that
 * another thread left half-updated.
 */
struct OSMSCOUT_CLIENT_API DatabasePathRegistration
{
  /**
   * The complete registered set after the call, in registration order.
   */
  std::vector<std::filesystem::path> paths;

  /**
   * Disposition of every input path, index-aligned with the input: true when
   * the path is part of the registered set after the call.
   */
  std::vector<bool> registered;

  /**
   * Number of paths this call added (the rest were already registered).
   */
  size_t added{0};
};

/**
 * \ingroup ClientAPI
 *
 * Owns the list of map database directories the client has been asked to open,
 * serialising every access to it.
 *
 * Several app paths open databases in one process - the phone map start, the
 * car session warmup, a map scan, a download completion - and they run on
 * different threads. An unguarded list is mutated and read concurrently, and
 * the caller hands the list to the database thread, which copies it while it is
 * being reallocated: a use-after-free that takes the process down.
 *
 * This class closes that gap with a single mutex around the list. Each
 * registration returns a value snapshot taken under the lock, so the caller can
 * publish the set to the database thread after the lock is released, without
 * holding an app-facing lock while the database thread works, and without any
 * lock-order relation to the database latch.
 *
 * RegisterAll() registers a whole list as **one** operation: one call, one
 * snapshot, one database-set publication. Registering K directories
 * individually needs K publications - each of them closes and reopens every
 * database - so a caller that has a list must hand it over in one call.
 *
 * All methods are safe to call from any thread and at any time.
 */
class OSMSCOUT_CLIENT_API DatabasePathRegistry
{
public:
  DatabasePathRegistry() = default;
  ~DatabasePathRegistry() = default;

  DatabasePathRegistry(const DatabasePathRegistry &) = delete;
  DatabasePathRegistry &operator=(const DatabasePathRegistry &) = delete;

  /**
   * Register a single directory, unless it is already registered.
   *
   * @param path  the map database directory
   * @return true when the path is part of the registered set afterwards
   */
  bool Register(const std::filesystem::path &path);

  /**
   * Register a list of directories as one operation, skipping paths already
   * registered and duplicates inside the list.
   *
   * @param paths the map database directories
   * @return the set after the call, the per-path disposition and the number added
   */
  DatabasePathRegistration RegisterAll(const std::vector<std::filesystem::path> &paths);

  /**
   * The registered directories, in registration order.
   */
  std::vector<std::filesystem::path> Snapshot() const;

  /**
   * Number of registered directories.
   */
  size_t Size() const;

  /**
   * How often the set was registered as a whole: one per Register() or
   * RegisterAll() call. This is the number of database-set publications the
   * caller has to issue, and it is what makes the "one coordinated change per
   * call" contract observable without a JNI environment.
   */
  size_t SetChangeCount() const;

  /**
   * Forget every registered directory. The set change count is kept, so it
   * keeps counting registrations for the lifetime of the registry.
   */
  void Clear();

private:
  mutable std::mutex mutex_;
  std::vector<std::filesystem::path> paths_;
  size_t setChangeCount_{0};
};

}

#endif
