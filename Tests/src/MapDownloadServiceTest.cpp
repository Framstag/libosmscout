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
#include <osmscoutclient/HttpClient.h>
#include <osmscoutclient/MapDownloadService.h>
#include <osmscoutclient/MapManager.h>
#include <osmscoutclient/Settings.h>

#include <osmscoutclient/json/json.hpp>

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <set>
#include <thread>

namespace {

class MockHttpClient : public osmscout::HttpClient
{
public:
  std::string fetchResponse;
  bool downloadSuccess{true};
  std::set<std::string> downloadedUrls;

  std::string Fetch([[maybe_unused]] const std::string &url) override
  {
    return fetchResponse;
  }

  bool Download(const std::string &url,
                const std::filesystem::path &dest,
                [[maybe_unused]] osmscout::ProgressCallback progress) override
  {
    downloadedUrls.insert(url);
    if (!downloadSuccess) {
      return false;
    }
    std::ofstream out(dest);
    out << "mock data";
    out.close();
    return true;
  }
};

class TestSettingsStorage : public osmscout::SettingsStorage
{
private:
  std::map<std::string, std::string> values;

public:
  void SetValue(const std::string &key, double d) override { values[key] = std::to_string(d); }
  void SetValue(const std::string &key, uint32_t i) override { values[key] = std::to_string(i); }
  void SetValue(const std::string &key, const std::string &str) override { values[key] = str; }
  void SetValue(const std::string &key, bool b) override { values[key] = b ? "true" : "false"; }
  void SetValue(const std::string &key, std::vector<char> bytes) override { values[key] = std::string(bytes.begin(), bytes.end()); }
  double GetDouble(const std::string &key, double defaultValue) override
  {
    auto it = values.find(key);
    return it != values.end() ? std::stod(it->second) : defaultValue;
  }
  uint32_t GetUInt(const std::string &key, uint32_t defaultValue) override
  {
    auto it = values.find(key);
    return it != values.end() ? static_cast<uint32_t>(std::stoul(it->second)) : defaultValue;
  }
  std::string GetString(const std::string &key, const std::string &defaultValue) override
  {
    auto it = values.find(key);
    return it != values.end() ? it->second : defaultValue;
  }
  bool GetBool(const std::string &key, bool defaultValue) override
  {
    auto it = values.find(key);
    return it != values.end() ? it->second == "true" : defaultValue;
  }
  std::vector<char> GetBytes(const std::string &key) override
  {
    auto it = values.find(key);
    return it != values.end() ? std::vector<char>(it->second.begin(), it->second.end()) : std::vector<char>();
  }
  std::vector<std::string> Keys(const std::string &prefix) override
  {
    std::vector<std::string> result;
    for (const auto &[key, _] : values) {
      if (key.find(prefix) == 0) result.push_back(key);
    }
    return result;
  }
};

struct TestFixture
{
  MockHttpClient httpClient; // Must be before service (destroyed after)
  std::shared_ptr<TestSettingsStorage> storage;
  std::shared_ptr<osmscout::Settings> settings;
  std::shared_ptr<osmscout::MapManager> mapManager;
  std::shared_ptr<osmscout::MapDownloadService> service;

  TestFixture()
    : httpClient()
    , storage(std::make_shared<TestSettingsStorage>())
    , settings(std::make_shared<osmscout::Settings>(storage, 96.0, "metrics"))
    , mapManager(std::make_shared<osmscout::MapManager>(
          std::vector<std::filesystem::path>{}))
    , service(std::make_shared<osmscout::MapDownloadService>(mapManager, settings))
  {
  }

  ~TestFixture()
  {
    // Give worker thread time to settle before destruction
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    // Destroy service first (joins worker thread) while httpClient is still alive
    service.reset();
  }
};

} // namespace

// --------------------------------------------------------------------------
// AvailableMapEntry JSON parsing tests
// --------------------------------------------------------------------------

TEST_CASE("AvailableMapEntry::FromJsonArray parses map entries")
{
  osmscout::MapProvider provider("test", "https://example.com", "https://example.com/list.php?from=%1&to=%2");

  nlohmann::json json = nlohmann::json::parse(R"([
    {
      "version": 10,
      "timestamp": 1480801927,
      "name": "Czech Republic",
      "directory": "europe/czech-republic-10-20161203",
      "size": 622036876,
      "map": "europe/czech-republic"
    },
    {
      "dir": "europe",
      "name": "Europe"
    }
  ])");

  auto entries = osmscout::AvailableMapEntry::FromJsonArray(json, provider);

  REQUIRE(entries.size() == 2);
  REQUIRE_FALSE(entries[0].IsDirectory());
  REQUIRE(entries[0].GetName() == "Czech Republic");
  REQUIRE(entries[0].GetSize() == 622036876);
  REQUIRE(entries[0].GetVersion() == 10);
  REQUIRE(entries[0].GetServerDirectory() == "europe/czech-republic-10-20161203");
  REQUIRE(entries[0].GetPath().size() == 1);
  REQUIRE(entries[0].GetPath()[0] == "europe");
  REQUIRE(entries[1].IsDirectory());
  REQUIRE(entries[1].GetName() == "Europe");
}

TEST_CASE("AvailableMapEntry::FromJsonArray handles empty array")
{
  osmscout::MapProvider provider("test", "https://example.com", "");
  nlohmann::json json = nlohmann::json::parse(R"([])");
  auto entries = osmscout::AvailableMapEntry::FromJsonArray(json, provider);
  REQUIRE(entries.empty());
}

TEST_CASE("AvailableMapEntry::FromJsonArray handles non-array input")
{
  osmscout::MapProvider provider("test", "https://example.com", "");
  nlohmann::json json = nlohmann::json::parse(R"({"error": "not found"})");
  auto entries = osmscout::AvailableMapEntry::FromJsonArray(json, provider);
  REQUIRE(entries.empty());
}

TEST_CASE("AvailableMapEntry::FromJsonArray skips invalid entries")
{
  osmscout::MapProvider provider("test", "https://example.com", "");
  nlohmann::json json = nlohmann::json::parse(R"([
    {"name": "valid map", "map": "europe/germany", "size": 100},
    {"name": "invalid - no map or dir field"},
    {"name": "valid dir", "dir": "asia"}
  ])");
  auto entries = osmscout::AvailableMapEntry::FromJsonArray(json, provider);
  REQUIRE(entries.size() == 2);
}

// --------------------------------------------------------------------------
// MapDownloadService tests
// --------------------------------------------------------------------------

TEST_CASE("MapDownloadService::FetchMapList returns parsed entries")
{
  TestFixture fx;
  fx.httpClient.fetchResponse = R"([
    {"name": "Germany", "map": "europe/germany", "size": 500000000, "version": 10, "timestamp": 1480801927, "directory": "europe/germany-10"},
    {"dir": "europe", "name": "Europe"}
  ])";

  osmscout::MapProvider provider("test", "https://example.com",
                                  "https://example.com/list.php?from=%1&to=%2");

  auto future = fx.service->FetchMapList(provider, fx.httpClient);
  auto entries = future.StdFuture().get();

  REQUIRE(entries.size() == 2);
  REQUIRE(entries[0].GetName() == "Germany");
  REQUIRE_FALSE(entries[0].IsDirectory());
  REQUIRE(entries[1].GetName() == "Europe");
  REQUIRE(entries[1].IsDirectory());

  // Small delay to let worker thread settle before fixture destruction
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

TEST_CASE("MapDownloadService::FetchMapList returns empty on HTTP error")
{
  TestFixture fx;
  fx.httpClient.fetchResponse = "";

  osmscout::MapProvider provider("test", "https://example.com",
                                  "https://example.com/list.php?from=%1&to=%2");

  auto future = fx.service->FetchMapList(provider, fx.httpClient);
  auto entries = future.StdFuture().get();
  REQUIRE(entries.empty());
}

TEST_CASE("MapDownloadService::DownloadMap creates files and metadata")
{
  TestFixture fx;
  fx.httpClient.downloadSuccess = true;

  std::error_code ec;
  std::filesystem::path tmpDir = std::filesystem::temp_directory_path(ec) / "osmscout-test-download";
  std::filesystem::remove_all(tmpDir, ec);
  std::filesystem::create_directories(tmpDir, ec);

  osmscout::MapProvider provider("test", "https://example.com",
                                  "https://example.com/list.php?from=%1&to=%2");

  osmscout::AvailableMapEntry entry("TestMap", {"europe", "test"}, "A test map",
    provider, 1000, "test-dir",
    osmscout::Timestamp(std::chrono::seconds(1480801927)), 10);

  auto future = fx.service->DownloadMap(entry, tmpDir, fx.httpClient);
  REQUIRE(future.StdFuture().get());

  std::filesystem::path metadataPath = tmpDir / osmscout::MapDirectory::FileMetadata;
  REQUIRE(std::filesystem::exists(metadataPath));

  for (const auto &f : osmscout::MapDirectory::MandatoryFiles()) {
    REQUIRE(std::filesystem::exists(tmpDir / f));
  }
  REQUIRE(fx.httpClient.downloadedUrls.size() > 0);

  std::filesystem::remove_all(tmpDir, ec);
}

TEST_CASE("MapDownloadService::DownloadMap returns false on HTTP error")
{
  auto storage = std::make_shared<TestSettingsStorage>();
  auto settings = std::make_shared<osmscout::Settings>(storage, 96.0, "metrics");
  auto mapManager = std::make_shared<osmscout::MapManager>(
      std::vector<std::filesystem::path>{});

  // Use raw pointer and manual delete to control destruction order precisely
  auto *httpClient = new MockHttpClient();
  httpClient->downloadSuccess = false;

  auto *service = new osmscout::MapDownloadService(mapManager, settings);

  std::error_code ec;
  std::filesystem::path tmpDir = std::filesystem::temp_directory_path(ec) / "osmscout-test-fail";
  std::filesystem::remove_all(tmpDir, ec);
  std::filesystem::create_directories(tmpDir, ec);

  osmscout::MapProvider provider("test", "https://example.com", "");
  osmscout::AvailableMapEntry entry("FailMap", {}, "", provider, 100, "dir",
                                      osmscout::Timestamp(), 1);

  auto future = service->DownloadMap(entry, tmpDir, *httpClient);
  bool result = future.StdFuture().get();

  REQUIRE_FALSE(result);

  // Destroy in correct order: service first (joins thread), then httpClient
  delete service;
  delete httpClient;

  std::filesystem::remove_all(tmpDir, ec);
}

TEST_CASE("MapDownloadService::MapFiles returns mandatory and optional files")
{
  auto files = osmscout::MapDownloadService::MapFiles();
  REQUIRE_FALSE(files.empty());

  auto mandatory = osmscout::MapDirectory::MandatoryFiles();
  auto optional = osmscout::MapDirectory::OptionalFiles();

  for (const auto &f : mandatory) {
    REQUIRE(std::find(files.begin(), files.end(), f) != files.end());
  }
  for (const auto &f : optional) {
    REQUIRE(std::find(files.begin(), files.end(), f) != files.end());
  }
}

TEST_CASE("MapDownloadService::PrepareMapDirectory creates directory and metadata")
{
  std::error_code ec;
  std::filesystem::path tmpDir = std::filesystem::temp_directory_path(ec) / "osmscout-test-prepare";
  std::filesystem::remove_all(tmpDir, ec);

  osmscout::MapProvider provider("test", "https://example.com", "");
  osmscout::AvailableMapEntry entry("PrepareMap", {"europe", "test"}, "",
                                      provider, 1000, "test-dir",
                                      osmscout::Timestamp(std::chrono::seconds(1480801927)), 10);

  REQUIRE(osmscout::MapDownloadService::PrepareMapDirectory(entry, tmpDir));
  REQUIRE(std::filesystem::is_directory(tmpDir, ec));

  std::filesystem::path metadataPath = tmpDir / osmscout::MapDirectory::FileMetadata;
  REQUIRE(std::filesystem::exists(metadataPath, ec));

  // Metadata should contain the map name
  std::ifstream in(metadataPath);
  std::string content((std::istreambuf_iterator<char>(in)),
                       std::istreambuf_iterator<char>());
  REQUIRE(content.find("PrepareMap") != std::string::npos);

  std::filesystem::remove_all(tmpDir, ec);
}

TEST_CASE("MapDownloadService::RegisterMapDirectory adds lookup directory")
{
  TestFixture fx;

  std::error_code ec;
  std::filesystem::path tmpDir = std::filesystem::temp_directory_path(ec) / "osmscout-test-register";
  std::filesystem::remove_all(tmpDir, ec);
  std::filesystem::create_directories(tmpDir, ec);

  REQUIRE(osmscout::MapDownloadService::RegisterMapDirectory(tmpDir, fx.mapManager));

  auto dirs = fx.mapManager->GetLookupDirectories();
  REQUIRE(std::find(dirs.begin(), dirs.end(), tmpDir) != dirs.end());

  std::filesystem::remove_all(tmpDir, ec);
}

// --------------------------------------------------------------------------
// MapManager lookup directory tests
// --------------------------------------------------------------------------

TEST_CASE("MapManager::RemoveLookupDirectory triggers database rescan")
{
  std::error_code ec;
  std::filesystem::path tmpDir = std::filesystem::temp_directory_path(ec) / "osmscout-test-remove-lookup";
  std::filesystem::remove_all(tmpDir, ec);
  std::filesystem::create_directories(tmpDir, ec);

  // Create a valid map directory (all mandatory files present)
  for (const auto &f : osmscout::MapDirectory::MandatoryFiles()) {
    std::ofstream out(tmpDir / f);
    out.close();
  }

  auto mapManager = std::make_shared<osmscout::MapManager>(
      std::vector<std::filesystem::path>{});

  // Record databaseListChanged emissions
  std::mutex signalMutex;
  std::vector<std::vector<std::filesystem::path>> emissions;
  osmscout::Slot<std::vector<std::filesystem::path>> listChangedSlot(
      [&](const std::vector<std::filesystem::path> &dirs) {
        std::unique_lock<std::mutex> lock(signalMutex);
        emissions.push_back(dirs);
      });
  mapManager->databaseListChanged.Connect(listChangedSlot);

  // Add lookup dir: maps in it must become visible
  mapManager->AddLookupDirectory(tmpDir);

  auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (std::chrono::steady_clock::now() < deadline) {
    if (!mapManager->GetDatabaseDirectories().empty()) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  auto dirs = mapManager->GetDatabaseDirectories();
  REQUIRE(dirs.size() == 1);
  REQUIRE(std::filesystem::weakly_canonical(dirs[0].GetDir()) ==
          std::filesystem::weakly_canonical(tmpDir));

  // Remove lookup dir: maps in it must disappear and databaseListChanged
  // must be emitted without the removed directory
  mapManager->RemoveLookupDirectory(tmpDir);

  deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (std::chrono::steady_clock::now() < deadline) {
    if (mapManager->GetDatabaseDirectories().empty()) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  REQUIRE(mapManager->GetDatabaseDirectories().empty());
  REQUIRE(mapManager->GetLookupDirectories().empty());

  std::unique_lock<std::mutex> lock(signalMutex);
  REQUIRE_FALSE(emissions.empty());
  REQUIRE(emissions.back().empty());

  std::filesystem::remove_all(tmpDir, ec);
}
