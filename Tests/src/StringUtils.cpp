#include <iostream>

#include <osmscout/util/String.h>

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

TEST_CASE("Split empty string")
{
  auto elements = osmscout::SplitString("", ";");
  REQUIRE(elements.empty());
}

TEST_CASE("Split string by simple separator")
{
  auto elements=osmscout::SplitString("asphalt;ground;gravel", ";");
  REQUIRE(elements.size() == 3);
  auto it=elements.begin();
  REQUIRE(*(it++) == "asphalt");
  REQUIRE(*(it++) == "ground");
  REQUIRE(*(it++) == "gravel");
}

TEST_CASE("Separator on the end dont produce empty element")
{
  auto elements=osmscout::SplitString(";asphalt;ground;gravel;", ";");
  REQUIRE(elements.size() == 4);
  auto it=elements.begin();
  REQUIRE(*(it++) == "");
  REQUIRE(*(it++) == "asphalt");
  REQUIRE(*(it++) == "ground");
  REQUIRE(*(it++) == "gravel");
}

TEST_CASE("Split string by multi-character separator")
{
  auto elements=osmscout::SplitString("asphalt <> ground <> gravel", " <> ");
  REQUIRE(elements.size() == 3);
  auto it=elements.begin();
  REQUIRE(*(it++) == "asphalt");
  REQUIRE(*(it++) == "ground");
  REQUIRE(*(it++) == "gravel");
}
