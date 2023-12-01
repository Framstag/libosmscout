#include <TestMain.h>

#include <filesystem>
#include <list>

namespace fs = std::filesystem;

static const std::list<fs::path> projects{
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

/**
 * General expected order
 *
 * osmscout.private
 * osmscout.system
 * osmscout.async
 * osmscout.log
 * osmscout.cli
 * osmscout.util
 * osmscout.projection
 * osmscout.io
 * osmscout.db
 * osmscout.feature
 * osmscout.ost
 * osmscout.elevation
 * osmscout.location
 * osmscout.poi
 * osmscout.routing
 * osmscout.navigation
 *
 */

static const std::set<std::string> allowedDependencies{
    "osmscout => osmscout.lib",
    "osmscout => osmscout.system",
    "osmscout => osmscout.log",
    "osmscout => osmscout.util",
    "osmscout => osmscout.io",
    "osmscout => osmscout.feature",
    "osmscout => osmscout.ost",
    "osmscout.system => osmscout.lib",
    "osmscout.system => osmscout.private",
    "osmscout.async => osmscout.lib",
    "osmscout.async => osmscout.private",
    "osmscout.async => osmscout.log",
    "osmscout.log => osmscout.lib",
    "osmscout.log => osmscout.system",
    "osmscout.log => osmscout.util", // Fix this -> Move database to package
    "osmscout.cli => osmscout.system",
    "osmscout.cli => osmscout.lib",
    "osmscout.cli => osmscout.util", // Fix this -> Move String.h into package
    "osmscout.cli => osmscout", // Fix this -> Move GeoCoord into package
    "osmscout.util => osmscout.lib",
    "osmscout.util => osmscout.private",
    "osmscout.util => osmscout.system",
    "osmscout.util => osmscout.log", // Fix this? Which parts of util should use a logger?
    "osmscout.util => osmscout.projection", // Fix this
    "osmscout.util => osmscout", // Fix this
    "osmscout.projection => osmscout.lib",
    "osmscout.projection => osmscout.system",
    "osmscout.projection => osmscout.util",
    "osmscout.projection => osmscout", // Fix this
    "osmscout.io => osmscout.lib",
    "osmscout.io => osmscout.private",
    "osmscout.io => osmscout.system",
    "osmscout.io => osmscout.log",
    "osmscout.io => osmscout.util",
    "osmscout.io => osmscout", // Fix this
    "osmscout.db => osmscout.lib",
    "osmscout.db => osmscout.system",
    "osmscout.db => osmscout.log",
    "osmscout.db => osmscout.util",
    "osmscout.db => osmscout.io",
    "osmscout.db => osmscout.elevation", // Fix this
    "osmscout.db => osmscout.location", // Fix this
    "osmscout.db => osmscout.routing", // Fix this
    "osmscout.db => osmscout", // Fix this
    "osmscout.feature => osmscout.util",
    "osmscout.feature => osmscout", // Fix this
    "osmscout.ost => osmscout.system",
    "osmscout.ost => osmscout.util",
    "osmscout.ost => osmscout.io", // Fix this?
    "osmscout.ost => osmscout.feature",
    "osmscout.ost => osmscout", // Fix this
    "osmscout.elevation => osmscout.async",
    "osmscout.elevation => osmscout.lib",
    "osmscout.elevation => osmscout.system",
    "osmscout.elevation => osmscout.log",
    "osmscout.elevation => osmscout.util",
    "osmscout.elevation => osmscout.feature",
    "osmscout.elevation => osmscout", // Fix this
    "osmscout.location => osmscout.system",
    "osmscout.location => osmscout.async",
    "osmscout.location => osmscout.log",
    "osmscout.location => osmscout.util",
    "osmscout.location => osmscout.db",
    "osmscout.location => osmscout.feature",
    "osmscout.location => osmscout", // Fix this
    "osmscout.poi => osmscout", // Fix this
    "osmscout.poi => osmscout.log",
    "osmscout.poi => osmscout.db",
    "osmscout.poi => osmscout.util",
    "osmscout.routing => osmscout.lib",
    "osmscout.routing => osmscout.system",
    "osmscout.routing => osmscout.async",
    "osmscout.routing => osmscout.log",
    "osmscout.routing => osmscout.util",
    "osmscout.routing => osmscout.io",
    "osmscout.routing => osmscout.db",
    "osmscout.routing => osmscout.feature",
    "osmscout.routing => osmscout.location",
    "osmscout.routing => osmscout", // Fix this
    "osmscout.navigation => osmscout.system",
    "osmscout.navigation => osmscout.log",
    "osmscout.navigation => osmscout.util",
    "osmscout.navigation => osmscout.feature",
    "osmscout.navigation => osmscout.location",
    "osmscout.navigation => osmscout.routing",
    "osmscout.navigation => osmscout", // Fix this

    "osmscoutclient => osmscoutclient.json",
    "osmscoutclient => osmscoutclient.private",
    "osmscoutclient => osmscout",
    "osmscoutclient => osmscout.async",
    "osmscoutclient => osmscout.db",
    "osmscoutclient => osmscout.io",
    "osmscoutclient => osmscout.routing",
    "osmscoutclient => osmscout.util",
    "osmscoutclient => osmscout.location",
    "osmscoutclient => osmscout.log",
    "osmscoutclient => osmscoutmap",

    "osmscoutclientqt => osmscout.system",
    "osmscoutclientqt => osmscout.async",
    "osmscoutclientqt => osmscout.log",
    "osmscoutclientqt => osmscout.util",
    "osmscoutclientqt => osmscout.projection",
    "osmscoutclientqt => osmscout.db",
    "osmscoutclientqt => osmscout.feature",
    "osmscoutclientqt => osmscout.elevation",
    "osmscoutclientqt => osmscout.location",
    "osmscoutclientqt => osmscout.poi",
    "osmscoutclientqt => osmscout.routing",
    "osmscoutclientqt => osmscout.navigation",
    "osmscoutclientqt => osmscout",
    "osmscoutclientqt => osmscoutmap",
    "osmscoutclientqt => osmscoutmapqt",
    "osmscoutclientqt => osmscoutclient",
    "osmscoutclientqt => osmscoutclient.json",

    "osmscoutgpx => osmscout.system",
    "osmscoutgpx => osmscout.async",
    "osmscoutgpx => osmscout.log",
    "osmscoutgpx => osmscout.util",
    "osmscoutgpx => osmscout.io",
    "osmscoutgpx => osmscout", // Fix this

    "osmscoutimport => osmscout.system",
    "osmscoutimport => osmscout.async",
    "osmscoutimport => osmscout.util",
    "osmscoutimport => osmscout.projection",
    "osmscoutimport => osmscout.io",
    "osmscoutimport => osmscout.db",
    "osmscoutimport => osmscout.feature",
    "osmscoutimport => osmscout.routing",
    "osmscoutimport => osmscout", // Fix it
    "osmscoutimport => osmscoutimport.private",
    "osmscoutimport => osmscoutimport.pbf",

    "osmscoutmap.oss => osmscout.system",
    "osmscoutmap.oss => osmscout.log",
    "osmscoutmap.oss => osmscout.util",
    "osmscoutmap.oss => osmscout.io",
    "osmscoutmap.oss => osmscout", // Fix this -> Helper and TypeService
    "osmscoutmap.oss => osmscoutmap", // Fix this -> use of StyleConfig
    "osmscoutmap => osmscout.system",
    "osmscoutmap => osmscout.async",
    "osmscoutmap => osmscout.log",
    "osmscoutmap => osmscout.util",
    "osmscoutmap => osmscout.projection",
    "osmscoutmap => osmscout.feature",
    "osmscoutmap => osmscout.io",
    "osmscoutmap => osmscout.db", // Fix this? -> MapService makes heavy use of Database
    "osmscoutmap => osmscout.elevation", // Fix this?
    "osmscoutmap => osmscoutmap.oss",
    "osmscoutmap => osmscout",

    "osmscoutmapagg => osmscout.system",
    "osmscoutmapagg => osmscout.util",
    "osmscoutmapagg => osmscoutmap",

    "osmscoutmapcairo => osmscout.system",
    "osmscoutmapcairo => osmscout.log",
    "osmscoutmapcairo => osmscout.util",
    "osmscoutmapcairo => osmscoutmap",

    "osmscoutmapdirectx => osmscout.system",
    "osmscoutmapdirectx => osmscout.util",
    "osmscoutmapdirectx => osmscoutmap",

    "osmscoutmapgdi => osmscout.util",
    "osmscoutmapgdi => osmscout.projection",
    "osmscoutmapgdi => osmscout.io",
    "osmscoutmapgdi => osmscoutmap",

    "osmscoutmapopengl => osmscout.system",
    "osmscoutmapopengl => osmscout.log",
    "osmscoutmapopengl => osmscout.util",
    "osmscoutmapopengl => osmscout.projection",
    "osmscoutmapopengl => osmscout.io",
    "osmscoutmapopengl => osmscoutmap",

    "osmscoutmapqt => osmscout.system",
    "osmscoutmapqt => osmscout.log",
    "osmscoutmapqt => osmscout.util",
    "osmscoutmapqt => osmscout.io",
    "osmscoutmapqt => osmscoutmap",

    "osmscoutmapsvg => osmscout.system",
    "osmscoutmapsvg => osmscout.util",
    "osmscoutmapsvg => osmscout.io",
    "osmscoutmapsvg => osmscoutmap",

    "osmscout-test.olt => osmscout.util",
    "osmscout-test => osmscout.system",
    "osmscout-test => osmscout.util",
    "osmscout-test => osmscout.io",
    "osmscout-test => osmscoutimport",
};

std::list<fs::path> GetFiles(const fs::path& directory, const std::string& extension)
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

std::list<fs::path> GetAllFiles(const fs::path& sourceRoot) {
    std::list<fs::path> allFiles;
    fs::path            root=sourceRoot;

    CHECK(exists(root));
    CHECK(is_directory(root));

    std::cout << "SOURCE_ROOT=" << root << std::endl;

    for (const auto& project : projects) {
        std::list<fs::path> files;
        fs::path projectPath=root / project;

        files= GetFiles(projectPath / "include", ".h");
        allFiles.insert(allFiles.end(),files.begin(),files.end());

        files= GetFiles(projectPath / "src", ".cpp");
        allFiles.insert(allFiles.end(),files.begin(),files.end());
    }

    return allFiles;
}

std::list<std::string> GetIncludes(const fs::path& filename)
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

std::string FilenameToPackage(const std::string& filename) {
    auto startOfPackage=filename.find("/osmscout");

    if (startOfPackage==std::string::npos) {
        return "";
    }

    auto endOfPackage=filename.rfind('/');

    if (endOfPackage==std::string::npos) {
        return "";
    }

    auto package=filename.substr(startOfPackage + 1, endOfPackage - startOfPackage - 1);

    for (char & character : package) {
        if (character == '/') {
            character='.';
        }
    }

    return package;
}

std::string IncludeToPackage(const std::string& filename) {
    auto startOfPackage=filename.find("osmscout");

    if (startOfPackage!=0) {
        return "";
    }

    auto endOfPackage=filename.rfind('/');

    if (endOfPackage==std::string::npos) {
        return "";
    }

    auto package=filename.substr(startOfPackage, endOfPackage - startOfPackage);

    for (char & character : package) {
        if (character == '/') {
            character='.';
        }
    }

    return package;
}

TEST_CASE("Check format of includes")
{
    const char* sourceRoot = getenv("SOURCE_ROOT");

    CHECK(sourceRoot!= nullptr);

    fs::path            root=sourceRoot;

    CHECK(exists(root));
    CHECK(is_directory(root));

    std::list<fs::path> allFiles=GetAllFiles(root);

    std::regex includeExpression("#include *[\"][a-zA-Z0-9/.]+[\"]");

    size_t violationCount=0;

    for (const auto& file : allFiles) {
        auto includes= GetIncludes(file);

        for (const auto& include : includes)  {
            if (std::regex_search(include, includeExpression)) {
                std::cerr << "File '" << file.generic_string() << "' using wrong include format for include '" << include << "'!" << '\n';
                violationCount++;
            }
        }
    }

    REQUIRE(violationCount==0);
}

TEST_CASE("Check include dependencies")
{
    const char* sourceRoot = getenv("SOURCE_ROOT");

    CHECK(sourceRoot!= nullptr);

    fs::path root=sourceRoot;

    CHECK(exists(root));
    CHECK(is_directory(root));

    std::list<fs::path> allFiles=GetAllFiles(root);

    std::regex            includeExpression("#include *[\"<]([a-zA-Z0-9/.]+)[\">]");
    std::set<std::string> foundDependencies;
    size_t                violationCount=0;

    for (const auto& file : allFiles) {
        auto filePackage= FilenameToPackage(file.generic_string());
        auto includes= GetIncludes(file.generic_string());

        for (const auto& incudeStatement : includes)  {
            std::smatch result;
            if (std::regex_search(incudeStatement, result, includeExpression)) {
                std::string include=result[1];
                std::string includePackage=IncludeToPackage(include);

                if (filePackage.empty()) {
                    // Not scope of test
                    continue;
                }

                if (includePackage.empty()) {
                    // Every package is allowed to have dependencies on non-osmscout headers
                    continue;
                }

                if (includePackage==filePackage) {
                    // Every package is allowed to use functionality from its own package
                    continue;
                }

                std::string dependencyDescription=filePackage + " => " + includePackage;

                foundDependencies.insert(dependencyDescription);

                if (allowedDependencies.find(dependencyDescription)==allowedDependencies.end()) {
                    std::cerr << "File '" << file.generic_string() << " in package '" << filePackage << "' has forbidden dependency to package '" << includePackage << "'" << '\n';
                    violationCount++;
                }
            }
        }
    }

    for (const auto& expectedDependency : allowedDependencies) {
        if (foundDependencies.find(expectedDependency) == foundDependencies.end()) {
            std::cerr << "We expected dependency '" << expectedDependency << "' but did not find it in the code, please check" << '\n';
            violationCount++;
        }
    }

    REQUIRE(violationCount==0);
}
