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

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <exception>
#include <fstream>
#include <filesystem>
#include <mutex>
#include <ostream>
#include <shared_mutex>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace osmscout {

namespace {

const char *const JSON_KEY_FORMAT_VERSION = "formatVersion";
const char *const JSON_KEY_GROUPS = "groups";
const char *const JSON_KEY_NAME = "name";
const char *const JSON_KEY_ATTRIBUTES = "attributes";
const char *const JSON_KEY_FAVORITES = "favorites";
const char *const JSON_KEY_LAT = "lat";
const char *const JSON_KEY_LON = "lon";
const char *const JSON_KEY_STARRED = "starred";
const char *const JSON_KEY_STARRED_POSITION = "starredPosition";

/**
 * The distance between two neighbouring star positions. The values are spaced
 * apart so that a later insertion between two entries does not have to renumber
 * its neighbours; a move that cannot be expressed by picking a free value in
 * between falls back to writing the whole order again.
 */
const long long STARRED_POSITION_STEP = 100;

/**
 * One entry of the starred order, as collected before the order is applied. The
 * indices locate the favorite so that a reordering can be written back without
 * a second lookup.
 */
struct StarOrderEntry
{
  size_t groupIndex = 0;
  size_t favIndex = 0;
  std::string groupName;
  std::string favName;
  bool hasPosition = false;
  long long position = 0;
};

bool IsFavStarred(const FavLocation &fav)
{
  auto it = fav.attributes.find(JSON_KEY_STARRED);
  return it != fav.attributes.end() && it->second == "true";
}

/**
 * Read the star position of a favorite. Returns false when the favorite carries
 * no value, when the value is not a number, or when it is not a number in full,
 * which is how a hand-edited file is tolerated.
 */
bool ReadStarPosition(const FavLocation &fav,
                      long long &position)
{
  auto it = fav.attributes.find(JSON_KEY_STARRED_POSITION);
  if (it == fav.attributes.end()) {
    return false;
  }

  try {
    size_t consumed = 0;
    long long value = std::stoll(it->second, &consumed);

    if (consumed != it->second.size()) {
      return false;
    }

    position = value;
    return true;
  } catch (const std::exception &) {
    return false;
  }
}

/**
 * The starred order: known positions first, in ascending order, then everything
 * without a usable position, and ties broken by the group order and the favorite
 * name. The result is a total order, whatever the file holds.
 */
bool StarOrderLess(const StarOrderEntry &a,
                   const StarOrderEntry &b)
{
  if (a.hasPosition != b.hasPosition) {
    return a.hasPosition;
  }

  if (a.hasPosition && a.position != b.position) {
    return a.position < b.position;
  }

  if (a.groupIndex != b.groupIndex) {
    return a.groupIndex < b.groupIndex;
  }

  return a.favName < b.favName;
}

/**
 * Collect the starred favorites of all groups, sorted into the starred order.
 */
std::vector<StarOrderEntry> CollectStarred(const std::vector<FavLocationGroup> &groups)
{
  std::vector<StarOrderEntry> result;

  for (size_t groupIndex = 0; groupIndex < groups.size(); groupIndex++) {
    const auto &group = groups[groupIndex];

    for (size_t favIndex = 0; favIndex < group.favorites.size(); favIndex++) {
      const auto &fav = group.favorites[favIndex];

      if (!IsFavStarred(fav)) {
        continue;
      }

      StarOrderEntry entry;
      entry.groupIndex = groupIndex;
      entry.favIndex = favIndex;
      entry.groupName = group.name;
      entry.favName = fav.name;
      entry.hasPosition = ReadStarPosition(fav, entry.position);

      result.push_back(std::move(entry));
    }
  }

  std::sort(result.begin(), result.end(), StarOrderLess);

  return result;
}

/**
 * Read one group from its JSON object. `fallbackName` is used when the object
 * carries no name of its own, which is the shape of a group in the pre-version
 * form of the file (the group's key was its name).
 */
FavLocationGroup ReadGroup(const nlohmann::json &groupJson,
                           const std::string &fallbackName)
{
  FavLocationGroup group;
  group.name = groupJson.value(JSON_KEY_NAME, fallbackName);

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

  return group;
}

} // namespace

FavLocationGroup *FavoriteLocationService::FindGroup(const std::string &name)
{
  for (auto &group : groups_) {
    if (group.name == name) {
      return &group;
    }
  }

  return nullptr;
}

const FavLocationGroup *FavoriteLocationService::FindGroup(const std::string &name) const
{
  for (const auto &group : groups_) {
    if (group.name == name) {
      return &group;
    }
  }

  return nullptr;
}

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

    // A document without a version is the pre-version form of the file.
    int version = LegacyFileFormatVersion;
    auto versionIt = root.find(JSON_KEY_FORMAT_VERSION);
    if (versionIt != root.end() && versionIt->is_number_integer()) {
      version = versionIt->get<int>();
    }

    if (version > CurrentFileFormatVersion) {
      // Written by a newer client: report the version, read no groups from it
      // and keep the file off limits for writing, so content this client does
      // not understand is never overwritten.
      fileVersion_ = version;
      fileVersionSupported_ = false;
      return false;
    }

    fileVersion_ = version;
    fileVersionSupported_ = true;

    auto groupsIt = root.find(JSON_KEY_GROUPS);
    if (groupsIt == root.end()) {
      return true; // empty file, no groups yet
    }

    if (groupsIt->is_array()) {
      // Ordered form: the sequence of the array is the group order
      for (auto &groupJson : *groupsIt) {
        groups_.push_back(ReadGroup(groupJson, ""));
      }
    } else if (groupsIt->is_object()) {
      // Pre-version form: groups keyed by name, carrying no order. Report them
      // sorted by name, which is the order the reader of that form reported.
      for (auto &[key, groupJson] : groupsIt->items()) {
        groups_.push_back(ReadGroup(groupJson, key));
      }

      std::sort(groups_.begin(),
                groups_.end(),
                [](const FavLocationGroup &a, const FavLocationGroup &b) {
                  return a.name < b.name;
                });
    }

    return true;
  } catch (const nlohmann::json::exception &) {
    // Nothing could be read, so no version is known and nothing may be written
    // over the file.
    groups_.clear();
    fileVersion_ = UnknownFileFormatVersion;
    fileVersionSupported_ = false;
    return false;
  }
}

bool FavoriteLocationService::Save()
{
  // Write to temp file first, then atomic rename
  std::string tmpPath = filePath_ + ".tmp";

  {
    std::unique_lock lock(mutex_);

    // A file carrying a version this client does not understand is never
    // overwritten, so content written by a newer client survives.
    if (!fileVersionSupported_) {
      return false;
    }

    // Hand-edited content can carry a starred order that its values cannot
    // express: a starred favorite without a position, an unparsable value, or
    // two favorites claiming the same place. The reader tolerates that with a
    // defined fallback order, and the writer makes the file self-consistent
    // again by writing the whole reported order with its own values.
    std::vector<StarOrderEntry> starred = CollectStarred(groups_);
    bool needsStarNormalization = false;

    for (size_t i = 0; i < starred.size() && !needsStarNormalization; i++) {
      if (!starred[i].hasPosition) {
        needsStarNormalization = true;
        break;
      }

      for (size_t j = i + 1; j < starred.size(); j++) {
        if (starred[j].hasPosition && starred[j].position == starred[i].position) {
          needsStarNormalization = true;
          break;
        }
      }
    }

    if (needsStarNormalization) {
      for (size_t i = 0; i < starred.size(); i++) {
        groups_[starred[i].groupIndex].favorites[starred[i].favIndex]
          .attributes[JSON_KEY_STARRED_POSITION] =
            std::to_string((static_cast<long long>(i) + 1) * STARRED_POSITION_STEP);
      }
    }

    nlohmann::json root;
    root[JSON_KEY_FORMAT_VERSION] = CurrentFileFormatVersion;

    nlohmann::json groupsJson = nlohmann::json::array();
    for (auto &group : groups_) {
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

      groupsJson.push_back(std::move(groupJson));
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

  if (ec) {
    return false;
  }

  // The file now carries the version this client writes.
  std::unique_lock lock(mutex_);
  fileVersion_ = CurrentFileFormatVersion;
  fileVersionSupported_ = true;
  return true;
}

int FavoriteLocationService::GetFileFormatVersion() const
{
  std::shared_lock lock(mutex_);

  return fileVersion_;
}

bool FavoriteLocationService::IsFileFormatSupported() const
{
  std::shared_lock lock(mutex_);

  return fileVersionSupported_;
}

std::vector<FavLocationGroup> FavoriteLocationService::GetGroups() const
{
  std::shared_lock lock(mutex_);

  return groups_;
}

bool FavoriteLocationService::AddGroup(const std::string &name)
{
  std::unique_lock lock(mutex_);

  if (FindGroup(name) != nullptr) {
    return false;
  }

  FavLocationGroup group;
  group.name = name;
  groups_.push_back(std::move(group));
  return true;
}

bool FavoriteLocationService::DeleteGroup(const std::string &name)
{
  std::unique_lock lock(mutex_);

  for (auto it = groups_.begin(); it != groups_.end(); ++it) {
    if (it->name == name) {
      groups_.erase(it);
      return true;
    }
  }

  return false;
}

bool FavoriteLocationService::RenameGroup(const std::string &oldName,
                                          const std::string &newName)
{
  std::unique_lock lock(mutex_);

  FavLocationGroup *group = FindGroup(oldName);
  if (group == nullptr) {
    return false;
  }

  if (FindGroup(newName) != nullptr) {
    return false;
  }

  // The group keeps its position: only the name changes.
  group->name = newName;
  return true;
}

bool FavoriteLocationService::MoveGroup(const std::string &name,
                                        size_t newIndex)
{
  std::unique_lock lock(mutex_);

  auto groupIt = groups_.end();
  for (auto it = groups_.begin(); it != groups_.end(); ++it) {
    if (it->name == name) {
      groupIt = it;
      break;
    }
  }

  if (groupIt == groups_.end()) {
    return false;
  }

  // Take the group out, then insert it at the (clamped) target index. Moving a
  // group to the position it already occupies is a no-op that still reports
  // success.
  FavLocationGroup group = std::move(*groupIt);
  groups_.erase(groupIt);

  newIndex = std::min(newIndex, groups_.size());

  groups_.insert(groups_.begin() + static_cast<std::vector<FavLocationGroup>::difference_type>(newIndex),
                 std::move(group));
  return true;
}

std::vector<FavLocation> FavoriteLocationService::GetFavorites(const std::string &groupName) const
{
  std::shared_lock lock(mutex_);

  const FavLocationGroup *group = FindGroup(groupName);
  if (group == nullptr) {
    return {};
  }

  return group->favorites;
}

bool FavoriteLocationService::AddFavorite(const std::string &groupName, const FavLocation &fav)
{
  std::unique_lock lock(mutex_);

  FavLocationGroup *group = FindGroup(groupName);
  if (group == nullptr) {
    return false;
  }

  // Check for duplicate name
  for (auto &existing : group->favorites) {
    if (existing.name == fav.name) {
      return false;
    }
  }

  group->favorites.push_back(fav);
  return true;
}

bool FavoriteLocationService::DeleteFavorite(const std::string &groupName, const std::string &favName)
{
  std::unique_lock lock(mutex_);

  FavLocationGroup *group = FindGroup(groupName);
  if (group == nullptr) {
    return false;
  }

  auto &favs = group->favorites;
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

  FavLocationGroup *group = FindGroup(groupName);
  if (group == nullptr) {
    return false;
  }

  auto &favs = group->favorites;
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

bool FavoriteLocationService::MoveFavorite(const std::string &groupName,
                                           const std::string &favName,
                                           size_t newIndex)
{
  std::unique_lock lock(mutex_);

  FavLocationGroup *group = FindGroup(groupName);
  if (group == nullptr) {
    return false;
  }

  auto &favs = group->favorites;

  auto favIt = favs.end();
  for (auto it = favs.begin(); it != favs.end(); ++it) {
    if (it->name == favName) {
      favIt = it;
      break;
    }
  }

  if (favIt == favs.end()) {
    return false;
  }

  // Take the favorite out, then insert it at the (clamped) target index.
  // Moving a favorite to the position it already occupies is a no-op that
  // still reports success.
  FavLocation fav = std::move(*favIt);
  favs.erase(favIt);

  newIndex = std::min(newIndex, favs.size());

  favs.insert(favs.begin() + static_cast<std::vector<FavLocation>::difference_type>(newIndex),
              std::move(fav));
  return true;
}

bool FavoriteLocationService::MoveFavoriteToGroup(const std::string &srcGroup,
                                                  const std::string &favName,
                                                  const std::string &dstGroup,
                                                  size_t newIndex)
{
  std::unique_lock lock(mutex_);

  FavLocationGroup *source = FindGroup(srcGroup);
  if (source == nullptr) {
    return false;
  }

  FavLocationGroup *target = FindGroup(dstGroup);
  if (target == nullptr) {
    return false;
  }

  auto &srcFavs = source->favorites;

  auto favIt = srcFavs.end();
  for (auto it = srcFavs.begin(); it != srcFavs.end(); ++it) {
    if (it->name == favName) {
      favIt = it;
      break;
    }
  }

  if (favIt == srcFavs.end()) {
    return false;
  }

  // Moving a favorite into the group it already belongs to succeeds without
  // changing anything.
  if (source == target) {
    return true;
  }

  // The collision check runs before anything is removed, so a refused move
  // cannot leave the favorite in limbo and leaves both groups unchanged.
  for (const auto &existing : target->favorites) {
    if (existing.name == favName) {
      return false;
    }
  }

  // Take the favorite out of its group, then insert it at the (clamped) target
  // index of the destination group.
  FavLocation fav = std::move(*favIt);
  srcFavs.erase(favIt);

  auto &dstFavs = target->favorites;

  newIndex = std::min(newIndex, dstFavs.size());

  dstFavs.insert(dstFavs.begin() + static_cast<std::vector<FavLocation>::difference_type>(newIndex),
                 std::move(fav));
  return true;
}

bool FavoriteLocationService::SetStarred(const std::string &groupName,
                                          const std::string &favName,
                                          bool starred)
{
  std::unique_lock lock(mutex_);

  FavLocationGroup *group = FindGroup(groupName);
  if (group == nullptr) {
    return false;
  }

  for (auto &fav : group->favorites) {
    if (fav.name != favName) {
      continue;
    }

    if (!starred) {
      // Unstarring takes the favorite out of the starred order together with the
      // position it had in it.
      fav.attributes.erase(JSON_KEY_STARRED);
      fav.attributes.erase(JSON_KEY_STARRED_POSITION);
      return true;
    }

    if (IsFavStarred(fav)) {
      // Starring a favorite that is already starred leaves its place unchanged.
      fav.attributes[JSON_KEY_STARRED] = "true";
      return true;
    }

    // Starring appends at the end of the starred order.
    long long maxPosition = 0;
    for (const auto &entry : CollectStarred(groups_)) {
      if (entry.hasPosition && entry.position > maxPosition) {
        maxPosition = entry.position;
      }
    }

    fav.attributes[JSON_KEY_STARRED] = "true";
    fav.attributes[JSON_KEY_STARRED_POSITION] = std::to_string(maxPosition + STARRED_POSITION_STEP);
    return true;
  }

  return false;
}

std::vector<FavLocationStarredEntry> FavoriteLocationService::GetStarred() const
{
  std::shared_lock lock(mutex_);

  std::vector<FavLocationStarredEntry> result;

  for (const auto &entry : CollectStarred(groups_)) {
    FavLocationStarredEntry starred;
    starred.groupName = entry.groupName;
    starred.favorite = groups_[entry.groupIndex].favorites[entry.favIndex];

    result.push_back(std::move(starred));
  }

  return result;
}

bool FavoriteLocationService::MoveStarred(const std::string &groupName,
                                          const std::string &favName,
                                          size_t newIndex)
{
  std::unique_lock lock(mutex_);

  std::vector<StarOrderEntry> order = CollectStarred(groups_);

  auto entryIt = order.end();
  for (auto it = order.begin(); it != order.end(); ++it) {
    if (it->groupName == groupName && it->favName == favName) {
      entryIt = it;
      break;
    }
  }

  if (entryIt == order.end()) {
    // Unknown group, unknown favorite or a favorite that is not starred: the
    // starred order stays as it is.
    return false;
  }

  // Take the entry out of the order, then insert it at the (clamped) target
  // index. Moving an entry to the position it already occupies is a no-op that
  // still reports success.
  StarOrderEntry moved = *entryIt;
  order.erase(entryIt);

  newIndex = std::min(newIndex, order.size());

  order.insert(order.begin() + static_cast<std::vector<StarOrderEntry>::difference_type>(newIndex),
               std::move(moved));

  // Write the resulting sequence back with the service's own values, spaced so
  // that a later insertion between two entries needs no renumbering.
  for (size_t i = 0; i < order.size(); i++) {
    groups_[order[i].groupIndex].favorites[order[i].favIndex]
      .attributes[JSON_KEY_STARRED_POSITION] =
        std::to_string((static_cast<long long>(i) + 1) * STARRED_POSITION_STEP);
  }

  return true;
}

bool FavoriteLocationService::IsStarred(const std::string &groupName,
                                         const std::string &favName) const
{
  std::shared_lock lock(mutex_);

  const FavLocationGroup *group = FindGroup(groupName);
  if (group == nullptr) {
    return false;
  }

  for (const auto &fav : group->favorites) {
    if (fav.name == favName) {
      auto it = fav.attributes.find("starred");
      return it != fav.attributes.end() && it->second == "true";
    }
  }

  return false;
}

bool FavoriteLocationService::SetGroupColor(const std::string &groupName,
                                              const std::string &color)
{
  std::unique_lock lock(mutex_);

  FavLocationGroup *group = FindGroup(groupName);
  if (group == nullptr) {
    return false;
  }

  if (color.empty()) {
    group->attributes.erase("color");
    return true;
  }

  // Validate: exactly 6 hex characters
  if (color.length() != 6) {
    return false;
  }
  for (char c : color) {
    if (!std::isxdigit(static_cast<unsigned char>(c))) {
      return false;
    }
  }

  group->attributes["color"] = color;
  return true;
}

std::string FavoriteLocationService::GetGroupColor(const std::string &groupName) const
{
  std::shared_lock lock(mutex_);

  const FavLocationGroup *group = FindGroup(groupName);
  if (group == nullptr) {
    return std::string();
  }

  auto it = group->attributes.find("color");
  if (it == group->attributes.end()) {
    return std::string();
  }

  return it->second;
}

void FavoriteLocationService::ClearAll()
{
  std::unique_lock lock(mutex_);
  groups_.clear();
}

}
