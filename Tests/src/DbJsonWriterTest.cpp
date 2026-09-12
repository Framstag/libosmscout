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

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

#include <osmscout/io/Crc32.h>

#include <DbJsonWriter.h>

namespace {

  std::string GetEnv(const char* name,
                     const std::string& defaultValue)
  {
    const char * value=getenv(name);

    if (value==nullptr) {
      return defaultValue;
    }

    return std::string(value);
  }

  std::filesystem::path GetTempDir()
  {
    return std::filesystem::path(GetEnv("TESTS_TMP_DIR",
                                        std::filesystem::temp_directory_path().string()));
  }

  void WriteFile(const std::filesystem::path& path,
                 const std::string& content)
  {
    std::ofstream file(path);

    file << content;
  }

  std::string ReadFile(const std::filesystem::path& path)
  {
    std::ifstream file(path);
    std::string   content((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());

    return content;
  }
}

TEST_CASE("ComputeFileCrc32 computes the standard CRC-32 check value", "[DbJsonWriter]")
{
  std::filesystem::path tmp=GetTempDir() / "DbJsonWriterTest_crc32.txt";

  WriteFile(tmp,"123456789");

  uint32_t crc32=0;

  REQUIRE(osmscout::ComputeFileCrc32(tmp.string(),crc32));
  REQUIRE(crc32==0xCBF43926U);

  std::filesystem::remove(tmp);
}

TEST_CASE("Crc32 matches the standard check value and chains like zlib", "[DbJsonWriter]")
{
  const char * data="123456789";

  REQUIRE(osmscout::Crc32(data,9)==0xCBF43926U);

  // chaining two buffers equals hashing them in one go
  uint32_t chained=osmscout::Crc32(data+5,
                                   4,
                                   osmscout::Crc32(data,5));

  REQUIRE(chained==0xCBF43926U);

  // empty input leaves the initial value unchanged
  REQUIRE(osmscout::Crc32(data,0)==0U);
}

TEST_CASE("ComputeFileCrc32 fails for missing file", "[DbJsonWriter]")
{
  uint32_t crc32=0;

  REQUIRE_FALSE(osmscout::ComputeFileCrc32((GetTempDir()/"DbJsonWriterTest_missing.txt").string(),
                                           crc32));
}

TEST_CASE("BuildDbJsonInventory lists existing files and skips missing ones", "[DbJsonWriter]")
{
  std::filesystem::path tmp=GetTempDir() / "DbJsonWriterTest_inventory";

  std::filesystem::create_directories(tmp);

  WriteFile(tmp/"a.dat","hello");
  WriteFile(tmp/"b.dat","world");

  std::vector<std::string>               fileNames={"a.dat","b.dat","missing.dat"};
  std::vector<osmscout::DbJsonFileEntry> files;

  REQUIRE(osmscout::BuildDbJsonInventory(tmp.string(),
                                         fileNames,
                                         files));
  REQUIRE(files.size()==2);
  REQUIRE(files[0].name=="a.dat");
  REQUIRE(files[0].size==5);
  REQUIRE(files[1].name=="b.dat");
  REQUIRE(files[1].size==5);

  std::filesystem::remove_all(tmp);
}

TEST_CASE("WriteDbJson writes db.json atomically", "[DbJsonWriter]")
{
  std::filesystem::path tmp=GetTempDir() / "DbJsonWriterTest_write";

  std::filesystem::create_directories(tmp);

  osmscout::DbJsonData data;

  data.generatedAt="2026-09-06T15:43:46Z";
  data.typeConfigVersion=27;
  data.sourceUrl="https://download.geofabrik.de/europe/germany/berlin-latest.osm.pbf";
  data.sourceMd5="146f59bf3b42630f89572160de6260bb";
  data.toolVersion="1.1.1";
  data.startStep=0;
  data.endStep=40;
  data.durationSeconds=123.4;
  data.boundingBox.Set(osmscout::GeoCoord(52.34,13.09),
                       osmscout::GeoCoord(52.68,13.76));
  data.files.push_back(osmscout::DbJsonFileEntry {"map.lib",12345678,0xA1B2C3D4U});
  data.typeCount=1527;

  REQUIRE(osmscout::WriteDbJson(tmp.string(),
                                data));

  REQUIRE(std::filesystem::exists(tmp/"db.json"));
  REQUIRE_FALSE(std::filesystem::exists(tmp/"db.json.tmp"));

  std::string content=ReadFile(tmp/"db.json");

  REQUIRE(content.find("\"schema\": 1")!=std::string::npos);
  REQUIRE(content.find("\"typeConfigVersion\": 27")!=std::string::npos);
  REQUIRE(content.find("\"generatedAt\": \"2026-09-06T15:43:46Z\"")!=std::string::npos);
  REQUIRE(content.find("\"url\": \"https://download.geofabrik.de/europe/germany/berlin-latest.osm.pbf\"")!=
          std::string::npos);
  REQUIRE(content.find("\"md5\": \"146f59bf3b42630f89572160de6260bb\"")!=std::string::npos);
  REQUIRE(content.find("\"version\": \"1.1.1\"")!=std::string::npos);
  REQUIRE(content.find("\"startStep\": 0")!=std::string::npos);
  REQUIRE(content.find("\"endStep\": 40")!=std::string::npos);
  REQUIRE(content.find("\"durationSeconds\": 123.4")!=std::string::npos);
  REQUIRE(content.find("\"minLon\": 13.09")!=std::string::npos);
  REQUIRE(content.find("\"minLat\": 52.34")!=std::string::npos);
  REQUIRE(content.find("\"maxLon\": 13.76")!=std::string::npos);
  REQUIRE(content.find("\"maxLat\": 52.68")!=std::string::npos);
  REQUIRE(content.find("\"map.lib\"")!=std::string::npos);
  REQUIRE(content.find("\"size\": 12345678")!=std::string::npos);
  REQUIRE(content.find("\"crc32\": 2712847316")!=std::string::npos);
  REQUIRE(content.find("\"types\": 1527")!=std::string::npos);

  std::filesystem::remove_all(tmp);
}

TEST_CASE("WriteDbJson omits source section when no source facts given", "[DbJsonWriter]")
{
  std::filesystem::path tmp=GetTempDir() / "DbJsonWriterTest_nosource";

  std::filesystem::create_directories(tmp);

  osmscout::DbJsonData data;

  data.generatedAt="2026-09-06T15:43:46Z";
  data.typeConfigVersion=27;
  data.toolVersion="1.1.1";
  data.startStep=0;
  data.endStep=40;
  data.durationSeconds=1.0;

  REQUIRE(osmscout::WriteDbJson(tmp.string(),
                                data));

  std::string content=ReadFile(tmp/"db.json");

  REQUIRE(content.find("\"source\"")==std::string::npos);

  std::filesystem::remove_all(tmp);
}
