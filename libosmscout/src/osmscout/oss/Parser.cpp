
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

#include <cassert>

#include <osmscout/oss/Scanner.h>
#include <osmscout/oss/Parser.h>

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
		while (!(la->kind == 0 || la->kind == 6)) {SynErr(62); Get();}
		Expect(6);
		while (la->kind == 8 || la->kind == 9 || la->kind == 11) {
			STYLE();
		}
		Expect(7);
}

void Parser::STYLE() {
		if (la->kind == 8) {
			NODESTYLE();
		} else if (la->kind == 9) {
			WAYSTYLE();
		} else if (la->kind == 11) {
			AREASTYLE();
		} else SynErr(63);
}

void Parser::NODESTYLE() {
		TypeId      type=typeIgnore;
		std::string name;
		
		while (!(la->kind == 0 || la->kind == 8)) {SynErr(64); Get();}
		Expect(8);
		Expect(5);
		name=Destring(t->val); 
		type=config.GetTypeConfig()->GetNodeTypeId(name);
		
		                  if (type==typeIgnore) {
		                    std::string e="Unknown type '"+name+"'";
		
		                    SemErr(e.c_str());
		                  }
		
		                
		while (StartOf(1)) {
			if (la->kind == 20) {
				LabelStyle labelStyle; 
				LABELDEF(labelStyle);
				config.SetNodeLabelStyle(type,labelStyle); 
			} else if (la->kind == 24) {
				LabelStyle refStyle; 
				REFDEF(refStyle);
				config.SetNodeRefLabelStyle(type,refStyle); 
			} else if (la->kind == 25) {
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
		size_t      prio;
		
		while (!(la->kind == 0 || la->kind == 9)) {SynErr(65); Get();}
		Expect(9);
		Expect(5);
		name=Destring(t->val); 
		Expect(10);
		Expect(2);
		type=config.GetTypeConfig()->GetWayTypeId(name);
		
		                  if (type==typeIgnore) {
		                    std::string e="Unknown type '"+name+"'";
		
		                    SemErr(e.c_str());
		                  }
		                  else if (!StringToNumber(t->val,prio)) {
		                    std::string e="Cannot parse priority '"+std::string(t->val)+"'";
		
		                    SemErr(e.c_str());
		                  }
		                  else {
		                    config.SetWayPrio(type,prio);
		                  }
		                
		while (la->kind == 12 || la->kind == 20 || la->kind == 24) {
			if (la->kind == 12) {
				LineStyle lineStyle; 
				LINEDEF(lineStyle);
				config.SetWayLineStyle(type,lineStyle); 
			} else if (la->kind == 20) {
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
		
		while (!(la->kind == 0 || la->kind == 11)) {SynErr(66); Get();}
		Expect(11);
		Expect(5);
		name=Destring(t->val); 
		type=config.GetTypeConfig()->GetAreaTypeId(name);
		
		                  if (type==typeIgnore) {
		                    std::string e="Unknown type '"+name+"'";
		
		                    SemErr(e.c_str());
		                  }
		
		                
		while (StartOf(2)) {
			switch (la->kind) {
			case 17: {
				FillStyle   fillStyle;
				std::string filter;
				
				FILLDEF(fillStyle,filter);
				if (filter=="building") {
				 config.SetAreaBuildingFillStyle(type,fillStyle);
				}
				else {
				  config.SetAreaFillStyle(type,fillStyle);
				}
				
				break;
			}
			case 18: {
				PatternStyle patternStyle; 
				PATTERNDEF(patternStyle);
				config.SetAreaPatternStyle(type,patternStyle); 
				break;
			}
			case 19: {
				LineStyle lineStyle; 
				BORDERDEF(lineStyle);
				config.SetAreaBorderStyle(type,lineStyle); 
				break;
			}
			case 20: {
				LabelStyle labelStyle; 
				LABELDEF(labelStyle);
				config.SetAreaLabelStyle(type,labelStyle); 
				break;
			}
			case 25: {
				SymbolStyle symbolStyle; 
				SYMBOLDEF(symbolStyle);
				config.SetAreaSymbolStyle(type,symbolStyle); 
				break;
			}
			case 26: {
				IconStyle iconStyle; 
				ICONDEF(iconStyle);
				config.SetAreaIconStyle(type,iconStyle); 
				break;
			}
			}
		}
}

void Parser::LABELDEF(LabelStyle& style) {
		while (!(la->kind == 0 || la->kind == 20)) {SynErr(67); Get();}
		Expect(20);
		if (la->kind == 13) {
			while (!(la->kind == 0 || la->kind == 13)) {SynErr(68); Get();}
			Get();
			if (la->kind == 27) {
				LabelStyle::Style s; 
				LABELSTYLE(s);
				style.SetStyle(s); 
			}
			if (la->kind == 21) {
				double cr,cg,cb,ca; 
				Get();
				COLOR(cr,cg,cb,ca);
				style.SetTextColor(cr,cg,cb,ca); 
			}
			if (la->kind == 22) {
				double cr,cg,cb,ca; 
				Get();
				COLOR(cr,cg,cb,ca);
				style.SetBgColor(cr,cg,cb,ca); 
			}
			if (la->kind == 23) {
				double cr,cg,cb,ca; 
				Get();
				COLOR(cr,cg,cb,ca);
				style.SetBorderColor(cr,cg,cb,ca); 
			}
			if (la->kind == 57) {
				double size=style.GetSize(); 
				SIZE(size);
				style.SetSize(size); 
			}
			if (la->kind == 41) {
				Mag minMag=style.GetMinMag(); 
				MINMAG(minMag);
				style.SetMinMag(minMag); 
			}
			if (la->kind == 42) {
				Mag maxMag=style.GetMaxMag(); 
				MAXMAG(maxMag);
				style.SetMaxMag(maxMag); 
			}
			if (la->kind == 43) {
				Mag scaleMag=style.GetScaleAndFadeMag(); 
				SCALEMAG(scaleMag);
				style.SetScaleAndFadeMag(scaleMag); 
			}
		}
}

void Parser::REFDEF(LabelStyle& style) {
		while (!(la->kind == 0 || la->kind == 24)) {SynErr(69); Get();}
		Expect(24);
		if (la->kind == 13) {
			while (!(la->kind == 0 || la->kind == 13)) {SynErr(70); Get();}
			Get();
			if (la->kind == 27) {
				LabelStyle::Style s; 
				LABELSTYLE(s);
				style.SetStyle(s); 
			}
			if (la->kind == 21) {
				double cr,cg,cb,ca; 
				Get();
				COLOR(cr,cg,cb,ca);
				style.SetTextColor(cr,cg,cb,ca); 
			}
			if (la->kind == 22) {
				double cr,cg,cb,ca; 
				Get();
				COLOR(cr,cg,cb,ca);
				style.SetBgColor(cr,cg,cb,ca); 
			}
			if (la->kind == 23) {
				double cr,cg,cb,ca; 
				Get();
				COLOR(cr,cg,cb,ca);
				style.SetBorderColor(cr,cg,cb,ca); 
			}
			if (la->kind == 57) {
				double size=style.GetSize(); 
				SIZE(size);
				style.SetSize(size); 
			}
			if (la->kind == 41) {
				Mag minMag=style.GetMinMag(); 
				MINMAG(minMag);
				style.SetMinMag(minMag); 
			}
			if (la->kind == 42) {
				Mag maxMag=style.GetMaxMag(); 
				MAXMAG(maxMag);
				style.SetMaxMag(maxMag); 
			}
			if (la->kind == 43) {
				Mag scaleMag=style.GetScaleAndFadeMag(); 
				SCALEMAG(scaleMag);
				style.SetScaleAndFadeMag(scaleMag); 
			}
		}
}

void Parser::SYMBOLDEF(SymbolStyle& style) {
		SymbolStyle::Style s;
		double             r,g,b,a;
		double             size=style.GetSize();
		
		while (!(la->kind == 0 || la->kind == 25)) {SynErr(71); Get();}
		Expect(25);
		SYMBOLSTYLE(s);
		style.SetStyle(s); 
		COLOR(r,g,b,a);
		style.SetFillColor(r,g,b,a); 
		DOUBLE(size);
		style.SetSize(size); 
		if (la->kind == 13) {
			while (!(la->kind == 0 || la->kind == 13)) {SynErr(72); Get();}
			Get();
			if (la->kind == 41) {
				Mag minMag=style.GetMinMag(); 
				MINMAG(minMag);
				style.SetMinMag(minMag); 
			}
		}
}

void Parser::ICONDEF(IconStyle& style) {
		while (!(la->kind == 0 || la->kind == 26)) {SynErr(73); Get();}
		Expect(26);
		Expect(1);
		style.SetIconName(t->val); 
		if (la->kind == 13) {
			while (!(la->kind == 0 || la->kind == 13)) {SynErr(74); Get();}
			Get();
			if (la->kind == 41) {
				Mag minMag=style.GetMinMag(); 
				MINMAG(minMag);
				style.SetMinMag(minMag); 
			}
		}
}

void Parser::LINEDEF(LineStyle& style) {
		double r,g,b,a;
		
		while (!(la->kind == 0 || la->kind == 12)) {SynErr(75); Get();}
		Expect(12);
		COLOR(r,g,b,a);
		style.SetLineColor(r,g,b,a); 
		if (la->kind == 13) {
			while (!(la->kind == 0 || la->kind == 13)) {SynErr(76); Get();}
			Get();
			if (la->kind == 27) {
				LineStyle::Style s=style.GetStyle(); 
				LINESTYLE(s);
				style.SetStyle(s); 
			}
			if (la->kind == 14) {
				double cr,cg,cb,ca; 
				Get();
				COLOR(cr,cg,cb,ca);
				style.SetAlternateColor(cr,cg,cb,ca); 
			}
			if (la->kind == 15) {
				double cr,cg,cb,ca; 
				Get();
				COLOR(cr,cg,cb,ca);
				style.SetOutlineColor(cr,cg,cb,ca); 
			}
			if (la->kind == 58) {
				double minPixel=style.GetMinPixel(); 
				MINPIXEL(minPixel);
				style.SetMinPixel(minPixel); 
			}
			if (la->kind == 59) {
				double width=style.GetWidth(); 
				WIDTH(width);
				style.SetWidth(width); 
			}
			if (la->kind == 16) {
				Get();
				style.SetFixedWidth(true); 
			}
			if (la->kind == 60) {
				double outline=style.GetOutline(); 
				OUTLINE(outline);
				style.SetOutline(outline); 
			}
		}
}

void Parser::FILLDEF(FillStyle& style, std::string& filter) {
		double r,g,b,a;
		
		while (!(la->kind == 0 || la->kind == 17)) {SynErr(77); Get();}
		Expect(17);
		COLOR(r,g,b,a);
		style.SetColor(r,g,b,a); 
		if (la->kind == 13) {
			while (!(la->kind == 0 || la->kind == 13)) {SynErr(78); Get();}
			Get();
			if (la->kind == 39) {
				int layer=style.GetLayer(); 
				LAYER(layer);
				style.SetLayer(layer); 
			}
			if (la->kind == 40) {
				FILTER(filter);
			}
		}
}

void Parser::PATTERNDEF(PatternStyle& style) {
		while (!(la->kind == 0 || la->kind == 18)) {SynErr(79); Get();}
		Expect(18);
		Expect(5);
		style.SetPattern(Destring(t->val)); 
		if (la->kind == 13) {
			while (!(la->kind == 0 || la->kind == 13)) {SynErr(80); Get();}
			Get();
			if (la->kind == 41) {
				Mag minMag=style.GetMinMag(); 
				MINMAG(minMag);
				style.SetMinMag(minMag); 
			}
			if (la->kind == 39) {
				int layer=style.GetLayer(); 
				LAYER(layer);
				style.SetLayer(layer); 
			}
		}
}

void Parser::BORDERDEF(LineStyle& style) {
		double r,g,b,a;
		
		while (!(la->kind == 0 || la->kind == 19)) {SynErr(81); Get();}
		Expect(19);
		COLOR(r,g,b,a);
		style.SetLineColor(r,g,b,a); 
		if (la->kind == 13) {
			while (!(la->kind == 0 || la->kind == 13)) {SynErr(82); Get();}
			Get();
			if (la->kind == 27) {
				LineStyle::Style s=style.GetStyle(); 
				LINESTYLE(s);
				style.SetStyle(s); 
			}
			if (la->kind == 58) {
				double minPixel=style.GetMinPixel(); 
				MINPIXEL(minPixel);
				style.SetMinPixel(minPixel); 
			}
			if (la->kind == 59) {
				double width=style.GetWidth(); 
				WIDTH(width);
				style.SetWidth(width); 
			}
		}
}

void Parser::COLOR(double& r, double& g, double& b, double& a) {
		Expect(4);
		if (strlen(t->val)==7 ||
		   strlen(t->val)==9) {
		 ToRGBA(t->val,r,g,b,a);
		}
		else {
		  r=1.0;
		  g=0.0;
		  b=0.0;
		  a=1.0;
		}
		
}

void Parser::LINESTYLE(LineStyle::Style& style) {
		Expect(27);
		if (la->kind == 28) {
			Get();
			style=LineStyle::none; 
		} else if (la->kind == 29) {
			Get();
			style=LineStyle::normal; 
		} else if (la->kind == 30) {
			Get();
			style=LineStyle::longDash; 
		} else if (la->kind == 31) {
			Get();
			style=LineStyle::dotted; 
		} else if (la->kind == 32) {
			Get();
			style=LineStyle::lineDot; 
		} else SynErr(83);
}

void Parser::MINPIXEL(double& value) {
		Expect(58);
		DOUBLE(value);
}

void Parser::WIDTH(double& value) {
		Expect(59);
		DOUBLE(value);
}

void Parser::OUTLINE(double& value) {
		Expect(60);
		DOUBLE(value);
}

void Parser::LAYER(int& layer) {
		Expect(39);
		Expect(2);
		if (!StringToNumber(t->val,layer)) {
		 std::string e="Cannot parse number '"+std::string(t->val)+"'";
		
		                      SemErr(e.c_str());
		                    }
		                  
}

void Parser::FILTER(std::string& filter) {
		Expect(40);
		Expect(5);
		filter=Destring(t->val); 
}

void Parser::MINMAG(Mag& mag) {
		Expect(41);
		MAG(mag);
}

void Parser::LABELSTYLE(LabelStyle::Style& style) {
		Expect(27);
		if (la->kind == 29) {
			Get();
			style=LabelStyle::normal; 
		} else if (la->kind == 33) {
			Get();
			style=LabelStyle::contour; 
		} else if (la->kind == 34) {
			Get();
			style=LabelStyle::plate; 
		} else if (la->kind == 35) {
			Get();
			style=LabelStyle::emphasize; 
		} else SynErr(84);
}

void Parser::SIZE(double& value) {
		Expect(57);
		DOUBLE(value);
}

void Parser::MAXMAG(Mag& mag) {
		Expect(42);
		MAG(mag);
}

void Parser::SCALEMAG(Mag& mag) {
		Expect(43);
		Expect(44);
		MAG(mag);
}

void Parser::SYMBOLSTYLE(SymbolStyle::Style& style) {
		if (la->kind == 28) {
			Get();
			style=SymbolStyle::none; 
		} else if (la->kind == 36) {
			Get();
			style=SymbolStyle::box; 
		} else if (la->kind == 37) {
			Get();
			style=SymbolStyle::triangle; 
		} else if (la->kind == 38) {
			Get();
			style=SymbolStyle::circle; 
		} else SynErr(85);
}

void Parser::DOUBLE(double& value) {
		if (la->kind == 2) {
			Get();
			if (!StringToDouble(t->val,value)) {
			 std::string e="Cannot parse double '"+std::string(t->val)+"'";
			
			                      SemErr(e.c_str());
			                    }
			                  
		} else if (la->kind == 3) {
			Get();
			if (!StringToDouble(t->val,value)) {
			 std::string e="Cannot parse double '"+std::string(t->val)+"'";
			
			                      SemErr(e.c_str());
			                    }
			                  
		} else SynErr(86);
}

void Parser::MAG(Mag& mag) {
		switch (la->kind) {
		case 45: {
			Get();
			mag=magWorld; 
			break;
		}
		case 46: {
			Get();
			mag=magState; 
			break;
		}
		case 47: {
			Get();
			mag=magStateOver; 
			break;
		}
		case 48: {
			Get();
			mag=magCounty; 
			break;
		}
		case 49: {
			Get();
			mag=magRegion; 
			break;
		}
		case 50: {
			Get();
			mag=magProximity; 
			break;
		}
		case 51: {
			Get();
			mag=magCityOver; 
			break;
		}
		case 52: {
			Get();
			mag=magCity; 
			break;
		}
		case 53: {
			Get();
			mag=magSuburb; 
			break;
		}
		case 54: {
			Get();
			mag=magDetail; 
			break;
		}
		case 55: {
			Get();
			mag=magClose; 
			break;
		}
		case 56: {
			Get();
			mag=magVeryClose; 
			break;
		}
		default: SynErr(87); break;
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
	maxT = 61;

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

	static bool set[3][63] = {
		{T,x,x,x, x,x,T,x, T,T,x,T, T,T,x,x, x,T,T,T, T,x,x,x, T,T,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, T,x,x,x, T,T,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,T,T,T, T,x,x,x, x,T,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x}
	};



  return set[s][la->kind];
}

Parser::~Parser()
{
  delete errors;
  delete dummyToken;
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
			case 8: s = coco_string_create("\"NODE\" expected"); break;
			case 9: s = coco_string_create("\"WAY\" expected"); break;
			case 10: s = coco_string_create("\"PRIO\" expected"); break;
			case 11: s = coco_string_create("\"AREA\" expected"); break;
			case 12: s = coco_string_create("\"LINE\" expected"); break;
			case 13: s = coco_string_create("\"WITH\" expected"); break;
			case 14: s = coco_string_create("\"ALTCOLOR\" expected"); break;
			case 15: s = coco_string_create("\"OUTLINECOLOR\" expected"); break;
			case 16: s = coco_string_create("\"FIXEDWIDTH\" expected"); break;
			case 17: s = coco_string_create("\"FILL\" expected"); break;
			case 18: s = coco_string_create("\"PATTERN\" expected"); break;
			case 19: s = coco_string_create("\"BORDER\" expected"); break;
			case 20: s = coco_string_create("\"LABEL\" expected"); break;
			case 21: s = coco_string_create("\"COLOR\" expected"); break;
			case 22: s = coco_string_create("\"BGCOLOR\" expected"); break;
			case 23: s = coco_string_create("\"BORDERCOLOR\" expected"); break;
			case 24: s = coco_string_create("\"REF\" expected"); break;
			case 25: s = coco_string_create("\"SYMBOL\" expected"); break;
			case 26: s = coco_string_create("\"ICON\" expected"); break;
			case 27: s = coco_string_create("\"STYLE\" expected"); break;
			case 28: s = coco_string_create("\"none\" expected"); break;
			case 29: s = coco_string_create("\"normal\" expected"); break;
			case 30: s = coco_string_create("\"longDash\" expected"); break;
			case 31: s = coco_string_create("\"dotted\" expected"); break;
			case 32: s = coco_string_create("\"lineDot\" expected"); break;
			case 33: s = coco_string_create("\"contour\" expected"); break;
			case 34: s = coco_string_create("\"plate\" expected"); break;
			case 35: s = coco_string_create("\"emphasize\" expected"); break;
			case 36: s = coco_string_create("\"box\" expected"); break;
			case 37: s = coco_string_create("\"triangle\" expected"); break;
			case 38: s = coco_string_create("\"circle\" expected"); break;
			case 39: s = coco_string_create("\"LAYER\" expected"); break;
			case 40: s = coco_string_create("\"FILTER\" expected"); break;
			case 41: s = coco_string_create("\"MINMAG\" expected"); break;
			case 42: s = coco_string_create("\"MAXMAG\" expected"); break;
			case 43: s = coco_string_create("\"FADE\" expected"); break;
			case 44: s = coco_string_create("\"AT\" expected"); break;
			case 45: s = coco_string_create("\"world\" expected"); break;
			case 46: s = coco_string_create("\"state\" expected"); break;
			case 47: s = coco_string_create("\"stateOver\" expected"); break;
			case 48: s = coco_string_create("\"county\" expected"); break;
			case 49: s = coco_string_create("\"region\" expected"); break;
			case 50: s = coco_string_create("\"proximity\" expected"); break;
			case 51: s = coco_string_create("\"cityOver\" expected"); break;
			case 52: s = coco_string_create("\"city\" expected"); break;
			case 53: s = coco_string_create("\"suburb\" expected"); break;
			case 54: s = coco_string_create("\"detail\" expected"); break;
			case 55: s = coco_string_create("\"close\" expected"); break;
			case 56: s = coco_string_create("\"veryClose\" expected"); break;
			case 57: s = coco_string_create("\"SIZE\" expected"); break;
			case 58: s = coco_string_create("\"MINPIXEL\" expected"); break;
			case 59: s = coco_string_create("\"WIDTH\" expected"); break;
			case 60: s = coco_string_create("\"OUTLINE\" expected"); break;
			case 61: s = coco_string_create("??? expected"); break;
			case 62: s = coco_string_create("this symbol not expected in OSS"); break;
			case 63: s = coco_string_create("invalid STYLE"); break;
			case 64: s = coco_string_create("this symbol not expected in NODESTYLE"); break;
			case 65: s = coco_string_create("this symbol not expected in WAYSTYLE"); break;
			case 66: s = coco_string_create("this symbol not expected in AREASTYLE"); break;
			case 67: s = coco_string_create("this symbol not expected in LABELDEF"); break;
			case 68: s = coco_string_create("this symbol not expected in LABELDEF"); break;
			case 69: s = coco_string_create("this symbol not expected in REFDEF"); break;
			case 70: s = coco_string_create("this symbol not expected in REFDEF"); break;
			case 71: s = coco_string_create("this symbol not expected in SYMBOLDEF"); break;
			case 72: s = coco_string_create("this symbol not expected in SYMBOLDEF"); break;
			case 73: s = coco_string_create("this symbol not expected in ICONDEF"); break;
			case 74: s = coco_string_create("this symbol not expected in ICONDEF"); break;
			case 75: s = coco_string_create("this symbol not expected in LINEDEF"); break;
			case 76: s = coco_string_create("this symbol not expected in LINEDEF"); break;
			case 77: s = coco_string_create("this symbol not expected in FILLDEF"); break;
			case 78: s = coco_string_create("this symbol not expected in FILLDEF"); break;
			case 79: s = coco_string_create("this symbol not expected in PATTERNDEF"); break;
			case 80: s = coco_string_create("this symbol not expected in PATTERNDEF"); break;
			case 81: s = coco_string_create("this symbol not expected in BORDERDEF"); break;
			case 82: s = coco_string_create("this symbol not expected in BORDERDEF"); break;
			case 83: s = coco_string_create("invalid LINESTYLE"); break;
			case 84: s = coco_string_create("invalid LABELSTYLE"); break;
			case 85: s = coco_string_create("invalid SYMBOLSTYLE"); break;
			case 86: s = coco_string_create("invalid DOUBLE"); break;
			case 87: s = coco_string_create("invalid MAG"); break;

    default:
    {
      s = coco_string_create("Unknown error");
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


