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

#include <osmscoutmap/oss/Parser.h>

#include <sstream>

#include <osmscout/system/Assert.h>

#include <osmscout/util/Logger.h>
#include <osmscout/util/String.h>

#include <osmscoutmap/oss/Scanner.h>

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
		while (!(la->kind == _EOF || la->kind == 7 /* "OSS" */)) {SynErr(67); Get();}
		Expect(7 /* "OSS" */);
		state=true; 
		if (la->kind == 9 /* "MODULE" */) {
			IMPORTS();
		}
		while (la->kind == 10 /* "FLAG" */) {
			FLAGSECTION();
		}
		if (la->kind == 9 /* "MODULE" */) {
			IMPORTS();
		}
		if (la->kind == 18 /* "ORDER" */) {
			WAYORDER();
		}
		if (la->kind == 9 /* "MODULE" */) {
			IMPORTS();
		}
		while (la->kind == 31 /* "CONST" */) {
			CONSTSECTION();
		}
		if (la->kind == 9 /* "MODULE" */) {
			IMPORTS();
		}
		while (la->kind == 22 /* "SYMBO" */) {
			SYMBOLSECTION();
		}
		if (la->kind == 9 /* "MODULE" */) {
			IMPORTS();
		}
		while (la->kind == 38 /* "STYLE" */) {
			STYLESECTION();
		}
		if (la->kind == 9 /* "MODULE" */) {
			IMPORTS();
		}
		Expect(8 /* "END" */);
}

void Parser::IMPORTS() {
		IMPORT();
		while (la->kind == 9 /* "MODULE" */) {
			IMPORT();
		}
}

void Parser::FLAGSECTION() {
		while (!(la->kind == _EOF || la->kind == 10 /* "FLAG" */)) {SynErr(68); Get();}
		Expect(10 /* "FLAG" */);
		FLAGBLOCK(true);
}

void Parser::WAYORDER() {
		while (!(la->kind == _EOF || la->kind == 18 /* "ORDER" */)) {SynErr(69); Get();}
		Expect(18 /* "ORDER" */);
		Expect(19 /* "WAYS" */);
		size_t priority=1;
		while (la->kind == 20 /* "GROUP" */) {
			WAYGROUP(priority);
			priority++;
		}
}

void Parser::CONSTSECTION() {
		while (!(la->kind == _EOF || la->kind == 31 /* "CONST" */)) {SynErr(70); Get();}
		Expect(31 /* "CONST" */);
		CONSTBLOCK(true);
}

void Parser::SYMBOLSECTION() {
		while (!(la->kind == _EOF || la->kind == 22 /* "SYMBO" */)) {SynErr(71); Get();}
		Expect(22 /* "SYMBO" */);
		std::string name;
		
		IDENT(name);
		SymbolRef symbol=std::make_shared<Symbol>(name);
		
		while (la->kind == 26 /* "POLYGON" */ || la->kind == 28 /* "RECTANGLE" */ || la->kind == 30 /* "CIRCLE" */) {
			if (la->kind == 26 /* "POLYGON" */) {
				POLYGON(*symbol);
			} else if (la->kind == 28 /* "RECTANGLE" */) {
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
		Expect(38 /* "STYLE" */);
		StyleFilter filter; 
		STYLEBLOCK(filter,true);
}

void Parser::IMPORT() {
		while (!(la->kind == _EOF || la->kind == 9 /* "MODULE" */)) {SynErr(72); Get();}
		Expect(9 /* "MODULE" */);
		std::string moduleName; 
		STRING(moduleName);
		std::string directory=osmscout::GetDirectory(filename);
		
		std::string moduleFileName;
		
		if (!directory.empty()) {
		 moduleFileName=osmscout::AppendFileToDir(directory,moduleName)+".oss";
		}
		else {
		 moduleFileName=moduleName+".ost";
		}
		
		bool success=config.Load(moduleFileName,
		                        colorPostprocessor);
		
		if (!success) {
		 std::string e="Cannot load module '"+moduleFileName+"'";
		
		 SemErr(e.c_str());
		}
		
}

void Parser::STRING(std::string& value) {
		Expect(_string);
		value=Destring(t->val);
		
}

void Parser::FLAGBLOCK(bool state) {
		while (la->kind == _ident || la->kind == 11 /* "IF" */) {
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
		Expect(16 /* "=" */);
		BOOL(value);
		if (state) {
		 if (!config.HasFlag(name)) {
		   config.AddFlag(name,value);
		 }
		}
		
		Expect(17 /* ";" */);
}

void Parser::FLAGCONDBLOCK(bool state) {
		bool newState=state;
		bool executed=false;
		
		Expect(11 /* "IF" */);
		IFCOND(newState);
		Expect(12 /* "{" */);
		this->state=state && newState;
		
		FLAGBLOCK(state && newState);
		this->state=state;
		executed=newState;
		
		Expect(13 /* "}" */);
		while (la->kind == 14 /* "ELIF" */) {
			Get();
			IFCOND(newState);
			Expect(12 /* "{" */);
			this->state=!executed && state && newState;
			
			FLAGBLOCK(!executed && state && newState);
			this->state=state;
			executed=newState;
			
			Expect(13 /* "}" */);
		}
		if (la->kind == 15 /* "ELSE" */) {
			Get();
			Expect(12 /* "{" */);
			this->state=!executed && state;
			
			FLAGBLOCK(!executed && state);
			this->state=state;
			
			Expect(13 /* "}" */);
		}
}

void Parser::IFCOND(bool& state) {
		std::string flag;
		bool        negate=false;
		
		if (la->kind == 65 /* "!" */) {
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
		while (!(la->kind == _EOF || la->kind == 20 /* "GROUP" */)) {SynErr(73); Get();}
		Expect(20 /* "GROUP" */);
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
		while (la->kind == 21 /* "," */) {
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
		while (!(la->kind == _EOF || la->kind == 26 /* "POLYGON" */)) {SynErr(74); Get();}
		Expect(26 /* "POLYGON" */);
		StyleFilter         filter;
		DrawPrimitive::ProjectionMode projectionMode=DrawPrimitive::ProjectionMode::MAP;
		FillPartialStyle    fillStyle;
		BorderPartialStyle  borderStyle;
		PolygonPrimitiveRef polygon;
		Vertex2D            coord;
		
		if (la->kind == 27 /* "GROUND" */) {
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
		while (la->kind == _number || la->kind == _double || la->kind == 44 /* "-" */) {
			COORD(coord);
			polygon->AddCoord(coord); 
		}
		AREASYMBOLSTYLE(fillStyle,borderStyle);
		symbol.AddPrimitive(polygon); 
}

void Parser::RECTANGLE(Symbol& symbol) {
		while (!(la->kind == _EOF || la->kind == 28 /* "RECTANGLE" */)) {SynErr(75); Get();}
		Expect(28 /* "RECTANGLE" */);
		StyleFilter        filter;
		DrawPrimitive::ProjectionMode projectionMode=DrawPrimitive::ProjectionMode::MAP;
		FillPartialStyle   fillStyle;
		BorderPartialStyle borderStyle;
		Vertex2D           topLeft;
		double             width;
		double             height;
		
		if (la->kind == 27 /* "GROUND" */) {
			Get();
			projectionMode=DrawPrimitive::ProjectionMode::GROUND; 
		}
		COORD(topLeft);
		UDOUBLE(width);
		Expect(29 /* "x" */);
		UDOUBLE(height);
		AREASYMBOLSTYLE(fillStyle,borderStyle);
		symbol.AddPrimitive(std::make_shared<RectanglePrimitive>(projectionMode,
		                                                        topLeft,
		                                                        width,height,
		                                                        fillStyle.style,
		                                                        borderStyle.style));
		
}

void Parser::CIRCLE(Symbol& symbol) {
		while (!(la->kind == _EOF || la->kind == 30 /* "CIRCLE" */)) {SynErr(76); Get();}
		Expect(30 /* "CIRCLE" */);
		StyleFilter                   filter;
		DrawPrimitive::ProjectionMode projectionMode=DrawPrimitive::ProjectionMode::MAP;
		FillPartialStyle              fillStyle;
		BorderPartialStyle            borderStyle;
		Vertex2D                      center;
		double                        radius;
		
		if (la->kind == 27 /* "GROUND" */) {
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
		while (!(la->kind == _EOF || la->kind == 12 /* "{" */)) {SynErr(77); Get();}
		Expect(12 /* "{" */);
		while (la->kind == _ident || la->kind == 59 /* "name" */) {
			FILLSTYLEATTR(fillStyle);
			ExpectWeak(17 /* ";" */, 1);
		}
		Expect(13 /* "}" */);
}

void Parser::FILLSTYLEATTR(FillPartialStyle& style) {
		ATTRIBUTE(style,*FillStyle::GetDescriptor());
}

void Parser::AREABORDERSYMSTYLE(BorderPartialStyle& borderStyle) {
		while (!(la->kind == _EOF || la->kind == 23 /* "BORDER" */)) {SynErr(78); Get();}
		Expect(23 /* "BORDER" */);
		Expect(12 /* "{" */);
		while (la->kind == _ident || la->kind == 59 /* "name" */) {
			BORDERSTYLEATTR(borderStyle);
			ExpectWeak(17 /* ";" */, 1);
		}
		Expect(13 /* "}" */);
}

void Parser::BORDERSTYLEATTR(BorderPartialStyle& style) {
		ATTRIBUTE(style,*BorderStyle::GetDescriptor());
}

void Parser::AREASYMBOLSTYLE(FillPartialStyle& fillStyle, BorderPartialStyle& borderStyle) {
		while (!(la->kind == _EOF || la->kind == 12 /* "{" */)) {SynErr(79); Get();}
		Expect(12 /* "{" */);
		while (la->kind == 24 /* "AREA" */) {
			while (!(la->kind == _EOF || la->kind == 24 /* "AREA" */)) {SynErr(80); Get();}
			Get();
			if (la->kind == 12 /* "{" */) {
				AREAFILLSYMSTYLE(fillStyle);
			} else if (la->kind == 25 /* "." */) {
				Get();
				AREABORDERSYMSTYLE(borderStyle);
			} else SynErr(81);
		}
		while (!(la->kind == _EOF || la->kind == 13 /* "}" */)) {SynErr(82); Get();}
		Expect(13 /* "}" */);
}

void Parser::COORD(Vertex2D& coord) {
		double x;
		double y;
		
		DOUBLE(x);
		Expect(21 /* "," */);
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
			
		} else SynErr(83);
}

void Parser::DOUBLE(double& value) {
		bool negate=false; 
		if (la->kind == 44 /* "-" */) {
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
			
		} else SynErr(84);
		if (negate) {
		 value=-value;
		}
		
}

void Parser::CONSTBLOCK(bool state) {
		while (StartOf(2)) {
			if (la->kind == 11 /* "IF" */) {
				CONSTCONDBLOCK(state);
			} else {
				CONSTDEF();
				Expect(17 /* ";" */);
			}
		}
}

void Parser::CONSTCONDBLOCK(bool state) {
		bool newState=state;
		bool executed=false;
		
		Expect(11 /* "IF" */);
		IFCOND(newState);
		Expect(12 /* "{" */);
		this->state=state && newState;
		
		CONSTBLOCK(state && newState);
		this->state=state;
		executed=newState;
		
		Expect(13 /* "}" */);
		while (la->kind == 14 /* "ELIF" */) {
			Get();
			IFCOND(newState);
			Expect(12 /* "{" */);
			this->state=!executed && state && newState;
			
			CONSTBLOCK(!executed && state && newState);
			this->state=state;
			executed=newState;
			
			Expect(13 /* "}" */);
		}
		if (la->kind == 15 /* "ELSE" */) {
			Get();
			Expect(12 /* "{" */);
			this->state=!executed && state;
			
			CONSTBLOCK(!executed && state);
			this->state=state;
			
			Expect(13 /* "}" */);
		}
}

void Parser::CONSTDEF() {
		if (la->kind == 32 /* "COLOR" */) {
			COLORCONSTDEF();
		} else if (la->kind == 33 /* "MAG" */) {
			MAGCONSTDEF();
		} else if (la->kind == 34 /* "UINT" */) {
			UINTCONSTDEF();
		} else if (la->kind == 35 /* "WIDTH" */) {
			WIDTHCONSTDEF();
		} else SynErr(85);
}

void Parser::COLORCONSTDEF() {
		std::string      name;
		StyleConstantRef constant;
		Color            color;
		
		Expect(32 /* "COLOR" */);
		IDENT(name);
		Expect(16 /* "=" */);
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
		
		Expect(33 /* "MAG" */);
		IDENT(name);
		Expect(16 /* "=" */);
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
		
		Expect(34 /* "UINT" */);
		IDENT(name);
		Expect(16 /* "=" */);
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

void Parser::WIDTHCONSTDEF() {
		std::string              name;
		StyleConstantRef         constant;
		double                   width;
		std::string              unitValue;
		StyleConstantWidth::Unit unit;
		
		Expect(35 /* "WIDTH" */);
		IDENT(name);
		Expect(16 /* "=" */);
		DOUBLE(width);
		if (la->kind == _ident) {
			IDENT(unitValue);
		} else if (la->kind == 36 /* "mm" */) {
			Get();
			unitValue="mm"; 
		} else if (la->kind == 37 /* "m" */) {
			Get();
			unitValue="m"; 
		} else SynErr(86);
		if (state) {
		 constant=config.GetConstantByName(name);
		
		 if (constant) {
		   std::string e="Constant already defined";
		
		   SemErr(e.c_str());
		 }
		
		 if (unitValue=="mm") {
		   unit=StyleConstantWidth::Unit::mm;
		 }
		 else if (unitValue=="m") {
		   unit=StyleConstantWidth::Unit::m;
		 }
		 else {
		   std::string e="Unsupported unit '"+unitValue+"'";
		   SemErr(e.c_str());
		 }
		
		 if (!errors->hasErrors) {
		   config.AddConstant(name,std::make_shared<StyleConstantWidth>(width,unit));
		 }
		}
		
}

void Parser::COLOR(Color& color) {
		StyleConstantRef constant;
		double           factor;
		
		if (la->kind == 60 /* "lighten" */) {
			Get();
			Expect(63 /* "(" */);
			COLOR(color);
			Expect(21 /* "," */);
			UDOUBLE(factor);
			Expect(64 /* ")" */);
			if (factor>=0.0 && factor<=1.0) {
			 color=PostprocessColor(color.Lighten(factor));
			}
			else {
			std::string e="Factor must be in the range [0..1]";
			
			 SemErr(e.c_str());
			}
			
		} else if (la->kind == 61 /* "darken" */) {
			double factor; 
			Get();
			Expect(63 /* "(" */);
			COLOR(color);
			Expect(21 /* "," */);
			UDOUBLE(factor);
			Expect(64 /* ")" */);
			if (factor>=0.0 && factor<=1.0) {
			 color=PostprocessColor(color.Darken(factor));
			}
			else {
			std::string e="Factor must be in the range [0..1]";
			
			 SemErr(e.c_str());
			}
			
		} else if (la->kind == _color) {
			COLOR_VALUE(color);
			color=PostprocessColor(color);
			
		} else if (la->kind == _variable) {
			CONSTANT(constant);
			if (!constant) {
			 color=Color::BLACK;
			}
			else if (dynamic_cast<StyleConstantColor*>(constant.get())==nullptr) {
			 std::string e="Constant is not of type 'COLOR'";
			
			 SemErr(e.c_str());
			
			 color=Color::BLACK;
			}
			else {
			 StyleConstantColor* colorConstant=dynamic_cast<StyleConstantColor*>(constant.get());
			
			 color=colorConstant->GetColor();
			}
			
		} else SynErr(87);
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
			uint32_t level; 
			Get();
			if (!StringToNumber(t->val,level)) {
			 std::string e="Cannot parse number '"+std::string(t->val)+"'";
			
			 SemErr(e.c_str());
			}
			else {
			 magnification.SetLevel(osmscout::MagnificationLevel(level));
			}
			
		} else if (la->kind == _variable) {
			StyleConstantRef constant; 
			CONSTANT(constant);
			if (!constant) {
			 magnification.SetLevel(osmscout::MagnificationLevel(0));
			}
			else if (dynamic_cast<StyleConstantMag*>(constant.get())==nullptr) {
			 std::string e="Variable is not of type 'MAG'";
			
			 SemErr(e.c_str());
			}
			else {
			 StyleConstantMag* magConstant=dynamic_cast<StyleConstantMag*>(constant.get());
			
			 magnification=magConstant->GetMag();
			}
			
		} else SynErr(88);
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
			else if (dynamic_cast<StyleConstantUInt*>(constant.get())==nullptr) {
			 std::string e="Constant is not of type 'UINT'";
			
			 SemErr(e.c_str());
			}
			
			if (!errors->hasErrors) {
			 StyleConstantUInt* uintConstant=dynamic_cast<StyleConstantUInt*>(constant.get());
			
			 value=uintConstant->GetUInt();
			}
			
		} else SynErr(89);
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
		if (la->kind == 39 /* "[" */) {
			STYLEFILTER(filter);
		}
		if (la->kind == 12 /* "{" */) {
			Get();
			STYLEBLOCK(filter,state);
			Expect(13 /* "}" */);
		} else if (StartOf(5)) {
			STYLEDEF(filter,state);
		} else SynErr(90);
}

void Parser::STYLECONDBLOCK(StyleFilter filter, bool state) {
		bool newState=state;
		bool executed=false;
		
		Expect(11 /* "IF" */);
		IFCOND(newState);
		Expect(12 /* "{" */);
		this->state=state && newState;
		
		STYLEBLOCK(filter,state && newState);
		this->state=state;
		executed=newState;
		
		Expect(13 /* "}" */);
		while (la->kind == 14 /* "ELIF" */) {
			Get();
			IFCOND(newState);
			Expect(12 /* "{" */);
			this->state=!executed && state && newState;
			
			STYLEBLOCK(filter,!executed && state && newState);
			this->state=state;
			executed=newState;
			
			Expect(13 /* "}" */);
		}
		if (la->kind == 15 /* "ELSE" */) {
			Get();
			Expect(12 /* "{" */);
			this->state=!executed && state;
			
			STYLEBLOCK(filter,!executed && state);
			this->state=state;
			
			Expect(13 /* "}" */);
		}
}

void Parser::STYLEFILTER(StyleFilter& filter) {
		Expect(39 /* "[" */);
		if (la->kind == 20 /* "GROUP" */) {
			STYLEFILTER_GROUP(filter);
		}
		if (la->kind == 41 /* "FEATURE" */) {
			STYLEFILTER_FEATURE(filter);
		}
		if (la->kind == 42 /* "PATH" */) {
			STYLEFILTER_PATH(filter);
		}
		if (la->kind == 43 /* "TYPE" */) {
			STYLEFILTER_TYPE(filter);
		}
		if (la->kind == 33 /* "MAG" */) {
			STYLEFILTER_MAG(filter);
		}
		if (la->kind == 45 /* "ONEWAY" */) {
			STYLEFILTER_ONEWAY(filter);
		}
		if (la->kind == 46 /* "SIZE" */) {
			STYLEFILTER_SIZE(filter);
		}
		Expect(40 /* "]" */);
}

void Parser::STYLEDEF(StyleFilter filter, bool state) {
		if (la->kind == 50 /* "NODE" */) {
			NODESTYLEDEF(filter,state);
		} else if (la->kind == 54 /* "WAY" */) {
			WAYSTYLEDEF(filter,state);
		} else if (la->kind == 24 /* "AREA" */) {
			AREASTYLEDEF(filter,state);
		} else if (la->kind == 58 /* "ROUTE" */) {
			ROUTESTYLEDEF(filter,state);
		} else SynErr(91);
}

void Parser::STYLEFILTER_GROUP(StyleFilter& filter) {
		TypeInfoSet types;
		std::string groupName;
		
		Expect(20 /* "GROUP" */);
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
		
		while (la->kind == 21 /* "," */) {
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
		
		Expect(41 /* "FEATURE" */);
		STYLEFILTER_FEATURE_ENTRY(filter,types);
		while (la->kind == 21 /* "," */) {
			Get();
			STYLEFILTER_FEATURE_ENTRY(filter,types);
		}
		filter.SetTypes(types); 
}

void Parser::STYLEFILTER_PATH(StyleFilter& filter) {
		TypeInfoSet types;
		
		Expect(42 /* "PATH" */);
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
		
		Expect(43 /* "TYPE" */);
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
		
		while (la->kind == 21 /* "," */) {
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
		Expect(33 /* "MAG" */);
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
		Expect(44 /* "-" */);
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
		Expect(45 /* "ONEWAY" */);
		filter.SetOneway(true);
		
}

void Parser::STYLEFILTER_SIZE(StyleFilter& filter) {
		SizeConditionRef sizeCondition; 
		Expect(46 /* "SIZE" */);
		SIZECONDITION(sizeCondition);
		filter.SetSizeCondition(sizeCondition); 
}

void Parser::STYLEFILTER_FEATURE_ENTRY(StyleFilter& filter, TypeInfoSet& types) {
		std::string featureName;
		std::string flagName;
		
		IDENT(featureName);
		if (la->kind == 25 /* "." */) {
			Get();
			IDENT(flagName);
		}
		AddFeatureToFilter(filter,
		                  featureName,
		                  flagName,
		                  types);
		
}

void Parser::SIZECONDITION(SizeConditionRef& condition) {
		double            widthInMeter;
		StyleConstantRef constant;
		
		condition=std::make_shared<SizeCondition>();
		
		UMAP(widthInMeter);
		if (la->kind == _number || la->kind == _double || la->kind == 47 /* ":" */) {
			if (la->kind == _number || la->kind == _double) {
				double minMM; 
				UDOUBLE(minMM);
				Expect(36 /* "mm" */);
				if (widthInMeter>0.0) {
				 condition->SetMinMM(minMM/widthInMeter);
				}
				
			}
			Expect(47 /* ":" */);
			if (la->kind == _number || la->kind == _double) {
				double minPx; 
				UDOUBLE(minPx);
				Expect(48 /* "px" */);
				if (widthInMeter>0.0) {
				 condition->SetMinPx(minPx/widthInMeter);
				}
				
			}
		}
		Expect(49 /* "<" */);
		if (la->kind == _number || la->kind == _double || la->kind == 47 /* ":" */) {
			if (la->kind == _number || la->kind == _double) {
				double maxMM; 
				UDOUBLE(maxMM);
				Expect(36 /* "mm" */);
				if (widthInMeter>0.0) {
				 condition->SetMaxMM(maxMM/widthInMeter);
				}
				
			}
			Expect(47 /* ":" */);
			if (la->kind == _number || la->kind == _double) {
				double maxPx; 
				UDOUBLE(maxPx);
				Expect(48 /* "px" */);
				if (widthInMeter>0.0) {
				 condition->SetMaxPx(maxPx/widthInMeter);
				}
				
			}
		}
}

void Parser::UMAP(double& width) {
		StyleConstantRef constant;
		
		if (la->kind == _number || la->kind == _double) {
			UDOUBLE(width);
			Expect(37 /* "m" */);
		} else if (la->kind == _variable) {
			CONSTANT(constant);
		} else SynErr(92);
		if (constant) {
		 if (dynamic_cast<StyleConstantWidth*>(constant.get())==nullptr) {
		   std::string e="Constant is not of type 'WIDTH'";
		
		   SemErr(e.c_str());
		 }
		
		 if (!errors->hasErrors) {
		   StyleConstantWidth* widthConstant=dynamic_cast<StyleConstantWidth*>(constant.get());
		
		   if (widthConstant->GetUnit()!=StyleConstantWidth::Unit::m) {
		     std::string e="Constant is not of unit 'm'";
		
		     SemErr(e.c_str());
		   }
		   else {
		     width=widthConstant->GetWidth();
		   }
		 }
		}
		
		if (width<0.0) {
		 std::string e="Width must be >= 0.0";
		
		 SemErr(e.c_str());
		}
		
}

void Parser::NODESTYLEDEF(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 50 /* "NODE" */)) {SynErr(93); Get();}
		Expect(50 /* "NODE" */);
		Expect(25 /* "." */);
		if (la->kind == 51 /* "TEXT" */) {
			NODETEXTSTYLE(filter,state);
		} else if (la->kind == 53 /* "ICON" */) {
			NODEICONSTYLE(filter,state);
		} else SynErr(94);
}

void Parser::WAYSTYLEDEF(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 54 /* "WAY" */)) {SynErr(95); Get();}
		Expect(54 /* "WAY" */);
		if (la->kind == 12 /* "{" */ || la->kind == 52 /* "#" */) {
			WAYSTYLE(filter,state);
		} else if (la->kind == 25 /* "." */) {
			Get();
			if (la->kind == 51 /* "TEXT" */) {
				WAYPATHTEXTSTYLE(filter,state);
			} else if (la->kind == 22 /* "SYMBO" */) {
				WAYPATHSYMBOLSTYLE(filter,state);
			} else if (la->kind == 55 /* "SHIELD" */) {
				WAYSHIELDSTYLE(filter,state);
			} else SynErr(96);
		} else SynErr(97);
}

void Parser::AREASTYLEDEF(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 24 /* "AREA" */)) {SynErr(98); Get();}
		Expect(24 /* "AREA" */);
		if (la->kind == 12 /* "{" */) {
			AREASTYLE(filter,state);
		} else if (la->kind == 25 /* "." */) {
			Get();
			if (la->kind == 51 /* "TEXT" */) {
				AREATEXTSTYLE(filter,state);
			} else if (la->kind == 53 /* "ICON" */) {
				AREAICONSTYLE(filter,state);
			} else if (la->kind == 23 /* "BORDER" */) {
				AREABORDERSTYLE(filter,state);
			} else if (la->kind == 56 /* "BORDERTEXT" */) {
				AREABORDERTEXTSTYLE(filter,state);
			} else if (la->kind == 57 /* "BORDERSYMBO" */) {
				AREABORDERSYMBOLSTYLE(filter,state);
			} else SynErr(99);
		} else SynErr(100);
}

void Parser::ROUTESTYLEDEF(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 58 /* "ROUTE" */)) {SynErr(101); Get();}
		Expect(58 /* "ROUTE" */);
		if (la->kind == 12 /* "{" */) {
			ROUTESTYLE(filter,state);
		} else if (la->kind == 25 /* "." */) {
			Get();
			ROUTEPATHTEXTSTYLE(filter,state);
		} else SynErr(102);
}

void Parser::NODETEXTSTYLE(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 51 /* "TEXT" */)) {SynErr(103); Get();}
		Expect(51 /* "TEXT" */);
		TextPartialStyle style;
		std::string      slot;
		
		if (la->kind == 52 /* "#" */) {
			Get();
			IDENT(slot);
			style.style->SetSlot(slot); 
		}
		while (!(la->kind == _EOF || la->kind == 12 /* "{" */)) {SynErr(104); Get();}
		Expect(12 /* "{" */);
		while (la->kind == _ident || la->kind == 59 /* "name" */) {
			TEXTSTYLEATTR(style);
			ExpectWeak(17 /* ";" */, 1);
		}
		while (!(la->kind == _EOF || la->kind == 13 /* "}" */)) {SynErr(105); Get();}
		Expect(13 /* "}" */);
		if (state) {
		 config.AddNodeTextStyle(filter,style);
		}
		
}

void Parser::NODEICONSTYLE(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 53 /* "ICON" */)) {SynErr(106); Get();}
		Expect(53 /* "ICON" */);
		IconPartialStyle style;
		
		while (!(la->kind == _EOF || la->kind == 12 /* "{" */)) {SynErr(107); Get();}
		Expect(12 /* "{" */);
		while (la->kind == _ident || la->kind == 59 /* "name" */) {
			ICONSTYLEATTR(style);
			ExpectWeak(17 /* ";" */, 1);
		}
		while (!(la->kind == _EOF || la->kind == 13 /* "}" */)) {SynErr(108); Get();}
		Expect(13 /* "}" */);
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
		
		if (la->kind == 52 /* "#" */) {
			Get();
			IDENT(slot);
			style.style->SetSlot(slot); 
		}
		while (!(la->kind == _EOF || la->kind == 12 /* "{" */)) {SynErr(109); Get();}
		Expect(12 /* "{" */);
		while (la->kind == _ident || la->kind == 59 /* "name" */) {
			LINESTYLEATTR(style);
			ExpectWeak(17 /* ";" */, 1);
		}
		while (!(la->kind == _EOF || la->kind == 13 /* "}" */)) {SynErr(110); Get();}
		Expect(13 /* "}" */);
		if (state) {
		 config.AddWayLineStyle(filter,style);
		}
		
}

void Parser::WAYPATHTEXTSTYLE(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 51 /* "TEXT" */)) {SynErr(111); Get();}
		Expect(51 /* "TEXT" */);
		PathTextPartialStyle style;
		
		while (!(la->kind == _EOF || la->kind == 12 /* "{" */)) {SynErr(112); Get();}
		Expect(12 /* "{" */);
		while (la->kind == _ident || la->kind == 59 /* "name" */) {
			PATHTEXTSTYLEATTR(style);
			ExpectWeak(17 /* ";" */, 1);
		}
		while (!(la->kind == _EOF || la->kind == 13 /* "}" */)) {SynErr(113); Get();}
		Expect(13 /* "}" */);
		if (state) {
		 config.AddWayPathTextStyle(filter,style);
		}
		
}

void Parser::WAYPATHSYMBOLSTYLE(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 22 /* "SYMBO" */)) {SynErr(114); Get();}
		Expect(22 /* "SYMBO" */);
		PathSymbolPartialStyle style;
		std::string      slot;
		
		if (la->kind == 52 /* "#" */) {
			Get();
			IDENT(slot);
			style.style->SetSlot(slot); 
		}
		while (!(la->kind == _EOF || la->kind == 12 /* "{" */)) {SynErr(115); Get();}
		Expect(12 /* "{" */);
		while (la->kind == _ident || la->kind == 59 /* "name" */) {
			PATHSYMBOLSTYLEATTR(style);
			ExpectWeak(17 /* ";" */, 1);
		}
		while (!(la->kind == _EOF || la->kind == 13 /* "}" */)) {SynErr(116); Get();}
		Expect(13 /* "}" */);
		if (state) {
		 config.AddWayPathSymbolStyle(filter,style);
		}
		
}

void Parser::WAYSHIELDSTYLE(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 55 /* "SHIELD" */)) {SynErr(117); Get();}
		Expect(55 /* "SHIELD" */);
		PathShieldPartialStyle style;
		
		while (!(la->kind == _EOF || la->kind == 12 /* "{" */)) {SynErr(118); Get();}
		Expect(12 /* "{" */);
		while (la->kind == _ident || la->kind == 59 /* "name" */) {
			PATHSHIELDSTYLEATTR(style);
			ExpectWeak(17 /* ";" */, 1);
		}
		while (!(la->kind == _EOF || la->kind == 13 /* "}" */)) {SynErr(119); Get();}
		Expect(13 /* "}" */);
		if (state) {
		 config.AddWayPathShieldStyle(filter,style);
		}
		
}

void Parser::LINESTYLEATTR(LinePartialStyle& style) {
		ATTRIBUTE(style,*LineStyle::GetDescriptor());
}

void Parser::ROUTEPATHTEXTSTYLE(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 51 /* "TEXT" */)) {SynErr(120); Get();}
		Expect(51 /* "TEXT" */);
		PathTextPartialStyle style;
		
		while (!(la->kind == _EOF || la->kind == 12 /* "{" */)) {SynErr(121); Get();}
		Expect(12 /* "{" */);
		while (la->kind == _ident || la->kind == 59 /* "name" */) {
			PATHTEXTSTYLEATTR(style);
			ExpectWeak(17 /* ";" */, 1);
		}
		while (!(la->kind == _EOF || la->kind == 13 /* "}" */)) {SynErr(122); Get();}
		Expect(13 /* "}" */);
		if (state) {
		 config.AddRoutePathTextStyle(filter,style);
		}
		
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
		
		while (!(la->kind == _EOF || la->kind == 12 /* "{" */)) {SynErr(123); Get();}
		Expect(12 /* "{" */);
		while (la->kind == _ident || la->kind == 59 /* "name" */) {
			FILLSTYLEATTR(style);
			ExpectWeak(17 /* ";" */, 1);
		}
		while (!(la->kind == _EOF || la->kind == 13 /* "}" */)) {SynErr(124); Get();}
		Expect(13 /* "}" */);
		if (state) {
		 config.AddAreaFillStyle(filter,style);
		}
		
}

void Parser::AREATEXTSTYLE(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 51 /* "TEXT" */)) {SynErr(125); Get();}
		Expect(51 /* "TEXT" */);
		TextPartialStyle style;
		std::string      slot;
		
		if (la->kind == 52 /* "#" */) {
			Get();
			IDENT(slot);
			style.style->SetSlot(slot); 
		}
		while (!(la->kind == _EOF || la->kind == 12 /* "{" */)) {SynErr(126); Get();}
		Expect(12 /* "{" */);
		while (la->kind == _ident || la->kind == 59 /* "name" */) {
			TEXTSTYLEATTR(style);
			ExpectWeak(17 /* ";" */, 1);
		}
		while (!(la->kind == _EOF || la->kind == 13 /* "}" */)) {SynErr(127); Get();}
		Expect(13 /* "}" */);
		if (state) {
		 config.AddAreaTextStyle(filter,style);
		}
		
}

void Parser::AREAICONSTYLE(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 53 /* "ICON" */)) {SynErr(128); Get();}
		Expect(53 /* "ICON" */);
		IconPartialStyle style;
		
		while (!(la->kind == _EOF || la->kind == 12 /* "{" */)) {SynErr(129); Get();}
		Expect(12 /* "{" */);
		while (la->kind == _ident || la->kind == 59 /* "name" */) {
			ICONSTYLEATTR(style);
			ExpectWeak(17 /* ";" */, 1);
		}
		while (!(la->kind == _EOF || la->kind == 13 /* "}" */)) {SynErr(130); Get();}
		Expect(13 /* "}" */);
		if (state) {
		 config.AddAreaIconStyle(filter,style);
		}
		
}

void Parser::AREABORDERSTYLE(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 23 /* "BORDER" */)) {SynErr(131); Get();}
		Expect(23 /* "BORDER" */);
		BorderPartialStyle style;
		std::string        slot;
		
		if (la->kind == 52 /* "#" */) {
			Get();
			IDENT(slot);
			style.style->SetSlot(slot); 
		}
		while (!(la->kind == _EOF || la->kind == 12 /* "{" */)) {SynErr(132); Get();}
		Expect(12 /* "{" */);
		while (la->kind == _ident || la->kind == 59 /* "name" */) {
			BORDERSTYLEATTR(style);
			ExpectWeak(17 /* ";" */, 1);
		}
		while (!(la->kind == _EOF || la->kind == 13 /* "}" */)) {SynErr(133); Get();}
		Expect(13 /* "}" */);
		if (state) {
		 config.AddAreaBorderStyle(filter,style);
		}
		
}

void Parser::AREABORDERTEXTSTYLE(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 56 /* "BORDERTEXT" */)) {SynErr(134); Get();}
		Expect(56 /* "BORDERTEXT" */);
		PathTextPartialStyle style;
		
		while (!(la->kind == _EOF || la->kind == 12 /* "{" */)) {SynErr(135); Get();}
		Expect(12 /* "{" */);
		while (la->kind == _ident || la->kind == 59 /* "name" */) {
			PATHTEXTSTYLEATTR(style);
			ExpectWeak(17 /* ";" */, 1);
		}
		while (!(la->kind == _EOF || la->kind == 13 /* "}" */)) {SynErr(136); Get();}
		Expect(13 /* "}" */);
		if (state) {
		 config.AddAreaBorderTextStyle(filter,style);
		}
		
}

void Parser::AREABORDERSYMBOLSTYLE(StyleFilter filter, bool state) {
		while (!(la->kind == _EOF || la->kind == 57 /* "BORDERSYMBO" */)) {SynErr(137); Get();}
		Expect(57 /* "BORDERSYMBO" */);
		PathSymbolPartialStyle style;
		
		while (!(la->kind == _EOF || la->kind == 12 /* "{" */)) {SynErr(138); Get();}
		Expect(12 /* "{" */);
		while (la->kind == _ident || la->kind == 59 /* "name" */) {
			PATHSYMBOLSTYLEATTR(style);
			ExpectWeak(17 /* ";" */, 1);
		}
		while (!(la->kind == _EOF || la->kind == 13 /* "}" */)) {SynErr(139); Get();}
		Expect(13 /* "}" */);
		if (state) {
		 config.AddAreaBorderSymbolStyle(filter,style);
		}
		
}

void Parser::ROUTESTYLE(StyleFilter filter, bool state) {
		LinePartialStyle style;
		std::string      slot;
		
		while (!(la->kind == _EOF || la->kind == 12 /* "{" */)) {SynErr(140); Get();}
		Expect(12 /* "{" */);
		while (la->kind == _ident || la->kind == 59 /* "name" */) {
			LINESTYLEATTR(style);
			ExpectWeak(17 /* ";" */, 1);
		}
		while (!(la->kind == _EOF || la->kind == 13 /* "}" */)) {SynErr(141); Get();}
		Expect(13 /* "}" */);
		if (state) {
		 config.AddRouteLineStyle(filter,style);
		}
		
}

void Parser::ATTRIBUTE(PartialStyleBase& style, const StyleDescriptor& descriptor) {
		std::string                 attributeName;
		StyleAttributeDescriptorRef attributeDescriptor;
		
		if (la->kind == _ident) {
			IDENT(attributeName);
		} else if (la->kind == 59 /* "name" */) {
			Get();
			attributeName="name"; 
		} else SynErr(142);
		attributeDescriptor=descriptor.GetAttribute(attributeName);
		
		if (!attributeDescriptor) {
		 std::string e="'"+attributeName+"' is not a known attribute of the given style";
		
		 SemErr(e.c_str());
		
		 attributeDescriptor=std::make_shared<StyleVoidAttributeDescriptor>();
		}
		
		Expect(47 /* ":" */);
		ATTRIBUTEVALUE(style,*attributeDescriptor);
}

void Parser::ATTRIBUTEVALUE(PartialStyleBase& style, const StyleAttributeDescriptor& descriptor) {
		ValueType              valueType=ValueType::NO_VALUE;
		bool                   negate=false;
		std::string            ident;
		std::string            subIdent;
		std::string            stringValue;
		std::string            function;
		double                 factor=-1;
		std::string            unit;
		std::string            number;
		std::list<std::string> numberList;
		Color                  color;
		StyleConstantRef       constant;
		
		if (StartOf(6)) {
			if (la->kind == _ident) {
				IDENT(ident);
				valueType=ValueType::IDENT; 
			} else if (la->kind == 60 /* "lighten" */) {
				Get();
				ident="lighten"; valueType=ValueType::IDENT; 
			} else if (la->kind == 61 /* "darken" */) {
				Get();
				ident="darken"; valueType=ValueType::IDENT; 
			} else {
				Get();
				ident="alpha"; valueType=ValueType::IDENT; 
			}
			if (la->kind == 25 /* "." */ || la->kind == 63 /* "(" */) {
				if (la->kind == 63 /* "(" */) {
					Get();
					valueType=ValueType::COLOR;
					function=ident;
					
					if (la->kind == _color) {
						COLOR_VALUE(color);
					} else if (la->kind == _variable) {
						CONSTANT(constant);
					} else SynErr(143);
					Expect(21 /* "," */);
					UDOUBLE(factor);
					Expect(64 /* ")" */);
				} else {
					Get();
					if (la->kind == _ident) {
						IDENT(subIdent);
					} else if (la->kind == 59 /* "name" */) {
						Get();
						subIdent="name"; 
					} else SynErr(144);
				}
			}
		} else if (la->kind == _string) {
			STRING(stringValue);
			valueType=ValueType::STRING; 
		} else if (la->kind == _number || la->kind == _double || la->kind == 44 /* "-" */) {
			if (la->kind == 44 /* "-" */) {
				Get();
				negate=true; 
			}
			if (la->kind == _number) {
				Get();
				number=t->val;
				valueType=ValueType::NUMBER;
				
				if (la->kind == _ident || la->kind == 36 /* "mm" */ || la->kind == 37 /* "m" */) {
					if (la->kind == _ident) {
						IDENT(unit);
					} else if (la->kind == 36 /* "mm" */) {
						Get();
						unit="mm"; 
					} else {
						Get();
						unit="m"; 
					}
				}
				while (la->kind == 21 /* "," */) {
					Get();
					if (la->kind == _number) {
						Get();
						numberList.push_back(t->val); 
					} else if (la->kind == _double) {
						Get();
						numberList.push_back(t->val); 
					} else SynErr(145);
				}
			} else if (la->kind == _double) {
				Get();
				number=t->val;
				valueType=ValueType::NUMBER;
				
				if (la->kind == _ident || la->kind == 36 /* "mm" */ || la->kind == 37 /* "m" */) {
					if (la->kind == _ident) {
						IDENT(unit);
					} else if (la->kind == 36 /* "mm" */) {
						Get();
						unit="mm"; 
					} else {
						Get();
						unit="m"; 
					}
				}
				while (la->kind == 21 /* "," */) {
					Get();
					if (la->kind == _number) {
						Get();
						numberList.push_back(t->val); 
					} else if (la->kind == _double) {
						Get();
						numberList.push_back(t->val); 
					} else SynErr(146);
				}
			} else SynErr(147);
		} else if (la->kind == _color) {
			COLOR_VALUE(color);
			valueType=ValueType::COLOR; 
		} else if (la->kind == _variable) {
			CONSTANT(constant);
			valueType=ValueType::CONSTANT; 
		} else SynErr(148);
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
		     if (StyleConstantColor* colorConstant = dynamic_cast<StyleConstantColor*>(constant.get());
		         colorConstant == nullptr) {
		       std::string e="Constant is not of type 'COLOR'";
		
		       SemErr(e.c_str());
		     }
		     else {
		       color=colorConstant->GetColor();
		     }
		   }
		
		   if (!function.empty()) {
		     if (factor<0.0 || factor>1.0) {
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
		     else if (function=="alpha") {
		       if (!errors->hasErrors) {
		         style.SetColorValue(descriptor.GetAttribute(),color.Alpha(factor));
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
		   else if (dynamic_cast<StyleConstantColor*>(constant.get())==nullptr) {
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
		   uint32_t      level;
		
		   if (StringToNumber(number,level)) {
		     magnification.SetLevel(osmscout::MagnificationLevel(level));
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
		   else if (dynamic_cast<StyleConstantMag*>(constant.get())==nullptr) {
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
		 else if (valueType==ValueType::CONSTANT) {
		   if (!constant) {
		     // no code
		   }
		   else if (dynamic_cast<StyleConstantWidth*>(constant.get())==nullptr) {
		     std::string e="Constant is not of type 'WIDTH'";
		
		     SemErr(e.c_str());
		   }
		
		   if (!errors->hasErrors) {
		     StyleConstantWidth* widthConstant=dynamic_cast<StyleConstantWidth*>(constant.get());
		
		     if (widthConstant->GetUnit()!=StyleConstantWidth::Unit::m) {
		       std::string e="Constant is not of unit 'm'";
		
		       SemErr(e.c_str());
		     }
		     else {
		       style.SetDoubleValue(descriptor.GetAttribute(),widthConstant->GetWidth());
		     }
		   }
		 }
		}
		else if (descriptor.GetType()==StyleAttributeType::TYPE_UMAP_SIZE) {
		 if (valueType==ValueType::NUMBER) {
		   double value;
		
		   if (negate) {
		     std::string e="Width must be >= 0.0";
		
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
		 else if (valueType==ValueType::CONSTANT) {
		   if (!constant) {
		     // no code
		   }
		   else if (dynamic_cast<StyleConstantWidth*>(constant.get())==nullptr) {
		     std::string e="Constant is not of type 'WIDTH'";
		
		     SemErr(e.c_str());
		   }
		
		   if (!errors->hasErrors) {
		     StyleConstantWidth* widthConstant=dynamic_cast<StyleConstantWidth*>(constant.get());
		
		     if (widthConstant->GetUnit()!=StyleConstantWidth::Unit::m) {
		       std::string e="Constant is not of unit 'm'";
		
		       SemErr(e.c_str());
		     }
		     else if (widthConstant->GetWidth()<0.0) {
		       std::string e="Width must be >= 0.0";
		
		       SemErr(e.c_str());
		     }
		     else {
		       style.SetDoubleValue(descriptor.GetAttribute(),widthConstant->GetWidth());
		     }
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
		   std::vector<double> valueList;
		   valueList.reserve(numberList.size()+1);
		
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
		
		   for (const auto& number : numberList) {
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
		   else if (dynamic_cast<StyleConstantInt*>(constant.get())==nullptr) {
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
		   else if (dynamic_cast<StyleConstantUInt*>(constant.get())==nullptr) {
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
		 color=PostprocessColor(osmscout::Color::FromHexString(c));
		}
		
}

void Parser::CONSTANT(StyleConstantRef& constant) {
		constant=nullptr;
		
		Expect(_variable);
		constant=config.GetConstantByName(t->val+1);
		
		if (!constant) {
		 std::string e=std::string("Constant \"") + t->val + "\" not defined";
		
		 SemErr(e.c_str());
		}
		
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
               const std::string& filename,
               StyleConfig& config,
               osmscout::ColorPostprocessor colorPostprocessor)
 : filename(filename),
   config(config)
{
	maxT = 66;

  dummyToken = NULL;
  t = la = NULL;
  minErrDist = 2;
  errDist = minErrDist;
  this->scanner = scanner;
  errors = new Errors();
  this->colorPostprocessor=colorPostprocessor;
}

bool Parser::StartOf(int s)
{
  const bool T = true;
  const bool x = false;

	static bool set[7][68] = {
		{T,x,x,x, x,x,x,T, x,T,T,x, T,T,x,x, x,x,T,x, T,x,T,T, T,x,T,x, T,x,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,T,T, x,T,T,T, T,T,T,x, x,x,x,x, x,x,x,x},
		{T,T,x,x, x,x,x,T, x,T,T,x, T,T,x,x, x,x,T,x, T,x,T,T, T,x,T,x, T,x,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,T,T, x,T,T,T, T,T,T,T, x,x,x,x, x,x,x,x},
		{x,x,x,x, x,x,x,x, x,x,x,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, T,T,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x},
		{x,x,x,x, x,x,x,x, x,x,x,T, T,x,x,x, x,x,x,x, x,x,x,x, T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,T, x,x,x,x, x,x,x,x, x,x,T,x, x,x,T,x, x,x,T,x, x,x,x,x, x,x,x,x},
		{x,x,x,x, x,x,x,x, x,x,x,x, T,x,x,x, x,x,x,x, x,x,x,x, T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,T, x,x,x,x, x,x,x,x, x,x,T,x, x,x,T,x, x,x,T,x, x,x,x,x, x,x,x,x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,T,x, x,x,T,x, x,x,T,x, x,x,x,x, x,x,x,x},
		{x,T,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, T,T,T,x, x,x,x,x}
	};



  return set[s][la->kind];
}

Parser::~Parser()
{
  delete errors;
}

osmscout::Color Parser::PostprocessColor(const osmscout::Color& color) const
{
  if (colorPostprocessor!=nullptr) {
    return (*colorPostprocessor)(color);
  }
  else {
    return color;
  }
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
			case 9: s = coco_string_create("\"MODULE\" expected"); break;
			case 10: s = coco_string_create("\"FLAG\" expected"); break;
			case 11: s = coco_string_create("\"IF\" expected"); break;
			case 12: s = coco_string_create("\"{\" expected"); break;
			case 13: s = coco_string_create("\"}\" expected"); break;
			case 14: s = coco_string_create("\"ELIF\" expected"); break;
			case 15: s = coco_string_create("\"ELSE\" expected"); break;
			case 16: s = coco_string_create("\"=\" expected"); break;
			case 17: s = coco_string_create("\";\" expected"); break;
			case 18: s = coco_string_create("\"ORDER\" expected"); break;
			case 19: s = coco_string_create("\"WAYS\" expected"); break;
			case 20: s = coco_string_create("\"GROUP\" expected"); break;
			case 21: s = coco_string_create("\",\" expected"); break;
			case 22: s = coco_string_create("\"SYMBOL\" expected"); break;
			case 23: s = coco_string_create("\"BORDER\" expected"); break;
			case 24: s = coco_string_create("\"AREA\" expected"); break;
			case 25: s = coco_string_create("\".\" expected"); break;
			case 26: s = coco_string_create("\"POLYGON\" expected"); break;
			case 27: s = coco_string_create("\"GROUND\" expected"); break;
			case 28: s = coco_string_create("\"RECTANGLE\" expected"); break;
			case 29: s = coco_string_create("\"x\" expected"); break;
			case 30: s = coco_string_create("\"CIRCLE\" expected"); break;
			case 31: s = coco_string_create("\"CONST\" expected"); break;
			case 32: s = coco_string_create("\"COLOR\" expected"); break;
			case 33: s = coco_string_create("\"MAG\" expected"); break;
			case 34: s = coco_string_create("\"UINT\" expected"); break;
			case 35: s = coco_string_create("\"WIDTH\" expected"); break;
			case 36: s = coco_string_create("\"mm\" expected"); break;
			case 37: s = coco_string_create("\"m\" expected"); break;
			case 38: s = coco_string_create("\"STYLE\" expected"); break;
			case 39: s = coco_string_create("\"[\" expected"); break;
			case 40: s = coco_string_create("\"]\" expected"); break;
			case 41: s = coco_string_create("\"FEATURE\" expected"); break;
			case 42: s = coco_string_create("\"PATH\" expected"); break;
			case 43: s = coco_string_create("\"TYPE\" expected"); break;
			case 44: s = coco_string_create("\"-\" expected"); break;
			case 45: s = coco_string_create("\"ONEWAY\" expected"); break;
			case 46: s = coco_string_create("\"SIZE\" expected"); break;
			case 47: s = coco_string_create("\":\" expected"); break;
			case 48: s = coco_string_create("\"px\" expected"); break;
			case 49: s = coco_string_create("\"<\" expected"); break;
			case 50: s = coco_string_create("\"NODE\" expected"); break;
			case 51: s = coco_string_create("\"TEXT\" expected"); break;
			case 52: s = coco_string_create("\"#\" expected"); break;
			case 53: s = coco_string_create("\"ICON\" expected"); break;
			case 54: s = coco_string_create("\"WAY\" expected"); break;
			case 55: s = coco_string_create("\"SHIELD\" expected"); break;
			case 56: s = coco_string_create("\"BORDERTEXT\" expected"); break;
			case 57: s = coco_string_create("\"BORDERSYMBOL\" expected"); break;
			case 58: s = coco_string_create("\"ROUTE\" expected"); break;
			case 59: s = coco_string_create("\"name\" expected"); break;
			case 60: s = coco_string_create("\"lighten\" expected"); break;
			case 61: s = coco_string_create("\"darken\" expected"); break;
			case 62: s = coco_string_create("\"alpha\" expected"); break;
			case 63: s = coco_string_create("\"(\" expected"); break;
			case 64: s = coco_string_create("\")\" expected"); break;
			case 65: s = coco_string_create("\"!\" expected"); break;
			case 66: s = coco_string_create("??? expected"); break;
			case 67: s = coco_string_create("this symbol not expected in OSS"); break;
			case 68: s = coco_string_create("this symbol not expected in FLAGSECTION"); break;
			case 69: s = coco_string_create("this symbol not expected in WAYORDER"); break;
			case 70: s = coco_string_create("this symbol not expected in CONSTSECTION"); break;
			case 71: s = coco_string_create("this symbol not expected in SYMBOLSECTION"); break;
			case 72: s = coco_string_create("this symbol not expected in IMPORT"); break;
			case 73: s = coco_string_create("this symbol not expected in WAYGROUP"); break;
			case 74: s = coco_string_create("this symbol not expected in POLYGON"); break;
			case 75: s = coco_string_create("this symbol not expected in RECTANGLE"); break;
			case 76: s = coco_string_create("this symbol not expected in CIRCLE"); break;
			case 77: s = coco_string_create("this symbol not expected in AREAFILLSYMSTYLE"); break;
			case 78: s = coco_string_create("this symbol not expected in AREABORDERSYMSTYLE"); break;
			case 79: s = coco_string_create("this symbol not expected in AREASYMBOLSTYLE"); break;
			case 80: s = coco_string_create("this symbol not expected in AREASYMBOLSTYLE"); break;
			case 81: s = coco_string_create("invalid AREASYMBOLSTYLE"); break;
			case 82: s = coco_string_create("this symbol not expected in AREASYMBOLSTYLE"); break;
			case 83: s = coco_string_create("invalid UDOUBLE"); break;
			case 84: s = coco_string_create("invalid DOUBLE"); break;
			case 85: s = coco_string_create("invalid CONSTDEF"); break;
			case 86: s = coco_string_create("invalid WIDTHCONSTDEF"); break;
			case 87: s = coco_string_create("invalid COLOR"); break;
			case 88: s = coco_string_create("invalid MAG"); break;
			case 89: s = coco_string_create("invalid UINT"); break;
			case 90: s = coco_string_create("invalid STYLE"); break;
			case 91: s = coco_string_create("invalid STYLEDEF"); break;
			case 92: s = coco_string_create("invalid UMAP"); break;
			case 93: s = coco_string_create("this symbol not expected in NODESTYLEDEF"); break;
			case 94: s = coco_string_create("invalid NODESTYLEDEF"); break;
			case 95: s = coco_string_create("this symbol not expected in WAYSTYLEDEF"); break;
			case 96: s = coco_string_create("invalid WAYSTYLEDEF"); break;
			case 97: s = coco_string_create("invalid WAYSTYLEDEF"); break;
			case 98: s = coco_string_create("this symbol not expected in AREASTYLEDEF"); break;
			case 99: s = coco_string_create("invalid AREASTYLEDEF"); break;
			case 100: s = coco_string_create("invalid AREASTYLEDEF"); break;
			case 101: s = coco_string_create("this symbol not expected in ROUTESTYLEDEF"); break;
			case 102: s = coco_string_create("invalid ROUTESTYLEDEF"); break;
			case 103: s = coco_string_create("this symbol not expected in NODETEXTSTYLE"); break;
			case 104: s = coco_string_create("this symbol not expected in NODETEXTSTYLE"); break;
			case 105: s = coco_string_create("this symbol not expected in NODETEXTSTYLE"); break;
			case 106: s = coco_string_create("this symbol not expected in NODEICONSTYLE"); break;
			case 107: s = coco_string_create("this symbol not expected in NODEICONSTYLE"); break;
			case 108: s = coco_string_create("this symbol not expected in NODEICONSTYLE"); break;
			case 109: s = coco_string_create("this symbol not expected in WAYSTYLE"); break;
			case 110: s = coco_string_create("this symbol not expected in WAYSTYLE"); break;
			case 111: s = coco_string_create("this symbol not expected in WAYPATHTEXTSTYLE"); break;
			case 112: s = coco_string_create("this symbol not expected in WAYPATHTEXTSTYLE"); break;
			case 113: s = coco_string_create("this symbol not expected in WAYPATHTEXTSTYLE"); break;
			case 114: s = coco_string_create("this symbol not expected in WAYPATHSYMBOLSTYLE"); break;
			case 115: s = coco_string_create("this symbol not expected in WAYPATHSYMBOLSTYLE"); break;
			case 116: s = coco_string_create("this symbol not expected in WAYPATHSYMBOLSTYLE"); break;
			case 117: s = coco_string_create("this symbol not expected in WAYSHIELDSTYLE"); break;
			case 118: s = coco_string_create("this symbol not expected in WAYSHIELDSTYLE"); break;
			case 119: s = coco_string_create("this symbol not expected in WAYSHIELDSTYLE"); break;
			case 120: s = coco_string_create("this symbol not expected in ROUTEPATHTEXTSTYLE"); break;
			case 121: s = coco_string_create("this symbol not expected in ROUTEPATHTEXTSTYLE"); break;
			case 122: s = coco_string_create("this symbol not expected in ROUTEPATHTEXTSTYLE"); break;
			case 123: s = coco_string_create("this symbol not expected in AREASTYLE"); break;
			case 124: s = coco_string_create("this symbol not expected in AREASTYLE"); break;
			case 125: s = coco_string_create("this symbol not expected in AREATEXTSTYLE"); break;
			case 126: s = coco_string_create("this symbol not expected in AREATEXTSTYLE"); break;
			case 127: s = coco_string_create("this symbol not expected in AREATEXTSTYLE"); break;
			case 128: s = coco_string_create("this symbol not expected in AREAICONSTYLE"); break;
			case 129: s = coco_string_create("this symbol not expected in AREAICONSTYLE"); break;
			case 130: s = coco_string_create("this symbol not expected in AREAICONSTYLE"); break;
			case 131: s = coco_string_create("this symbol not expected in AREABORDERSTYLE"); break;
			case 132: s = coco_string_create("this symbol not expected in AREABORDERSTYLE"); break;
			case 133: s = coco_string_create("this symbol not expected in AREABORDERSTYLE"); break;
			case 134: s = coco_string_create("this symbol not expected in AREABORDERTEXTSTYLE"); break;
			case 135: s = coco_string_create("this symbol not expected in AREABORDERTEXTSTYLE"); break;
			case 136: s = coco_string_create("this symbol not expected in AREABORDERTEXTSTYLE"); break;
			case 137: s = coco_string_create("this symbol not expected in AREABORDERSYMBOLSTYLE"); break;
			case 138: s = coco_string_create("this symbol not expected in AREABORDERSYMBOLSTYLE"); break;
			case 139: s = coco_string_create("this symbol not expected in AREABORDERSYMBOLSTYLE"); break;
			case 140: s = coco_string_create("this symbol not expected in ROUTESTYLE"); break;
			case 141: s = coco_string_create("this symbol not expected in ROUTESTYLE"); break;
			case 142: s = coco_string_create("invalid ATTRIBUTE"); break;
			case 143: s = coco_string_create("invalid ATTRIBUTEVALUE"); break;
			case 144: s = coco_string_create("invalid ATTRIBUTEVALUE"); break;
			case 145: s = coco_string_create("invalid ATTRIBUTEVALUE"); break;
			case 146: s = coco_string_create("invalid ATTRIBUTEVALUE"); break;
			case 147: s = coco_string_create("invalid ATTRIBUTEVALUE"); break;
			case 148: s = coco_string_create("invalid ATTRIBUTEVALUE"); break;

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

