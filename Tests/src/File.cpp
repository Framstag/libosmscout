#include <filesystem>
#include <iostream>
#include <utility>

#include <osmscout/io/File.h>

#include <TestMain.h>

TEST_CASE("Current Directory")
{
  std::filesystem::path path=std::filesystem::current_path();

  std::cout << "Current path: '" << path << std::endl;

  REQUIRE(std::filesystem::is_directory(path));
}

TEST_CASE("GetDirectory")
{
#if defined(__WIN32__) || defined(WIN32)
  std::wcout << L"Windows path mode using '" << std::filesystem::path::preferred_separator << "'" << std::endl;
  std::vector<std::pair<std::string,std::string>> testData = {{"c:\\directory\\file.txt", "c:\\directory\\"},
                                                              {"c:\\directory\\", "c:\\directory\\"},
                                                              {"c:\\directory", "c:\\"},
                                                              {"c:\\", "c:\\"},
                                                              {"", ""}};
#else
  std::cout << "Unix path mode using '" << std::filesystem::path::preferred_separator << "'" << std::endl;
  std::vector<std::pair<std::string,std::string>> testData = {{"/directory/file.txt", "/directory/"},
                                                              {"/directory/", "/directory/"},
                                                              {"/directory", "/"},
                                                              {"/", "/"},
                                                              {"", ""}};
#endif

  for (const auto& test : testData) {
    std::string directory=osmscout::GetDirectory(test.first);
    REQUIRE(test.second == directory);
  }
}
