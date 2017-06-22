#include <iostream>

#include <osmscout/util/CmdLineParsing.h>

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

class Arguments
{
private:
  bool        magicFlag;
  bool        witchyFlag;
  size_t      distance;
  std::string path;

private:
  friend std::ostream & operator<<(std::ostream &os, const Arguments& args);

public:
  Arguments()
  : magicFlag(false),
    witchyFlag(false),
    distance(0)
  {
    // no code
  }

  void SetMagicFlag(const bool& magicFlag)
  {
    this->magicFlag=magicFlag;
  }

  void SetWitchyFlag(const bool& witchyFlag)
  {
    this->witchyFlag=witchyFlag;
  }

  void SetDistance(const size_t& distance)
  {
    this->distance=distance;
  }

  void SetPath(const std::string& path)
  {
    this->path=path;
  }

  bool operator==(const Arguments& other) const
  {
    return this->magicFlag==other.magicFlag &&
           this->witchyFlag==other.witchyFlag &&
           this->distance==other.distance &&
           this->path==other.path;
  }

};

std::ostream &operator<<(std::ostream &out, const Arguments& args)
{
  out << "magicFlag: " << args.magicFlag;
  out << " witchyFlag " << args.witchyFlag;
  out << " distance " << args.distance;
  out << " path " << args.path;

  return out;
}

void CallParser(const std::vector<std::string>& arguments,
                osmscout::CmdLineParseResult& actualResult,
                Arguments& actualData)
{
  osmscout::CmdLineParser parser(arguments);

  parser.AddOptionalArg(osmscout::CmdLineFlag(std::ref(actualData),&Arguments::SetMagicFlag),
                        "set some magic flag",
                        "--magicFlag");

  parser.AddOptionalArg(osmscout::CmdLineBoolOption(std::ref(actualData),&Arguments::SetWitchyFlag),
                        "set some witchy flag",
                        "--witchyFlag");

  parser.AddOptionalArg(osmscout::CmdLineSizeTOption(std::ref(actualData),&Arguments::SetDistance),
                        "set distance",
                        "--distance");

  parser.AddOptionalArg(osmscout::CmdLineStringOption(std::ref(actualData),&Arguments::SetPath),
                        "set path",
                        "--path");

  actualResult=parser.Parse();
}

TEST_CASE("Parsing of flag option") {
  std::vector<std::string> arguments={"Test", "--magicFlag"};
  osmscout::CmdLineParseResult actualResult;
  Arguments                    actualData;
  Arguments                    expectedData;

  expectedData.SetMagicFlag(true);

  CallParser(arguments,
             actualResult,
             actualData);

  REQUIRE(actualResult.Success());
  REQUIRE(actualData==expectedData);
}

TEST_CASE("Parsing of bool option with valid value") {
  std::vector<std::string> arguments={"Test", "--witchyFlag", "true"};
  osmscout::CmdLineParseResult actualResult;
  Arguments                    actualData;
  Arguments                    expectedData;

  expectedData.SetWitchyFlag(true);

  CallParser(arguments,
             actualResult,
             actualData);

  REQUIRE(actualResult.Success());
  REQUIRE(actualData==expectedData);
}

TEST_CASE("Parsing of bool option without value") {
  std::vector<std::string> arguments={"Test", "--witchyFlag"};
  osmscout::CmdLineParseResult actualResult;
  Arguments                    actualData;
  Arguments                    expectedData;

  CallParser(arguments,
             actualResult,
             actualData);

  REQUIRE(actualResult.HasError());
  REQUIRE(actualData==expectedData);
}

TEST_CASE("Parsing of bool option with wrong value") {
  std::vector<std::string> arguments={"Test", "--witchyFlag", "witchy"};
  osmscout::CmdLineParseResult actualResult;
  Arguments                    actualData;
  Arguments                    expectedData;

  CallParser(arguments,
             actualResult,
             actualData);

  REQUIRE(actualResult.HasError());
  REQUIRE(actualData==expectedData);
}

TEST_CASE("Parsing of number option with valid value") {
  std::vector<std::string> arguments={"Test", "--distance", "1000"};
  osmscout::CmdLineParseResult actualResult;
  Arguments                    actualData;
  Arguments                    expectedData;

  expectedData.SetDistance(1000);

  CallParser(arguments,
             actualResult,
             actualData);

  REQUIRE(actualResult.Success());
  REQUIRE(actualData==expectedData);
}

TEST_CASE("Parsing of number option with invalid value") {
  std::vector<std::string> arguments={"Test", "--distance", "-1000"};
  osmscout::CmdLineParseResult actualResult;
  Arguments                    actualData;
  Arguments                    expectedData;


  CallParser(arguments,
             actualResult,
             actualData);

  REQUIRE(actualResult.HasError());
  REQUIRE(actualData==expectedData);
}

TEST_CASE("Parsing of string option with valid value") {
  std::vector<std::string> arguments={"Test", "--path", "/dev/null"};
  osmscout::CmdLineParseResult actualResult;
  Arguments                    actualData;
  Arguments                    expectedData;

  expectedData.SetPath("/dev/null");

  CallParser(arguments,
             actualResult,
             actualData);

  REQUIRE(actualResult.Success());
  REQUIRE(actualData==expectedData);
}
