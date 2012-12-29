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

#include <cassert>
#include <sstream>

#include <osmscout/oss/Scanner.h>

#include <osmscout/util/String.h>

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
		while (!(la->kind == _EOF || la->kind == 6 /* "OSS" */)) {SynErr(80); Get();}
		Expect(6 /* "OSS" */);
		if (la->kind == 8 /* "ORDER" */) {
			WAYORDER();
		}
		while (la->kind == 12 /* "SYMBO" */) {
			SYMBOL();
		}
		while (StartOf(1)) {
			StyleFilter filter; 
			STYLE(filter);
		}
		Expect(7 /* "END" */);
		
}

void Parser::WAYORDER() {
		Expect(8 /* "ORDER" */);
		Expect(9 /* "WAYS" */);
		size_t priority=1;
		while (la->kind == 10 /* "GROUP" */) {
			WAYGROUP(priority);
			priority++;
		}
}

void Parser::SYMBOL() {
		Expect(12 /* "SYMBO" */);
		std::string name;
		
		IDENT(name);
		SymbolRef   symbol=new Symbol(name);
		
		while (la->kind == 13 /* "POLYGON" */ || la->kind == 17 /* "RECTANGLE" */ || la->kind == 19 /* "CIRCLE" */) {
			if (la->kind == 13 /* "POLYGON" */) {
				POLYGON(*symbol);
			} else if (la->kind == 17 /* "RECTANGLE" */) {
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

void Parser::STYLE(StyleFilter filter) {
		if (la->kind == 20 /* "[" */) {
			STYLEFILTER(filter);
		}
		if (la->kind == 14 /* "{" */) {
			Get();
			while (StartOf(1)) {
				STYLE(filter);
			}
			Expect(16 /* "}" */);
		} else if (la->kind == 26 /* "NODE" */ || la->kind == 30 /* "WAY" */ || la->kind == 32 /* "AREA" */) {
			STYLEDEF(filter);
		} else SynErr(81);
}

void Parser::WAYGROUP(size_t priority) {
		Expect(10 /* "GROUP" */);
		if (la->kind == _ident) {
			std::string wayTypeName;
			TypeId      wayType;
			
			IDENT(wayTypeName);
			wayType=config.GetTypeConfig()->GetWayTypeId(wayTypeName);
			
			if (wayType==typeIgnore) {
			  std::string e="Unknown way type '"+wayTypeName+"'";
			  SemErr(e.c_str());
			}
			else {
			  config.SetWayPrio(wayType,priority);
			}
			
		}
		while (la->kind == 11 /* "," */) {
			std::string wayTypeName;
			TypeId      wayType;
			
			Get();
			IDENT(wayTypeName);
			wayType=config.GetTypeConfig()->GetWayTypeId(wayTypeName);
			
			if (wayType==typeIgnore) {
			  std::string e="Unknown way type '"+wayTypeName+"'";
			 SemErr(e.c_str());
			}
			else {
			  config.SetWayPrio(wayType,priority);
			}
			
		}
}

void Parser::IDENT(std::string& value) {
		Expect(_ident);
		value=t->val;
		
}

void Parser::POLYGON(Symbol& symbol) {
		Expect(13 /* "POLYGON" */);
		StyleFilter         filter;
		FillPartialStyle    style;
		PolygonPrimitiveRef polygon(new PolygonPrimitive(style.style));
		Pixel               pixel;
		
		PIXEL(pixel);
		polygon->AddPixel(pixel); 
		PIXEL(pixel);
		polygon->AddPixel(pixel); 
		while (la->kind == _number || la->kind == _double) {
			PIXEL(pixel);
			polygon->AddPixel(pixel); 
		}
		while (!(la->kind == _EOF || la->kind == 14 /* "{" */)) {SynErr(82); Get();}
		Expect(14 /* "{" */);
		while (StartOf(2)) {
			FILLSTYLEATTR(style);
			ExpectWeak(15 /* ";" */, 3);
		}
		while (!(la->kind == _EOF || la->kind == 16 /* "}" */)) {SynErr(83); Get();}
		Expect(16 /* "}" */);
		symbol.AddPrimitive(polygon); 
}

void Parser::RECTANGLE(Symbol& symbol) {
		Expect(17 /* "RECTANGLE" */);
		StyleFilter       filter;
		FillPartialStyle  style;
		Pixel             topLeft;
		double            width;
		double            height;
		
		PIXEL(topLeft);
		DOUBLE(width);
		Expect(18 /* "x" */);
		DOUBLE(height);
		while (!(la->kind == _EOF || la->kind == 14 /* "{" */)) {SynErr(84); Get();}
		Expect(14 /* "{" */);
		while (StartOf(2)) {
			FILLSTYLEATTR(style);
			ExpectWeak(15 /* ";" */, 3);
		}
		while (!(la->kind == _EOF || la->kind == 16 /* "}" */)) {SynErr(85); Get();}
		Expect(16 /* "}" */);
		symbol.AddPrimitive(new RectanglePrimitive(topLeft,
		                                          width,height,
		                                          style.style));
		
}

void Parser::CIRCLE(Symbol& symbol) {
		Expect(19 /* "CIRCLE" */);
		Pixel             center;
		double            radius;
		StyleFilter       filter;
		FillPartialStyle  style;
		
		PIXEL(center);
		DOUBLE(radius);
		while (!(la->kind == _EOF || la->kind == 14 /* "{" */)) {SynErr(86); Get();}
		Expect(14 /* "{" */);
		while (StartOf(2)) {
			FILLSTYLEATTR(style);
			ExpectWeak(15 /* ";" */, 3);
		}
		while (!(la->kind == _EOF || la->kind == 16 /* "}" */)) {SynErr(87); Get();}
		Expect(16 /* "}" */);
		symbol.AddPrimitive(new CirclePrimitive(center,
		                                       radius,
		                                       style.style));
		
}

void Parser::PIXEL(Pixel& pixel) {
		double x;
		double y;
		
		DOUBLE(x);
		Expect(11 /* "," */);
		DOUBLE(y);
		pixel=Pixel(x,y); 
}

void Parser::FILLSTYLEATTR(FillPartialStyle& style) {
		switch (la->kind) {
		case 33 /* "color" */: {
			Color fillColor; 
			Get();
			Expect(34 /* ":" */);
			COLOR(fillColor);
			style.style->SetFillColor(fillColor);
			style.attributes.insert(FillStyle::attrFillColor);
			
			break;
		}
		case 43 /* "pattern" */: {
			std::string patternName; 
			Get();
			Expect(34 /* ":" */);
			STRING(patternName);
			style.style->SetPattern(patternName);
			style.attributes.insert(FillStyle::attrPattern);
			
			break;
		}
		case 44 /* "patternMinMag" */: {
			Magnification minMag; 
			Get();
			Expect(34 /* ":" */);
			MAG(minMag);
			style.style->SetPatternMinMag(minMag);
			style.attributes.insert(FillStyle::attrPatternMinMag);
			
			break;
		}
		case 45 /* "borderColor" */: {
			Color borderColor; 
			Get();
			Expect(34 /* ":" */);
			COLOR(borderColor);
			style.style->SetBorderColor(borderColor);
			style.attributes.insert(FillStyle::attrBorderColor);
			
			break;
		}
		case 46 /* "borderWidth" */: {
			double width; 
			Get();
			Expect(34 /* ":" */);
			DISPLAYSIZE(width);
			style.style->SetBorderWidth(width);
			style.attributes.insert(FillStyle::attrBorderWidth);
			
			break;
		}
		case 47 /* "borderDash" */: {
			std::vector<double> dashes;
			double              dash;
			
			Get();
			Expect(34 /* ":" */);
			DOUBLE(dash);
			dashes.push_back(dash); 
			while (la->kind == 11 /* "," */) {
				Get();
				DOUBLE(dash);
				dashes.push_back(dash); 
			}
			style.style->SetBorderDashes(dashes);
			style.attributes.insert(FillStyle::attrBorderDashes);
			
			break;
		}
		default: SynErr(88); break;
		}
}

void Parser::DOUBLE(double& value) {
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
			
		} else SynErr(89);
}

void Parser::STYLEFILTER(StyleFilter& filter) {
		Expect(20 /* "[" */);
		if (la->kind == 21 /* "TYPE" */) {
			TypeSet     types;
			std::string name;
			
			Get();
			IDENT(name);
			TypeId type=config.GetTypeConfig()->GetTypeId(name);
			
			if (type==typeIgnore) {
			 std::string e="Unknown type '"+name+"'";
			
			 SemErr(e.c_str());
			}
			else if (filter.HasTypes() &&
			        !filter.HasType(type)) {
			 std::string e="Type '"+name+"' is not included by parent filter";
			
			 SemErr(e.c_str());
			}
			else {
			 types.SetType(type);
			}
			
			while (la->kind == 11 /* "," */) {
				std::string name; 
				Get();
				IDENT(name);
				TypeId      type=config.GetTypeConfig()->GetTypeId(name);
				
				if (type==typeIgnore) {
				 std::string e="Unknown type '"+name+"'";
				
				 SemErr(e.c_str());
				}
				else if (filter.HasTypes() &&
				        !filter.HasType(type)) {
				 std::string e="Type '"+name+"' is not included by parent filter";
				
				 SemErr(e.c_str());
				}
				else {
				 types.SetType(type);
				}
				
			}
			filter.SetTypes(types); 
		}
		if (la->kind == 22 /* "MAG" */) {
			Get();
			if (StartOf(4)) {
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
			Expect(23 /* "-" */);
			if (StartOf(4)) {
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
		if (la->kind == 24 /* "ONEWAY" */) {
			Get();
			filter.SetOneway(true);
			
		}
		Expect(25 /* "]" */);
}

void Parser::STYLEDEF(StyleFilter filter) {
		if (la->kind == 26 /* "NODE" */) {
			NODESTYLEDEF(filter);
		} else if (la->kind == 30 /* "WAY" */) {
			WAYSTYLEDEF(filter);
		} else if (la->kind == 32 /* "AREA" */) {
			AREASTYLEDEF(filter);
		} else SynErr(90);
}

void Parser::MAG(Magnification& magnification) {
		switch (la->kind) {
		case 63 /* "world" */: {
			Get();
			magnification.SetMagnification(Magnification::magWorld); 
			break;
		}
		case 64 /* "continent" */: {
			Get();
			magnification.SetMagnification(Magnification::magContinent); 
			break;
		}
		case 65 /* "state" */: {
			Get();
			magnification.SetMagnification(Magnification::magState); 
			break;
		}
		case 66 /* "stateOver" */: {
			Get();
			magnification.SetMagnification(Magnification::magStateOver); 
			break;
		}
		case 67 /* "county" */: {
			Get();
			magnification.SetMagnification(Magnification::magCounty); 
			break;
		}
		case 68 /* "region" */: {
			Get();
			magnification.SetMagnification(Magnification::magRegion); 
			break;
		}
		case 69 /* "proximity" */: {
			Get();
			magnification.SetMagnification(Magnification::magProximity); 
			break;
		}
		case 70 /* "cityOver" */: {
			Get();
			magnification.SetMagnification(Magnification::magCityOver); 
			break;
		}
		case 71 /* "city" */: {
			Get();
			magnification.SetMagnification(Magnification::magCity); 
			break;
		}
		case 72 /* "suburb" */: {
			Get();
			magnification.SetMagnification(Magnification::magSuburb); 
			break;
		}
		case 73 /* "detail" */: {
			Get();
			magnification.SetMagnification(Magnification::magDetail); 
			break;
		}
		case 74 /* "close" */: {
			Get();
			magnification.SetMagnification(Magnification::magClose); 
			break;
		}
		case 75 /* "veryClose" */: {
			Get();
			magnification.SetMagnification(Magnification::magVeryClose); 
			break;
		}
		case 76 /* "block" */: {
			Get();
			magnification.SetMagnification(Magnification::magBlock); 
			break;
		}
		default: SynErr(91); break;
		}
}

void Parser::NODESTYLEDEF(StyleFilter filter) {
		while (!(la->kind == _EOF || la->kind == 26 /* "NODE" */)) {SynErr(92); Get();}
		Expect(26 /* "NODE" */);
		Expect(27 /* "." */);
		if (la->kind == 28 /* "TEXT" */) {
			NODETEXTSTYLE(filter);
		} else if (la->kind == 29 /* "ICON" */) {
			NODEICONSTYLE(filter);
		} else SynErr(93);
}

void Parser::WAYSTYLEDEF(StyleFilter filter) {
		while (!(la->kind == _EOF || la->kind == 30 /* "WAY" */)) {SynErr(94); Get();}
		Expect(30 /* "WAY" */);
		if (la->kind == 14 /* "{" */ || la->kind == 20 /* "[" */) {
			WAYSTYLE(filter);
		} else if (la->kind == 27 /* "." */) {
			Get();
			if (la->kind == 28 /* "TEXT" */) {
				WAYPATHTEXTSTYLE(filter);
			} else if (la->kind == 12 /* "SYMBO" */) {
				WAYPATHSYMBOLSTYLE(filter);
			} else if (la->kind == 31 /* "SHIELD" */) {
				WAYSHIELDSTYLE(filter);
			} else SynErr(95);
		} else SynErr(96);
}

void Parser::AREASTYLEDEF(StyleFilter filter) {
		while (!(la->kind == _EOF || la->kind == 32 /* "AREA" */)) {SynErr(97); Get();}
		Expect(32 /* "AREA" */);
		if (la->kind == 14 /* "{" */ || la->kind == 20 /* "[" */) {
			AREASTYLE(filter);
		} else if (la->kind == 27 /* "." */) {
			Get();
			if (la->kind == 28 /* "TEXT" */) {
				AREATEXTSTYLE(filter);
			} else if (la->kind == 29 /* "ICON" */) {
				AREAICONSTYLE(filter);
			} else SynErr(98);
		} else SynErr(99);
}

void Parser::NODETEXTSTYLE(StyleFilter filter) {
		while (!(la->kind == _EOF || la->kind == 28 /* "TEXT" */)) {SynErr(100); Get();}
		Expect(28 /* "TEXT" */);
		if (la->kind == 20 /* "[" */) {
			STYLEFILTER(filter);
		}
		TextPartialStyle style;
		
		while (!(la->kind == _EOF || la->kind == 14 /* "{" */)) {SynErr(101); Get();}
		Expect(14 /* "{" */);
		while (StartOf(5)) {
			TEXTSTYLEATTR(style);
			ExpectWeak(15 /* ";" */, 6);
		}
		while (!(la->kind == _EOF || la->kind == 16 /* "}" */)) {SynErr(102); Get();}
		Expect(16 /* "}" */);
		config.AddNodeTextStyle(filter,style);
		
}

void Parser::NODEICONSTYLE(StyleFilter filter) {
		while (!(la->kind == _EOF || la->kind == 29 /* "ICON" */)) {SynErr(103); Get();}
		Expect(29 /* "ICON" */);
		if (la->kind == 20 /* "[" */) {
			STYLEFILTER(filter);
		}
		IconPartialStyle style;
		
		while (!(la->kind == _EOF || la->kind == 14 /* "{" */)) {SynErr(104); Get();}
		Expect(14 /* "{" */);
		while (la->kind == 54 /* "symbol" */ || la->kind == 56 /* "name" */) {
			ICONSTYLEATTR(style);
			ExpectWeak(15 /* ";" */, 7);
		}
		while (!(la->kind == _EOF || la->kind == 16 /* "}" */)) {SynErr(105); Get();}
		Expect(16 /* "}" */);
		config.AddNodeIconStyle(filter,style);
		
}

void Parser::TEXTSTYLEATTR(TextPartialStyle& style) {
		switch (la->kind) {
		case 48 /* "label" */: {
			TextStyle::Label label; 
			Get();
			Expect(34 /* ":" */);
			TEXTLABEL(label);
			style.style->SetLabel(label);
			style.attributes.insert(TextStyle::attrLabel);
			
			break;
		}
		case 49 /* "style" */: {
			TextStyle::Style labelStyle; 
			Get();
			Expect(34 /* ":" */);
			LABELSTYLE(labelStyle);
			style.style->SetStyle(labelStyle);
			style.attributes.insert(TextStyle::attrStyle);
			
			break;
		}
		case 33 /* "color" */: {
			Color textColor; 
			Get();
			Expect(34 /* ":" */);
			COLOR(textColor);
			style.style->SetTextColor(textColor);
			style.attributes.insert(TextStyle::attrTextColor);
			
			break;
		}
		case 50 /* "size" */: {
			double size; 
			Get();
			Expect(34 /* ":" */);
			DOUBLE(size);
			style.style->SetSize(size);
			style.attributes.insert(TextStyle::attrSize);
			
			break;
		}
		case 51 /* "scaleMag" */: {
			Magnification scaleMag; 
			Get();
			Expect(34 /* ":" */);
			MAG(scaleMag);
			style.style->SetScaleAndFadeMag(scaleMag);
			style.attributes.insert(TextStyle::attrScaleAndFadeMag);
			
			break;
		}
		case 52 /* "priority" */: {
			size_t priority; 
			Get();
			Expect(34 /* ":" */);
			INTEGER(priority);
			if (priority>=0 && priority<std::numeric_limits<uint8_t>::max()) {
			   style.style->SetPriority((uint8_t)priority);
			   style.attributes.insert(TextStyle::attrPriority);
			}
			else {
			 std::string e="Priority must be in the interval [0,"+
			               NumberToString(std::numeric_limits<uint8_t>::max())+"[";
			
			 SemErr(e.c_str());
			}
			
			break;
		}
		default: SynErr(106); break;
		}
}

void Parser::ICONSTYLEATTR(IconPartialStyle& style) {
		if (la->kind == 54 /* "symbol" */) {
			std::string name;
			SymbolRef   symbol;
			
			Get();
			Expect(34 /* ":" */);
			IDENT(name);
			symbol=config.GetSymbol(name);
			
			if (symbol.Invalid()) {
			 std::string e="Map symbol '"+name+"' is not defined";
			
			 SemErr(e.c_str());
			}
			else {
			 style.style->SetSymbol(symbol);
			 style.attributes.insert(IconStyle::attrSymbol);
			}
			
		} else if (la->kind == 56 /* "name" */) {
			std::string name; 
			Get();
			Expect(34 /* ":" */);
			IDENT(name);
			style.style->SetIconName(name);
			style.attributes.insert(IconStyle::attrIconName);
			
		} else SynErr(107);
}

void Parser::WAYSTYLE(StyleFilter filter) {
		if (la->kind == 20 /* "[" */) {
			STYLEFILTER(filter);
		}
		LinePartialStyle style;
		
		while (!(la->kind == _EOF || la->kind == 14 /* "{" */)) {SynErr(108); Get();}
		Expect(14 /* "{" */);
		while (StartOf(8)) {
			LINESTYLEATTR(style);
			ExpectWeak(15 /* ";" */, 9);
		}
		while (!(la->kind == _EOF || la->kind == 16 /* "}" */)) {SynErr(109); Get();}
		Expect(16 /* "}" */);
		config.AddWayLineStyle(filter,style);
		
}

void Parser::WAYPATHTEXTSTYLE(StyleFilter filter) {
		while (!(la->kind == _EOF || la->kind == 28 /* "TEXT" */)) {SynErr(110); Get();}
		Expect(28 /* "TEXT" */);
		if (la->kind == 20 /* "[" */) {
			STYLEFILTER(filter);
		}
		PathTextPartialStyle style;
		
		while (!(la->kind == _EOF || la->kind == 14 /* "{" */)) {SynErr(111); Get();}
		Expect(14 /* "{" */);
		while (la->kind == 33 /* "color" */ || la->kind == 48 /* "label" */ || la->kind == 50 /* "size" */) {
			PATHTEXTSTYLEATTR(style);
			ExpectWeak(15 /* ";" */, 10);
		}
		while (!(la->kind == _EOF || la->kind == 16 /* "}" */)) {SynErr(112); Get();}
		Expect(16 /* "}" */);
		config.AddWayPathTextStyle(filter,style);
		
}

void Parser::WAYPATHSYMBOLSTYLE(StyleFilter filter) {
		while (!(la->kind == _EOF || la->kind == 12 /* "SYMBO" */)) {SynErr(113); Get();}
		Expect(12 /* "SYMBO" */);
		if (la->kind == 20 /* "[" */) {
			STYLEFILTER(filter);
		}
		PathSymbolPartialStyle style;
		
		while (!(la->kind == _EOF || la->kind == 14 /* "{" */)) {SynErr(114); Get();}
		Expect(14 /* "{" */);
		while (la->kind == 54 /* "symbol" */ || la->kind == 55 /* "symbolSpace" */) {
			PATHSYMBOLSTYLEATTR(style);
			ExpectWeak(15 /* ";" */, 11);
		}
		while (!(la->kind == _EOF || la->kind == 16 /* "}" */)) {SynErr(115); Get();}
		Expect(16 /* "}" */);
		config.AddWayPathSymbolStyle(filter,style);
		
}

void Parser::WAYSHIELDSTYLE(StyleFilter filter) {
		while (!(la->kind == _EOF || la->kind == 31 /* "SHIELD" */)) {SynErr(116); Get();}
		Expect(31 /* "SHIELD" */);
		if (la->kind == 20 /* "[" */) {
			STYLEFILTER(filter);
		}
		PathShieldPartialStyle style;
		
		while (!(la->kind == _EOF || la->kind == 14 /* "{" */)) {SynErr(117); Get();}
		Expect(14 /* "{" */);
		while (StartOf(12)) {
			SHIELDSTYLEATTR(style);
			ExpectWeak(15 /* ";" */, 13);
		}
		while (!(la->kind == _EOF || la->kind == 16 /* "}" */)) {SynErr(118); Get();}
		Expect(16 /* "}" */);
		config.AddWayPathShieldStyle(filter,style);
		
}

void Parser::LINESTYLEATTR(LinePartialStyle& style) {
		switch (la->kind) {
		case 33 /* "color" */: {
			Color lineColor; 
			Get();
			Expect(34 /* ":" */);
			COLOR(lineColor);
			style.style->SetLineColor(lineColor);
			style.attributes.insert(LineStyle::attrLineColor);
			
			break;
		}
		case 35 /* "altColor" */: {
			Color alternateColor; 
			Get();
			Expect(34 /* ":" */);
			COLOR(alternateColor);
			style.style->SetAlternateColor(alternateColor);
			style.attributes.insert(LineStyle::attrAlternateColor);
			
			break;
		}
		case 36 /* "outlineColor" */: {
			Color outlineColor;
			Get();
			Expect(34 /* ":" */);
			COLOR(outlineColor);
			style.style->SetOutlineColor(outlineColor);
			style.attributes.insert(LineStyle::attrOutlineColor);
			
			break;
		}
		case 37 /* "dash" */: {
			std::vector<double> dashes;
			double              dash;
			
			Get();
			Expect(34 /* ":" */);
			DOUBLE(dash);
			dashes.push_back(dash); 
			while (la->kind == 11 /* "," */) {
				Get();
				DOUBLE(dash);
				dashes.push_back(dash); 
			}
			style.style->SetDashes(dashes);
			style.attributes.insert(LineStyle::attrDashes);
			
			break;
		}
		case 38 /* "gapColor" */: {
			Color gapColor; 
			Get();
			Expect(34 /* ":" */);
			COLOR(gapColor);
			style.style->SetGapColor(gapColor);
			style.attributes.insert(LineStyle::attrGapColor);
			
			break;
		}
		case 39 /* "displayWidth" */: {
			double displayWidth; 
			Get();
			Expect(34 /* ":" */);
			DISPLAYSIZE(displayWidth);
			style.style->SetDisplayWidth(displayWidth);
			style.attributes.insert(LineStyle::attrDisplayWidth);
			
			break;
		}
		case 40 /* "width" */: {
			double width; 
			Get();
			Expect(34 /* ":" */);
			MAPSIZE(width);
			style.style->SetWidth(width);
			style.attributes.insert(LineStyle::attrWidth);
			
			break;
		}
		case 41 /* "cap" */: {
			LineStyle::CapStyle capStyle; 
			Get();
			Expect(34 /* ":" */);
			CAPSTYLE(capStyle);
			style.style->SetCapStyle(capStyle);
			style.attributes.insert(LineStyle::attrCapStyle);
			
			break;
		}
		case 42 /* "outline" */: {
			double outline; 
			Get();
			Expect(34 /* ":" */);
			DISPLAYSIZE(outline);
			style.style->SetOutline(outline);
			style.attributes.insert(LineStyle::attrOutline);
			
			break;
		}
		default: SynErr(119); break;
		}
}

void Parser::PATHTEXTSTYLEATTR(PathTextPartialStyle& style) {
		if (la->kind == 48 /* "label" */) {
			PathTextStyle::Label label; 
			Get();
			Expect(34 /* ":" */);
			PATHTEXTLABEL(label);
			style.style->SetLabel(label);
			style.attributes.insert(PathTextStyle::attrLabel);
			
		} else if (la->kind == 33 /* "color" */) {
			Color textColor; 
			Get();
			Expect(34 /* ":" */);
			COLOR(textColor);
			style.style->SetTextColor(textColor);
			style.attributes.insert(PathTextStyle::attrTextColor);
			
		} else if (la->kind == 50 /* "size" */) {
			double size; 
			Get();
			Expect(34 /* ":" */);
			DOUBLE(size);
			style.style->SetSize(size);
			style.attributes.insert(PathTextStyle::attrSize);
			
		} else SynErr(120);
}

void Parser::PATHSYMBOLSTYLEATTR(PathSymbolPartialStyle& style) {
		if (la->kind == 54 /* "symbol" */) {
			std::string name;
			SymbolRef   symbol;
			
			Get();
			Expect(34 /* ":" */);
			IDENT(name);
			symbol=config.GetSymbol(name);
			
			if (symbol.Invalid()) {
			 std::string e="Map symbol '"+name+"' is not defined";
			
			 SemErr(e.c_str());
			}
			else {
			 style.style->SetSymbol(symbol);
			 style.attributes.insert(PathSymbolStyle::attrSymbol);
			}
			
		} else if (la->kind == 55 /* "symbolSpace" */) {
			double symbolSpace; 
			Get();
			Expect(34 /* ":" */);
			DISPLAYSIZE(symbolSpace);
			style.style->SetSymbolSpace(symbolSpace);
			style.attributes.insert(PathSymbolStyle::attrSymbolSpace);
			
		} else SynErr(121);
}

void Parser::SHIELDSTYLEATTR(PathShieldPartialStyle& style) {
		switch (la->kind) {
		case 48 /* "label" */: {
			ShieldStyle::Label label; 
			Get();
			Expect(34 /* ":" */);
			SHIELDLABEL(label);
			style.style->SetLabel(label);
			style.attributes.insert(PathShieldStyle::attrLabel);
			
			break;
		}
		case 33 /* "color" */: {
			Color textColor; 
			Get();
			Expect(34 /* ":" */);
			COLOR(textColor);
			style.style->SetTextColor(textColor);
			style.attributes.insert(PathShieldStyle::attrTextColor);
			
			break;
		}
		case 53 /* "backgroundColor" */: {
			Color bgColor; 
			Get();
			Expect(34 /* ":" */);
			COLOR(bgColor);
			style.style->SetBgColor(bgColor);
			style.attributes.insert(PathShieldStyle::attrBgColor);
			
			break;
		}
		case 45 /* "borderColor" */: {
			Color borderColor; 
			Get();
			Expect(34 /* ":" */);
			COLOR(borderColor);
			style.style->SetBorderColor(borderColor);
			style.attributes.insert(PathShieldStyle::attrBorderColor);
			
			break;
		}
		case 50 /* "size" */: {
			double size; 
			Get();
			Expect(34 /* ":" */);
			DOUBLE(size);
			style.style->SetSize(size);
			style.attributes.insert(PathShieldStyle::attrSize);
			
			break;
		}
		case 52 /* "priority" */: {
			size_t priority; 
			Get();
			Expect(34 /* ":" */);
			INTEGER(priority);
			if (priority>=0 && priority<std::numeric_limits<uint8_t>::max()) {
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
		default: SynErr(122); break;
		}
}

void Parser::AREASTYLE(StyleFilter filter) {
		if (la->kind == 20 /* "[" */) {
			STYLEFILTER(filter);
		}
		FillPartialStyle style;
		
		while (!(la->kind == _EOF || la->kind == 14 /* "{" */)) {SynErr(123); Get();}
		Expect(14 /* "{" */);
		while (StartOf(2)) {
			FILLSTYLEATTR(style);
			ExpectWeak(15 /* ";" */, 3);
		}
		while (!(la->kind == _EOF || la->kind == 16 /* "}" */)) {SynErr(124); Get();}
		Expect(16 /* "}" */);
		config.AddAreaFillStyle(filter,style);
		
}

void Parser::AREATEXTSTYLE(StyleFilter filter) {
		while (!(la->kind == _EOF || la->kind == 28 /* "TEXT" */)) {SynErr(125); Get();}
		Expect(28 /* "TEXT" */);
		if (la->kind == 20 /* "[" */) {
			STYLEFILTER(filter);
		}
		TextPartialStyle style;
		
		while (!(la->kind == _EOF || la->kind == 14 /* "{" */)) {SynErr(126); Get();}
		Expect(14 /* "{" */);
		while (StartOf(5)) {
			TEXTSTYLEATTR(style);
			ExpectWeak(15 /* ";" */, 6);
		}
		while (!(la->kind == _EOF || la->kind == 16 /* "}" */)) {SynErr(127); Get();}
		Expect(16 /* "}" */);
		config.AddAreaTextStyle(filter,style);
		
}

void Parser::AREAICONSTYLE(StyleFilter filter) {
		while (!(la->kind == _EOF || la->kind == 29 /* "ICON" */)) {SynErr(128); Get();}
		Expect(29 /* "ICON" */);
		if (la->kind == 20 /* "[" */) {
			STYLEFILTER(filter);
		}
		IconPartialStyle style;
		
		while (!(la->kind == _EOF || la->kind == 14 /* "{" */)) {SynErr(129); Get();}
		Expect(14 /* "{" */);
		while (la->kind == 54 /* "symbol" */ || la->kind == 56 /* "name" */) {
			ICONSTYLEATTR(style);
			ExpectWeak(15 /* ";" */, 7);
		}
		while (!(la->kind == _EOF || la->kind == 16 /* "}" */)) {SynErr(130); Get();}
		Expect(16 /* "}" */);
		config.AddAreaIconStyle(filter,style);
		
}

void Parser::COLOR(Color& color) {
		Expect(_color);
		if (strlen(t->val)==7 ||
		   strlen(t->val)==9) {
		 ToRGBA(t->val,color);
		}
		else {
		 color=Color();
		}
		
}

void Parser::DISPLAYSIZE(double& value) {
		DOUBLE(value);
		Expect(77 /* "mm" */);
}

void Parser::MAPSIZE(double& value) {
		DOUBLE(value);
		Expect(78 /* "m" */);
}

void Parser::CAPSTYLE(LineStyle::CapStyle& style) {
		if (la->kind == 57 /* "butt" */) {
			Get();
			style=LineStyle::capButt; 
		} else if (la->kind == 58 /* "round" */) {
			Get();
			style=LineStyle::capRound; 
		} else if (la->kind == 59 /* "square" */) {
			Get();
			style=LineStyle::capSquare; 
		} else SynErr(131);
}

void Parser::STRING(std::string& value) {
		Expect(_string);
		value=Destring(t->val);
		
}

void Parser::TEXTLABEL(TextStyle::Label& label) {
		if (la->kind == 56 /* "name" */) {
			Get();
			label=TextStyle::name; 
		} else if (la->kind == 62 /* "ref" */) {
			Get();
			label=TextStyle::ref; 
		} else SynErr(132);
}

void Parser::LABELSTYLE(TextStyle::Style& style) {
		if (la->kind == 60 /* "normal" */) {
			Get();
			style=TextStyle::normal; 
		} else if (la->kind == 61 /* "emphasize" */) {
			Get();
			style=TextStyle::emphasize; 
		} else SynErr(133);
}

void Parser::INTEGER(size_t& value) {
		Expect(_number);
		if (!StringToNumber(t->val,value)) {
		 std::string e="Cannot parse number '"+std::string(t->val)+"'";
		
		 SemErr(e.c_str());
		}
		
}

void Parser::SHIELDLABEL(ShieldStyle::Label& label) {
		if (la->kind == 56 /* "name" */) {
			Get();
			label=ShieldStyle::name; 
		} else if (la->kind == 62 /* "ref" */) {
			Get();
			label=ShieldStyle::ref; 
		} else SynErr(134);
}

void Parser::PATHTEXTLABEL(PathTextStyle::Label& label) {
		if (la->kind == 56 /* "name" */) {
			Get();
			label=PathTextStyle::name; 
		} else if (la->kind == 62 /* "ref" */) {
			Get();
			label=PathTextStyle::ref; 
		} else SynErr(135);
}



void Parser::Parse()
{
  t = NULL;
  la = dummyToken = new Token();
  la->val = coco_string_create("Dummy Token");
  Get();
	OSS();
	Expect(0);
}

Parser::Parser(Scanner *scanner,
               StyleConfig& config)
 : config(config)
{
	maxT = 79;

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

	static bool set[14][81] = {
		{T,x,x,x, x,x,T,x, x,x,x,x, T,x,T,x, T,x,x,x, x,x,x,x, x,x,T,x, T,T,T,T, T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,T,x, x,x,x,x, T,x,x,x, x,x,T,x, x,x,T,x, T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,T,x,x, x,x,x,x, x,x,x,T, T,T,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x},
		{T,x,x,x, x,x,T,x, x,x,x,x, T,x,T,x, T,x,x,x, x,x,x,x, x,x,T,x, T,T,T,T, T,T,x,x, x,x,x,x, x,x,x,T, T,T,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,T, T,T,T,T, T,T,T,T, T,T,T,T, T,x,x,x, x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,T,x,x, x,x,x,x, x,x,x,x, x,x,x,x, T,T,T,T, T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x},
		{T,x,x,x, x,x,T,x, x,x,x,x, T,x,T,x, T,x,x,x, x,x,x,x, x,x,T,x, T,T,T,T, T,T,x,x, x,x,x,x, x,x,x,x, x,x,x,x, T,T,T,T, T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x},
		{T,x,x,x, x,x,T,x, x,x,x,x, T,x,T,x, T,x,x,x, x,x,x,x, x,x,T,x, T,T,T,T, T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,T,x, T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,T,x,T, T,T,T,T, T,T,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x},
		{T,x,x,x, x,x,T,x, x,x,x,x, T,x,T,x, T,x,x,x, x,x,x,x, x,x,T,x, T,T,T,T, T,T,x,T, T,T,T,T, T,T,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x},
		{T,x,x,x, x,x,T,x, x,x,x,x, T,x,T,x, T,x,x,x, x,x,x,x, x,x,T,x, T,T,T,T, T,T,x,x, x,x,x,x, x,x,x,x, x,x,x,x, T,x,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x},
		{T,x,x,x, x,x,T,x, x,x,x,x, T,x,T,x, T,x,x,x, x,x,x,x, x,x,T,x, T,T,T,T, T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,T,x,x, x,x,x,x, x,x,x,x, x,T,x,x, T,x,T,x, T,T,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x},
		{T,x,x,x, x,x,T,x, x,x,x,x, T,x,T,x, T,x,x,x, x,x,x,x, x,x,T,x, T,T,T,T, T,T,x,x, x,x,x,x, x,x,x,x, x,T,x,x, T,x,T,x, T,T,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x}
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
			case 5: s = coco_string_create("string expected"); break;
			case 6: s = coco_string_create("\"OSS\" expected"); break;
			case 7: s = coco_string_create("\"END\" expected"); break;
			case 8: s = coco_string_create("\"ORDER\" expected"); break;
			case 9: s = coco_string_create("\"WAYS\" expected"); break;
			case 10: s = coco_string_create("\"GROUP\" expected"); break;
			case 11: s = coco_string_create("\",\" expected"); break;
			case 12: s = coco_string_create("\"SYMBOL\" expected"); break;
			case 13: s = coco_string_create("\"POLYGON\" expected"); break;
			case 14: s = coco_string_create("\"{\" expected"); break;
			case 15: s = coco_string_create("\";\" expected"); break;
			case 16: s = coco_string_create("\"}\" expected"); break;
			case 17: s = coco_string_create("\"RECTANGLE\" expected"); break;
			case 18: s = coco_string_create("\"x\" expected"); break;
			case 19: s = coco_string_create("\"CIRCLE\" expected"); break;
			case 20: s = coco_string_create("\"[\" expected"); break;
			case 21: s = coco_string_create("\"TYPE\" expected"); break;
			case 22: s = coco_string_create("\"MAG\" expected"); break;
			case 23: s = coco_string_create("\"-\" expected"); break;
			case 24: s = coco_string_create("\"ONEWAY\" expected"); break;
			case 25: s = coco_string_create("\"]\" expected"); break;
			case 26: s = coco_string_create("\"NODE\" expected"); break;
			case 27: s = coco_string_create("\".\" expected"); break;
			case 28: s = coco_string_create("\"TEXT\" expected"); break;
			case 29: s = coco_string_create("\"ICON\" expected"); break;
			case 30: s = coco_string_create("\"WAY\" expected"); break;
			case 31: s = coco_string_create("\"SHIELD\" expected"); break;
			case 32: s = coco_string_create("\"AREA\" expected"); break;
			case 33: s = coco_string_create("\"color\" expected"); break;
			case 34: s = coco_string_create("\":\" expected"); break;
			case 35: s = coco_string_create("\"altColor\" expected"); break;
			case 36: s = coco_string_create("\"outlineColor\" expected"); break;
			case 37: s = coco_string_create("\"dash\" expected"); break;
			case 38: s = coco_string_create("\"gapColor\" expected"); break;
			case 39: s = coco_string_create("\"displayWidth\" expected"); break;
			case 40: s = coco_string_create("\"width\" expected"); break;
			case 41: s = coco_string_create("\"cap\" expected"); break;
			case 42: s = coco_string_create("\"outline\" expected"); break;
			case 43: s = coco_string_create("\"pattern\" expected"); break;
			case 44: s = coco_string_create("\"patternMinMag\" expected"); break;
			case 45: s = coco_string_create("\"borderColor\" expected"); break;
			case 46: s = coco_string_create("\"borderWidth\" expected"); break;
			case 47: s = coco_string_create("\"borderDash\" expected"); break;
			case 48: s = coco_string_create("\"label\" expected"); break;
			case 49: s = coco_string_create("\"style\" expected"); break;
			case 50: s = coco_string_create("\"size\" expected"); break;
			case 51: s = coco_string_create("\"scaleMag\" expected"); break;
			case 52: s = coco_string_create("\"priority\" expected"); break;
			case 53: s = coco_string_create("\"backgroundColor\" expected"); break;
			case 54: s = coco_string_create("\"symbol\" expected"); break;
			case 55: s = coco_string_create("\"symbolSpace\" expected"); break;
			case 56: s = coco_string_create("\"name\" expected"); break;
			case 57: s = coco_string_create("\"butt\" expected"); break;
			case 58: s = coco_string_create("\"round\" expected"); break;
			case 59: s = coco_string_create("\"square\" expected"); break;
			case 60: s = coco_string_create("\"normal\" expected"); break;
			case 61: s = coco_string_create("\"emphasize\" expected"); break;
			case 62: s = coco_string_create("\"ref\" expected"); break;
			case 63: s = coco_string_create("\"world\" expected"); break;
			case 64: s = coco_string_create("\"continent\" expected"); break;
			case 65: s = coco_string_create("\"state\" expected"); break;
			case 66: s = coco_string_create("\"stateOver\" expected"); break;
			case 67: s = coco_string_create("\"county\" expected"); break;
			case 68: s = coco_string_create("\"region\" expected"); break;
			case 69: s = coco_string_create("\"proximity\" expected"); break;
			case 70: s = coco_string_create("\"cityOver\" expected"); break;
			case 71: s = coco_string_create("\"city\" expected"); break;
			case 72: s = coco_string_create("\"suburb\" expected"); break;
			case 73: s = coco_string_create("\"detail\" expected"); break;
			case 74: s = coco_string_create("\"close\" expected"); break;
			case 75: s = coco_string_create("\"veryClose\" expected"); break;
			case 76: s = coco_string_create("\"block\" expected"); break;
			case 77: s = coco_string_create("\"mm\" expected"); break;
			case 78: s = coco_string_create("\"m\" expected"); break;
			case 79: s = coco_string_create("??? expected"); break;
			case 80: s = coco_string_create("this symbol not expected in OSS"); break;
			case 81: s = coco_string_create("invalid STYLE"); break;
			case 82: s = coco_string_create("this symbol not expected in POLYGON"); break;
			case 83: s = coco_string_create("this symbol not expected in POLYGON"); break;
			case 84: s = coco_string_create("this symbol not expected in RECTANGLE"); break;
			case 85: s = coco_string_create("this symbol not expected in RECTANGLE"); break;
			case 86: s = coco_string_create("this symbol not expected in CIRCLE"); break;
			case 87: s = coco_string_create("this symbol not expected in CIRCLE"); break;
			case 88: s = coco_string_create("invalid FILLSTYLEATTR"); break;
			case 89: s = coco_string_create("invalid DOUBLE"); break;
			case 90: s = coco_string_create("invalid STYLEDEF"); break;
			case 91: s = coco_string_create("invalid MAG"); break;
			case 92: s = coco_string_create("this symbol not expected in NODESTYLEDEF"); break;
			case 93: s = coco_string_create("invalid NODESTYLEDEF"); break;
			case 94: s = coco_string_create("this symbol not expected in WAYSTYLEDEF"); break;
			case 95: s = coco_string_create("invalid WAYSTYLEDEF"); break;
			case 96: s = coco_string_create("invalid WAYSTYLEDEF"); break;
			case 97: s = coco_string_create("this symbol not expected in AREASTYLEDEF"); break;
			case 98: s = coco_string_create("invalid AREASTYLEDEF"); break;
			case 99: s = coco_string_create("invalid AREASTYLEDEF"); break;
			case 100: s = coco_string_create("this symbol not expected in NODETEXTSTYLE"); break;
			case 101: s = coco_string_create("this symbol not expected in NODETEXTSTYLE"); break;
			case 102: s = coco_string_create("this symbol not expected in NODETEXTSTYLE"); break;
			case 103: s = coco_string_create("this symbol not expected in NODEICONSTYLE"); break;
			case 104: s = coco_string_create("this symbol not expected in NODEICONSTYLE"); break;
			case 105: s = coco_string_create("this symbol not expected in NODEICONSTYLE"); break;
			case 106: s = coco_string_create("invalid TEXTSTYLEATTR"); break;
			case 107: s = coco_string_create("invalid ICONSTYLEATTR"); break;
			case 108: s = coco_string_create("this symbol not expected in WAYSTYLE"); break;
			case 109: s = coco_string_create("this symbol not expected in WAYSTYLE"); break;
			case 110: s = coco_string_create("this symbol not expected in WAYPATHTEXTSTYLE"); break;
			case 111: s = coco_string_create("this symbol not expected in WAYPATHTEXTSTYLE"); break;
			case 112: s = coco_string_create("this symbol not expected in WAYPATHTEXTSTYLE"); break;
			case 113: s = coco_string_create("this symbol not expected in WAYPATHSYMBOLSTYLE"); break;
			case 114: s = coco_string_create("this symbol not expected in WAYPATHSYMBOLSTYLE"); break;
			case 115: s = coco_string_create("this symbol not expected in WAYPATHSYMBOLSTYLE"); break;
			case 116: s = coco_string_create("this symbol not expected in WAYSHIELDSTYLE"); break;
			case 117: s = coco_string_create("this symbol not expected in WAYSHIELDSTYLE"); break;
			case 118: s = coco_string_create("this symbol not expected in WAYSHIELDSTYLE"); break;
			case 119: s = coco_string_create("invalid LINESTYLEATTR"); break;
			case 120: s = coco_string_create("invalid PATHTEXTSTYLEATTR"); break;
			case 121: s = coco_string_create("invalid PATHSYMBOLSTYLEATTR"); break;
			case 122: s = coco_string_create("invalid SHIELDSTYLEATTR"); break;
			case 123: s = coco_string_create("this symbol not expected in AREASTYLE"); break;
			case 124: s = coco_string_create("this symbol not expected in AREASTYLE"); break;
			case 125: s = coco_string_create("this symbol not expected in AREATEXTSTYLE"); break;
			case 126: s = coco_string_create("this symbol not expected in AREATEXTSTYLE"); break;
			case 127: s = coco_string_create("this symbol not expected in AREATEXTSTYLE"); break;
			case 128: s = coco_string_create("this symbol not expected in AREAICONSTYLE"); break;
			case 129: s = coco_string_create("this symbol not expected in AREAICONSTYLE"); break;
			case 130: s = coco_string_create("this symbol not expected in AREAICONSTYLE"); break;
			case 131: s = coco_string_create("invalid CAPSTYLE"); break;
			case 132: s = coco_string_create("invalid TEXTLABE"); break;
			case 133: s = coco_string_create("invalid LABELSTYLE"); break;
			case 134: s = coco_string_create("invalid SHIELDLABE"); break;
			case 135: s = coco_string_create("invalid PATHTEXTLABE"); break;

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

