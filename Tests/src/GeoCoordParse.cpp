#include <iostream>

#include <osmscout/GeoCoord.h>

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

int main()
{
  int errors=0;

  // Empty string
  if (!CheckParseFail("")) {
    errors++;
  }

  // Only whitespace
  if (!CheckParseFail(" ")) {
    errors++;
  }

  // Illegal character
  if (!CheckParseFail("X")) {
    errors++;
  }

  // Incomplete
  if (!CheckParseFail("N")) {
    errors++;
  }

  // Incomplete
  if (!CheckParseFail("N ")) {
    errors++;
  }

  // Incomplete
  if (!CheckParseFail("N 40")) {
    errors++;
  }

  // Incomplete
  if (!CheckParseFail("N 40 W")) {
    errors++;
  }

  // Incomplete
  if (!CheckParseFail("N 40 X")) {
    errors++;
  }

  // Simplest complete case
  if (!CheckParseSuccess("40 7",
                         osmscout::GeoCoord(40.0,7.0))) {
    errors++;
  }

  //
  // various variants for positive/negative values
  //
  if (!CheckParseSuccess("40 -7",
                         osmscout::GeoCoord(40.0,-7.0))) {
    errors++;
  }

  if (!CheckParseSuccess("+40 -7",
                         osmscout::GeoCoord(40.0,-7.0))) {
    errors++;
  }

  if (!CheckParseSuccess("N40 E7",
                         osmscout::GeoCoord(40.0,7.0))) {
    errors++;
  }

  if (!CheckParseSuccess("N 40 E 7",
                         osmscout::GeoCoord(40.0,7.0))) {
    errors++;
  }

  if (!CheckParseSuccess("40 N E 7",
                         osmscout::GeoCoord(40.0,7.0))) {
    errors++;
  }

  if (!CheckParseSuccess("40 N 7 E",
                         osmscout::GeoCoord(40.0,7.0))) {
    errors++;
  }

  if (!CheckParseSuccess("40 N 7 E  ",
                         osmscout::GeoCoord(40.0,7.0))) {
    errors++;
  }

  // Trailing garbage
  if (!CheckParseFail("40 7X")) {
    errors++;
  }

  // Trailing garbage
  if (!CheckParseFail("40 7 X")) {
    errors++;
  }

  //
  // Now with fraction values
  //

  if (!CheckParseSuccess("40.1 7.1",
                         osmscout::GeoCoord(40.1,7.1))) {
    errors++;
  }

  if (!CheckParseSuccess("40.12 7.12",
                         osmscout::GeoCoord(40.12,7.12))) {
    errors++;
  }

  if (!CheckParseSuccess("40,123 7,123",
                         osmscout::GeoCoord(40.123,7.123))) {
    errors++;
  }

  if (!CheckParseSuccess("40,123 -7,123",
                         osmscout::GeoCoord(40.123,-7.123))) {
    errors++;
  }

  if (!CheckParseSuccess( "50°5'8.860\"N 14°24'37.592\"E",
                         osmscout::GeoCoord(50.0857944, 14.4104422))) {
    errors++;
  }

  if (!CheckParseSuccess("N 50°5.14767' E 14°24.62653'",
                         osmscout::GeoCoord(50.0857944, 14.4104422))) {
    errors++;
  }

  if (errors!=0) {
    return 1;
  }

  return 0;
}
