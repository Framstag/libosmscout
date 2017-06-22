#include <iostream>

#include <osmscout/util/CmdLineParsing.h>

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

bool CheckParsing(const std::string& testName,
                  const std::vector<std::string>& arguments,
                  bool success,
                  const Arguments& expectedResult)
{
  Arguments               actualResult;
  osmscout::CmdLineParser parser(arguments);

  std::cout << testName << std::endl;

  parser.AddOptionalArg(osmscout::CmdLineFlag(std::ref(actualResult),&Arguments::SetMagicFlag),
                        "set some magic flag",
                        "--magicFlag");

  parser.AddOptionalArg(osmscout::CmdLineBoolOption(std::ref(actualResult),&Arguments::SetWitchyFlag),
                        "set some witchy flag",
                        "--witchyFlag");

  parser.AddOptionalArg(osmscout::CmdLineSizeTOption(std::ref(actualResult),&Arguments::SetDistance),
                        "set distance",
                        "--distance");

  parser.AddOptionalArg(osmscout::CmdLineStringOption(std::ref(actualResult),&Arguments::SetPath),
                        "set path",
                        "--path");

  std::cout << parser.GetHelp() << std::endl;

  osmscout::CmdLineParseResult result=parser.Parse();

  if (result.Success()!=success) {
    std::cerr << testName << ": expected success " << success << ", but actual success was " << result.Success() << " " << result.GetErrorDescription() << std::endl;
    return false;
  }

  if (actualResult==expectedResult) {
    return true;
  }
  else {
    std::cerr << testName << ": expected result was not equal to actual result" << std::endl;
    std::cout << "Expected: " << expectedResult << std::endl;
    std::cout << "Actual: " << actualResult << std::endl;
    return false;
  }
}

int main()
{
  int errors=0;

  {
    // Simple flag parsing
    std::vector<std::string> arguments={"Test", "--magicFlag"};
    Arguments                expectedResult;

    expectedResult.SetMagicFlag(true);

    if (!CheckParsing("Test 1",
                      arguments,
                      true,
                      expectedResult)) {
      errors++;
    }
  }

  {
    // bool option without value => Error expectedc
    std::vector<std::string> arguments={"Test", "--witchyFlag"};
    Arguments                expectedResult;

    if (!CheckParsing("Test 2",
                      arguments,
                      false,
                      expectedResult)) {
      errors++;
    }
  }

  {
    // Bool option with parameter
    std::vector<std::string> arguments={"Test", "--witchyFlag", "true"};
    Arguments                expectedResult;

    expectedResult.SetWitchyFlag(true);

    if (!CheckParsing("Test 3",
                      arguments,
                      true,
                      expectedResult)) {
      errors++;
    }
  }

  {
    // Number option with parameter
    std::vector<std::string> arguments={"Test", "--distance", "1000"};
    Arguments                expectedResult;

    expectedResult.SetDistance(1000);

    if (!CheckParsing("Test 4",
                      arguments,
                      true,
                      expectedResult)) {
      errors++;
    }
  }

  {
    // String option with parameter
    std::vector<std::string> arguments={"Test", "--path", "/dev/null"};
    Arguments                expectedResult;

    expectedResult.SetPath("/dev/null");

    if (!CheckParsing("Test 5",
                      arguments,
                      true,
                      expectedResult)) {
      errors++;
    }
  }

  if (errors!=0) {
    return 1;
  }
  else {
    return 0;
  }
}
