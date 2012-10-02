/*
  This source is part of the libosmscout library
  Copyright (C) 2011  Tim Teulings

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


#if !defined(osmscout_oss_PARSER_H)
#define osmscout_oss_PARSER_H

#include <iostream>
#include <limits>
#include <list>
#include <sstream>

#include <osmscout/TypeConfig.h>

#include <osmscout/util/String.h>


#include <osmscout/oss/Scanner.h>

#include <osmscout/StyleConfig.h>

namespace osmscout {
namespace oss {


class Errors
{
public:
  class Err
  {
  public:
    enum Type {
      Symbol,
      Error,
      Warning,
      Exception
    };

  public:
    Type        type;
    int         line;
    int         column;
    std::string text;
  };

public:
  std::list<Err> errors;
  bool           hasErrors;

public:
  Errors();
  void SynErr(int line, int col, int n);
  void Error(int line, int col, const char *s);
  void Warning(int line, int col, const char *s);
  void Warning(const char *s);
  void Exception(const char *s);

};

class Parser
{
private:
	enum {
		_EOF=0,
		_ident=1,
		_number=2,
		_double=3,
		_color=4,
		_string=5
	};
	int maxT;

  TokenRef dummyToken;
  int errDist;
  int minErrDist;

  void SynErr(int n);
  
  void Get();
  void Expect(int n);
  bool StartOf(int s);
  void ExpectWeak(int n, int follow);
  bool WeakSeparator(int n, int syFol, int repFol);

  Scanner *scanner;

  TokenRef t;  // last recognized token
  TokenRef la; // lookahead token

  StyleConfig& config;

public:
  Errors  *errors;

typedef std::list<FillStyleRef>   FillStyleList;
typedef std::list<IconStyleRef>   IconStyleList;
typedef std::list<LabelStyleRef>  LabelStyleList;
typedef std::list<LineStyleRef>   LineStyleList;
typedef std::list<SymbolStyleRef> SymbolStyleList;

inline std::string Destring(const char* str)
{
  std::string result(str);

  if (result.length()>=2 &&
      result[0]=='"' &&
      result[result.length()-1]=='"') {
    result=result.substr(1,result.length()-2);
  }

  return result;
}

inline bool StringToDouble(const char* string, double& value)
{
  std::istringstream buffer(string);

  buffer.imbue(std::locale::classic());

  buffer >> value;

  return !buffer.fail() && !buffer.bad() && buffer.eof();
}

inline size_t GetHexDigitValue(char c)
{
  if (c>='0' && c<='9') {
    return c-'0';
  }
  else if (c>='a' && c<='f') {
    return 10+(c-'a');
  }

  assert(false);
}

inline void ToRGBA(const char* str, Color& color)
{
  double r=(16*GetHexDigitValue(str[1])+GetHexDigitValue(str[2]))/255.0;
  double g=(16*GetHexDigitValue(str[3])+GetHexDigitValue(str[4]))/255.0;
  double b=(16*GetHexDigitValue(str[5])+GetHexDigitValue(str[6]))/255.0;
  double a;

  if (strlen(str)==9) {
    a=(16*GetHexDigitValue(str[7])+GetHexDigitValue(str[8]))/255.0;
  }
  else {
    a=1.0;
  }

  color=Color(r,g,b,a);
}



  Parser(Scanner *scanner,
         StyleConfig& config);
  ~Parser();
  
  void SemErr(const char* msg);
  void SemWarning(const char* msg);

	void OSS();
	void WAYORDER();
	void STYLE(StyleFilter filter);
	void WAYGROUP(size_t priority);
	void STYLEFILTER(StyleFilter& filter);
	void STYLEDEF(StyleFilter filter);
	void MAG(Mag& mag);
	void NODESTYLEDEF(StyleFilter filter);
	void WAYSTYLEDEF(StyleFilter filter);
	void AREASTYLEDEF(StyleFilter filter);
	void NODELABELSTYLE(StyleFilter filter);
	void NODEREFSTYLE(StyleFilter filter);
	void NODESYMBOLSTYLE(StyleFilter filter);
	void NODEICONSTYLE(StyleFilter filter);
	void LABELDEF(LabelStyleList& styles);
	void REFDEF(LabelStyleList& styles);
	void SYMBOLDEF(SymbolStyleList& styles);
	void ICONDEF(IconStyleList& styles);
	void WAYSTYLE(StyleFilter filter);
	void WAYLABELSTYLE(StyleFilter filter);
	void WAYREFSTYLE(StyleFilter filter);
	void LINEDEF(LineStyleList& styles);
	void AREASTYLE(StyleFilter filter);
	void AREALABELSTYLE(StyleFilter filter);
	void AREASYMBOLSTYLE(StyleFilter filter);
	void AREAICONSTYLE(StyleFilter filter);
	void FILLDEF(FillStyleList& styles);
	void COLOR(Color& color);
	void DOUBLE(double& value);
	void DISPLAYSIZE(double& value);
	void MAPSIZE(double& value);
	void CAPSTYLE(LineStyle::CapStyle& style);
	void LABELSTYLE(LabelStyle::Style& style);
	void INTEGER(size_t& value);
	void SYMBOLSTYLE(SymbolStyle::Style& style);

  void Parse();

};

} // namespace
} // namespace


#endif // !defined(COCO_PARSER_H__)

