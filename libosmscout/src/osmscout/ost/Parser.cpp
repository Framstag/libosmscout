
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
		while (!(la->kind == 0 || la->kind == 4)) {SynErr(31); Get();}
		Expect(4);
		if (la->kind == 6) {
			TYPES();
		}
		Expect(5);
}

void Parser::TYPES() {
		while (!(la->kind == 0 || la->kind == 6)) {SynErr(32); Get();}
		Expect(6);
		TYPE();
		while (la->kind == 7) {
			TYPE();
		}
}

void Parser::TYPE() {
		Condition     *condition=NULL;
		TypeInfo      typeInfo;
		unsigned char types;
		
		while (!(la->kind == 0 || la->kind == 7)) {SynErr(33); Get();}
		Expect(7);
		Expect(3);
		typeInfo.SetType(Destring(t->val)); 
		Expect(8);
		TYPEKINDS(types);
		Expect(9);
		CONDITION(condition);
		Expect(10);
		typeInfo.AddCondition(types,condition); 
		while (la->kind == 11) {
			Get();
			TYPEKINDS(types);
			Expect(9);
			CONDITION(condition);
			Expect(10);
			typeInfo.AddCondition(types,condition); 
		}
		if (la->kind == 12) {
			Get();
			TYPEOPTIONS(typeInfo);
		}
		config.AddTypeInfo(typeInfo);
		
}

void Parser::TYPEKINDS(unsigned char& types) {
		types=0;
		
		TYPEKIND(types);
		while (StartOf(1)) {
			if (la->kind == 19) {
				Get();
			}
			TYPEKIND(types);
		}
}

void Parser::CONDITION(Condition*& condition) {
		std::list<Condition*> conditions;
		Condition             *subCond;
		
		ANDCOND(subCond);
		conditions.push_back(subCond); 
		while (la->kind == 11) {
			Get();
			ANDCOND(subCond);
			conditions.push_back(subCond); 
		}
		if (conditions.size()==1) {
		 condition=conditions.front();
		}
		else {
		  OrCondition *orCondition=new OrCondition();
		
		                    for (std::list<Condition*>::const_iterator c=conditions.begin();
		                         c!=conditions.end();
		                         ++c) {
		                      orCondition->AddCondition(*c);
		                    }
		
		                    condition=orCondition;
		                  }
		                
}

void Parser::TYPEOPTIONS(TypeInfo& typeInfo) {
		TYPEOPTION(typeInfo);
		while (StartOf(2)) {
			TYPEOPTION(typeInfo);
		}
}

void Parser::ANDCOND(Condition*& condition) {
		std::list<Condition*> conditions;
		Condition             *subCond;
		
		BOOLCOND(subCond);
		conditions.push_back(subCond); 
		while (la->kind == 13) {
			Get();
			BOOLCOND(subCond);
			conditions.push_back(subCond); 
		}
		if (conditions.size()==1) {
		 condition=conditions.front();
		}
		else {
		  AndCondition *andCondition=new AndCondition();
		
		                    for (std::list<Condition*>::const_iterator c=conditions.begin();
		                         c!=conditions.end();
		                         ++c) {
		                      andCondition->AddCondition(*c);
		                    }
		
		                    condition=andCondition;
		                  }
		                
}

void Parser::BOOLCOND(Condition*& condition) {
		if (la->kind == 3) {
			BINARYCOND(condition);
		} else if (la->kind == 21) {
			EXISTSCOND(condition);
		} else if (la->kind == 9) {
			Get();
			CONDITION(condition);
			Expect(10);
		} else if (la->kind == 14) {
			Get();
			BOOLCOND(condition);
			condition=new NotCondition(condition); 
		} else SynErr(34);
}

void Parser::BINARYCOND(Condition*& condition) {
		std::string nameValue;
		
		Expect(3);
		nameValue=Destring(t->val); 
		if (la->kind == 15) {
			EQUALSCOND(nameValue,condition);
		} else if (la->kind == 16) {
			NOTEQUALSCOND(nameValue,condition);
		} else if (la->kind == 17) {
			ISINCOND(nameValue,condition);
		} else SynErr(35);
}

void Parser::EXISTSCOND(Condition*& condition) {
		Expect(21);
		Expect(3);
		condition=new ExistsCondition(config.RegisterTagForInternalUse(Destring(t->val)));
		
}

void Parser::EQUALSCOND(const std::string& tagName,Condition*& condition) {
		std::string valueValue;
		
		Expect(15);
		Expect(3);
		valueValue=Destring(t->val); 
		TagId tagId=config.RegisterTagForInternalUse(tagName);
		
		                  condition=new EqualsCondition(tagId,valueValue);
		                
}

void Parser::NOTEQUALSCOND(const std::string& tagName,Condition*& condition) {
		std::string valueValue;
		
		Expect(16);
		Expect(3);
		valueValue=Destring(t->val); 
		TagId tagId=config.RegisterTagForInternalUse(tagName);
		
		                  condition=new NotEqualsCondition(tagId,valueValue);
		                
}

void Parser::ISINCOND(const std::string& tagName,Condition*& condition) {
		std::list<std::string> values;
		
		Expect(17);
		Expect(18);
		Expect(3);
		values.push_back(Destring(t->val)); 
		while (la->kind == 19) {
			Get();
			Expect(3);
			values.push_back(Destring(t->val)); 
		}
		Expect(20);
		TagId tagId=config.RegisterTagForInternalUse(tagName);
		
		                  if (values.size()==1) {
		                    condition=new EqualsCondition(tagId,values.front());
		                  }
		                  else {
		                    IsInCondition *isInCondition=new IsInCondition(tagId);
		
		                    for (std::list<std::string>::const_iterator s=values.begin();
		                         s!=values.end();
		                         ++s) {
		                      isInCondition->AddTagValue(*s);
		                    }
		
		                    condition=isInCondition;
		                  }
		                
}

void Parser::TYPEKIND(unsigned char& types) {
		if (la->kind == 22) {
			Get();
			types|=TypeInfo::typeNode; 
		} else if (la->kind == 23) {
			Get();
			types|=TypeInfo::typeWay; 
		} else if (la->kind == 24) {
			Get();
			types|=TypeInfo::typeArea; 
		} else if (la->kind == 25) {
			Get();
			types|=TypeInfo::typeRelation; 
		} else SynErr(36);
}

void Parser::TYPEOPTION(TypeInfo& typeInfo) {
		if (la->kind == 26) {
			Get();
			typeInfo.CanBeRoute(true); 
		} else if (la->kind == 27) {
			Get();
			typeInfo.CanBeIndexed(true); 
		} else if (la->kind == 28) {
			Get();
			typeInfo.SetConsumeChildren(true); 
		} else if (la->kind == 29) {
			Get();
			typeInfo.SetIgnore(true); 
		} else SynErr(37);
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
	maxT = 30;

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

	static bool set[3][32] = {
		{T,x,x,x, T,x,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,T, x,x,T,T, T,T,x,x, x,x,x,x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,T,T, T,T,x,x}
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
			case 4: s = coco_string_create("\"OST\" expected"); break;
			case 5: s = coco_string_create("\"END\" expected"); break;
			case 6: s = coco_string_create("\"TYPES\" expected"); break;
			case 7: s = coco_string_create("\"TYPE\" expected"); break;
			case 8: s = coco_string_create("\"=\" expected"); break;
			case 9: s = coco_string_create("\"(\" expected"); break;
			case 10: s = coco_string_create("\")\" expected"); break;
			case 11: s = coco_string_create("\"OR\" expected"); break;
			case 12: s = coco_string_create("\"OPTIONS\" expected"); break;
			case 13: s = coco_string_create("\"AND\" expected"); break;
			case 14: s = coco_string_create("\"!\" expected"); break;
			case 15: s = coco_string_create("\"==\" expected"); break;
			case 16: s = coco_string_create("\"!=\" expected"); break;
			case 17: s = coco_string_create("\"IN\" expected"); break;
			case 18: s = coco_string_create("\"[\" expected"); break;
			case 19: s = coco_string_create("\",\" expected"); break;
			case 20: s = coco_string_create("\"]\" expected"); break;
			case 21: s = coco_string_create("\"EXISTS\" expected"); break;
			case 22: s = coco_string_create("\"NODE\" expected"); break;
			case 23: s = coco_string_create("\"WAY\" expected"); break;
			case 24: s = coco_string_create("\"AREA\" expected"); break;
			case 25: s = coco_string_create("\"RELATION\" expected"); break;
			case 26: s = coco_string_create("\"ROUTE\" expected"); break;
			case 27: s = coco_string_create("\"INDEX\" expected"); break;
			case 28: s = coco_string_create("\"CONSUME_CHILDREN\" expected"); break;
			case 29: s = coco_string_create("\"IGNORE\" expected"); break;
			case 30: s = coco_string_create("??? expected"); break;
			case 31: s = coco_string_create("this symbol not expected in OST"); break;
			case 32: s = coco_string_create("this symbol not expected in TYPES"); break;
			case 33: s = coco_string_create("this symbol not expected in TYPE"); break;
			case 34: s = coco_string_create("invalid BOOLCOND"); break;
			case 35: s = coco_string_create("invalid BINARYCOND"); break;
			case 36: s = coco_string_create("invalid TYPEKIND"); break;
			case 37: s = coco_string_create("invalid TYPEOPTION"); break;

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


