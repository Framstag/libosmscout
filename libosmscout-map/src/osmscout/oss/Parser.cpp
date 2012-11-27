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
		while (!(la->kind == _EOF || la->kind == 6 /* "OSS" */)) {SynErr(78); Get();}
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
		
		while (la->kind == 13 /* "POLYGON" */ || la->kind == 16 /* "RECTANGLE" */ || la->kind == 18 /* "CIRCLE" */) {
			if (la->kind == 13 /* "POLYGON" */) {
				POLYGON(*symbol);
			} else if (la->kind == 16 /* "RECTANGLE" */) {
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
		if (la->kind == 19 /* "[" */) {
			STYLEFILTER(filter);
		}
		if (la->kind == 14 /* "{" */) {
			Get();
			while (StartOf(1)) {
				STYLE(filter);
			}
			Expect(15 /* "}" */);
		} else if (la->kind == 24 /* "NODE" */ || la->kind == 29 /* "WAY" */ || la->kind == 30 /* "AREA" */) {
			STYLEDEF(filter);
		} else SynErr(79);
}

void Parser::WAYGROUP(size_t priority) {
		Expect(10 /* "GROUP" */);
		if (la->kind == _string) {
			std::string wayTypeName;
			TypeId      wayType;
			
			STRING(wayTypeName);
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
			STRING(wayTypeName);
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

void Parser::STRING(std::string& value) {
		Expect(_string);
		value=Destring(t->val);
		
}

void Parser::IDENT(std::string& value) {
		Expect(_ident);
		value=t->val;
		
}

void Parser::POLYGON(Symbol& symbol) {
		Expect(13 /* "POLYGON" */);
		FillStyleRef        fillStyle(new FillStyle());
		PolygonPrimitiveRef polygon(new PolygonPrimitive(fillStyle));
		FillStyleList       fillStyles;
		Pixel               pixel;
		
		fillStyles.push_back(fillStyle);
		
		PIXEL(pixel);
		polygon->AddPixel(pixel); 
		PIXEL(pixel);
		polygon->AddPixel(pixel); 
		while (la->kind == _number || la->kind == _double) {
			PIXEL(pixel);
			polygon->AddPixel(pixel); 
		}
		while (!(la->kind == _EOF || la->kind == 14 /* "{" */)) {SynErr(80); Get();}
		Expect(14 /* "{" */);
		while (StartOf(2)) {
			FILLDEF(fillStyles);
		}
		while (!(la->kind == _EOF || la->kind == 15 /* "}" */)) {SynErr(81); Get();}
		Expect(15 /* "}" */);
		symbol.AddPrimitive(polygon); 
}

void Parser::RECTANGLE(Symbol& symbol) {
		Expect(16 /* "RECTANGLE" */);
		FillStyleRef  fillStyle(new FillStyle());
		FillStyleList fillStyles;
		Pixel         topLeft;
		double        width;
		double        height;
		
		fillStyles.push_back(fillStyle);
		
		PIXEL(topLeft);
		DOUBLE(width);
		Expect(17 /* "x" */);
		DOUBLE(height);
		while (!(la->kind == _EOF || la->kind == 14 /* "{" */)) {SynErr(82); Get();}
		Expect(14 /* "{" */);
		while (StartOf(2)) {
			FILLDEF(fillStyles);
		}
		while (!(la->kind == _EOF || la->kind == 15 /* "}" */)) {SynErr(83); Get();}
		Expect(15 /* "}" */);
		symbol.AddPrimitive(new RectanglePrimitive(topLeft,
		                                          width,height,
		                                          fillStyle));
		
}

void Parser::CIRCLE(Symbol& symbol) {
		Expect(18 /* "CIRCLE" */);
		FillStyleRef  fillStyle(new FillStyle());
		FillStyleList fillStyles;
		Pixel         center;
		double        radius;
		
		fillStyles.push_back(fillStyle);
		
		PIXEL(center);
		DOUBLE(radius);
		while (!(la->kind == _EOF || la->kind == 14 /* "{" */)) {SynErr(84); Get();}
		Expect(14 /* "{" */);
		while (StartOf(2)) {
			FILLDEF(fillStyles);
		}
		while (!(la->kind == _EOF || la->kind == 15 /* "}" */)) {SynErr(85); Get();}
		Expect(15 /* "}" */);
		symbol.AddPrimitive(new CirclePrimitive(center,
		                                       radius,
		                                       fillStyle));
		
}

void Parser::PIXEL(Pixel& pixel) {
		double x;
		double y;
		
		DOUBLE(x);
		Expect(11 /* "," */);
		DOUBLE(y);
		pixel=Pixel(x,y); 
}

void Parser::FILLDEF(FillStyleList& styles) {
		switch (la->kind) {
		case 31 /* "color" */: {
			Color fillColor; 
			Get();
			Expect(32 /* ":" */);
			COLOR(fillColor);
			ExpectWeak(33 /* ";" */, 3);
			for (FillStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 FillStyleRef style(*s);
			
			style->SetFillColor(fillColor);
			}
			
			break;
		}
		case 42 /* "pattern" */: {
			std::string patternName; 
			Get();
			Expect(32 /* ":" */);
			STRING(patternName);
			ExpectWeak(33 /* ";" */, 3);
			for (FillStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 FillStyleRef style(*s);
			
			style->SetPattern(patternName);
			}
			
			break;
		}
		case 43 /* "patternMinMag" */: {
			Mag minMag; 
			Get();
			Expect(32 /* ":" */);
			MAG(minMag);
			ExpectWeak(33 /* ";" */, 3);
			for (FillStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 FillStyleRef style(*s);
			
			style->SetPatternMinMag(minMag);
			}
			
			break;
		}
		case 44 /* "borderColor" */: {
			Color borderColor; 
			Get();
			Expect(32 /* ":" */);
			COLOR(borderColor);
			ExpectWeak(33 /* ";" */, 3);
			for (FillStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 FillStyleRef style(*s);
			
			style->SetBorderColor(borderColor);
			}
			
			break;
		}
		case 45 /* "borderWidth" */: {
			double width; 
			Get();
			Expect(32 /* ":" */);
			DISPLAYSIZE(width);
			ExpectWeak(33 /* ";" */, 3);
			for (FillStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 FillStyleRef style(*s);
			
			style->SetBorderWidth(width);
			}
			
			break;
		}
		case 46 /* "borderDash" */: {
			std::vector<double> dashes;
			double              dash;
			
			Get();
			Expect(32 /* ":" */);
			DOUBLE(dash);
			dashes.push_back(dash); 
			while (la->kind == 11 /* "," */) {
				Get();
				DOUBLE(dash);
				dashes.push_back(dash); 
			}
			for (FillStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 FillStyleRef style(*s);
			
			 style->SetBorderDashes(dashes);
			}
			
			ExpectWeak(33 /* ";" */, 3);
			break;
		}
		default: SynErr(86); break;
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

void Parser::STYLEFILTER(StyleFilter& filter) {
		Expect(19 /* "[" */);
		if (la->kind == 20 /* "TYPE" */) {
			TypeSet     types;
			std::string name;
			
			Get();
			STRING(name);
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
				STRING(name);
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
		if (la->kind == 21 /* "MAG" */) {
			Get();
			if (StartOf(4)) {
				Mag mag; 
				MAG(mag);
				size_t level=MagToLevel(mag);
				
				if (level<filter.GetMinLevel()) {
				std::string e="The magnification interval start is not within the parent magnification range";
				
				SemErr(e.c_str());
				}
				else {
				 filter.SetMinLevel(level);
				}
				
			}
			Expect(22 /* "-" */);
			if (StartOf(4)) {
				Mag mag; 
				MAG(mag);
				size_t level=MagToLevel(mag);
				
				if (level>filter.GetMaxLevel()) {
				std::string e="The magnification interval end is not within the parent magnification range";
				
				SemErr(e.c_str());
				}
				else {
				 filter.SetMaxLevel(level);
				}
				
			}
		}
		Expect(23 /* "]" */);
}

void Parser::STYLEDEF(StyleFilter filter) {
		if (la->kind == 24 /* "NODE" */) {
			NODESTYLEDEF(filter);
		} else if (la->kind == 29 /* "WAY" */) {
			WAYSTYLEDEF(filter);
		} else if (la->kind == 30 /* "AREA" */) {
			AREASTYLEDEF(filter);
		} else SynErr(88);
}

void Parser::MAG(Mag& mag) {
		switch (la->kind) {
		case 61 /* "world" */: {
			Get();
			mag=magWorld; 
			break;
		}
		case 62 /* "continent" */: {
			Get();
			mag=magContinent; 
			break;
		}
		case 63 /* "state" */: {
			Get();
			mag=magState; 
			break;
		}
		case 64 /* "stateOver" */: {
			Get();
			mag=magStateOver; 
			break;
		}
		case 65 /* "county" */: {
			Get();
			mag=magCounty; 
			break;
		}
		case 66 /* "region" */: {
			Get();
			mag=magRegion; 
			break;
		}
		case 67 /* "proximity" */: {
			Get();
			mag=magProximity; 
			break;
		}
		case 68 /* "cityOver" */: {
			Get();
			mag=magCityOver; 
			break;
		}
		case 69 /* "city" */: {
			Get();
			mag=magCity; 
			break;
		}
		case 70 /* "suburb" */: {
			Get();
			mag=magSuburb; 
			break;
		}
		case 71 /* "detail" */: {
			Get();
			mag=magDetail; 
			break;
		}
		case 72 /* "close" */: {
			Get();
			mag=magClose; 
			break;
		}
		case 73 /* "veryClose" */: {
			Get();
			mag=magVeryClose; 
			break;
		}
		case 74 /* "block" */: {
			Get();
			mag=magBlock; 
			break;
		}
		default: SynErr(89); break;
		}
}

void Parser::NODESTYLEDEF(StyleFilter filter) {
		while (!(la->kind == _EOF || la->kind == 24 /* "NODE" */)) {SynErr(90); Get();}
		Expect(24 /* "NODE" */);
		Expect(25 /* "." */);
		if (la->kind == 26 /* "LABE" */) {
			NODELABELSTYLE(filter);
		} else if (la->kind == 27 /* "REF" */) {
			NODEREFSTYLE(filter);
		} else if (la->kind == 28 /* "ICON" */) {
			NODEICONSTYLE(filter);
		} else SynErr(91);
}

void Parser::WAYSTYLEDEF(StyleFilter filter) {
		while (!(la->kind == _EOF || la->kind == 29 /* "WAY" */)) {SynErr(92); Get();}
		Expect(29 /* "WAY" */);
		if (la->kind == 14 /* "{" */ || la->kind == 19 /* "[" */) {
			WAYSTYLE(filter);
		} else if (la->kind == 25 /* "." */) {
			Get();
			if (la->kind == 26 /* "LABE" */) {
				WAYLABELSTYLE(filter);
			} else if (la->kind == 27 /* "REF" */) {
				WAYREFSTYLE(filter);
			} else SynErr(93);
		} else SynErr(94);
}

void Parser::AREASTYLEDEF(StyleFilter filter) {
		while (!(la->kind == _EOF || la->kind == 30 /* "AREA" */)) {SynErr(95); Get();}
		Expect(30 /* "AREA" */);
		if (la->kind == 14 /* "{" */ || la->kind == 19 /* "[" */) {
			AREASTYLE(filter);
		} else if (la->kind == 25 /* "." */) {
			Get();
			if (la->kind == 26 /* "LABE" */) {
				AREALABELSTYLE(filter);
			} else if (la->kind == 28 /* "ICON" */) {
				AREAICONSTYLE(filter);
			} else SynErr(96);
		} else SynErr(97);
}

void Parser::NODELABELSTYLE(StyleFilter filter) {
		while (!(la->kind == _EOF || la->kind == 26 /* "LABE" */)) {SynErr(98); Get();}
		Expect(26 /* "LABE" */);
		if (la->kind == 19 /* "[" */) {
			STYLEFILTER(filter);
		}
		LabelStyleList labelStyles;
		
		config.GetNodeNameLabelStyles(filter,labelStyles);
		
		while (!(la->kind == _EOF || la->kind == 14 /* "{" */)) {SynErr(99); Get();}
		Expect(14 /* "{" */);
		while (StartOf(5)) {
			LABELDEF(labelStyles);
		}
		while (!(la->kind == _EOF || la->kind == 15 /* "}" */)) {SynErr(100); Get();}
		Expect(15 /* "}" */);
}

void Parser::NODEREFSTYLE(StyleFilter filter) {
		while (!(la->kind == _EOF || la->kind == 27 /* "REF" */)) {SynErr(101); Get();}
		Expect(27 /* "REF" */);
		if (la->kind == 19 /* "[" */) {
			STYLEFILTER(filter);
		}
		LabelStyleList labelStyles;
		
		config.GetNodeRefLabelStyles(filter,labelStyles);
		
		while (!(la->kind == _EOF || la->kind == 14 /* "{" */)) {SynErr(102); Get();}
		Expect(14 /* "{" */);
		while (StartOf(5)) {
			REFDEF(labelStyles);
		}
		while (!(la->kind == _EOF || la->kind == 15 /* "}" */)) {SynErr(103); Get();}
		Expect(15 /* "}" */);
}

void Parser::NODEICONSTYLE(StyleFilter filter) {
		while (!(la->kind == _EOF || la->kind == 28 /* "ICON" */)) {SynErr(104); Get();}
		Expect(28 /* "ICON" */);
		if (la->kind == 19 /* "[" */) {
			STYLEFILTER(filter);
		}
		IconStyleList iconStyles;
		
		config.GetNodeIconStyles(filter,iconStyles);
		
		while (!(la->kind == _EOF || la->kind == 14 /* "{" */)) {SynErr(105); Get();}
		Expect(14 /* "{" */);
		while (la->kind == 52 /* "symbol" */ || la->kind == 53 /* "name" */) {
			ICONDEF(iconStyles);
		}
		while (!(la->kind == _EOF || la->kind == 15 /* "}" */)) {SynErr(106); Get();}
		Expect(15 /* "}" */);
}

void Parser::LABELDEF(LabelStyleList& styles) {
		switch (la->kind) {
		case 47 /* "style" */: {
			LabelStyle::Style labelStyle; 
			Get();
			Expect(32 /* ":" */);
			LABELSTYLE(labelStyle);
			ExpectWeak(33 /* ";" */, 6);
			for (LabelStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 LabelStyleRef style(*s);
			
			style->SetStyle(labelStyle);
			}
			
			break;
		}
		case 31 /* "color" */: {
			Color textColor; 
			Get();
			Expect(32 /* ":" */);
			COLOR(textColor);
			ExpectWeak(33 /* ";" */, 6);
			for (LabelStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 LabelStyleRef style(*s);
			
			style->SetTextColor(textColor);
			}
			
			break;
		}
		case 48 /* "backgroundColor" */: {
			Color bgColor; 
			Get();
			Expect(32 /* ":" */);
			COLOR(bgColor);
			ExpectWeak(33 /* ";" */, 6);
			for (LabelStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 LabelStyleRef style(*s);
			
			style->SetBgColor(bgColor);
			}
			
			break;
		}
		case 44 /* "borderColor" */: {
			Color borderColor; 
			Get();
			Expect(32 /* ":" */);
			COLOR(borderColor);
			ExpectWeak(33 /* ";" */, 6);
			for (LabelStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 LabelStyleRef style(*s);
			
			style->SetBorderColor(borderColor);
			}
			
			break;
		}
		case 49 /* "size" */: {
			double size; 
			Get();
			Expect(32 /* ":" */);
			DOUBLE(size);
			ExpectWeak(33 /* ";" */, 6);
			for (LabelStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 LabelStyleRef style(*s);
			
			style->SetSize(size);
			}
			
			break;
		}
		case 50 /* "scaleMag" */: {
			Mag scaleMag; 
			Get();
			Expect(32 /* ":" */);
			MAG(scaleMag);
			ExpectWeak(33 /* ";" */, 6);
			for (LabelStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 LabelStyleRef style(*s);
			
			style->SetScaleAndFadeMag(scaleMag);
			}
			
			break;
		}
		case 51 /* "priority" */: {
			size_t priority; 
			Get();
			Expect(32 /* ":" */);
			INTEGER(priority);
			ExpectWeak(33 /* ";" */, 6);
			if (priority>=0 && priority<std::numeric_limits<uint8_t>::max()) {
			    for (LabelStyleList::iterator s=styles.begin();
			         s!=styles.end();
			         ++s) {
			      LabelStyleRef style(*s);
			
				    style->SetPriority((uint8_t)priority);
				  }
			}
			else {
			 std::string e="Priority must be in the interval [0,"+
			               NumberToString(std::numeric_limits<uint8_t>::max())+"[";
			
			 SemErr(e.c_str());
			}
			
			break;
		}
		default: SynErr(107); break;
		}
}

void Parser::REFDEF(LabelStyleList& styles) {
		for (LabelStyleList::iterator s=styles.begin();
		    s!=styles.end();
		    ++s) {
		 LabelStyleRef style(*s);
		
		 style->SetStyle(LabelStyle::plate);
		}
		
		switch (la->kind) {
		case 47 /* "style" */: {
			LabelStyle::Style labelStyle; 
			Get();
			Expect(32 /* ":" */);
			LABELSTYLE(labelStyle);
			ExpectWeak(33 /* ";" */, 6);
			for (LabelStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 LabelStyleRef style(*s);
			
			style->SetStyle(labelStyle);
			}
			
			break;
		}
		case 31 /* "color" */: {
			Color textColor; 
			Get();
			Expect(32 /* ":" */);
			COLOR(textColor);
			ExpectWeak(33 /* ";" */, 6);
			for (LabelStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 LabelStyleRef style(*s);
			
			style->SetTextColor(textColor);
			}
			
			break;
		}
		case 48 /* "backgroundColor" */: {
			Color bgColor; 
			Get();
			Expect(32 /* ":" */);
			COLOR(bgColor);
			ExpectWeak(33 /* ";" */, 6);
			for (LabelStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 LabelStyleRef style(*s);
			
			style->SetBgColor(bgColor);
			}
			
			break;
		}
		case 44 /* "borderColor" */: {
			Color borderColor; 
			Get();
			Expect(32 /* ":" */);
			COLOR(borderColor);
			ExpectWeak(33 /* ";" */, 6);
			for (LabelStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 LabelStyleRef style(*s);
			
			style->SetBorderColor(borderColor);
			}
			
			break;
		}
		case 49 /* "size" */: {
			double size; 
			Get();
			Expect(32 /* ":" */);
			DOUBLE(size);
			ExpectWeak(33 /* ";" */, 6);
			for (LabelStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 LabelStyleRef style(*s);
			
			style->SetSize(size);
			}
			
			break;
		}
		case 50 /* "scaleMag" */: {
			Mag scaleMag; 
			Get();
			Expect(32 /* ":" */);
			MAG(scaleMag);
			ExpectWeak(33 /* ";" */, 6);
			for (LabelStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 LabelStyleRef style(*s);
			
			style->SetScaleAndFadeMag(scaleMag);
			}
			
			break;
		}
		case 51 /* "priority" */: {
			size_t priority; 
			Get();
			Expect(32 /* ":" */);
			INTEGER(priority);
			ExpectWeak(33 /* ";" */, 6);
			if (priority>=0 && priority<std::numeric_limits<uint8_t>::max()) {
			    for (LabelStyleList::iterator s=styles.begin();
			         s!=styles.end();
			         ++s) {
			      LabelStyleRef style(*s);
			
				    style->SetPriority((uint8_t)priority);
				  }
			}
			else {
			 std::string e="Priority must be in the interval [0,"+
			               NumberToString(std::numeric_limits<uint8_t>::max())+"[";
			
			 SemErr(e.c_str());
			}
			
			break;
		}
		default: SynErr(108); break;
		}
}

void Parser::ICONDEF(IconStyleList& styles) {
		if (la->kind == 52 /* "symbol" */) {
			std::string name;
			SymbolRef   symbol;
			
			Get();
			Expect(32 /* ":" */);
			IDENT(name);
			ExpectWeak(33 /* ";" */, 7);
			symbol=config.GetSymbol(name);
			
			if (symbol.Invalid()) {
			 std::string e="Map symbol '"+symbol->GetName()+"' is not defined";
			
			 SemErr(e.c_str());
			}
			
			for (IconStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 IconStyleRef style(*s);
			
			style->SetSymbol(symbol);
			}
			
		} else if (la->kind == 53 /* "name" */) {
			std::string name; 
			Get();
			Expect(32 /* ":" */);
			IDENT(name);
			ExpectWeak(33 /* ";" */, 7);
			for (IconStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 IconStyleRef style(*s);
			
			style->SetIconName(name);
			}
			
		} else SynErr(109);
}

void Parser::WAYSTYLE(StyleFilter filter) {
		if (la->kind == 19 /* "[" */) {
			STYLEFILTER(filter);
		}
		LineStyleList lineStyles;
		
		config.GetWayLineStyles(filter,lineStyles);
		
		while (!(la->kind == _EOF || la->kind == 14 /* "{" */)) {SynErr(110); Get();}
		Expect(14 /* "{" */);
		while (StartOf(8)) {
			LINEDEF(lineStyles);
		}
		while (!(la->kind == _EOF || la->kind == 15 /* "}" */)) {SynErr(111); Get();}
		Expect(15 /* "}" */);
}

void Parser::WAYLABELSTYLE(StyleFilter filter) {
		while (!(la->kind == _EOF || la->kind == 26 /* "LABE" */)) {SynErr(112); Get();}
		Expect(26 /* "LABE" */);
		if (la->kind == 19 /* "[" */) {
			STYLEFILTER(filter);
		}
		LabelStyleList labelStyles;
		
		config.GetWayNameLabelStyles(filter,labelStyles);
		
		Expect(14 /* "{" */);
		while (StartOf(5)) {
			LABELDEF(labelStyles);
		}
		Expect(15 /* "}" */);
}

void Parser::WAYREFSTYLE(StyleFilter filter) {
		while (!(la->kind == _EOF || la->kind == 27 /* "REF" */)) {SynErr(113); Get();}
		Expect(27 /* "REF" */);
		if (la->kind == 19 /* "[" */) {
			STYLEFILTER(filter);
		}
		LabelStyleList labelStyles;
		
		config.GetWayRefLabelStyles(filter,labelStyles);
		
		while (!(la->kind == _EOF || la->kind == 14 /* "{" */)) {SynErr(114); Get();}
		Expect(14 /* "{" */);
		while (StartOf(5)) {
			LABELDEF(labelStyles);
		}
		while (!(la->kind == _EOF || la->kind == 15 /* "}" */)) {SynErr(115); Get();}
		Expect(15 /* "}" */);
}

void Parser::LINEDEF(LineStyleList& styles) {
		switch (la->kind) {
		case 31 /* "color" */: {
			Color lineColor; 
			Get();
			Expect(32 /* ":" */);
			COLOR(lineColor);
			ExpectWeak(33 /* ";" */, 9);
			for (LineStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 LineStyleRef style(*s);
			
			style->SetLineColor(lineColor);
			}
			
			break;
		}
		case 34 /* "altColor" */: {
			Color alternateColor; 
			Get();
			Expect(32 /* ":" */);
			COLOR(alternateColor);
			ExpectWeak(33 /* ";" */, 9);
			for (LineStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 LineStyleRef style(*s);
			
			style->SetAlternateColor(alternateColor);
			}
			
			break;
		}
		case 35 /* "outlineColor" */: {
			Color outlineColor;
			Get();
			Expect(32 /* ":" */);
			COLOR(outlineColor);
			ExpectWeak(33 /* ";" */, 9);
			for (LineStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 LineStyleRef style(*s);
			
			style->SetOutlineColor(outlineColor);
			}
			
			break;
		}
		case 36 /* "dash" */: {
			std::vector<double> dashes;
			double              dash;
			
			Get();
			Expect(32 /* ":" */);
			DOUBLE(dash);
			dashes.push_back(dash); 
			while (la->kind == 11 /* "," */) {
				Get();
				DOUBLE(dash);
				dashes.push_back(dash); 
			}
			for (LineStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 LineStyleRef style(*s);
			
			 style->SetDashes(dashes);
			}
			
			ExpectWeak(33 /* ";" */, 9);
			break;
		}
		case 37 /* "gapColor" */: {
			Color gapColor; 
			Get();
			Expect(32 /* ":" */);
			COLOR(gapColor);
			ExpectWeak(33 /* ";" */, 9);
			for (LineStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 LineStyleRef style(*s);
			
			style->SetGapColor(gapColor);
			}
			
			break;
		}
		case 38 /* "displayWidth" */: {
			double displayWidth; 
			Get();
			Expect(32 /* ":" */);
			DISPLAYSIZE(displayWidth);
			ExpectWeak(33 /* ";" */, 9);
			for (LineStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 LineStyleRef style(*s);
			
			style->SetDisplayWidth(displayWidth);
			}
			
			break;
		}
		case 39 /* "width" */: {
			double width; 
			Get();
			Expect(32 /* ":" */);
			MAPSIZE(width);
			ExpectWeak(33 /* ";" */, 9);
			for (LineStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 LineStyleRef style(*s);
			
			style->SetWidth(width);
			}
			
			break;
		}
		case 40 /* "cap" */: {
			LineStyle::CapStyle capStyle; 
			Get();
			Expect(32 /* ":" */);
			CAPSTYLE(capStyle);
			ExpectWeak(33 /* ";" */, 9);
			for (LineStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 LineStyleRef style(*s);
			
			style->SetCapStyle(capStyle);
			}
			
			break;
		}
		case 41 /* "outline" */: {
			double outline; 
			Get();
			Expect(32 /* ":" */);
			DISPLAYSIZE(outline);
			ExpectWeak(33 /* ";" */, 9);
			for (LineStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 LineStyleRef style(*s);
			
			style->SetOutline(outline);
			}
			
			break;
		}
		default: SynErr(116); break;
		}
}

void Parser::AREASTYLE(StyleFilter filter) {
		if (la->kind == 19 /* "[" */) {
			STYLEFILTER(filter);
		}
		FillStyleList fillStyles;
		
		config.GetAreaFillStyles(filter,fillStyles);
		
		while (!(la->kind == _EOF || la->kind == 14 /* "{" */)) {SynErr(117); Get();}
		Expect(14 /* "{" */);
		while (StartOf(2)) {
			FILLDEF(fillStyles);
		}
		while (!(la->kind == _EOF || la->kind == 15 /* "}" */)) {SynErr(118); Get();}
		Expect(15 /* "}" */);
}

void Parser::AREALABELSTYLE(StyleFilter filter) {
		while (!(la->kind == _EOF || la->kind == 26 /* "LABE" */)) {SynErr(119); Get();}
		Expect(26 /* "LABE" */);
		if (la->kind == 19 /* "[" */) {
			STYLEFILTER(filter);
		}
		LabelStyleList labelStyles;
		
		config.GetAreaLabelStyles(filter,labelStyles);
		
		while (!(la->kind == _EOF || la->kind == 14 /* "{" */)) {SynErr(120); Get();}
		Expect(14 /* "{" */);
		while (StartOf(5)) {
			LABELDEF(labelStyles);
		}
		while (!(la->kind == _EOF || la->kind == 15 /* "}" */)) {SynErr(121); Get();}
		Expect(15 /* "}" */);
}

void Parser::AREAICONSTYLE(StyleFilter filter) {
		while (!(la->kind == _EOF || la->kind == 28 /* "ICON" */)) {SynErr(122); Get();}
		Expect(28 /* "ICON" */);
		if (la->kind == 19 /* "[" */) {
			STYLEFILTER(filter);
		}
		IconStyleList iconStyles;
		
		config.GetAreaIconStyles(filter,iconStyles);
		
		while (!(la->kind == _EOF || la->kind == 14 /* "{" */)) {SynErr(123); Get();}
		Expect(14 /* "{" */);
		while (la->kind == 52 /* "symbol" */ || la->kind == 53 /* "name" */) {
			ICONDEF(iconStyles);
		}
		while (!(la->kind == _EOF || la->kind == 15 /* "}" */)) {SynErr(124); Get();}
		Expect(15 /* "}" */);
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
		Expect(75 /* "mm" */);
}

void Parser::MAPSIZE(double& value) {
		DOUBLE(value);
		Expect(76 /* "m" */);
}

void Parser::CAPSTYLE(LineStyle::CapStyle& style) {
		if (la->kind == 54 /* "butt" */) {
			Get();
			style=LineStyle::capButt; 
		} else if (la->kind == 55 /* "round" */) {
			Get();
			style=LineStyle::capRound; 
		} else if (la->kind == 56 /* "square" */) {
			Get();
			style=LineStyle::capSquare; 
		} else SynErr(125);
}

void Parser::LABELSTYLE(LabelStyle::Style& style) {
		if (la->kind == 57 /* "normal" */) {
			Get();
			style=LabelStyle::normal; 
		} else if (la->kind == 58 /* "contour" */) {
			Get();
			style=LabelStyle::contour; 
		} else if (la->kind == 59 /* "plate" */) {
			Get();
			style=LabelStyle::plate; 
		} else if (la->kind == 60 /* "emphasize" */) {
			Get();
			style=LabelStyle::emphasize; 
		} else SynErr(126);
}

void Parser::INTEGER(size_t& value) {
		Expect(_number);
		if (!StringToNumber(t->val,value)) {
		 std::string e="Cannot parse number '"+std::string(t->val)+"'";
		
		 SemErr(e.c_str());
		}
		
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
	maxT = 77;

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

	static bool set[10][79] = {
		{T,x,x,x, x,x,T,x, x,x,x,x, x,x,T,T, x,x,x,x, x,x,x,x, T,x,T,T, T,T,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,T,x, x,x,x,T, x,x,x,x, T,x,x,x, x,T,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,T, x,x,x,x, x,x,x,x, x,x,T,T, T,T,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{T,x,x,x, x,x,T,x, x,x,x,x, x,x,T,T, x,x,x,x, x,x,x,x, T,x,T,T, T,T,T,T, x,x,x,x, x,x,x,x, x,x,T,T, T,T,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,T,T,T, T,T,T,T, T,T,T,T, T,T,T,x, x,x,x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,T, x,x,x,x, x,x,x,x, x,x,x,x, T,x,x,T, T,T,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{T,x,x,x, x,x,T,x, x,x,x,x, x,x,T,T, x,x,x,x, x,x,x,x, T,x,T,T, T,T,T,T, x,x,x,x, x,x,x,x, x,x,x,x, T,x,x,T, T,T,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{T,x,x,x, x,x,T,x, x,x,x,x, x,x,T,T, x,x,x,x, x,x,x,x, T,x,T,T, T,T,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, T,T,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,T, x,x,T,T, T,T,T,T, T,T,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{T,x,x,x, x,x,T,x, x,x,x,x, x,x,T,T, x,x,x,x, x,x,x,x, T,x,T,T, T,T,T,T, x,x,T,T, T,T,T,T, T,T,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x}
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
			case 15: s = coco_string_create("\"}\" expected"); break;
			case 16: s = coco_string_create("\"RECTANGLE\" expected"); break;
			case 17: s = coco_string_create("\"x\" expected"); break;
			case 18: s = coco_string_create("\"CIRCLE\" expected"); break;
			case 19: s = coco_string_create("\"[\" expected"); break;
			case 20: s = coco_string_create("\"TYPE\" expected"); break;
			case 21: s = coco_string_create("\"MAG\" expected"); break;
			case 22: s = coco_string_create("\"-\" expected"); break;
			case 23: s = coco_string_create("\"]\" expected"); break;
			case 24: s = coco_string_create("\"NODE\" expected"); break;
			case 25: s = coco_string_create("\".\" expected"); break;
			case 26: s = coco_string_create("\"LABEL\" expected"); break;
			case 27: s = coco_string_create("\"REF\" expected"); break;
			case 28: s = coco_string_create("\"ICON\" expected"); break;
			case 29: s = coco_string_create("\"WAY\" expected"); break;
			case 30: s = coco_string_create("\"AREA\" expected"); break;
			case 31: s = coco_string_create("\"color\" expected"); break;
			case 32: s = coco_string_create("\":\" expected"); break;
			case 33: s = coco_string_create("\";\" expected"); break;
			case 34: s = coco_string_create("\"altColor\" expected"); break;
			case 35: s = coco_string_create("\"outlineColor\" expected"); break;
			case 36: s = coco_string_create("\"dash\" expected"); break;
			case 37: s = coco_string_create("\"gapColor\" expected"); break;
			case 38: s = coco_string_create("\"displayWidth\" expected"); break;
			case 39: s = coco_string_create("\"width\" expected"); break;
			case 40: s = coco_string_create("\"cap\" expected"); break;
			case 41: s = coco_string_create("\"outline\" expected"); break;
			case 42: s = coco_string_create("\"pattern\" expected"); break;
			case 43: s = coco_string_create("\"patternMinMag\" expected"); break;
			case 44: s = coco_string_create("\"borderColor\" expected"); break;
			case 45: s = coco_string_create("\"borderWidth\" expected"); break;
			case 46: s = coco_string_create("\"borderDash\" expected"); break;
			case 47: s = coco_string_create("\"style\" expected"); break;
			case 48: s = coco_string_create("\"backgroundColor\" expected"); break;
			case 49: s = coco_string_create("\"size\" expected"); break;
			case 50: s = coco_string_create("\"scaleMag\" expected"); break;
			case 51: s = coco_string_create("\"priority\" expected"); break;
			case 52: s = coco_string_create("\"symbol\" expected"); break;
			case 53: s = coco_string_create("\"name\" expected"); break;
			case 54: s = coco_string_create("\"butt\" expected"); break;
			case 55: s = coco_string_create("\"round\" expected"); break;
			case 56: s = coco_string_create("\"square\" expected"); break;
			case 57: s = coco_string_create("\"normal\" expected"); break;
			case 58: s = coco_string_create("\"contour\" expected"); break;
			case 59: s = coco_string_create("\"plate\" expected"); break;
			case 60: s = coco_string_create("\"emphasize\" expected"); break;
			case 61: s = coco_string_create("\"world\" expected"); break;
			case 62: s = coco_string_create("\"continent\" expected"); break;
			case 63: s = coco_string_create("\"state\" expected"); break;
			case 64: s = coco_string_create("\"stateOver\" expected"); break;
			case 65: s = coco_string_create("\"county\" expected"); break;
			case 66: s = coco_string_create("\"region\" expected"); break;
			case 67: s = coco_string_create("\"proximity\" expected"); break;
			case 68: s = coco_string_create("\"cityOver\" expected"); break;
			case 69: s = coco_string_create("\"city\" expected"); break;
			case 70: s = coco_string_create("\"suburb\" expected"); break;
			case 71: s = coco_string_create("\"detail\" expected"); break;
			case 72: s = coco_string_create("\"close\" expected"); break;
			case 73: s = coco_string_create("\"veryClose\" expected"); break;
			case 74: s = coco_string_create("\"block\" expected"); break;
			case 75: s = coco_string_create("\"mm\" expected"); break;
			case 76: s = coco_string_create("\"m\" expected"); break;
			case 77: s = coco_string_create("??? expected"); break;
			case 78: s = coco_string_create("this symbol not expected in OSS"); break;
			case 79: s = coco_string_create("invalid STYLE"); break;
			case 80: s = coco_string_create("this symbol not expected in POLYGON"); break;
			case 81: s = coco_string_create("this symbol not expected in POLYGON"); break;
			case 82: s = coco_string_create("this symbol not expected in RECTANGLE"); break;
			case 83: s = coco_string_create("this symbol not expected in RECTANGLE"); break;
			case 84: s = coco_string_create("this symbol not expected in CIRCLE"); break;
			case 85: s = coco_string_create("this symbol not expected in CIRCLE"); break;
			case 86: s = coco_string_create("invalid FILLDEF"); break;
			case 87: s = coco_string_create("invalid DOUBLE"); break;
			case 88: s = coco_string_create("invalid STYLEDEF"); break;
			case 89: s = coco_string_create("invalid MAG"); break;
			case 90: s = coco_string_create("this symbol not expected in NODESTYLEDEF"); break;
			case 91: s = coco_string_create("invalid NODESTYLEDEF"); break;
			case 92: s = coco_string_create("this symbol not expected in WAYSTYLEDEF"); break;
			case 93: s = coco_string_create("invalid WAYSTYLEDEF"); break;
			case 94: s = coco_string_create("invalid WAYSTYLEDEF"); break;
			case 95: s = coco_string_create("this symbol not expected in AREASTYLEDEF"); break;
			case 96: s = coco_string_create("invalid AREASTYLEDEF"); break;
			case 97: s = coco_string_create("invalid AREASTYLEDEF"); break;
			case 98: s = coco_string_create("this symbol not expected in NODELABELSTYLE"); break;
			case 99: s = coco_string_create("this symbol not expected in NODELABELSTYLE"); break;
			case 100: s = coco_string_create("this symbol not expected in NODELABELSTYLE"); break;
			case 101: s = coco_string_create("this symbol not expected in NODEREFSTYLE"); break;
			case 102: s = coco_string_create("this symbol not expected in NODEREFSTYLE"); break;
			case 103: s = coco_string_create("this symbol not expected in NODEREFSTYLE"); break;
			case 104: s = coco_string_create("this symbol not expected in NODEICONSTYLE"); break;
			case 105: s = coco_string_create("this symbol not expected in NODEICONSTYLE"); break;
			case 106: s = coco_string_create("this symbol not expected in NODEICONSTYLE"); break;
			case 107: s = coco_string_create("invalid LABELDEF"); break;
			case 108: s = coco_string_create("invalid REFDEF"); break;
			case 109: s = coco_string_create("invalid ICONDEF"); break;
			case 110: s = coco_string_create("this symbol not expected in WAYSTYLE"); break;
			case 111: s = coco_string_create("this symbol not expected in WAYSTYLE"); break;
			case 112: s = coco_string_create("this symbol not expected in WAYLABELSTYLE"); break;
			case 113: s = coco_string_create("this symbol not expected in WAYREFSTYLE"); break;
			case 114: s = coco_string_create("this symbol not expected in WAYREFSTYLE"); break;
			case 115: s = coco_string_create("this symbol not expected in WAYREFSTYLE"); break;
			case 116: s = coco_string_create("invalid LINEDEF"); break;
			case 117: s = coco_string_create("this symbol not expected in AREASTYLE"); break;
			case 118: s = coco_string_create("this symbol not expected in AREASTYLE"); break;
			case 119: s = coco_string_create("this symbol not expected in AREALABELSTYLE"); break;
			case 120: s = coco_string_create("this symbol not expected in AREALABELSTYLE"); break;
			case 121: s = coco_string_create("this symbol not expected in AREALABELSTYLE"); break;
			case 122: s = coco_string_create("this symbol not expected in AREAICONSTYLE"); break;
			case 123: s = coco_string_create("this symbol not expected in AREAICONSTYLE"); break;
			case 124: s = coco_string_create("this symbol not expected in AREAICONSTYLE"); break;
			case 125: s = coco_string_create("invalid CAPSTYLE"); break;
			case 126: s = coco_string_create("invalid LABELSTYLE"); break;

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

