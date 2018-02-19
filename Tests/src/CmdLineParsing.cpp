#include <iostream>

#include <osmscout/util/CmdLineParsing.h>

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

class Arguments
{
private:
  bool               help;
  bool               magicFlag;
  bool               witchyFlag;
  size_t             distance;
  double             doubleTrouble;
  osmscout::GeoCoord location;
  std::string        path;
  std::string        file;
  size_t             enumeration;

private:
  friend std::ostream & operator<<(std::ostream &out, const Arguments& args);

public:
  Arguments()
  : help(false),
    magicFlag(false),
    witchyFlag(false),
    distance(0),
    doubleTrouble(0.0),
    location(osmscout::GeoCoord(0.0,0.0)),
    enumeration(0)
  {
    // no code
  }

  void SetHelp(const bool& help)
  {
    this->help=help;
  }

  bool GetHelp() const
  {
    return help;
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

  void SetDoubleTrouble(const double& doubleTrouble)
  {
    this->doubleTrouble=doubleTrouble;
  }

  void SetLocation(const osmscout::GeoCoord& location)
  {
    this->location=location;
  }

  void SetPath(const std::string& path)
  {
    this->path=path;
  }

  void SetFile(const std::string& file)
  {
    this->file=file;
  }

  void SetEnumeration(size_t enumeration)
  {
    this->enumeration=enumeration;
  }

  bool operator==(const Arguments& other) const
  {
    return this->help==other.help &&
           this->magicFlag==other.magicFlag &&
           this->witchyFlag==other.witchyFlag &&
           this->distance==other.distance &&
           this->doubleTrouble==other.doubleTrouble &&
           this->location==other.location &&
           this->path==other.path &&
           this->file==other.file &&
           this->enumeration==other.enumeration;
  }

};

std::ostream &operator<<(std::ostream &out, const Arguments& args)
{
  out << "help: " << args.help;
  out << " magicFlag: " << args.magicFlag;
  out << " witchyFlag " << args.witchyFlag;
  out << " distance " << args.distance;
  out << " doubleTrouble " << args.doubleTrouble;
  out << " location " << args.location.GetDisplayText();
  out << " path " << args.path;
  out << " file " << args.file;
  out << " enumeration " << args.enumeration;

  return out;
}

void CallParser(const std::vector<std::string>& arguments,
                osmscout::CmdLineParseResult& actualResult,
                Arguments& actualData)
{
  osmscout::CmdLineParser parser("Test",
                                 arguments);

  std::vector<std::string> helpArgs={"h","help"};

  parser.AddOption(osmscout::CmdLineFlag([&actualData](const bool& flag) {
                     actualData.SetHelp(flag);
                   }),
                   helpArgs,
                   "show some help",
                   true);

  parser.AddOption(osmscout::CmdLineFlag([&actualData](const bool& flag) {
                     actualData.SetMagicFlag(flag);
                   }),
                   "magicFlag",
                   "set some magic flag");

  parser.AddOption(osmscout::CmdLineBoolOption([&actualData](const bool& flag) {
                     actualData.SetWitchyFlag(flag);
                   }),
                   "witchyFlag",
                   "set some witchy flag");

  parser.AddOption(osmscout::CmdLineSizeTOption([&actualData](const size_t& value) {
                     actualData.SetDistance(value);
                   }),
                   "distance",
                   "set distance");

  parser.AddOption(osmscout::CmdLineDoubleOption([&actualData](const double& value) {
                     actualData.SetDoubleTrouble(value);
                   }),
                   "doubleTrouble",
                   "set double trouble");

  parser.AddOption(osmscout::CmdLineGeoCoordOption([&actualData](const osmscout::GeoCoord& value) {
                     actualData.SetLocation(value);
                   }),
                   "location",
                   "set location");

  parser.AddOption(osmscout::CmdLineStringOption([&actualData](const std::string& value) {
                     actualData.SetPath(value);
                   }),
                   "path",
                   "set path");

  parser.AddOption(osmscout::CmdLineAlternativeFlag([&actualData](const std::string& value) {
                     if (value=="O1") {
                       actualData.SetEnumeration(1);
                     }
                     else if (value=="O2") {
                       actualData.SetEnumeration(2);
                     }
                     else if (value=="O3") {
                       actualData.SetEnumeration(3);
                     }
                   }),
                   {"O1","O2","O3"},
                   "set enumeration");

  parser.AddPositional(osmscout::CmdLineStringOption([&actualData](const std::string& value) {
                         actualData.SetFile(value);
                       }),
                       "file",
                       "set file");

  actualResult=parser.Parse();

  if (actualData.GetHelp()) {
    std::cout << parser.GetHelp() << std::endl;
  }
}

TEST_CASE("Parsing of short help option") {
  std::vector<std::string> arguments={"Test", "-h"};
  osmscout::CmdLineParseResult actualResult;
  Arguments                    actualData;
  Arguments                    expectedData;

  expectedData.SetHelp(true);

  CallParser(arguments,
             actualResult,
             actualData);

  REQUIRE(actualResult.Success());
  REQUIRE(actualData==expectedData);
}

TEST_CASE("Parsing of long help option") {
  std::vector<std::string> arguments={"Test", "--help"};
  osmscout::CmdLineParseResult actualResult;
  Arguments                    actualData;
  Arguments                    expectedData;

  expectedData.SetHelp(true);

  CallParser(arguments,
             actualResult,
             actualData);

  REQUIRE(actualResult.Success());
  REQUIRE(actualData==expectedData);
}

TEST_CASE("Parsing of flag option") {
  std::vector<std::string> arguments={"Test", "--magicFlag", "filename"};
  osmscout::CmdLineParseResult actualResult;
  Arguments                    actualData;
  Arguments                    expectedData;

  expectedData.SetMagicFlag(true);
  expectedData.SetFile("filename");

  CallParser(arguments,
             actualResult,
             actualData);

  REQUIRE(actualResult.Success());
  REQUIRE(actualData==expectedData);
}

TEST_CASE("Parsing of bool option with valid value") {
  std::vector<std::string> arguments={"Test", "--witchyFlag", "true", "filename"};
  osmscout::CmdLineParseResult actualResult;
  Arguments                    actualData;
  Arguments                    expectedData;

  expectedData.SetWitchyFlag(true);
  expectedData.SetFile("filename");

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
  std::vector<std::string> arguments={"Test", "--witchyFlag", "witchy", "filename"};
  osmscout::CmdLineParseResult actualResult;
  Arguments                    actualData;
  Arguments                    expectedData;

  CallParser(arguments,
             actualResult,
             actualData);

  REQUIRE(actualResult.HasError());
  REQUIRE(actualData==expectedData);
}

TEST_CASE("Parsing of size_t option with valid value") {
  std::vector<std::string> arguments={"Test", "--distance", "1000", "filename"};
  osmscout::CmdLineParseResult actualResult;
  Arguments                    actualData;
  Arguments                    expectedData;

  expectedData.SetDistance(1000);
  expectedData.SetFile("filename");

  CallParser(arguments,
             actualResult,
             actualData);

  REQUIRE(actualResult.Success());
  REQUIRE(actualData==expectedData);
}

TEST_CASE("Parsing of size_t option with negative value") {
  std::vector<std::string> arguments={"Test", "--distance", "-1000", "filename"};
  osmscout::CmdLineParseResult actualResult;
  Arguments                    actualData;
  Arguments                    expectedData;

  CallParser(arguments,
             actualResult,
             actualData);

  REQUIRE(actualResult.HasError());
  REQUIRE(actualData==expectedData);
}

TEST_CASE("Parsing of double option with valid value") {
  std::vector<std::string> arguments={"Test", "--doubleTrouble", "1000.0", "filename"};
  osmscout::CmdLineParseResult actualResult;
  Arguments                    actualData;
  Arguments                    expectedData;

  expectedData.SetDoubleTrouble(1000.0);
  expectedData.SetFile("filename");

  CallParser(arguments,
             actualResult,
             actualData);

  REQUIRE(actualResult.Success());
  REQUIRE(actualData==expectedData);
}

TEST_CASE("Parsing of string option with valid value") {
  std::vector<std::string> arguments={"Test", "--path", "/dev/null", "filename"};
  osmscout::CmdLineParseResult actualResult;
  Arguments                    actualData;
  Arguments                    expectedData;

  expectedData.SetPath("/dev/null");
  expectedData.SetFile("filename");

  CallParser(arguments,
             actualResult,
             actualData);

  REQUIRE(actualResult.Success());
  REQUIRE(actualData==expectedData);
}

TEST_CASE("Parsing of geo coordinate option with valid value") {
  std::vector<std::string> arguments={"Test", "--location", "34.5", "-77.8", "filename"};
  osmscout::CmdLineParseResult actualResult;
  Arguments                    actualData;
  Arguments                    expectedData;

  expectedData.SetLocation(osmscout::GeoCoord(34.5,-77.8));
  expectedData.SetFile("filename");

  CallParser(arguments,
             actualResult,
             actualData);

  REQUIRE(actualResult.Success());
  REQUIRE(actualData==expectedData);
}

TEST_CASE("Parsing of geo coordinate option with invalid value") {
  std::vector<std::string> arguments={"Test", "--location", "120.0", "-77.8", "filename"};
  osmscout::CmdLineParseResult actualResult;
  Arguments                    actualData;
  Arguments                    expectedData;

  CallParser(arguments,
             actualResult,
             actualData);

  REQUIRE(actualResult.HasError());
  REQUIRE(actualData==expectedData);
}

TEST_CASE("Parsing of unknown option") {
  std::vector<std::string> arguments={"Test", "--unknown", "filename"};
  osmscout::CmdLineParseResult actualResult;
  Arguments                    actualData;
  Arguments                    expectedData;

  CallParser(arguments,
             actualResult,
             actualData);

  REQUIRE(actualResult.HasError());
  REQUIRE(actualData==expectedData);
}

TEST_CASE("Parsing of valid alternative flag option") {
  std::vector<std::string> arguments={"Test", "--O1", "filename"};
  osmscout::CmdLineParseResult actualResult;
  Arguments                    actualData;
  Arguments                    expectedData;

  expectedData.SetEnumeration(1);
  expectedData.SetFile("filename");

  CallParser(arguments,
             actualResult,
             actualData);

  REQUIRE(actualResult.Success());
  REQUIRE(actualData==expectedData);
}

TEST_CASE("Parsing of alternative flag option with multiple instances") {
  std::vector<std::string> arguments={"Test", "--O1", "--O2", "filename"};
  osmscout::CmdLineParseResult actualResult;
  Arguments                    actualData;
  Arguments                    expectedData;

  expectedData.SetEnumeration(1);
  //expectedData.SetFile("filename");

  CallParser(arguments,
             actualResult,
             actualData);

  REQUIRE(actualResult.HasError());
  REQUIRE(actualResult.GetErrorDescription()=="You cannot call flag argument '--O2', since alternative flag argument '--O1' was already called");
  REQUIRE(actualData==expectedData);
}
