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

#include <osmscoutclient/AvailableMapEntry.h>

#include <osmscoutclient/json/json.hpp>

#include <osmscout/log/Logger.h>

namespace osmscout {

AvailableMapEntry::AvailableMapEntry(const std::string &name,
                                      const std::vector<std::string> &path,
                                      const std::string &description)
  : valid(true)
  , isDirectory(true)
  , name(name)
  , path(path)
  , description(description)
{
}

AvailableMapEntry::AvailableMapEntry(const std::string &name,
                                      const std::vector<std::string> &path,
                                      const std::string &description,
                                      const MapProvider &provider,
                                      uint64_t size,
                                      const std::string &serverDirectory,
                                      const Timestamp &creation,
                                      int version)
  : valid(true)
  , isDirectory(false)
  , name(name)
  , path(path)
  , description(description)
  , provider(provider)
  , size(size)
  , serverDirectory(serverDirectory)
  , creation(creation)
  , version(version)
{
}

std::vector<AvailableMapEntry> AvailableMapEntry::FromJsonArray(const nlohmann::json &json,
                                                                 const MapProvider &provider)
{
  std::vector<AvailableMapEntry> result;

  if (!json.is_array()) {
    log.Error() << "AvailableMapEntry::FromJsonArray: expected JSON array, got "
                << json.type_name();
    return result;
  }

  for (const auto &item : json) {
    if (!item.is_object()) {
      continue;
    }

    // Directory entries have a "dir" field
    if (item.contains("dir") && item["dir"].is_string()) {
      std::string dirName = item["dir"].get<std::string>();
      std::string entryName;
      if (item.contains("name") && item["name"].is_string()) {
        entryName = item["name"].get<std::string>();
      } else {
        entryName = dirName;
      }

      std::string description;
      if (item.contains("description") && item["description"].is_string()) {
        description = item["description"].get<std::string>();
      }

      // Path is the directory name split by '/'
      std::vector<std::string> path;
      size_t start = 0;
      size_t end = dirName.find('/');
      while (end != std::string::npos) {
        path.push_back(dirName.substr(start, end - start));
        start = end + 1;
        end = dirName.find('/', start);
      }
      // Don't add the last segment as path — it's this entry's name

      result.emplace_back(entryName, path, description);
      continue;
    }

    // Map entries have "map" field
    if (!(item.contains("map") && item["map"].is_string())) {
      continue;
    }

    std::string mapPath = item["map"].get<std::string>();

    std::string entryName;
    if (item.contains("name") && item["name"].is_string()) {
      entryName = item["name"].get<std::string>();
    } else {
      // Use last path segment as name
      auto pos = mapPath.rfind('/');
      entryName = (pos != std::string::npos) ? mapPath.substr(pos + 1) : mapPath;
    }

    std::string description;
    if (item.contains("description") && item["description"].is_string()) {
      description = item["description"].get<std::string>();
    }

    // Parse path from "map" field (e.g., "europe/czech-republic")
    std::vector<std::string> path;
    size_t start = 0;
    size_t end = mapPath.find('/');
    while (end != std::string::npos) {
      path.push_back(mapPath.substr(start, end - start));
      start = end + 1;
      end = mapPath.find('/', start);
    }

    uint64_t size = 0;
    if (item.contains("size") && item["size"].is_number_unsigned()) {
      size = item["size"].get<uint64_t>();
    }

    std::string serverDirectory;
    if (item.contains("directory") && item["directory"].is_string()) {
      serverDirectory = item["directory"].get<std::string>();
    }

    Timestamp creation;
    if (item.contains("timestamp") && item["timestamp"].is_number_unsigned()) {
      creation = Timestamp(std::chrono::seconds(item["timestamp"].get<uint64_t>()));
    }

    int version = -1;
    if (item.contains("version") && item["version"].is_number_integer()) {
      version = item["version"].get<int>();
    }

    result.emplace_back(entryName, path, description,
                        provider, size, serverDirectory, creation, version);
  }

  return result;
}

}
