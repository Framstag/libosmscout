#include <iostream>

#include <osmscout/util/Color.h>

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

struct TestData
{
  std::string     colorString;
  osmscout::Color value;
};

static std::vector<TestData> testData = {
  {"#000000", osmscout::Color::BLACK},
  {"#ffffff", osmscout::Color::WHITE},
  {"#ff0000",osmscout::Color::RED},
  {"#00ff00",osmscout::Color::GREEN},
  {"#0000ff",osmscout::Color::BLUE}
};

TEST_CASE("Parsing of #XXXXXX syntax") {
  for (const auto& test : testData) {
    SECTION("Conversion of "+test.colorString) {
      std::string colorString=test.colorString;
      osmscout::Color parsedColor=osmscout::Color::FromHexString(colorString);

      std::string printedColor=parsedColor.ToHexString();

      REQUIRE(colorString==printedColor);
      REQUIRE(parsedColor==test.value);
    }
  }
}
