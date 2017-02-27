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

#include <osmscout/Pixel.h>

#include <osmscout/TypeConfig.h>

#include <osmscout/util/String.h>
#include <osmscout/util/Transformation.h>


#include <osmscout/oss/Scanner.h>

#include <osmscout/StyleConfig.h>

#ifdef CONST
  #undef CONST
#endif

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
		_variable=5,
		_string=6
	};
	int maxT;

  TokenRef dummyToken;
  int      errDist;
  int      minErrDist;
  Scanner  *scanner;
  TokenRef t;  // last recognized token
  TokenRef la; // lookahead token

  void SynErr(int n);

  void Get();
  void Expect(int n);
  bool StartOf(int s);
  void ExpectWeak(int n, int follow);
  bool WeakSeparator(int n, int syFol, int repFol);

public:
  Errors  *errors;

typedef std::list<FillStyleRef>       FillStyleList;
typedef std::list<BorderStyleRef>     BorderStyleList;
typedef std::list<IconStyleRef>       IconStyleList;
typedef std::list<TextStyleRef>       TextStyleList;
typedef std::list<LineStyleRef>       LineStyleList;
typedef std::list<PathTextStyleRef>   PathTextStyleList;
typedef std::list<PathShieldStyleRef> PathShieldStyleList;

StyleConfig&                          config;
MagnificationConverter                magnificationConverter;
bool                                  state;

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
  return 0;
}

inline void ToRGBA(const std::string& str, Color& color)
{
  double r=(16*GetHexDigitValue(str[1])+GetHexDigitValue(str[2]))/255.0;
  double g=(16*GetHexDigitValue(str[3])+GetHexDigitValue(str[4]))/255.0;
  double b=(16*GetHexDigitValue(str[5])+GetHexDigitValue(str[6]))/255.0;
  double a;

  if (str.length()==9) {
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
	void FLAGSECTION();
	void WAYORDER();
	void CONSTSECTION();
	void SYMBOLSECTION();
	void STYLESECTION();
	void FLAGBLOCK(bool state);
	void FLAGDEF();
	void FLAGCONDBLOCK(bool state);
	void IFCOND(bool& state);
	void IDENT(std::string& value);
	void BOOL(bool& value);
	void WAYGROUP(size_t priority);
	void POLYGON(Symbol& symbol);
	void RECTANGLE(Symbol& symbol);
	void CIRCLE(Symbol& symbol);
	void AREAFILLSYMSTYLE(FillPartialStyle& fillStyle);
	void FILLSTYLEATTR(FillPartialStyle& style);
	void AREABORDERSYMSTYLE(BorderPartialStyle& borderStyle);
	void BORDERSTYLEATTR(BorderPartialStyle& style);
	void AREASYMBOLSTYLE(FillPartialStyle& fillStyle, BorderPartialStyle& borderStyle);
	void COORD(Vertex2D& coord);
	void UDOUBLE(double& value);
	void DOUBLE(double& value);
	void CONSTBLOCK(bool state);
	void CONSTCONDBLOCK(bool state);
	void CONSTDEF();
	void COLORCONSTDEF();
	void MAGCONSTDEF();
	void UINTCONSTDEF();
	void COLOR(Color& color);
	void MAG(Magnification& magnification);
	void UINT(size_t& value);
	void STYLEBLOCK(StyleFilter filter, bool state);
	void STYLE(StyleFilter filter, bool state);
	void STYLECONDBLOCK(StyleFilter filter, bool state);
	void STYLEFILTER(StyleFilter& filter);
	void STYLEDEF(StyleFilter filter, bool state);
	void STYLEFILTER_GROUP(StyleFilter& filter);
	void STYLEFILTER_FEATURE(StyleFilter& filter);
	void STYLEFILTER_PATH(StyleFilter& filter);
	void STYLEFILTER_TYPE(StyleFilter& filter);
	void STYLEFILTER_MAG(StyleFilter& filter);
	void STYLEFILTER_ONEWAY(StyleFilter& filter);
	void STYLEFILTER_BRIDGE(StyleFilter& filter);
	void STYLEFILTER_TUNNEL(StyleFilter& filter);
	void STYLEFILTER_SIZE(StyleFilter& filter);
	void SIZECONDITION(SizeConditionRef& condition);
	void NODESTYLEDEF(StyleFilter filter, bool state);
	void WAYSTYLEDEF(StyleFilter filter, bool state);
	void AREASTYLEDEF(StyleFilter filter, bool state);
	void NODETEXTSTYLE(StyleFilter filter, bool state);
	void NODEICONSTYLE(StyleFilter filter, bool state);
	void TEXTSTYLEATTR(TextPartialStyle& style);
	void ICONSTYLEATTR(IconPartialStyle& style);
	void WAYSTYLE(StyleFilter filter, bool state);
	void WAYPATHTEXTSTYLE(StyleFilter filter, bool state);
	void WAYPATHSYMBOLSTYLE(StyleFilter filter, bool state);
	void WAYSHIELDSTYLE(StyleFilter filter, bool state);
	void LINESTYLEATTR(LinePartialStyle& style);
	void PATHTEXTSTYLEATTR(PathTextPartialStyle& style);
	void PATHSYMBOLSTYLEATTR(PathSymbolPartialStyle& style);
	void PATHSHIELDSTYLEATTR(PathShieldPartialStyle& style);
	void AREASTYLE(StyleFilter filter, bool state);
	void AREATEXTSTYLE(StyleFilter filter, bool state);
	void AREAICONSTYLE(StyleFilter filter, bool state);
	void AREABORDERSTYLE(StyleFilter filter, bool state);
	void AREABORDERTEXTSTYLE(StyleFilter filter, bool state);
	void AREABORDERSYMBOLSTYLE(StyleFilter filter, bool state);
	void UDISPLAYSIZE(double& value);
	void UMAPSIZE(double& value);
	void DISPLAYSIZE(double& value);
	void MAPSIZE(double& value);
	void CAPSTYLE(LineStyle::CapStyle& style);
	void INT(int& value);
	void STRING(std::string& value);
	void TEXTLABEL(LabelProviderRef& label);
	void LABELSTYLE(TextStyle::Style& style);

  void Parse();

};

} // namespace
} // namespace


#endif // !defined(COCO_PARSER_H__)

