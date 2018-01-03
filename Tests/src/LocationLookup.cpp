#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include <osmscout/import/Import.h>

#include <osmscout/PreprocessOLT.h>

#include <osmscout/LocationService.h>

osmscout::DatabaseRef        database;
osmscout::LocationServiceRef locationService;

/*
 * A 1:1 match for the city name => match
 */
TEST_CASE("Search for city 'Dortmund' (match)") {
  osmscout::LocationStringSearchParameter parameter("Dortmund");
  osmscout::LocationSearchResult          result;

  bool success=locationService->SearchForLocationByString(parameter,
                                                          result);

  REQUIRE(success);
  REQUIRE_FALSE(result.limitReached);
  REQUIRE(result.results.size()==1);
  REQUIRE(result.results.front().adminRegion->name=="Dortmund");
  REQUIRE(result.results.front().adminRegionMatchQuality==osmscout::LocationSearchResult::match);
}

/*
 * Substring of actual city name => candidate
 */
TEST_CASE("Search for city 'Dortm' (candidate)") {
  osmscout::LocationStringSearchParameter parameter("Dortm");
  osmscout::LocationSearchResult          result;

  bool success=locationService->SearchForLocationByString(parameter,
                                                          result);

  REQUIRE(success);
  REQUIRE_FALSE(result.limitReached);
  REQUIRE(result.results.size()==1);
  REQUIRE(result.results.front().adminRegion->name=="Dortmund");
  REQUIRE(result.results.front().adminRegionMatchQuality==osmscout::LocationSearchResult::candidate);
}

/*
 * 1:1 match of a alias of the city => match
 */
TEST_CASE("Search for city 'Brechten' (alias of Dortmund)") {
  osmscout::LocationStringSearchParameter parameter("Brechten");
  osmscout::LocationSearchResult          result;

  bool success=locationService->SearchForLocationByString(parameter,
                                                          result);

  REQUIRE(success);
  REQUIRE_FALSE(result.limitReached);
  REQUIRE(result.results.size()==1);
  REQUIRE(result.results.front().adminRegion->name=="Dortmund");
  REQUIRE(result.results.front().adminRegionMatchQuality==osmscout::LocationSearchResult::match);
}

/*
 * Substring of alias of the city => candiate
 */
TEST_CASE("Search for city 'Brecht' (alias of Dortmund)") {
  osmscout::LocationStringSearchParameter parameter("Brecht");
  osmscout::LocationSearchResult          result;

  bool success=locationService->SearchForLocationByString(parameter,
                                                          result);

  REQUIRE(success);
  REQUIRE_FALSE(result.limitReached);
  REQUIRE(result.results.size()==1);
  REQUIRE(result.results.front().adminRegion->name=="Dortmund");
  REQUIRE(result.results.front().adminRegionMatchQuality==osmscout::LocationSearchResult::candidate);
}

class PreprocessorFactory : public osmscout::PreprocessorFactory
{
public:
  std::unique_ptr<osmscout::Preprocessor> GetProcessor(const std::string& /*filename*/,
                                                       osmscout::PreprocessorCallback& callback) const override
  {
    return std::unique_ptr<osmscout::Preprocessor>(new osmscout::test::PreprocessOLT(callback));
  }
};

int main(int argc, char* argv[])
{
  std::cout << "Global setup..." << std::endl;

  osmscout::ImportParameter importParameter;
  osmscout::ConsoleProgress progress;
  std::list<std::string>    mapfiles;

  try {
    std::locale::global(std::locale(""));
  }
  catch (const std::runtime_error& e) {
    progress.Error("Cannot set locale: \""+std::string(e.what())+"\"");
    progress.Error("Note that (near) future version of the Importer require a working locale environment!");
  }

  char*testsTopDirEnv=getenv("TESTS_TOP_DIR");

  if (testsTopDirEnv==nullptr) {
    std::cerr << "Expected environment variable 'TESTS_TOP_DIR' not set" << std::endl;
    // CMake-based tests would fail, if we do not exit here
    return 1;
  }

  std::string testsTopDir=testsTopDirEnv;

  if (testsTopDir.empty()) {
    std::cerr << "Environment variable 'TESTS_TOP_DIR' is empty" << std::endl;
    return 77;
  }

  if (!osmscout::IsDirectory(testsTopDir)) {
    std::cerr << "Environment variable 'TESTS_TOP_DIR' does not point to directory" << std::endl;
    return 77;
  }

  mapfiles.emplace_back(osmscout::AppendFileToDir(testsTopDir,"LocationTest.olt"));

  importParameter.SetTypefile(osmscout::AppendFileToDir(testsTopDir,"../stylesheets/map.ost"));
  importParameter.SetMapfiles(mapfiles);
  importParameter.SetPreprocessorFactory(std::make_shared<PreprocessorFactory>());

  try {
    osmscout::Importer importer(importParameter);

    bool result=importer.Import(progress);

    if (result) {
      progress.Info("Import OK!");
    }
    else {
      progress.Error("Import failed!");
      return 1;
    }
  }
  catch (osmscout::IOException& e) {
    progress.Error("Import failed: "+e.GetDescription());
    return 1;
  }

  osmscout::DatabaseParameter dbParameter;

  database=std::make_shared<osmscout::Database>(dbParameter);

  if (!database->Open(".")) {
    std::cerr << "Cannot open database" << std::endl;
    return 77;
  }

  locationService=std::make_shared<osmscout::LocationService>(database);

  std::cout << "Testing..." << std::endl;

  int result = Catch::Session().run(argc,argv);

  std::cout << "Testing done." << std::endl;

  std::cout << "tearing down..." << std::endl;

  locationService.reset();
  database->Close();
  database.reset();

  std::cout << "Tearing down done." << std::endl;

  return result;
}
