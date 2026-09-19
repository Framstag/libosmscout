/*
  This source is part of the libosmscout library
  Copyright (C) 2026  Tim Teulings

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
*/

#include <osmscoutclient/FavoriteLocationService.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <filesystem>
#include <fstream>
#include <thread>
#include <vector>

TEST_CASE("Create empty favorites file on first access")
{
    std::filesystem::path tmp = std::filesystem::temp_directory_path() / "fav_locations_create_test.json";
    std::error_code ec;
    std::filesystem::remove(tmp, ec);

    osmscout::FavoriteLocationService service(tmp.string());

    REQUIRE(std::filesystem::exists(tmp));
    REQUIRE(service.GetGroups().empty());

    std::ifstream stream(tmp);
    std::string content((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    REQUIRE(content.find("\"groups\"") != std::string::npos);

    std::filesystem::remove(tmp, ec);
}

TEST_CASE("Add and retrieve groups")
{
    std::filesystem::path tmp = std::filesystem::temp_directory_path() / "fav_locations_groups_test.json";
    std::error_code ec;
    std::filesystem::remove(tmp, ec);

    osmscout::FavoriteLocationService service(tmp.string());

    REQUIRE(service.AddGroup("Work"));
    REQUIRE(service.AddGroup("Home"));
    REQUIRE_FALSE(service.AddGroup("Work"));

    auto groups = service.GetGroups();
    REQUIRE(groups.size() == 2);
    REQUIRE(service.DeleteGroup("Work"));
    REQUIRE_FALSE(service.DeleteGroup("Missing"));

    groups = service.GetGroups();
    REQUIRE(groups.size() == 1);

    std::filesystem::remove(tmp, ec);
}

TEST_CASE("Add and delete favorites")
{
    std::filesystem::path tmp = std::filesystem::temp_directory_path() / "fav_locations_favs_test.json";
    std::error_code ec;
    std::filesystem::remove(tmp, ec);

    osmscout::FavoriteLocationService service(tmp.string());

    service.AddGroup("Work");

    osmscout::FavLocation office;
    office.name = "Office";
    office.lat = 51.1657;
    office.lon = 10.4515;

    REQUIRE(service.AddFavorite("Work", office));
    REQUIRE_FALSE(service.AddFavorite("Work", office)); // duplicate
    REQUIRE_FALSE(service.AddFavorite("Missing", office)); // group missing

    auto favs = service.GetFavorites("Work");
    REQUIRE(favs.size() == 1);
    REQUIRE(favs[0].name == "Office");
    REQUIRE(favs[0].lat == 51.1657);
    REQUIRE(favs[0].lon == 10.4515);

    REQUIRE(service.DeleteFavorite("Work", "Office"));
    REQUIRE_FALSE(service.DeleteFavorite("Work", "Office"));

    std::filesystem::remove(tmp, ec);
}

TEST_CASE("Rename favorite")
{
    std::filesystem::path tmp = std::filesystem::temp_directory_path() / "fav_locations_rename_test.json";
    std::error_code ec;
    std::filesystem::remove(tmp, ec);

    osmscout::FavoriteLocationService service(tmp.string());

    service.AddGroup("Group1");

    osmscout::FavLocation a;
    a.name = "A";
    a.lat = 1.0;
    a.lon = 2.0;
    osmscout::FavLocation b;
    b.name = "B";
    b.lat = 3.0;
    b.lon = 4.0;

    service.AddFavorite("Group1", a);
    service.AddFavorite("Group1", b);

    REQUIRE(service.RenameFavorite("Group1", "A", "A2"));
    REQUIRE_FALSE(service.RenameFavorite("Group1", "A2", "B")); // duplicate
    REQUIRE_FALSE(service.RenameFavorite("Group1", "Missing", "X"));

    auto favs = service.GetFavorites("Group1");
    REQUIRE(favs.size() == 2);
    REQUIRE(favs[0].name == "A2");

    std::filesystem::remove(tmp, ec);
}

TEST_CASE("Save and reload preserves data")
{
    std::filesystem::path tmp = std::filesystem::temp_directory_path() / "fav_locations_save_test.json";
    std::error_code ec;
    std::filesystem::remove(tmp, ec);

    {
        osmscout::FavoriteLocationService service(tmp.string());
        service.AddGroup("Cities");

        osmscout::FavLocation berlin;
        berlin.name = "Berlin";
        berlin.lat = 52.52;
        berlin.lon = 13.405;
        berlin.attributes["country"] = "DE";

        service.AddFavorite("Cities", berlin);
        REQUIRE(service.Save());
    }

    {
        osmscout::FavoriteLocationService service(tmp.string());
        auto groups = service.GetGroups();
        REQUIRE(groups.size() == 1);
        REQUIRE(groups[0].name == "Cities");
        REQUIRE(groups[0].favorites.size() == 1);
        REQUIRE(groups[0].favorites[0].name == "Berlin");
        REQUIRE(groups[0].favorites[0].lat == 52.52);
        REQUIRE(groups[0].favorites[0].attributes["country"] == "DE");
    }

    std::filesystem::remove(tmp, ec);
}

TEST_CASE("Concurrent reads do not crash")
{
    std::filesystem::path tmp = std::filesystem::temp_directory_path() / "fav_locations_thread_test.json";
    std::error_code ec;
    std::filesystem::remove(tmp, ec);

    osmscout::FavoriteLocationService service(tmp.string());
    service.AddGroup("Group");

    osmscout::FavLocation fav;
    fav.name = "F";
    fav.lat = 0.0;
    fav.lon = 0.0;
    service.AddFavorite("Group", fav);

    std::vector<std::thread> threads;
    for (int i = 0; i < 4; i++) {
        threads.emplace_back([&service]() {
            for (int j = 0; j < 100; j++) {
                auto groups = service.GetGroups();
                auto favs = service.GetFavorites("Group");
                (void)groups;
                (void)favs;
            }
        });
    }

    for (auto &t : threads) {
        t.join();
    }

    REQUIRE(service.GetGroups().size() == 1);
    REQUIRE(service.GetFavorites("Group").size() == 1);

    std::filesystem::remove(tmp, ec);
}

TEST_CASE("Star and unstar a favorite")
{
    std::filesystem::path tmp = std::filesystem::temp_directory_path() / "fav_locations_star_test.json";
    std::error_code ec;
    std::filesystem::remove(tmp, ec);

    osmscout::FavoriteLocationService service(tmp.string());
    service.AddGroup("Work");

    osmscout::FavLocation office;
    office.name = "Office";
    office.lat = 51.0;
    office.lon = 10.0;
    service.AddFavorite("Work", office);

    // Star it
    REQUIRE(service.SetStarred("Work", "Office", true));
    REQUIRE(service.IsStarred("Work", "Office"));

    // Check attribute stored
    auto favs = service.GetFavorites("Work");
    REQUIRE(favs[0].attributes["starred"] == "true");

    // Unstar
    REQUIRE(service.SetStarred("Work", "Office", false));
    REQUIRE_FALSE(service.IsStarred("Work", "Office"));

    // Key removed from attributes
    favs = service.GetFavorites("Work");
    REQUIRE(favs[0].attributes.find("starred") == favs[0].attributes.end());

    // Non-existent group/fav
    REQUIRE_FALSE(service.SetStarred("Missing", "X", true));
    REQUIRE_FALSE(service.IsStarred("Missing", "X"));
    REQUIRE_FALSE(service.SetStarred("Work", "Missing", true));
    REQUIRE_FALSE(service.IsStarred("Work", "Missing"));

    std::filesystem::remove(tmp, ec);
}

TEST_CASE("Star persists across save and load")
{
    std::filesystem::path tmp = std::filesystem::temp_directory_path() / "fav_locations_star_persist_test.json";
    std::error_code ec;
    std::filesystem::remove(tmp, ec);

    {
        osmscout::FavoriteLocationService service(tmp.string());
        service.AddGroup("Work");

        osmscout::FavLocation office;
        office.name = "Office";
        office.lat = 51.0;
        office.lon = 10.0;
        service.AddFavorite("Work", office);
        service.SetStarred("Work", "Office", true);
        REQUIRE(service.Save());
    }

    {
        osmscout::FavoriteLocationService service(tmp.string());
        REQUIRE(service.IsStarred("Work", "Office"));
    }

    std::filesystem::remove(tmp, ec);
}

TEST_CASE("Set and get group color")
{
    std::filesystem::path tmp = std::filesystem::temp_directory_path() / "fav_locations_color_test.json";
    std::error_code ec;
    std::filesystem::remove(tmp, ec);

    osmscout::FavoriteLocationService service(tmp.string());
    service.AddGroup("Work");

    // Set valid color
    REQUIRE(service.SetGroupColor("Work", "FF5733"));
    REQUIRE(service.GetGroupColor("Work") == "FF5733");

    // Check attribute stored
    auto groups = service.GetGroups();
    REQUIRE(groups[0].attributes["color"] == "FF5733");

    // Clear color
    REQUIRE(service.SetGroupColor("Work", ""));
    REQUIRE(service.GetGroupColor("Work").empty());

    // Key removed from attributes
    groups = service.GetGroups();
    REQUIRE(groups[0].attributes.find("color") == groups[0].attributes.end());

    // Invalid colors rejected
    REQUIRE_FALSE(service.SetGroupColor("Work", "XYZ"));
    REQUIRE_FALSE(service.SetGroupColor("Work", "FF573"));   // 5 chars
    REQUIRE_FALSE(service.SetGroupColor("Work", "FF57330")); // 7 chars

    // Non-existent group
    REQUIRE_FALSE(service.SetGroupColor("Missing", "FF5733"));
    REQUIRE(service.GetGroupColor("Missing").empty());

    std::filesystem::remove(tmp, ec);
}

TEST_CASE("Group color persists across save and load")
{
    std::filesystem::path tmp = std::filesystem::temp_directory_path() / "fav_locations_color_persist_test.json";
    std::error_code ec;
    std::filesystem::remove(tmp, ec);

    {
        osmscout::FavoriteLocationService service(tmp.string());
        service.AddGroup("Work");
        service.SetGroupColor("Work", "00AAFF");
        REQUIRE(service.Save());
    }

    {
        osmscout::FavoriteLocationService service(tmp.string());
        REQUIRE(service.GetGroupColor("Work") == "00AAFF");
    }

    std::filesystem::remove(tmp, ec);
}

// Appends a favorite carrying only a name; coordinates are irrelevant for
// ordering tests.
static void AddNamedFavorite(osmscout::FavoriteLocationService &service,
                            const std::string &groupName,
                            const std::string &favName)
{
    osmscout::FavLocation fav;
    fav.name = favName;
    fav.lat = 1.0;
    fav.lon = 2.0;
    service.AddFavorite(groupName, fav);
}

// Renders a group's favorites as a comma separated list of names, so that the
// full order can be asserted in one expression.
static std::string FavoriteNames(const osmscout::FavoriteLocationService &service,
                                 const std::string &groupName)
{
    std::string result;

    for (const auto &fav : service.GetFavorites(groupName)) {
        if (!result.empty()) {
            result += ',';
        }
        result += fav.name;
    }

    return result;
}

TEST_CASE("Move favorite within a group")
{
    std::filesystem::path tmp = std::filesystem::temp_directory_path() / "fav_locations_move_test.json";
    std::error_code ec;
    std::filesystem::remove(tmp, ec);

    osmscout::FavoriteLocationService service(tmp.string());
    service.AddGroup("Work");
    AddNamedFavorite(service, "Work", "A");
    AddNamedFavorite(service, "Work", "B");
    AddNamedFavorite(service, "Work", "C");
    AddNamedFavorite(service, "Work", "D");

    REQUIRE(FavoriteNames(service, "Work") == "A,B,C,D");

    // Move to the front
    REQUIRE(service.MoveFavorite("Work", "D", 0));
    REQUIRE(FavoriteNames(service, "Work") == "D,A,B,C");

    // Move to the middle (index refers to the list after removal)
    REQUIRE(service.MoveFavorite("Work", "D", 2));
    REQUIRE(FavoriteNames(service, "Work") == "A,B,D,C");

    // Move to the last position
    REQUIRE(service.MoveFavorite("Work", "A", 3));
    REQUIRE(FavoriteNames(service, "Work") == "B,D,C,A");

    // Index beyond the end is clamped to the last position
    REQUIRE(service.MoveFavorite("Work", "B", 100));
    REQUIRE(FavoriteNames(service, "Work") == "D,C,A,B");

    // Moving to the current position succeeds without changing anything
    REQUIRE(service.MoveFavorite("Work", "D", 0));
    REQUIRE(FavoriteNames(service, "Work") == "D,C,A,B");

    // Unknown group / unknown favorite
    REQUIRE_FALSE(service.MoveFavorite("Missing", "A", 0));
    REQUIRE_FALSE(service.MoveFavorite("Work", "Missing", 0));
    REQUIRE(FavoriteNames(service, "Work") == "D,C,A,B");

    // A group with a single favorite: no-op that reports success
    service.AddGroup("Single");
    AddNamedFavorite(service, "Single", "Only");
    REQUIRE(service.MoveFavorite("Single", "Only", 0));
    REQUIRE(service.MoveFavorite("Single", "Only", 7));
    REQUIRE(FavoriteNames(service, "Single") == "Only");

    // An empty group has nothing to move
    service.AddGroup("Empty");
    REQUIRE_FALSE(service.MoveFavorite("Empty", "A", 0));

    std::filesystem::remove(tmp, ec);
}

TEST_CASE("Favorite order is per group and stable across other operations")
{
    std::filesystem::path tmp = std::filesystem::temp_directory_path() / "fav_locations_order_test.json";
    std::error_code ec;
    std::filesystem::remove(tmp, ec);

    osmscout::FavoriteLocationService service(tmp.string());
    service.AddGroup("First");
    service.AddGroup("Second");

    for (const auto &group : {"First", "Second"}) {
        AddNamedFavorite(service, group, "Home");
        AddNamedFavorite(service, group, "Work");
        AddNamedFavorite(service, group, "Gym");
    }

    // Same names in two groups: moving "Home" in the first group leaves the
    // second group's order untouched.
    REQUIRE(service.MoveFavorite("First", "Home", 2));
    REQUIRE(FavoriteNames(service, "First") == "Work,Gym,Home");
    REQUIRE(FavoriteNames(service, "Second") == "Home,Work,Gym");

    // Adding appends at the end
    AddNamedFavorite(service, "First", "New");
    REQUIRE(FavoriteNames(service, "First") == "Work,Gym,Home,New");

    // Deleting keeps the relative order of the remaining favorites
    REQUIRE(service.DeleteFavorite("First", "Gym"));
    REQUIRE(FavoriteNames(service, "First") == "Work,Home,New");

    // Renaming keeps the position
    REQUIRE(service.RenameFavorite("First", "Home", "Flat"));
    REQUIRE(FavoriteNames(service, "First") == "Work,Flat,New");

    // Starring keeps the position and does not disturb the order
    REQUIRE(service.SetStarred("First", "Work", true));
    REQUIRE(FavoriteNames(service, "First") == "Work,Flat,New");
    REQUIRE(service.SetStarred("First", "Work", false));
    REQUIRE(FavoriteNames(service, "First") == "Work,Flat,New");

    std::filesystem::remove(tmp, ec);
}

TEST_CASE("Favorite order persists across save and load")
{
    std::filesystem::path tmp = std::filesystem::temp_directory_path() / "fav_locations_order_persist_test.json";
    std::error_code ec;
    std::filesystem::remove(tmp, ec);

    {
        osmscout::FavoriteLocationService service(tmp.string());
        service.AddGroup("Cities");
        AddNamedFavorite(service, "Cities", "Berlin");
        AddNamedFavorite(service, "Cities", "Paris");
        AddNamedFavorite(service, "Cities", "Rome");

        REQUIRE(service.MoveFavorite("Cities", "Rome", 0));
        REQUIRE(service.MoveFavorite("Cities", "Paris", 2));
        REQUIRE(FavoriteNames(service, "Cities") == "Rome,Berlin,Paris");
        REQUIRE(service.Save());
    }

    {
        osmscout::FavoriteLocationService service(tmp.string());
        REQUIRE(FavoriteNames(service, "Cities") == "Rome,Berlin,Paris");
    }

    std::filesystem::remove(tmp, ec);
}
