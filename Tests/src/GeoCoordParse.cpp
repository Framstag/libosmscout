#include <iostream>

#include <osmscout/GeoCoord.h>

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

bool CheckParseFail(const std::string& text)
{
  osmscout::GeoCoord coord;

  std::cout << "Expect '" << text << "' to fail" << std::endl;

  if (!osmscout::GeoCoord::Parse(text,
                                 coord)) {
    std::cout << "OK" << std::endl;
    return true;
  }

  std::cerr << "FAIL: Was parsed to " << coord.GetDisplayText() << std::endl;
  return false;
}

bool CheckParseSuccess(const std::string& text,
                       const osmscout::GeoCoord& expected)
{
  osmscout::GeoCoord coord;

  std::cout << "Expect '" << text << "' to parse to " << expected.GetDisplayText() << std::endl;

  if (osmscout::GeoCoord::Parse(text,
                                coord)) {
    if (coord.GetLat()>=expected.GetLat()-0.00001 &&
        coord.GetLat()<=expected.GetLat()+0.00001 &&
        coord.GetLon()>=expected.GetLon()-0.00001 &&
        coord.GetLon()<=expected.GetLon()+0.00001) {
      std::cout << "OK" << std::endl;
      return true;
    }

    std::cerr << "FAIL: Was parsed to " <<  coord.GetDisplayText() << std::endl;
    return false;
  }

  std::cerr << "FAIL: parsing failed" << std::endl;
  return false;
}

TEST_CASE()
{
    // Empty string
    REQUIRE(CheckParseFail(""));

    // Only whitespace
    REQUIRE(CheckParseFail(" "));

    // Illegal character
    REQUIRE(CheckParseFail("X"));

    // Incomplete
    REQUIRE(CheckParseFail("N"));

    // Incomplete
    REQUIRE(CheckParseFail("N "));

    // Incomplete
    REQUIRE(CheckParseFail("N 40"));

    // Incomplete
    REQUIRE(CheckParseFail("N 40 W"));

    // Incomplete
    REQUIRE(CheckParseFail("N 40 X"));

    // Simplest complete case
    REQUIRE(CheckParseSuccess("40 7", osmscout::GeoCoord(40.0, 7.0)));

    //
    // various variants for positive/negative values
    //
    REQUIRE(CheckParseSuccess("40 -7", osmscout::GeoCoord(40.0, -7.0)));
    REQUIRE(CheckParseSuccess("+40 -7", osmscout::GeoCoord(40.0, -7.0)));
    REQUIRE(CheckParseSuccess("N40 E7", osmscout::GeoCoord(40.0, 7.0)));
    REQUIRE(CheckParseSuccess("N 40 E 7", osmscout::GeoCoord(40.0, 7.0)));
    REQUIRE(CheckParseSuccess("40 N E 7", osmscout::GeoCoord(40.0, 7.0)));
    REQUIRE(CheckParseSuccess("40 N 7 E", osmscout::GeoCoord(40.0, 7.0)));
    REQUIRE(CheckParseSuccess("40 N 7 E  ", osmscout::GeoCoord(40.0, 7.0)));

    // Trailing garbage
    REQUIRE(CheckParseFail("40 7X"));

    // Trailing garbage
    REQUIRE(CheckParseFail("40 7 X"));

    //
    // Now with fraction values
    //
    REQUIRE(CheckParseSuccess("40.1 7.1", osmscout::GeoCoord(40.1, 7.1)));
    REQUIRE(CheckParseSuccess("40.12 7.12", osmscout::GeoCoord(40.12, 7.12)));
    REQUIRE(CheckParseSuccess("40,123 7,123", osmscout::GeoCoord(40.123, 7.123)));
    REQUIRE(CheckParseSuccess("40,123 -7,123", osmscout::GeoCoord(40.123, -7.123)));
    REQUIRE(CheckParseSuccess("50°5'8.860\"N 14°24'37.592\"E", osmscout::GeoCoord(50.0857944, 14.4104422)));
    REQUIRE(CheckParseSuccess("N 50°5.14767' E 14°24.62653'", osmscout::GeoCoord(50.0857944, 14.4104422)));
}
