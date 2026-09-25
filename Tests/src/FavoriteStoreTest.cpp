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

#include <osmscoutclient/FavoriteStore.h>
#include <osmscoutclient/FavoriteLocationService.h>

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <thread>
#include <vector>

namespace {

std::vector<osmscout::FavLocationGroup> MakeGroups(size_t groupCount,
                                                   size_t favsPerGroup,
                                                   const std::string &prefix)
{
  std::vector<osmscout::FavLocationGroup> groups;
  groups.reserve(groupCount);

  for (size_t g=0; g<groupCount; g++) {
    osmscout::FavLocationGroup group;
    group.name = prefix + std::to_string(g);
    group.attributes["color"] = "ff8800";

    for (size_t f=0; f<favsPerGroup; f++) {
      osmscout::FavLocation fav;
      fav.name = "fav" + std::to_string(f);
      fav.lat = 50.0 + static_cast<double>(g) / 100.0;
      fav.lon = 7.0 + static_cast<double>(f) / 100.0;
      group.favorites.push_back(fav);
    }

    groups.push_back(std::move(group));
  }

  return groups;
}

/**
 * True if a read returned a complete generation of the store: either empty
 * (before the first replacement) or exactly the expected content. A store that
 * is observed while it is being rebuilt shows fewer groups, or a group with
 * fewer favorites, and fails this check.
 */
bool IsCompleteGeneration(const std::vector<osmscout::FavLocationGroup> &groups,
                          size_t groupCount,
                          size_t favsPerGroup,
                          const std::string &prefix)
{
  if (groups.empty()) {
    return true;
  }

  if (groups.size()!=groupCount) {
    return false;
  }

  for (const auto &group : groups) {
    if (group.name.rfind(prefix, 0)!=0) {
      return false;
    }
    if (group.favorites.size()!=favsPerGroup) {
      return false;
    }
  }

  return true;
}

std::string ReadFile(const std::filesystem::path &path)
{
  std::ifstream stream(path);
  return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}

}

TEST_CASE("A replacement is never observed half-rebuilt")
{
  std::filesystem::path tmp = std::filesystem::temp_directory_path() / "fav_store_atomic_test.json";
  std::error_code ec;
  std::filesystem::remove(tmp, ec);

  const size_t groupCount = 8;
  const size_t favsPerGroup = 20;
  const size_t replacements = 200;
  const std::string prefix = "group";

  auto groups = MakeGroups(groupCount, favsPerGroup, prefix);

  osmscout::FavoriteStore store;

  std::atomic<bool> stop{false};
  std::atomic<size_t> reads{0};
  std::atomic<bool> partialObserved{false};

  std::thread reader([&]() {
    while (!stop.load()) {
      auto snapshot = store.GetGroups();
      reads.fetch_add(1);
      if (!IsCompleteGeneration(snapshot, groupCount, favsPerGroup, prefix)) {
        partialObserved.store(true);
        return;
      }
    }
  });

  for (size_t i=0; i<replacements; i++) {
    REQUIRE(store.ReplaceAndSave(tmp.string(), groups));
  }

  stop.store(true);
  reader.join();

  CHECK(reads.load()>0);
  CHECK_FALSE(partialObserved.load());

  std::filesystem::remove(tmp, ec);
}

TEST_CASE("Concurrent mutations during a replacement neither fault nor half-apply")
{
  std::filesystem::path tmp = std::filesystem::temp_directory_path() / "fav_store_mutation_test.json";
  std::error_code ec;
  std::filesystem::remove(tmp, ec);

  const size_t groupCount = 4;
  const size_t favsPerGroup = 10;
  const std::string prefix = "base";

  auto groups = MakeGroups(groupCount, favsPerGroup, prefix);

  osmscout::FavoriteStore store;
  REQUIRE(store.ReplaceAndSave(tmp.string(), groups));

  std::atomic<bool> stop{false};
  std::atomic<size_t> mutations{0};

  // Mutations run against the store while it is being replaced over and over.
  // They may be discarded by a replacement (the caller supplies the complete
  // new content), but they must never fault and never leave a partial state.
  std::thread mutator([&]() {
    size_t i = 0;
    while (!stop.load()) {
      store.AddGroup("extra" + std::to_string(i));
      store.SetGroupColor(prefix + "0", "00ff00");
      store.AddFavorite(prefix + "0", osmscout::FavLocation{"extra", 1.0, 2.0, {}});
      store.GetGroups();
      mutations.fetch_add(1);
      i++;
    }
  });

  for (size_t i=0; i<100; i++) {
    REQUIRE(store.ReplaceAndSave(tmp.string(), groups));
  }

  stop.store(true);
  mutator.join();

  CHECK(mutations.load()>0);

  // Mutations have stopped, so one final replacement must leave exactly the
  // supplied content: no half-applied mutation may survive it.
  REQUIRE(store.ReplaceAndSave(tmp.string(), groups));

  auto finalState = store.GetGroups();
  REQUIRE(finalState.size()==groupCount);
  for (const auto &group : finalState) {
    CHECK(group.name.rfind(prefix, 0)==0);
    CHECK(group.favorites.size()==favsPerGroup);
  }
  CHECK(store.GetGroupColor(prefix + "0")=="ff8800");

  std::filesystem::remove(tmp, ec);
}

TEST_CASE("A replaced store persists the supplied content unchanged")
{
  std::filesystem::path tmp = std::filesystem::temp_directory_path() / "fav_store_persist_test.json";
  std::error_code ec;
  std::filesystem::remove(tmp, ec);

  auto groups = MakeGroups(2, 3, "g");
  groups[0].attributes["color"] = "aabbcc";
  groups[1].favorites[1].attributes["starred"] = "true";

  osmscout::FavoriteStore store;
  REQUIRE(store.ReplaceAndSave(tmp.string(), groups));

  auto loaded = store.GetGroups();
  REQUIRE(loaded.size()==2);
  CHECK(loaded[0].name=="g0");
  CHECK(loaded[0].favorites.size()==3);
  CHECK(store.GetGroupColor("g0")=="aabbcc");
  CHECK(store.IsStarred("g1", "fav1"));
  CHECK_FALSE(store.IsStarred("g0", "fav1"));

  // The file is written completely, not partially
  std::string content = ReadFile(tmp);
  CHECK(content.find("g0")!=std::string::npos);
  CHECK(content.find("g1")!=std::string::npos);

  // An independent service reading the same file sees the same content
  osmscout::FavoriteLocationService reread(tmp.string());
  auto rereadGroups = reread.GetGroups();
  REQUIRE(rereadGroups.size()==2);
  CHECK(rereadGroups[0].favorites.size()==3);

  std::filesystem::remove(tmp, ec);
}

TEST_CASE("A store that was shut down reports no store and stays usable")
{
  std::filesystem::path tmp = std::filesystem::temp_directory_path() / "fav_store_shutdown_test.json";
  std::error_code ec;
  std::filesystem::remove(tmp, ec);

  const size_t groupCount = 2;
  const size_t favsPerGroup = 3;
  const std::string prefix = "s";

  auto groups = MakeGroups(groupCount, favsPerGroup, prefix);

  osmscout::FavoriteStore store;
  REQUIRE(store.ReplaceAndSave(tmp.string(), groups));
  REQUIRE(store.HasStore());
  REQUIRE(store.GetGroups().size()==groupCount);

  store.Shutdown();

  CHECK_FALSE(store.HasStore());
  CHECK(store.GetGroups().empty());
  CHECK_FALSE(store.AddGroup("late"));
  CHECK_FALSE(store.DeleteGroup(prefix + "0"));
  CHECK_FALSE(store.RenameGroup(prefix + "0", "renamed"));
  CHECK_FALSE(store.AddFavorite(prefix + "0", osmscout::FavLocation{"late", 1.0, 2.0, {}}));
  CHECK_FALSE(store.DeleteFavorite(prefix + "0", "fav0"));
  CHECK_FALSE(store.RenameFavorite(prefix + "0", "fav0", "fav9"));
  CHECK_FALSE(store.MoveFavorite(prefix + "0", "fav0", 1));
  CHECK_FALSE(store.SetStarred(prefix + "0", "fav0", true));
  CHECK_FALSE(store.IsStarred(prefix + "0", "fav0"));
  CHECK_FALSE(store.SetGroupColor(prefix + "0", "123456"));
  CHECK(store.GetGroupColor(prefix + "0").empty());

  // Shutting down twice is safe, and a later replacement makes it usable again
  store.Shutdown();
  REQUIRE(store.ReplaceAndSave(tmp.string(), groups));
  CHECK(store.HasStore());
  CHECK(store.GetGroups().size()==groupCount);

  std::filesystem::remove(tmp, ec);
}

TEST_CASE("A fresh store reports no store, and a failed write keeps the supplied content")
{
  // A store that never had a file set reports the no-store state
  osmscout::FavoriteStore fresh;

  CHECK_FALSE(fresh.HasStore());
  CHECK(fresh.GetGroups().empty());
  CHECK_FALSE(fresh.AddGroup("late"));

  // A replacement that cannot be written reports the failure and keeps the
  // caller's snapshot in memory: the store has no earlier generation of its own
  // to restore, so the content the caller handed over is what it holds
  std::filesystem::path missingDir = std::filesystem::temp_directory_path() / "fav_store_missing_dir_test";
  std::error_code ec;
  std::filesystem::remove_all(missingDir, ec);

  auto groups = MakeGroups(2, 2, "u");

  osmscout::FavoriteStore store;
  CHECK_FALSE(store.ReplaceAndSave((missingDir / "favs.json").string(), groups));
  REQUIRE(store.HasStore());

  auto held = store.GetGroups();
  REQUIRE(held.size()==2);
  CHECK(held[0].name=="u0");
  CHECK(held[0].favorites.size()==2);

  std::filesystem::remove_all(missingDir, ec);
}

TEST_CASE("A replacement can replace an existing store with different content")
{
  std::filesystem::path tmp = std::filesystem::temp_directory_path() / "fav_store_resize_test.json";
  std::error_code ec;
  std::filesystem::remove(tmp, ec);

  osmscout::FavoriteStore store;
  REQUIRE(store.ReplaceAndSave(tmp.string(), MakeGroups(5, 4, "big")));
  REQUIRE(store.GetGroups().size()==5);

  // Shrinking the content must not leave remnants of the previous generation
  REQUIRE(store.ReplaceAndSave(tmp.string(), MakeGroups(1, 1, "small")));

  auto shrunk = store.GetGroups();
  REQUIRE(shrunk.size()==1);
  CHECK(shrunk[0].name=="small0");
  CHECK(shrunk[0].favorites.size()==1);

  osmscout::FavoriteLocationService reread(tmp.string());
  auto rereadGroups = reread.GetGroups();
  REQUIRE(rereadGroups.size()==1);
  CHECK(rereadGroups[0].name=="small0");

  std::filesystem::remove(tmp, ec);
}
