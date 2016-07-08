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
  if (!state) {
    return;
  }

  if (errDist >= minErrDist) {
    errors->Error(t->line, t->col, msg);
  }
  errDist = 0;
}

void Parser::SemWarning(const char* msg)
{
  if (!state) {
    return;
  }

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
		while (!(la->kind == _EOF || la->kind == 7 /* "OSS" */)) {SynErr(94); Get();}
		Expect(7 /* "OSS" */);
		state=true; 
		while (la->kind == 9 /* "FLAG" */) {
			FLAGSECTION();
		}
		if (la->kind == 17 /* "ORDER" */) {
			WAYORDER();
		}
		while (la->kind == 26 /* "CONST" */) {
			CONSTSECTION();
		}
		while (la->kind == 21 /* "SYMBO" */) {
			SYMBOLSECTION();
		}
		while (la->kind == 30 /* "STYLE" */) {
			STYLESECTION();
		}
		Expect(8 /* "END" */);
}

void Parser::FLAGSECTION() {
		while (!(la->kind == _EOF || la->kind == 9 /* "FLAG" */)) {SynErr(95); Get();}
		Expect(9 /* "FLAG" */);
		FLAGBLOCK(true);
}

void Parser::WAYORDER() {
		while (!(la->kind == _EOF || la->kind == 17 /* "ORDER" */)) {SynErr(96); Get();}
		Expect(17 /* "ORDER" */);
		Expect(18 /* "WAYS" */);
		size_t priority=1;
		while (la->kind == 19 /* "GROUP" */) {
			WAYGROUP(priority);
			priority++;
		}
}

void Parser::CONSTSECTION() {
		while (!(la->kind == _EOF || la->kind == 26 /* "CONST" */)) {SynErr(97); Get();}
		Expect(26 /* "CONST" */);
		CONSTBLOCK(true);
}

void Parser::SYMBOLSECTION() {
		while (!(la->kind == _EOF || la->kind == 21 /* "SYMBO" */)) {SynErr(98); Get();}
		Expect(21 /* "SYMBO" */);
		std::string name;
		
		IDENT(name);
		SymbolRef symbol=std::make_shared<Symbol>(name);
		
		while (la->kind == 22 /* "POLYGON" */ || la->kind == 23 /* "RECTANGLE" */ || la->kind == 25 /* "CIRCLE" */) {
			if (la->kind == 22 /* "POLYGON" */) {
				POLYGON(*symbol);
			} else if (la->kind == 23 /* "RECTANGLE" */) {
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

void Parser::STYLESECTION() {
		Expect(30 /* "STYLE" */);
		StyleFilter filter; 
		STYLEBLOCK(filter,true);
}

void Parser::FLAGBLOCK(bool state) {
		while (la->kind == _ident || la->kind == 10 /* "IF" */) {
			if (la->kind == _ident) {
				FLAGDEF();
			} else {
				FLAGCONDBLOCK(state);
			}
		}
}

void Parser::FLAGDEF() {
		std::string name;
		bool        value;
		
		IDENT(name);
		Expect(15 /* "=" */);
		BOOL(value);
		if (state) {
		 if (!config.HasFlag(name)) {
		   config.AddFlag(name,value);
		 }
		}
		
		Expect(16 /* ";" */);
}

void Parser::FLAGCONDBLOCK(bool state) {
		bool newState=state;
		bool executed=false;
		
		Expect(10 /* "IF" */);
		IFCOND(newState);
		Expect(11 /* "{" */);
		this->state=state && newState;
		
		FLAGBLOCK(state && newState);
		this->state=state; 
		executed=newState;
		
		Expect(12 /* "}" */);
		while (la->kind == 13 /* "ELIF" */) {
			Get();
			IFCOND(newState);
			Expect(11 /* "{" */);
			this->state=!executed && state && newState;
			
			FLAGBLOCK(!executed && state && newState);
			this->state=state; 
			executed=newState;
			
			Expect(12 /* "}" */);
		}
		if (la->kind == 14 /* "ELSE" */) {
			Get();
			Expect(11 /* "{" */);
			this->state=!executed && state;
			
			FLAGBLOCK(!executed && state);
			this->state=state; 
			
			Expect(12 /* "}" */);
		}
}

void Parser::IFCOND(bool& state) {
		std::string flag;
		bool        negate=false;
		
		if (la->kind == 92 /* "!" */) {
			Get();
			negate=true; 
		}
		IDENT(flag);
		if (!config.HasFlag(flag)) {
		 std::string e="Flag '" + flag +"' is unknown, ignoring";
		
		 SemErr(e.c_str());
		 state=false;
		}
		else {
		 state=config.GetFlagByName(flag);
		}
		
		if (negate) {
		 state=!state;
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
		while (!(la->kind == _EOF || la->kind == 19 /* "GROUP" */)) {SynErr(99); Get();}
		Expect(19 /* "GROUP" */);
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
		while (la->kind == 20 /* "," */) {
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
		while (!(la->kind == _EOF || la->kind == 22 /* "POLYGON" */)) {SynErr(100); Get();}
		Expect(22 /* "POLYGON" */);
		StyleFilter         filter;
		FillPartialStyle    style;
		PolygonPrimitiveRef polygon=std::make_shared<PolygonPrimitive>(style.style);
		Vertex2D            coord;
		
		COORD(coord);
		polygon->AddCoord(coord); 
		COORD(coord);
		polygon->AddCoord(coord); 
		while (la->kind == _number || la->kind == _double || la->kind == 36 /* "-" */) {
			COORD(coord);
			polygon->AddCoord(coord); 
		}
		while (!(la->kind == _EOF || la->kind == 11 /* "{" */)) {SynErr(101); Get();}
		Expect(11 /* "{" */);
		while (StartOf(1)) {
			FILLSTYLEATTR(style);
			ExpectWeak(16 /* ";" */, 2);
		}
		while (!(la->kind == _EOF || la->kind == 12 /* "}" */)) {SynErr(102); Get();}
		Expect(12 /* "}" */);
		symbol.AddPrimitive(polygon); 
}

void Parser::RECTANGLE(Symbol& symbol) {
		while (!(la->kind == _EOF || la->kind == 23 /* "RECTANGLE" */)) {SynErr(103); Get();}
		Expect(23 /* "RECTANGLE" */);
		StyleFilter       filter;
		FillPartialStyle  style;
		Vertex2D          topLeft;
		double            width;
		double            height;
		
		COORD(topLeft);
		UDOUBLE(width);
		Expect(24 /* "x" */);
		UDOUBLE(height);
		while (!(la->kind == _EOF || la->kind == 11 /* "{" */)) {SynErr(104); Get();}
		Expect(11 /* "{" */);
		while (StartOf(1)) {
			FILLSTYLEATTR(style);
			ExpectWeak(16 /* ";" */, 2);
		}
		while (!(la->kind == _EOF || la->kind == 12 /* "}" */)) {SynErr(105); Get();}
		Expect(12 /* "}" */);
		symbol.AddPrimitive(std::make_shared<RectanglePrimitive>(topLeft,
		                                                        width,height,
		                                                        style.style));
		
}

void Parser::CIRCLE(Symbol& symbol) {
		while (!(la->kind == _EOF || la->kind == 25 /* "CIRCLE" */)) {SynErr(106); Get();}
		Expect(25 /* "CIRCLE" */);
		Vertex2D          center;
		double            radius;
		StyleFilter       filter;
		FillPartialStyle  style;
		
		COORD(center);
		UDOUBLE(radius);
		while (!(la->kind == _EOF || la->kind == 11 /* "{" */)) {SynErr(107); Get();}
		Expect(11 /* "{" */);
		while (StartOf(1)) {
			FILLSTYLEATTR(style);
			ExpectWeak(16 /* ";" */, 2);
		}
		while (!(la->kind == _EOF || la->kind == 12 /* "}" */)) {SynErr(108); Get();}
		Expect(12 /* "}" */);
		symbol.AddPrimitive(std::make_shared<CirclePrimitive>(center,
		                                                     radius,
		                                                     style.style));
		
}

void Parser::COORD(Vertex2D& coord) {
		double x;
		double y;
		
		DOUBLE(x);
		Expect(20 /* "," */);
		DOUBLE(y);
		coord=Vertex2D(x,y); 
}

void Parser::FILLSTYLEATTR(FillPartialStyle& style) {
		switch (la->kind) {
		case 55 /* "color" */: {
			Color fillColor; 
			Get();
			Expect(43 /* ":" */);
			COLOR(fillColor);
			style.style->SetFillColor(fillColor);
			style.attributes.insert(FillStyle::attrFillColor);
			
			break;
		}
		case 67 /* "pattern" */: {
			std::string patternName; 
			Get();
			Expect(43 /* ":" */);
			STRING(patternName);
			style.style->SetPattern(patternName);
			style.attributes.insert(FillStyle::attrPattern);
			
			break;
		}
		case 68 /* "patternMinMag" */: {
			Magnification minMag; 
			Get();
			Expect(43 /* ":" */);
			MAG(minMag);
			style.style->SetPatternMinMag(minMag);
			style.attributes.insert(FillStyle::attrPatternMinMag);
			
			break;
		}
		case 69 /* "borderColor" */: {
			Color borderColor; 
			Get();
			Expect(43 /* ":" */);
			COLOR(borderColor);
			style.style->SetBorderColor(borderColor);
			style.attributes.insert(FillStyle::attrBorderColor);
			
			break;
		}
		case 70 /* "borderWidth" */: {
			double width; 
			Get();
			Expect(43 /* ":" */);
			UDISPLAYSIZE(width);
			style.style->SetBorderWidth(width);
			style.attributes.insert(FillStyle::attrBorderWidth);
			
			break;
		}
		case 71 /* "borderDash" */: {
			std::vector<double> dashes;
			double              dash;
			
			Get();
			Expect(43 /* ":" */);
			UDOUBLE(dash);
			dashes.push_back(dash); 
			while (la->kind == 20 /* "," */) {
				Get();
				UDOUBLE(dash);
				dashes.push_back(dash); 
			}
			style.style->SetBorderDashes(dashes);
			style.attributes.insert(FillStyle::attrBorderDashes);
			
			break;
		}
		default: SynErr(109); break;
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
			
		} else SynErr(110);
}

void Parser::DOUBLE(double& value) {
		bool negate=false; 
		if (la->kind == 36 /* "-" */) {
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
			
		} else SynErr(111);
		if (negate) {
		 value=-value;
		}
		
}

void Parser::CONSTBLOCK(bool state) {
		while (StartOf(3)) {
			if (la->kind == 10 /* "IF" */) {
				CONSTCONDBLOCK(state);
			} else {
				CONSTDEF();
				Expect(16 /* ";" */);
			}
		}
}

void Parser::CONSTCONDBLOCK(bool state) {
		bool newState=state;
		bool executed=false;
		
		Expect(10 /* "IF" */);
		IFCOND(newState);
		Expect(11 /* "{" */);
		this->state=state && newState;
		
		CONSTBLOCK(state && newState);
		this->state=state; 
		executed=newState;
		
		Expect(12 /* "}" */);
		while (la->kind == 13 /* "ELIF" */) {
			Get();
			IFCOND(newState);
			Expect(11 /* "{" */);
			this->state=!executed && state && newState;
			
			CONSTBLOCK(!executed && state && newState);
			this->state=state; 
			executed=newState;
			
			Expect(12 /* "}" */);
		}
		if (la->kind == 14 /* "ELSE" */) {
			Get();
			Expect(11 /* "{" */);
			this->state=!executed && state;
			
			CONSTBLOCK(!executed && state);
			this->state=state; 
			
			Expect(12 /* "}" */);
		}
}

void Parser::CONSTDEF() {
		if (la->kind == 27 /* "COLOR" */) {
			COLORCONSTDEF();
		} else if (la->kind == 28 /* "MAG" */) {
			MAGCONSTDEF();
		} else if (la->kind == 29 /* "UINT" */) {
			UINTCONSTDEF();
		} else SynErr(112);
}

void Parser::COLORCONSTDEF() {
		std::string      name;
		StyleConstantRef constant;
		Color            color;
		
		Expect(27 /* "COLOR" */);
		IDENT(name);
		Expect(15 /* "=" */);
		COLOR(color);
		if (state) {
		 constant=config.GetConstantByName(name);
		
		 if (constant) {
		   std::string e="Constant already defined";
		
		   SemErr(e.c_str());
		 }
		
		 if (!errors->hasErrors) {
		   config.AddConstant(name,std::make_shared<StyleConstantColor>(color));
		 }
		}
		
}

void Parser::MAGCONSTDEF() {
		std::string      name;
		StyleConstantRef constant;
		Magnification    magnification;
		
		Expect(28 /* "MAG" */);
		IDENT(name);
		Expect(15 /* "=" */);
		MAG(magnification);
		if (state) {
		 constant=config.GetConstantByName(name);
		
		 if (constant) {
		   std::string e="Constant already defined";
		
		   SemErr(e.c_str());
		 }
		
		 if (!errors->hasErrors) {
		   config.AddConstant(name,std::make_shared<StyleConstantMag>(magnification));
		 }
		}
		
}

void Parser::UINTCONSTDEF() {
		std::string      name;
		StyleConstantRef constant;
		size_t           value;
		
		Expect(29 /* "UINT" */);
		IDENT(name);
		Expect(15 /* "=" */);
		UINT(value);
		if (state) {
		 constant=config.GetConstantByName(name);
		
		 if (constant) {
		   std::string e="Constant already defined";
		
		   SemErr(e.c_str());
		 }
		
		 if (!errors->hasErrors) {
		   config.AddConstant(name,std::make_shared<StyleConstantUInt>(value));
		 }
		}
		
}

void Parser::COLOR(Color& color) {
		if (la->kind == 88 /* "lighten" */) {
			double factor; 
			Get();
			Expect(89 /* "(" */);
			COLOR(color);
			Expect(20 /* "," */);
			UDOUBLE(factor);
			Expect(90 /* ")" */);
			if (factor>=0.0 && factor<=1.0) {
			 color=color.Lighten(factor);
			}
			else {
			std::string e="Factor must be in the range [0..1]";
			
			 SemErr(e.c_str());
			}
			
		} else if (la->kind == 91 /* "darken" */) {
			double factor; 
			Get();
			Expect(89 /* "(" */);
			COLOR(color);
			Expect(20 /* "," */);
			UDOUBLE(factor);
			Expect(90 /* ")" */);
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
			 
			 color=Color::BLACK;
			}
			else if (dynamic_cast<StyleConstantColor*>(constant.get())==NULL) {
			 std::string e="Constant is not of type 'COLOR'";
			
			 SemErr(e.c_str());
			 
			 color=Color::BLACK;
			}
			else {
			 StyleConstantColor* colorConstant=dynamic_cast<StyleConstantColor*>(constant.get());
			
			 color=colorConstant->GetColor();
			}
			
		} else SynErr(113);
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
			
		} else SynErr(114);
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
			
		} else SynErr(115);
}

void Parser::STYLEBLOCK(StyleFilter filter, bool state) {
		while (StartOf(4)) {
			if (StartOf(5)) {
				STYLE(filter,state);
			} else {
				STYLECONDBLOCK(filter,state);
			}
		}
}

void Parser::STYLE(StyleFilter filter, bool state) {
		if (la->kind == 31 /* "[" */) {
			STYLEFILTER(filter);
		}
		if (la->kind == 11 /* "{" */) {
			Get();
			STYLEBLOCK(filter,state);
			Expect(12 /* "}" */);
		} else if (la->kind == 46 /* "NODE" */ || la->kind == 51 /* "WAY" */ || la->kind == 53 /* "AREA" */) {
			STYLEDEF(filter,state);
		} else SynErr(116);
}

void Parser::STYLECONDBLOCK(StyleFilter filter, bool state) {
		bool newState=state;
		bool executed=false;
		
		Expect(10 /* "IF" */);
		IFCOND(newState);
		Expect(11 /* "{" */);
		this->state=state && newState;
		
		STYLEBLOCK(filter,state && newState);
		this->state=state;
		executed=newState;
		
		Expect(12 /* "}" */);
		while (la->kind == 13 /* "ELIF" */) {
			Get();
			IFCOND(newState);
			Expect(11 /* "{" */);
			this->state=!executed && state && newState;
			
			STYLEBLOCK(filter,!executed && state && newState);
			this->state=state;
			executed=newState;
			
			Expect(12 /* "}" */);
		}
		if (la->kind == 14 /* "ELSE" */) {
			Get();
			Expect(11 /* "{" */);
			this->state=!executed && state;
			
			STYLEBLOCK(filter,!executed && state);
			this->state=state;
			
			Expect(12 /* "}" */);
		}
}

void Parser::STYLEFILTER(StyleFilter& filter) {
		Expect(31 /* "[" */);
		if (la->kind == 19 /* "GROUP" */) {
			STYLEFILTER_GROUP(filter);
		}
		if (la->kind == 33 /* "FEATURE" */) {
			STYLEFILTER_FEATURE(filter);
		}
		if (la->kind == 34 /* "PATH" */) {
			STYLEFILTER_PATH(filter);
		}
		if (la->kind == 35 /* "TYPE" */) {
			STYLEFILTER_TYPE(filter);
		}
		if (la->kind == 28 /* "MAG" */) {
			STYLEFILTER_MAG(filter);
		}
		if (la->kind == 37 /* "ONEWAY" */) {
			STYLEFILTER_ONEWAY(filter);
		}
		if (la->kind == 38 /* "BRIDGE" */) {
			STYLEFILTER_BRIDGE(filter);
		}
		if (la->kind == 39 /* "TUNNE" */) {
			STYLEFILTER_TUNNEL(filter);
		}
		if (la->kind == 40 /* "SIZE" */) {
			STYLEFILTER_SIZE(filter);
		}
		Expect(32 /* "]" */);
}

void Parser::STYLEDEF(StyleFilter filter, bool state) {
		if (la->kind == 46 /* "NODE" */) {
			NODESTYLEDEF(filter,state);
		} else if (la->kind == 51 /* "WAY" */) {
			WAYSTYLEDEF(filter,state);
		} else if (la->kind == 53 /* "AREA" */) {
			AREASTYLEDEF(filter,state);
		} else SynErr(117);
}

void Parser::STYLEFILTER_GROUP(StyleFilter& filter) {
		TypeInfoSet types;
		std::string groupName;
		
		Expect(19 /* "GROUP" */);
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
		
		while (la->kind == 20 /* "," */) {
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
		
		Expect(33 /* "FEATURE" */);
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
		
		while (la->kind == 20 /* "," */) {
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
		
		Expect(34 /* "PATH" */);
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
		
		Expect(35 /* "TYPE" */);
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
		
		while (la->kind == 20 /* "," */) {
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
		Expect(28 /* "MAG" */);
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
		Expect(36 /* "-" */);
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
		Expect(37 /* "ONEWAY" */);
		filter.SetOneway(true);
		
}

void Parser::STYLEFILTER_BRIDGE(StyleFilter& filter) {
		Expect(38 /* "BRIDGE" */);
		filter.SetBridge(true);
		
}

void Parser::STYLEFILTER_TUNNEL(StyleFilter& filter) {
		Expect(39 /* "TUNNE" */);
		filter.SetTunnel(true);
		
}

void Parser::STYLEFILTER_SIZE(StyleFilter& filter) {
		SizeConditionRef sizeCondition; 
		Expect(40 /* "SIZE" */);
		SIZECONDITION(sizeCondition);
		filter.SetSizeCondition(sizeCondition); 
}

void Parser::SIZECONDITION(SizeConditionRef& condition) {
		condition=std::make_shared<SizeCondition>();
		double widthInMeter;
		
		UDOUBLE(widthInMeter);
		Expect(41 /* "m" */);
		if (widthInMeter<0.0) {
		std::string e="Width must be >= 0.0";
		
		SemErr(e.c_str());
		}
		
		if (la->kind == _number || la->kind == _double || la->kind == 43 /* ":" */) {
			if (la->kind == _number || la->kind == _double) {
				double minMM; 
				UDOUBLE(minMM);
				Expect(42 /* "mm" */);
				if (widthInMeter>0.0) {
				 condition->SetMinMM(minMM/widthInMeter);
				}
				
			}
			Expect(43 /* ":" */);
			if (la->kind == _number || la->kind == _double) {
				double minPx; 
				UDOUBLE(minPx);
				Expect(44 /* "px" */);
				if (widthInMeter>0.0) {
				 condition->SetMinPx(minPx/widthInMeter);
				}
				
			}
		}
		Expect(45 /* "<" */);
		if (la->kind == _number || la->kind == _double || la->kind == 43 /* ":" */) {
			if (la->kind == _number || la->kind == _double) {
				double maxMM; 
				UDOUBLE(maxMM);
				Expect(42 /* "mm" */);
				if (widthInMeter>0.0) {
				 condition->SetMaxMM(maxMM/widthInMeter);
				}
				
			}
			Expect(43 /* ":" */);
			if (la->kind == _number || la->kind == _double) {
				double maxPx; 
				UDOUBLE(maxPx);
				Expect(44 /* "px" */);
				if (widthInMeter>0.0) {
				 condition->SetMaxPx(maxPx/widthInMeter);
				}
				
			}
		}
}

void Parser::NODESTYLEDEF(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 46 /* "NODE" */)) {SynErr(118); Get();}
		Expect(46 /* "NODE" */);
		Expect(47 /* "." */);
		if (la->kind == 48 /* "TEXT" */) {
			NODETEXTSTYLE(filter,state);
		} else if (la->kind == 50 /* "ICON" */) {
			NODEICONSTYLE(filter,state);
		} else SynErr(119);
}

void Parser::WAYSTYLEDEF(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 51 /* "WAY" */)) {SynErr(120); Get();}
		Expect(51 /* "WAY" */);
		if (la->kind == 11 /* "{" */ || la->kind == 49 /* "#" */) {
			WAYSTYLE(filter,state);
		} else if (la->kind == 47 /* "." */) {
			Get();
			if (la->kind == 48 /* "TEXT" */) {
				WAYPATHTEXTSTYLE(filter,state);
			} else if (la->kind == 21 /* "SYMBO" */) {
				WAYPATHSYMBOLSTYLE(filter,state);
			} else if (la->kind == 52 /* "SHIELD" */) {
				WAYSHIELDSTYLE(filter,state);
			} else SynErr(121);
		} else SynErr(122);
}

void Parser::AREASTYLEDEF(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 53 /* "AREA" */)) {SynErr(123); Get();}
		Expect(53 /* "AREA" */);
		if (la->kind == 11 /* "{" */) {
			AREASTYLE(filter,state);
		} else if (la->kind == 47 /* "." */) {
			Get();
			if (la->kind == 48 /* "TEXT" */) {
				AREATEXTSTYLE(filter,state);
			} else if (la->kind == 50 /* "ICON" */) {
				AREAICONSTYLE(filter,state);
			} else if (la->kind == 54 /* "BORDERTEXT" */) {
				AREABORDERTEXTSTYLE(filter,state);
			} else SynErr(124);
		} else SynErr(125);
}

void Parser::NODETEXTSTYLE(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 48 /* "TEXT" */)) {SynErr(126); Get();}
		Expect(48 /* "TEXT" */);
		TextPartialStyle style;
		std::string      slot;
		
		if (la->kind == 49 /* "#" */) {
			Get();
			IDENT(slot);
			style.style->SetSlot(slot); 
		}
		while (!(la->kind == _EOF || la->kind == 11 /* "{" */)) {SynErr(127); Get();}
		Expect(11 /* "{" */);
		while (StartOf(6)) {
			TEXTSTYLEATTR(style);
			ExpectWeak(16 /* ";" */, 7);
		}
		while (!(la->kind == _EOF || la->kind == 12 /* "}" */)) {SynErr(128); Get();}
		Expect(12 /* "}" */);
		if (state) {
		 config.AddNodeTextStyle(filter,style);
		}
		
}

void Parser::NODEICONSTYLE(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 50 /* "ICON" */)) {SynErr(129); Get();}
		Expect(50 /* "ICON" */);
		IconPartialStyle style;
		
		while (!(la->kind == _EOF || la->kind == 11 /* "{" */)) {SynErr(130); Get();}
		Expect(11 /* "{" */);
		while (la->kind == 77 /* "position" */ || la->kind == 80 /* "symbol" */ || la->kind == 82 /* "name" */) {
			ICONSTYLEATTR(style);
			ExpectWeak(16 /* ";" */, 8);
		}
		while (!(la->kind == _EOF || la->kind == 12 /* "}" */)) {SynErr(131); Get();}
		Expect(12 /* "}" */);
		if (state) {
		 config.AddNodeIconStyle(filter,style);
		}
		
}

void Parser::TEXTSTYLEATTR(TextPartialStyle& style) {
		switch (la->kind) {
		case 72 /* "label" */: {
			LabelProviderRef label; 
			Get();
			Expect(43 /* ":" */);
			TEXTLABEL(label);
			if (label) {
			 style.style->SetLabel(label);
			 style.attributes.insert(TextStyle::attrLabel);
			}
			
			break;
		}
		case 73 /* "style" */: {
			TextStyle::Style labelStyle; 
			Get();
			Expect(43 /* ":" */);
			LABELSTYLE(labelStyle);
			style.style->SetStyle(labelStyle);
			style.attributes.insert(TextStyle::attrStyle);
			
			break;
		}
		case 55 /* "color" */: {
			Color textColor; 
			Get();
			Expect(43 /* ":" */);
			COLOR(textColor);
			style.style->SetTextColor(textColor);
			style.attributes.insert(TextStyle::attrTextColor);
			
			break;
		}
		case 74 /* "size" */: {
			double size; 
			Get();
			Expect(43 /* ":" */);
			UDOUBLE(size);
			style.style->SetSize(size);
			style.attributes.insert(TextStyle::attrSize);
			
			break;
		}
		case 75 /* "scaleMag" */: {
			Magnification scaleMag; 
			Get();
			Expect(43 /* ":" */);
			MAG(scaleMag);
			style.style->SetScaleAndFadeMag(scaleMag);
			style.attributes.insert(TextStyle::attrScaleAndFadeMag);
			
			break;
		}
		case 76 /* "autoSize" */: {
			bool autoSize; 
			Get();
			Expect(43 /* ":" */);
			BOOL(autoSize);
			style.style->SetAutoSize(autoSize);
			style.attributes.insert(TextStyle::attrAutoSize);
			
			break;
		}
		case 65 /* "priority" */: {
			size_t priority; 
			Get();
			Expect(43 /* ":" */);
			UINT(priority);
			style.style->SetPriority((uint8_t)priority);
			style.attributes.insert(TextStyle::attrPriority);
			
			break;
		}
		case 77 /* "position" */: {
			size_t position; 
			Get();
			Expect(43 /* ":" */);
			UINT(position);
			style.style->SetPosition(position);
			style.attributes.insert(TextStyle::attrPosition);
			
			break;
		}
		default: SynErr(132); break;
		}
}

void Parser::ICONSTYLEATTR(IconPartialStyle& style) {
		if (la->kind == 80 /* "symbol" */) {
			std::string name;
			SymbolRef   symbol;
			
			Get();
			Expect(43 /* ":" */);
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
			
		} else if (la->kind == 82 /* "name" */) {
			std::string name; 
			Get();
			Expect(43 /* ":" */);
			IDENT(name);
			style.style->SetIconName(name);
			style.attributes.insert(IconStyle::attrIconName);
			
		} else if (la->kind == 77 /* "position" */) {
			size_t position; 
			Get();
			Expect(43 /* ":" */);
			UINT(position);
			style.style->SetPosition(position);
			style.attributes.insert(IconStyle::attrPosition);
			
		} else SynErr(133);
}

void Parser::WAYSTYLE(StyleFilter filter, bool state) {
		LinePartialStyle style;
		std::string      slot;
		
		if (la->kind == 49 /* "#" */) {
			Get();
			IDENT(slot);
			style.style->SetSlot(slot); 
		}
		while (!(la->kind == _EOF || la->kind == 11 /* "{" */)) {SynErr(134); Get();}
		Expect(11 /* "{" */);
		while (StartOf(9)) {
			LINESTYLEATTR(style);
			ExpectWeak(16 /* ";" */, 10);
		}
		while (!(la->kind == _EOF || la->kind == 12 /* "}" */)) {SynErr(135); Get();}
		Expect(12 /* "}" */);
		if (state) {
		 config.AddWayLineStyle(filter,style);
		}
		
}

void Parser::WAYPATHTEXTSTYLE(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 48 /* "TEXT" */)) {SynErr(136); Get();}
		Expect(48 /* "TEXT" */);
		PathTextPartialStyle style;
		
		while (!(la->kind == _EOF || la->kind == 11 /* "{" */)) {SynErr(137); Get();}
		Expect(11 /* "{" */);
		while (la->kind == 55 /* "color" */ || la->kind == 72 /* "label" */ || la->kind == 74 /* "size" */) {
			PATHTEXTSTYLEATTR(style);
			ExpectWeak(16 /* ";" */, 11);
		}
		while (!(la->kind == _EOF || la->kind == 12 /* "}" */)) {SynErr(138); Get();}
		Expect(12 /* "}" */);
		if (state) {
		 config.AddWayPathTextStyle(filter,style);
		}
		
}

void Parser::WAYPATHSYMBOLSTYLE(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 21 /* "SYMBO" */)) {SynErr(139); Get();}
		Expect(21 /* "SYMBO" */);
		PathSymbolPartialStyle style;
		
		while (!(la->kind == _EOF || la->kind == 11 /* "{" */)) {SynErr(140); Get();}
		Expect(11 /* "{" */);
		while (la->kind == 80 /* "symbol" */ || la->kind == 81 /* "symbolSpace" */) {
			PATHSYMBOLSTYLEATTR(style);
			ExpectWeak(16 /* ";" */, 12);
		}
		while (!(la->kind == _EOF || la->kind == 12 /* "}" */)) {SynErr(141); Get();}
		Expect(12 /* "}" */);
		if (state) {
		 config.AddWayPathSymbolStyle(filter,style);
		}
		
}

void Parser::WAYSHIELDSTYLE(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 52 /* "SHIELD" */)) {SynErr(142); Get();}
		Expect(52 /* "SHIELD" */);
		PathShieldPartialStyle style;
		
		while (!(la->kind == _EOF || la->kind == 11 /* "{" */)) {SynErr(143); Get();}
		Expect(11 /* "{" */);
		while (StartOf(13)) {
			PATHSHIELDSTYLEATTR(style);
			ExpectWeak(16 /* ";" */, 14);
		}
		while (!(la->kind == _EOF || la->kind == 12 /* "}" */)) {SynErr(144); Get();}
		Expect(12 /* "}" */);
		if (state) {
		 config.AddWayPathShieldStyle(filter,style);
		}
		
}

void Parser::LINESTYLEATTR(LinePartialStyle& style) {
		switch (la->kind) {
		case 55 /* "color" */: {
			Color lineColor; 
			Get();
			Expect(43 /* ":" */);
			COLOR(lineColor);
			style.style->SetLineColor(lineColor);
			style.attributes.insert(LineStyle::attrLineColor);
			
			break;
		}
		case 56 /* "dash" */: {
			std::vector<double> dashes;
			double              dash;
			
			Get();
			Expect(43 /* ":" */);
			UDOUBLE(dash);
			dashes.push_back(dash); 
			while (la->kind == 20 /* "," */) {
				Get();
				UDOUBLE(dash);
				dashes.push_back(dash); 
			}
			style.style->SetDashes(dashes);
			style.attributes.insert(LineStyle::attrDashes);
			
			break;
		}
		case 57 /* "gapColor" */: {
			Color gapColor; 
			Get();
			Expect(43 /* ":" */);
			COLOR(gapColor);
			style.style->SetGapColor(gapColor);
			style.attributes.insert(LineStyle::attrGapColor);
			
			break;
		}
		case 58 /* "displayWidth" */: {
			double displayWidth; 
			Get();
			Expect(43 /* ":" */);
			UDISPLAYSIZE(displayWidth);
			style.style->SetDisplayWidth(displayWidth);
			style.attributes.insert(LineStyle::attrDisplayWidth);
			
			break;
		}
		case 59 /* "width" */: {
			double width; 
			Get();
			Expect(43 /* ":" */);
			UMAPSIZE(width);
			style.style->SetWidth(width);
			style.attributes.insert(LineStyle::attrWidth);
			
			break;
		}
		case 60 /* "displayOffset" */: {
			double displayOffset; 
			Get();
			Expect(43 /* ":" */);
			DISPLAYSIZE(displayOffset);
			style.style->SetDisplayOffset(displayOffset);
			style.attributes.insert(LineStyle::attrDisplayOffset);
			
			break;
		}
		case 61 /* "offset" */: {
			double offset; 
			Get();
			Expect(43 /* ":" */);
			MAPSIZE(offset);
			style.style->SetOffset(offset);
			style.attributes.insert(LineStyle::attrOffset);
			
			break;
		}
		case 62 /* "cap" */: {
			LineStyle::CapStyle capStyle; 
			Get();
			Expect(43 /* ":" */);
			CAPSTYLE(capStyle);
			style.style->SetJoinCap(capStyle);
			style.attributes.insert(LineStyle::attrJoinCap);
			
			style.style->SetEndCap(capStyle);
			style.attributes.insert(LineStyle::attrEndCap);
			
			break;
		}
		case 63 /* "joinCap" */: {
			LineStyle::CapStyle capStyle; 
			Get();
			Expect(43 /* ":" */);
			CAPSTYLE(capStyle);
			style.style->SetJoinCap(capStyle);
			style.attributes.insert(LineStyle::attrJoinCap);
			
			break;
		}
		case 64 /* "endCap" */: {
			LineStyle::CapStyle capStyle; 
			Get();
			Expect(43 /* ":" */);
			CAPSTYLE(capStyle);
			style.style->SetEndCap(capStyle);
			style.attributes.insert(LineStyle::attrEndCap);
			
			break;
		}
		case 65 /* "priority" */: {
			int priority; 
			Get();
			Expect(43 /* ":" */);
			INT(priority);
			style.style->SetPriority(priority);
			style.attributes.insert(LineStyle::attrPriority);
			
			break;
		}
		case 66 /* "zIndex" */: {
			int zIndex; 
			Get();
			Expect(43 /* ":" */);
			INT(zIndex);
			style.style->SetZIndex(zIndex);
			style.attributes.insert(LineStyle::attrZIndex);
			
			break;
		}
		default: SynErr(145); break;
		}
}

void Parser::PATHTEXTSTYLEATTR(PathTextPartialStyle& style) {
		if (la->kind == 72 /* "label" */) {
			LabelProviderRef label; 
			Get();
			Expect(43 /* ":" */);
			TEXTLABEL(label);
			if (label) {
			 style.style->SetLabel(label);
			 style.attributes.insert(PathTextStyle::attrLabel);
			}
			
		} else if (la->kind == 55 /* "color" */) {
			Color textColor; 
			Get();
			Expect(43 /* ":" */);
			COLOR(textColor);
			style.style->SetTextColor(textColor);
			style.attributes.insert(PathTextStyle::attrTextColor);
			
		} else if (la->kind == 74 /* "size" */) {
			double size; 
			Get();
			Expect(43 /* ":" */);
			UDOUBLE(size);
			style.style->SetSize(size);
			style.attributes.insert(PathTextStyle::attrSize);
			
		} else SynErr(146);
}

void Parser::PATHSYMBOLSTYLEATTR(PathSymbolPartialStyle& style) {
		if (la->kind == 80 /* "symbol" */) {
			std::string name;
			SymbolRef   symbol;
			
			Get();
			Expect(43 /* ":" */);
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
			
		} else if (la->kind == 81 /* "symbolSpace" */) {
			double symbolSpace; 
			Get();
			Expect(43 /* ":" */);
			UDISPLAYSIZE(symbolSpace);
			style.style->SetSymbolSpace(symbolSpace);
			style.attributes.insert(PathSymbolStyle::attrSymbolSpace);
			
		} else SynErr(147);
}

void Parser::PATHSHIELDSTYLEATTR(PathShieldPartialStyle& style) {
		switch (la->kind) {
		case 72 /* "label" */: {
			LabelProviderRef label; 
			Get();
			Expect(43 /* ":" */);
			TEXTLABEL(label);
			if (label) {
			 style.style->SetLabel(label);
			 style.attributes.insert(PathShieldStyle::attrLabel);
			}
			
			break;
		}
		case 55 /* "color" */: {
			Color textColor; 
			Get();
			Expect(43 /* ":" */);
			COLOR(textColor);
			style.style->SetTextColor(textColor);
			style.attributes.insert(PathShieldStyle::attrTextColor);
			
			break;
		}
		case 78 /* "backgroundColor" */: {
			Color bgColor; 
			Get();
			Expect(43 /* ":" */);
			COLOR(bgColor);
			style.style->SetBgColor(bgColor);
			style.attributes.insert(PathShieldStyle::attrBgColor);
			
			break;
		}
		case 69 /* "borderColor" */: {
			Color borderColor; 
			Get();
			Expect(43 /* ":" */);
			COLOR(borderColor);
			style.style->SetBorderColor(borderColor);
			style.attributes.insert(PathShieldStyle::attrBorderColor);
			
			break;
		}
		case 74 /* "size" */: {
			double size; 
			Get();
			Expect(43 /* ":" */);
			UDOUBLE(size);
			style.style->SetSize(size);
			style.attributes.insert(PathShieldStyle::attrSize);
			
			break;
		}
		case 65 /* "priority" */: {
			size_t priority; 
			Get();
			Expect(43 /* ":" */);
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
		case 79 /* "shieldSpace" */: {
			double shieldSpace; 
			Get();
			Expect(43 /* ":" */);
			UDISPLAYSIZE(shieldSpace);
			style.style->SetShieldSpace(shieldSpace);
			style.attributes.insert(PathShieldStyle::attrShieldSpace);
			
			break;
		}
		default: SynErr(148); break;
		}
}

void Parser::AREASTYLE(StyleFilter filter, bool state) {
		FillPartialStyle style;
		
		while (!(la->kind == _EOF || la->kind == 11 /* "{" */)) {SynErr(149); Get();}
		Expect(11 /* "{" */);
		while (StartOf(1)) {
			FILLSTYLEATTR(style);
			ExpectWeak(16 /* ";" */, 2);
		}
		while (!(la->kind == _EOF || la->kind == 12 /* "}" */)) {SynErr(150); Get();}
		Expect(12 /* "}" */);
		if (state) {
		 config.AddAreaFillStyle(filter,style);
		}
		
}

void Parser::AREATEXTSTYLE(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 48 /* "TEXT" */)) {SynErr(151); Get();}
		Expect(48 /* "TEXT" */);
		TextPartialStyle style;
		std::string      slot;
		
		if (la->kind == 49 /* "#" */) {
			Get();
			IDENT(slot);
			style.style->SetSlot(slot); 
		}
		while (!(la->kind == _EOF || la->kind == 11 /* "{" */)) {SynErr(152); Get();}
		Expect(11 /* "{" */);
		while (StartOf(6)) {
			TEXTSTYLEATTR(style);
			ExpectWeak(16 /* ";" */, 7);
		}
		while (!(la->kind == _EOF || la->kind == 12 /* "}" */)) {SynErr(153); Get();}
		Expect(12 /* "}" */);
		if (state) {
		 config.AddAreaTextStyle(filter,style);
		}
		
}

void Parser::AREAICONSTYLE(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 50 /* "ICON" */)) {SynErr(154); Get();}
		Expect(50 /* "ICON" */);
		IconPartialStyle style;
		
		while (!(la->kind == _EOF || la->kind == 11 /* "{" */)) {SynErr(155); Get();}
		Expect(11 /* "{" */);
		while (la->kind == 77 /* "position" */ || la->kind == 80 /* "symbol" */ || la->kind == 82 /* "name" */) {
			ICONSTYLEATTR(style);
			ExpectWeak(16 /* ";" */, 8);
		}
		while (!(la->kind == _EOF || la->kind == 12 /* "}" */)) {SynErr(156); Get();}
		Expect(12 /* "}" */);
		if (state) {
		 config.AddAreaIconStyle(filter,style);
		}
		
}

void Parser::AREABORDERTEXTSTYLE(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 54 /* "BORDERTEXT" */)) {SynErr(157); Get();}
		Expect(54 /* "BORDERTEXT" */);
		PathTextPartialStyle style;
		
		while (!(la->kind == _EOF || la->kind == 11 /* "{" */)) {SynErr(158); Get();}
		Expect(11 /* "{" */);
		while (la->kind == 55 /* "color" */ || la->kind == 72 /* "label" */ || la->kind == 74 /* "size" */) {
			PATHTEXTSTYLEATTR(style);
			ExpectWeak(16 /* ";" */, 11);
		}
		while (!(la->kind == _EOF || la->kind == 12 /* "}" */)) {SynErr(159); Get();}
		Expect(12 /* "}" */);
		if (state) {
		 config.AddAreaBorderTextStyle(filter,style);
		}
		
}

void Parser::UDISPLAYSIZE(double& value) {
		UDOUBLE(value);
		Expect(42 /* "mm" */);
}

void Parser::UMAPSIZE(double& value) {
		UDOUBLE(value);
		Expect(41 /* "m" */);
}

void Parser::DISPLAYSIZE(double& value) {
		DOUBLE(value);
		Expect(42 /* "mm" */);
}

void Parser::MAPSIZE(double& value) {
		DOUBLE(value);
		Expect(41 /* "m" */);
}

void Parser::CAPSTYLE(LineStyle::CapStyle& style) {
		if (la->kind == 83 /* "butt" */) {
			Get();
			style=LineStyle::capButt; 
		} else if (la->kind == 84 /* "round" */) {
			Get();
			style=LineStyle::capRound; 
		} else if (la->kind == 85 /* "square" */) {
			Get();
			style=LineStyle::capSquare; 
		} else SynErr(160);
}

void Parser::INT(int& value) {
		bool negate=false; 
		if (la->kind == 36 /* "-" */) {
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
		if (la->kind == 47 /* "." */) {
			Get();
			if (la->kind == _ident) {
				IDENT(labelName);
			} else if (la->kind == 82 /* "name" */) {
				Get();
				labelName="name"; 
			} else SynErr(161);
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
		if (la->kind == 86 /* "normal" */) {
			Get();
			style=TextStyle::normal; 
		} else if (la->kind == 87 /* "emphasize" */) {
			Get();
			style=TextStyle::emphasize; 
		} else SynErr(162);
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
	maxT = 93;

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

	static bool set[15][95] = {
		{T,x,x,x, x,x,x,T, x,T,x,T, T,x,x,x, x,T,x,T, x,T,T,T, x,T,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,T,x, T,x,T,T, T,T,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,T, x,x,x,x, x,x,x,x, x,x,x,T, T,T,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{T,x,x,x, x,x,x,T, x,T,x,T, T,x,x,x, x,T,x,T, x,T,T,T, x,T,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,T,x, T,x,T,T, T,T,T,T, x,x,x,x, x,x,x,x, x,x,x,T, T,T,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{x,x,x,x, x,x,x,x, x,x,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,T, T,T,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{x,x,x,x, x,x,x,x, x,x,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,T,x, x,x,x,T, x,T,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{x,x,x,x, x,x,x,x, x,x,x,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,T,x, x,x,x,T, x,T,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,T, x,x,x,x, x,x,x,x, x,T,x,x, x,x,x,x, T,T,T,T, T,T,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{T,x,x,x, x,x,x,T, x,T,x,T, T,x,x,x, x,T,x,T, x,T,T,T, x,T,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,T,x, T,x,T,T, T,T,T,T, x,x,x,x, x,x,x,x, x,T,x,x, x,x,x,x, T,T,T,T, T,T,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{T,x,x,x, x,x,x,T, x,T,x,T, T,x,x,x, x,T,x,T, x,T,T,T, x,T,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,T,x, T,x,T,T, T,T,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,T,x,x, T,x,T,x, x,x,x,x, x,x,x,x, x,x,x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,T, T,T,T,T, T,T,T,T, T,T,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{T,x,x,x, x,x,x,T, x,T,x,T, T,x,x,x, x,T,x,T, x,T,T,T, x,T,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,T,x, T,x,T,T, T,T,T,T, T,T,T,T, T,T,T,T, T,T,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{T,x,x,x, x,x,x,T, x,T,x,T, T,x,x,x, x,T,x,T, x,T,T,T, x,T,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,T,x, T,x,T,T, T,T,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, T,x,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{T,x,x,x, x,x,x,T, x,T,x,T, T,x,x,x, x,T,x,T, x,T,T,T, x,T,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,T,x, T,x,T,T, T,T,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, T,T,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,T, x,x,x,x, x,x,x,x, x,T,x,x, x,T,x,x, T,x,T,x, x,x,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{T,x,x,x, x,x,x,T, x,T,x,T, T,x,x,x, x,T,x,T, x,T,T,T, x,T,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,T,x, T,x,T,T, T,T,T,T, x,x,x,x, x,x,x,x, x,T,x,x, x,T,x,x, T,x,T,x, x,x,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x}
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
			case 10: s = coco_string_create("\"IF\" expected"); break;
			case 11: s = coco_string_create("\"{\" expected"); break;
			case 12: s = coco_string_create("\"}\" expected"); break;
			case 13: s = coco_string_create("\"ELIF\" expected"); break;
			case 14: s = coco_string_create("\"ELSE\" expected"); break;
			case 15: s = coco_string_create("\"=\" expected"); break;
			case 16: s = coco_string_create("\";\" expected"); break;
			case 17: s = coco_string_create("\"ORDER\" expected"); break;
			case 18: s = coco_string_create("\"WAYS\" expected"); break;
			case 19: s = coco_string_create("\"GROUP\" expected"); break;
			case 20: s = coco_string_create("\",\" expected"); break;
			case 21: s = coco_string_create("\"SYMBOL\" expected"); break;
			case 22: s = coco_string_create("\"POLYGON\" expected"); break;
			case 23: s = coco_string_create("\"RECTANGLE\" expected"); break;
			case 24: s = coco_string_create("\"x\" expected"); break;
			case 25: s = coco_string_create("\"CIRCLE\" expected"); break;
			case 26: s = coco_string_create("\"CONST\" expected"); break;
			case 27: s = coco_string_create("\"COLOR\" expected"); break;
			case 28: s = coco_string_create("\"MAG\" expected"); break;
			case 29: s = coco_string_create("\"UINT\" expected"); break;
			case 30: s = coco_string_create("\"STYLE\" expected"); break;
			case 31: s = coco_string_create("\"[\" expected"); break;
			case 32: s = coco_string_create("\"]\" expected"); break;
			case 33: s = coco_string_create("\"FEATURE\" expected"); break;
			case 34: s = coco_string_create("\"PATH\" expected"); break;
			case 35: s = coco_string_create("\"TYPE\" expected"); break;
			case 36: s = coco_string_create("\"-\" expected"); break;
			case 37: s = coco_string_create("\"ONEWAY\" expected"); break;
			case 38: s = coco_string_create("\"BRIDGE\" expected"); break;
			case 39: s = coco_string_create("\"TUNNEL\" expected"); break;
			case 40: s = coco_string_create("\"SIZE\" expected"); break;
			case 41: s = coco_string_create("\"m\" expected"); break;
			case 42: s = coco_string_create("\"mm\" expected"); break;
			case 43: s = coco_string_create("\":\" expected"); break;
			case 44: s = coco_string_create("\"px\" expected"); break;
			case 45: s = coco_string_create("\"<\" expected"); break;
			case 46: s = coco_string_create("\"NODE\" expected"); break;
			case 47: s = coco_string_create("\".\" expected"); break;
			case 48: s = coco_string_create("\"TEXT\" expected"); break;
			case 49: s = coco_string_create("\"#\" expected"); break;
			case 50: s = coco_string_create("\"ICON\" expected"); break;
			case 51: s = coco_string_create("\"WAY\" expected"); break;
			case 52: s = coco_string_create("\"SHIELD\" expected"); break;
			case 53: s = coco_string_create("\"AREA\" expected"); break;
			case 54: s = coco_string_create("\"BORDERTEXT\" expected"); break;
			case 55: s = coco_string_create("\"color\" expected"); break;
			case 56: s = coco_string_create("\"dash\" expected"); break;
			case 57: s = coco_string_create("\"gapColor\" expected"); break;
			case 58: s = coco_string_create("\"displayWidth\" expected"); break;
			case 59: s = coco_string_create("\"width\" expected"); break;
			case 60: s = coco_string_create("\"displayOffset\" expected"); break;
			case 61: s = coco_string_create("\"offset\" expected"); break;
			case 62: s = coco_string_create("\"cap\" expected"); break;
			case 63: s = coco_string_create("\"joinCap\" expected"); break;
			case 64: s = coco_string_create("\"endCap\" expected"); break;
			case 65: s = coco_string_create("\"priority\" expected"); break;
			case 66: s = coco_string_create("\"zIndex\" expected"); break;
			case 67: s = coco_string_create("\"pattern\" expected"); break;
			case 68: s = coco_string_create("\"patternMinMag\" expected"); break;
			case 69: s = coco_string_create("\"borderColor\" expected"); break;
			case 70: s = coco_string_create("\"borderWidth\" expected"); break;
			case 71: s = coco_string_create("\"borderDash\" expected"); break;
			case 72: s = coco_string_create("\"label\" expected"); break;
			case 73: s = coco_string_create("\"style\" expected"); break;
			case 74: s = coco_string_create("\"size\" expected"); break;
			case 75: s = coco_string_create("\"scaleMag\" expected"); break;
			case 76: s = coco_string_create("\"autoSize\" expected"); break;
			case 77: s = coco_string_create("\"position\" expected"); break;
			case 78: s = coco_string_create("\"backgroundColor\" expected"); break;
			case 79: s = coco_string_create("\"shieldSpace\" expected"); break;
			case 80: s = coco_string_create("\"symbol\" expected"); break;
			case 81: s = coco_string_create("\"symbolSpace\" expected"); break;
			case 82: s = coco_string_create("\"name\" expected"); break;
			case 83: s = coco_string_create("\"butt\" expected"); break;
			case 84: s = coco_string_create("\"round\" expected"); break;
			case 85: s = coco_string_create("\"square\" expected"); break;
			case 86: s = coco_string_create("\"normal\" expected"); break;
			case 87: s = coco_string_create("\"emphasize\" expected"); break;
			case 88: s = coco_string_create("\"lighten\" expected"); break;
			case 89: s = coco_string_create("\"(\" expected"); break;
			case 90: s = coco_string_create("\")\" expected"); break;
			case 91: s = coco_string_create("\"darken\" expected"); break;
			case 92: s = coco_string_create("\"!\" expected"); break;
			case 93: s = coco_string_create("??? expected"); break;
			case 94: s = coco_string_create("this symbol not expected in OSS"); break;
			case 95: s = coco_string_create("this symbol not expected in FLAGSECTION"); break;
			case 96: s = coco_string_create("this symbol not expected in WAYORDER"); break;
			case 97: s = coco_string_create("this symbol not expected in CONSTSECTION"); break;
			case 98: s = coco_string_create("this symbol not expected in SYMBOLSECTION"); break;
			case 99: s = coco_string_create("this symbol not expected in WAYGROUP"); break;
			case 100: s = coco_string_create("this symbol not expected in POLYGON"); break;
			case 101: s = coco_string_create("this symbol not expected in POLYGON"); break;
			case 102: s = coco_string_create("this symbol not expected in POLYGON"); break;
			case 103: s = coco_string_create("this symbol not expected in RECTANGLE"); break;
			case 104: s = coco_string_create("this symbol not expected in RECTANGLE"); break;
			case 105: s = coco_string_create("this symbol not expected in RECTANGLE"); break;
			case 106: s = coco_string_create("this symbol not expected in CIRCLE"); break;
			case 107: s = coco_string_create("this symbol not expected in CIRCLE"); break;
			case 108: s = coco_string_create("this symbol not expected in CIRCLE"); break;
			case 109: s = coco_string_create("invalid FILLSTYLEATTR"); break;
			case 110: s = coco_string_create("invalid UDOUBLE"); break;
			case 111: s = coco_string_create("invalid DOUBLE"); break;
			case 112: s = coco_string_create("invalid CONSTDEF"); break;
			case 113: s = coco_string_create("invalid COLOR"); break;
			case 114: s = coco_string_create("invalid MAG"); break;
			case 115: s = coco_string_create("invalid UINT"); break;
			case 116: s = coco_string_create("invalid STYLE"); break;
			case 117: s = coco_string_create("invalid STYLEDEF"); break;
			case 118: s = coco_string_create("this symbol not expected in NODESTYLEDEF"); break;
			case 119: s = coco_string_create("invalid NODESTYLEDEF"); break;
			case 120: s = coco_string_create("this symbol not expected in WAYSTYLEDEF"); break;
			case 121: s = coco_string_create("invalid WAYSTYLEDEF"); break;
			case 122: s = coco_string_create("invalid WAYSTYLEDEF"); break;
			case 123: s = coco_string_create("this symbol not expected in AREASTYLEDEF"); break;
			case 124: s = coco_string_create("invalid AREASTYLEDEF"); break;
			case 125: s = coco_string_create("invalid AREASTYLEDEF"); break;
			case 126: s = coco_string_create("this symbol not expected in NODETEXTSTYLE"); break;
			case 127: s = coco_string_create("this symbol not expected in NODETEXTSTYLE"); break;
			case 128: s = coco_string_create("this symbol not expected in NODETEXTSTYLE"); break;
			case 129: s = coco_string_create("this symbol not expected in NODEICONSTYLE"); break;
			case 130: s = coco_string_create("this symbol not expected in NODEICONSTYLE"); break;
			case 131: s = coco_string_create("this symbol not expected in NODEICONSTYLE"); break;
			case 132: s = coco_string_create("invalid TEXTSTYLEATTR"); break;
			case 133: s = coco_string_create("invalid ICONSTYLEATTR"); break;
			case 134: s = coco_string_create("this symbol not expected in WAYSTYLE"); break;
			case 135: s = coco_string_create("this symbol not expected in WAYSTYLE"); break;
			case 136: s = coco_string_create("this symbol not expected in WAYPATHTEXTSTYLE"); break;
			case 137: s = coco_string_create("this symbol not expected in WAYPATHTEXTSTYLE"); break;
			case 138: s = coco_string_create("this symbol not expected in WAYPATHTEXTSTYLE"); break;
			case 139: s = coco_string_create("this symbol not expected in WAYPATHSYMBOLSTYLE"); break;
			case 140: s = coco_string_create("this symbol not expected in WAYPATHSYMBOLSTYLE"); break;
			case 141: s = coco_string_create("this symbol not expected in WAYPATHSYMBOLSTYLE"); break;
			case 142: s = coco_string_create("this symbol not expected in WAYSHIELDSTYLE"); break;
			case 143: s = coco_string_create("this symbol not expected in WAYSHIELDSTYLE"); break;
			case 144: s = coco_string_create("this symbol not expected in WAYSHIELDSTYLE"); break;
			case 145: s = coco_string_create("invalid LINESTYLEATTR"); break;
			case 146: s = coco_string_create("invalid PATHTEXTSTYLEATTR"); break;
			case 147: s = coco_string_create("invalid PATHSYMBOLSTYLEATTR"); break;
			case 148: s = coco_string_create("invalid PATHSHIELDSTYLEATTR"); break;
			case 149: s = coco_string_create("this symbol not expected in AREASTYLE"); break;
			case 150: s = coco_string_create("this symbol not expected in AREASTYLE"); break;
			case 151: s = coco_string_create("this symbol not expected in AREATEXTSTYLE"); break;
			case 152: s = coco_string_create("this symbol not expected in AREATEXTSTYLE"); break;
			case 153: s = coco_string_create("this symbol not expected in AREATEXTSTYLE"); break;
			case 154: s = coco_string_create("this symbol not expected in AREAICONSTYLE"); break;
			case 155: s = coco_string_create("this symbol not expected in AREAICONSTYLE"); break;
			case 156: s = coco_string_create("this symbol not expected in AREAICONSTYLE"); break;
			case 157: s = coco_string_create("this symbol not expected in AREABORDERTEXTSTYLE"); break;
			case 158: s = coco_string_create("this symbol not expected in AREABORDERTEXTSTYLE"); break;
			case 159: s = coco_string_create("this symbol not expected in AREABORDERTEXTSTYLE"); break;
			case 160: s = coco_string_create("invalid CAPSTYLE"); break;
			case 161: s = coco_string_create("invalid TEXTLABE"); break;
			case 162: s = coco_string_create("invalid LABELSTYLE"); break;

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

