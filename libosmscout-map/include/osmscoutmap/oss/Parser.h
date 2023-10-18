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

#include <limits>
#include <list>
#include <sstream>

#include <osmscout/Pixel.h>

#include <osmscout/TypeConfig.h>

#include <osmscout/io/File.h>

#include <osmscout/util/String.h>
#include <osmscout/util/Transformation.h>
#include <osmscout/log/Logger.h>


#include <osmscoutmap/oss/Scanner.h>

#include <osmscoutmap/StyleConfig.h>

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
  bool           hasErrors=false;
  Log            log;

public:
  explicit Errors(const Log &log);
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

  osmscout::ColorPostprocessor colorPostprocessor;

  void SynErr(int n);

  void Get();
  void Expect(int n);
  bool StartOf(int s);
  void ExpectWeak(int n, int follow);
  bool WeakSeparator(int n, int syFol, int repFol);

  osmscout::Color PostprocessColor(const osmscout::Color& color) const;

public:
  Errors  *errors;

std::string                           filename;
StyleConfig&                          config;
MagnificationConverter                magnificationConverter;
bool                                  state;

enum class ValueType
{
  NO_VALUE,
  IDENT,
  STRING,
  COLOR,
  NUMBER,
  CONSTANT
};

std::string Destring(const char* str)
{
  std::string result(str);

  if (result.length()>=2 &&
      result[0]=='"' &&
      result[result.length()-1]=='"') {
    result=result.substr(1,result.length()-2);
  }

  return result;
}

bool StringToDouble(const char* string, double& value)
{
  std::istringstream buffer(string);

  buffer.imbue(std::locale::classic());

  buffer >> value;

  return !buffer.fail() && !buffer.bad() && buffer.eof();
}

size_t GetHexDigitValue(char c)
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

void AddFeatureToFilter(StyleFilter& filter,
                        const std::string& featureName,
                        const std::string& flagName,
                        TypeInfoSet& resultTypes)
{
  FeatureRef feature=config.GetTypeConfig()->GetFeature(featureName);

  if (!feature) {
    std::string e="Unknown feature '"+featureName+"'";

    SemErr(e.c_str());
    return;
  }

  size_t flagIndex=std::numeric_limits<size_t>::max();

  if (!flagName.empty() &&
      !feature->GetFlagIndex(flagName,
                             flagIndex)) {
    std::string e="Unknown feature flag '"+featureName+"."+flagName+"'";

    SemErr(e.c_str());
    return;
  }

  for (const auto& type : config.GetTypeConfig()->GetTypes()) {
    if (type->HasFeature(featureName)) {
      if (!filter.FiltersByType() ||
          filter.HasType(type)) {
        // Add type only if the filter either has no types or
        // if the type is already filtered
        resultTypes.Set(type);
      }
    }
  }

  if (!resultTypes.Empty()) {
    size_t featureFilterIndex=config.GetFeatureFilterIndex(*feature);

    filter.AddFeature(featureFilterIndex,
                      flagIndex);
  }
}



  Parser(Scanner *scanner,
         const std::string& filename,
         StyleConfig& config,
         osmscout::ColorPostprocessor colorPostprocessor=nullptr,
         const Log &log=osmscout::log);
  ~Parser();

  void SemErr(const char* msg);
  void SemWarning(const char* msg);

	void OSS();
	void IMPORTS();
	void FLAGSECTION();
	void WAYORDER();
	void CONSTSECTION();
	void SYMBOLSECTION();
	void STYLESECTION();
	void IMPORT();
	void STRING(std::string& value);
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
	void WIDTHCONSTDEF();
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
	void STYLEFILTER_SIZE(StyleFilter& filter);
	void STYLEFILTER_FEATURE_ENTRY(StyleFilter& filter, TypeInfoSet& types);
	void SIZECONDITION(SizeConditionRef& condition);
	void UMAP(double& width);
	void NODESTYLEDEF(StyleFilter filter, bool state);
	void WAYSTYLEDEF(StyleFilter filter, bool state);
	void AREASTYLEDEF(StyleFilter filter, bool state);
	void ROUTESTYLEDEF(StyleFilter filter, bool state);
	void NODETEXTSTYLE(StyleFilter filter, bool state);
	void NODEICONSTYLE(StyleFilter filter, bool state);
	void TEXTSTYLEATTR(TextPartialStyle& style);
	void ICONSTYLEATTR(IconPartialStyle& style);
	void WAYSTYLE(StyleFilter filter, bool state);
	void WAYPATHTEXTSTYLE(StyleFilter filter, bool state);
	void WAYPATHSYMBOLSTYLE(StyleFilter filter, bool state);
	void WAYSHIELDSTYLE(StyleFilter filter, bool state);
	void LINESTYLEATTR(LinePartialStyle& style);
	void ROUTEPATHTEXTSTYLE(StyleFilter filter, bool state);
	void PATHTEXTSTYLEATTR(PathTextPartialStyle& style);
	void PATHSYMBOLSTYLEATTR(PathSymbolPartialStyle& style);
	void PATHSHIELDSTYLEATTR(PathShieldPartialStyle& style);
	void AREASTYLE(StyleFilter filter, bool state);
	void AREATEXTSTYLE(StyleFilter filter, bool state);
	void AREAICONSTYLE(StyleFilter filter, bool state);
	void AREABORDERSTYLE(StyleFilter filter, bool state);
	void AREABORDERTEXTSTYLE(StyleFilter filter, bool state);
	void AREABORDERSYMBOLSTYLE(StyleFilter filter, bool state);
	void ROUTESTYLE(StyleFilter filter, bool state);
	void ATTRIBUTE(PartialStyleBase& style, const StyleDescriptor& descriptor);
	void ATTRIBUTEVALUE(PartialStyleBase& style, const StyleAttributeDescriptor& descriptor);
	void COLOR_VALUE(Color& color);
	void CONSTANT(StyleConstantRef& constant);

  void Parse();

};

} // namespace
} // namespace


#endif // !defined(COCO_PARSER_H__)

