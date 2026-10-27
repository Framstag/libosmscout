/*
  TypeConfigDescriptionTest - a test program for libosmscout
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
#include <memory>
#include <string>
#include <utility>

#include <osmscout/TypeConfig.h>

/**
 * The descriptions a type carries and how a caller resolves them.
 *
 * A caller asks for a language. If the type carries no description for that language, the English
 * description answers; if the type carries none at all, the result is empty, so that a caller can
 * tell "no description" from a description and show the technical type name instead.
 *
 * A type configuration that carries no descriptions loads without error, so a database generated
 * before the descriptions existed keeps working.
 */
namespace {

  std::string GetEnv(const char* name,
                     const std::string& fallback)
  {
    const char *value=std::getenv(name);

    return value!=nullptr ? std::string(value) : fallback;
  }

  /** Writable directory of this test. Created on first use. */
  std::filesystem::path GetTempDir()
  {
    std::filesystem::path dir=std::filesystem::path(GetEnv("TESTS_TMP_DIR","..")) / "type-config-description";

    std::filesystem::create_directories(dir);

    return dir;
  }

  /** A type of the given kind with the given descriptions. */
  osmscout::TypeInfoRef MakeType(const std::string& name,
                                 const std::pair<std::string,std::string>& descriptions)
  {
    auto type=std::make_shared<osmscout::TypeInfo>(name);

    type->CanBeNode(true);

    if (!descriptions.first.empty()) {
      type->AddDescription("en",descriptions.first);
    }

    if (!descriptions.second.empty()) {
      type->AddDescription("de",descriptions.second);
    }

    return type;
  }

  /** Write a type definition file with the given type definitions. */
  std::filesystem::path WriteTypeDefinitionFile(const std::filesystem::path& file,
                                                const std::string& types)
  {
    std::ofstream stream(file);

    stream << "OST\n\nTYPES\n" << types << "\nEND\n";
    stream.close();

    return file;
  }

  /**
   * The type configuration of a definition file with two types: one described in German and
   * English, one without any description.
   */
  const std::string describedTypes="  TYPE test_bank\n"
                                   "    = NODE (\"amenity\"==\"bank\")\n"
                                   "      DESC\n"
                                   "        en: \"bank\"\n"
                                   "        de: \"Bank\"\n"
                                   "\n"
                                   "  TYPE test_bench\n"
                                   "    = NODE (\"amenity\"==\"bench\")\n";

  /** The same two types, without any description. */
  const std::string undescribedTypes="  TYPE test_bank\n"
                                     "    = NODE (\"amenity\"==\"bank\")\n"
                                     "\n"
                                     "  TYPE test_bench\n"
                                     "    = NODE (\"amenity\"==\"bench\")\n";
}

TEST_CASE("A description is resolved by language, or not at all", "[TypeDescription]")
{
  auto config=std::make_shared<osmscout::TypeConfig>();

  config->RegisterType(MakeType("test_bank",{"bank","Bank"}));
  config->RegisterType(MakeType("test_bench",{"bench",""}));
  config->RegisterType(MakeType("test_atm",{"",""}));

  auto both=config->GetTypeInfo("test_bank");
  auto english=config->GetTypeInfo("test_bench");
  auto none=config->GetTypeInfo("test_atm");

  REQUIRE(both!=nullptr);
  REQUIRE(english!=nullptr);
  REQUIRE(none!=nullptr);

  SECTION("The requested language answers")
  {
    CHECK(both->GetDescription("de")=="Bank");
    CHECK(both->GetLocalizedDescription("de")=="Bank");
    CHECK(both->GetLocalizedDescription("en")=="bank");
  }

  SECTION("English answers a language the configuration does not carry")
  {
    CHECK(english->GetDescription("de").empty());
    CHECK(english->GetLocalizedDescription("de")=="bench");
    CHECK(english->GetLocalizedDescription("fr")=="bench");
  }

  SECTION("A type without a description reports none for every language")
  {
    CHECK(none->GetDescription("de").empty());
    CHECK(none->GetLocalizedDescription("de").empty());
    CHECK(none->GetLocalizedDescription("en").empty());
    CHECK(none->GetLocalizedDescription("fr").empty());
  }

  SECTION("A caller can obtain every description of a type")
  {
    CHECK(both->GetDescriptions().size()==2);
    CHECK(both->GetDescriptions().at("en")=="bank");
    CHECK(both->GetDescriptions().at("de")=="Bank");
    CHECK(none->GetDescriptions().empty());
  }

  SECTION("An unknown type is reported instead of raising an error")
  {
    CHECK(config->GetTypeInfo("test_does_not_exist")==nullptr);
  }
}

TEST_CASE("Descriptions survive a store and a load of the type configuration", "[TypeDescription]")
{
  std::filesystem::path directory=GetTempDir() / "stored";
  std::filesystem::path ostFile=WriteTypeDefinitionFile(GetTempDir() / "described.ost",describedTypes);

  std::filesystem::create_directories(directory);

  auto written=std::make_shared<osmscout::TypeConfig>();

  REQUIRE(written->LoadFromOSTFile(ostFile.string()));

  auto writtenBank=written->GetTypeInfo("test_bank");
  auto writtenBench=written->GetTypeInfo("test_bench");

  REQUIRE(writtenBank!=nullptr);
  REQUIRE(writtenBench!=nullptr);

  REQUIRE(written->StoreToDataFile(directory.string()));

  auto read=std::make_shared<osmscout::TypeConfig>();

  REQUIRE(read->LoadFromDataFile(directory.string()));

  auto readBank=read->GetTypeInfo("test_bank");
  auto readBench=read->GetTypeInfo("test_bench");

  REQUIRE(readBank!=nullptr);
  REQUIRE(readBench!=nullptr);

  CHECK(readBank->GetDescription("en")=="bank");
  CHECK(readBank->GetDescription("de")=="Bank");
  CHECK(readBank->GetLocalizedDescription("de")=="Bank");
  CHECK(readBank->GetDescriptions().size()==2);

  CHECK(readBench->GetDescription("en").empty());
  CHECK(readBench->GetDescription("de").empty());
  CHECK(readBench->GetLocalizedDescription("en").empty());
}

TEST_CASE("A type configuration without descriptions loads and reports none", "[TypeDescription]")
{
  std::filesystem::path directory=GetTempDir() / "undescribed";
  std::filesystem::path ostFile=WriteTypeDefinitionFile(GetTempDir() / "undescribed.ost",undescribedTypes);

  std::filesystem::create_directories(directory);

  auto written=std::make_shared<osmscout::TypeConfig>();

  REQUIRE(written->LoadFromOSTFile(ostFile.string()));
  REQUIRE(written->StoreToDataFile(directory.string()));

  auto read=std::make_shared<osmscout::TypeConfig>();

  REQUIRE(read->LoadFromDataFile(directory.string()));

  for (const auto& type : read->GetTypes()) {
    INFO("type: " << type->GetName());

    CHECK(type->GetDescriptions().empty());
    CHECK(type->GetDescription("de").empty());
    CHECK(type->GetLocalizedDescription("de").empty());
  }

  CHECK(read->GetTypeInfo("test_bank")!=nullptr);
  CHECK(read->GetTypeInfo("test_bench")!=nullptr);
}

TEST_CASE("The shipped type configuration keeps its descriptions through a database", "[TypeDescription]")
{
  // End to end over the shipped files: the type definition file that an import reads is stored
  // into the type configuration file of a database and read back from it by a client.
  const char *topDir=std::getenv("TESTS_TOP_DIR");

  if (topDir==nullptr || *topDir=='\0') {
    SKIP("TESTS_TOP_DIR is not set, cannot locate the type definition files");
  }

  std::filesystem::path directory=GetTempDir() / "shipped";

  std::filesystem::create_directories(directory);

  auto written=std::make_shared<osmscout::TypeConfig>();

  REQUIRE(written->LoadFromOSTFile(std::string(topDir)+"/../stylesheets/map.ost"));
  REQUIRE(written->StoreToDataFile(directory.string()));

  auto read=std::make_shared<osmscout::TypeConfig>();

  REQUIRE(read->LoadFromDataFile(directory.string()));

  // A type the shipped configuration describes
  auto bank=read->GetTypeInfo("amenity_bank");

  REQUIRE(bank!=nullptr);

  CHECK(bank->GetDescription("en")=="bank");
  CHECK(bank->GetDescription("de")=="Bank");
  CHECK(bank->GetLocalizedDescription("de")=="Bank");
  CHECK(bank->GetLocalizedDescription("fr")=="bank");

  // A parked type describes itself inside its PARKED-NEW-TYPE block, and the block is not part of
  // what a database carries: the type is not in the configuration at all.
  CHECK(read->GetTypeInfo("shop_wool")==nullptr);

  // A type the shipped configuration deliberately leaves without a description
  auto undocumented=read->GetTypeInfo("amenity_gambling");

  REQUIRE(undocumented!=nullptr);

  CHECK(undocumented->GetDescriptions().empty());
  CHECK(undocumented->GetLocalizedDescription("de").empty());
  CHECK(undocumented->GetLocalizedDescription("en").empty());
}

TEST_CASE("A type registered at runtime carries no description", "[TypeDescription]")
{
  auto config=std::make_shared<osmscout::TypeConfig>();

  // The same shape an application registers for an overlay of its own.
  auto type=std::make_shared<osmscout::TypeInfo>("_test_poi");

  type->SetInternal()
  .CanBeWay(true)
  .CanBeArea(true)
  .CanBeNode(true);

  config->RegisterType(type);

  auto registered=config->GetTypeInfo("_test_poi");

  REQUIRE(registered!=nullptr);

  CHECK(registered->GetDescriptions().empty());
  CHECK(registered->GetLocalizedDescription("de").empty());
  CHECK(registered->GetLocalizedDescription("en").empty());

  SECTION("A stored configuration does not carry a runtime type")
  {
    std::filesystem::path directory=GetTempDir() / "runtime";
    std::filesystem::path ostFile=WriteTypeDefinitionFile(GetTempDir() / "runtime.ost",undescribedTypes);

    std::filesystem::create_directories(directory);

    REQUIRE(config->LoadFromOSTFile(ostFile.string()));
    REQUIRE(config->StoreToDataFile(directory.string()));

    auto read=std::make_shared<osmscout::TypeConfig>();

    REQUIRE(read->LoadFromDataFile(directory.string()));

    CHECK(read->GetTypeInfo("_test_poi")==nullptr);
  }
}
