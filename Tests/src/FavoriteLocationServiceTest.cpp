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

// Renders the group order as a comma separated list of names, so that the
// whole order can be asserted in one expression.
static std::string GroupNames(const osmscout::FavoriteLocationService &service)
{
    std::string result;

    for (const auto &group : service.GetGroups()) {
        if (!result.empty()) {
            result += ',';
        }
        result += group.name;
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

TEST_CASE("Move favorite keeps the favorite's data")
{
    std::filesystem::path tmp = std::filesystem::temp_directory_path() / "fav_locations_move_keep_data_test.json";
    std::error_code ec;
    std::filesystem::remove(tmp, ec);

    osmscout::FavoriteLocationService service(tmp.string());
    service.AddGroup("Work");

    osmscout::FavLocation fav;
    fav.name = "A";
    fav.lat = 51.5;
    fav.lon = 7.25;
    fav.attributes["custom"] = "value";

    REQUIRE(service.AddFavorite("Work", fav));
    AddNamedFavorite(service, "Work", "B");
    REQUIRE(service.SetStarred("Work", "A", true));

    REQUIRE(service.MoveFavorite("Work", "A", 1));
    REQUIRE(FavoriteNames(service, "Work") == "B,A");

    // The moved favorite keeps name, coordinates and attributes
    auto favs = service.GetFavorites("Work");
    REQUIRE(favs.size() == 2);
    REQUIRE(favs[1].name == "A");
    REQUIRE(favs[1].lat == 51.5);
    REQUIRE(favs[1].lon == 7.25);
    REQUIRE(favs[1].attributes["custom"] == "value");
    REQUIRE(service.IsStarred("Work", "A"));

    // The favorite that was not moved is intact as well
    REQUIRE(favs[0].name == "B");
    REQUIRE(favs[0].lat == 1.0);
    REQUIRE(favs[0].lon == 2.0);

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

TEST_CASE("Groups are reported in the order they were added")
{
    std::filesystem::path tmp = std::filesystem::temp_directory_path() / "fav_locations_group_order_test.json";
    std::error_code ec;
    std::filesystem::remove(tmp, ec);

    osmscout::FavoriteLocationService service(tmp.string());

    // Added in an order that is not the alphabetical one, so a name-derived
    // order would be visible here.
    REQUIRE(service.AddGroup("Work"));
    REQUIRE(service.AddGroup("Home"));
    REQUIRE(service.AddGroup("Uni"));

    REQUIRE(GroupNames(service) == "Work,Home,Uni");

    // Adding appends at the end
    REQUIRE(service.AddGroup("Gym"));
    REQUIRE(GroupNames(service) == "Work,Home,Uni,Gym");

    // A duplicate name is refused and does not reorder anything
    REQUIRE_FALSE(service.AddGroup("Home"));
    REQUIRE(GroupNames(service) == "Work,Home,Uni,Gym");

    std::filesystem::remove(tmp, ec);
}

TEST_CASE("Renaming a group keeps its position")
{
    std::filesystem::path tmp = std::filesystem::temp_directory_path() / "fav_locations_group_rename_order_test.json";
    std::error_code ec;
    std::filesystem::remove(tmp, ec);

    osmscout::FavoriteLocationService service(tmp.string());
    service.AddGroup("Work");
    service.AddGroup("Home");
    service.AddGroup("Uni");

    // Renaming to a name that sorts before the first group must not move it
    REQUIRE(service.RenameGroup("Home", "Flat"));
    REQUIRE(GroupNames(service) == "Work,Flat,Uni");
    REQUIRE(service.GetFavorites("Flat").empty());

    // Renaming to a name that sorts after the last group must not move it either
    REQUIRE(service.RenameGroup("Work", "Zoo"));
    REQUIRE(GroupNames(service) == "Zoo,Flat,Uni");

    // An unknown name and a name that already exists are both refused without
    // changing the order
    REQUIRE_FALSE(service.RenameGroup("Missing", "Elsewhere"));
    REQUIRE_FALSE(service.RenameGroup("Zoo", "Uni"));
    REQUIRE(GroupNames(service) == "Zoo,Flat,Uni");

    std::filesystem::remove(tmp, ec);
}

TEST_CASE("Deleting a group keeps the order of the remaining groups")
{
    std::filesystem::path tmp = std::filesystem::temp_directory_path() / "fav_locations_group_delete_order_test.json";
    std::error_code ec;
    std::filesystem::remove(tmp, ec);

    osmscout::FavoriteLocationService service(tmp.string());
    service.AddGroup("Work");
    service.AddGroup("Home");
    service.AddGroup("Uni");
    AddNamedFavorite(service, "Uni", "Campus");

    REQUIRE(service.DeleteGroup("Home"));
    REQUIRE(GroupNames(service) == "Work,Uni");
    REQUIRE(FavoriteNames(service, "Uni") == "Campus");

    REQUIRE_FALSE(service.DeleteGroup("Home"));
    REQUIRE(GroupNames(service) == "Work,Uni");

    std::filesystem::remove(tmp, ec);
}

TEST_CASE("Move group within the group order")
{
    std::filesystem::path tmp = std::filesystem::temp_directory_path() / "fav_locations_group_move_test.json";
    std::error_code ec;
    std::filesystem::remove(tmp, ec);

    osmscout::FavoriteLocationService service(tmp.string());
    service.AddGroup("Work");
    service.AddGroup("Home");
    service.AddGroup("Uni");

    REQUIRE(GroupNames(service) == "Work,Home,Uni");

    // Move to the front
    REQUIRE(service.MoveGroup("Uni", 0));
    REQUIRE(GroupNames(service) == "Uni,Work,Home");

    // Move into the middle (index refers to the order after removal)
    REQUIRE(service.MoveGroup("Uni", 1));
    REQUIRE(GroupNames(service) == "Work,Uni,Home");

    // Move to the last position
    REQUIRE(service.MoveGroup("Work", 2));
    REQUIRE(GroupNames(service) == "Uni,Home,Work");

    // Index beyond the end is clamped to the last position
    REQUIRE(service.MoveGroup("Home", 100));
    REQUIRE(GroupNames(service) == "Uni,Work,Home");

    // Moving to the current position succeeds without changing anything
    REQUIRE(service.MoveGroup("Home", 2));
    REQUIRE(GroupNames(service) == "Uni,Work,Home");

    // An unknown group fails without changing the order
    REQUIRE_FALSE(service.MoveGroup("Missing", 0));
    REQUIRE(GroupNames(service) == "Uni,Work,Home");

    // A single group: no-op that reports success
    service.AddGroup("Single");
    REQUIRE(service.MoveGroup("Single", 4));
    REQUIRE(GroupNames(service) == "Uni,Work,Home,Single");
    REQUIRE(service.MoveGroup("Single", 4));
    REQUIRE(GroupNames(service) == "Uni,Work,Home,Single");

    std::filesystem::remove(tmp, ec);
}

TEST_CASE("Move group keeps the group's content")
{
    std::filesystem::path tmp = std::filesystem::temp_directory_path() / "fav_locations_group_move_keep_data_test.json";
    std::error_code ec;
    std::filesystem::remove(tmp, ec);

    osmscout::FavoriteLocationService service(tmp.string());
    service.AddGroup("Work");
    service.AddGroup("Home");
    service.AddGroup("Uni");

    REQUIRE(service.SetGroupColor("Home", "FF5733"));
    AddNamedFavorite(service, "Home", "A");
    AddNamedFavorite(service, "Home", "B");
    AddNamedFavorite(service, "Home", "C");
    AddNamedFavorite(service, "Uni", "Campus");

    REQUIRE(service.MoveGroup("Home", 2));
    REQUIRE(GroupNames(service) == "Work,Uni,Home");

    // The moved group keeps its name, its attributes and its favorites in order
    REQUIRE(service.GetGroupColor("Home") == "FF5733");
    REQUIRE(FavoriteNames(service, "Home") == "A,B,C");

    // The groups that were not moved are intact as well
    REQUIRE(service.GetGroupColor("Work").empty());
    REQUIRE(service.GetFavorites("Work").empty());
    REQUIRE(FavoriteNames(service, "Uni") == "Campus");

    std::filesystem::remove(tmp, ec);
}

TEST_CASE("Group order persists across save and load")
{
    std::filesystem::path tmp = std::filesystem::temp_directory_path() / "fav_locations_group_order_persist_test.json";
    std::error_code ec;
    std::filesystem::remove(tmp, ec);

    {
        osmscout::FavoriteLocationService service(tmp.string());
        service.AddGroup("Work");
        service.AddGroup("Home");
        service.AddGroup("Uni");
        AddNamedFavorite(service, "Home", "Flat");

        REQUIRE(service.MoveGroup("Uni", 0));
        REQUIRE(service.MoveGroup("Home", 2));
        REQUIRE(GroupNames(service) == "Uni,Work,Home");
        REQUIRE(service.Save());
    }

    {
        osmscout::FavoriteLocationService service(tmp.string());
        REQUIRE(GroupNames(service) == "Uni,Work,Home");
        REQUIRE(FavoriteNames(service, "Home") == "Flat");
    }

    std::filesystem::remove(tmp, ec);
}

TEST_CASE("Two stores can hold the same groups in different orders")
{
    std::filesystem::path first =
        std::filesystem::temp_directory_path() / "fav_locations_two_stores_first_test.json";
    std::filesystem::path second =
        std::filesystem::temp_directory_path() / "fav_locations_two_stores_second_test.json";
    std::error_code ec;
    std::filesystem::remove(first, ec);
    std::filesystem::remove(second, ec);

    {
        osmscout::FavoriteLocationService service(first.string());
        service.AddGroup("Work");
        service.AddGroup("Home");
        REQUIRE(service.Save());
    }

    {
        osmscout::FavoriteLocationService service(second.string());
        service.AddGroup("Home");
        service.AddGroup("Work");
        REQUIRE(service.Save());
    }

    {
        osmscout::FavoriteLocationService service(first.string());
        REQUIRE(GroupNames(service) == "Work,Home");
    }

    {
        osmscout::FavoriteLocationService service(second.string());
        REQUIRE(GroupNames(service) == "Home,Work");
    }

    std::filesystem::remove(first, ec);
    std::filesystem::remove(second, ec);
}

// Writes the given content to the given path, so that tests can present files
// the current writer would not produce (the pre-version form, a newer version).
static void WriteFile(const std::filesystem::path &path,
                      const std::string &content)
{
    std::ofstream stream(path);
    stream << content;
}

static std::string ReadFile(const std::filesystem::path &path)
{
    std::ifstream stream(path);
    return std::string((std::istreambuf_iterator<char>(stream)),
                       std::istreambuf_iterator<char>());
}

TEST_CASE("A fresh favorites file carries the format version")
{
    std::filesystem::path tmp = std::filesystem::temp_directory_path() / "fav_locations_fresh_version_test.json";
    std::error_code ec;
    std::filesystem::remove(tmp, ec);

    osmscout::FavoriteLocationService service(tmp.string());

    REQUIRE(std::filesystem::exists(tmp));
    REQUIRE(service.GetGroups().empty());
    REQUIRE(service.GetFileFormatVersion() == osmscout::FavoriteLocationService::CurrentFileFormatVersion);
    REQUIRE(service.IsFileFormatSupported());

    std::string content = ReadFile(tmp);
    REQUIRE(content.find("\"formatVersion\": 1") != std::string::npos);
    REQUIRE(content.find("\"groups\": []") != std::string::npos);

    // The created file is writable, so nothing about it is off limits
    REQUIRE(service.Save());

    std::filesystem::remove(tmp, ec);
}

TEST_CASE("Pre-version favorites file is read and rewritten in the versioned form")
{
    std::filesystem::path tmp = std::filesystem::temp_directory_path() / "fav_locations_preversion_test.json";
    std::error_code ec;
    std::filesystem::remove(tmp, ec);

    WriteFile(tmp,
              R"JSON({
  "groups": {
    "Work": {
      "name": "Work",
      "attributes": { "color": "FF5733" },
      "favorites": [
        { "name": "Office", "lat": 51.5, "lon": 7.25,
          "attributes": { "starred": "true", "custom": "value" } }
      ]
    },
    "Home": {
      "name": "Home",
      "attributes": {},
      "favorites": []
    }
  }
}
)JSON");

    {
        osmscout::FavoriteLocationService service(tmp.string());

        // No version key: the pre-version form, reported sorted by group name
        REQUIRE(service.GetFileFormatVersion() == osmscout::FavoriteLocationService::LegacyFileFormatVersion);
        REQUIRE(service.IsFileFormatSupported());
        REQUIRE(GroupNames(service) == "Home,Work");

        // Nothing is lost: attributes and favorites are all there
        REQUIRE(service.GetGroupColor("Work") == "FF5733");
        REQUIRE(FavoriteNames(service, "Work") == "Office");
        auto favs = service.GetFavorites("Work");
        REQUIRE(favs.size() == 1);
        REQUIRE(favs[0].lat == 51.5);
        REQUIRE(favs[0].lon == 7.25);
        REQUIRE(favs[0].attributes["custom"] == "value");
        REQUIRE(service.IsStarred("Work", "Office"));

        // Saving converges on the versioned form
        REQUIRE(service.Save());
        REQUIRE(service.GetFileFormatVersion() == osmscout::FavoriteLocationService::CurrentFileFormatVersion);
    }

    // The rewritten file carries the version and the order, and loads as versioned content
    std::string content = ReadFile(tmp);
    REQUIRE(content.find("\"formatVersion\": 1") != std::string::npos);

    {
        osmscout::FavoriteLocationService service(tmp.string());
        REQUIRE(service.GetFileFormatVersion() == osmscout::FavoriteLocationService::CurrentFileFormatVersion);
        REQUIRE(GroupNames(service) == "Home,Work");
        REQUIRE(FavoriteNames(service, "Work") == "Office");
        REQUIRE(service.GetGroupColor("Work") == "FF5733");
        REQUIRE(service.IsStarred("Work", "Office"));
    }

    std::filesystem::remove(tmp, ec);
}

TEST_CASE("Pre-version content without groups is readable")
{
    std::filesystem::path tmp = std::filesystem::temp_directory_path() / "fav_locations_preversion_empty_test.json";
    std::error_code ec;
    std::filesystem::remove(tmp, ec);

    WriteFile(tmp, "{}");

    osmscout::FavoriteLocationService service(tmp.string());

    REQUIRE(service.GetGroups().empty());
    REQUIRE(service.GetFileFormatVersion() == osmscout::FavoriteLocationService::LegacyFileFormatVersion);
    REQUIRE(service.IsFileFormatSupported());
    REQUIRE(service.Save());

    std::filesystem::remove(tmp, ec);
}

TEST_CASE("A versioned file keeps the stored group order")
{
    std::filesystem::path tmp = std::filesystem::temp_directory_path() / "fav_locations_versioned_order_test.json";
    std::error_code ec;
    std::filesystem::remove(tmp, ec);

    WriteFile(tmp,
              R"JSON({
  "formatVersion": 1,
  "groups": [
    { "name": "Work", "attributes": {}, "favorites": [] },
    { "name": "Home", "attributes": {}, "favorites": [] },
    { "name": "Uni", "attributes": {}, "favorites": [] }
  ]
}
)JSON");

    {
        osmscout::FavoriteLocationService service(tmp.string());

        REQUIRE(service.GetFileFormatVersion() == osmscout::FavoriteLocationService::CurrentFileFormatVersion);
        REQUIRE(GroupNames(service) == "Work,Home,Uni");

        REQUIRE(service.Save());
    }

    {
        osmscout::FavoriteLocationService service(tmp.string());
        REQUIRE(GroupNames(service) == "Work,Home,Uni");
    }

    std::filesystem::remove(tmp, ec);
}

TEST_CASE("A file written by a newer client is reported and never overwritten")
{
    std::filesystem::path tmp =
        std::filesystem::temp_directory_path() / "fav_locations_newer_version_test.json";
    std::error_code ec;
    std::filesystem::remove(tmp, ec);

    WriteFile(tmp,
              R"JSON({
  "formatVersion": 2,
  "groups": [
    { "name": "Work", "attributes": {}, "favorites": [
      { "name": "Office", "lat": 51.5, "lon": 7.25, "attributes": {} }
    ] }
  ],
  "somethingNew": { "kept": "by a newer client" }
}
)JSON");

    std::string before = ReadFile(tmp);

    osmscout::FavoriteLocationService service(tmp.string());

    // The version is reported instead of the groups, so a caller can tell an
    // unsupported file apart from an empty one
    REQUIRE(service.GetFileFormatVersion() == 2);
    REQUIRE_FALSE(service.IsFileFormatSupported());
    REQUIRE(service.GetGroups().empty());

    // Every write over that path is refused and the file is untouched
    REQUIRE_FALSE(service.Save());
    REQUIRE(ReadFile(tmp) == before);

    // Even after a mutation, the file is still not written
    REQUIRE(service.AddGroup("Home"));
    REQUIRE_FALSE(service.Save());
    REQUIRE(ReadFile(tmp) == before);

    // A second load reports the same state
    REQUIRE_FALSE(service.Load());
    REQUIRE(service.GetFileFormatVersion() == 2);
    REQUIRE_FALSE(service.IsFileFormatSupported());

    std::filesystem::remove(tmp, ec);
}

TEST_CASE("Move favorite into another group")
{
    std::filesystem::path tmp =
        std::filesystem::temp_directory_path() / "fav_locations_move_to_group_test.json";
    std::error_code ec;
    std::filesystem::remove(tmp, ec);

    osmscout::FavoriteLocationService service(tmp.string());
    service.AddGroup("Home");
    service.AddGroup("Work");

    AddNamedFavorite(service, "Home", "A");
    AddNamedFavorite(service, "Home", "B");
    AddNamedFavorite(service, "Home", "C");
    AddNamedFavorite(service, "Work", "X");
    AddNamedFavorite(service, "Work", "Y");

    // Into the middle: the target index refers to the destination list
    REQUIRE(service.MoveFavoriteToGroup("Home", "B", "Work", 1));
    REQUIRE(FavoriteNames(service, "Home") == "A,C");
    REQUIRE(FavoriteNames(service, "Work") == "X,B,Y");

    // To the front
    REQUIRE(service.MoveFavoriteToGroup("Home", "A", "Work", 0));
    REQUIRE(FavoriteNames(service, "Home") == "C");
    REQUIRE(FavoriteNames(service, "Work") == "A,X,B,Y");

    // An index beyond the end is clamped to the last position
    REQUIRE(service.MoveFavoriteToGroup("Home", "C", "Work", 100));
    REQUIRE(service.GetFavorites("Home").empty());
    REQUIRE(FavoriteNames(service, "Work") == "A,X,B,Y,C");

    // Into an empty group
    service.AddGroup("Empty");
    REQUIRE(service.MoveFavoriteToGroup("Work", "C", "Empty", 0));
    REQUIRE(FavoriteNames(service, "Work") == "A,X,B,Y");
    REQUIRE(FavoriteNames(service, "Empty") == "C");

    // Into the group the favorite already belongs to: no-op that reports success
    REQUIRE(service.MoveFavoriteToGroup("Work", "X", "Work", 7));
    REQUIRE(FavoriteNames(service, "Work") == "A,X,B,Y");

    std::filesystem::remove(tmp, ec);
}

TEST_CASE("Move favorite into another group keeps the favorite's data")
{
    std::filesystem::path tmp =
        std::filesystem::temp_directory_path() / "fav_locations_move_to_group_data_test.json";
    std::error_code ec;
    std::filesystem::remove(tmp, ec);

    osmscout::FavoriteLocationService service(tmp.string());
    service.AddGroup("Home");
    service.AddGroup("Work");
    AddNamedFavorite(service, "Work", "X");

    osmscout::FavLocation fav;
    fav.name = "B";
    fav.lat = 51.5;
    fav.lon = 7.25;
    fav.attributes["custom"] = "value";
    REQUIRE(service.AddFavorite("Home", fav));
    AddNamedFavorite(service, "Home", "C");

    REQUIRE(service.SetStarred("Home", "B", true));
    REQUIRE(service.SetGroupColor("Home", "FF5733"));

    REQUIRE(service.MoveFavoriteToGroup("Home", "B", "Work", 1));

    // The moved favorite keeps its name, coordinates and attributes
    auto workFavs = service.GetFavorites("Work");
    REQUIRE(workFavs.size() == 2);
    REQUIRE(workFavs[1].name == "B");
    REQUIRE(workFavs[1].lat == 51.5);
    REQUIRE(workFavs[1].lon == 7.25);
    REQUIRE(workFavs[1].attributes["custom"] == "value");
    REQUIRE(service.IsStarred("Work", "B"));

    // The groups keep their own remaining favorites, their attributes and the
    // group order
    REQUIRE(FavoriteNames(service, "Home") == "C");
    REQUIRE(FavoriteNames(service, "Work") == "X,B");
    REQUIRE(service.GetGroupColor("Home") == "FF5733");
    REQUIRE(GroupNames(service) == "Home,Work");

    std::filesystem::remove(tmp, ec);
}

TEST_CASE("Move favorite into another group refuses a name collision and unknown names")
{
    std::filesystem::path tmp =
        std::filesystem::temp_directory_path() / "fav_locations_move_to_group_refusal_test.json";
    std::error_code ec;
    std::filesystem::remove(tmp, ec);

    osmscout::FavoriteLocationService service(tmp.string());
    service.AddGroup("Home");
    service.AddGroup("Work");

    AddNamedFavorite(service, "Home", "A");
    AddNamedFavorite(service, "Home", "B");
    AddNamedFavorite(service, "Home", "C");
    AddNamedFavorite(service, "Work", "B");
    AddNamedFavorite(service, "Work", "X");

    // The destination already holds a favorite of that name: nothing changes,
    // in particular the destination favorite is not replaced or renamed
    REQUIRE_FALSE(service.MoveFavoriteToGroup("Home", "B", "Work", 0));
    REQUIRE(FavoriteNames(service, "Home") == "A,B,C");
    REQUIRE(FavoriteNames(service, "Work") == "B,X");

    // Unknown source group, unknown destination group, unknown favorite
    REQUIRE_FALSE(service.MoveFavoriteToGroup("Missing", "A", "Work", 0));
    REQUIRE_FALSE(service.MoveFavoriteToGroup("Home", "A", "Missing", 0));
    REQUIRE_FALSE(service.MoveFavoriteToGroup("Home", "Missing", "Work", 0));
    REQUIRE(FavoriteNames(service, "Home") == "A,B,C");
    REQUIRE(FavoriteNames(service, "Work") == "B,X");

    std::filesystem::remove(tmp, ec);
}

TEST_CASE("Move favorite into another group persists across save and load")
{
    std::filesystem::path tmp =
        std::filesystem::temp_directory_path() / "fav_locations_move_to_group_persist_test.json";
    std::error_code ec;
    std::filesystem::remove(tmp, ec);

    {
        osmscout::FavoriteLocationService service(tmp.string());
        service.AddGroup("Home");
        service.AddGroup("Work");

        AddNamedFavorite(service, "Home", "A");
        AddNamedFavorite(service, "Home", "B");
        AddNamedFavorite(service, "Work", "X");
        AddNamedFavorite(service, "Work", "Y");

        REQUIRE(service.MoveFavoriteToGroup("Home", "B", "Work", 1));
        REQUIRE(FavoriteNames(service, "Home") == "A");
        REQUIRE(FavoriteNames(service, "Work") == "X,B,Y");
        REQUIRE(service.Save());
    }

    {
        osmscout::FavoriteLocationService service(tmp.string());
        REQUIRE(FavoriteNames(service, "Home") == "A");
        REQUIRE(FavoriteNames(service, "Work") == "X,B,Y");
    }

    std::filesystem::remove(tmp, ec);
}

// Renders the starred order as a comma separated list of group/favorite pairs,
// so that the whole order and the owning group can be asserted in one
// expression.
static std::string StarredNames(const osmscout::FavoriteLocationService &service)
{
    std::string result;

    for (const auto &entry : service.GetStarred()) {
        if (!result.empty()) {
            result += ',';
        }
        result += entry.groupName + "/" + entry.favorite.name;
    }

    return result;
}

TEST_CASE("Starred favorites are reported in their order and span groups")
{
    std::filesystem::path tmp = std::filesystem::temp_directory_path() / "fav_locations_starred_reader_test.json";
    std::error_code ec;
    std::filesystem::remove(tmp, ec);

    osmscout::FavoriteLocationService service(tmp.string());

    // An empty store has an empty starred order
    REQUIRE(service.GetStarred().empty());

    service.AddGroup("Work");
    service.AddGroup("Home");
    service.AddGroup("Sport");
    AddNamedFavorite(service, "Work", "Office");
    AddNamedFavorite(service, "Work", "Desk");
    AddNamedFavorite(service, "Home", "Home");
    AddNamedFavorite(service, "Sport", "Gym");

    // Starring appends at the end, independent of the group order
    REQUIRE(service.SetStarred("Work", "Office", true));
    REQUIRE(service.SetStarred("Home", "Home", true));
    REQUIRE(service.SetStarred("Sport", "Gym", true));

    REQUIRE(StarredNames(service) == "Work/Office,Home/Home,Sport/Gym");

    // Only starred favorites appear: "Desk" is not starred and stays out
    auto starred = service.GetStarred();
    REQUIRE(starred.size() == 3);
    REQUIRE(starred[0].groupName == "Work");
    REQUIRE(starred[0].favorite.name == "Office");
    REQUIRE(starred[1].groupName == "Home");
    REQUIRE(starred[1].favorite.name == "Home");
    REQUIRE(starred[2].groupName == "Sport");
    REQUIRE(starred[2].favorite.name == "Gym");

    // A group with a single starred favorite reports exactly that one
    service.AddGroup("Only");
    AddNamedFavorite(service, "Only", "A");
    AddNamedFavorite(service, "Only", "B");
    REQUIRE(service.SetStarred("Only", "B", true));
    REQUIRE(StarredNames(service) == "Work/Office,Home/Home,Sport/Gym,Only/B");

    std::filesystem::remove(tmp, ec);
}

TEST_CASE("Move a starred favorite within the starred order")
{
    std::filesystem::path tmp = std::filesystem::temp_directory_path() / "fav_locations_starred_move_test.json";
    std::error_code ec;
    std::filesystem::remove(tmp, ec);

    osmscout::FavoriteLocationService service(tmp.string());
    service.AddGroup("Work");
    service.AddGroup("Home");
    service.AddGroup("Sport");
    AddNamedFavorite(service, "Work", "Office");
    AddNamedFavorite(service, "Home", "Home");
    AddNamedFavorite(service, "Sport", "Gym");
    AddNamedFavorite(service, "Sport", "Plain");

    REQUIRE(service.SetStarred("Work", "Office", true));
    REQUIRE(service.SetStarred("Home", "Home", true));
    REQUIRE(service.SetStarred("Sport", "Gym", true));
    REQUIRE(StarredNames(service) == "Work/Office,Home/Home,Sport/Gym");

    // Move to the front
    REQUIRE(service.MoveStarred("Sport", "Gym", 0));
    REQUIRE(StarredNames(service) == "Sport/Gym,Work/Office,Home/Home");

    // Move to the front again, from a different group
    REQUIRE(service.MoveStarred("Home", "Home", 0));
    REQUIRE(StarredNames(service) == "Home/Home,Sport/Gym,Work/Office");

    // Index beyond the end is clamped to the last position
    REQUIRE(service.MoveStarred("Home", "Home", 100));
    REQUIRE(StarredNames(service) == "Sport/Gym,Work/Office,Home/Home");

    // Moving to the position it already occupies succeeds without changing anything
    REQUIRE(service.MoveStarred("Home", "Home", 2));
    REQUIRE(StarredNames(service) == "Sport/Gym,Work/Office,Home/Home");

    // A favorite that is not starred cannot be moved
    REQUIRE_FALSE(service.MoveStarred("Sport", "Plain", 0));
    REQUIRE(StarredNames(service) == "Sport/Gym,Work/Office,Home/Home");

    // Unknown group and unknown favorite fail without changing anything
    REQUIRE_FALSE(service.MoveStarred("Missing", "Gym", 0));
    REQUIRE_FALSE(service.MoveStarred("Sport", "Missing", 0));
    REQUIRE(StarredNames(service) == "Sport/Gym,Work/Office,Home/Home");

    // The groups, their favorite order and their membership are untouched
    REQUIRE(GroupNames(service) == "Work,Home,Sport");
    REQUIRE(FavoriteNames(service, "Work") == "Office");
    REQUIRE(FavoriteNames(service, "Home") == "Home");
    REQUIRE(FavoriteNames(service, "Sport") == "Gym,Plain");

    std::filesystem::remove(tmp, ec);
}

TEST_CASE("Starring and unstarring move a favorite in and out of the starred order")
{
    std::filesystem::path tmp = std::filesystem::temp_directory_path() / "fav_locations_starred_lifecycle_test.json";
    std::error_code ec;
    std::filesystem::remove(tmp, ec);

    osmscout::FavoriteLocationService service(tmp.string());
    service.AddGroup("Work");
    AddNamedFavorite(service, "Work", "Office");
    AddNamedFavorite(service, "Work", "Home");
    AddNamedFavorite(service, "Work", "Gym");

    // Starring appends at the end
    REQUIRE(service.SetStarred("Work", "Office", true));
    REQUIRE(service.SetStarred("Work", "Home", true));
    REQUIRE(service.SetStarred("Work", "Gym", true));
    REQUIRE(StarredNames(service) == "Work/Office,Work/Home,Work/Gym");

    // Starring again keeps the existing position
    REQUIRE(service.SetStarred("Work", "Office", true));
    REQUIRE(StarredNames(service) == "Work/Office,Work/Home,Work/Gym");

    // Unstarring removes the entry, together with the position it had
    REQUIRE(service.SetStarred("Work", "Office", false));
    REQUIRE(StarredNames(service) == "Work/Home,Work/Gym");

    // Starring again appends at the end instead of restoring the old place
    REQUIRE(service.SetStarred("Work", "Office", true));
    REQUIRE(StarredNames(service) == "Work/Home,Work/Gym,Work/Office");

    // Unstarring a favorite that is not starred changes nothing
    REQUIRE(service.SetStarred("Work", "Office", false));
    REQUIRE(StarredNames(service) == "Work/Home,Work/Gym");
    REQUIRE(service.SetStarred("Work", "Office", false));
    REQUIRE(StarredNames(service) == "Work/Home,Work/Gym");

    std::filesystem::remove(tmp, ec);
}

TEST_CASE("The starred order persists and survives a move between groups")
{
    std::filesystem::path tmp = std::filesystem::temp_directory_path() / "fav_locations_starred_persist_test.json";
    std::error_code ec;
    std::filesystem::remove(tmp, ec);

    {
        osmscout::FavoriteLocationService service(tmp.string());
        service.AddGroup("Home");
        service.AddGroup("Work");
        AddNamedFavorite(service, "Home", "A");
        AddNamedFavorite(service, "Home", "B");
        AddNamedFavorite(service, "Work", "Office");

        REQUIRE(service.SetStarred("Home", "A", true));
        REQUIRE(service.SetStarred("Home", "B", true));
        REQUIRE(service.SetStarred("Work", "Office", true));
        REQUIRE(StarredNames(service) == "Home/A,Home/B,Work/Office");

        // Arrange a different order, then move a favorite to another group: its
        // place in the starred order must not change.
        REQUIRE(service.MoveStarred("Work", "Office", 0));
        REQUIRE(StarredNames(service) == "Work/Office,Home/A,Home/B");

        REQUIRE(service.MoveFavoriteToGroup("Home", "B", "Work", 0));
        REQUIRE(StarredNames(service) == "Work/Office,Home/A,Work/B");
        REQUIRE(service.IsStarred("Work", "B"));

        REQUIRE(service.Save());
    }

    {
        osmscout::FavoriteLocationService service(tmp.string());
        REQUIRE(StarredNames(service) == "Work/Office,Home/A,Work/B");
    }

    std::filesystem::remove(tmp, ec);
}

TEST_CASE("A hand-edited starred order is read in a defined order and rewritten")
{
    std::filesystem::path tmp = std::filesystem::temp_directory_path() / "fav_locations_starred_tolerance_test.json";
    std::error_code ec;
    std::filesystem::remove(tmp, ec);

    // Duplicate and unparsable positions, and a starred favorite without one
    WriteFile(tmp,
              R"JSON({
  "groups": {
    "Work": {
      "name": "Work",
      "attributes": {},
      "favorites": [
        { "name": "Office", "lat": 1.0, "lon": 2.0,
          "attributes": { "starred": "true", "starredPosition": "100" } },
        { "name": "Desk", "lat": 1.0, "lon": 2.0,
          "attributes": { "starred": "true", "starredPosition": "100" } }
      ]
    },
    "Home": {
      "name": "Home",
      "attributes": {},
      "favorites": [
        { "name": "Flat", "lat": 1.0, "lon": 2.0,
          "attributes": { "starred": "true" } },
        { "name": "Garden", "lat": 1.0, "lon": 2.0,
          "attributes": { "starred": "true", "starredPosition": "abc" } }
      ]
    }
  }
}
)JSON");

    {
        osmscout::FavoriteLocationService service(tmp.string());

        // Known positions first (ties by group order and favorite name), then
        // the entries without a usable position, also by group order and name
        REQUIRE(StarredNames(service) == "Work/Desk,Work/Office,Home/Flat,Home/Garden");

        // The following save writes store-owned values, so the order is now
        // explicit and a hand-edited file converges on a self-consistent one
        REQUIRE(service.Save());
        REQUIRE(StarredNames(service) == "Work/Desk,Work/Office,Home/Flat,Home/Garden");
    }

    std::string content = ReadFile(tmp);
    REQUIRE(content.find("starredPosition") != std::string::npos);
    REQUIRE(content.find("abc") == std::string::npos);

    {
        osmscout::FavoriteLocationService service(tmp.string());
        REQUIRE(StarredNames(service) == "Work/Desk,Work/Office,Home/Flat,Home/Garden");
    }

    std::filesystem::remove(tmp, ec);
}
