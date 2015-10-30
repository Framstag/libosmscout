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

#include <osmscout/oss/Parser.h>

#include <sstream>

#include <osmscout/system/Assert.h>

#include <osmscout/util/String.h>

#include <osmscout/oss/Scanner.h>

namespace osmscout {
namespace oss {


void Parser::SynErr(int n)
{
  if (errDist >= minErrDist) {
    errors->SynErr(la->line, la->col, n);
  }
  errDist = 0;
}

void Parser::SemErr(const char* msg)
{
  if (errDist >= minErrDist) {
    errors->Error(t->line, t->col, msg);
  }
  errDist = 0;
}

void Parser::SemWarning(const char* msg)
{
  errors->Warning(t->line, t->col, msg);
}

void Parser::Get()
{
  for (;;) {
    t = la;
    la = scanner->Scan();
    if (la->kind <= maxT) {
      ++errDist;
      break;
    }

    if (dummyToken != t) {
      dummyToken->kind = t->kind;
      dummyToken->pos = t->pos;
      dummyToken->col = t->col;
      dummyToken->line = t->line;
      dummyToken->next = NULL;
      coco_string_delete(dummyToken->val);
      dummyToken->val = coco_string_create(t->val);
      t = dummyToken;
    }
    la = t;
  }
}

void Parser::Expect(int n)
{
  if (la->kind==n) {
    Get();
  }
  else {
    SynErr(n);
  }
}

void Parser::ExpectWeak(int n, int follow)
{
  if (la->kind == n) {
    Get();
  }
  else {
    SynErr(n);
    while (!StartOf(follow)) {
      Get();
    }
  }
}

bool Parser::WeakSeparator(int n, int syFol, int repFol)
{
  if (la->kind == n) {
    Get();
    return true;
  }
  else if (StartOf(repFol)) {
    return false;
  }
  else {
    SynErr(n);
    while (!(StartOf(syFol) || StartOf(repFol) || StartOf(0))) {
      Get();
    }
    return StartOf(syFol);
  }
}

void Parser::OSS() {
		while (!(la->kind == _EOF || la->kind == 7 /* "OSS" */)) {SynErr(90); Get();}
		Expect(7 /* "OSS" */);
		while (la->kind == 9 /* "FLAG" */) {
			FLAGBLOCK();
		}
		if (la->kind == 12 /* "ORDER" */) {
			WAYORDER();
		}
		while (la->kind == 23 /* "CONST" */) {
			CONSTBLOCK();
		}
		while (la->kind == 16 /* "SYMBO" */) {
			SYMBOLBLOCK();
		}
		while (la->kind == 29 /* "STYLE" */) {
			STYLEBLOCK();
		}
		Expect(8 /* "END" */);
}

void Parser::FLAGBLOCK() {
		while (!(la->kind == _EOF || la->kind == 9 /* "FLAG" */)) {SynErr(91); Get();}
		Expect(9 /* "FLAG" */);
		while (la->kind == _ident) {
			FLAGDEF();
		}
}

void Parser::WAYORDER() {
		while (!(la->kind == _EOF || la->kind == 12 /* "ORDER" */)) {SynErr(92); Get();}
		Expect(12 /* "ORDER" */);
		Expect(13 /* "WAYS" */);
		size_t priority=1;
		while (la->kind == 14 /* "GROUP" */) {
			WAYGROUP(priority);
			priority++;
		}
}

void Parser::CONSTBLOCK() {
		while (!(la->kind == _EOF || la->kind == 23 /* "CONST" */)) {SynErr(93); Get();}
		Expect(23 /* "CONST" */);
		while (StartOf(1)) {
			if (la->kind == 24 /* "IF" */) {
				CONSTCONDBLOCK();
			} else {
				CONSTDEF();
				Expect(11 /* ";" */);
			}
		}
}

void Parser::SYMBOLBLOCK() {
		while (!(la->kind == _EOF || la->kind == 16 /* "SYMBO" */)) {SynErr(94); Get();}
		Expect(16 /* "SYMBO" */);
		std::string name;
		
		IDENT(name);
		SymbolRef symbol=std::make_shared<Symbol>(name);
		
		while (la->kind == 17 /* "POLYGON" */ || la->kind == 20 /* "RECTANGLE" */ || la->kind == 22 /* "CIRCLE" */) {
			if (la->kind == 17 /* "POLYGON" */) {
				POLYGON(*symbol);
			} else if (la->kind == 20 /* "RECTANGLE" */) {
				RECTANGLE(*symbol);
			} else {
				CIRCLE(*symbol);
			}
		}
		if (!config.RegisterSymbol(symbol)) {
		 std::string e="Map symbol '"+symbol->GetName()+"' is already defined";
		 SemErr(e.c_str());
		}
		
}

void Parser::STYLEBLOCK() {
		Expect(29 /* "STYLE" */);
		while (StartOf(2)) {
			StyleFilter filter; 
			if (StartOf(3)) {
				STYLE(filter,true);
			} else {
				STYLECONDBLOCK(filter,true);
			}
		}
}

void Parser::FLAGDEF() {
		std::string name;
		bool        value;
		
		IDENT(name);
		Expect(10 /* "=" */);
		BOOL(value);
		Expect(11 /* ";" */);
		if (!config.HasFlag(name)) {
		 config.AddFlag(name,value);
		}
		
}

void Parser::IDENT(std::string& value) {
		Expect(_ident);
		value=t->val;
		
}

void Parser::BOOL(bool& value) {
		std::string ident; 
		Expect(_ident);
		ident=t->val;
		
		if (ident=="true") {
		 value=true;
		}
		else if (ident=="false") {
		 value=false;
		}
		else {
		 std::string e="'"+std::string(t->val)+"' is not a valid boolean value, only 'true' and 'false' are allowed";
		
		 SemErr(e.c_str());
		
		 value=false;
		}
		
}

void Parser::WAYGROUP(size_t priority) {
		while (!(la->kind == _EOF || la->kind == 14 /* "GROUP" */)) {SynErr(95); Get();}
		Expect(14 /* "GROUP" */);
		if (la->kind == _ident) {
			std::string wayTypeName;
			TypeInfoRef wayType;
			
			IDENT(wayTypeName);
			wayType=config.GetTypeConfig()->GetTypeInfo(wayTypeName);
			
			if (!wayType) {
			 std::string e="Unknown way type '"+wayTypeName+"'";
			 SemErr(e.c_str());
			}
			else if (!wayType->CanBeWay()) {
			 std::string e="Tyype '"+wayTypeName+"' is not a way type";
			 SemErr(e.c_str());
			}
			else {
			 config.SetWayPrio(wayType,
			                   priority);
			}
			
		}
		while (la->kind == 15 /* "," */) {
			std::string wayTypeName;
			TypeInfoRef wayType;
			
			Get();
			IDENT(wayTypeName);
			wayType=config.GetTypeConfig()->GetTypeInfo(wayTypeName);
			
			if (!wayType) {
			 std::string e="Unknown way type '"+wayTypeName+"'";
			 SemErr(e.c_str());
			}
			else if (!wayType->CanBeWay()) {
			 std::string e="Tyype '"+wayTypeName+"' is not a way type";
			 SemErr(e.c_str());
			}
			else {
			 config.SetWayPrio(wayType,
			                   priority);
			}
			
		}
}

void Parser::POLYGON(Symbol& symbol) {
		while (!(la->kind == _EOF || la->kind == 17 /* "POLYGON" */)) {SynErr(96); Get();}
		Expect(17 /* "POLYGON" */);
		StyleFilter         filter;
		FillPartialStyle    style;
		PolygonPrimitiveRef polygon=std::make_shared<PolygonPrimitive>(style.style);
		Coord               coord;
		
		COORD(coord);
		polygon->AddCoord(coord); 
		COORD(coord);
		polygon->AddCoord(coord); 
		while (la->kind == _number || la->kind == _double || la->kind == 35 /* "-" */) {
			COORD(coord);
			polygon->AddCoord(coord); 
		}
		while (!(la->kind == _EOF || la->kind == 18 /* "{" */)) {SynErr(97); Get();}
		Expect(18 /* "{" */);
		while (StartOf(4)) {
			FILLSTYLEATTR(style);
			ExpectWeak(11 /* ";" */, 5);
		}
		while (!(la->kind == _EOF || la->kind == 19 /* "}" */)) {SynErr(98); Get();}
		Expect(19 /* "}" */);
		symbol.AddPrimitive(polygon); 
}

void Parser::RECTANGLE(Symbol& symbol) {
		while (!(la->kind == _EOF || la->kind == 20 /* "RECTANGLE" */)) {SynErr(99); Get();}
		Expect(20 /* "RECTANGLE" */);
		StyleFilter       filter;
		FillPartialStyle  style;
		Coord             topLeft;
		double            width;
		double            height;
		
		COORD(topLeft);
		UDOUBLE(width);
		Expect(21 /* "x" */);
		UDOUBLE(height);
		while (!(la->kind == _EOF || la->kind == 18 /* "{" */)) {SynErr(100); Get();}
		Expect(18 /* "{" */);
		while (StartOf(4)) {
			FILLSTYLEATTR(style);
			ExpectWeak(11 /* ";" */, 5);
		}
		while (!(la->kind == _EOF || la->kind == 19 /* "}" */)) {SynErr(101); Get();}
		Expect(19 /* "}" */);
		symbol.AddPrimitive(std::make_shared<RectanglePrimitive>(topLeft,
		                                                        width,height,
		                                                        style.style));
		
}

void Parser::CIRCLE(Symbol& symbol) {
		while (!(la->kind == _EOF || la->kind == 22 /* "CIRCLE" */)) {SynErr(102); Get();}
		Expect(22 /* "CIRCLE" */);
		Coord             center;
		double            radius;
		StyleFilter       filter;
		FillPartialStyle  style;
		
		COORD(center);
		UDOUBLE(radius);
		while (!(la->kind == _EOF || la->kind == 18 /* "{" */)) {SynErr(103); Get();}
		Expect(18 /* "{" */);
		while (StartOf(4)) {
			FILLSTYLEATTR(style);
			ExpectWeak(11 /* ";" */, 5);
		}
		while (!(la->kind == _EOF || la->kind == 19 /* "}" */)) {SynErr(104); Get();}
		Expect(19 /* "}" */);
		symbol.AddPrimitive(std::make_shared<CirclePrimitive>(center,
		                                                     radius,
		                                                     style.style));
		
}

void Parser::COORD(Coord& coord) {
		double x;
		double y;
		
		DOUBLE(x);
		Expect(15 /* "," */);
		DOUBLE(y);
		coord=Coord(x,y); 
}

void Parser::FILLSTYLEATTR(FillPartialStyle& style) {
		switch (la->kind) {
		case 53 /* "color" */: {
			Color fillColor; 
			Get();
			Expect(42 /* ":" */);
			COLOR(fillColor);
			style.style->SetFillColor(fillColor);
			style.attributes.insert(FillStyle::attrFillColor);
			
			break;
		}
		case 64 /* "pattern" */: {
			std::string patternName; 
			Get();
			Expect(42 /* ":" */);
			STRING(patternName);
			style.style->SetPattern(patternName);
			style.attributes.insert(FillStyle::attrPattern);
			
			break;
		}
		case 65 /* "patternMinMag" */: {
			Magnification minMag; 
			Get();
			Expect(42 /* ":" */);
			MAG(minMag);
			style.style->SetPatternMinMag(minMag);
			style.attributes.insert(FillStyle::attrPatternMinMag);
			
			break;
		}
		case 66 /* "borderColor" */: {
			Color borderColor; 
			Get();
			Expect(42 /* ":" */);
			COLOR(borderColor);
			style.style->SetBorderColor(borderColor);
			style.attributes.insert(FillStyle::attrBorderColor);
			
			break;
		}
		case 67 /* "borderWidth" */: {
			double width; 
			Get();
			Expect(42 /* ":" */);
			UDISPLAYSIZE(width);
			style.style->SetBorderWidth(width);
			style.attributes.insert(FillStyle::attrBorderWidth);
			
			break;
		}
		case 68 /* "borderDash" */: {
			std::vector<double> dashes;
			double              dash;
			
			Get();
			Expect(42 /* ":" */);
			UDOUBLE(dash);
			dashes.push_back(dash); 
			while (la->kind == 15 /* "," */) {
				Get();
				UDOUBLE(dash);
				dashes.push_back(dash); 
			}
			style.style->SetBorderDashes(dashes);
			style.attributes.insert(FillStyle::attrBorderDashes);
			
			break;
		}
		default: SynErr(105); break;
		}
}

void Parser::UDOUBLE(double& value) {
		if (la->kind == _number) {
			Get();
			if (!StringToDouble(t->val,value)) {
			 std::string e="Cannot parse double '"+std::string(t->val)+"'";
			
			 SemErr(e.c_str());
			}
			
		} else if (la->kind == _double) {
			Get();
			if (!StringToDouble(t->val,value)) {
			 std::string e="Cannot parse double '"+std::string(t->val)+"'";
			
			 SemErr(e.c_str());
			}
			
		} else SynErr(106);
}

void Parser::DOUBLE(double& value) {
		bool negate=false; 
		if (la->kind == 35 /* "-" */) {
			Get();
			negate=true; 
		}
		if (la->kind == _number) {
			Get();
			if (!StringToDouble(t->val,value)) {
			 std::string e="Cannot parse double '"+std::string(t->val)+"'";
			
			 SemErr(e.c_str());
			}
			
		} else if (la->kind == _double) {
			Get();
			if (!StringToDouble(t->val,value)) {
			 std::string e="Cannot parse double '"+std::string(t->val)+"'";
			
			 SemErr(e.c_str());
			}
			
		} else SynErr(107);
		if (negate) {
		 value=-value;
		}
		
}

void Parser::CONSTCONDBLOCK() {
		bool        negate=false;
		std::string flag;
		
		Expect(24 /* "IF" */);
		if (la->kind == 25 /* "!" */) {
			Get();
			negate=true; 
		}
		IDENT(flag);
		Expect(18 /* "{" */);
		while (la->kind == 26 /* "COLOR" */ || la->kind == 27 /* "MAG" */ || la->kind == 28 /* "UINT" */) {
			CONSTDEF();
			Expect(11 /* ";" */);
		}
		Expect(19 /* "}" */);
}

void Parser::CONSTDEF() {
		if (la->kind == 26 /* "COLOR" */) {
			COLORCONSTDEF();
		} else if (la->kind == 27 /* "MAG" */) {
			MAGCONSTDEF();
		} else if (la->kind == 28 /* "UINT" */) {
			UINTCONSTDEF();
		} else SynErr(108);
}

void Parser::COLORCONSTDEF() {
		std::string      name;
		StyleConstantRef constant;
		Color            color;
		
		Expect(26 /* "COLOR" */);
		IDENT(name);
		constant=config.GetConstantByName(name);
		
		if (constant) {
		 std::string e="Constant already defined";
		
		 SemErr(e.c_str());
		}
		
		Expect(10 /* "=" */);
		COLOR(color);
		if (!errors->hasErrors) {
		 config.AddConstant(name,std::make_shared<StyleConstantColor>(color));
		}
		
}

void Parser::MAGCONSTDEF() {
		std::string      name;
		StyleConstantRef constant;
		Magnification    magnification;
		
		Expect(27 /* "MAG" */);
		IDENT(name);
		constant=config.GetConstantByName(name);
		
		if (constant) {
		 std::string e="Constant already defined";
		
		 SemErr(e.c_str());
		}
		
		Expect(10 /* "=" */);
		MAG(magnification);
		if (!errors->hasErrors) {
		 config.AddConstant(name,std::make_shared<StyleConstantMag>(magnification));
		}
		
}

void Parser::UINTCONSTDEF() {
		std::string      name;
		StyleConstantRef constant;
		size_t           value;
		
		Expect(28 /* "UINT" */);
		IDENT(name);
		constant=config.GetConstantByName(name);
		
		if (constant) {
		 std::string e="Constant already defined";
		
		 SemErr(e.c_str());
		}
		
		Expect(10 /* "=" */);
		UINT(value);
		if (!errors->hasErrors) {
		 config.AddConstant(name,std::make_shared<StyleConstantUInt>(value));
		}
		
}

void Parser::COLOR(Color& color) {
		if (la->kind == 85 /* "lighten" */) {
			double factor; 
			Get();
			Expect(86 /* "(" */);
			COLOR(color);
			Expect(15 /* "," */);
			UDOUBLE(factor);
			Expect(87 /* ")" */);
			if (factor>=0.0 && factor<=1.0) {
			 color=color.Lighten(factor);
			}
			else {
			std::string e="Factor must be in the range [0..1]";
			
			 SemErr(e.c_str());
			}
			
		} else if (la->kind == 88 /* "darken" */) {
			double factor; 
			Get();
			Expect(86 /* "(" */);
			COLOR(color);
			Expect(15 /* "," */);
			UDOUBLE(factor);
			Expect(87 /* ")" */);
			if (factor>=0.0 && factor<=1.0) {
			 color=color.Darken(factor);
			}
			else {
			std::string e="Factor must be in the range [0..1]";
			
			 SemErr(e.c_str());
			}
			
		} else if (la->kind == _color) {
			Get();
			std::string c(t->val);
			
			if (c.length()!=7 &&
			   c.length()!=9) {
			std::string e="Illegal color value";
			
			 SemErr(e.c_str());
			}
			
			if (!errors->hasErrors) {
			 ToRGBA(c,color);
			}
			
		} else if (la->kind == _variable) {
			Get();
			StyleConstantRef constant=config.GetConstantByName(t->val+1);
			
			if (!constant) {
			 std::string e="Constant not defined";
			
			 SemErr(e.c_str());
			}
			else if (dynamic_cast<StyleConstantColor*>(constant.get())==NULL) {
			 std::string e="Constant is not of type 'COLOR'";
			
			 SemErr(e.c_str());
			}
			
			if (!errors->hasErrors) {
			 StyleConstantColor* colorConstant=dynamic_cast<StyleConstantColor*>(constant.get());
			
			 color=colorConstant->GetColor();
			}
			
		} else SynErr(109);
}

void Parser::MAG(Magnification& magnification) {
		if (la->kind == _ident) {
			std::string name; 
			IDENT(name);
			if (!magnificationConverter.Convert(name,magnification)) {
			 std::string e="'"+std::string(name)+"' is not a valid magnification level";
			
			 SemErr(e.c_str());
			}
			
		} else if (la->kind == _number) {
			size_t level; 
			Get();
			if (!StringToNumber(t->val,level)) {
			 std::string e="Cannot parse number '"+std::string(t->val)+"'";
			
			 SemErr(e.c_str());
			}
			else {
			 magnification.SetLevel((uint32_t)level);
			}
			
		} else if (la->kind == _variable) {
			Get();
			StyleConstantRef constant=config.GetConstantByName(t->val+1);
			
			if (!constant) {
			 std::string e="Constant not defined";
			
			 SemErr(e.c_str());
			}
			else if (dynamic_cast<StyleConstantMag*>(constant.get())==NULL) {
			 std::string e="Variable is not of type 'MAG'";
			
			 SemErr(e.c_str());
			}
			
			if (!errors->hasErrors) {
			 StyleConstantMag* magConstant=dynamic_cast<StyleConstantMag*>(constant.get());
			
			 magnification=magConstant->GetMag();
			}
			
		} else SynErr(110);
}

void Parser::UINT(size_t& value) {
		if (la->kind == _number) {
			Get();
			if (!StringToNumber(t->val,value)) {
			 std::string e="Cannot parse number '"+std::string(t->val)+"'";
			
			 SemErr(e.c_str());
			}
			
		} else if (la->kind == _variable) {
			Get();
			StyleConstantRef constant=config.GetConstantByName(t->val+1);
			
			if (!constant) {
			 std::string e="Constant not defined";
			
			 SemErr(e.c_str());
			}
			else if (dynamic_cast<StyleConstantUInt*>(constant.get())==NULL) {
			 std::string e="Constant is not of type 'UINT'";
			
			 SemErr(e.c_str());
			}
			
			if (!errors->hasErrors) {
			 StyleConstantUInt* uintConstant=dynamic_cast<StyleConstantUInt*>(constant.get());
			
			 value=uintConstant->GetUInt();
			}
			
		} else SynErr(111);
}

void Parser::STYLE(StyleFilter filter, bool state) {
		if (la->kind == 30 /* "[" */) {
			STYLEFILTER(filter);
		}
		if (la->kind == 18 /* "{" */) {
			Get();
			while (StartOf(3)) {
				STYLE(filter,state);
			}
			Expect(19 /* "}" */);
		} else if (la->kind == 45 /* "NODE" */ || la->kind == 50 /* "WAY" */ || la->kind == 52 /* "AREA" */) {
			STYLEDEF(filter,state);
		} else SynErr(112);
}

void Parser::STYLECONDBLOCK(StyleFilter filter, bool state) {
		bool        negate=false;
		std::string flag;
		bool        newState=state;
		
		Expect(24 /* "IF" */);
		if (la->kind == 25 /* "!" */) {
			Get();
			negate=true; 
		}
		IDENT(flag);
		if (!config.HasFlag(flag)) {
		std::string e="Flag '" + flag +"' is unknown, ignoring";
		
		SemErr(e.c_str());
		}
		else {
		 newState=config.GetFlagByName(flag);
		 
		 if (negate) {
		   newState=!newState;
		 }
		}
		
		Expect(18 /* "{" */);
		while (StartOf(3)) {
			STYLE(filter,newState);
		}
		Expect(19 /* "}" */);
}

void Parser::STYLEFILTER(StyleFilter& filter) {
		Expect(30 /* "[" */);
		if (la->kind == 14 /* "GROUP" */) {
			STYLEFILTER_GROUP(filter);
		}
		if (la->kind == 32 /* "FEATURE" */) {
			STYLEFILTER_FEATURE(filter);
		}
		if (la->kind == 33 /* "PATH" */) {
			STYLEFILTER_PATH(filter);
		}
		if (la->kind == 34 /* "TYPE" */) {
			STYLEFILTER_TYPE(filter);
		}
		if (la->kind == 27 /* "MAG" */) {
			STYLEFILTER_MAG(filter);
		}
		if (la->kind == 36 /* "ONEWAY" */) {
			STYLEFILTER_ONEWAY(filter);
		}
		if (la->kind == 37 /* "BRIDGE" */) {
			STYLEFILTER_BRIDGE(filter);
		}
		if (la->kind == 38 /* "TUNNE" */) {
			STYLEFILTER_TUNNEL(filter);
		}
		if (la->kind == 39 /* "SIZE" */) {
			STYLEFILTER_SIZE(filter);
		}
		Expect(31 /* "]" */);
}

void Parser::STYLEDEF(StyleFilter filter, bool state) {
		if (la->kind == 45 /* "NODE" */) {
			NODESTYLEDEF(filter,state);
		} else if (la->kind == 50 /* "WAY" */) {
			WAYSTYLEDEF(filter,state);
		} else if (la->kind == 52 /* "AREA" */) {
			AREASTYLEDEF(filter,state);
		} else SynErr(113);
}

void Parser::STYLEFILTER_GROUP(StyleFilter& filter) {
		TypeInfoSet types;
		std::string groupName;
		
		Expect(14 /* "GROUP" */);
		IDENT(groupName);
		for (const auto& type : config.GetTypeConfig()->GetTypes()) {
		 if (type->IsInGroup(groupName)) {
		   if (filter.HasTypes() &&
		       !filter.HasType(type)) {
		     continue;
		   }
		   
		   types.Set(type);
		 }
		}
		
		while (la->kind == 15 /* "," */) {
			std::string groupName; 
			Get();
			IDENT(groupName);
			for (const auto& type : config.GetTypeConfig()->GetTypes()) {
			 if (types.IsSet(type) &&
			     !type->IsInGroup(groupName)) {
			   types.Remove(type);
			 }
			}
			
		}
		filter.SetTypes(types); 
}

void Parser::STYLEFILTER_FEATURE(StyleFilter& filter) {
		TypeInfoSet types;
		std::string featureName;
		
		Expect(32 /* "FEATURE" */);
		IDENT(featureName);
		FeatureRef feature=config.GetTypeConfig()->GetFeature(featureName);
		
		if (!feature) {
		 std::string e="Unknown feature '"+featureName+"'";
		
		 SemErr(e.c_str());
		}
		else {
		 for (const auto& type : config.GetTypeConfig()->GetTypes()) {
		    if (type->HasFeature(featureName)) {
		      if (filter.HasTypes() &&
		          !filter.HasType(type)) {
		        continue;
		      }
		       
		      types.Set(type);
		    }
		 }
		}
		
		while (la->kind == 15 /* "," */) {
			std::string featureName; 
			Get();
			IDENT(featureName);
			FeatureRef feature=config.GetTypeConfig()->GetFeature(featureName);
			
			if (!feature) {
			 std::string e="Unknown feature '"+featureName+"'";
			
			 SemErr(e.c_str());
			}
			else {
			 for (const auto& type : config.GetTypeConfig()->GetTypes()) {
			    if (types.IsSet(type) &&
			        !type->HasFeature(featureName)) {
			      types.Remove(type);
			    }
			 }
			}
			
		}
		filter.SetTypes(types); 
}

void Parser::STYLEFILTER_PATH(StyleFilter& filter) {
		TypeInfoSet types;
		
		Expect(33 /* "PATH" */);
		for (const auto& type : config.GetTypeConfig()->GetTypes()) {
		 if (type->IsPath()) {
		   if (filter.HasTypes() &&
		       !filter.HasType(type)) {
		     continue;
		   }
		    
		   types.Set(type);
		 }
		}
		
		filter.SetTypes(types);
		
}

void Parser::STYLEFILTER_TYPE(StyleFilter& filter) {
		TypeInfoSet types;
		std::string name;
		
		Expect(34 /* "TYPE" */);
		IDENT(name);
		TypeInfoRef type=config.GetTypeConfig()->GetTypeInfo(name);
		
		if (!type) {
		 std::string e="Unknown type '"+name+"'";
		
		 SemErr(e.c_str());
		}
		else if (filter.HasTypes() &&
		        !filter.HasType(type)) {
		 std::string e="Type '"+name+"' is not included by parent filter";
		
		 SemErr(e.c_str());
		}
		else {
		 types.Set(type);
		}
		
		while (la->kind == 15 /* "," */) {
			std::string name; 
			Get();
			IDENT(name);
			TypeInfoRef type=config.GetTypeConfig()->GetTypeInfo(name);
			
			if (!type) {
			 std::string e="Unknown type '"+name+"'";
			
			 SemErr(e.c_str());
			}
			else if (filter.HasTypes() &&
			        !filter.HasType(type)) {
			 std::string e="Type '"+name+"' is not included by parent filter";
			
			 SemErr(e.c_str());
			}
			else {
			 types.Set(type);
			}
			
		}
		filter.SetTypes(types); 
}

void Parser::STYLEFILTER_MAG(StyleFilter& filter) {
		Expect(27 /* "MAG" */);
		if (la->kind == _ident || la->kind == _number || la->kind == _variable) {
			Magnification magnification; 
			MAG(magnification);
			size_t level=magnification.GetLevel();
			
			if (level<filter.GetMinLevel()) {
			std::string e="The magnification interval start is not within the parent magnification range";
			
			SemErr(e.c_str());
			}
			else {
			 filter.SetMinLevel(level);
			}
			
		}
		Expect(35 /* "-" */);
		if (la->kind == _ident || la->kind == _number || la->kind == _variable) {
			Magnification magnification; 
			MAG(magnification);
			size_t level=magnification.GetLevel();
			
			if (level>filter.GetMaxLevel()) {
			std::string e="The magnification interval end is not within the parent magnification range";
			
			SemErr(e.c_str());
			}
			else {
			 filter.SetMaxLevel(level);
			}
			
		}
}

void Parser::STYLEFILTER_ONEWAY(StyleFilter& filter) {
		Expect(36 /* "ONEWAY" */);
		filter.SetOneway(true);
		
}

void Parser::STYLEFILTER_BRIDGE(StyleFilter& filter) {
		Expect(37 /* "BRIDGE" */);
		filter.SetBridge(true);
		
}

void Parser::STYLEFILTER_TUNNEL(StyleFilter& filter) {
		Expect(38 /* "TUNNE" */);
		filter.SetTunnel(true);
		
}

void Parser::STYLEFILTER_SIZE(StyleFilter& filter) {
		SizeConditionRef sizeCondition; 
		Expect(39 /* "SIZE" */);
		SIZECONDITION(sizeCondition);
		filter.SetSizeCondition(sizeCondition); 
}

void Parser::SIZECONDITION(SizeConditionRef& condition) {
		condition=std::make_shared<SizeCondition>();
		double widthInMeter;
		
		UDOUBLE(widthInMeter);
		Expect(40 /* "m" */);
		if (widthInMeter<0.0) {
		std::string e="Width must be >= 0.0";
		
		SemErr(e.c_str());
		}
		
		if (la->kind == _number || la->kind == _double || la->kind == 42 /* ":" */) {
			if (la->kind == _number || la->kind == _double) {
				double minMM; 
				UDOUBLE(minMM);
				Expect(41 /* "mm" */);
				if (widthInMeter>0.0) {
				 condition->SetMinMM(minMM/widthInMeter);
				}
				
			}
			Expect(42 /* ":" */);
			if (la->kind == _number || la->kind == _double) {
				double minPx; 
				UDOUBLE(minPx);
				Expect(43 /* "px" */);
				if (widthInMeter>0.0) {
				 condition->SetMinPx(minPx/widthInMeter);
				}
				
			}
		}
		Expect(44 /* "<" */);
		if (la->kind == _number || la->kind == _double || la->kind == 42 /* ":" */) {
			if (la->kind == _number || la->kind == _double) {
				double maxMM; 
				UDOUBLE(maxMM);
				Expect(41 /* "mm" */);
				if (widthInMeter>0.0) {
				 condition->SetMaxMM(maxMM/widthInMeter);
				}
				
			}
			Expect(42 /* ":" */);
			if (la->kind == _number || la->kind == _double) {
				double maxPx; 
				UDOUBLE(maxPx);
				Expect(43 /* "px" */);
				if (widthInMeter>0.0) {
				 condition->SetMaxPx(maxPx/widthInMeter);
				}
				
			}
		}
}

void Parser::NODESTYLEDEF(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 45 /* "NODE" */)) {SynErr(114); Get();}
		Expect(45 /* "NODE" */);
		Expect(46 /* "." */);
		if (la->kind == 47 /* "TEXT" */) {
			NODETEXTSTYLE(filter,state);
		} else if (la->kind == 49 /* "ICON" */) {
			NODEICONSTYLE(filter,state);
		} else SynErr(115);
}

void Parser::WAYSTYLEDEF(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 50 /* "WAY" */)) {SynErr(116); Get();}
		Expect(50 /* "WAY" */);
		if (la->kind == 18 /* "{" */ || la->kind == 48 /* "#" */) {
			WAYSTYLE(filter,state);
		} else if (la->kind == 46 /* "." */) {
			Get();
			if (la->kind == 47 /* "TEXT" */) {
				WAYPATHTEXTSTYLE(filter,state);
			} else if (la->kind == 16 /* "SYMBO" */) {
				WAYPATHSYMBOLSTYLE(filter,state);
			} else if (la->kind == 51 /* "SHIELD" */) {
				WAYSHIELDSTYLE(filter,state);
			} else SynErr(117);
		} else SynErr(118);
}

void Parser::AREASTYLEDEF(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 52 /* "AREA" */)) {SynErr(119); Get();}
		Expect(52 /* "AREA" */);
		if (la->kind == 18 /* "{" */) {
			AREASTYLE(filter,state);
		} else if (la->kind == 46 /* "." */) {
			Get();
			if (la->kind == 47 /* "TEXT" */) {
				AREATEXTSTYLE(filter,state);
			} else if (la->kind == 49 /* "ICON" */) {
				AREAICONSTYLE(filter,state);
			} else SynErr(120);
		} else SynErr(121);
}

void Parser::NODETEXTSTYLE(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 47 /* "TEXT" */)) {SynErr(122); Get();}
		Expect(47 /* "TEXT" */);
		TextPartialStyle style;
		std::string      slot;
		
		if (la->kind == 48 /* "#" */) {
			Get();
			IDENT(slot);
			style.style->SetSlot(slot); 
		}
		while (!(la->kind == _EOF || la->kind == 18 /* "{" */)) {SynErr(123); Get();}
		Expect(18 /* "{" */);
		while (StartOf(6)) {
			TEXTSTYLEATTR(style);
			ExpectWeak(11 /* ";" */, 7);
		}
		while (!(la->kind == _EOF || la->kind == 19 /* "}" */)) {SynErr(124); Get();}
		Expect(19 /* "}" */);
		if (state) {
		 config.AddNodeTextStyle(filter,style);
		}
		
}

void Parser::NODEICONSTYLE(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 49 /* "ICON" */)) {SynErr(125); Get();}
		Expect(49 /* "ICON" */);
		IconPartialStyle style;
		
		while (!(la->kind == _EOF || la->kind == 18 /* "{" */)) {SynErr(126); Get();}
		Expect(18 /* "{" */);
		while (la->kind == 74 /* "position" */ || la->kind == 77 /* "symbol" */ || la->kind == 79 /* "name" */) {
			ICONSTYLEATTR(style);
			ExpectWeak(11 /* ";" */, 8);
		}
		while (!(la->kind == _EOF || la->kind == 19 /* "}" */)) {SynErr(127); Get();}
		Expect(19 /* "}" */);
		if (state) {
		 config.AddNodeIconStyle(filter,style);
		}
		
}

void Parser::TEXTSTYLEATTR(TextPartialStyle& style) {
		switch (la->kind) {
		case 69 /* "label" */: {
			LabelProviderRef label; 
			Get();
			Expect(42 /* ":" */);
			TEXTLABEL(label);
			if (label) {
			 style.style->SetLabel(label);
			 style.attributes.insert(TextStyle::attrLabel);
			}
			
			break;
		}
		case 70 /* "style" */: {
			TextStyle::Style labelStyle; 
			Get();
			Expect(42 /* ":" */);
			LABELSTYLE(labelStyle);
			style.style->SetStyle(labelStyle);
			style.attributes.insert(TextStyle::attrStyle);
			
			break;
		}
		case 53 /* "color" */: {
			Color textColor; 
			Get();
			Expect(42 /* ":" */);
			COLOR(textColor);
			style.style->SetTextColor(textColor);
			style.attributes.insert(TextStyle::attrTextColor);
			
			break;
		}
		case 71 /* "size" */: {
			double size; 
			Get();
			Expect(42 /* ":" */);
			UDOUBLE(size);
			style.style->SetSize(size);
			style.attributes.insert(TextStyle::attrSize);
			
			break;
		}
		case 72 /* "scaleMag" */: {
			Magnification scaleMag; 
			Get();
			Expect(42 /* ":" */);
			MAG(scaleMag);
			style.style->SetScaleAndFadeMag(scaleMag);
			style.attributes.insert(TextStyle::attrScaleAndFadeMag);
			
			break;
		}
		case 73 /* "autoSize" */: {
			bool autoSize; 
			Get();
			Expect(42 /* ":" */);
			BOOL(autoSize);
			style.style->SetAutoSize(autoSize);
			style.attributes.insert(TextStyle::attrAutoSize);
			
			break;
		}
		case 63 /* "priority" */: {
			size_t priority; 
			Get();
			Expect(42 /* ":" */);
			UINT(priority);
			style.style->SetPriority((uint8_t)priority);
			style.attributes.insert(TextStyle::attrPriority);
			
			break;
		}
		case 74 /* "position" */: {
			size_t position; 
			Get();
			Expect(42 /* ":" */);
			UINT(position);
			style.style->SetPosition(position);
			style.attributes.insert(TextStyle::attrPosition);
			
			break;
		}
		default: SynErr(128); break;
		}
}

void Parser::ICONSTYLEATTR(IconPartialStyle& style) {
		if (la->kind == 77 /* "symbol" */) {
			std::string name;
			SymbolRef   symbol;
			
			Get();
			Expect(42 /* ":" */);
			IDENT(name);
			symbol=config.GetSymbol(name);
			
			if (!symbol) {
			 std::string e="Map symbol '"+name+"' is not defined";
			
			 SemErr(e.c_str());
			}
			else {
			 style.style->SetSymbol(symbol);
			 style.attributes.insert(IconStyle::attrSymbol);
			}
			
		} else if (la->kind == 79 /* "name" */) {
			std::string name; 
			Get();
			Expect(42 /* ":" */);
			IDENT(name);
			style.style->SetIconName(name);
			style.attributes.insert(IconStyle::attrIconName);
			
		} else if (la->kind == 74 /* "position" */) {
			size_t position; 
			Get();
			Expect(42 /* ":" */);
			UINT(position);
			style.style->SetPosition(position);
			style.attributes.insert(IconStyle::attrPosition);
			
		} else SynErr(129);
}

void Parser::WAYSTYLE(StyleFilter filter, bool state) {
		LinePartialStyle style;
		std::string      slot;
		
		if (la->kind == 48 /* "#" */) {
			Get();
			IDENT(slot);
			style.style->SetSlot(slot); 
		}
		while (!(la->kind == _EOF || la->kind == 18 /* "{" */)) {SynErr(130); Get();}
		Expect(18 /* "{" */);
		while (StartOf(9)) {
			LINESTYLEATTR(style);
			ExpectWeak(11 /* ";" */, 10);
		}
		while (!(la->kind == _EOF || la->kind == 19 /* "}" */)) {SynErr(131); Get();}
		Expect(19 /* "}" */);
		if (state) {
		 config.AddWayLineStyle(filter,style);
		}
		
}

void Parser::WAYPATHTEXTSTYLE(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 47 /* "TEXT" */)) {SynErr(132); Get();}
		Expect(47 /* "TEXT" */);
		PathTextPartialStyle style;
		
		while (!(la->kind == _EOF || la->kind == 18 /* "{" */)) {SynErr(133); Get();}
		Expect(18 /* "{" */);
		while (la->kind == 53 /* "color" */ || la->kind == 69 /* "label" */ || la->kind == 71 /* "size" */) {
			PATHTEXTSTYLEATTR(style);
			ExpectWeak(11 /* ";" */, 11);
		}
		while (!(la->kind == _EOF || la->kind == 19 /* "}" */)) {SynErr(134); Get();}
		Expect(19 /* "}" */);
		if (state) {
		 config.AddWayPathTextStyle(filter,style);
		}
		
}

void Parser::WAYPATHSYMBOLSTYLE(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 16 /* "SYMBO" */)) {SynErr(135); Get();}
		Expect(16 /* "SYMBO" */);
		PathSymbolPartialStyle style;
		
		while (!(la->kind == _EOF || la->kind == 18 /* "{" */)) {SynErr(136); Get();}
		Expect(18 /* "{" */);
		while (la->kind == 77 /* "symbol" */ || la->kind == 78 /* "symbolSpace" */) {
			PATHSYMBOLSTYLEATTR(style);
			ExpectWeak(11 /* ";" */, 12);
		}
		while (!(la->kind == _EOF || la->kind == 19 /* "}" */)) {SynErr(137); Get();}
		Expect(19 /* "}" */);
		if (state) {
		 config.AddWayPathSymbolStyle(filter,style);
		}
		
}

void Parser::WAYSHIELDSTYLE(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 51 /* "SHIELD" */)) {SynErr(138); Get();}
		Expect(51 /* "SHIELD" */);
		PathShieldPartialStyle style;
		
		while (!(la->kind == _EOF || la->kind == 18 /* "{" */)) {SynErr(139); Get();}
		Expect(18 /* "{" */);
		while (StartOf(13)) {
			PATHSHIELDSTYLEATTR(style);
			ExpectWeak(11 /* ";" */, 14);
		}
		while (!(la->kind == _EOF || la->kind == 19 /* "}" */)) {SynErr(140); Get();}
		Expect(19 /* "}" */);
		if (state) {
		 config.AddWayPathShieldStyle(filter,style);
		}
		
}

void Parser::LINESTYLEATTR(LinePartialStyle& style) {
		switch (la->kind) {
		case 53 /* "color" */: {
			Color lineColor; 
			Get();
			Expect(42 /* ":" */);
			COLOR(lineColor);
			style.style->SetLineColor(lineColor);
			style.attributes.insert(LineStyle::attrLineColor);
			
			break;
		}
		case 54 /* "dash" */: {
			std::vector<double> dashes;
			double              dash;
			
			Get();
			Expect(42 /* ":" */);
			UDOUBLE(dash);
			dashes.push_back(dash); 
			while (la->kind == 15 /* "," */) {
				Get();
				UDOUBLE(dash);
				dashes.push_back(dash); 
			}
			style.style->SetDashes(dashes);
			style.attributes.insert(LineStyle::attrDashes);
			
			break;
		}
		case 55 /* "gapColor" */: {
			Color gapColor; 
			Get();
			Expect(42 /* ":" */);
			COLOR(gapColor);
			style.style->SetGapColor(gapColor);
			style.attributes.insert(LineStyle::attrGapColor);
			
			break;
		}
		case 56 /* "displayWidth" */: {
			double displayWidth; 
			Get();
			Expect(42 /* ":" */);
			UDISPLAYSIZE(displayWidth);
			style.style->SetDisplayWidth(displayWidth);
			style.attributes.insert(LineStyle::attrDisplayWidth);
			
			break;
		}
		case 57 /* "width" */: {
			double width; 
			Get();
			Expect(42 /* ":" */);
			UMAPSIZE(width);
			style.style->SetWidth(width);
			style.attributes.insert(LineStyle::attrWidth);
			
			break;
		}
		case 58 /* "displayOffset" */: {
			double displayOffset; 
			Get();
			Expect(42 /* ":" */);
			DISPLAYSIZE(displayOffset);
			style.style->SetDisplayOffset(displayOffset);
			style.attributes.insert(LineStyle::attrDisplayOffset);
			
			break;
		}
		case 59 /* "offset" */: {
			double offset; 
			Get();
			Expect(42 /* ":" */);
			MAPSIZE(offset);
			style.style->SetOffset(offset);
			style.attributes.insert(LineStyle::attrOffset);
			
			break;
		}
		case 60 /* "cap" */: {
			LineStyle::CapStyle capStyle; 
			Get();
			Expect(42 /* ":" */);
			CAPSTYLE(capStyle);
			style.style->SetJoinCap(capStyle);
			style.attributes.insert(LineStyle::attrJoinCap);
			
			style.style->SetEndCap(capStyle);
			style.attributes.insert(LineStyle::attrEndCap);
			
			break;
		}
		case 61 /* "joinCap" */: {
			LineStyle::CapStyle capStyle; 
			Get();
			Expect(42 /* ":" */);
			CAPSTYLE(capStyle);
			style.style->SetJoinCap(capStyle);
			style.attributes.insert(LineStyle::attrJoinCap);
			
			break;
		}
		case 62 /* "endCap" */: {
			LineStyle::CapStyle capStyle; 
			Get();
			Expect(42 /* ":" */);
			CAPSTYLE(capStyle);
			style.style->SetEndCap(capStyle);
			style.attributes.insert(LineStyle::attrEndCap);
			
			break;
		}
		case 63 /* "priority" */: {
			int priority; 
			Get();
			Expect(42 /* ":" */);
			INT(priority);
			style.style->SetPriority(priority);
			style.attributes.insert(LineStyle::attrPriority);
			
			break;
		}
		default: SynErr(141); break;
		}
}

void Parser::PATHTEXTSTYLEATTR(PathTextPartialStyle& style) {
		if (la->kind == 69 /* "label" */) {
			LabelProviderRef label; 
			Get();
			Expect(42 /* ":" */);
			TEXTLABEL(label);
			if (label) {
			 style.style->SetLabel(label);
			 style.attributes.insert(PathTextStyle::attrLabel);
			}
			
		} else if (la->kind == 53 /* "color" */) {
			Color textColor; 
			Get();
			Expect(42 /* ":" */);
			COLOR(textColor);
			style.style->SetTextColor(textColor);
			style.attributes.insert(PathTextStyle::attrTextColor);
			
		} else if (la->kind == 71 /* "size" */) {
			double size; 
			Get();
			Expect(42 /* ":" */);
			UDOUBLE(size);
			style.style->SetSize(size);
			style.attributes.insert(PathTextStyle::attrSize);
			
		} else SynErr(142);
}

void Parser::PATHSYMBOLSTYLEATTR(PathSymbolPartialStyle& style) {
		if (la->kind == 77 /* "symbol" */) {
			std::string name;
			SymbolRef   symbol;
			
			Get();
			Expect(42 /* ":" */);
			IDENT(name);
			symbol=config.GetSymbol(name);
			
			if (!symbol) {
			 std::string e="Map symbol '"+name+"' is not defined";
			
			 SemErr(e.c_str());
			}
			else {
			 style.style->SetSymbol(symbol);
			 style.attributes.insert(PathSymbolStyle::attrSymbol);
			}
			
		} else if (la->kind == 78 /* "symbolSpace" */) {
			double symbolSpace; 
			Get();
			Expect(42 /* ":" */);
			UDISPLAYSIZE(symbolSpace);
			style.style->SetSymbolSpace(symbolSpace);
			style.attributes.insert(PathSymbolStyle::attrSymbolSpace);
			
		} else SynErr(143);
}

void Parser::PATHSHIELDSTYLEATTR(PathShieldPartialStyle& style) {
		switch (la->kind) {
		case 69 /* "label" */: {
			LabelProviderRef label; 
			Get();
			Expect(42 /* ":" */);
			TEXTLABEL(label);
			if (label) {
			 style.style->SetLabel(label);
			 style.attributes.insert(PathShieldStyle::attrLabel);
			}
			
			break;
		}
		case 53 /* "color" */: {
			Color textColor; 
			Get();
			Expect(42 /* ":" */);
			COLOR(textColor);
			style.style->SetTextColor(textColor);
			style.attributes.insert(PathShieldStyle::attrTextColor);
			
			break;
		}
		case 75 /* "backgroundColor" */: {
			Color bgColor; 
			Get();
			Expect(42 /* ":" */);
			COLOR(bgColor);
			style.style->SetBgColor(bgColor);
			style.attributes.insert(PathShieldStyle::attrBgColor);
			
			break;
		}
		case 66 /* "borderColor" */: {
			Color borderColor; 
			Get();
			Expect(42 /* ":" */);
			COLOR(borderColor);
			style.style->SetBorderColor(borderColor);
			style.attributes.insert(PathShieldStyle::attrBorderColor);
			
			break;
		}
		case 71 /* "size" */: {
			double size; 
			Get();
			Expect(42 /* ":" */);
			UDOUBLE(size);
			style.style->SetSize(size);
			style.attributes.insert(PathShieldStyle::attrSize);
			
			break;
		}
		case 63 /* "priority" */: {
			size_t priority; 
			Get();
			Expect(42 /* ":" */);
			UINT(priority);
			if (priority<std::numeric_limits<uint8_t>::max()) {
			 style.style->SetPriority((uint8_t)priority);
			 style.attributes.insert(PathShieldStyle::attrPriority);
			}
			else {
			 std::string e="Priority must be in the interval [0,"+
			               NumberToString(std::numeric_limits<uint8_t>::max())+"[";
			
			 SemErr(e.c_str());
			}
			
			break;
		}
		case 76 /* "shieldSpace" */: {
			double shieldSpace; 
			Get();
			Expect(42 /* ":" */);
			UDISPLAYSIZE(shieldSpace);
			style.style->SetShieldSpace(shieldSpace);
			style.attributes.insert(PathShieldStyle::attrShieldSpace);
			
			break;
		}
		default: SynErr(144); break;
		}
}

void Parser::AREASTYLE(StyleFilter filter, bool state) {
		FillPartialStyle style;
		
		while (!(la->kind == _EOF || la->kind == 18 /* "{" */)) {SynErr(145); Get();}
		Expect(18 /* "{" */);
		while (StartOf(4)) {
			FILLSTYLEATTR(style);
			ExpectWeak(11 /* ";" */, 5);
		}
		while (!(la->kind == _EOF || la->kind == 19 /* "}" */)) {SynErr(146); Get();}
		Expect(19 /* "}" */);
		if (state) {
		 config.AddAreaFillStyle(filter,style);
		}
		
}

void Parser::AREATEXTSTYLE(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 47 /* "TEXT" */)) {SynErr(147); Get();}
		Expect(47 /* "TEXT" */);
		TextPartialStyle style;
		std::string      slot;
		
		if (la->kind == 48 /* "#" */) {
			Get();
			IDENT(slot);
			style.style->SetSlot(slot); 
		}
		while (!(la->kind == _EOF || la->kind == 18 /* "{" */)) {SynErr(148); Get();}
		Expect(18 /* "{" */);
		while (StartOf(6)) {
			TEXTSTYLEATTR(style);
			ExpectWeak(11 /* ";" */, 7);
		}
		while (!(la->kind == _EOF || la->kind == 19 /* "}" */)) {SynErr(149); Get();}
		Expect(19 /* "}" */);
		if (state) {
		 config.AddAreaTextStyle(filter,style);
		}
		
}

void Parser::AREAICONSTYLE(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 49 /* "ICON" */)) {SynErr(150); Get();}
		Expect(49 /* "ICON" */);
		IconPartialStyle style;
		
		while (!(la->kind == _EOF || la->kind == 18 /* "{" */)) {SynErr(151); Get();}
		Expect(18 /* "{" */);
		while (la->kind == 74 /* "position" */ || la->kind == 77 /* "symbol" */ || la->kind == 79 /* "name" */) {
			ICONSTYLEATTR(style);
			ExpectWeak(11 /* ";" */, 8);
		}
		while (!(la->kind == _EOF || la->kind == 19 /* "}" */)) {SynErr(152); Get();}
		Expect(19 /* "}" */);
		if (state) {
		 config.AddAreaIconStyle(filter,style);
		}
		
}

void Parser::UDISPLAYSIZE(double& value) {
		UDOUBLE(value);
		Expect(41 /* "mm" */);
}

void Parser::UMAPSIZE(double& value) {
		UDOUBLE(value);
		Expect(40 /* "m" */);
}

void Parser::DISPLAYSIZE(double& value) {
		DOUBLE(value);
		Expect(41 /* "mm" */);
}

void Parser::MAPSIZE(double& value) {
		DOUBLE(value);
		Expect(40 /* "m" */);
}

void Parser::CAPSTYLE(LineStyle::CapStyle& style) {
		if (la->kind == 80 /* "butt" */) {
			Get();
			style=LineStyle::capButt; 
		} else if (la->kind == 81 /* "round" */) {
			Get();
			style=LineStyle::capRound; 
		} else if (la->kind == 82 /* "square" */) {
			Get();
			style=LineStyle::capSquare; 
		} else SynErr(153);
}

void Parser::INT(int& value) {
		bool negate=false; 
		if (la->kind == 35 /* "-" */) {
			Get();
			negate=true; 
		}
		Expect(_number);
		if (!StringToNumber(t->val,value)) {
		 std::string e="Cannot parse number '"+std::string(t->val)+"'";
		
		 SemErr(e.c_str());
		}
		
		if (negate) {
		 value=-value;
		}
		
}

void Parser::STRING(std::string& value) {
		Expect(_string);
		value=Destring(t->val);
		
}

void Parser::TEXTLABEL(LabelProviderRef& label) {
		std::string featureName;
		std::string labelName;
		
		IDENT(featureName);
		if (la->kind == 46 /* "." */) {
			Get();
			if (la->kind == _ident) {
				IDENT(labelName);
			} else if (la->kind == 79 /* "name" */) {
				Get();
				labelName="name"; 
			} else SynErr(154);
		}
		if (!labelName.empty()) {
		 FeatureRef feature;
		
		 feature=config.GetTypeConfig()->GetFeature(featureName);
		
		 if (!feature) {
		   std::string e="'"+featureName+"' is not a registered feature";
		
		   SemErr(e.c_str());
		   return;
		 }
		
		 if (!feature->HasLabel()) {
		   std::string e="'"+featureName+"' does not support labels";
		
		   SemErr(e.c_str());
		   return;
		 }
		
		 size_t labelIndex;
		
		 if (!feature->GetLabelIndex(labelName,
		                             labelIndex)) {
		   std::string e="'"+featureName+"' does not have a label named '"+labelName+"'";
		
		   SemErr(e.c_str());
		   return;
		 }
		
		 label=std::make_shared<DynamicFeatureLabelReader>(*config.GetTypeConfig(),
		                                                   featureName,
		                                                   labelName);
		}
		else {
		 label=config.GetLabelProvider(featureName);
		 
		 if (!label) {
		   std::string e="There is no label provider with name '"+featureName+"' registered";
		
		   SemErr(e.c_str());
		   return;
		 }
		}
		
}

void Parser::LABELSTYLE(TextStyle::Style& style) {
		if (la->kind == 83 /* "normal" */) {
			Get();
			style=TextStyle::normal; 
		} else if (la->kind == 84 /* "emphasize" */) {
			Get();
			style=TextStyle::emphasize; 
		} else SynErr(155);
}



void Parser::Parse()
{
  t = NULL;
  la = dummyToken = std::make_shared<Token>();
  la->val = coco_string_create("Dummy Token");
  Get();
	OSS();
	Expect(0);
}

Parser::Parser(Scanner *scanner,
               StyleConfig& config)
 : config(config)
{
	maxT = 89;

  dummyToken = NULL;
  t = la = NULL;
  minErrDist = 2;
  errDist = minErrDist;
  this->scanner = scanner;
  errors = new Errors();
}

bool Parser::StartOf(int s)
{
  const bool T = true;
  const bool x = false;

	static bool set[15][91] = {
		{T,x,x,x, x,x,x,T, x,T,x,x, T,x,T,x, T,T,T,T, T,x,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,T,x,T, x,T,T,T, T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, T,x,T,T, T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,T,x, x,x,x,x, T,x,x,x, x,x,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,T,x,x, x,x,T,x, T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,T,x, x,x,x,x, x,x,x,x, x,x,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,T,x,x, x,x,T,x, T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,T,x,x, x,x,x,x, x,x,x,x, T,T,T,T, T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{T,x,x,x, x,x,x,T, x,T,x,x, T,x,T,x, T,T,T,T, T,x,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,T,x,T, x,T,T,T, T,T,x,x, x,x,x,x, x,x,x,x, T,T,T,T, T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,T,x,x, x,x,x,x, x,x,x,T, x,x,x,x, x,T,T,T, T,T,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{T,x,x,x, x,x,x,T, x,T,x,x, T,x,T,x, T,T,T,T, T,x,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,T,x,T, x,T,T,T, T,T,x,x, x,x,x,x, x,x,x,T, x,x,x,x, x,T,T,T, T,T,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{T,x,x,x, x,x,x,T, x,T,x,x, T,x,T,x, T,T,T,T, T,x,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,T,x,T, x,T,T,T, T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,T,x, x,T,x,T, x,x,x,x, x,x,x,x, x,x,x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,T,T,T, T,T,T,T, T,T,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{T,x,x,x, x,x,x,T, x,T,x,x, T,x,T,x, T,T,T,T, T,x,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,T,x,T, x,T,T,T, T,T,T,T, T,T,T,T, T,T,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{T,x,x,x, x,x,x,T, x,T,x,x, T,x,T,x, T,T,T,T, T,x,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,T,x,T, x,T,T,T, T,T,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,T,x,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{T,x,x,x, x,x,x,T, x,T,x,x, T,x,T,x, T,T,T,T, T,x,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,T,x,T, x,T,T,T, T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,T,T,x, x,x,x,x, x,x,x,x, x,x,x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,T,x,x, x,x,x,x, x,x,x,T, x,x,T,x, x,T,x,T, x,x,x,T, T,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{T,x,x,x, x,x,x,T, x,T,x,x, T,x,T,x, T,T,T,T, T,x,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,T,x,T, x,T,T,T, T,T,x,x, x,x,x,x, x,x,x,T, x,x,T,x, x,T,x,T, x,x,x,T, T,x,x,x, x,x,x,x, x,x,x,x, x,x,x}
	};



  return set[s][la->kind];
}

Parser::~Parser()
{
  delete errors;
}

Errors::Errors()
 : hasErrors(false)
{
  // no code
}

void Errors::SynErr(int line, int col, int n)
{
  char* s;
  switch (n) {
			case 0: s = coco_string_create("EOF expected"); break;
			case 1: s = coco_string_create("ident expected"); break;
			case 2: s = coco_string_create("number expected"); break;
			case 3: s = coco_string_create("double expected"); break;
			case 4: s = coco_string_create("color expected"); break;
			case 5: s = coco_string_create("variable expected"); break;
			case 6: s = coco_string_create("string expected"); break;
			case 7: s = coco_string_create("\"OSS\" expected"); break;
			case 8: s = coco_string_create("\"END\" expected"); break;
			case 9: s = coco_string_create("\"FLAG\" expected"); break;
			case 10: s = coco_string_create("\"=\" expected"); break;
			case 11: s = coco_string_create("\";\" expected"); break;
			case 12: s = coco_string_create("\"ORDER\" expected"); break;
			case 13: s = coco_string_create("\"WAYS\" expected"); break;
			case 14: s = coco_string_create("\"GROUP\" expected"); break;
			case 15: s = coco_string_create("\",\" expected"); break;
			case 16: s = coco_string_create("\"SYMBOL\" expected"); break;
			case 17: s = coco_string_create("\"POLYGON\" expected"); break;
			case 18: s = coco_string_create("\"{\" expected"); break;
			case 19: s = coco_string_create("\"}\" expected"); break;
			case 20: s = coco_string_create("\"RECTANGLE\" expected"); break;
			case 21: s = coco_string_create("\"x\" expected"); break;
			case 22: s = coco_string_create("\"CIRCLE\" expected"); break;
			case 23: s = coco_string_create("\"CONST\" expected"); break;
			case 24: s = coco_string_create("\"IF\" expected"); break;
			case 25: s = coco_string_create("\"!\" expected"); break;
			case 26: s = coco_string_create("\"COLOR\" expected"); break;
			case 27: s = coco_string_create("\"MAG\" expected"); break;
			case 28: s = coco_string_create("\"UINT\" expected"); break;
			case 29: s = coco_string_create("\"STYLE\" expected"); break;
			case 30: s = coco_string_create("\"[\" expected"); break;
			case 31: s = coco_string_create("\"]\" expected"); break;
			case 32: s = coco_string_create("\"FEATURE\" expected"); break;
			case 33: s = coco_string_create("\"PATH\" expected"); break;
			case 34: s = coco_string_create("\"TYPE\" expected"); break;
			case 35: s = coco_string_create("\"-\" expected"); break;
			case 36: s = coco_string_create("\"ONEWAY\" expected"); break;
			case 37: s = coco_string_create("\"BRIDGE\" expected"); break;
			case 38: s = coco_string_create("\"TUNNEL\" expected"); break;
			case 39: s = coco_string_create("\"SIZE\" expected"); break;
			case 40: s = coco_string_create("\"m\" expected"); break;
			case 41: s = coco_string_create("\"mm\" expected"); break;
			case 42: s = coco_string_create("\":\" expected"); break;
			case 43: s = coco_string_create("\"px\" expected"); break;
			case 44: s = coco_string_create("\"<\" expected"); break;
			case 45: s = coco_string_create("\"NODE\" expected"); break;
			case 46: s = coco_string_create("\".\" expected"); break;
			case 47: s = coco_string_create("\"TEXT\" expected"); break;
			case 48: s = coco_string_create("\"#\" expected"); break;
			case 49: s = coco_string_create("\"ICON\" expected"); break;
			case 50: s = coco_string_create("\"WAY\" expected"); break;
			case 51: s = coco_string_create("\"SHIELD\" expected"); break;
			case 52: s = coco_string_create("\"AREA\" expected"); break;
			case 53: s = coco_string_create("\"color\" expected"); break;
			case 54: s = coco_string_create("\"dash\" expected"); break;
			case 55: s = coco_string_create("\"gapColor\" expected"); break;
			case 56: s = coco_string_create("\"displayWidth\" expected"); break;
			case 57: s = coco_string_create("\"width\" expected"); break;
			case 58: s = coco_string_create("\"displayOffset\" expected"); break;
			case 59: s = coco_string_create("\"offset\" expected"); break;
			case 60: s = coco_string_create("\"cap\" expected"); break;
			case 61: s = coco_string_create("\"joinCap\" expected"); break;
			case 62: s = coco_string_create("\"endCap\" expected"); break;
			case 63: s = coco_string_create("\"priority\" expected"); break;
			case 64: s = coco_string_create("\"pattern\" expected"); break;
			case 65: s = coco_string_create("\"patternMinMag\" expected"); break;
			case 66: s = coco_string_create("\"borderColor\" expected"); break;
			case 67: s = coco_string_create("\"borderWidth\" expected"); break;
			case 68: s = coco_string_create("\"borderDash\" expected"); break;
			case 69: s = coco_string_create("\"label\" expected"); break;
			case 70: s = coco_string_create("\"style\" expected"); break;
			case 71: s = coco_string_create("\"size\" expected"); break;
			case 72: s = coco_string_create("\"scaleMag\" expected"); break;
			case 73: s = coco_string_create("\"autoSize\" expected"); break;
			case 74: s = coco_string_create("\"position\" expected"); break;
			case 75: s = coco_string_create("\"backgroundColor\" expected"); break;
			case 76: s = coco_string_create("\"shieldSpace\" expected"); break;
			case 77: s = coco_string_create("\"symbol\" expected"); break;
			case 78: s = coco_string_create("\"symbolSpace\" expected"); break;
			case 79: s = coco_string_create("\"name\" expected"); break;
			case 80: s = coco_string_create("\"butt\" expected"); break;
			case 81: s = coco_string_create("\"round\" expected"); break;
			case 82: s = coco_string_create("\"square\" expected"); break;
			case 83: s = coco_string_create("\"normal\" expected"); break;
			case 84: s = coco_string_create("\"emphasize\" expected"); break;
			case 85: s = coco_string_create("\"lighten\" expected"); break;
			case 86: s = coco_string_create("\"(\" expected"); break;
			case 87: s = coco_string_create("\")\" expected"); break;
			case 88: s = coco_string_create("\"darken\" expected"); break;
			case 89: s = coco_string_create("??? expected"); break;
			case 90: s = coco_string_create("this symbol not expected in OSS"); break;
			case 91: s = coco_string_create("this symbol not expected in FLAGBLOCK"); break;
			case 92: s = coco_string_create("this symbol not expected in WAYORDER"); break;
			case 93: s = coco_string_create("this symbol not expected in CONSTBLOCK"); break;
			case 94: s = coco_string_create("this symbol not expected in SYMBOLBLOCK"); break;
			case 95: s = coco_string_create("this symbol not expected in WAYGROUP"); break;
			case 96: s = coco_string_create("this symbol not expected in POLYGON"); break;
			case 97: s = coco_string_create("this symbol not expected in POLYGON"); break;
			case 98: s = coco_string_create("this symbol not expected in POLYGON"); break;
			case 99: s = coco_string_create("this symbol not expected in RECTANGLE"); break;
			case 100: s = coco_string_create("this symbol not expected in RECTANGLE"); break;
			case 101: s = coco_string_create("this symbol not expected in RECTANGLE"); break;
			case 102: s = coco_string_create("this symbol not expected in CIRCLE"); break;
			case 103: s = coco_string_create("this symbol not expected in CIRCLE"); break;
			case 104: s = coco_string_create("this symbol not expected in CIRCLE"); break;
			case 105: s = coco_string_create("invalid FILLSTYLEATTR"); break;
			case 106: s = coco_string_create("invalid UDOUBLE"); break;
			case 107: s = coco_string_create("invalid DOUBLE"); break;
			case 108: s = coco_string_create("invalid CONSTDEF"); break;
			case 109: s = coco_string_create("invalid COLOR"); break;
			case 110: s = coco_string_create("invalid MAG"); break;
			case 111: s = coco_string_create("invalid UINT"); break;
			case 112: s = coco_string_create("invalid STYLE"); break;
			case 113: s = coco_string_create("invalid STYLEDEF"); break;
			case 114: s = coco_string_create("this symbol not expected in NODESTYLEDEF"); break;
			case 115: s = coco_string_create("invalid NODESTYLEDEF"); break;
			case 116: s = coco_string_create("this symbol not expected in WAYSTYLEDEF"); break;
			case 117: s = coco_string_create("invalid WAYSTYLEDEF"); break;
			case 118: s = coco_string_create("invalid WAYSTYLEDEF"); break;
			case 119: s = coco_string_create("this symbol not expected in AREASTYLEDEF"); break;
			case 120: s = coco_string_create("invalid AREASTYLEDEF"); break;
			case 121: s = coco_string_create("invalid AREASTYLEDEF"); break;
			case 122: s = coco_string_create("this symbol not expected in NODETEXTSTYLE"); break;
			case 123: s = coco_string_create("this symbol not expected in NODETEXTSTYLE"); break;
			case 124: s = coco_string_create("this symbol not expected in NODETEXTSTYLE"); break;
			case 125: s = coco_string_create("this symbol not expected in NODEICONSTYLE"); break;
			case 126: s = coco_string_create("this symbol not expected in NODEICONSTYLE"); break;
			case 127: s = coco_string_create("this symbol not expected in NODEICONSTYLE"); break;
			case 128: s = coco_string_create("invalid TEXTSTYLEATTR"); break;
			case 129: s = coco_string_create("invalid ICONSTYLEATTR"); break;
			case 130: s = coco_string_create("this symbol not expected in WAYSTYLE"); break;
			case 131: s = coco_string_create("this symbol not expected in WAYSTYLE"); break;
			case 132: s = coco_string_create("this symbol not expected in WAYPATHTEXTSTYLE"); break;
			case 133: s = coco_string_create("this symbol not expected in WAYPATHTEXTSTYLE"); break;
			case 134: s = coco_string_create("this symbol not expected in WAYPATHTEXTSTYLE"); break;
			case 135: s = coco_string_create("this symbol not expected in WAYPATHSYMBOLSTYLE"); break;
			case 136: s = coco_string_create("this symbol not expected in WAYPATHSYMBOLSTYLE"); break;
			case 137: s = coco_string_create("this symbol not expected in WAYPATHSYMBOLSTYLE"); break;
			case 138: s = coco_string_create("this symbol not expected in WAYSHIELDSTYLE"); break;
			case 139: s = coco_string_create("this symbol not expected in WAYSHIELDSTYLE"); break;
			case 140: s = coco_string_create("this symbol not expected in WAYSHIELDSTYLE"); break;
			case 141: s = coco_string_create("invalid LINESTYLEATTR"); break;
			case 142: s = coco_string_create("invalid PATHTEXTSTYLEATTR"); break;
			case 143: s = coco_string_create("invalid PATHSYMBOLSTYLEATTR"); break;
			case 144: s = coco_string_create("invalid PATHSHIELDSTYLEATTR"); break;
			case 145: s = coco_string_create("this symbol not expected in AREASTYLE"); break;
			case 146: s = coco_string_create("this symbol not expected in AREASTYLE"); break;
			case 147: s = coco_string_create("this symbol not expected in AREATEXTSTYLE"); break;
			case 148: s = coco_string_create("this symbol not expected in AREATEXTSTYLE"); break;
			case 149: s = coco_string_create("this symbol not expected in AREATEXTSTYLE"); break;
			case 150: s = coco_string_create("this symbol not expected in AREAICONSTYLE"); break;
			case 151: s = coco_string_create("this symbol not expected in AREAICONSTYLE"); break;
			case 152: s = coco_string_create("this symbol not expected in AREAICONSTYLE"); break;
			case 153: s = coco_string_create("invalid CAPSTYLE"); break;
			case 154: s = coco_string_create("invalid TEXTLABE"); break;
			case 155: s = coco_string_create("invalid LABELSTYLE"); break;

    default:
    {
      std::stringstream buffer;

      buffer << "error " << n;

      s = coco_string_create(buffer.str().c_str());
    }
    break;
  }

  Err error;

  error.type=Err::Symbol;
  error.line=line;
  error.column=col;
  error.text=s;

  coco_string_delete(s);

  std::cout << error.line << "," << error.column << " " << "Symbol: " << error.text << std::endl;

  errors.push_back(error);
  hasErrors=true;
}

void Errors::Error(int line, int col, const char *s)
{
  Err error;

  error.type=Err::Error;
  error.line=line;
  error.column=col;
  error.text=s;

  std::cout << error.line << "," << error.column << " " << "Error: " << error.text << std::endl;

  errors.push_back(error);
  hasErrors=true;
}

void Errors::Warning(int line, int col, const char *s)
{
  Err error;

  error.type=Err::Warning;
  error.line=line;
  error.column=col;
  error.text=s;

  std::cout << error.line << "," << error.column << " " << "Warning: " << error.text << std::endl;

  errors.push_back(error);
}

void Errors::Warning(const char *s)
{
  Err error;

  error.type=Err::Warning;
  error.line=0;
  error.column=0;
  error.text=s;

  std::cout << error.line << "," << error.column << " " << "Warning: " << error.text << std::endl;

  errors.push_back(error);
}

void Errors::Exception(const char* s)
{
  Err error;

  error.type=Err::Exception;
  error.line=0;
  error.column=0;
  error.text=s;

  std::cout << error.line << "," << error.column << " " << "Exception: " << error.text << std::endl;

  errors.push_back(error);
  hasErrors=true;
}

} // namespace
} // namespace

