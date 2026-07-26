
#include <filesystem>

#include <osmscout/db/Database.h>
#include <osmscout/TypeConfig.h>
#include <osmscoutclient/MapProvider.h>
#include <osmscoutclient/json/json.hpp>

#include <catch2/catch_test_macros.hpp>

static std::string GetTestDatabaseDirectory()
{
  char* testsTopDirEnv=::getenv("TESTS_TOP_DIR");

  if (testsTopDirEnv==nullptr) {
    throw osmscout::UninitializedException("Expected environment variable 'TESTS_TOP_DIR' not set");
  }

  std::string testsTopDir=testsTopDirEnv;

  if (testsTopDir.empty()) {
    throw osmscout::UninitializedException("Environment variable 'TESTS_TOP_DIR' is empty");
  }

  if (!osmscout::IsDirectory(testsTopDir)) {
    throw osmscout::UninitializedException("Environment variable 'TESTS_TOP_DIR' does not point to directory");
  }

  return std::filesystem::path(testsTopDir).append("data").append("testregion").string();
}

TEST_CASE("TypeConfig::GetFileFormatVersion(dir) throws exception on missing db") {
  CHECK_THROWS_AS(osmscout::TypeConfig::GetDatabaseFileFormatVersion("does_not_exist"),osmscout::IOException);
}

TEST_CASE("Database::GetFileFormatVersion(dir) throws exception on missing db") {
  CHECK_THROWS_AS(osmscout::Database::GetDatabaseFileFormatVersion("does_not_exist"),osmscout::IOException);
}

TEST_CASE("TypeConfig::GetFileFormatVersion(dir) returns current version for test database") {
  REQUIRE(osmscout::TypeConfig::GetDatabaseFileFormatVersion(GetTestDatabaseDirectory()) == osmscout::FILE_FORMAT_VERSION);
}

TEST_CASE("Database::GetFileFormatVersion(dir) returns current version for test database") {
  REQUIRE(osmscout::Database::GetDatabaseFileFormatVersion(GetTestDatabaseDirectory()) == osmscout::FILE_FORMAT_VERSION);
}

TEST_CASE("Database::GetLibraryFileFormatVersion() returns current version") {
  REQUIRE(osmscout::Database::GetLibraryFileFormatVersion() == osmscout::FILE_FORMAT_VERSION);
}

TEST_CASE("MapProvider list URI substitutes FILE_FORMAT_VERSION") {
  osmscout::MapProvider provider("test",
                                 "https://example.com",
                                 "https://example.com/list.php?fromVersion=%1&toVersion=%2&locale=%3");

  std::string uri = provider.getListUri(osmscout::FILE_FORMAT_VERSION,
                                         osmscout::FILE_FORMAT_VERSION,
                                         "en");

  std::string expected = "https://example.com/list.php?fromVersion="
                         + std::to_string(osmscout::FILE_FORMAT_VERSION)
                         + "&toVersion="
                         + std::to_string(osmscout::FILE_FORMAT_VERSION)
                         + "&locale=en";

  REQUIRE(uri == expected);
}

TEST_CASE("MapProvider list URI version range matches library version") {
  osmscout::MapProvider provider("test",
                                 "https://example.com",
                                 "https://example.com/list.php?from=%1&to=%2");

  // The download request MUST ask the server for maps matching the
  // library's FILE_FORMAT_VERSION. Both bounds use the same version
  // because MIN_FORMAT_VERSION == MAX_FORMAT_VERSION == FILE_FORMAT_VERSION.
  std::string uri = provider.getListUri(osmscout::FILE_FORMAT_VERSION,
                                         osmscout::FILE_FORMAT_VERSION);

  REQUIRE(uri.find("from=" + std::to_string(osmscout::FILE_FORMAT_VERSION)) != std::string::npos);
  REQUIRE(uri.find("to=" + std::to_string(osmscout::FILE_FORMAT_VERSION)) != std::string::npos);
}

TEST_CASE("MapProvider list URI locale substitution") {
  osmscout::MapProvider provider("test",
                                 "https://example.com",
                                 "https://example.com/list.php?locale=%3");

  std::string uri = provider.getListUri(osmscout::FILE_FORMAT_VERSION,
                                         osmscout::FILE_FORMAT_VERSION,
                                         "cs");

  REQUIRE(uri == "https://example.com/list.php?locale=cs");
}

TEST_CASE("MapProvider list URI with empty locale defaults to en") {
  osmscout::MapProvider provider("test",
                                 "https://example.com",
                                 "https://example.com/list.php?locale=%3");

  std::string uri = provider.getListUri(osmscout::FILE_FORMAT_VERSION,
                                         osmscout::FILE_FORMAT_VERSION);

  REQUIRE(uri == "https://example.com/list.php?locale=en");
}

TEST_CASE("MapProvider fromJson constructs valid provider") {
  nlohmann::json obj = {
    {"name", "karry.cz"},
    {"uri", "https://osmscout.karry.cz"},
    {"listUri", "https://osmscout.karry.cz/latest.php?fromVersion=%1&toVersion=%2&locale=%3"}
  };

  osmscout::MapProvider provider = osmscout::MapProvider::fromJson(obj);

  REQUIRE(provider.isValid());
  REQUIRE(provider.getName() == "karry.cz");
  REQUIRE(provider.getUri() == "https://osmscout.karry.cz");

  std::string uri = provider.getListUri(osmscout::FILE_FORMAT_VERSION,
                                         osmscout::FILE_FORMAT_VERSION,
                                         "en");
  REQUIRE(uri.find("fromVersion=" + std::to_string(osmscout::FILE_FORMAT_VERSION)) != std::string::npos);
  REQUIRE(uri.find("toVersion=" + std::to_string(osmscout::FILE_FORMAT_VERSION)) != std::string::npos);
}

TEST_CASE("MapProvider fromJson returns invalid for non-object") {
  nlohmann::json obj = nlohmann::json::array();
  osmscout::MapProvider provider = osmscout::MapProvider::fromJson(obj);
  REQUIRE_FALSE(provider.isValid());
}

TEST_CASE("MapProvider fromJson returns invalid for missing fields") {
  nlohmann::json obj = {{"name", "incomplete"}};
  osmscout::MapProvider provider = osmscout::MapProvider::fromJson(obj);
  REQUIRE_FALSE(provider.isValid());
}
