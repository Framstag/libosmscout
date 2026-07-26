#ifndef OSMSCOUT_CLIENT_AVAILABLEMAPENTRY_H
#define OSMSCOUT_CLIENT_AVAILABLEMAPENTRY_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2025  Tim Teulings

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
#include <osmscoutclient/MapProvider.h>

#include <osmscout/util/Time.h>

#include <osmscoutclient/json/json_fwd.hpp>

#include <memory>
#include <string>
#include <vector>

namespace osmscout {

/**
 * \ingroup ClientAPI
 *
 * Represents a single entry in the available maps tree returned by a map provider.
 * An entry is either a directory (grouping of maps) or a leaf map that can be downloaded.
 */
class OSMSCOUT_CLIENT_API AvailableMapEntry
{
public:
  /**
   * Create a directory entry.
   */
  AvailableMapEntry(const std::string &name,
                    const std::vector<std::string> &path,
                    const std::string &description);

  /**
   * Create a leaf map entry.
   */
  AvailableMapEntry(const std::string &name,
                    const std::vector<std::string> &path,
                    const std::string &description,
                    const MapProvider &provider,
                    uint64_t size,
                    const std::string &serverDirectory,
                    const Timestamp &creation,
                    int version);

  AvailableMapEntry(const AvailableMapEntry &) = default;
  AvailableMapEntry(AvailableMapEntry &&) = default;
  ~AvailableMapEntry() = default;

  AvailableMapEntry& operator=(const AvailableMapEntry &) = default;
  AvailableMapEntry& operator=(AvailableMapEntry &&) = default;

  bool IsValid() const { return valid; }
  bool IsDirectory() const { return isDirectory; }

  std::string GetName() const { return name; }
  std::vector<std::string> GetPath() const { return path; }
  std::string GetDescription() const { return description; }

  /** Only valid for leaf maps. */
  MapProvider GetProvider() const { return provider; }

  /** Size in bytes. 0 for directories. */
  uint64_t GetSize() const { return size; }

  /** Server directory path for download. Empty for directories. */
  std::string GetServerDirectory() const { return serverDirectory; }

  /** Map creation timestamp. Invalid for directories. */
  Timestamp GetCreation() const { return creation; }

  /** Map data version. -1 for directories. */
  int GetVersion() const { return version; }

  /** Child entries (for directories). */
  std::vector<AvailableMapEntry> GetChildren() const { return children; }
  void AddChild(const AvailableMapEntry &child) { children.push_back(child); }

  /**
   * Parse a JSON array of map/directory entries from a map provider's list endpoint.
   *
   * Expected JSON format:
   * \code
   * [
   *   {
   *     "version" : 10,
   *     "timestamp" : 1480801927,
   *     "name" : "Czech Republic",
   *     "directory" : "europe/czech-republic-10-20161203",
   *     "size" : 622036876,
   *     "map" : "europe/czech-republic"
   *   },
   *   {
   *     "dir" : "europe",
   *     "name" : "Europe"
   *   }
   * ]
   * \endcode
   *
   * @param json     JSON array to parse
   * @param provider the provider that returned this listing
   * @return list of top-level entries (directories may have children nested via path)
   */
  static std::vector<AvailableMapEntry> FromJsonArray(const nlohmann::json &json,
                                                       const MapProvider &provider);

private:
  bool valid{false};
  bool isDirectory{false};
  std::string name;
  std::vector<std::string> path;
  std::string description;
  MapProvider provider;
  uint64_t size{0};
  std::string serverDirectory;
  Timestamp creation;
  int version{-1};
  std::vector<AvailableMapEntry> children;
};

}

#endif /* OSMSCOUT_CLIENT_AVAILABLEMAPENTRY_H */
