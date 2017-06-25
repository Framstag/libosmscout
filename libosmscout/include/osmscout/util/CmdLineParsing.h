#ifndef OSMSCOUT_UTIL_CMDLINEPARSING_H
#define OSMSCOUT_UTIL_CMDLINEPARSING_H

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

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <osmscout/private/CoreImportExport.h>

#include <osmscout/GeoCoord.h>

#include <osmscout/util/String.h>

namespace osmscout {

  class OSMSCOUT_API CmdLineScanner
  {
  private:
    std::vector<std::string> arguments;
    size_t                   nextArg;

  public:
    CmdLineScanner(int argc, char* argv[]);

    CmdLineScanner(const std::vector<std::string>& arguments);

    bool HasNextArg() const;
    std::string PeakNextArg() const;
    std::string Advance();
    std::string GetCurrentArg() const;
  };


  class OSMSCOUT_API CmdLineParseResult
  {
  private:
    bool        hasError;
    std::string errorDescription;

  public:
    CmdLineParseResult();
    CmdLineParseResult(const std::string& errorDescription);

    bool Success() const;
    bool HasError() const;

    std::string GetErrorDescription() const;
  };

  class OSMSCOUT_API CmdLineArgParser
  {
  private:
    std::string argumentName;

  protected:
    std::string GetArgumentName() const;

  public:
    virtual ~CmdLineArgParser();

    void SetArgumentName(const std::string& argumentName);

    virtual std::string GetOptionHint() const = 0;
    virtual std::string GetPositionalHint(const std::string& positional) const = 0;
    virtual CmdLineParseResult Parse(CmdLineScanner& scanner) = 0;
  };

  typedef std::shared_ptr<CmdLineArgParser> CmdLineArgParserRef;

  class OSMSCOUT_API CmdLineFlagArgParser : public CmdLineArgParser
  {
  public:
    typedef std::function<void(const bool&)> SetterFunction;

  private:
     SetterFunction setter;

  public:
    CmdLineFlagArgParser(SetterFunction&& setter);

    std::string GetOptionHint() const;
    std::string GetPositionalHint(const std::string& positional) const;

    CmdLineParseResult Parse(CmdLineScanner& scanner);
  };

  class OSMSCOUT_API CmdLineBoolArgParser : public CmdLineArgParser
  {
  public:
    typedef std::function<void(const bool&)> SetterFunction;

  private:
    SetterFunction setter;

  public:
    CmdLineBoolArgParser(SetterFunction&& setter);

    std::string GetOptionHint() const;
    std::string GetPositionalHint(const std::string& positional) const;

    CmdLineParseResult Parse(CmdLineScanner& scanner);
  };

  class OSMSCOUT_API CmdLineStringArgParser : public CmdLineArgParser
  {
  public:
    typedef std::function<void(const std::string&)> SetterFunction;

  private:
    SetterFunction setter;

  public:
    CmdLineStringArgParser(SetterFunction&& setter);

    std::string GetOptionHint() const;
    std::string GetPositionalHint(const std::string& positional) const;

    CmdLineParseResult Parse(CmdLineScanner& scanner);
  };

  class OSMSCOUT_API CmdLineStringListArgParser : public CmdLineArgParser
  {
  public:
    typedef std::function<void(const std::string&)> AppendFunction;

  private:
    AppendFunction appender;

  public:
    CmdLineStringListArgParser(AppendFunction&& appender);

    std::string GetOptionHint() const;
    std::string GetPositionalHint(const std::string& positional) const;

    CmdLineParseResult Parse(CmdLineScanner& scanner);
  };

  template<typename N>
  class CmdLineNumberArgParser : public CmdLineArgParser
  {
  public:
    typedef std::function<void(const N&)> SetterFunction;

  private:
    SetterFunction setter;

  public:
    CmdLineNumberArgParser(SetterFunction&& setter)
      : setter(setter)
    {
      // no code
    }

    std::string GetOptionHint() const
    {
      return "number";
    }

    std::string GetPositionalHint(const std::string& positional) const
    {
      return positional;
    }

    CmdLineParseResult Parse(CmdLineScanner& scanner)
    {
      if (!scanner.HasNextArg()) {
        return CmdLineParseResult("Missing value for number argument '"+GetArgumentName()+"'");
      }

      std::string valueString=scanner.Advance();
      N           value;

      if (StringToNumber(valueString,value)) {
        setter(value);
        return CmdLineParseResult();
      }
      else {
        return CmdLineParseResult("Value for number argument '"+GetArgumentName()+"' is not a valid number '"+valueString+"'");
      }
    }
  };

  class OSMSCOUT_API CmdLineGeoCoordArgParser : public CmdLineArgParser
  {
  public:
    typedef std::function<void(const GeoCoord&)> SetterFunction;

  private:
    SetterFunction setter;

  public:
    CmdLineGeoCoordArgParser(SetterFunction&& setter);

    std::string GetOptionHint() const;
    std::string GetPositionalHint(const std::string& positional) const;

    CmdLineParseResult Parse(CmdLineScanner& scanner);
  };

  template<class ...Args>
  CmdLineArgParserRef CmdLineFlag(Args&& ...args)
  {
    return std::make_shared<CmdLineFlagArgParser>(std::forward<Args>(args)...);
  }

  template<class ...Args>
  CmdLineArgParserRef CmdLineBoolOption(Args&& ...args)
  {
    return std::make_shared<CmdLineBoolArgParser>(std::forward<Args>(args)...);
  }

  template<class ...Args>
  CmdLineArgParserRef CmdLineStringOption(Args&& ...args)
  {
    return std::make_shared<CmdLineStringArgParser>(std::forward<Args>(args)...);
  }

  template<class ...Args>
  CmdLineArgParserRef CmdLineStringListOption(Args&& ...args)
  {
    return std::make_shared<CmdLineStringListArgParser>(std::forward<Args>(args)...);
  }

  template<class T, class ...Args>
  CmdLineArgParserRef CmdLineNumberOption(Args&& ...args)
  {
    return std::make_shared<CmdLineNumberArgParser<T>>(std::forward<Args>(args)...);
  }

  template<class ...Args>
  CmdLineArgParserRef CmdLineIntOption(Args&& ...args)
  {
    return std::make_shared<CmdLineNumberArgParser<int>>(std::forward<Args>(args)...);
  }

  template<class ...Args>
  CmdLineArgParserRef CmdLineUIntOption(Args&& ...args)
  {
    return std::make_shared<CmdLineNumberArgParser<unsigned int>>(std::forward<Args>(args)...);
  }

  template<class ...Args>
  CmdLineArgParserRef CmdLineLongOption(Args&& ...args)
  {
    return std::make_shared<CmdLineNumberArgParser<long>>(std::forward<Args>(args)...);
  }

  template<class ...Args>
  CmdLineArgParserRef CmdLineULongOption(Args&& ...args)
  {
    return std::make_shared<CmdLineNumberArgParser<unsigned long>>(std::forward<Args>(args)...);
  }

  template<class ...Args>
  CmdLineArgParserRef CmdLineSizeTOption(Args&& ...args)
  {
    return std::make_shared<CmdLineNumberArgParser<size_t>>(std::forward<Args>(args)...);
  }

  template<class ...Args>
  CmdLineArgParserRef CmdLineDoubleOption(Args&& ...args)
  {
    return std::make_shared<CmdLineNumberArgParser<double>>(std::forward<Args>(args)...);
  }

  template<class ...Args>
  CmdLineArgParserRef CmdLineGeoCoordOption(Args&& ...args)
  {
    return std::make_shared<CmdLineGeoCoordArgParser>(std::forward<Args>(args)...);
  }

  class OSMSCOUT_API CmdLineParser
  {
  private:
    struct CmdLineOption
    {
      CmdLineArgParserRef parser;
      std::string         option;
      bool                stopParsing;

      CmdLineOption(const CmdLineArgParserRef& parser,
                    const std::string& option,
                    bool stopParsing)
      : parser(parser),
        option(option),
        stopParsing(stopParsing)
      {
        // no code
      }
    };

    struct CmdLinePositional
    {
      CmdLineArgParserRef parser;
      std::string         positional;

      CmdLinePositional(const CmdLineArgParserRef& parser,
                        const std::string& positional)
        : parser(parser),
          positional(positional)
      {
        // no code
      }
    };

    struct CmdLineArgHelp
    {
      std::vector<std::string> argTemplates;
      std::string              helpString;

      CmdLineArgHelp(const std::string& argTemplate,
                     const std::string& helpString)
      : helpString(helpString)
      {
        argTemplates.push_back((argTemplate));
      }

      CmdLineArgHelp(const std::vector<std::string>& argTemplates,
                     const std::string& helpString)
      : argTemplates(argTemplates),
        helpString(helpString)
      {
        // no code
      }
    };

  private:
    std::string    appName;
    CmdLineScanner scanner;

    std::map<std::string,CmdLineOption> options;
    std::list<CmdLinePositional>        positionals;
    std::list<CmdLineArgHelp>           optionHelps;
    std::list<CmdLineArgHelp>           positionalHelps;

  public:
    CmdLineParser(const std::string& appName,
                  int argc, char* argv[]);
    CmdLineParser(const std::string& appName,
                  const std::vector<std::string>& arguments);

    void AddOption(const CmdLineArgParserRef& parser,
                   const std::string& optionName,
                   const std::string& helpString,
                   bool stopParsing=false);

    void AddOption(const CmdLineArgParserRef& parser,
                   const std::vector<std::string>& optionNames,
                   const std::string& helpString,
                   bool stopParsing=false);

    void AddPositional(const CmdLineArgParserRef& parser,
                       const std::string& helpString,
                       const std::string& argumentName);

    CmdLineParseResult Parse();

    std::string GetHelp(size_t indent=2) const;
  };

  extern OSMSCOUT_API bool ParseBoolArgument(int argc,
                                             char* argv[],
                                             int& currentIndex,
                                             bool& value);

  extern OSMSCOUT_API bool ParseStringArgument(int argc,
                                               char* argv[],
                                               int& currentIndex,
                                               std::string& value);

  extern OSMSCOUT_API bool ParseSizeTArgument(int argc,
                                              char* argv[],
                                              int& currentIndex,
                                              size_t& value);
}

#endif
