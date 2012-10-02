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
		while (!(la->kind == _EOF || la->kind == 6 /* "OSS" */)) {SynErr(77); Get();}
		Expect(6 /* "OSS" */);
		if (la->kind == 8 /* "ORDER" */) {
			WAYORDER();
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

void Parser::STYLE(StyleFilter filter) {
		if (la->kind == 14 /* "[" */) {
			STYLEFILTER(filter);
		}
		if (la->kind == 12 /* "{" */) {
			Get();
			while (StartOf(1)) {
				STYLE(filter);
			}
			Expect(13 /* "}" */);
		} else if (la->kind == 19 /* "NODE" */ || la->kind == 25 /* "WAY" */ || la->kind == 26 /* "AREA" */) {
			STYLEDEF(filter);
		} else SynErr(78);
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

void Parser::STYLEFILTER(StyleFilter& filter) {
		Expect(14 /* "[" */);
		if (la->kind == 15 /* "TYPE" */) {
			TypeSet types;
			Get();
			Expect(_string);
			std::string name=Destring(t->val);
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
			
			while (la->kind == 11 /* "," */) {
				Get();
				Expect(_string);
				std::string name=Destring(t->val);
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
		if (la->kind == 16 /* "MAG" */) {
			Get();
			if (StartOf(2)) {
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
			Expect(17 /* "-" */);
			if (StartOf(2)) {
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
		Expect(18 /* "]" */);
}

void Parser::STYLEDEF(StyleFilter filter) {
		if (la->kind == 19 /* "NODE" */) {
			NODESTYLEDEF(filter);
		} else if (la->kind == 25 /* "WAY" */) {
			WAYSTYLEDEF(filter);
		} else if (la->kind == 26 /* "AREA" */) {
			AREASTYLEDEF(filter);
		} else SynErr(79);
}

void Parser::MAG(Mag& mag) {
		switch (la->kind) {
		case 60 /* "world" */: {
			Get();
			mag=magWorld; 
			break;
		}
		case 61 /* "continent" */: {
			Get();
			mag=magContinent; 
			break;
		}
		case 62 /* "state" */: {
			Get();
			mag=magState; 
			break;
		}
		case 63 /* "stateOver" */: {
			Get();
			mag=magStateOver; 
			break;
		}
		case 64 /* "county" */: {
			Get();
			mag=magCounty; 
			break;
		}
		case 65 /* "region" */: {
			Get();
			mag=magRegion; 
			break;
		}
		case 66 /* "proximity" */: {
			Get();
			mag=magProximity; 
			break;
		}
		case 67 /* "cityOver" */: {
			Get();
			mag=magCityOver; 
			break;
		}
		case 68 /* "city" */: {
			Get();
			mag=magCity; 
			break;
		}
		case 69 /* "suburb" */: {
			Get();
			mag=magSuburb; 
			break;
		}
		case 70 /* "detail" */: {
			Get();
			mag=magDetail; 
			break;
		}
		case 71 /* "close" */: {
			Get();
			mag=magClose; 
			break;
		}
		case 72 /* "veryClose" */: {
			Get();
			mag=magVeryClose; 
			break;
		}
		case 73 /* "block" */: {
			Get();
			mag=magBlock; 
			break;
		}
		default: SynErr(80); break;
		}
}

void Parser::NODESTYLEDEF(StyleFilter filter) {
		while (!(la->kind == _EOF || la->kind == 19 /* "NODE" */)) {SynErr(81); Get();}
		Expect(19 /* "NODE" */);
		Expect(20 /* "." */);
		if (la->kind == 21 /* "LABE" */) {
			NODELABELSTYLE(filter);
		} else if (la->kind == 22 /* "REF" */) {
			NODEREFSTYLE(filter);
		} else if (la->kind == 23 /* "SYMBO" */) {
			NODESYMBOLSTYLE(filter);
		} else if (la->kind == 24 /* "ICON" */) {
			NODEICONSTYLE(filter);
		} else SynErr(82);
}

void Parser::WAYSTYLEDEF(StyleFilter filter) {
		while (!(la->kind == _EOF || la->kind == 25 /* "WAY" */)) {SynErr(83); Get();}
		Expect(25 /* "WAY" */);
		if (la->kind == 12 /* "{" */ || la->kind == 14 /* "[" */) {
			WAYSTYLE(filter);
		} else if (la->kind == 20 /* "." */) {
			Get();
			if (la->kind == 21 /* "LABE" */) {
				WAYLABELSTYLE(filter);
			} else if (la->kind == 22 /* "REF" */) {
				WAYREFSTYLE(filter);
			} else SynErr(84);
		} else SynErr(85);
}

void Parser::AREASTYLEDEF(StyleFilter filter) {
		while (!(la->kind == _EOF || la->kind == 26 /* "AREA" */)) {SynErr(86); Get();}
		Expect(26 /* "AREA" */);
		if (la->kind == 12 /* "{" */ || la->kind == 14 /* "[" */) {
			AREASTYLE(filter);
		} else if (la->kind == 20 /* "." */) {
			Get();
			if (la->kind == 21 /* "LABE" */) {
				AREALABELSTYLE(filter);
			} else if (la->kind == 23 /* "SYMBO" */) {
				AREASYMBOLSTYLE(filter);
			} else if (la->kind == 24 /* "ICON" */) {
				AREAICONSTYLE(filter);
			} else SynErr(87);
		} else SynErr(88);
}

void Parser::NODELABELSTYLE(StyleFilter filter) {
		while (!(la->kind == _EOF || la->kind == 21 /* "LABE" */)) {SynErr(89); Get();}
		Expect(21 /* "LABE" */);
		if (la->kind == 14 /* "[" */) {
			STYLEFILTER(filter);
		}
		LabelStyleList labelStyles;
		
		config.GetNodeNameLabelStyles(filter,labelStyles);
		
		while (!(la->kind == _EOF || la->kind == 12 /* "{" */)) {SynErr(90); Get();}
		Expect(12 /* "{" */);
		while (StartOf(3)) {
			LABELDEF(labelStyles);
		}
		while (!(la->kind == _EOF || la->kind == 13 /* "}" */)) {SynErr(91); Get();}
		Expect(13 /* "}" */);
}

void Parser::NODEREFSTYLE(StyleFilter filter) {
		while (!(la->kind == _EOF || la->kind == 22 /* "REF" */)) {SynErr(92); Get();}
		Expect(22 /* "REF" */);
		if (la->kind == 14 /* "[" */) {
			STYLEFILTER(filter);
		}
		LabelStyleList labelStyles;
		
		config.GetNodeRefLabelStyles(filter,labelStyles);
		
		while (!(la->kind == _EOF || la->kind == 12 /* "{" */)) {SynErr(93); Get();}
		Expect(12 /* "{" */);
		while (StartOf(3)) {
			REFDEF(labelStyles);
		}
		while (!(la->kind == _EOF || la->kind == 13 /* "}" */)) {SynErr(94); Get();}
		Expect(13 /* "}" */);
}

void Parser::NODESYMBOLSTYLE(StyleFilter filter) {
		while (!(la->kind == _EOF || la->kind == 23 /* "SYMBO" */)) {SynErr(95); Get();}
		Expect(23 /* "SYMBO" */);
		if (la->kind == 14 /* "[" */) {
			STYLEFILTER(filter);
		}
		SymbolStyleList symbolStyles;
		
		config.GetNodeSymbolStyles(filter,symbolStyles);
		
		while (!(la->kind == _EOF || la->kind == 12 /* "{" */)) {SynErr(96); Get();}
		Expect(12 /* "{" */);
		while (la->kind == 27 /* "color" */ || la->kind == 43 /* "style" */ || la->kind == 45 /* "size" */) {
			SYMBOLDEF(symbolStyles);
		}
		while (!(la->kind == _EOF || la->kind == 13 /* "}" */)) {SynErr(97); Get();}
		Expect(13 /* "}" */);
}

void Parser::NODEICONSTYLE(StyleFilter filter) {
		while (!(la->kind == _EOF || la->kind == 24 /* "ICON" */)) {SynErr(98); Get();}
		Expect(24 /* "ICON" */);
		if (la->kind == 14 /* "[" */) {
			STYLEFILTER(filter);
		}
		IconStyleList iconStyles;
		
		config.GetNodeIconStyles(filter,iconStyles);
		
		while (!(la->kind == _EOF || la->kind == 12 /* "{" */)) {SynErr(99); Get();}
		Expect(12 /* "{" */);
		while (la->kind == 48 /* "name" */) {
			ICONDEF(iconStyles);
		}
		while (!(la->kind == _EOF || la->kind == 13 /* "}" */)) {SynErr(100); Get();}
		Expect(13 /* "}" */);
}

void Parser::LABELDEF(LabelStyleList& styles) {
		switch (la->kind) {
		case 43 /* "style" */: {
			LabelStyle::Style labelStyle; 
			Get();
			Expect(28 /* ":" */);
			LABELSTYLE(labelStyle);
			ExpectWeak(29 /* ";" */, 4);
			for (LabelStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 LabelStyleRef style(*s);
			
			style->SetStyle(labelStyle);
			}
			
			break;
		}
		case 27 /* "color" */: {
			Color textColor; 
			Get();
			Expect(28 /* ":" */);
			COLOR(textColor);
			ExpectWeak(29 /* ";" */, 4);
			for (LabelStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 LabelStyleRef style(*s);
			
			style->SetTextColor(textColor);
			}
			
			break;
		}
		case 44 /* "backgroundColor" */: {
			Color bgColor; 
			Get();
			Expect(28 /* ":" */);
			COLOR(bgColor);
			ExpectWeak(29 /* ";" */, 4);
			for (LabelStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 LabelStyleRef style(*s);
			
			style->SetBgColor(bgColor);
			}
			
			break;
		}
		case 40 /* "borderColor" */: {
			Color borderColor; 
			Get();
			Expect(28 /* ":" */);
			COLOR(borderColor);
			ExpectWeak(29 /* ";" */, 4);
			for (LabelStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 LabelStyleRef style(*s);
			
			style->SetBorderColor(borderColor);
			}
			
			break;
		}
		case 45 /* "size" */: {
			double size; 
			Get();
			Expect(28 /* ":" */);
			DOUBLE(size);
			ExpectWeak(29 /* ";" */, 4);
			for (LabelStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 LabelStyleRef style(*s);
			
			style->SetSize(size);
			}
			
			break;
		}
		case 46 /* "scaleMag" */: {
			Mag scaleMag; 
			Get();
			Expect(28 /* ":" */);
			MAG(scaleMag);
			ExpectWeak(29 /* ";" */, 4);
			for (LabelStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 LabelStyleRef style(*s);
			
			style->SetScaleAndFadeMag(scaleMag);
			}
			
			break;
		}
		case 47 /* "priority" */: {
			size_t priority; 
			Get();
			Expect(28 /* ":" */);
			INTEGER(priority);
			ExpectWeak(29 /* ";" */, 4);
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
		default: SynErr(101); break;
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
		case 43 /* "style" */: {
			LabelStyle::Style labelStyle; 
			Get();
			Expect(28 /* ":" */);
			LABELSTYLE(labelStyle);
			ExpectWeak(29 /* ";" */, 4);
			for (LabelStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 LabelStyleRef style(*s);
			
			style->SetStyle(labelStyle);
			}
			
			break;
		}
		case 27 /* "color" */: {
			Color textColor; 
			Get();
			Expect(28 /* ":" */);
			COLOR(textColor);
			ExpectWeak(29 /* ";" */, 4);
			for (LabelStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 LabelStyleRef style(*s);
			
			style->SetTextColor(textColor);
			}
			
			break;
		}
		case 44 /* "backgroundColor" */: {
			Color bgColor; 
			Get();
			Expect(28 /* ":" */);
			COLOR(bgColor);
			ExpectWeak(29 /* ";" */, 4);
			for (LabelStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 LabelStyleRef style(*s);
			
			style->SetBgColor(bgColor);
			}
			
			break;
		}
		case 40 /* "borderColor" */: {
			Color borderColor; 
			Get();
			Expect(28 /* ":" */);
			COLOR(borderColor);
			ExpectWeak(29 /* ";" */, 4);
			for (LabelStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 LabelStyleRef style(*s);
			
			style->SetBorderColor(borderColor);
			}
			
			break;
		}
		case 45 /* "size" */: {
			double size; 
			Get();
			Expect(28 /* ":" */);
			DOUBLE(size);
			ExpectWeak(29 /* ";" */, 4);
			for (LabelStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 LabelStyleRef style(*s);
			
			style->SetSize(size);
			}
			
			break;
		}
		case 46 /* "scaleMag" */: {
			Mag scaleMag; 
			Get();
			Expect(28 /* ":" */);
			MAG(scaleMag);
			ExpectWeak(29 /* ";" */, 4);
			for (LabelStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 LabelStyleRef style(*s);
			
			style->SetScaleAndFadeMag(scaleMag);
			}
			
			break;
		}
		case 47 /* "priority" */: {
			size_t priority; 
			Get();
			Expect(28 /* ":" */);
			INTEGER(priority);
			ExpectWeak(29 /* ";" */, 4);
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
		default: SynErr(102); break;
		}
}

void Parser::SYMBOLDEF(SymbolStyleList& styles) {
		if (la->kind == 43 /* "style" */) {
			SymbolStyle::Style symbolStyle; 
			Get();
			Expect(28 /* ":" */);
			SYMBOLSTYLE(symbolStyle);
			ExpectWeak(29 /* ";" */, 5);
			for (SymbolStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 SymbolStyleRef style(*s);
			
			style->SetStyle(symbolStyle);
			}
			
		} else if (la->kind == 27 /* "color" */) {
			Color fillColor; 
			Get();
			Expect(28 /* ":" */);
			COLOR(fillColor);
			ExpectWeak(29 /* ";" */, 5);
			for (SymbolStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 SymbolStyleRef style(*s);
			
			style->SetFillColor(fillColor);
			}
			
		} else if (la->kind == 45 /* "size" */) {
			double size; 
			Get();
			Expect(28 /* ":" */);
			DOUBLE(size);
			ExpectWeak(29 /* ";" */, 5);
			for (SymbolStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 SymbolStyleRef style(*s);
			
			style->SetSize(size);
			}
			
		} else SynErr(103);
}

void Parser::ICONDEF(IconStyleList& styles) {
		std::string name; 
		Expect(48 /* "name" */);
		Expect(28 /* ":" */);
		Expect(_ident);
		name=Destring(t->val); 
		ExpectWeak(29 /* ";" */, 6);
		for (IconStyleList::iterator s=styles.begin();
		    s!=styles.end();
		    ++s) {
		 IconStyleRef style(*s);
		
		style->SetIconName(name);
		}
		
}

void Parser::WAYSTYLE(StyleFilter filter) {
		if (la->kind == 14 /* "[" */) {
			STYLEFILTER(filter);
		}
		LineStyleList lineStyles;
		
		config.GetWayLineStyles(filter,lineStyles);
		
		while (!(la->kind == _EOF || la->kind == 12 /* "{" */)) {SynErr(104); Get();}
		Expect(12 /* "{" */);
		while (StartOf(7)) {
			LINEDEF(lineStyles);
		}
		while (!(la->kind == _EOF || la->kind == 13 /* "}" */)) {SynErr(105); Get();}
		Expect(13 /* "}" */);
}

void Parser::WAYLABELSTYLE(StyleFilter filter) {
		while (!(la->kind == _EOF || la->kind == 21 /* "LABE" */)) {SynErr(106); Get();}
		Expect(21 /* "LABE" */);
		if (la->kind == 14 /* "[" */) {
			STYLEFILTER(filter);
		}
		LabelStyleList labelStyles;
		
		config.GetWayNameLabelStyles(filter,labelStyles);
		
		Expect(12 /* "{" */);
		while (StartOf(3)) {
			LABELDEF(labelStyles);
		}
		Expect(13 /* "}" */);
}

void Parser::WAYREFSTYLE(StyleFilter filter) {
		while (!(la->kind == _EOF || la->kind == 22 /* "REF" */)) {SynErr(107); Get();}
		Expect(22 /* "REF" */);
		if (la->kind == 14 /* "[" */) {
			STYLEFILTER(filter);
		}
		LabelStyleList labelStyles;
		
		config.GetWayRefLabelStyles(filter,labelStyles);
		
		while (!(la->kind == _EOF || la->kind == 12 /* "{" */)) {SynErr(108); Get();}
		Expect(12 /* "{" */);
		while (StartOf(3)) {
			LABELDEF(labelStyles);
		}
		while (!(la->kind == _EOF || la->kind == 13 /* "}" */)) {SynErr(109); Get();}
		Expect(13 /* "}" */);
}

void Parser::LINEDEF(LineStyleList& styles) {
		switch (la->kind) {
		case 27 /* "color" */: {
			Color lineColor; 
			Get();
			Expect(28 /* ":" */);
			COLOR(lineColor);
			ExpectWeak(29 /* ";" */, 8);
			for (LineStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 LineStyleRef style(*s);
			
			style->SetLineColor(lineColor);
			}
			
			break;
		}
		case 30 /* "altColor" */: {
			Color alternateColor; 
			Get();
			Expect(28 /* ":" */);
			COLOR(alternateColor);
			ExpectWeak(29 /* ";" */, 8);
			for (LineStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 LineStyleRef style(*s);
			
			style->SetAlternateColor(alternateColor);
			}
			
			break;
		}
		case 31 /* "outlineColor" */: {
			Color outlineColor;
			Get();
			Expect(28 /* ":" */);
			COLOR(outlineColor);
			ExpectWeak(29 /* ";" */, 8);
			for (LineStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 LineStyleRef style(*s);
			
			style->SetOutlineColor(outlineColor);
			}
			
			break;
		}
		case 32 /* "dash" */: {
			std::vector<double> dashes;
			double              dash;
			
			Get();
			Expect(28 /* ":" */);
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
			
			ExpectWeak(29 /* ";" */, 8);
			break;
		}
		case 33 /* "gapColor" */: {
			Color gapColor; 
			Get();
			Expect(28 /* ":" */);
			COLOR(gapColor);
			ExpectWeak(29 /* ";" */, 8);
			for (LineStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 LineStyleRef style(*s);
			
			style->SetGapColor(gapColor);
			}
			
			break;
		}
		case 34 /* "displayWidth" */: {
			double displayWidth; 
			Get();
			Expect(28 /* ":" */);
			DISPLAYSIZE(displayWidth);
			ExpectWeak(29 /* ";" */, 8);
			for (LineStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 LineStyleRef style(*s);
			
			style->SetDisplayWidth(displayWidth);
			}
			
			break;
		}
		case 35 /* "width" */: {
			double width; 
			Get();
			Expect(28 /* ":" */);
			MAPSIZE(width);
			ExpectWeak(29 /* ";" */, 8);
			for (LineStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 LineStyleRef style(*s);
			
			style->SetWidth(width);
			}
			
			break;
		}
		case 36 /* "cap" */: {
			LineStyle::CapStyle capStyle; 
			Get();
			Expect(28 /* ":" */);
			CAPSTYLE(capStyle);
			ExpectWeak(29 /* ";" */, 8);
			for (LineStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 LineStyleRef style(*s);
			
			style->SetCapStyle(capStyle);
			}
			
			break;
		}
		case 37 /* "outline" */: {
			double outline; 
			Get();
			Expect(28 /* ":" */);
			DISPLAYSIZE(outline);
			ExpectWeak(29 /* ";" */, 8);
			for (LineStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 LineStyleRef style(*s);
			
			style->SetOutline(outline);
			}
			
			break;
		}
		default: SynErr(110); break;
		}
}

void Parser::AREASTYLE(StyleFilter filter) {
		if (la->kind == 14 /* "[" */) {
			STYLEFILTER(filter);
		}
		FillStyleList fillStyles;
		
		config.GetAreaFillStyles(filter,fillStyles);
		
		while (!(la->kind == _EOF || la->kind == 12 /* "{" */)) {SynErr(111); Get();}
		Expect(12 /* "{" */);
		while (StartOf(9)) {
			FILLDEF(fillStyles);
		}
		while (!(la->kind == _EOF || la->kind == 13 /* "}" */)) {SynErr(112); Get();}
		Expect(13 /* "}" */);
}

void Parser::AREALABELSTYLE(StyleFilter filter) {
		while (!(la->kind == _EOF || la->kind == 21 /* "LABE" */)) {SynErr(113); Get();}
		Expect(21 /* "LABE" */);
		if (la->kind == 14 /* "[" */) {
			STYLEFILTER(filter);
		}
		LabelStyleList labelStyles;
		
		config.GetAreaLabelStyles(filter,labelStyles);
		
		while (!(la->kind == _EOF || la->kind == 12 /* "{" */)) {SynErr(114); Get();}
		Expect(12 /* "{" */);
		while (StartOf(3)) {
			LABELDEF(labelStyles);
		}
		while (!(la->kind == _EOF || la->kind == 13 /* "}" */)) {SynErr(115); Get();}
		Expect(13 /* "}" */);
}

void Parser::AREASYMBOLSTYLE(StyleFilter filter) {
		while (!(la->kind == _EOF || la->kind == 23 /* "SYMBO" */)) {SynErr(116); Get();}
		Expect(23 /* "SYMBO" */);
		if (la->kind == 14 /* "[" */) {
			STYLEFILTER(filter);
		}
		SymbolStyleList symbolStyles;
		
		config.GetAreaSymbolStyles(filter,symbolStyles);
		
		while (!(la->kind == _EOF || la->kind == 12 /* "{" */)) {SynErr(117); Get();}
		Expect(12 /* "{" */);
		while (la->kind == 27 /* "color" */ || la->kind == 43 /* "style" */ || la->kind == 45 /* "size" */) {
			SYMBOLDEF(symbolStyles);
		}
		while (!(la->kind == _EOF || la->kind == 13 /* "}" */)) {SynErr(118); Get();}
		Expect(13 /* "}" */);
}

void Parser::AREAICONSTYLE(StyleFilter filter) {
		while (!(la->kind == _EOF || la->kind == 24 /* "ICON" */)) {SynErr(119); Get();}
		Expect(24 /* "ICON" */);
		if (la->kind == 14 /* "[" */) {
			STYLEFILTER(filter);
		}
		IconStyleList iconStyles;
		
		config.GetAreaIconStyles(filter,iconStyles);
		
		while (!(la->kind == _EOF || la->kind == 12 /* "{" */)) {SynErr(120); Get();}
		Expect(12 /* "{" */);
		while (la->kind == 48 /* "name" */) {
			ICONDEF(iconStyles);
		}
		while (!(la->kind == _EOF || la->kind == 13 /* "}" */)) {SynErr(121); Get();}
		Expect(13 /* "}" */);
}

void Parser::FILLDEF(FillStyleList& styles) {
		switch (la->kind) {
		case 27 /* "color" */: {
			Color fillColor; 
			Get();
			Expect(28 /* ":" */);
			COLOR(fillColor);
			ExpectWeak(29 /* ";" */, 10);
			for (FillStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 FillStyleRef style(*s);
			
			style->SetFillColor(fillColor);
			}
			
			break;
		}
		case 38 /* "pattern" */: {
			std::string patternName; 
			Get();
			Expect(28 /* ":" */);
			Expect(_string);
			patternName=Destring(t->val); 
			ExpectWeak(29 /* ";" */, 10);
			for (FillStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 FillStyleRef style(*s);
			
			style->SetPattern(patternName);
			}
			
			break;
		}
		case 39 /* "patternMinMag" */: {
			Mag minMag; 
			Get();
			Expect(28 /* ":" */);
			MAG(minMag);
			ExpectWeak(29 /* ";" */, 10);
			for (FillStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 FillStyleRef style(*s);
			
			style->SetPatternMinMag(minMag);
			}
			
			break;
		}
		case 40 /* "borderColor" */: {
			Color borderColor; 
			Get();
			Expect(28 /* ":" */);
			COLOR(borderColor);
			ExpectWeak(29 /* ";" */, 10);
			for (FillStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 FillStyleRef style(*s);
			
			style->SetBorderColor(borderColor);
			}
			
			break;
		}
		case 41 /* "borderWidth" */: {
			double width; 
			Get();
			Expect(28 /* ":" */);
			DISPLAYSIZE(width);
			ExpectWeak(29 /* ";" */, 10);
			for (FillStyleList::iterator s=styles.begin();
			    s!=styles.end();
			    ++s) {
			 FillStyleRef style(*s);
			
			style->SetBorderWidth(width);
			}
			
			break;
		}
		case 42 /* "borderDash" */: {
			std::vector<double> dashes;
			double              dash;
			
			Get();
			Expect(28 /* ":" */);
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
			
			ExpectWeak(29 /* ";" */, 10);
			break;
		}
		default: SynErr(122); break;
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
			
		} else SynErr(123);
}

void Parser::DISPLAYSIZE(double& value) {
		DOUBLE(value);
		Expect(74 /* "mm" */);
}

void Parser::MAPSIZE(double& value) {
		DOUBLE(value);
		Expect(75 /* "m" */);
}

void Parser::CAPSTYLE(LineStyle::CapStyle& style) {
		if (la->kind == 49 /* "butt" */) {
			Get();
			style=LineStyle::capButt; 
		} else if (la->kind == 50 /* "round" */) {
			Get();
			style=LineStyle::capRound; 
		} else if (la->kind == 51 /* "square" */) {
			Get();
			style=LineStyle::capSquare; 
		} else SynErr(124);
}

void Parser::LABELSTYLE(LabelStyle::Style& style) {
		if (la->kind == 52 /* "normal" */) {
			Get();
			style=LabelStyle::normal; 
		} else if (la->kind == 53 /* "contour" */) {
			Get();
			style=LabelStyle::contour; 
		} else if (la->kind == 54 /* "plate" */) {
			Get();
			style=LabelStyle::plate; 
		} else if (la->kind == 55 /* "emphasize" */) {
			Get();
			style=LabelStyle::emphasize; 
		} else SynErr(125);
}

void Parser::INTEGER(size_t& value) {
		Expect(_number);
		if (!StringToNumber(t->val,value)) {
		 std::string e="Cannot parse number '"+std::string(t->val)+"'";
		
		 SemErr(e.c_str());
		}
		
}

void Parser::SYMBOLSTYLE(SymbolStyle::Style& style) {
		if (la->kind == 56 /* "none" */) {
			Get();
			style=SymbolStyle::none; 
		} else if (la->kind == 57 /* "box" */) {
			Get();
			style=SymbolStyle::box; 
		} else if (la->kind == 58 /* "triangle" */) {
			Get();
			style=SymbolStyle::triangle; 
		} else if (la->kind == 59 /* "circle" */) {
			Get();
			style=SymbolStyle::circle; 
		} else SynErr(126);
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
	maxT = 76;

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

	static bool set[11][78] = {
		{T,x,x,x, x,x,T,x, x,x,x,x, T,T,x,x, x,x,x,T, x,T,T,T, T,T,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x},
		{x,x,x,x, x,x,x,x, x,x,x,x, T,x,T,x, x,x,x,T, x,x,x,x, x,T,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, T,T,T,T, T,T,T,T, T,T,T,T, T,T,x,x, x,x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,T, x,x,x,x, x,x,x,x, x,x,x,x, T,x,x,T, T,T,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x},
		{T,x,x,x, x,x,T,x, x,x,x,x, T,T,x,x, x,x,x,T, x,T,T,T, T,T,T,T, x,x,x,x, x,x,x,x, x,x,x,x, T,x,x,T, T,T,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x},
		{T,x,x,x, x,x,T,x, x,x,x,x, T,T,x,x, x,x,x,T, x,T,T,T, T,T,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,T, x,T,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x},
		{T,x,x,x, x,x,T,x, x,x,x,x, T,T,x,x, x,x,x,T, x,T,T,T, T,T,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,T, x,x,T,T, T,T,T,T, T,T,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x},
		{T,x,x,x, x,x,T,x, x,x,x,x, T,T,x,x, x,x,x,T, x,T,T,T, T,T,T,T, x,x,T,T, T,T,T,T, T,T,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,T, x,x,x,x, x,x,x,x, x,x,T,T, T,T,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x},
		{T,x,x,x, x,x,T,x, x,x,x,x, T,T,x,x, x,x,x,T, x,T,T,T, T,T,T,T, x,x,x,x, x,x,x,x, x,x,T,T, T,T,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x}
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
			case 12: s = coco_string_create("\"{\" expected"); break;
			case 13: s = coco_string_create("\"}\" expected"); break;
			case 14: s = coco_string_create("\"[\" expected"); break;
			case 15: s = coco_string_create("\"TYPE\" expected"); break;
			case 16: s = coco_string_create("\"MAG\" expected"); break;
			case 17: s = coco_string_create("\"-\" expected"); break;
			case 18: s = coco_string_create("\"]\" expected"); break;
			case 19: s = coco_string_create("\"NODE\" expected"); break;
			case 20: s = coco_string_create("\".\" expected"); break;
			case 21: s = coco_string_create("\"LABEL\" expected"); break;
			case 22: s = coco_string_create("\"REF\" expected"); break;
			case 23: s = coco_string_create("\"SYMBOL\" expected"); break;
			case 24: s = coco_string_create("\"ICON\" expected"); break;
			case 25: s = coco_string_create("\"WAY\" expected"); break;
			case 26: s = coco_string_create("\"AREA\" expected"); break;
			case 27: s = coco_string_create("\"color\" expected"); break;
			case 28: s = coco_string_create("\":\" expected"); break;
			case 29: s = coco_string_create("\";\" expected"); break;
			case 30: s = coco_string_create("\"altColor\" expected"); break;
			case 31: s = coco_string_create("\"outlineColor\" expected"); break;
			case 32: s = coco_string_create("\"dash\" expected"); break;
			case 33: s = coco_string_create("\"gapColor\" expected"); break;
			case 34: s = coco_string_create("\"displayWidth\" expected"); break;
			case 35: s = coco_string_create("\"width\" expected"); break;
			case 36: s = coco_string_create("\"cap\" expected"); break;
			case 37: s = coco_string_create("\"outline\" expected"); break;
			case 38: s = coco_string_create("\"pattern\" expected"); break;
			case 39: s = coco_string_create("\"patternMinMag\" expected"); break;
			case 40: s = coco_string_create("\"borderColor\" expected"); break;
			case 41: s = coco_string_create("\"borderWidth\" expected"); break;
			case 42: s = coco_string_create("\"borderDash\" expected"); break;
			case 43: s = coco_string_create("\"style\" expected"); break;
			case 44: s = coco_string_create("\"backgroundColor\" expected"); break;
			case 45: s = coco_string_create("\"size\" expected"); break;
			case 46: s = coco_string_create("\"scaleMag\" expected"); break;
			case 47: s = coco_string_create("\"priority\" expected"); break;
			case 48: s = coco_string_create("\"name\" expected"); break;
			case 49: s = coco_string_create("\"butt\" expected"); break;
			case 50: s = coco_string_create("\"round\" expected"); break;
			case 51: s = coco_string_create("\"square\" expected"); break;
			case 52: s = coco_string_create("\"normal\" expected"); break;
			case 53: s = coco_string_create("\"contour\" expected"); break;
			case 54: s = coco_string_create("\"plate\" expected"); break;
			case 55: s = coco_string_create("\"emphasize\" expected"); break;
			case 56: s = coco_string_create("\"none\" expected"); break;
			case 57: s = coco_string_create("\"box\" expected"); break;
			case 58: s = coco_string_create("\"triangle\" expected"); break;
			case 59: s = coco_string_create("\"circle\" expected"); break;
			case 60: s = coco_string_create("\"world\" expected"); break;
			case 61: s = coco_string_create("\"continent\" expected"); break;
			case 62: s = coco_string_create("\"state\" expected"); break;
			case 63: s = coco_string_create("\"stateOver\" expected"); break;
			case 64: s = coco_string_create("\"county\" expected"); break;
			case 65: s = coco_string_create("\"region\" expected"); break;
			case 66: s = coco_string_create("\"proximity\" expected"); break;
			case 67: s = coco_string_create("\"cityOver\" expected"); break;
			case 68: s = coco_string_create("\"city\" expected"); break;
			case 69: s = coco_string_create("\"suburb\" expected"); break;
			case 70: s = coco_string_create("\"detail\" expected"); break;
			case 71: s = coco_string_create("\"close\" expected"); break;
			case 72: s = coco_string_create("\"veryClose\" expected"); break;
			case 73: s = coco_string_create("\"block\" expected"); break;
			case 74: s = coco_string_create("\"mm\" expected"); break;
			case 75: s = coco_string_create("\"m\" expected"); break;
			case 76: s = coco_string_create("??? expected"); break;
			case 77: s = coco_string_create("this symbol not expected in OSS"); break;
			case 78: s = coco_string_create("invalid STYLE"); break;
			case 79: s = coco_string_create("invalid STYLEDEF"); break;
			case 80: s = coco_string_create("invalid MAG"); break;
			case 81: s = coco_string_create("this symbol not expected in NODESTYLEDEF"); break;
			case 82: s = coco_string_create("invalid NODESTYLEDEF"); break;
			case 83: s = coco_string_create("this symbol not expected in WAYSTYLEDEF"); break;
			case 84: s = coco_string_create("invalid WAYSTYLEDEF"); break;
			case 85: s = coco_string_create("invalid WAYSTYLEDEF"); break;
			case 86: s = coco_string_create("this symbol not expected in AREASTYLEDEF"); break;
			case 87: s = coco_string_create("invalid AREASTYLEDEF"); break;
			case 88: s = coco_string_create("invalid AREASTYLEDEF"); break;
			case 89: s = coco_string_create("this symbol not expected in NODELABELSTYLE"); break;
			case 90: s = coco_string_create("this symbol not expected in NODELABELSTYLE"); break;
			case 91: s = coco_string_create("this symbol not expected in NODELABELSTYLE"); break;
			case 92: s = coco_string_create("this symbol not expected in NODEREFSTYLE"); break;
			case 93: s = coco_string_create("this symbol not expected in NODEREFSTYLE"); break;
			case 94: s = coco_string_create("this symbol not expected in NODEREFSTYLE"); break;
			case 95: s = coco_string_create("this symbol not expected in NODESYMBOLSTYLE"); break;
			case 96: s = coco_string_create("this symbol not expected in NODESYMBOLSTYLE"); break;
			case 97: s = coco_string_create("this symbol not expected in NODESYMBOLSTYLE"); break;
			case 98: s = coco_string_create("this symbol not expected in NODEICONSTYLE"); break;
			case 99: s = coco_string_create("this symbol not expected in NODEICONSTYLE"); break;
			case 100: s = coco_string_create("this symbol not expected in NODEICONSTYLE"); break;
			case 101: s = coco_string_create("invalid LABELDEF"); break;
			case 102: s = coco_string_create("invalid REFDEF"); break;
			case 103: s = coco_string_create("invalid SYMBOLDEF"); break;
			case 104: s = coco_string_create("this symbol not expected in WAYSTYLE"); break;
			case 105: s = coco_string_create("this symbol not expected in WAYSTYLE"); break;
			case 106: s = coco_string_create("this symbol not expected in WAYLABELSTYLE"); break;
			case 107: s = coco_string_create("this symbol not expected in WAYREFSTYLE"); break;
			case 108: s = coco_string_create("this symbol not expected in WAYREFSTYLE"); break;
			case 109: s = coco_string_create("this symbol not expected in WAYREFSTYLE"); break;
			case 110: s = coco_string_create("invalid LINEDEF"); break;
			case 111: s = coco_string_create("this symbol not expected in AREASTYLE"); break;
			case 112: s = coco_string_create("this symbol not expected in AREASTYLE"); break;
			case 113: s = coco_string_create("this symbol not expected in AREALABELSTYLE"); break;
			case 114: s = coco_string_create("this symbol not expected in AREALABELSTYLE"); break;
			case 115: s = coco_string_create("this symbol not expected in AREALABELSTYLE"); break;
			case 116: s = coco_string_create("this symbol not expected in AREASYMBOLSTYLE"); break;
			case 117: s = coco_string_create("this symbol not expected in AREASYMBOLSTYLE"); break;
			case 118: s = coco_string_create("this symbol not expected in AREASYMBOLSTYLE"); break;
			case 119: s = coco_string_create("this symbol not expected in AREAICONSTYLE"); break;
			case 120: s = coco_string_create("this symbol not expected in AREAICONSTYLE"); break;
			case 121: s = coco_string_create("this symbol not expected in AREAICONSTYLE"); break;
			case 122: s = coco_string_create("invalid FILLDEF"); break;
			case 123: s = coco_string_create("invalid DOUBLE"); break;
			case 124: s = coco_string_create("invalid CAPSTYLE"); break;
			case 125: s = coco_string_create("invalid LABELSTYLE"); break;
			case 126: s = coco_string_create("invalid SYMBOLSTYLE"); break;

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

