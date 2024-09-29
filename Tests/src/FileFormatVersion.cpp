
#include <filesystem>

#include <osmscout/db/Database.h>
#include <osmscout/TypeConfig.h>

#include <TestMain.h>

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
