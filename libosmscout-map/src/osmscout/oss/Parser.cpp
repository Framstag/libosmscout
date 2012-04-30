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
		while (!(la->kind == _EOF || la->kind == 6 /* "OSS" */)) {SynErr(66); Get();}
		Expect(6 /* "OSS" */);
		if (la->kind == 8 /* "ORDER" */) {
			WAYORDER();
		}
		while (la->kind == 12 /* "NODE" */ || la->kind == 13 /* "WAY" */ || la->kind == 15 /* "AREA" */) {
			STYLE();
		}
		Expect(7 /* "END" */);
		for (TypeId type=0; type<=config.GetTypeConfig()->GetMaxTypeId(); type++) {
		 if (config.GetTypeConfig()->GetTypeInfo(type).CanBeWay()) {
		if (config.GetWayPrio(type)==std::numeric_limits<size_t>::max() &&
		    config.GetWayLineStyle(type)!=NULL) {
		   std::string e="Way type '"+config.GetTypeConfig()->GetTypeInfo(type).GetName()+"' has style but is not in a group";
		   SemWarning(e.c_str());
		}
		else if (config.GetWayPrio(type)!=std::numeric_limits<size_t>::max() &&
		       config.GetWayLineStyle(type)==NULL) {
		   std::string e="Way type '"+config.GetTypeConfig()->GetTypeInfo(type).GetName()+"' is in group but has no style";
		   SemWarning(e.c_str());
		}
		 }
		}
		
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

void Parser::STYLE() {
		if (la->kind == 12 /* "NODE" */) {
			NODESTYLE();
		} else if (la->kind == 13 /* "WAY" */) {
			WAYSTYLE();
		} else if (la->kind == 15 /* "AREA" */) {
			AREASTYLE();
		} else SynErr(67);
}

void Parser::WAYGROUP(size_t priority) {
		Expect(10 /* "GROUP" */);
		if (la->kind == _string) {
			std::string wayTypeName;
			TypeId      wayType;
			
			Get();
			wayTypeName=Destring(t->val); 
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
			Expect(_string);
			wayTypeName=Destring(t->val); 
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

void Parser::NODESTYLE() {
		TypeId      type=typeIgnore;
		std::string name;
		
		while (!(la->kind == _EOF || la->kind == 12 /* "NODE" */)) {SynErr(68); Get();}
		Expect(12 /* "NODE" */);
		Expect(_string);
		name=Destring(t->val); 
		type=config.GetTypeConfig()->GetNodeTypeId(name);
		
		if (type==typeIgnore) {
		 std::string e="Unknown type '"+name+"'";
		
		 SemErr(e.c_str());
		}
		
		
		while (StartOf(1)) {
			if (la->kind == 28 /* "LABE" */) {
				LabelStyle labelStyle; 
				LABELDEF(labelStyle);
				config.SetNodeLabelStyle(type,labelStyle); 
			} else if (la->kind == 33 /* "REF" */) {
				LabelStyle refStyle; 
				REFDEF(refStyle);
				config.SetNodeRefLabelStyle(type,refStyle); 
			} else if (la->kind == 34 /* "SYMBO" */) {
				SymbolStyle symbolStyle; 
				SYMBOLDEF(symbolStyle);
				config.SetNodeSymbolStyle(type,symbolStyle); 
			} else {
				IconStyle iconStyle; 
				ICONDEF(iconStyle);
				config.SetNodeIconStyle(type,iconStyle); 
			}
		}
}

void Parser::WAYSTYLE() {
		TypeId      type=typeIgnore;
		std::string name;
		Mag         mag;
		
		while (!(la->kind == _EOF || la->kind == 13 /* "WAY" */)) {SynErr(69); Get();}
		Expect(13 /* "WAY" */);
		Expect(_string);
		name=Destring(t->val); 
		Expect(14 /* "MINMAG" */);
		MAG(mag);
		type=config.GetTypeConfig()->GetWayTypeId(name);
		
		if (type==typeIgnore) {
		 std::string e="Unknown type '"+name+"'";
		
		 SemErr(e.c_str());
		}
		else {
		 config.SetWayMag(type,mag);
		}
		
		while (la->kind == 16 /* "LINE" */ || la->kind == 28 /* "LABE" */ || la->kind == 33 /* "REF" */) {
			if (la->kind == 16 /* "LINE" */) {
				LineStyle lineStyle; 
				LINEDEF(lineStyle);
				config.SetWayLineStyle(type,lineStyle); 
			} else if (la->kind == 28 /* "LABE" */) {
				LabelStyle labelStyle; 
				LABELDEF(labelStyle);
				config.SetWayNameLabelStyle(type,labelStyle); 
			} else {
				LabelStyle refStyle; 
				REFDEF(refStyle);
				config.SetWayRefLabelStyle(type,refStyle); 
			}
		}
}

void Parser::AREASTYLE() {
		TypeId      type=typeIgnore;
		std::string name;
		std::string value;
		Mag         mag=magWorld;
		
		while (!(la->kind == _EOF || la->kind == 15 /* "AREA" */)) {SynErr(70); Get();}
		Expect(15 /* "AREA" */);
		Expect(_string);
		name=Destring(t->val); 
		if (la->kind == 14 /* "MINMAG" */) {
			Get();
			MAG(mag);
		}
		type=config.GetTypeConfig()->GetAreaTypeId(name);
		
		if (type==typeIgnore) {
		 std::string e="Unknown type '"+name+"'";
		
		 SemErr(e.c_str());
		}
		else if (mag!=magWorld) {
		 config.SetAreaMag(type,mag);
		}
		
		while (StartOf(2)) {
			if (la->kind == 25 /* "FIL" */) {
				FillStyle   fillStyle;
				
				FILLDEF(fillStyle);
				config.SetAreaFillStyle(type,fillStyle);
				
			} else if (la->kind == 28 /* "LABE" */) {
				LabelStyle labelStyle; 
				LABELDEF(labelStyle);
				config.SetAreaLabelStyle(type,labelStyle); 
			} else if (la->kind == 34 /* "SYMBO" */) {
				SymbolStyle symbolStyle; 
				SYMBOLDEF(symbolStyle);
				config.SetAreaSymbolStyle(type,symbolStyle); 
			} else {
				IconStyle iconStyle; 
				ICONDEF(iconStyle);
				config.SetAreaIconStyle(type,iconStyle); 
			}
		}
}

void Parser::LABELDEF(LabelStyle& style) {
		while (!(la->kind == _EOF || la->kind == 28 /* "LABE" */)) {SynErr(71); Get();}
		Expect(28 /* "LABE" */);
		style.SetStyle(LabelStyle::normal); 
		if (la->kind == 17 /* "WITH" */) {
			while (!(la->kind == _EOF || la->kind == 17 /* "WITH" */)) {SynErr(72); Get();}
			Get();
			if (la->kind == 36 /* "STYLE" */) {
				LabelStyle::Style s; 
				LABELSTYLE(s);
				style.SetStyle(s); 
			}
			if (la->kind == 29 /* "COLOR" */) {
				Color textColor; 
				Get();
				COLOR(textColor);
				style.SetTextColor(textColor); 
			}
			if (la->kind == 30 /* "BGCOLOR" */) {
				Color bgColor; 
				Get();
				COLOR(bgColor);
				style.SetBgColor(bgColor); 
			}
			if (la->kind == 31 /* "BORDERCOLOR" */) {
				Color borderColor; 
				Get();
				COLOR(borderColor);
				style.SetBorderColor(borderColor); 
			}
			if (la->kind == 64 /* "SIZE" */) {
				double size=style.GetSize(); 
				SIZE(size);
				style.SetSize(size); 
			}
			if (la->kind == 14 /* "MINMAG" */) {
				Mag minMag=style.GetMinMag(); 
				MINMAG(minMag);
				style.SetMinMag(minMag); 
			}
			if (la->kind == 45 /* "MAXMAG" */) {
				Mag maxMag=style.GetMaxMag(); 
				MAXMAG(maxMag);
				style.SetMaxMag(maxMag); 
			}
			if (la->kind == 46 /* "FADE" */) {
				Mag scaleMag=style.GetScaleAndFadeMag(); 
				SCALEMAG(scaleMag);
				style.SetScaleAndFadeMag(scaleMag); 
			}
			if (la->kind == 32 /* "PRIO" */) {
				size_t priority; 
				Get();
				INTEGER(priority);
				if (priority>=0 && priority<std::numeric_limits<uint8_t>::max()) {
				 style.SetPriority((uint8_t)priority);
				}
				else {
				 std::string e="Priority must be in the interval [0,"+
				               NumberToString(std::numeric_limits<uint8_t>::max())+"[";
				
				 SemErr(e.c_str());
				}
				
			}
		}
}

void Parser::REFDEF(LabelStyle& style) {
		while (!(la->kind == _EOF || la->kind == 33 /* "REF" */)) {SynErr(73); Get();}
		Expect(33 /* "REF" */);
		if (la->kind == 17 /* "WITH" */) {
			while (!(la->kind == _EOF || la->kind == 17 /* "WITH" */)) {SynErr(74); Get();}
			Get();
			if (la->kind == 36 /* "STYLE" */) {
				LabelStyle::Style s; 
				LABELSTYLE(s);
				style.SetStyle(s); 
			}
			if (la->kind == 29 /* "COLOR" */) {
				Color textColor; 
				Get();
				COLOR(textColor);
				style.SetTextColor(textColor); 
			}
			if (la->kind == 30 /* "BGCOLOR" */) {
				Color bgColor; 
				Get();
				COLOR(bgColor);
				style.SetBgColor(bgColor); 
			}
			if (la->kind == 31 /* "BORDERCOLOR" */) {
				Color borderColor; 
				Get();
				COLOR(borderColor);
				style.SetBorderColor(borderColor); 
			}
			if (la->kind == 64 /* "SIZE" */) {
				double size=style.GetSize(); 
				SIZE(size);
				style.SetSize(size); 
			}
			if (la->kind == 14 /* "MINMAG" */) {
				Mag minMag=style.GetMinMag(); 
				MINMAG(minMag);
				style.SetMinMag(minMag); 
			}
			if (la->kind == 45 /* "MAXMAG" */) {
				Mag maxMag=style.GetMaxMag(); 
				MAXMAG(maxMag);
				style.SetMaxMag(maxMag); 
			}
			if (la->kind == 46 /* "FADE" */) {
				Mag scaleMag=style.GetScaleAndFadeMag(); 
				SCALEMAG(scaleMag);
				style.SetScaleAndFadeMag(scaleMag); 
			}
			if (la->kind == 32 /* "PRIO" */) {
				size_t priority; 
				Get();
				INTEGER(priority);
				if (priority>=0 && priority<std::numeric_limits<uint8_t>::max()) {
				 style.SetPriority((uint8_t)priority);
				}
				else {
				 std::string e="Priority must be in the interval [0,"+
				               NumberToString(std::numeric_limits<uint8_t>::max())+"[";
				
				 SemErr(e.c_str());
				}
				
			}
		}
}

void Parser::SYMBOLDEF(SymbolStyle& style) {
		SymbolStyle::Style s;
		Color              fillColor;
		double             size=style.GetSize();
		
		while (!(la->kind == _EOF || la->kind == 34 /* "SYMBO" */)) {SynErr(75); Get();}
		Expect(34 /* "SYMBO" */);
		SYMBOLSTYLE(s);
		style.SetStyle(s); 
		COLOR(fillColor);
		style.SetFillColor(fillColor); 
		DOUBLE(size);
		style.SetSize(size); 
		if (la->kind == 17 /* "WITH" */) {
			while (!(la->kind == _EOF || la->kind == 17 /* "WITH" */)) {SynErr(76); Get();}
			Get();
			if (la->kind == 14 /* "MINMAG" */) {
				Mag minMag=style.GetMinMag(); 
				MINMAG(minMag);
				style.SetMinMag(minMag); 
			}
		}
}

void Parser::ICONDEF(IconStyle& style) {
		while (!(la->kind == _EOF || la->kind == 35 /* "ICON" */)) {SynErr(77); Get();}
		Expect(35 /* "ICON" */);
		Expect(_ident);
		style.SetIconName(t->val); 
		if (la->kind == 17 /* "WITH" */) {
			while (!(la->kind == _EOF || la->kind == 17 /* "WITH" */)) {SynErr(78); Get();}
			Get();
			if (la->kind == 14 /* "MINMAG" */) {
				Mag minMag=style.GetMinMag(); 
				MINMAG(minMag);
				style.SetMinMag(minMag); 
			}
		}
}

void Parser::MAG(Mag& mag) {
		switch (la->kind) {
		case 48 /* "world" */: {
			Get();
			mag=magWorld; 
			break;
		}
		case 49 /* "continent" */: {
			Get();
			mag=magContinent; 
			break;
		}
		case 50 /* "state" */: {
			Get();
			mag=magState; 
			break;
		}
		case 51 /* "stateOver" */: {
			Get();
			mag=magStateOver; 
			break;
		}
		case 52 /* "county" */: {
			Get();
			mag=magCounty; 
			break;
		}
		case 53 /* "region" */: {
			Get();
			mag=magRegion; 
			break;
		}
		case 54 /* "proximity" */: {
			Get();
			mag=magProximity; 
			break;
		}
		case 55 /* "cityOver" */: {
			Get();
			mag=magCityOver; 
			break;
		}
		case 56 /* "city" */: {
			Get();
			mag=magCity; 
			break;
		}
		case 57 /* "suburb" */: {
			Get();
			mag=magSuburb; 
			break;
		}
		case 58 /* "detail" */: {
			Get();
			mag=magDetail; 
			break;
		}
		case 59 /* "close" */: {
			Get();
			mag=magClose; 
			break;
		}
		case 60 /* "veryClose" */: {
			Get();
			mag=magVeryClose; 
			break;
		}
		case 61 /* "block" */: {
			Get();
			mag=magBlock; 
			break;
		}
		default: SynErr(79); break;
		}
}

void Parser::LINEDEF(LineStyle& style) {
		Color lineColor;
		
		while (!(la->kind == _EOF || la->kind == 16 /* "LINE" */)) {SynErr(80); Get();}
		Expect(16 /* "LINE" */);
		COLOR(lineColor);
		style.SetLineColor(lineColor); 
		if (la->kind == 17 /* "WITH" */) {
			while (!(la->kind == _EOF || la->kind == 17 /* "WITH" */)) {SynErr(81); Get();}
			Get();
			if (la->kind == 18 /* "ALTCOLOR" */) {
				Color alternateColor; 
				Get();
				COLOR(alternateColor);
				style.SetAlternateColor(alternateColor); 
			}
			if (la->kind == 19 /* "OUTLINECOLOR" */) {
				Color outlineColor; 
				Get();
				COLOR(outlineColor);
				style.SetOutlineColor(outlineColor); 
			}
			if (la->kind == 20 /* "DASH" */) {
				double dash; 
				Get();
				DOUBLE(dash);
				style.AddDashValue(dash); 
				while (la->kind == 11 /* "," */) {
					Get();
					DOUBLE(dash);
					style.AddDashValue(dash); 
				}
				if (la->kind == 21 /* "GAPCOLOR" */) {
					Color gapColor; 
					Get();
					COLOR(gapColor);
					style.SetGapColor(gapColor); 
				}
			}
			if (la->kind == 22 /* "MINWIDTH" */) {
				double minWidth=style.GetMinWidth(); 
				Get();
				DISPLAYSIZE(minWidth);
				style.SetMinWidth(minWidth); 
			}
			if (la->kind == 23 /* "WIDTH" */) {
				double width=style.GetWidth(); 
				Get();
				MAPSIZE(width);
				style.SetWidth(width); 
			}
			if (la->kind == 24 /* "OUTLINE" */) {
				double outline=style.GetOutline(); 
				Get();
				DISPLAYSIZE(outline);
				style.SetOutline(outline); 
			}
		}
}

void Parser::FILLDEF(FillStyle& style) {
		Color fillColor;
		
		while (!(la->kind == _EOF || la->kind == 25 /* "FIL" */)) {SynErr(82); Get();}
		Expect(25 /* "FIL" */);
		COLOR(fillColor);
		style.SetFillColor(fillColor); 
		if (la->kind == 26 /* "PATTERN" */) {
			while (!(la->kind == _EOF || la->kind == 26 /* "PATTERN" */)) {SynErr(83); Get();}
			Get();
			Expect(_string);
			style.SetPattern(Destring(t->val)); 
			if (la->kind == 17 /* "WITH" */) {
				while (!(la->kind == _EOF || la->kind == 17 /* "WITH" */)) {SynErr(84); Get();}
				Get();
				if (la->kind == 14 /* "MINMAG" */) {
					Mag minMag=style.GetPatternMinMag(); 
					MINMAG(minMag);
					style.SetPatternMinMag(minMag); 
				}
			}
		}
		if (la->kind == 27 /* "BORDER" */) {
			Color borderColor; 
			while (!(la->kind == _EOF || la->kind == 27 /* "BORDER" */)) {SynErr(85); Get();}
			Get();
			COLOR(borderColor);
			style.SetBorderColor(borderColor); 
			if (la->kind == 23 /* "WIDTH" */) {
				double width=style.GetBorderWidth(); 
				Get();
				DISPLAYSIZE(width);
				style.SetBorderWidth(width); 
			}
			if (la->kind == 17 /* "WITH" */) {
				while (!(la->kind == _EOF || la->kind == 17 /* "WITH" */)) {SynErr(86); Get();}
				Get();
				if (la->kind == 20 /* "DASH" */) {
					double dash; 
					Get();
					DOUBLE(dash);
					style.AddBorderDashValue(dash); 
					while (la->kind == 11 /* "," */) {
						Get();
						DOUBLE(dash);
						style.AddBorderDashValue(dash); 
					}
				}
			}
		}
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
			
		} else SynErr(87);
}

void Parser::DISPLAYSIZE(double& value) {
		DOUBLE(value);
		Expect(62 /* "mm" */);
}

void Parser::MAPSIZE(double& value) {
		DOUBLE(value);
		Expect(63 /* "m" */);
}

void Parser::MINMAG(Mag& mag) {
		Expect(14 /* "MINMAG" */);
		MAG(mag);
}

void Parser::LABELSTYLE(LabelStyle::Style& style) {
		Expect(36 /* "STYLE" */);
		if (la->kind == 37 /* "normal" */) {
			Get();
			style=LabelStyle::normal; 
		} else if (la->kind == 38 /* "contour" */) {
			Get();
			style=LabelStyle::contour; 
		} else if (la->kind == 39 /* "plate" */) {
			Get();
			style=LabelStyle::plate; 
		} else if (la->kind == 40 /* "emphasize" */) {
			Get();
			style=LabelStyle::emphasize; 
		} else SynErr(88);
}

void Parser::SIZE(double& value) {
		Expect(64 /* "SIZE" */);
		DOUBLE(value);
}

void Parser::MAXMAG(Mag& mag) {
		Expect(45 /* "MAXMAG" */);
		MAG(mag);
}

void Parser::SCALEMAG(Mag& mag) {
		Expect(46 /* "FADE" */);
		Expect(47 /* "AT" */);
		MAG(mag);
}

void Parser::INTEGER(size_t& value) {
		Expect(_number);
		if (!StringToNumber(t->val,value)) {
		 std::string e="Cannot parse number '"+std::string(t->val)+"'";
		
		 SemErr(e.c_str());
		}
		
}

void Parser::SYMBOLSTYLE(SymbolStyle::Style& style) {
		if (la->kind == 41 /* "none" */) {
			Get();
			style=SymbolStyle::none; 
		} else if (la->kind == 42 /* "box" */) {
			Get();
			style=SymbolStyle::box; 
		} else if (la->kind == 43 /* "triangle" */) {
			Get();
			style=SymbolStyle::triangle; 
		} else if (la->kind == 44 /* "circle" */) {
			Get();
			style=SymbolStyle::circle; 
		} else SynErr(89);
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
	maxT = 65;

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

	static bool set[3][67] = {
		{T,x,x,x, x,x,T,x, x,x,x,x, T,T,x,T, T,T,x,x, x,x,x,x, x,T,T,T, T,x,x,x, x,T,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, T,x,x,x, x,T,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,T,x,x, T,x,x,x, x,x,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x}
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
			case 12: s = coco_string_create("\"NODE\" expected"); break;
			case 13: s = coco_string_create("\"WAY\" expected"); break;
			case 14: s = coco_string_create("\"MINMAG\" expected"); break;
			case 15: s = coco_string_create("\"AREA\" expected"); break;
			case 16: s = coco_string_create("\"LINE\" expected"); break;
			case 17: s = coco_string_create("\"WITH\" expected"); break;
			case 18: s = coco_string_create("\"ALTCOLOR\" expected"); break;
			case 19: s = coco_string_create("\"OUTLINECOLOR\" expected"); break;
			case 20: s = coco_string_create("\"DASH\" expected"); break;
			case 21: s = coco_string_create("\"GAPCOLOR\" expected"); break;
			case 22: s = coco_string_create("\"MINWIDTH\" expected"); break;
			case 23: s = coco_string_create("\"WIDTH\" expected"); break;
			case 24: s = coco_string_create("\"OUTLINE\" expected"); break;
			case 25: s = coco_string_create("\"FILL\" expected"); break;
			case 26: s = coco_string_create("\"PATTERN\" expected"); break;
			case 27: s = coco_string_create("\"BORDER\" expected"); break;
			case 28: s = coco_string_create("\"LABEL\" expected"); break;
			case 29: s = coco_string_create("\"COLOR\" expected"); break;
			case 30: s = coco_string_create("\"BGCOLOR\" expected"); break;
			case 31: s = coco_string_create("\"BORDERCOLOR\" expected"); break;
			case 32: s = coco_string_create("\"PRIO\" expected"); break;
			case 33: s = coco_string_create("\"REF\" expected"); break;
			case 34: s = coco_string_create("\"SYMBOL\" expected"); break;
			case 35: s = coco_string_create("\"ICON\" expected"); break;
			case 36: s = coco_string_create("\"STYLE\" expected"); break;
			case 37: s = coco_string_create("\"normal\" expected"); break;
			case 38: s = coco_string_create("\"contour\" expected"); break;
			case 39: s = coco_string_create("\"plate\" expected"); break;
			case 40: s = coco_string_create("\"emphasize\" expected"); break;
			case 41: s = coco_string_create("\"none\" expected"); break;
			case 42: s = coco_string_create("\"box\" expected"); break;
			case 43: s = coco_string_create("\"triangle\" expected"); break;
			case 44: s = coco_string_create("\"circle\" expected"); break;
			case 45: s = coco_string_create("\"MAXMAG\" expected"); break;
			case 46: s = coco_string_create("\"FADE\" expected"); break;
			case 47: s = coco_string_create("\"AT\" expected"); break;
			case 48: s = coco_string_create("\"world\" expected"); break;
			case 49: s = coco_string_create("\"continent\" expected"); break;
			case 50: s = coco_string_create("\"state\" expected"); break;
			case 51: s = coco_string_create("\"stateOver\" expected"); break;
			case 52: s = coco_string_create("\"county\" expected"); break;
			case 53: s = coco_string_create("\"region\" expected"); break;
			case 54: s = coco_string_create("\"proximity\" expected"); break;
			case 55: s = coco_string_create("\"cityOver\" expected"); break;
			case 56: s = coco_string_create("\"city\" expected"); break;
			case 57: s = coco_string_create("\"suburb\" expected"); break;
			case 58: s = coco_string_create("\"detail\" expected"); break;
			case 59: s = coco_string_create("\"close\" expected"); break;
			case 60: s = coco_string_create("\"veryClose\" expected"); break;
			case 61: s = coco_string_create("\"block\" expected"); break;
			case 62: s = coco_string_create("\"mm\" expected"); break;
			case 63: s = coco_string_create("\"m\" expected"); break;
			case 64: s = coco_string_create("\"SIZE\" expected"); break;
			case 65: s = coco_string_create("??? expected"); break;
			case 66: s = coco_string_create("this symbol not expected in OSS"); break;
			case 67: s = coco_string_create("invalid STYLE"); break;
			case 68: s = coco_string_create("this symbol not expected in NODESTYLE"); break;
			case 69: s = coco_string_create("this symbol not expected in WAYSTYLE"); break;
			case 70: s = coco_string_create("this symbol not expected in AREASTYLE"); break;
			case 71: s = coco_string_create("this symbol not expected in LABELDEF"); break;
			case 72: s = coco_string_create("this symbol not expected in LABELDEF"); break;
			case 73: s = coco_string_create("this symbol not expected in REFDEF"); break;
			case 74: s = coco_string_create("this symbol not expected in REFDEF"); break;
			case 75: s = coco_string_create("this symbol not expected in SYMBOLDEF"); break;
			case 76: s = coco_string_create("this symbol not expected in SYMBOLDEF"); break;
			case 77: s = coco_string_create("this symbol not expected in ICONDEF"); break;
			case 78: s = coco_string_create("this symbol not expected in ICONDEF"); break;
			case 79: s = coco_string_create("invalid MAG"); break;
			case 80: s = coco_string_create("this symbol not expected in LINEDEF"); break;
			case 81: s = coco_string_create("this symbol not expected in LINEDEF"); break;
			case 82: s = coco_string_create("this symbol not expected in FILLDEF"); break;
			case 83: s = coco_string_create("this symbol not expected in FILLDEF"); break;
			case 84: s = coco_string_create("this symbol not expected in FILLDEF"); break;
			case 85: s = coco_string_create("this symbol not expected in FILLDEF"); break;
			case 86: s = coco_string_create("this symbol not expected in FILLDEF"); break;
			case 87: s = coco_string_create("invalid DOUBLE"); break;
			case 88: s = coco_string_create("invalid LABELSTYLE"); break;
			case 89: s = coco_string_create("invalid SYMBOLSTYLE"); break;

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

