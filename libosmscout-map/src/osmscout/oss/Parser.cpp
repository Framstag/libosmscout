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

#include <osmscout/util/Logger.h>
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
		while (!(la->kind == _EOF || la->kind == 7 /* "OSS" */)) {SynErr(63); Get();}
		Expect(7 /* "OSS" */);
		state=true; 
		while (la->kind == 9 /* "FLAG" */) {
			FLAGSECTION();
		}
		if (la->kind == 17 /* "ORDER" */) {
			WAYORDER();
		}
		while (la->kind == 30 /* "CONST" */) {
			CONSTSECTION();
		}
		while (la->kind == 21 /* "SYMBO" */) {
			SYMBOLSECTION();
		}
		while (la->kind == 34 /* "STYLE" */) {
			STYLESECTION();
		}
		Expect(8 /* "END" */);
}

void Parser::FLAGSECTION() {
		while (!(la->kind == _EOF || la->kind == 9 /* "FLAG" */)) {SynErr(64); Get();}
		Expect(9 /* "FLAG" */);
		FLAGBLOCK(true);
}

void Parser::WAYORDER() {
		while (!(la->kind == _EOF || la->kind == 17 /* "ORDER" */)) {SynErr(65); Get();}
		Expect(17 /* "ORDER" */);
		Expect(18 /* "WAYS" */);
		size_t priority=1;
		while (la->kind == 19 /* "GROUP" */) {
			WAYGROUP(priority);
			priority++;
		}
}

void Parser::CONSTSECTION() {
		while (!(la->kind == _EOF || la->kind == 30 /* "CONST" */)) {SynErr(66); Get();}
		Expect(30 /* "CONST" */);
		CONSTBLOCK(true);
}

void Parser::SYMBOLSECTION() {
		while (!(la->kind == _EOF || la->kind == 21 /* "SYMBO" */)) {SynErr(67); Get();}
		Expect(21 /* "SYMBO" */);
		std::string name;
		
		IDENT(name);
		SymbolRef symbol=std::make_shared<Symbol>(name);
		
		while (la->kind == 25 /* "POLYGON" */ || la->kind == 27 /* "RECTANGLE" */ || la->kind == 29 /* "CIRCLE" */) {
			if (la->kind == 25 /* "POLYGON" */) {
				POLYGON(*symbol);
			} else if (la->kind == 27 /* "RECTANGLE" */) {
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
		Expect(34 /* "STYLE" */);
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
		
		if (la->kind == 61 /* "!" */) {
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
		while (!(la->kind == _EOF || la->kind == 19 /* "GROUP" */)) {SynErr(68); Get();}
		Expect(19 /* "GROUP" */);
		if (la->kind == _ident) {
			std::string wayTypeName;
			TypeInfoRef wayType;
			
			IDENT(wayTypeName);
			wayType=config.GetTypeConfig()->GetTypeInfo(wayTypeName);
			
			if (!wayType) {
			 std::string e="Unknown way type '"+wayTypeName+"'";
			 SemWarning(e.c_str());
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
			 SemWarning(e.c_str());
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
		while (!(la->kind == _EOF || la->kind == 25 /* "POLYGON" */)) {SynErr(69); Get();}
		Expect(25 /* "POLYGON" */);
		StyleFilter         filter;
		DrawPrimitive::ProjectionMode projectionMode=DrawPrimitive::ProjectionMode::MAP;
		FillPartialStyle    fillStyle;
		BorderPartialStyle  borderStyle;
		PolygonPrimitiveRef polygon;
		Vertex2D            coord;
		
		if (la->kind == 26 /* "GROUND" */) {
			Get();
			projectionMode=DrawPrimitive::ProjectionMode::GROUND; 
		}
		polygon=std::make_shared<PolygonPrimitive>(projectionMode,
		                                          fillStyle.style,
		                                          borderStyle.style);
		
		COORD(coord);
		polygon->AddCoord(coord); 
		COORD(coord);
		polygon->AddCoord(coord); 
		while (la->kind == _number || la->kind == _double || la->kind == 40 /* "-" */) {
			COORD(coord);
			polygon->AddCoord(coord); 
		}
		AREASYMBOLSTYLE(fillStyle,borderStyle);
		symbol.AddPrimitive(polygon); 
}

void Parser::RECTANGLE(Symbol& symbol) {
		while (!(la->kind == _EOF || la->kind == 27 /* "RECTANGLE" */)) {SynErr(70); Get();}
		Expect(27 /* "RECTANGLE" */);
		StyleFilter        filter;
		DrawPrimitive::ProjectionMode projectionMode=DrawPrimitive::ProjectionMode::MAP;
		FillPartialStyle   fillStyle;
		BorderPartialStyle borderStyle;
		Vertex2D           topLeft;
		double             width;
		double             height;
		
		if (la->kind == 26 /* "GROUND" */) {
			Get();
			projectionMode=DrawPrimitive::ProjectionMode::GROUND; 
		}
		COORD(topLeft);
		UDOUBLE(width);
		Expect(28 /* "x" */);
		UDOUBLE(height);
		AREASYMBOLSTYLE(fillStyle,borderStyle);
		symbol.AddPrimitive(std::make_shared<RectanglePrimitive>(projectionMode,
		                                                        topLeft,
		                                                        width,height,
		                                                        fillStyle.style,
		                                                        borderStyle.style));
		
}

void Parser::CIRCLE(Symbol& symbol) {
		while (!(la->kind == _EOF || la->kind == 29 /* "CIRCLE" */)) {SynErr(71); Get();}
		Expect(29 /* "CIRCLE" */);
		StyleFilter                   filter;
		DrawPrimitive::ProjectionMode projectionMode=DrawPrimitive::ProjectionMode::MAP;
		FillPartialStyle              fillStyle;
		BorderPartialStyle            borderStyle;
		Vertex2D                      center;
		double                        radius;
		
		if (la->kind == 26 /* "GROUND" */) {
			Get();
			projectionMode=DrawPrimitive::ProjectionMode::GROUND; 
		}
		COORD(center);
		UDOUBLE(radius);
		AREASYMBOLSTYLE(fillStyle,borderStyle);
		symbol.AddPrimitive(std::make_shared<CirclePrimitive>(projectionMode,
		                                                     center,
		                                                     radius,
		                                                     fillStyle.style,
		                                                     borderStyle.style));
		
}

void Parser::AREAFILLSYMSTYLE(FillPartialStyle& fillStyle) {
		while (!(la->kind == _EOF || la->kind == 11 /* "{" */)) {SynErr(72); Get();}
		Expect(11 /* "{" */);
		while (la->kind == _ident || la->kind == 56 /* "name" */) {
			FILLSTYLEATTR(fillStyle);
			ExpectWeak(16 /* ";" */, 1);
		}
		Expect(12 /* "}" */);
}

void Parser::FILLSTYLEATTR(FillPartialStyle& style) {
		ATTRIBUTE(style,*FillStyle::GetDescriptor());
}

void Parser::AREABORDERSYMSTYLE(BorderPartialStyle& borderStyle) {
		while (!(la->kind == _EOF || la->kind == 22 /* "BORDER" */)) {SynErr(73); Get();}
		Expect(22 /* "BORDER" */);
		Expect(11 /* "{" */);
		while (la->kind == _ident || la->kind == 56 /* "name" */) {
			BORDERSTYLEATTR(borderStyle);
			ExpectWeak(16 /* ";" */, 1);
		}
		Expect(12 /* "}" */);
}

void Parser::BORDERSTYLEATTR(BorderPartialStyle& style) {
		ATTRIBUTE(style,*BorderStyle::GetDescriptor());
}

void Parser::AREASYMBOLSTYLE(FillPartialStyle& fillStyle, BorderPartialStyle& borderStyle) {
		while (!(la->kind == _EOF || la->kind == 11 /* "{" */)) {SynErr(74); Get();}
		Expect(11 /* "{" */);
		while (la->kind == 23 /* "AREA" */) {
			while (!(la->kind == _EOF || la->kind == 23 /* "AREA" */)) {SynErr(75); Get();}
			Get();
			if (la->kind == 11 /* "{" */) {
				AREAFILLSYMSTYLE(fillStyle);
			} else if (la->kind == 24 /* "." */) {
				Get();
				AREABORDERSYMSTYLE(borderStyle);
			} else SynErr(76);
		}
		while (!(la->kind == _EOF || la->kind == 12 /* "}" */)) {SynErr(77); Get();}
		Expect(12 /* "}" */);
}

void Parser::COORD(Vertex2D& coord) {
		double x;
		double y;
		
		DOUBLE(x);
		Expect(20 /* "," */);
		DOUBLE(y);
		coord=Vertex2D(x,y); 
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
			
		} else SynErr(78);
}

void Parser::DOUBLE(double& value) {
		bool negate=false; 
		if (la->kind == 40 /* "-" */) {
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
			
		} else SynErr(79);
		if (negate) {
		 value=-value;
		}
		
}

void Parser::CONSTBLOCK(bool state) {
		while (StartOf(2)) {
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
		if (la->kind == 31 /* "COLOR" */) {
			COLORCONSTDEF();
		} else if (la->kind == 32 /* "MAG" */) {
			MAGCONSTDEF();
		} else if (la->kind == 33 /* "UINT" */) {
			UINTCONSTDEF();
		} else SynErr(80);
}

void Parser::COLORCONSTDEF() {
		std::string      name;
		StyleConstantRef constant;
		Color            color;
		
		Expect(31 /* "COLOR" */);
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
		
		Expect(32 /* "MAG" */);
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
		
		Expect(33 /* "UINT" */);
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
		StyleConstantRef constant;
		double           factor;
		
		if (la->kind == 57 /* "lighten" */) {
			Get();
			Expect(59 /* "(" */);
			COLOR(color);
			Expect(20 /* "," */);
			UDOUBLE(factor);
			Expect(60 /* ")" */);
			if (factor>=0.0 && factor<=1.0) {
			 color=color.Lighten(factor);
			}
			else {
			std::string e="Factor must be in the range [0..1]";
			
			 SemErr(e.c_str());
			}
			
		} else if (la->kind == 58 /* "darken" */) {
			double factor; 
			Get();
			Expect(59 /* "(" */);
			COLOR(color);
			Expect(20 /* "," */);
			UDOUBLE(factor);
			Expect(60 /* ")" */);
			if (factor>=0.0 && factor<=1.0) {
			 color=color.Darken(factor);
			}
			else {
			std::string e="Factor must be in the range [0..1]";
			
			 SemErr(e.c_str());
			}
			
		} else if (la->kind == _color) {
			COLOR_VALUE(color);
		} else if (la->kind == _variable) {
			CONSTANT(constant);
			if (!constant) {
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
			
		} else SynErr(81);
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
			StyleConstantRef constant; 
			CONSTANT(constant);
			if (!constant) {
			 magnification.SetLevel(0);
			}
			else if (dynamic_cast<StyleConstantMag*>(constant.get())==NULL) {
			 std::string e="Variable is not of type 'MAG'";
			
			 SemErr(e.c_str());
			}
			else {
			 StyleConstantMag* magConstant=dynamic_cast<StyleConstantMag*>(constant.get());
			
			 magnification=magConstant->GetMag();
			}
			
		} else SynErr(82);
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
			
		} else SynErr(83);
}

void Parser::STYLEBLOCK(StyleFilter filter, bool state) {
		while (StartOf(3)) {
			if (StartOf(4)) {
				STYLE(filter,state);
			} else {
				STYLECONDBLOCK(filter,state);
			}
		}
}

void Parser::STYLE(StyleFilter filter, bool state) {
		if (la->kind == 35 /* "[" */) {
			STYLEFILTER(filter);
		}
		if (la->kind == 11 /* "{" */) {
			Get();
			STYLEBLOCK(filter,state);
			Expect(12 /* "}" */);
		} else if (la->kind == 23 /* "AREA" */ || la->kind == 48 /* "NODE" */ || la->kind == 52 /* "WAY" */) {
			STYLEDEF(filter,state);
		} else SynErr(84);
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
		Expect(35 /* "[" */);
		if (la->kind == 19 /* "GROUP" */) {
			STYLEFILTER_GROUP(filter);
		}
		if (la->kind == 37 /* "FEATURE" */) {
			STYLEFILTER_FEATURE(filter);
		}
		if (la->kind == 38 /* "PATH" */) {
			STYLEFILTER_PATH(filter);
		}
		if (la->kind == 39 /* "TYPE" */) {
			STYLEFILTER_TYPE(filter);
		}
		if (la->kind == 32 /* "MAG" */) {
			STYLEFILTER_MAG(filter);
		}
		if (la->kind == 41 /* "ONEWAY" */) {
			STYLEFILTER_ONEWAY(filter);
		}
		if (la->kind == 42 /* "SIZE" */) {
			STYLEFILTER_SIZE(filter);
		}
		Expect(36 /* "]" */);
}

void Parser::STYLEDEF(StyleFilter filter, bool state) {
		if (la->kind == 48 /* "NODE" */) {
			NODESTYLEDEF(filter,state);
		} else if (la->kind == 52 /* "WAY" */) {
			WAYSTYLEDEF(filter,state);
		} else if (la->kind == 23 /* "AREA" */) {
			AREASTYLEDEF(filter,state);
		} else SynErr(85);
}

void Parser::STYLEFILTER_GROUP(StyleFilter& filter) {
		TypeInfoSet types;
		std::string groupName;
		
		Expect(19 /* "GROUP" */);
		IDENT(groupName);
		for (const auto& type : config.GetTypeConfig()->GetTypes()) {
		 if (type->IsInGroup(groupName)) {
		   if (filter.FiltersByType() &&
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
		
		Expect(37 /* "FEATURE" */);
		IDENT(featureName);
		AddFeatureToFilter(filter,
		                  featureName,
		                  types);
		
		while (la->kind == 20 /* "," */) {
			Get();
			IDENT(featureName);
			AddFeatureToFilter(filter,
			                  featureName,
			                  types);
			
		}
		filter.SetTypes(types); 
}

void Parser::STYLEFILTER_PATH(StyleFilter& filter) {
		TypeInfoSet types;
		
		Expect(38 /* "PATH" */);
		for (const auto& type : config.GetTypeConfig()->GetTypes()) {
		 if (type->IsPath()) {
		   if (filter.FiltersByType() &&
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
		
		Expect(39 /* "TYPE" */);
		IDENT(name);
		TypeInfoRef type=config.GetTypeConfig()->GetTypeInfo(name);
		
		if (!type) {
		 std::string e="Unknown type '"+name+"'";
		
		 SemWarning(e.c_str());
		}
		else if (filter.FiltersByType() &&
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
			
			 SemWarning(e.c_str());
			}
			else if (filter.FiltersByType() &&
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
		Expect(32 /* "MAG" */);
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
		Expect(40 /* "-" */);
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
		Expect(41 /* "ONEWAY" */);
		filter.SetOneway(true);
		
}

void Parser::STYLEFILTER_SIZE(StyleFilter& filter) {
		SizeConditionRef sizeCondition; 
		Expect(42 /* "SIZE" */);
		SIZECONDITION(sizeCondition);
		filter.SetSizeCondition(sizeCondition); 
}

void Parser::SIZECONDITION(SizeConditionRef& condition) {
		condition=std::make_shared<SizeCondition>();
		double widthInMeter;
		
		UDOUBLE(widthInMeter);
		Expect(43 /* "m" */);
		if (widthInMeter<0.0) {
		std::string e="Width must be >= 0.0";
		
		SemErr(e.c_str());
		}
		
		if (la->kind == _number || la->kind == _double || la->kind == 45 /* ":" */) {
			if (la->kind == _number || la->kind == _double) {
				double minMM; 
				UDOUBLE(minMM);
				Expect(44 /* "mm" */);
				if (widthInMeter>0.0) {
				 condition->SetMinMM(minMM/widthInMeter);
				}
				
			}
			Expect(45 /* ":" */);
			if (la->kind == _number || la->kind == _double) {
				double minPx; 
				UDOUBLE(minPx);
				Expect(46 /* "px" */);
				if (widthInMeter>0.0) {
				 condition->SetMinPx(minPx/widthInMeter);
				}
				
			}
		}
		Expect(47 /* "<" */);
		if (la->kind == _number || la->kind == _double || la->kind == 45 /* ":" */) {
			if (la->kind == _number || la->kind == _double) {
				double maxMM; 
				UDOUBLE(maxMM);
				Expect(44 /* "mm" */);
				if (widthInMeter>0.0) {
				 condition->SetMaxMM(maxMM/widthInMeter);
				}
				
			}
			Expect(45 /* ":" */);
			if (la->kind == _number || la->kind == _double) {
				double maxPx; 
				UDOUBLE(maxPx);
				Expect(46 /* "px" */);
				if (widthInMeter>0.0) {
				 condition->SetMaxPx(maxPx/widthInMeter);
				}
				
			}
		}
}

void Parser::NODESTYLEDEF(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 48 /* "NODE" */)) {SynErr(86); Get();}
		Expect(48 /* "NODE" */);
		Expect(24 /* "." */);
		if (la->kind == 49 /* "TEXT" */) {
			NODETEXTSTYLE(filter,state);
		} else if (la->kind == 51 /* "ICON" */) {
			NODEICONSTYLE(filter,state);
		} else SynErr(87);
}

void Parser::WAYSTYLEDEF(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 52 /* "WAY" */)) {SynErr(88); Get();}
		Expect(52 /* "WAY" */);
		if (la->kind == 11 /* "{" */ || la->kind == 50 /* "#" */) {
			WAYSTYLE(filter,state);
		} else if (la->kind == 24 /* "." */) {
			Get();
			if (la->kind == 49 /* "TEXT" */) {
				WAYPATHTEXTSTYLE(filter,state);
			} else if (la->kind == 21 /* "SYMBO" */) {
				WAYPATHSYMBOLSTYLE(filter,state);
			} else if (la->kind == 53 /* "SHIELD" */) {
				WAYSHIELDSTYLE(filter,state);
			} else SynErr(89);
		} else SynErr(90);
}

void Parser::AREASTYLEDEF(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 23 /* "AREA" */)) {SynErr(91); Get();}
		Expect(23 /* "AREA" */);
		if (la->kind == 11 /* "{" */) {
			AREASTYLE(filter,state);
		} else if (la->kind == 24 /* "." */) {
			Get();
			if (la->kind == 49 /* "TEXT" */) {
				AREATEXTSTYLE(filter,state);
			} else if (la->kind == 51 /* "ICON" */) {
				AREAICONSTYLE(filter,state);
			} else if (la->kind == 22 /* "BORDER" */) {
				AREABORDERSTYLE(filter,state);
			} else if (la->kind == 54 /* "BORDERTEXT" */) {
				AREABORDERTEXTSTYLE(filter,state);
			} else if (la->kind == 55 /* "BORDERSYMBO" */) {
				AREABORDERSYMBOLSTYLE(filter,state);
			} else SynErr(92);
		} else SynErr(93);
}

void Parser::NODETEXTSTYLE(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 49 /* "TEXT" */)) {SynErr(94); Get();}
		Expect(49 /* "TEXT" */);
		TextPartialStyle style;
		std::string      slot;
		
		if (la->kind == 50 /* "#" */) {
			Get();
			IDENT(slot);
			style.style->SetSlot(slot); 
		}
		while (!(la->kind == _EOF || la->kind == 11 /* "{" */)) {SynErr(95); Get();}
		Expect(11 /* "{" */);
		while (la->kind == _ident || la->kind == 56 /* "name" */) {
			TEXTSTYLEATTR(style);
			ExpectWeak(16 /* ";" */, 1);
		}
		while (!(la->kind == _EOF || la->kind == 12 /* "}" */)) {SynErr(96); Get();}
		Expect(12 /* "}" */);
		if (state) {
		 config.AddNodeTextStyle(filter,style);
		}
		
}

void Parser::NODEICONSTYLE(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 51 /* "ICON" */)) {SynErr(97); Get();}
		Expect(51 /* "ICON" */);
		IconPartialStyle style;
		
		while (!(la->kind == _EOF || la->kind == 11 /* "{" */)) {SynErr(98); Get();}
		Expect(11 /* "{" */);
		while (la->kind == _ident || la->kind == 56 /* "name" */) {
			ICONSTYLEATTR(style);
			ExpectWeak(16 /* ";" */, 1);
		}
		while (!(la->kind == _EOF || la->kind == 12 /* "}" */)) {SynErr(99); Get();}
		Expect(12 /* "}" */);
		if (state) {
		 config.AddNodeIconStyle(filter,style);
		}
		
}

void Parser::TEXTSTYLEATTR(TextPartialStyle& style) {
		ATTRIBUTE(style,*TextStyle::GetDescriptor());
}

void Parser::ICONSTYLEATTR(IconPartialStyle& style) {
		ATTRIBUTE(style,*IconStyle::GetDescriptor());
}

void Parser::WAYSTYLE(StyleFilter filter, bool state) {
		LinePartialStyle style;
		std::string      slot;
		
		if (la->kind == 50 /* "#" */) {
			Get();
			IDENT(slot);
			style.style->SetSlot(slot); 
		}
		while (!(la->kind == _EOF || la->kind == 11 /* "{" */)) {SynErr(100); Get();}
		Expect(11 /* "{" */);
		while (la->kind == _ident || la->kind == 56 /* "name" */) {
			LINESTYLEATTR(style);
			ExpectWeak(16 /* ";" */, 1);
		}
		while (!(la->kind == _EOF || la->kind == 12 /* "}" */)) {SynErr(101); Get();}
		Expect(12 /* "}" */);
		if (state) {
		 config.AddWayLineStyle(filter,style);
		}
		
}

void Parser::WAYPATHTEXTSTYLE(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 49 /* "TEXT" */)) {SynErr(102); Get();}
		Expect(49 /* "TEXT" */);
		PathTextPartialStyle style;
		
		while (!(la->kind == _EOF || la->kind == 11 /* "{" */)) {SynErr(103); Get();}
		Expect(11 /* "{" */);
		while (la->kind == _ident || la->kind == 56 /* "name" */) {
			PATHTEXTSTYLEATTR(style);
			ExpectWeak(16 /* ";" */, 1);
		}
		while (!(la->kind == _EOF || la->kind == 12 /* "}" */)) {SynErr(104); Get();}
		Expect(12 /* "}" */);
		if (state) {
		 config.AddWayPathTextStyle(filter,style);
		}
		
}

void Parser::WAYPATHSYMBOLSTYLE(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 21 /* "SYMBO" */)) {SynErr(105); Get();}
		Expect(21 /* "SYMBO" */);
		PathSymbolPartialStyle style;
		
		while (!(la->kind == _EOF || la->kind == 11 /* "{" */)) {SynErr(106); Get();}
		Expect(11 /* "{" */);
		while (la->kind == _ident || la->kind == 56 /* "name" */) {
			PATHSYMBOLSTYLEATTR(style);
			ExpectWeak(16 /* ";" */, 1);
		}
		while (!(la->kind == _EOF || la->kind == 12 /* "}" */)) {SynErr(107); Get();}
		Expect(12 /* "}" */);
		if (state) {
		 config.AddWayPathSymbolStyle(filter,style);
		}
		
}

void Parser::WAYSHIELDSTYLE(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 53 /* "SHIELD" */)) {SynErr(108); Get();}
		Expect(53 /* "SHIELD" */);
		PathShieldPartialStyle style;
		
		while (!(la->kind == _EOF || la->kind == 11 /* "{" */)) {SynErr(109); Get();}
		Expect(11 /* "{" */);
		while (la->kind == _ident || la->kind == 56 /* "name" */) {
			PATHSHIELDSTYLEATTR(style);
			ExpectWeak(16 /* ";" */, 1);
		}
		while (!(la->kind == _EOF || la->kind == 12 /* "}" */)) {SynErr(110); Get();}
		Expect(12 /* "}" */);
		if (state) {
		 config.AddWayPathShieldStyle(filter,style);
		}
		
}

void Parser::LINESTYLEATTR(LinePartialStyle& style) {
		ATTRIBUTE(style,*LineStyle::GetDescriptor());
}

void Parser::PATHTEXTSTYLEATTR(PathTextPartialStyle& style) {
		ATTRIBUTE(style,*PathTextStyle::GetDescriptor());
}

void Parser::PATHSYMBOLSTYLEATTR(PathSymbolPartialStyle& style) {
		ATTRIBUTE(style,*PathSymbolStyle::GetDescriptor());
}

void Parser::PATHSHIELDSTYLEATTR(PathShieldPartialStyle& style) {
		ATTRIBUTE(style,*PathShieldStyle::GetDescriptor());
}

void Parser::AREASTYLE(StyleFilter filter, bool state) {
		FillPartialStyle style;
		
		while (!(la->kind == _EOF || la->kind == 11 /* "{" */)) {SynErr(111); Get();}
		Expect(11 /* "{" */);
		while (la->kind == _ident || la->kind == 56 /* "name" */) {
			FILLSTYLEATTR(style);
			ExpectWeak(16 /* ";" */, 1);
		}
		while (!(la->kind == _EOF || la->kind == 12 /* "}" */)) {SynErr(112); Get();}
		Expect(12 /* "}" */);
		if (state) {
		 config.AddAreaFillStyle(filter,style);
		}
		
}

void Parser::AREATEXTSTYLE(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 49 /* "TEXT" */)) {SynErr(113); Get();}
		Expect(49 /* "TEXT" */);
		TextPartialStyle style;
		std::string      slot;
		
		if (la->kind == 50 /* "#" */) {
			Get();
			IDENT(slot);
			style.style->SetSlot(slot); 
		}
		while (!(la->kind == _EOF || la->kind == 11 /* "{" */)) {SynErr(114); Get();}
		Expect(11 /* "{" */);
		while (la->kind == _ident || la->kind == 56 /* "name" */) {
			TEXTSTYLEATTR(style);
			ExpectWeak(16 /* ";" */, 1);
		}
		while (!(la->kind == _EOF || la->kind == 12 /* "}" */)) {SynErr(115); Get();}
		Expect(12 /* "}" */);
		if (state) {
		 config.AddAreaTextStyle(filter,style);
		}
		
}

void Parser::AREAICONSTYLE(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 51 /* "ICON" */)) {SynErr(116); Get();}
		Expect(51 /* "ICON" */);
		IconPartialStyle style;
		
		while (!(la->kind == _EOF || la->kind == 11 /* "{" */)) {SynErr(117); Get();}
		Expect(11 /* "{" */);
		while (la->kind == _ident || la->kind == 56 /* "name" */) {
			ICONSTYLEATTR(style);
			ExpectWeak(16 /* ";" */, 1);
		}
		while (!(la->kind == _EOF || la->kind == 12 /* "}" */)) {SynErr(118); Get();}
		Expect(12 /* "}" */);
		if (state) {
		 config.AddAreaIconStyle(filter,style);
		}
		
}

void Parser::AREABORDERSTYLE(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 22 /* "BORDER" */)) {SynErr(119); Get();}
		Expect(22 /* "BORDER" */);
		BorderPartialStyle style;
		std::string        slot;
		
		if (la->kind == 50 /* "#" */) {
			Get();
			IDENT(slot);
			style.style->SetSlot(slot); 
		}
		while (!(la->kind == _EOF || la->kind == 11 /* "{" */)) {SynErr(120); Get();}
		Expect(11 /* "{" */);
		while (la->kind == _ident || la->kind == 56 /* "name" */) {
			BORDERSTYLEATTR(style);
			ExpectWeak(16 /* ";" */, 1);
		}
		while (!(la->kind == _EOF || la->kind == 12 /* "}" */)) {SynErr(121); Get();}
		Expect(12 /* "}" */);
		if (state) {
		 config.AddAreaBorderStyle(filter,style);
		}
		
}

void Parser::AREABORDERTEXTSTYLE(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 54 /* "BORDERTEXT" */)) {SynErr(122); Get();}
		Expect(54 /* "BORDERTEXT" */);
		PathTextPartialStyle style;
		
		while (!(la->kind == _EOF || la->kind == 11 /* "{" */)) {SynErr(123); Get();}
		Expect(11 /* "{" */);
		while (la->kind == _ident || la->kind == 56 /* "name" */) {
			PATHTEXTSTYLEATTR(style);
			ExpectWeak(16 /* ";" */, 1);
		}
		while (!(la->kind == _EOF || la->kind == 12 /* "}" */)) {SynErr(124); Get();}
		Expect(12 /* "}" */);
		if (state) {
		 config.AddAreaBorderTextStyle(filter,style);
		}
		
}

void Parser::AREABORDERSYMBOLSTYLE(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 55 /* "BORDERSYMBO" */)) {SynErr(125); Get();}
		Expect(55 /* "BORDERSYMBO" */);
		PathSymbolPartialStyle style;
		
		while (!(la->kind == _EOF || la->kind == 11 /* "{" */)) {SynErr(126); Get();}
		Expect(11 /* "{" */);
		while (la->kind == _ident || la->kind == 56 /* "name" */) {
			PATHSYMBOLSTYLEATTR(style);
			ExpectWeak(16 /* ";" */, 1);
		}
		while (!(la->kind == _EOF || la->kind == 12 /* "}" */)) {SynErr(127); Get();}
		Expect(12 /* "}" */);
		if (state) {
		 config.AddAreaBorderSymbolStyle(filter,style);
		}
		
}

void Parser::ATTRIBUTE(PartialStyleBase& style, const StyleDescriptor& descriptor) {
		std::string                 attributeName;
		StyleAttributeDescriptorRef attributeDescriptor;
		
		if (la->kind == _ident) {
			IDENT(attributeName);
		} else if (la->kind == 56 /* "name" */) {
			Get();
			attributeName="name"; 
		} else SynErr(128);
		attributeDescriptor=descriptor.GetAttribute(attributeName);
		
		if (!attributeDescriptor) {
		 std::string e="'"+attributeName+"' is not a known attribute of the given style";
		
		 SemErr(e.c_str());
		
		 attributeDescriptor=std::make_shared<StyleVoidAttributeDescriptor>();
		}
		
		Expect(45 /* ":" */);
		ATTRIBUTEVALUE(style,*attributeDescriptor);
}

void Parser::ATTRIBUTEVALUE(PartialStyleBase& style, const StyleAttributeDescriptor& descriptor) {
		ValueType              valueType=ValueType::NO_VALUE;
		bool                   negate=false;
		std::string            ident;
		std::string            subIdent;
		std::string            stringValue;
		std::string            function;
		double                 factor;
		std::string            unit;
		std::string            number;
		std::list<std::string> numberList;
		Color                  color;
		StyleConstantRef       constant;
		
		if (la->kind == _ident || la->kind == 57 /* "lighten" */ || la->kind == 58 /* "darken" */) {
			if (la->kind == _ident) {
				IDENT(ident);
				valueType=ValueType::IDENT; 
			} else if (la->kind == 57 /* "lighten" */) {
				Get();
				ident="lighten"; valueType=ValueType::IDENT; 
			} else {
				Get();
				ident="darken"; valueType=ValueType::IDENT; 
			}
			if (la->kind == 24 /* "." */ || la->kind == 59 /* "(" */) {
				if (la->kind == 59 /* "(" */) {
					Get();
					valueType=ValueType::COLOR;
					function=ident;
					
					if (la->kind == _color) {
						COLOR_VALUE(color);
					} else if (la->kind == _variable) {
						CONSTANT(constant);
					} else SynErr(129);
					Expect(20 /* "," */);
					UDOUBLE(factor);
					Expect(60 /* ")" */);
				} else {
					Get();
					if (la->kind == _ident) {
						IDENT(subIdent);
					} else if (la->kind == 56 /* "name" */) {
						Get();
						subIdent="name"; 
					} else SynErr(130);
				}
			}
		} else if (la->kind == _string) {
			STRING(stringValue);
			valueType=ValueType::STRING; 
		} else if (la->kind == _number || la->kind == _double || la->kind == 40 /* "-" */) {
			if (la->kind == 40 /* "-" */) {
				Get();
				negate=true; 
			}
			if (la->kind == _number) {
				Get();
				number=t->val;
				valueType=ValueType::NUMBER;
				
				if (la->kind == _ident || la->kind == 43 /* "m" */ || la->kind == 44 /* "mm" */) {
					if (la->kind == _ident) {
						IDENT(unit);
					} else if (la->kind == 44 /* "mm" */) {
						Get();
						unit="mm"; 
					} else {
						Get();
						unit="m"; 
					}
				}
				while (la->kind == 20 /* "," */) {
					Get();
					if (la->kind == _number) {
						Get();
						numberList.push_back(t->val); 
					} else if (la->kind == _double) {
						Get();
						numberList.push_back(t->val); 
					} else SynErr(131);
				}
			} else if (la->kind == _double) {
				Get();
				number=t->val;
				valueType=ValueType::NUMBER;
				
				if (la->kind == _ident || la->kind == 43 /* "m" */ || la->kind == 44 /* "mm" */) {
					if (la->kind == _ident) {
						IDENT(unit);
					} else if (la->kind == 44 /* "mm" */) {
						Get();
						unit="mm"; 
					} else {
						Get();
						unit="m"; 
					}
				}
				while (la->kind == 20 /* "," */) {
					Get();
					if (la->kind == _number) {
						Get();
						numberList.push_back(t->val); 
					} else if (la->kind == _double) {
						Get();
						numberList.push_back(t->val); 
					} else SynErr(132);
				}
			} else SynErr(133);
		} else if (la->kind == _color) {
			COLOR_VALUE(color);
			valueType=ValueType::COLOR; 
		} else if (la->kind == _variable) {
			CONSTANT(constant);
			valueType=ValueType::CONSTANT; 
		} else SynErr(134);
		if (descriptor.GetType()==StyleAttributeType::TYPE_BOOL) {
		 if (valueType==ValueType::IDENT) {
		   if (ident=="true" && subIdent.empty()) {
		     style.SetBoolValue(descriptor.GetAttribute(),true);
		   }
		   else if (ident=="false" && subIdent.empty()) {
		     style.SetBoolValue(descriptor.GetAttribute(),false);
		   }
		   else {
		     std::string e="Attribute '"+descriptor.GetName()+"' requires a boolean value ('true' or 'false')";
		
		     SemErr(e.c_str());
		   }
		 }
		 else {
		   std::string e="Attribute '"+descriptor.GetName()+"' has incompatible type for value";
		
		   SemErr(e.c_str());
		 }
		}
		else if (descriptor.GetType()==StyleAttributeType::TYPE_STRING) {
		 if (valueType==ValueType::IDENT) {
		   if (subIdent.empty()) {
		     style.SetStringValue(descriptor.GetAttribute(),ident);
		   }
		   else {
		     std::string e="Attribute '"+descriptor.GetName()+"' requires a simple IDENT as value";
		
		     SemErr(e.c_str());
		   }
		 }
		 else if (valueType==ValueType::STRING) {
		   style.SetStringValue(descriptor.GetAttribute(),stringValue);
		 }
		 else {
		   std::string e="Attribute '"+descriptor.GetName()+"' has incompatible type for value";
		
		   SemErr(e.c_str());
		 }
		}
		else if (descriptor.GetType()==StyleAttributeType::TYPE_COLOR) {
		 if (valueType==ValueType::COLOR) {
		   if (constant) {
		     if (dynamic_cast<StyleConstantColor*>(constant.get())==NULL) {
		       std::string e="Constant is not of type 'COLOR'";
		
		       SemErr(e.c_str());
		     }
		     else {
		       StyleConstantColor* colorConstant=dynamic_cast<StyleConstantColor*>(constant.get());
		
		       color=colorConstant->GetColor();
		     }
		   }
		
		   if (!function.empty()) {
		     if (factor<0.0 && factor>1.0) {
		      std::string e="Factor must be in the range [0..1]";
		
		       SemErr(e.c_str());
		     }
		
		     if (function=="lighten") {
		       if (!errors->hasErrors) {
		         style.SetColorValue(descriptor.GetAttribute(),color.Lighten(factor));
		       }
		     }
		     else if (function=="darken") {
		       if (!errors->hasErrors) {
		         style.SetColorValue(descriptor.GetAttribute(),color.Darken(factor));
		       }
		     }
		     else {
		       std::string e="Unknown color function '"+function+"'";
		
		       SemErr(e.c_str());
		     }
		   }
		   else {
		     style.SetColorValue(descriptor.GetAttribute(),color);
		   }
		 }
		 else if (valueType==ValueType::CONSTANT) {
		   if (!constant) {
		     // no code
		   }
		   else if (dynamic_cast<StyleConstantColor*>(constant.get())==NULL) {
		     std::string e="Constant is not of type 'COLOR'";
		
		     SemErr(e.c_str());
		   }
		
		   if (!errors->hasErrors) {
		     StyleConstantColor* colorConstant=dynamic_cast<StyleConstantColor*>(constant.get());
		
		     style.SetColorValue(descriptor.GetAttribute(),colorConstant->GetColor());
		   }
		 }
		 else {
		   std::string e="Attribute '"+descriptor.GetName()+"' has incompatible type for value";
		
		   SemErr(e.c_str());
		 }
		}
		else if (descriptor.GetType()==StyleAttributeType::TYPE_MAGNIFICATION) {
		 if (valueType==ValueType::IDENT) {
		   Magnification magnification;
		
		   if (subIdent.empty() && magnificationConverter.Convert(ident,magnification)) {
		     style.SetMagnificationValue(descriptor.GetAttribute(),magnification);
		   }
		   else {
		     std::string e="'"+std::string(ident)+"' is not a valid magnification level";
		
		     SemErr(e.c_str());
		   }
		 }
		 else if (valueType==ValueType::NUMBER) {
		   Magnification magnification;
		   size_t        level;
		
		   if (StringToNumber(number,level)) {
		     magnification.SetLevel((uint32_t)level);
		   }
		   else {
		     std::string e="Cannot parse number '"+std::string(number)+"'";
		
		     SemErr(e.c_str());
		   }
		 }
		 else if (valueType==ValueType::CONSTANT) {
		   if (!constant) {
		     // no code
		   }
		   else if (dynamic_cast<StyleConstantMag*>(constant.get())==NULL) {
		     std::string e="Constant is not of type 'MAGNIFICATION'";
		
		     SemErr(e.c_str());
		   }
		
		   if (!errors->hasErrors) {
		     StyleConstantMag* uintConstant=dynamic_cast<StyleConstantMag*>(constant.get());
		
		     style.SetMagnificationValue(descriptor.GetAttribute(),uintConstant->GetMag());
		   }
		 }
		 else {
		   std::string e="Attribute '"+descriptor.GetName()+"' has incompatible type for value";
		
		   SemErr(e.c_str());
		 }
		}
		else if (descriptor.GetType()==StyleAttributeType::TYPE_ENUM) {
		 const StyleEnumAttributeDescriptor& enumDescriptor=dynamic_cast<const StyleEnumAttributeDescriptor&>(descriptor);
		
		 if (valueType==ValueType::IDENT) {
		   int value=enumDescriptor.GetEnumValue(ident);
		
		   if (value>=0) {
		     style.SetIntValue(descriptor.GetAttribute(),value);
		   }
		   else {
		     std::string e="'"+ident+"' is not a valid enumeration value for this attribute";
		
		     SemErr(e.c_str());
		   }
		 }
		 else {
		   std::string e="Attribute '"+descriptor.GetName()+"' has incompatible type for value";
		
		   SemErr(e.c_str());
		 }
		}
		else if (descriptor.GetType()==StyleAttributeType::TYPE_DISPLAY_SIZE) {
		 if (valueType==ValueType::NUMBER) {
		   double value;
		
		   if (unit!="mm") {
		     std::string e="Value must have unit 'mm' and not '"+unit+"'";
		
		     SemErr(e.c_str());
		   }
		
		   if (!StringToNumber(number,value)) {
		     std::string e="Cannot parse number '"+number+"'";
		
		     SemErr(e.c_str());
		   }
		
		   if (!errors->hasErrors) {
		     if (negate) {
		       value=-value;
		     }
		     style.SetDoubleValue(descriptor.GetAttribute(),value);
		   }
		 }
		}
		else if (descriptor.GetType()==StyleAttributeType::TYPE_UDISPLAY_SIZE) {
		 if (valueType==ValueType::NUMBER) {
		   double value;
		
		   if (negate) {
		     std::string e="Negative numbers not allowed here";
		
		     SemErr(e.c_str());
		   }
		
		   if (unit!="mm") {
		     std::string e="Value must have unit 'mm' and not '"+unit+"'";
		
		     SemErr(e.c_str());
		   }
		
		   if (!StringToNumber(number,value)) {
		     std::string e="Cannot parse number '"+number+"'";
		
		     SemErr(e.c_str());
		   }
		
		   if (!errors->hasErrors) {
		     style.SetDoubleValue(descriptor.GetAttribute(),value);
		   }
		 }
		}
		else if (descriptor.GetType()==StyleAttributeType::TYPE_MAP_SIZE) {
		 if (valueType==ValueType::NUMBER) {
		   double value;
		
		   if (unit!="m") {
		     std::string e="Value must have unit 'm' and not '"+unit+"'";
		
		     SemErr(e.c_str());
		   }
		
		   if (!StringToNumber(number,value)) {
		     std::string e="Cannot parse number '"+number+"'";
		
		     SemErr(e.c_str());
		   }
		
		   if (!errors->hasErrors) {
		     if (negate) {
		       value=-value;
		     }
		     style.SetDoubleValue(descriptor.GetAttribute(),value);
		   }
		 }
		}
		else if (descriptor.GetType()==StyleAttributeType::TYPE_UMAP_SIZE) {
		 if (valueType==ValueType::NUMBER) {
		   double value;
		
		   if (negate) {
		     std::string e="Negative numbers not allowed here";
		
		     SemErr(e.c_str());
		   }
		
		   if (unit!="m") {
		     std::string e="Value must have unit 'm' and not '"+unit+"'";
		
		     SemErr(e.c_str());
		   }
		
		   if (!StringToNumber(number,value)) {
		     std::string e="Cannot parse number '"+number+"'";
		
		     SemErr(e.c_str());
		   }
		
		   if (!errors->hasErrors) {
		     style.SetDoubleValue(descriptor.GetAttribute(),value);
		   }
		 }
		}
		else if (descriptor.GetType()==StyleAttributeType::TYPE_DOUBLE) {
		 if (valueType==ValueType::NUMBER) {
		   double value;
		
		   if (!unit.empty()) {
		     std::string e="Value must not have a unit";
		
		     SemErr(e.c_str());
		   }
		
		   if (!StringToNumber(number,value)) {
		     std::string e="Cannot parse number '"+number+"'";
		
		     SemErr(e.c_str());
		   }
		
		   if (!errors->hasErrors) {
		     if (negate) {
		       value=-value;
		     }
		     style.SetDoubleValue(descriptor.GetAttribute(),value);
		   }
		 }
		}
		else if (descriptor.GetType()==StyleAttributeType::TYPE_UDOUBLE) {
		 if (valueType==ValueType::NUMBER) {
		   double value;
		
		   if (negate) {
		     std::string e="Negative numbers not allowed here";
		
		     SemErr(e.c_str());
		   }
		
		   if (!unit.empty()) {
		     std::string e="Value must not have unit";
		
		     SemErr(e.c_str());
		   }
		
		   if (!StringToNumber(number,value)) {
		     std::string e="Cannot parse number '"+number+"'";
		
		     SemErr(e.c_str());
		   }
		
		   if (!errors->hasErrors) {
		     style.SetDoubleValue(descriptor.GetAttribute(),value);
		   }
		 }
		}
		else if (descriptor.GetType()==StyleAttributeType::TYPE_UDOUBLE_ARRAY) {
		 if (valueType==ValueType::NUMBER) {
		   double              value;
		   std::vector<double> valueList(numberList.size()+1);
		
		   if (negate) {
		     std::string e="Negative numbers not allowed here";
		
		     SemErr(e.c_str());
		   }
		
		   if (!unit.empty()) {
		     std::string e="Value must not have unit";
		
		     SemErr(e.c_str());
		   }
		
		   if (!StringToNumber(number,value)) {
		     std::string e="Cannot parse number '"+number+"'";
		
		     SemErr(e.c_str());
		   }
		
		   valueList.push_back(value);
		
		   for (const auto number : numberList) {
		     if (!StringToNumber(number,value)) {
		       std::string e="Cannot parse number '"+number+"'";
		
		       SemErr(e.c_str());
		     }
		
		     valueList.push_back(value);
		   }
		
		   if (!errors->hasErrors) {
		     style.SetDoubleArrayValue(descriptor.GetAttribute(),valueList);
		   }
		 }
		}
		else if (descriptor.GetType()==StyleAttributeType::TYPE_LABEL) {
		 if (!subIdent.empty()) {
		   FeatureRef feature;
		
		   feature=config.GetTypeConfig()->GetFeature(ident);
		
		   if (!feature) {
		     std::string e="'"+ident+"' is not a registered feature";
		
		     SemErr(e.c_str());
		   }
		   else if (!feature->HasLabel()) {
		     std::string e="'"+ident+"' does not support labels";
		
		     SemErr(e.c_str());
		   }
		   else {
		     size_t labelIndex;
		
		     if (!feature->GetLabelIndex(subIdent,
		                                 labelIndex)) {
		       std::string e="'"+ident+"' does not have a label named '"+subIdent+"'";
		
		       SemErr(e.c_str());
		     }
		     else {
		       LabelProviderRef label;
		
		       label=std::make_shared<DynamicFeatureLabelReader>(*config.GetTypeConfig(),
		                                                         ident,
		                                                         subIdent);
		       style.SetLabelValue(descriptor.GetAttribute(),label);
		     }
		   }
		 }
		 else {
		   LabelProviderRef label;
		
		   label=config.GetLabelProvider(ident);
		
		   if (!label) {
		     std::string e="There is no label provider with name '"+ident+"' registered";
		
		     SemErr(e.c_str());
		   }
		   else {
		     style.SetLabelValue(descriptor.GetAttribute(),label);
		   }
		 }
		}
		else if (descriptor.GetType()==StyleAttributeType::TYPE_SYMBOL) {
		 if (valueType==ValueType::IDENT) {
		   SymbolRef symbol=config.GetSymbol(ident);
		
		   if (symbol && subIdent.empty()) {
		     style.SetSymbolValue(descriptor.GetAttribute(),symbol);
		   }
		   else {
		     std::string e="Map symbol '"+ident+"' is not defined";
		
		     SemErr(e.c_str());
		   }
		 }
		 else {
		   std::string e="Attribute '"+descriptor.GetName()+"' has incompatible type for value";
		
		   SemErr(e.c_str());
		 }
		}
		else if (descriptor.GetType()==StyleAttributeType::TYPE_INT) {
		 if (valueType==ValueType::NUMBER) {
		   int value;
		
		   if (!unit.empty()) {
		     std::string e="Vaue must not have a unit";
		
		     SemErr(e.c_str());
		   }
		
		   if (!StringToNumber(number,value)) {
		     std::string e="Cannot parse number '"+number+"'";
		
		     SemErr(e.c_str());
		   }
		
		   if (!errors->hasErrors) {
		     if (negate) {
		       value=-value;
		     }
		     style.SetIntValue(descriptor.GetAttribute(),value);
		   }
		 }
		 /*else if (valueType==ValueType::CONSTANT) {
		   if (!constant) {
		     // no code
		   }
		   else if (dynamic_cast<StyleConstantInt*>(constant.get())==NULL) {
		     std::string e="Constant is not of type 'INT'";
		
		     SemErr(e.c_str());
		   }
		
		   if (!errors->hasErrors) {
		     StyleConstantInt* uintConstant=dynamic_cast<StyleConstantInt*>(constant.get());
		
		     style.SetIntValue(descriptor.GetAttribute(),intConstant->GetInt());
		   }
		 }*/
		 else {
		   std::string e="Attribute '"+descriptor.GetName()+"' has incompatible type for value";
		
		   SemErr(e.c_str());
		 }
		}
		else if (descriptor.GetType()==StyleAttributeType::TYPE_UINT) {
		 if (valueType==ValueType::NUMBER) {
		   size_t value;
		
		   if (negate) {
		     std::string e="Negative numbers not allowed here";
		
		     SemErr(e.c_str());
		   }
		
		   if (!unit.empty()) {
		     std::string e="Vaue must not have a unit";
		
		     SemErr(e.c_str());
		   }
		
		   if (!StringToNumber(number,value)) {
		     std::string e="Cannot parse number '"+number+"'";
		
		     SemErr(e.c_str());
		   }
		
		   if (!errors->hasErrors) {
		     style.SetUIntValue(descriptor.GetAttribute(),value);
		   }
		 }
		 else if (valueType==ValueType::CONSTANT) {
		   if (!constant) {
		     // no code
		   }
		   else if (dynamic_cast<StyleConstantUInt*>(constant.get())==NULL) {
		     std::string e="Constant is not of type 'UINT'";
		
		     SemErr(e.c_str());
		   }
		
		   if (!errors->hasErrors) {
		     StyleConstantUInt* uintConstant=dynamic_cast<StyleConstantUInt*>(constant.get());
		
		     style.SetUIntValue(descriptor.GetAttribute(),uintConstant->GetUInt());
		   }
		 }
		 else {
		   std::string e="Attribute '"+descriptor.GetName()+"' has incompatible type for value";
		
		   SemErr(e.c_str());
		 }
		}
		
}

void Parser::COLOR_VALUE(Color& color) {
		Expect(_color);
		std::string c(t->val);
		
		if (c.length()!=7 &&
		   c.length()!=9) {
		 std::string e="Illegal color value";
		
		 SemErr(e.c_str());
		}
		
		if (!errors->hasErrors) {
		 color=osmscout::Color::FromHexString(c);
		}
		
}

void Parser::CONSTANT(StyleConstantRef& constant) {
		constant=NULL;
		
		Expect(_variable);
		constant=config.GetConstantByName(t->val+1);
		
		if (!constant) {
		 std::string e=std::string("Constant \"") + t->val + "\" not defined";
		
		 SemErr(e.c_str());
		}
		
}

void Parser::STRING(std::string& value) {
		Expect(_string);
		value=Destring(t->val);
		
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
	maxT = 62;

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

	static bool set[5][64] = {
		{T,x,x,x, x,x,x,T, x,T,x,T, T,x,x,x, x,T,x,T, x,T,T,T, x,T,x,T, x,T,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, T,T,x,T, T,T,T,T, x,x,x,x, x,x,x,x},
		{T,T,x,x, x,x,x,T, x,T,x,T, T,x,x,x, x,T,x,T, x,T,T,T, x,T,x,T, x,T,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, T,T,x,T, T,T,T,T, T,x,x,x, x,x,x,x},
		{x,x,x,x, x,x,x,x, x,x,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,T, T,T,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x},
		{x,x,x,x, x,x,x,x, x,x,T,T, x,x,x,x, x,x,x,x, x,x,x,T, x,x,x,x, x,x,x,x, x,x,x,T, x,x,x,x, x,x,x,x, x,x,x,x, T,x,x,x, T,x,x,x, x,x,x,x, x,x,x,x},
		{x,x,x,x, x,x,x,x, x,x,x,T, x,x,x,x, x,x,x,x, x,x,x,T, x,x,x,x, x,x,x,x, x,x,x,T, x,x,x,x, x,x,x,x, x,x,x,x, T,x,x,x, T,x,x,x, x,x,x,x, x,x,x,x}
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
			case 22: s = coco_string_create("\"BORDER\" expected"); break;
			case 23: s = coco_string_create("\"AREA\" expected"); break;
			case 24: s = coco_string_create("\".\" expected"); break;
			case 25: s = coco_string_create("\"POLYGON\" expected"); break;
			case 26: s = coco_string_create("\"GROUND\" expected"); break;
			case 27: s = coco_string_create("\"RECTANGLE\" expected"); break;
			case 28: s = coco_string_create("\"x\" expected"); break;
			case 29: s = coco_string_create("\"CIRCLE\" expected"); break;
			case 30: s = coco_string_create("\"CONST\" expected"); break;
			case 31: s = coco_string_create("\"COLOR\" expected"); break;
			case 32: s = coco_string_create("\"MAG\" expected"); break;
			case 33: s = coco_string_create("\"UINT\" expected"); break;
			case 34: s = coco_string_create("\"STYLE\" expected"); break;
			case 35: s = coco_string_create("\"[\" expected"); break;
			case 36: s = coco_string_create("\"]\" expected"); break;
			case 37: s = coco_string_create("\"FEATURE\" expected"); break;
			case 38: s = coco_string_create("\"PATH\" expected"); break;
			case 39: s = coco_string_create("\"TYPE\" expected"); break;
			case 40: s = coco_string_create("\"-\" expected"); break;
			case 41: s = coco_string_create("\"ONEWAY\" expected"); break;
			case 42: s = coco_string_create("\"SIZE\" expected"); break;
			case 43: s = coco_string_create("\"m\" expected"); break;
			case 44: s = coco_string_create("\"mm\" expected"); break;
			case 45: s = coco_string_create("\":\" expected"); break;
			case 46: s = coco_string_create("\"px\" expected"); break;
			case 47: s = coco_string_create("\"<\" expected"); break;
			case 48: s = coco_string_create("\"NODE\" expected"); break;
			case 49: s = coco_string_create("\"TEXT\" expected"); break;
			case 50: s = coco_string_create("\"#\" expected"); break;
			case 51: s = coco_string_create("\"ICON\" expected"); break;
			case 52: s = coco_string_create("\"WAY\" expected"); break;
			case 53: s = coco_string_create("\"SHIELD\" expected"); break;
			case 54: s = coco_string_create("\"BORDERTEXT\" expected"); break;
			case 55: s = coco_string_create("\"BORDERSYMBOL\" expected"); break;
			case 56: s = coco_string_create("\"name\" expected"); break;
			case 57: s = coco_string_create("\"lighten\" expected"); break;
			case 58: s = coco_string_create("\"darken\" expected"); break;
			case 59: s = coco_string_create("\"(\" expected"); break;
			case 60: s = coco_string_create("\")\" expected"); break;
			case 61: s = coco_string_create("\"!\" expected"); break;
			case 62: s = coco_string_create("??? expected"); break;
			case 63: s = coco_string_create("this symbol not expected in OSS"); break;
			case 64: s = coco_string_create("this symbol not expected in FLAGSECTION"); break;
			case 65: s = coco_string_create("this symbol not expected in WAYORDER"); break;
			case 66: s = coco_string_create("this symbol not expected in CONSTSECTION"); break;
			case 67: s = coco_string_create("this symbol not expected in SYMBOLSECTION"); break;
			case 68: s = coco_string_create("this symbol not expected in WAYGROUP"); break;
			case 69: s = coco_string_create("this symbol not expected in POLYGON"); break;
			case 70: s = coco_string_create("this symbol not expected in RECTANGLE"); break;
			case 71: s = coco_string_create("this symbol not expected in CIRCLE"); break;
			case 72: s = coco_string_create("this symbol not expected in AREAFILLSYMSTYLE"); break;
			case 73: s = coco_string_create("this symbol not expected in AREABORDERSYMSTYLE"); break;
			case 74: s = coco_string_create("this symbol not expected in AREASYMBOLSTYLE"); break;
			case 75: s = coco_string_create("this symbol not expected in AREASYMBOLSTYLE"); break;
			case 76: s = coco_string_create("invalid AREASYMBOLSTYLE"); break;
			case 77: s = coco_string_create("this symbol not expected in AREASYMBOLSTYLE"); break;
			case 78: s = coco_string_create("invalid UDOUBLE"); break;
			case 79: s = coco_string_create("invalid DOUBLE"); break;
			case 80: s = coco_string_create("invalid CONSTDEF"); break;
			case 81: s = coco_string_create("invalid COLOR"); break;
			case 82: s = coco_string_create("invalid MAG"); break;
			case 83: s = coco_string_create("invalid UINT"); break;
			case 84: s = coco_string_create("invalid STYLE"); break;
			case 85: s = coco_string_create("invalid STYLEDEF"); break;
			case 86: s = coco_string_create("this symbol not expected in NODESTYLEDEF"); break;
			case 87: s = coco_string_create("invalid NODESTYLEDEF"); break;
			case 88: s = coco_string_create("this symbol not expected in WAYSTYLEDEF"); break;
			case 89: s = coco_string_create("invalid WAYSTYLEDEF"); break;
			case 90: s = coco_string_create("invalid WAYSTYLEDEF"); break;
			case 91: s = coco_string_create("this symbol not expected in AREASTYLEDEF"); break;
			case 92: s = coco_string_create("invalid AREASTYLEDEF"); break;
			case 93: s = coco_string_create("invalid AREASTYLEDEF"); break;
			case 94: s = coco_string_create("this symbol not expected in NODETEXTSTYLE"); break;
			case 95: s = coco_string_create("this symbol not expected in NODETEXTSTYLE"); break;
			case 96: s = coco_string_create("this symbol not expected in NODETEXTSTYLE"); break;
			case 97: s = coco_string_create("this symbol not expected in NODEICONSTYLE"); break;
			case 98: s = coco_string_create("this symbol not expected in NODEICONSTYLE"); break;
			case 99: s = coco_string_create("this symbol not expected in NODEICONSTYLE"); break;
			case 100: s = coco_string_create("this symbol not expected in WAYSTYLE"); break;
			case 101: s = coco_string_create("this symbol not expected in WAYSTYLE"); break;
			case 102: s = coco_string_create("this symbol not expected in WAYPATHTEXTSTYLE"); break;
			case 103: s = coco_string_create("this symbol not expected in WAYPATHTEXTSTYLE"); break;
			case 104: s = coco_string_create("this symbol not expected in WAYPATHTEXTSTYLE"); break;
			case 105: s = coco_string_create("this symbol not expected in WAYPATHSYMBOLSTYLE"); break;
			case 106: s = coco_string_create("this symbol not expected in WAYPATHSYMBOLSTYLE"); break;
			case 107: s = coco_string_create("this symbol not expected in WAYPATHSYMBOLSTYLE"); break;
			case 108: s = coco_string_create("this symbol not expected in WAYSHIELDSTYLE"); break;
			case 109: s = coco_string_create("this symbol not expected in WAYSHIELDSTYLE"); break;
			case 110: s = coco_string_create("this symbol not expected in WAYSHIELDSTYLE"); break;
			case 111: s = coco_string_create("this symbol not expected in AREASTYLE"); break;
			case 112: s = coco_string_create("this symbol not expected in AREASTYLE"); break;
			case 113: s = coco_string_create("this symbol not expected in AREATEXTSTYLE"); break;
			case 114: s = coco_string_create("this symbol not expected in AREATEXTSTYLE"); break;
			case 115: s = coco_string_create("this symbol not expected in AREATEXTSTYLE"); break;
			case 116: s = coco_string_create("this symbol not expected in AREAICONSTYLE"); break;
			case 117: s = coco_string_create("this symbol not expected in AREAICONSTYLE"); break;
			case 118: s = coco_string_create("this symbol not expected in AREAICONSTYLE"); break;
			case 119: s = coco_string_create("this symbol not expected in AREABORDERSTYLE"); break;
			case 120: s = coco_string_create("this symbol not expected in AREABORDERSTYLE"); break;
			case 121: s = coco_string_create("this symbol not expected in AREABORDERSTYLE"); break;
			case 122: s = coco_string_create("this symbol not expected in AREABORDERTEXTSTYLE"); break;
			case 123: s = coco_string_create("this symbol not expected in AREABORDERTEXTSTYLE"); break;
			case 124: s = coco_string_create("this symbol not expected in AREABORDERTEXTSTYLE"); break;
			case 125: s = coco_string_create("this symbol not expected in AREABORDERSYMBOLSTYLE"); break;
			case 126: s = coco_string_create("this symbol not expected in AREABORDERSYMBOLSTYLE"); break;
			case 127: s = coco_string_create("this symbol not expected in AREABORDERSYMBOLSTYLE"); break;
			case 128: s = coco_string_create("invalid ATTRIBUTE"); break;
			case 129: s = coco_string_create("invalid ATTRIBUTEVALUE"); break;
			case 130: s = coco_string_create("invalid ATTRIBUTEVALUE"); break;
			case 131: s = coco_string_create("invalid ATTRIBUTEVALUE"); break;
			case 132: s = coco_string_create("invalid ATTRIBUTEVALUE"); break;
			case 133: s = coco_string_create("invalid ATTRIBUTEVALUE"); break;
			case 134: s = coco_string_create("invalid ATTRIBUTEVALUE"); break;

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

  log.Error() << error.line << "," << error.column << " " << "Symbol: " << error.text;

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

  log.Error() << error.line << "," << error.column << " " << "Error: " << error.text;

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

  log.Warn() << error.line << "," << error.column << " " << "Warning: " << error.text;

  errors.push_back(error);
}

void Errors::Warning(const char *s)
{
  Err error;

  error.type=Err::Warning;
  error.line=0;
  error.column=0;
  error.text=s;

  log.Warn() << error.line << "," << error.column << " " << "Warning: " << error.text;

  errors.push_back(error);
}

void Errors::Exception(const char* s)
{
  Err error;

  error.type=Err::Exception;
  error.line=0;
  error.column=0;
  error.text=s;

  log.Error() << error.line << "," << error.column << " " << "Exception: " << error.text;

  errors.push_back(error);
  hasErrors=true;
}

} // namespace
} // namespace

