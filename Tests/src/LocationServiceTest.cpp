#include <iostream>

#include <osmscoutimport/Import.h>
#include <osmscoutimport/ImportProgress.h>

#include <osmscout-test/PreprocessOLT.h>

#include <osmscout/LocationService.h>

#include <TestRunner.h>

osmscout::DatabaseRef        database;
osmscout::LocationServiceRef locationService;

class PreprocessorFactory : public osmscout::PreprocessorFactory
{
public:
  std::unique_ptr<osmscout::Preprocessor> GetProcessor(const std::string& /*filename*/,
                                                       osmscout::PreprocessorCallback& callback) const override
  {
    return std::unique_ptr<osmscout::Preprocessor>(new osmscout::test::PreprocessOLT(callback));
  }
};

std::string MatchQualityToString(osmscout::LocationSearchResult::MatchQuality quality)
{
  switch (quality) {
  case osmscout::LocationSearchResult::none:
    return "none";
  case osmscout::LocationSearchResult::match:
    return "match";
  case osmscout::LocationSearchResult::candidate:
    return "candidate";
  }

  return "???";
}

void DumpSeachResult(const osmscout::LocationSearchResult& result)
{
  std::cout << result.results.size() << " " << (result.results.size() >1 ? "results" : "result") << ":" << std::endl;

  size_t currentResult=1;
  for (const auto& entry : result.results) {
    std::cout << "#" << currentResult << std::endl;

    if (entry.adminRegionMatchQuality!=osmscout::LocationSearchResult::none) {
      std::cout << "Admin region: " << entry.adminRegion->name << " " << MatchQualityToString(entry.adminRegionMatchQuality) << std::endl;
    }

    if (entry.postalAreaMatchQuality!=osmscout::LocationSearchResult::none) {
      std::cout << "PostalArea: " << entry.postalArea->name << " " << MatchQualityToString(entry.postalAreaMatchQuality) << std::endl;
    }

    currentResult++;
  }
}

int main(int argc, char* argv[])
{
  std::cout << "Global setup..." << std::endl;

  osmscout::ImportParameter importParameter;
  osmscout::ImportProgress  progress;
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

  char*testsTmpDirEnv=getenv("TESTS_TMP_DIR");
  std::string testsTmpDir;
  if (testsTmpDirEnv==nullptr) {
    std::cerr << "Environment variable 'TESTS_TMP_DIR' not set, using current dir" << std::endl;
    testsTmpDir=".";
  } else {
    testsTmpDir=testsTmpDirEnv;
  }

  if (testsTmpDir.empty()) {
    std::cerr << "Environment variable 'TESTS_TMP_DIR' is empty" << std::endl;
    return 77;
  }

  if (!osmscout::IsDirectory(testsTmpDir)) {
    std::cerr << "Environment variable 'TESTS_TMP_DIR' does not point to directory" << std::endl;
    return 77;
  }

  mapfiles.emplace_back(osmscout::AppendFileToDir(testsTopDir,"LocationTest.olt"));

  importParameter.SetTypefile(osmscout::AppendFileToDir(testsTopDir,"../stylesheets/map.ost"));
  importParameter.SetMapfiles(mapfiles);
  importParameter.SetDestinationDirectory(testsTmpDir);
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

  if (!database->Open(testsTmpDir)) {
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
