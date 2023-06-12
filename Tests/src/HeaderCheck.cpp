#include <TestMain.h>

#include <filesystem>
#include <list>

namespace fs = std::filesystem;

std::list<fs::path> getFiles(const fs::path& directory, const std::string& extension)
{
    std::list<fs::path> files;

    if (exists(directory)) {
        for (auto const &entry: fs::recursive_directory_iterator(directory)) {
            if (entry.exists() && entry.is_regular_file() && entry.path().extension() == extension) {
                files.push_back(entry.path());
            }
        }
    }

    return files;
}

std::list<std::string> getIncludes(const fs::path& filename)
{
    std::list<std::string> includes;
    std::regex include("#include *[<\"][a-zA-Z0-9/.]+[>\"]");

    std::string line;
    std::ifstream file (filename);
    if (file.is_open()) {
        while (getline(file, line)) {
            std::smatch match;
            if (std::regex_search(line,match,include))  {
                includes.push_back(match[0]);
            }
        }
        file.close();
    }

    return includes;
}

TEST_CASE("Check format of includes")
{
    std::list<fs::path> projects{
        "libosmscout/",
        "libosmscout-client/",
        "libosmscout-client-qt/",
        "libosmscout-gpx/",
        "libosmscout-import/",
        "libosmscout-map/",
        "libosmscout-map-agg/",
        "libosmscout-map-cairo/",
        "libosmscout-map-directx/",
        "libosmscout-map-gdi/",
        "libosmscout-map-opengl/",
        "libosmscout-map-qt/",
        "libosmscout-map-svg/",
        "libosmscout-test/",
        "OSMScout2",
        "OSMScoutOpenGL",
        "PublicTransportMap",
        "StyleEditor",
        "Tests"};

    const char* sourceRoot = getenv("SOURCE_ROOT");

    CHECK(sourceRoot!= nullptr);


    std::list<fs::path> allFiles;
    fs::path            root=sourceRoot;

    CHECK(exists(root));
    CHECK(is_directory(root));

    std::cout << "SOURCE_ROOT=" << root << std::endl;

    for (const auto& project : projects) {
        std::list<fs::path> files;
        fs::path projectPath=root / project;

        files=getFiles(projectPath / "include",".h");
        allFiles.insert(allFiles.end(),files.begin(),files.end());

        files=getFiles(projectPath / "src",".cpp");
        allFiles.insert(allFiles.end(),files.begin(),files.end());
    }

    std::regex includeString("#include *[\"][a-zA-Z0-9/.]+[\"]");

    bool foundErrors=false;

    for (const auto& file : allFiles) {
        auto includes=getIncludes(file);

        for (const auto& include : includes)  {
            if (std::regex_search(include,includeString)) {
                std::cerr << "File '" << file.generic_string() << "' using wrong include format for include '" << include << "'!" << '\n';
                foundErrors=true;
            }
        }
    }

    REQUIRE_FALSE(foundErrors);
}

