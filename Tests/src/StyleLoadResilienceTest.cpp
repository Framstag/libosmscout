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

#include <osmscoutclient/DBInstance.h>

#include <osmscout/db/Database.h>
#include <osmscout/projection/MercatorProjection.h>
#include <osmscoutmap/MapPainterNoOp.h>
#include <osmscoutmap/StyleConfig.h>

#include <catch2/catch_test_macros.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <list>

/**
 * A stylesheet that cannot be parsed is never adopted as the active style
 * configuration: the database keeps the configuration it had before the
 * attempt, and a database that never had one gets the safe (empty) fallback.
 * Rendering therefore never runs without a style configuration.
 *
 * Test data: a database directory (argv[1] or TESTS_TOP_DIR/data/testregion)
 * and the stylesheet directory of this repository (argv[2] or
 * TESTS_TOP_DIR/../stylesheets).
 */
namespace {

std::filesystem::path TestsTopDir()
{
  const char *topDir = std::getenv("TESTS_TOP_DIR");

  REQUIRE(topDir != nullptr);

  return topDir;
}

std::filesystem::path TestDataDir()
{
  return TestsTopDir() / "data" / "testregion";
}

std::filesystem::path StyleSheetDir()
{
  return TestsTopDir().parent_path() / "stylesheets";
}

/**
 * A stylesheet that fails to parse: it includes a module that does not exist
 * and contains a syntax error. (Deliberately not an invalid colour literal -
 * that asserts in the colour helper instead of reporting an error.)
 */
std::filesystem::path WriteUnparsableStyleSheet()
{
  std::filesystem::path file=std::filesystem::temp_directory_path()/"osmscout_unparsable_style.oss";

  std::ofstream stream(file);
  stream << "MODULE \"missing-module-for-style-load-test\"" << std::endl;
  stream << "STYLE" << std::endl;
  stream << "  [TYPE _route" << std::endl;
  stream.close();

  return file;
}

struct TestDatabase
{
  std::filesystem::path dataDir;
  osmscout::DatabaseRef database;
  osmscout::TypeConfigRef typeConfig;

  explicit TestDatabase(const std::filesystem::path &dataDir)
    : dataDir(dataDir)
  {
    osmscout::DatabaseParameter parameter;

    database=std::make_shared<osmscout::Database>(parameter);
    REQUIRE(database->Open(dataDir.string()));

    typeConfig=database->GetTypeConfig();
    REQUIRE(typeConfig != nullptr);
  }

  osmscout::DBInstanceRef Instance(const osmscout::StyleConfigRef &styleConfig) const
  {
    return std::make_shared<osmscout::DBInstance>(dataDir.string(),
                                                 database,
                                                 std::make_shared<osmscout::LocationService>(database),
                                                 std::make_shared<osmscout::LocationDescriptionService>(database),
                                                 std::make_shared<osmscout::MapService>(database),
                                                 styleConfig);
  }
};

osmscout::StyleConfigRef LoadStyleSheet(const osmscout::TypeConfigRef &typeConfig,
                                        const std::filesystem::path &file)
{
  osmscout::StyleConfigRef config=std::make_shared<osmscout::StyleConfig>(typeConfig);

  REQUIRE(config->Load(file.string(), nullptr, false));

  return config;
}

}

TEST_CASE("Rejected stylesheet keeps the previously active configuration")
{
  TestDatabase testDatabase{TestDataDir()};

  auto stylesheetDir=StyleSheetDir();
  auto active=LoadStyleSheet(testDatabase.typeConfig, stylesheetDir/"standard.oss");
  auto fallback=std::make_shared<osmscout::StyleConfig>(testDatabase.typeConfig);

  auto instance=testDatabase.Instance(active);

  std::list<osmscout::StyleError> errors;
  REQUIRE_FALSE(instance->LoadStyle(WriteUnparsableStyleSheet().string(), {}, errors, fallback));
  REQUIRE_FALSE(errors.empty());

  // The rejected stylesheet is not the active style, and the previous
  // configuration was not released: rendering always has one.
  REQUIRE(instance->GetStyleConfig() == active);
}

TEST_CASE("First failed load installs the fallback configuration")
{
  TestDatabase testDatabase{TestDataDir()};

  auto fallback=std::make_shared<osmscout::StyleConfig>(testDatabase.typeConfig);

  // No stylesheet has ever loaded for this database.
  auto instance=testDatabase.Instance(nullptr);
  REQUIRE(instance->GetStyleConfig() == nullptr);

  std::list<osmscout::StyleError> errors;
  REQUIRE_FALSE(instance->LoadStyle(WriteUnparsableStyleSheet().string(), {}, errors, fallback));

  REQUIRE(instance->GetStyleConfig() == fallback);
}

TEST_CASE("Successful load adopts the requested stylesheet")
{
  TestDatabase testDatabase{TestDataDir()};

  auto stylesheetDir=StyleSheetDir();
  auto active=LoadStyleSheet(testDatabase.typeConfig, stylesheetDir/"standard.oss");
  auto fallback=std::make_shared<osmscout::StyleConfig>(testDatabase.typeConfig);

  auto instance=testDatabase.Instance(active);

  std::list<osmscout::StyleError> errors;
  REQUIRE(instance->LoadStyle((stylesheetDir/"cycle.oss").string(), {}, errors, fallback));
  REQUIRE(errors.empty());
  REQUIRE(instance->GetStyleConfig() != active);

  // Recovery: a valid stylesheet after a failed attempt is adopted.
  REQUIRE_FALSE(instance->LoadStyle(WriteUnparsableStyleSheet().string(), {}, errors, fallback));
  auto rejected=instance->GetStyleConfig();
  REQUIRE(rejected != nullptr);
  REQUIRE(instance->LoadStyle((stylesheetDir/"winter-sports.oss").string(), {}, errors, fallback));
  REQUIRE(instance->GetStyleConfig() != rejected);
}

TEST_CASE("Painting a batch with a database on the fallback configuration completes")
{
  TestDatabase testDatabase{TestDataDir()};

  auto stylesheetDir=StyleSheetDir();
  auto active=LoadStyleSheet(testDatabase.typeConfig, stylesheetDir/"standard.oss");
  auto fallback=std::make_shared<osmscout::StyleConfig>(testDatabase.typeConfig);

  // One database whose stylesheet was rejected (empty fallback configuration)
  // and one with a valid configuration, painted in the same batch: painting
  // never runs without a configuration and the healthy database is unaffected.
  auto rejectedInstance=testDatabase.Instance(nullptr);
  std::list<osmscout::StyleError> errors;
  REQUIRE_FALSE(rejectedInstance->LoadStyle(WriteUnparsableStyleSheet().string(), {}, errors, fallback));
  REQUIRE(rejectedInstance->GetStyleConfig() == fallback);

  auto healthyInstance=testDatabase.Instance(active);

  osmscout::MapData rejectedData;
  rejectedData.styleConfig=rejectedInstance->GetStyleConfig();
  osmscout::MapData healthyData;
  healthyData.styleConfig=healthyInstance->GetStyleConfig();

  osmscout::MercatorProjection projection;
  REQUIRE(projection.Set(osmscout::GeoCoord(50.412, 14.534),
                         osmscout::Magnification(osmscout::Magnification::magClose),
                         300,
                         400,
                         400));

  osmscout::MapParameter parameter;

  osmscout::MapPainterNoOp painter;
  REQUIRE(painter.DrawMap(projection,
                          parameter,
                          std::vector<osmscout::MapData>{rejectedData, healthyData}));
}
