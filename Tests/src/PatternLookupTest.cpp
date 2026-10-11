/*
  This source is part of the libosmscout library
  Copyright (C) 2026  Tim Teulings

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

#include <catch2/catch_test_macros.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <list>
#include <string>

#include <osmscoutmap/PatternLookup.h>

namespace {

  std::string GetEnv(const char* name,
                     const std::string& fallback)
  {
    const char * value=std::getenv(name);

    return value!=nullptr ? std::string(value) : fallback;
  }

  /**
   * Writable directory of this test. Created on first use.
   */
  std::filesystem::path GetTempDir()
  {
    std::filesystem::path dir=std::filesystem::path(GetEnv("TESTS_TMP_DIR","..")) / "pattern-lookup";

    std::filesystem::create_directories(dir);

    return dir;
  }

  /**
   * Directory holding no pattern image at all.
   */
  std::filesystem::path GetEmptyDir()
  {
    std::filesystem::path dir=GetTempDir() / "empty";

    std::filesystem::create_directories(dir);

    return dir;
  }

  /**
   * Directory holding an image for the given pattern name.
   */
  std::filesystem::path GetDirWithPattern(const std::string& patternName)
  {
    std::filesystem::path dir=GetTempDir() / ("with-" + patternName);

    std::filesystem::create_directories(dir);

    std::ofstream image(dir / (patternName + ".png"));

    image << "not a real image";

    return dir;
  }
}

TEST_CASE("Pattern lookup classifies a missing pattern image source", "[PatternLookup]")
{
  std::list<std::string> patternPaths;
  std::string            filename="untouched";

  auto                   status=osmscout::PatternLookup::Resolve(patternPaths,
                                                                 "natural_scrub",
                                                                 ".png",
                                                                 filename);

  REQUIRE(status==osmscout::PatternLookup::Status::NoSourceConfigured);
  REQUIRE(filename.empty());

  std::string report=osmscout::PatternLookup::Describe(patternPaths,
                                                       "natural_scrub",
                                                       ".png",
                                                       status);

  REQUIRE(report.find("natural_scrub")!=std::string::npos);
  REQUIRE(report.find("No pattern image source is configured")!=std::string::npos);
}

TEST_CASE("Pattern lookup reports the directories it searched", "[PatternLookup]")
{
  std::filesystem::path  emptyDir=GetEmptyDir();
  std::list<std::string> patternPaths{emptyDir.string()};
  std::string            filename="untouched";

  auto                   status=osmscout::PatternLookup::Resolve(patternPaths,
                                                                 "natural_scrub",
                                                                 ".png",
                                                                 filename);

  REQUIRE(status==osmscout::PatternLookup::Status::NotFound);
  REQUIRE(filename.empty());

  std::string report=osmscout::PatternLookup::Describe(patternPaths,
                                                       "natural_scrub",
                                                       ".png",
                                                       status);

  REQUIRE(report.find("natural_scrub.png")!=std::string::npos);
  REQUIRE(report.find(emptyDir.string())!=std::string::npos);
  REQUIRE(report.find("No pattern image source is configured")==std::string::npos);
}

TEST_CASE("Pattern lookup finds an image in a later directory", "[PatternLookup]")
{
  std::filesystem::path  emptyDir=GetEmptyDir();
  std::filesystem::path  patternDir=GetDirWithPattern("natural_scrub");
  std::list<std::string> patternPaths{emptyDir.string(),patternDir.string()};
  std::string            filename;

  auto                   status=osmscout::PatternLookup::Resolve(patternPaths,
                                                                 "natural_scrub",
                                                                 ".png",
                                                                 filename);

  REQUIRE(status==osmscout::PatternLookup::Status::Found);
  REQUIRE(std::filesystem::path(filename)==patternDir / "natural_scrub.png");
  REQUIRE(osmscout::PatternLookup::Describe(patternPaths,
                                            "natural_scrub",
                                            ".png",
                                            status).empty());
}

TEST_CASE("Pattern lookup accepts a directory without a trailing separator", "[PatternLookup]")
{
  std::filesystem::path  patternDir=GetDirWithPattern("landuse_cemetery");
  std::string            directory=patternDir.string();
  std::list<std::string> patternPaths{directory};
  std::string            filename;

  REQUIRE(directory.back()!='/');

  auto status=osmscout::PatternLookup::Resolve(patternPaths,
                                               "landuse_cemetery",
                                               ".png",
                                               filename);

  REQUIRE(status==osmscout::PatternLookup::Status::Found);
  REQUIRE(std::filesystem::path(filename)==patternDir / "landuse_cemetery.png");
}

TEST_CASE("Pattern lookup does not resolve another file extension", "[PatternLookup]")
{
  std::filesystem::path  patternDir=GetDirWithPattern("leisure_garden");
  std::list<std::string> patternPaths{patternDir.string()};
  std::string            filename;

  auto                   status=osmscout::PatternLookup::Resolve(patternPaths,
                                                                 "leisure_garden",
                                                                 ".svg",
                                                                 filename);

  REQUIRE(status==osmscout::PatternLookup::Status::NotFound);
}
