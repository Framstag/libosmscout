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
#include <osmscout/util/String.h>

#include <iostream>
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
  public:
    virtual ~CmdLineArgParser();

    virtual std::string GetArgTemplate(const std::string& arg) = 0;
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
    CmdLineFlagArgParser(SetterFunction&& setter)
    : setter(setter)
    {
      // no code
    }

    template <class C, class F>
    CmdLineFlagArgParser(C&& object, F&& setter)
      : setter(std::bind(setter,object,std::placeholders::_1))
    {
      // no code
    }

    std::string GetArgTemplate(const std::string& arg)
    {
      return arg;
    }

    CmdLineParseResult Parse(CmdLineScanner& /*scanner*/)
    {
      setter(true);

      return CmdLineParseResult();
    }
  };

  class OSMSCOUT_API CmdLineBoolArgParser : public CmdLineArgParser
  {
  public:
    typedef std::function<void(const bool&)> SetterFunction;

  private:
    SetterFunction setter;

  public:
    CmdLineBoolArgParser(SetterFunction&& setter)
      : setter(setter)
    {
      // no code
    }

    template <class C, class F>
    CmdLineBoolArgParser(C&& object, F&& setter)
      : setter(std::bind(setter,object,std::placeholders::_1))
    {
      // no code
    }

    std::string GetArgTemplate(const std::string& arg)
    {
      return arg+" <true|false>";
    }

    CmdLineParseResult Parse(CmdLineScanner& scanner)
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
  };

  class OSMSCOUT_API CmdLineStringArgParser : public CmdLineArgParser
  {
  public:
    typedef std::function<void(const std::string&)> SetterFunction;

  private:
    SetterFunction setter;

  public:
    CmdLineStringArgParser(SetterFunction&& setter)
      : setter(setter)
    {
      // no code
    }

    template <class C, class F>
    CmdLineStringArgParser(C&& object, F&& setter)
      : setter(std::bind(setter,object,std::placeholders::_1))
    {
      // no code
    }

    std::string GetArgTemplate(const std::string& arg)
    {
      return arg+" <string>";
    }

    CmdLineParseResult Parse(CmdLineScanner& scanner)
    {
      if (!scanner.HasNextArg()) {
        return CmdLineParseResult("Missing value for string option '"+scanner.GetCurrentArg()+"'");
      }

      std::string value=scanner.Advance();

      setter(value);

      return CmdLineParseResult();
    }
  };

  template<typename N>
  class OSMSCOUT_API CmdLineNumberArgParser : public CmdLineArgParser
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

    template <class C, class F>
    CmdLineNumberArgParser(C&& object, F&& setter)
      : setter(std::bind(setter,object,std::placeholders::_1))
    {
      // no code
    }

    std::string GetArgTemplate(const std::string& arg)
    {
      return arg+" <number>";
    }

    CmdLineParseResult Parse(CmdLineScanner& scanner)
    {
      if (!scanner.HasNextArg()) {
        return CmdLineParseResult("Missing value for number option '"+scanner.GetCurrentArg()+"'");
      }

      std::string valueString=scanner.Advance();
      N           value;

      if (StringToNumber(valueString,value)) {
        setter(value);
        return CmdLineParseResult();
      }
      else {
        return CmdLineParseResult("Value for number option '"+scanner.GetCurrentArg()+"' is not a valid number '"+valueString+"'");
      }
    }
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

  class OSMSCOUT_API CmdLineParser
  {
  private:
    struct CmdLineArgDesc
    {
      CmdLineArgParserRef parser;
      std::string         helpString;
      bool                optional;
      bool                called;

      CmdLineArgDesc(const CmdLineArgParserRef& parser,
                     const std::string& helpString,
                     bool optional)
      : parser(parser),
        helpString(helpString),
        optional(optional),
        called(false)
      {
        // no code
      }
    };

    struct CmdLineArgHelp
    {
      std::string argTemplate;
      std::string helpString;

      CmdLineArgHelp(const std::string& argTemplate,
                     const std::string& helpString)
      : argTemplate(argTemplate),
        helpString(helpString)
      {
        // no code
      }
    };

  private:
    CmdLineScanner scanner;

    std::list<CmdLineArgHelp>            helps;
    std::map<std::string,CmdLineArgDesc> options;
    std::list<CmdLineArgDesc>            positionals;

  public:
    CmdLineParser(int argc, char* argv[]);
    CmdLineParser(const std::vector<std::string>& arguments);

    void AddOptionalArg(const CmdLineArgParserRef& parser,
                        const std::string& helpString,
                        const std::string& argumentName);

    void AddPositionalArg(const CmdLineArgParserRef& parser,
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
