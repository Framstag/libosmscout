/*
  This source is part of the libosmscout library
  Copyright (C) 2017  Tim Teulings

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

#include <osmscout/util/CmdLineParsing.h>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <sstream>

#include <osmscout/util/String.h>

#include <osmscout/system/Assert.h>

namespace osmscout {

  CmdLineScanner::CmdLineScanner(int argc, char* argv[])
    : nextArg(0)
  {
    arguments.reserve((size_t)argc);

    for (int i=0; i<=argc; i++) {
      arguments.push_back(std::string(argv[i]));
    }

  }

  CmdLineScanner::CmdLineScanner(const std::vector<std::string>& arguments)
    : arguments(arguments),
      nextArg(0)
  {
    // no code
  }

  bool CmdLineScanner::HasNextArg() const
  {
    return nextArg<arguments.size();
  }

  std::string CmdLineScanner::PeakNextArg() const
  {
    assert(nextArg<arguments.size());
    return arguments[nextArg];
  }

  std::string CmdLineScanner::Advance()
  {
    assert(nextArg<arguments.size());

    size_t currentArg=nextArg;

    nextArg++;

    return arguments[currentArg];
  }

  std::string CmdLineScanner::GetCurrentArg() const
  {
    assert(nextArg>0);
    return arguments[nextArg-1];
  }

  CmdLineParseResult::CmdLineParseResult()
    : hasError(false)
  {
    // no code
  }

  CmdLineParseResult::CmdLineParseResult(const std::string& errorDescription)
    : hasError(true),
      errorDescription(errorDescription)
  {
    // no code
  }

  bool CmdLineParseResult::Success() const
  {
    return !hasError;
  }

  bool CmdLineParseResult::HasError() const
  {
    return hasError;
  }

  std::string CmdLineParseResult::GetErrorDescription() const
  {
    return errorDescription;
  }

  CmdLineArgParser::~CmdLineArgParser()
  {
    // no code
  }

  CmdLineFlagArgParser::CmdLineFlagArgParser(SetterFunction&& setter)
  : setter(setter)
  {
    // no code
  }

  std::string CmdLineFlagArgParser::GetArgTemplate(const std::string& arg) const
  {
    return arg;
  }

  CmdLineParseResult CmdLineFlagArgParser::Parse(CmdLineScanner& /*scanner*/)
  {
    setter(true);

    return CmdLineParseResult();
  }

  CmdLineBoolArgParser::CmdLineBoolArgParser(SetterFunction&& setter)
  : setter(setter)
  {
    // no code
  }

  std::string CmdLineBoolArgParser::GetArgTemplate(const std::string& arg) const
  {
    return arg+" <true|false>";
  }

  CmdLineParseResult CmdLineBoolArgParser::Parse(CmdLineScanner& scanner)
  {
    if (!scanner.HasNextArg()) {
      return CmdLineParseResult("Missing value for boolean option '"+scanner.GetCurrentArg()+"'");
    }

    std::string value=scanner.Advance();

    if (value=="true") {
      setter(true);
      return CmdLineParseResult();
    }
    else if (value=="false") {
      setter(false);
      return CmdLineParseResult();
    }
    else {
      return CmdLineParseResult("Value for boolean option '"+scanner.GetCurrentArg()+"' must be either 'true' or 'false' but not '"+value+"'");
    }
  }

  CmdLineStringArgParser::CmdLineStringArgParser(SetterFunction&& setter)
  : setter(setter)
  {
    // no code
  }

  std::string CmdLineStringArgParser::GetArgTemplate(const std::string& arg) const
  {
    return arg+" <string>";
  }

  CmdLineParseResult CmdLineStringArgParser::Parse(CmdLineScanner& scanner)
  {
    if (!scanner.HasNextArg()) {
      return CmdLineParseResult("Missing value for string option '"+scanner.GetCurrentArg()+"'");
    }

    std::string value=scanner.Advance();

    setter(value);

    return CmdLineParseResult();
  }

  CmdLineGeoCoordArgParser::CmdLineGeoCoordArgParser(SetterFunction&& setter)
  : setter(setter)
  {
    // no code
  }

  std::string CmdLineGeoCoordArgParser::GetArgTemplate(const std::string& arg) const
  {
    return arg+" <double> <double>";
  }

  CmdLineParseResult CmdLineGeoCoordArgParser::Parse(CmdLineScanner& scanner)
  {
    if (!scanner.HasNextArg()) {
      return CmdLineParseResult("Missing value for lat value of option '"+scanner.GetCurrentArg()+"'");
    }

    std::string latString=scanner.Advance();

    if (!scanner.HasNextArg()) {
      return CmdLineParseResult("Missing value for lon value of option '"+scanner.GetCurrentArg()+"'");
    }

    std::string lonString=scanner.Advance();

    double lat,lon;

    if (!StringToNumber(latString,lat)) {
      return CmdLineParseResult("Lat value of option '"+scanner.GetCurrentArg()+"' is not in valid format");
    }

    if (!StringToNumber(lonString,lon)) {
      return CmdLineParseResult("Lon value of option '"+scanner.GetCurrentArg()+"' is not in valid format");
    }

    if (lat<-90.0 || lat>90.0) {
      return CmdLineParseResult("Lat value of option '"+scanner.GetCurrentArg()+"' is not in valid range [-90.0,90.0]");
    }

    if (lon<-180.0 || lon>180.0) {
      return CmdLineParseResult("Lon value of option '"+scanner.GetCurrentArg()+"' is not in valid range [-180.0,180.0]");
    }

    setter(GeoCoord(lat,lon));

    return CmdLineParseResult();
  }

  CmdLineParser::CmdLineParser(int argc, char* argv[])
    : scanner(argc,argv)
  {
    // no code
  }

  CmdLineParser::CmdLineParser(const std::vector<std::string>& arguments)
    : scanner(arguments)
  {
    // no code
  }

  void CmdLineParser::AddOptionalArg(const CmdLineArgParserRef& parser,
                                     const std::string& helpString,
                                     const std::string& argumentName)
  {
    CmdLineArgDesc desc(parser,helpString,true);
    CmdLineArgHelp help(parser->GetArgTemplate(argumentName),helpString);

    options.insert(std::make_pair(argumentName,desc));
    helps.push_back(help);
  }

  void CmdLineParser::AddPositionalArg(const CmdLineArgParserRef& parser,
                                       const std::string& helpString,
                                       const std::string& argumentName)
  {
    CmdLineArgDesc desc(parser,helpString,true);
    CmdLineArgHelp help(parser->GetArgTemplate(argumentName),helpString);

    positionals.push_back(desc);
    helps.push_back(help);
  }

  CmdLineParseResult CmdLineParser::Parse()
  {
    // Skip the initial parameter naming the programm
    if (scanner.HasNextArg()) {
      scanner.Advance();
    }

    // Parse options until some was found that is not an option
    while (scanner.HasNextArg()) {
      std::string currentArg=scanner.PeakNextArg();

      auto option=options.find(currentArg);

      if (option==options.end()) {
        break;
      }

      /* ignore */ scanner.Advance();

      CmdLineParseResult result=option->second.parser->Parse(scanner);

      if (result.HasError()) {
        return result;
      }
    }

    if (scanner.HasNextArg()) {
      return CmdLineParseResult("Unknown command line argument '"+scanner.PeakNextArg()+"'");
    }

    // Alles OK
    return CmdLineParseResult();
  }

  std::string CmdLineParser::GetHelp(size_t indent) const
  {
    std::ostringstream stream;

    size_t maxNameLength=0;

    for (const auto& help : helps) {
      maxNameLength=std::max(maxNameLength,help.argTemplate.length());
    }

    for (const auto& help : helps) {
      for (size_t i=0; i<indent; i++) {
        stream << ' ';
      }

      stream << help.argTemplate;

      for (size_t i=0; i<maxNameLength-help.argTemplate.length(); i++) {
        stream << ' ';
      }

      stream << ' ' << help.helpString << std::endl;
    }

    return stream.str();

  }

  bool ParseBoolArgument(int argc,
                         char* argv[],
                         int& currentIndex,
                         bool& value)
  {
    int parameterIndex=currentIndex;
    int argumentIndex=currentIndex+1;

    currentIndex+=2;

    if (argumentIndex>=argc) {
      std::cerr << "Missing parameter after option '" << argv[parameterIndex] << "'" << std::endl;
      return false;
    }

    if (!StringToBool(argv[argumentIndex],
                      value)) {
      std::cerr << "Cannot parse argument for parameter '" << argv[parameterIndex] << "'" << std::endl;
      return false;
    }

    return true;
  }

  bool ParseStringArgument(int argc,
                           char* argv[],
                           int& currentIndex,
                           std::string& value)
  {
    int parameterIndex=currentIndex;
    int argumentIndex=currentIndex+1;

    currentIndex+=2;

    if (argumentIndex>=argc) {
      std::cerr << "Missing parameter after option '" << argv[parameterIndex] << "'" << std::endl;
      return false;
    }

    value=argv[argumentIndex];

    return true;
  }

  bool ParseSizeTArgument(int argc,
                          char* argv[],
                          int& currentIndex,
                          size_t& value)
  {
    int parameterIndex=currentIndex;
    int argumentIndex=currentIndex+1;

    currentIndex+=2;

    if (argumentIndex>=argc) {
      std::cerr << "Missing parameter after option '" << argv[parameterIndex] << "'" << std::endl;
      return false;
    }

    if (!StringToNumber(argv[argumentIndex],
                        value)) {
      std::cerr << "Cannot parse argument for parameter '" << argv[parameterIndex] << "'" << std::endl;
      return false;
    }

    return true;
  }
}
