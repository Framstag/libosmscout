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

    for (int i=0; i<argc; i++) {
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

  void CmdLineArgParser::SetArgumentName(const std::string& argumentName)
  {
    this->argumentName=argumentName;
  }

  std::string CmdLineArgParser::GetArgumentName() const
  {
    return argumentName;
  }

  CmdLineFlagArgParser::CmdLineFlagArgParser(SetterFunction&& setter)
  : setter(setter)
  {
    // no code
  }

  std::string CmdLineFlagArgParser::GetOptionHint() const
  {
    return "";
  }

  std::string CmdLineFlagArgParser::GetPositionalHint(const std::string& positional) const
  {
    return positional;
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

  std::string CmdLineBoolArgParser::GetOptionHint() const
  {
    return "true|false";
  }

  std::string CmdLineBoolArgParser::GetPositionalHint(const std::string& positional) const
  {
    return positional;
  }

  CmdLineParseResult CmdLineBoolArgParser::Parse(CmdLineScanner& scanner)
  {
    if (!scanner.HasNextArg()) {
      return CmdLineParseResult("Missing value for boolean Argument '"+GetArgumentName()+"'");
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
      return CmdLineParseResult("Value for boolean argument '"+GetArgumentName()+"' must be either 'true' or 'false' but not '"+value+"'");
    }
  }

  CmdLineStringArgParser::CmdLineStringArgParser(SetterFunction&& setter)
  : setter(setter)
  {
    // no code
  }

  std::string CmdLineStringArgParser::GetOptionHint() const
  {
    return "string";
  }

  std::string CmdLineStringArgParser::GetPositionalHint(const std::string& positional) const
  {
    return positional;
  }

  CmdLineParseResult CmdLineStringArgParser::Parse(CmdLineScanner& scanner)
  {
    if (!scanner.HasNextArg()) {
      return CmdLineParseResult("Missing value for string argument '"+GetArgumentName()+"'");
    }

    std::string value=scanner.Advance();

    setter(value);

    return CmdLineParseResult();
  }

  CmdLineStringListArgParser::CmdLineStringListArgParser(AppendFunction&& appender)
    : appender(appender)
  {
    // no code
  }

  std::string CmdLineStringListArgParser::GetOptionHint() const
  {
    return "string...";
  }

  std::string CmdLineStringListArgParser::GetPositionalHint(const std::string& positional) const
  {
    return positional+"...";
  }

  CmdLineParseResult CmdLineStringListArgParser::Parse(CmdLineScanner& scanner)
  {
    if (!scanner.HasNextArg()) {
      return CmdLineParseResult("Missing value for string list argument '"+GetArgumentName()+"'");
    }

    while (scanner.HasNextArg()) {
      std::string value=scanner.Advance();

      appender(value);
    }

    return CmdLineParseResult();
  }

  CmdLineGeoCoordArgParser::CmdLineGeoCoordArgParser(SetterFunction&& setter)
  : setter(setter)
  {
    // no code
  }

  std::string CmdLineGeoCoordArgParser::GetOptionHint() const
  {
    return "double double";
  }

  std::string CmdLineGeoCoordArgParser::GetPositionalHint(const std::string& positional) const
  {
    return positional;
  }

  CmdLineParseResult CmdLineGeoCoordArgParser::Parse(CmdLineScanner& scanner)
  {
    if (!scanner.HasNextArg()) {
      return CmdLineParseResult("Missing value for lat value of argument '"+GetArgumentName()+"'");
    }

    std::string latString=scanner.Advance();

    if (!scanner.HasNextArg()) {
      return CmdLineParseResult("Missing value for lon value of argument '"+GetArgumentName()+"'");
    }

    std::string lonString=scanner.Advance();

    double lat,lon;

    if (!StringToNumber(latString,lat)) {
      return CmdLineParseResult("Lat value of argument '"+GetArgumentName()+"' is not in valid format");
    }

    if (!StringToNumber(lonString,lon)) {
      return CmdLineParseResult("Lon value of argument '"+GetArgumentName()+"' is not in valid format");
    }

    if (lat<-90.0 || lat>90.0) {
      return CmdLineParseResult("Lat value of argument '"+GetArgumentName()+"' is not in valid range [-90.0,90.0]");
    }

    if (lon<-180.0 || lon>180.0) {
      return CmdLineParseResult("Lon value of argument '"+GetArgumentName()+"' is not in valid range [-180.0,180.0]");
    }

    setter(GeoCoord(lat,lon));

    return CmdLineParseResult();
  }

  CmdLineParser::CmdLineParser(const std::string& appName,
                               int argc, char* argv[])
    : appName(appName),
      scanner(argc,argv)
  {
    // no code
  }

  CmdLineParser::CmdLineParser(const std::string& appName,
                               const std::vector<std::string>& arguments)
    : appName(appName),
      scanner(arguments)
  {
    // no code
  }

  void CmdLineParser::AddOption(const CmdLineArgParserRef& parser,
                                const std::string& optionName,
                                const std::string& helpString,
                                bool stopParsing)
  {
    assert(optionName.length()>=1);
    assert(optionName[0]!='-');

    std::string argument;

    if (optionName.length()==1) {
      argument="-"+optionName;
    }
    else {
      argument="--"+optionName;
    }

    parser->SetArgumentName(argument);

    CmdLineOption option(parser,
                         argument,
                         stopParsing);

    assert(options.find(option.option)==options.end());

    options.insert(std::make_pair(option.option,option));

    std::string callDescription=option.option;
    std::string argumentType=parser->GetOptionHint();

    if (!argumentType.empty()) {
      callDescription+=" <"+argumentType+">";
    }

    CmdLineArgHelp help(callDescription,helpString);

    optionHelps.push_back(help);
  }

  void CmdLineParser::AddOption(const CmdLineArgParserRef& parser,
                                const std::vector<std::string>& optionNames,
                                const std::string& helpString,
                                bool stopParsing)
  {
    std::vector<std::string> callDescriptions;

    for (const auto& optionName : optionNames) {
      assert(optionName.length()>=1);
      assert(optionName[0]!='-');

      std::string argument;

      if (optionName.length()==1) {
        argument="-"+optionName;
      }
      else {
        argument="--"+optionName;
      }

      parser->SetArgumentName(argument);

      CmdLineOption option(parser,
                           argument,
                           stopParsing);

      assert(options.find(option.option)==options.end());

      options.insert(std::make_pair(option.option,option));

      std::string callDescription=option.option;
      std::string argumentType=parser->GetOptionHint();

      if (!argumentType.empty()) {
        callDescription+=" <"+argumentType+">";
      }

      callDescriptions.push_back(callDescription);
    }

    CmdLineArgHelp help(callDescriptions,helpString);

    optionHelps.push_back(help);
  }

  void CmdLineParser::AddPositional(const CmdLineArgParserRef& parser,
                                    const std::string& argumentName,
                                    const std::string& helpString)
  {
    assert(argumentName[0]!='-');

    parser->SetArgumentName(argumentName);

    CmdLinePositional desc(parser,argumentName);

    std::string callDescription=argumentName;

    CmdLineArgHelp help(callDescription,helpString);

    positionals.push_back(desc);
    positionalHelps.push_back(help);
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
        // Quit option parsing and continue with parsing of positionals
        break;
      }

      /* ignore */ scanner.Advance();

      CmdLineParseResult result=option->second.parser->Parse(scanner);

      if (result.HasError()) {
        return result;
      }

      if (option->second.stopParsing) {
        return CmdLineParseResult();
      }
    }

    for (const auto& positional : positionals) {
      CmdLineParseResult result=positional.parser->Parse(scanner);

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

    stream << appName;

    if (!options.empty()) {
      stream << " " << "[OPTION]...";
    }

    for (const auto& positional : positionals) {
      stream << " " << positional.parser->GetPositionalHint(positional.positional);
    }

    stream << std::endl;

    size_t maxNameLength=0;

    for (const auto& help : optionHelps) {
      for (const auto& argTemplate : help.argTemplates) {
        if (help.argTemplates.size()>1) {
          // for the ',' in the argument list
          maxNameLength=std::max(maxNameLength,argTemplate.length()+1);
        }
        else {
          maxNameLength=std::max(maxNameLength,argTemplate.length());
        }
      }
    }

    for (const auto& help : positionalHelps) {
      maxNameLength=std::max(maxNameLength,help.argTemplates.front().length());
    }

    if (!positionalHelps.empty()) {
      stream << std::endl;
      stream << "Mandatory arguments:" << std::endl;

      for (const auto& help : positionalHelps) {
        for (size_t t=0; t<help.argTemplates.size(); t++) {
          for (size_t i=0; i<indent; i++) {
            stream << ' ';
          }

          stream << help.argTemplates[t];

          if (t<help.argTemplates.size()-1) {
            stream << ',' << std::endl;
          }
          else {
            for (size_t i=0; i<maxNameLength-help.argTemplates[t].length(); i++) {
              stream << ' ';
            }

            stream << ' ' << help.helpString << std::endl;
          }
        }
      }
    }

    if (!optionHelps.empty()) {
      stream << std::endl;
      stream << "Options:" << std::endl;
      for (const auto& help : optionHelps) {
        for (size_t t=0; t<help.argTemplates.size(); t++) {
          for (size_t i=0; i<indent; i++) {
            stream << ' ';
          }

          stream << help.argTemplates[t];

          if (t<help.argTemplates.size()-1) {
            stream << ',' << std::endl;
          }
          else {
            for (size_t i=0; i<maxNameLength-help.argTemplates[t].length(); i++) {
              stream << ' ';
            }

            stream << ' ' << help.helpString << std::endl;
          }
        }
      }
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
