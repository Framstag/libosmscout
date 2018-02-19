/*
  This source is part of the libosmscout library
  Copyright (C) 2017  Tim Teulings

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

#include <osmscout/olt/Parser.h>

#include <osmscout/olt/Scanner.h>

#include <iostream>
#include <sstream>

namespace osmscout {
namespace olt {


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

void Parser::OLT() {
		while (!(la->kind == _EOF || la->kind == 4 /* "OLT" */)) {SynErr(22); Get();}
		Expect(4 /* "OLT" */);
		while (StartOf(1)) {
			osmscout::test::RegionRef region=std::make_shared<osmscout::test::Region>(); 
			REGION(*region);
			regionList.AddRegion(region); 
		}
		Expect(5 /* "END" */);
}

void Parser::REGION(osmscout::test::Region& region) {
		std::string name;
		
		while (!(StartOf(2))) {SynErr(23); Get();}
		if (la->kind == 6 /* "REGION" */) {
			Get();
			region.SetPlaceType(osmscout::test::PlaceType::region); 
		} else if (la->kind == 7 /* "COUNTY" */) {
			Get();
			region.SetPlaceType(osmscout::test::PlaceType::county); 
		} else if (la->kind == 8 /* "CITY" */) {
			Get();
			region.SetPlaceType(osmscout::test::PlaceType::city); 
		} else if (la->kind == 9 /* "SUBURB" */) {
			Get();
			region.SetPlaceType(osmscout::test::PlaceType::suburb); 
		} else if (la->kind == 10 /* "OBJECT" */) {
			Get();
		} else SynErr(24);
		if (la->kind == 11 /* "BOUNDARY" */) {
			size_t adminLevel; 
			Get();
			UINT(adminLevel);
			region.SetAdminLevel(adminLevel); 
		}
		if (la->kind == 12 /* "NODE" */) {
			Get();
			region.SetIsNode(); 
		}
		STRING(name);
		region.SetName(name); 
		if (la->kind == 13 /* "{" */) {
			Get();
			while (la->kind == 15 /* "POSTAL_AREA" */) {
				osmscout::test::PostalAreaRef postalArea=std::make_shared<osmscout::test::PostalArea>(); 
				POSTAL_AREA(*postalArea);
				region.AddPostalArea(postalArea); 
			}
			while (StartOf(1)) {
				osmscout::test::RegionRef childRegion=std::make_shared<osmscout::test::Region>(); 
				REGION(*childRegion);
				region.AddRegion(childRegion); 
			}
			Expect(14 /* "}" */);
		}
}

void Parser::UINT(size_t& value) {
		Expect(_number);
		if (!StringToNumber(t->val,value)) {
		 std::string e="Cannot parse number '"+std::string(t->val)+"'";
		
		 SemErr(e.c_str());
		}
		
}

void Parser::STRING(std::string& value) {
		Expect(_string);
		value=Destring(t->val);
		
}

void Parser::POSTAL_AREA(osmscout::test::PostalArea& postalArea) {
		std::string name;
		
		while (!(la->kind == _EOF || la->kind == 15 /* "POSTAL_AREA" */)) {SynErr(25); Get();}
		Expect(15 /* "POSTAL_AREA" */);
		if (la->kind == _string) {
			STRING(name);
			postalArea.SetName(name); 
		}
		while (la->kind == 16 /* "LOCATION" */) {
			osmscout::test::LocationRef location=std::make_shared<osmscout::test::Location>(); 
			LOCATION(*location);
			postalArea.AddLocation(location); 
		}
}

void Parser::LOCATION(osmscout::test::Location& location) {
		std::string name;
		
		while (!(la->kind == _EOF || la->kind == 16 /* "LOCATION" */)) {SynErr(26); Get();}
		Expect(16 /* "LOCATION" */);
		STRING(name);
		location.SetName(name); 
		while (la->kind == 17 /* "ADDRESS" */) {
			osmscout::test::AddressRef address=std::make_shared<osmscout::test::Address>(); 
			ADDRESS(*address);
			location.AddAddress(address); 
		}
}

void Parser::ADDRESS(osmscout::test::Address& address) {
		std::string name;
		
		while (!(la->kind == _EOF || la->kind == 17 /* "ADDRESS" */)) {SynErr(27); Get();}
		Expect(17 /* "ADDRESS" */);
		STRING(name);
		address.SetName(name); 
		if (la->kind == 18 /* "[" */) {
			Get();
			while (la->kind == _string) {
				std::string tagKey,tagValue; 
				STRING(tagKey);
				Expect(19 /* "=" */);
				STRING(tagValue);
				address.AddTag(tagKey,tagValue); 
			}
			Expect(20 /* "]" */);
		}
}



void Parser::Parse()
{
  t = NULL;
  la = dummyToken = std::make_shared<Token>();
  la->val = coco_string_create("Dummy Token");
  Get();
	OLT();
	Expect(0);
}

Parser::Parser(Scanner *scanner)
{
	maxT = 21;

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

	static bool set[3][23] = {
		{T,x,x,x, T,x,T,T, T,T,T,x, x,x,x,T, T,T,x,x, x,x,x},
		{x,x,x,x, x,x,T,T, T,T,T,x, x,x,x,x, x,x,x,x, x,x,x},
		{T,x,x,x, x,x,T,T, T,T,T,x, x,x,x,x, x,x,x,x, x,x,x}
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
			case 3: s = coco_string_create("string expected"); break;
			case 4: s = coco_string_create("\"OLT\" expected"); break;
			case 5: s = coco_string_create("\"END\" expected"); break;
			case 6: s = coco_string_create("\"REGION\" expected"); break;
			case 7: s = coco_string_create("\"COUNTY\" expected"); break;
			case 8: s = coco_string_create("\"CITY\" expected"); break;
			case 9: s = coco_string_create("\"SUBURB\" expected"); break;
			case 10: s = coco_string_create("\"OBJECT\" expected"); break;
			case 11: s = coco_string_create("\"BOUNDARY\" expected"); break;
			case 12: s = coco_string_create("\"NODE\" expected"); break;
			case 13: s = coco_string_create("\"{\" expected"); break;
			case 14: s = coco_string_create("\"}\" expected"); break;
			case 15: s = coco_string_create("\"POSTAL_AREA\" expected"); break;
			case 16: s = coco_string_create("\"LOCATION\" expected"); break;
			case 17: s = coco_string_create("\"ADDRESS\" expected"); break;
			case 18: s = coco_string_create("\"[\" expected"); break;
			case 19: s = coco_string_create("\"=\" expected"); break;
			case 20: s = coco_string_create("\"]\" expected"); break;
			case 21: s = coco_string_create("??? expected"); break;
			case 22: s = coco_string_create("this symbol not expected in OLT"); break;
			case 23: s = coco_string_create("this symbol not expected in REGION"); break;
			case 24: s = coco_string_create("invalid REGION"); break;
			case 25: s = coco_string_create("this symbol not expected in POSTAL_AREA"); break;
			case 26: s = coco_string_create("this symbol not expected in LOCATION"); break;
			case 27: s = coco_string_create("this symbol not expected in ADDRESS"); break;

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

