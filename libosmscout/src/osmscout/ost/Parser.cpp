
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

#include <wchar.h>
#include <osmscout/ost/Scanner.h>
#include <osmscout/ost/Parser.h>


namespace osmscout {
namespace ost {


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

void Parser::OST() {
		while (!(la->kind == 0 || la->kind == 4)) {SynErr(20); Get();}
		Expect(4);
		if (la->kind == 6) {
			TYPES();
		}
		Expect(5);
}

void Parser::TYPES() {
		while (!(la->kind == 0 || la->kind == 6)) {SynErr(21); Get();}
		Expect(6);
		TYPE();
		while (la->kind == 7) {
			TYPE();
		}
}

void Parser::TYPE() {
		std::string idValue;
		std::string typeValue;
		Condition   *condition=NULL;
		TypeInfo    typeInfo;
		
		while (!(la->kind == 0 || la->kind == 7)) {SynErr(22); Get();}
		Expect(7);
		Expect(3);
		typeValue=Destring(t->val);
		
		Expect(8);
		CONDITION(condition);
		typeInfo.SetType(typeValue,condition);
		
		TYPEKINDS(typeInfo);
		if (la->kind == 16) {
			TYPEOPTIONS(typeInfo);
		}
		config.AddTypeInfo(typeInfo);
		
}

void Parser::CONDITION(Condition*& condition) {
		std::string nameValue;
		std::string valueValue;
		
		Expect(3);
		nameValue=Destring(t->val); 
		Expect(9);
		Expect(3);
		valueValue=Destring(t->val); 
		TagId tagId=config.RegisterTagForInternalUse(nameValue);
		
		                  condition=new TagEquals(tagId,valueValue);
		                
}

void Parser::TYPEKINDS(TypeInfo& typeInfo) {
		Expect(10);
		Expect(11);
		TYPEKIND(typeInfo);
		while (StartOf(1)) {
			TYPEKIND(typeInfo);
		}
}

void Parser::TYPEOPTIONS(TypeInfo& typeInfo) {
		Expect(16);
		TYPEOPTION(typeInfo);
		while (la->kind == 17 || la->kind == 18) {
			TYPEOPTION(typeInfo);
		}
}

void Parser::TYPEKIND(TypeInfo& typeInfo) {
		if (la->kind == 12) {
			Get();
			typeInfo.CanBeNode(true); 
		} else if (la->kind == 13) {
			Get();
			typeInfo.CanBeWay(true); 
		} else if (la->kind == 14) {
			Get();
			typeInfo.CanBeArea(true); 
		} else if (la->kind == 15) {
			Get();
			typeInfo.CanBeRelation(true); 
		} else SynErr(23);
}

void Parser::TYPEOPTION(TypeInfo& typeInfo) {
		if (la->kind == 17) {
			Get();
			typeInfo.CanBeRoute(true); 
		} else if (la->kind == 18) {
			Get();
			typeInfo.CanBeIndexed(true); 
		} else SynErr(24);
}



void Parser::Parse()
{
  t = NULL;
  la = dummyToken = new Token();
  la->val = coco_string_create("Dummy Token");
  Get();
	OST();

  Expect(0);
}

Parser::Parser(Scanner *scanner,
               TypeConfig& config)
 : config(config)
{
	maxT = 19;

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

	static bool set[2][21] = {
		{T,x,x,x, T,x,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x},
		{x,x,x,x, x,x,x,x, x,x,x,x, T,T,T,T, x,x,x,x, x}
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
			case 3: s = coco_string_create("string expected"); break;
			case 4: s = coco_string_create("\"OST\" expected"); break;
			case 5: s = coco_string_create("\"END\" expected"); break;
			case 6: s = coco_string_create("\"TYPES\" expected"); break;
			case 7: s = coco_string_create("\"TYPE\" expected"); break;
			case 8: s = coco_string_create("\"WHERE\" expected"); break;
			case 9: s = coco_string_create("\"==\" expected"); break;
			case 10: s = coco_string_create("\"CAN\" expected"); break;
			case 11: s = coco_string_create("\"BE\" expected"); break;
			case 12: s = coco_string_create("\"NODE\" expected"); break;
			case 13: s = coco_string_create("\"WAY\" expected"); break;
			case 14: s = coco_string_create("\"AREA\" expected"); break;
			case 15: s = coco_string_create("\"RELATION\" expected"); break;
			case 16: s = coco_string_create("\"OPTIONS\" expected"); break;
			case 17: s = coco_string_create("\"ROUTE\" expected"); break;
			case 18: s = coco_string_create("\"INDEX\" expected"); break;
			case 19: s = coco_string_create("??? expected"); break;
			case 20: s = coco_string_create("this symbol not expected in OST"); break;
			case 21: s = coco_string_create("this symbol not expected in TYPES"); break;
			case 22: s = coco_string_create("this symbol not expected in TYPE"); break;
			case 23: s = coco_string_create("invalid TYPEKIND"); break;
			case 24: s = coco_string_create("invalid TYPEOPTION"); break;

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


