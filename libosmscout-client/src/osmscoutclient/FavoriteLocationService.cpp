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

#include <osmscoutclient/FavoriteLocationService.h>

#include <osmscoutclient/json/json.hpp>

#include <fstream>
#include <filesystem>
#include <mutex>

namespace osmscout {

static const char *JSON_KEY_GROUPS = "groups";
static const char *JSON_KEY_NAME = "name";
static const char *JSON_KEY_ATTRIBUTES = "attributes";
static const char *JSON_KEY_FAVORITES = "favorites";
static const char *JSON_KEY_LAT = "lat";
static const char *JSON_KEY_LON = "lon";

FavoriteLocationService::FavoriteLocationService(const std::string &filePath)
  : filePath_(filePath)
{
  // Create empty file on first access if it doesn't exist
  if (!std::filesystem::exists(filePath_)) {
    Save();
  } else {
    Load();
  }
}

bool FavoriteLocationService::Load()
{
  std::unique_lock lock(mutex_);

  std::ifstream stream(filePath_);
  if (!stream.is_open()) {
    return false;
  }

  try {
    nlohmann::json root;
    stream >> root;

    groups_.clear();

    auto groupsIt = root.find(JSON_KEY_GROUPS);
    if (groupsIt == root.end() || !groupsIt->is_object()) {
      return true; // empty file, no groups yet
    }

    for (auto &[key, groupJson] : groupsIt->items()) {
      FavLocationGroup group;
      group.name = groupJson.value(JSON_KEY_NAME, key);

      // Extensible attributes
      auto attrsIt = groupJson.find(JSON_KEY_ATTRIBUTES);
      if (attrsIt != groupJson.end() && attrsIt->is_object()) {
        for (auto &[attrKey, attrVal] : attrsIt->items()) {
          group.attributes[attrKey] = attrVal.get<std::string>();
        }
      }

      // Favorites
      auto favsIt = groupJson.find(JSON_KEY_FAVORITES);
      if (favsIt != groupJson.end() && favsIt->is_array()) {
        for (auto &favJson : *favsIt) {
          FavLocation fav;
          fav.name = favJson.value(JSON_KEY_NAME, "");
          fav.lat = favJson.value(JSON_KEY_LAT, 0.0);
          fav.lon = favJson.value(JSON_KEY_LON, 0.0);

          auto favAttrsIt = favJson.find(JSON_KEY_ATTRIBUTES);
          if (favAttrsIt != favJson.end() && favAttrsIt->is_object()) {
            for (auto &[attrKey, attrVal] : favAttrsIt->items()) {
              fav.attributes[attrKey] = attrVal.get<std::string>();
            }
          }

          group.favorites.push_back(std::move(fav));
        }
      }

      groups_[group.name] = std::move(group);
    }

    return true;
  } catch (const nlohmann::json::exception &) {
    groups_.clear();
    return false;
  }
}

bool FavoriteLocationService::Save()
{
  // Write to temp file first, then atomic rename
  std::string tmpPath = filePath_ + ".tmp";

  {
    std::unique_lock lock(mutex_);

    nlohmann::json root;

    nlohmann::json groupsJson = nlohmann::json::object();
    for (auto &[name, group] : groups_) {
      nlohmann::json groupJson;
      groupJson[JSON_KEY_NAME] = group.name;

      // Attributes
      nlohmann::json attrsJson = nlohmann::json::object();
      for (auto &[k, v] : group.attributes) {
        attrsJson[k] = v;
      }
      groupJson[JSON_KEY_ATTRIBUTES] = attrsJson;

      // Favorites
      nlohmann::json favsJson = nlohmann::json::array();
      for (auto &fav : group.favorites) {
        nlohmann::json favJson;
        favJson[JSON_KEY_NAME] = fav.name;
        favJson[JSON_KEY_LAT] = fav.lat;
        favJson[JSON_KEY_LON] = fav.lon;

        nlohmann::json favAttrsJson = nlohmann::json::object();
        for (auto &[k, v] : fav.attributes) {
          favAttrsJson[k] = v;
        }
        favJson[JSON_KEY_ATTRIBUTES] = favAttrsJson;

        favsJson.push_back(std::move(favJson));
      }
      groupJson[JSON_KEY_FAVORITES] = favsJson;

      groupsJson[name] = groupJson;
    }
    root[JSON_KEY_GROUPS] = groupsJson;

    std::ofstream stream(tmpPath);
    if (!stream.is_open()) {
      return false;
    }
    stream << root.dump(2) << std::endl;
    stream.close();
  } // release lock before rename

  std::error_code ec;
  std::filesystem::rename(tmpPath, filePath_, ec);
  return !ec;
}

std::vector<FavLocationGroup> FavoriteLocationService::GetGroups() const
{
  std::shared_lock lock(mutex_);

  std::vector<FavLocationGroup> result;
  result.reserve(groups_.size());
  for (auto &[name, group] : groups_) {
    result.push_back(group);
  }
  return result;
}

bool FavoriteLocationService::AddGroup(const std::string &name)
{
  std::unique_lock lock(mutex_);

  if (groups_.find(name) != groups_.end()) {
    return false;
  }

  FavLocationGroup group;
  group.name = name;
  groups_[name] = std::move(group);
  return true;
}

bool FavoriteLocationService::DeleteGroup(const std::string &name)
{
  std::unique_lock lock(mutex_);

  auto it = groups_.find(name);
  if (it == groups_.end()) {
    return false;
  }

  groups_.erase(it);
  return true;
}

std::vector<FavLocation> FavoriteLocationService::GetFavorites(const std::string &groupName) const
{
  std::shared_lock lock(mutex_);

  auto it = groups_.find(groupName);
  if (it == groups_.end()) {
    return {};
  }

  return it->second.favorites;
}

bool FavoriteLocationService::AddFavorite(const std::string &groupName, const FavLocation &fav)
{
  std::unique_lock lock(mutex_);

  auto it = groups_.find(groupName);
  if (it == groups_.end()) {
    return false;
  }

  // Check for duplicate name
  for (auto &existing : it->second.favorites) {
    if (existing.name == fav.name) {
      return false;
    }
  }

  it->second.favorites.push_back(fav);
  return true;
}

bool FavoriteLocationService::DeleteFavorite(const std::string &groupName, const std::string &favName)
{
  std::unique_lock lock(mutex_);

  auto it = groups_.find(groupName);
  if (it == groups_.end()) {
    return false;
  }

  auto &favs = it->second.favorites;
  for (auto fit = favs.begin(); fit != favs.end(); ++fit) {
    if (fit->name == favName) {
      favs.erase(fit);
      return true;
    }
  }

  return false;
}

bool FavoriteLocationService::RenameFavorite(const std::string &groupName,
                                              const std::string &oldName,
                                              const std::string &newName)
{
  std::unique_lock lock(mutex_);

  auto it = groups_.find(groupName);
  if (it == groups_.end()) {
    return false;
  }

  auto &favs = it->second.favorites;
  FavLocation *target = nullptr;

  for (auto &fav : favs) {
    if (fav.name == oldName) {
      target = &fav;
    }
    if (fav.name == newName) {
      // newName already exists — conflict
      return false;
    }
  }

  if (!target) {
    return false;
  }

  target->name = newName;
  return true;
}

}
