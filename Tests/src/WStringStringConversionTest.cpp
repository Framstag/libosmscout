#include <exception>
#include <iostream>

#include <osmscout/util/String.h>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Check ANSI charset conversion")
{
  const std::string oText = "abcABC";
  std::wstring wText = osmscout::UTF8StringToWString(oText);
  std::string uText = osmscout::WStringToUTF8String(wText);
  REQUIRE(oText == uText);
}

TEST_CASE("Check german umlauts upper and lower case and euro-sign charset conversion")
{
  try {
    std::locale::global(std::locale(""));
  }
  catch (const std::exception &e) {
    std::cerr << "ERROR: Cannot set locale: " << e.what() << std::endl;
  }
  const std::string oText = "\xc3\x84\xc3\x96\xc3\x9c\xc3\xa4\xc3\xb6\xc3\xbc\xe2\x82\xac";
  std::wstring wText = osmscout::UTF8StringToWString(oText);
  std::string uText = osmscout::WStringToUTF8String(wText);
  REQUIRE(oText == uText);
}

TEST_CASE("Check ANSI to lower")
{
  std::string lText = osmscout::UTF8StringToLower("abcABC");
  REQUIRE(lText == "abcabc");
}

TEST_CASE("Check ANSI to upper")
{
  std::string uText = osmscout::UTF8StringToUpper("abcABC");
  REQUIRE(uText == "ABCABC");
}

TEST_CASE("Check German umlauts upper and lower case and euro-sign to lower")
{
  try {
    std::locale::global(std::locale(""));
  }
  catch (const std::exception &e) {
    std::cerr << "ERROR: Cannot set locale: " << e.what() << std::endl;
  }
  std::string lText = osmscout::UTF8StringToLower("\xc3\x84\xc3\x96\xc3\x9c\xc3\xa4\xc3\xb6\xc3\xbc\xe2\x82\xac");
  REQUIRE(lText == "\xc3\xa4\xc3\xb6\xc3\xbc\xc3\xa4\xc3\xb6\xc3\xbc\xe2\x82\xac");
}

TEST_CASE("Check German umlauts upper and lower case and euro-sign to upper")
{
  try {
    std::locale::global(std::locale(""));
  }
  catch (const std::exception &e) {
    std::cerr << "ERROR: Cannot set locale: " << e.what() << std::endl;
  }
  std::string uText = osmscout::UTF8StringToUpper("\xc3\x84\xc3\x96\xc3\x9c\xc3\xa4\xc3\xb6\xc3\xbc\xe2\x82\xac");
  REQUIRE(uText == "\xc3\x84\xc3\x96\xc3\x9c\xc3\x84\xc3\x96\xc3\x9c\xe2\x82\xac");
}

TEST_CASE("Check UTF8StringToU32String")
{
  try {
    std::locale::global(std::locale(""));
  }
  catch (const std::exception &e) {
    std::cerr << "ERROR: Cannot set locale: " << e.what() << std::endl;
  }
  std::u32string u32Text = osmscout::UTF8StringToU32String("");
  REQUIRE(u32Text.empty());
  u32Text = osmscout::UTF8StringToU32String("test");
  REQUIRE(u32Text.size() == 4);
  if (u32Text[0] == 't') {
    REQUIRE(u32Text == std::u32string{'t', 'e', 's', 't'});
  } else {
    REQUIRE(u32Text ==
            std::u32string{char32_t(0x74000000), char32_t(0x65000000), char32_t(0x73000000), char32_t(0x74000000)});
  }
}
