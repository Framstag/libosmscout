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
#include <string>
#include <vector>

#include <osmscout/TypeConfig.h>
#include <osmscoutmap/StyleConfig.h>

namespace {

  std::string GetEnv(const char* name,
                     const std::string& fallback)
  {
    const char * value=std::getenv(name);

    return value!=nullptr ? std::string(value) : fallback;
  }

  osmscout::TypeConfigRef LoadTypeConfig()
  {
    std::filesystem::path ost=std::filesystem::path(GetEnv("TESTS_TOP_DIR","..")) / ".." / "stylesheets" / "map.ost";

    auto                  typeConfig=std::make_shared<osmscout::TypeConfig>();

    if (!typeConfig->LoadFromOSTFile(ost.string())) {
      FAIL("Cannot load type config file '" << ost.string() << "'");
    }

    return typeConfig;
  }
}

TEST_CASE("StyleConfig enumerates symbols of a stylesheet", "[StyleConfig]")
{
  std::filesystem::path oss=std::filesystem::path(GetEnv("TESTS_TOP_DIR",
                                                         "..")) / ".." / "stylesheets" / "motorways.oss";

  osmscout::StyleConfig styleConfig(LoadTypeConfig());

  REQUIRE(styleConfig.Load(oss.string()));

  std::vector<std::string> names=styleConfig.GetSymbolNames();

  REQUIRE(names.size()==3);
  REQUIRE(names==std::vector<std::string>({"highway_services",
                                           "place_city",
                                           "place_town"}));
}

TEST_CASE("StyleConfig returns empty symbol list for stylesheet without symbols", "[StyleConfig]")
{
  std::filesystem::path tmp=std::filesystem::path(GetEnv("TESTS_TMP_DIR",
                                                         std::filesystem::temp_directory_path().string()));
  std::filesystem::path oss=tmp / "StyleConfigSymbolsEmpty.oss";

  {
    std::ofstream file(oss);

    file << "OSS" << std::endl;
    file << "END" << std::endl;
  }

  osmscout::StyleConfig styleConfig(LoadTypeConfig());

  REQUIRE(styleConfig.Load(oss.string()));
  REQUIRE(styleConfig.GetSymbolNames().empty());
}
