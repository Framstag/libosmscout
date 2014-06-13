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

#include <osmscout/ost/Parser.h>

#include <osmscout/ost/Scanner.h>

#include <iostream>
#include <sstream>

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
		while (!(la->kind == _EOF || la->kind == 4 /* "OST" */)) {SynErr(50); Get();}
		Expect(4 /* "OST" */);
		if (la->kind == 6 /* "GRADES" */) {
			GRADES();
		}
		if (la->kind == 11 /* "TYPES" */) {
			TYPES();
		}
		if (la->kind == 18 /* "TAGS" */) {
			TAGS();
		}
		Expect(5 /* "END" */);
}

void Parser::GRADES() {
		while (!(la->kind == _EOF || la->kind == 6 /* "GRADES" */)) {SynErr(51); Get();}
		Expect(6 /* "GRADES" */);
		GRADE();
		while (la->kind == 7 /* "SURFACE" */) {
			GRADE();
		}
}

void Parser::TYPES() {
		while (!(la->kind == _EOF || la->kind == 11 /* "TYPES" */)) {SynErr(52); Get();}
		Expect(11 /* "TYPES" */);
		TYPE();
		while (la->kind == 12 /* "TYPE" */) {
			TYPE();
		}
}

void Parser::TAGS() {
		while (!(la->kind == _EOF || la->kind == 18 /* "TAGS" */)) {SynErr(53); Get();}
		Expect(18 /* "TAGS" */);
		TAG();
		while (la->kind == 19 /* "TAG" */) {
			TAG();
		}
}

void Parser::GRADE() {
		size_t grade;
		
		while (!(la->kind == _EOF || la->kind == 7 /* "SURFACE" */)) {SynErr(54); Get();}
		Expect(7 /* "SURFACE" */);
		Expect(8 /* "GRADE" */);
		UINT(grade);
		Expect(9 /* "{" */);
		while (la->kind == _string) {
			std::string surface;
			
			STRING(surface);
			if (grade>=1 && grade<=5) {
			 config.RegisterSurfaceToGradeMapping(surface,
			                                      grade);
			}
			else {
			 std::string e="Not a valid grade level: "+NumberToString(grade);
			
			 SemErr(e.c_str());
			}
			
		}
		Expect(10 /* "}" */);
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

void Parser::TYPE() {
		std::string   name;
		TagCondition  *condition=NULL;
		TypeInfo      typeInfo;
		unsigned char types;
		
		while (!(la->kind == _EOF || la->kind == 12 /* "TYPE" */)) {SynErr(55); Get();}
		Expect(12 /* "TYPE" */);
		IDENT(name);
		typeInfo.SetType(name); 
		Expect(13 /* "=" */);
		TYPEKINDS(types);
		Expect(14 /* "(" */);
		TAGCONDITION(condition);
		Expect(15 /* ")" */);
		typeInfo.AddCondition(types,condition); 
		while (la->kind == 16 /* "OR" */) {
			Get();
			TYPEKINDS(types);
			Expect(14 /* "(" */);
			TAGCONDITION(condition);
			Expect(15 /* ")" */);
			typeInfo.AddCondition(types,condition); 
		}
		if (la->kind == 17 /* "OPTIONS" */) {
			Get();
			TYPEOPTIONS(typeInfo);
		}
		config.AddTypeInfo(typeInfo);
		
}

void Parser::IDENT(std::string& value) {
		Expect(_ident);
		value=t->val;
		
}

void Parser::TYPEKINDS(unsigned char& types) {
		types=0;
		
		TYPEKIND(types);
		while (StartOf(1)) {
			if (la->kind == 30 /* "," */) {
				Get();
			}
			TYPEKIND(types);
		}
}

void Parser::TAGCONDITION(TagCondition*& condition) {
		std::list<TagCondition*> conditions;
		TagCondition             *subCond;
		
		TAGANDCOND(subCond);
		conditions.push_back(subCond); 
		while (la->kind == 16 /* "OR" */) {
			Get();
			TAGANDCOND(subCond);
			conditions.push_back(subCond); 
		}
		if (conditions.size()==1) {
		 condition=conditions.front();
		}
		else {
		 TagBoolCondition *orCondition=new TagBoolCondition(TagBoolCondition::boolOr);
		
		 for (std::list<TagCondition*>::const_iterator c=conditions.begin();
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

void Parser::TAG() {
		while (!(la->kind == _EOF || la->kind == 19 /* "TAG" */)) {SynErr(56); Get();}
		Expect(19 /* "TAG" */);
		Expect(_string);
		std::string tagName=Destring(t->val);
		
		if (tagName.empty()) {
		 std::string e="Empty tags are not allowed";
		
		 SemErr(e.c_str());
		}
		else {
		 config.RegisterTagForExternalUse(tagName);
		}
		
}

void Parser::TAGANDCOND(TagCondition*& condition) {
		std::list<TagCondition*> conditions;
		TagCondition             *subCond;
		
		TAGBOOLCOND(subCond);
		conditions.push_back(subCond); 
		while (la->kind == 20 /* "AND" */) {
			Get();
			TAGBOOLCOND(subCond);
			conditions.push_back(subCond); 
		}
		if (conditions.size()==1) {
		 condition=conditions.front();
		}
		else {
		 TagBoolCondition *andCondition=new TagBoolCondition(TagBoolCondition::boolAnd);
		
		 for (std::list<TagCondition*>::const_iterator c=conditions.begin();
		      c!=conditions.end();
		      ++c) {
		   andCondition->AddCondition(*c);
		 }
		
		 condition=andCondition;
		}
		
}

void Parser::TAGBOOLCOND(TagCondition*& condition) {
		if (la->kind == _string) {
			TAGBINCOND(condition);
		} else if (la->kind == 32 /* "EXISTS" */) {
			TAGEXISTSCOND(condition);
		} else if (la->kind == 14 /* "(" */) {
			Get();
			TAGCONDITION(condition);
			Expect(15 /* ")" */);
		} else if (la->kind == 21 /* "!" */) {
			Get();
			TAGBOOLCOND(condition);
			condition=new TagNotCondition(condition); 
		} else SynErr(57);
}

void Parser::TAGBINCOND(TagCondition*& condition) {
		std::string nameValue;
		
		Expect(_string);
		nameValue=Destring(t->val); 
		switch (la->kind) {
		case 22 /* "<" */: {
			TAGLESSCOND(nameValue,condition);
			break;
		}
		case 23 /* "<=" */: {
			TAGLESSEQUALCOND(nameValue,condition);
			break;
		}
		case 24 /* "==" */: {
			TAGEQUALSCOND(nameValue,condition);
			break;
		}
		case 25 /* "!=" */: {
			TAGNOTEQUALSCOND(nameValue,condition);
			break;
		}
		case 27 /* ">" */: {
			TAGGREATERCOND(nameValue,condition);
			break;
		}
		case 26 /* ">=" */: {
			TAGGREATEREQUALCOND(nameValue,condition);
			break;
		}
		case 28 /* "IN" */: {
			TAGISINCOND(nameValue,condition);
			break;
		}
		default: SynErr(58); break;
		}
}

void Parser::TAGEXISTSCOND(TagCondition*& condition) {
		Expect(32 /* "EXISTS" */);
		Expect(_string);
		condition=new TagExistsCondition(config.RegisterTagForInternalUse(Destring(t->val)));
		
}

void Parser::TAGLESSCOND(const std::string& tagName,TagCondition*& condition) {
		std::string stringValue;
		size_t      sizeValue;
		
		Expect(22 /* "<" */);
		if (la->kind == _string) {
			STRING(stringValue);
			TagId tagId=config.RegisterTagForInternalUse(tagName);
			
			condition=new TagBinaryCondition(tagId,operatorLess,stringValue);
			
		} else if (la->kind == _number) {
			UINT(sizeValue);
			TagId tagId=config.RegisterTagForInternalUse(tagName);
			
			condition=new TagBinaryCondition(tagId,operatorLess,sizeValue);
			
		} else SynErr(59);
}

void Parser::TAGLESSEQUALCOND(const std::string& tagName,TagCondition*& condition) {
		std::string stringValue;
		size_t      sizeValue;
		
		Expect(23 /* "<=" */);
		if (la->kind == _string) {
			STRING(stringValue);
			TagId tagId=config.RegisterTagForInternalUse(tagName);
			
			condition=new TagBinaryCondition(tagId,operatorLessEqual,stringValue);
			
		} else if (la->kind == _number) {
			UINT(sizeValue);
			TagId tagId=config.RegisterTagForInternalUse(tagName);
			
			condition=new TagBinaryCondition(tagId,operatorLessEqual,sizeValue);
			
		} else SynErr(60);
}

void Parser::TAGEQUALSCOND(const std::string& tagName,TagCondition*& condition) {
		std::string stringValue;
		size_t      sizeValue;
		
		Expect(24 /* "==" */);
		if (la->kind == _string) {
			STRING(stringValue);
			TagId tagId=config.RegisterTagForInternalUse(tagName);
			
			condition=new TagBinaryCondition(tagId,operatorEqual,stringValue);
			
		} else if (la->kind == _number) {
			UINT(sizeValue);
			TagId tagId=config.RegisterTagForInternalUse(tagName);
			
			condition=new TagBinaryCondition(tagId,operatorEqual,sizeValue);
			
		} else SynErr(61);
}

void Parser::TAGNOTEQUALSCOND(const std::string& tagName,TagCondition*& condition) {
		std::string stringValue;
		size_t      sizeValue;
		
		Expect(25 /* "!=" */);
		if (la->kind == _string) {
			STRING(stringValue);
			TagId tagId=config.RegisterTagForInternalUse(tagName);
			
			condition=new TagBinaryCondition(tagId,operatorNotEqual,stringValue);
			
		} else if (la->kind == _number) {
			UINT(sizeValue);
			TagId tagId=config.RegisterTagForInternalUse(tagName);
			
			condition=new TagBinaryCondition(tagId,operatorNotEqual,sizeValue);
			
		} else SynErr(62);
}

void Parser::TAGGREATERCOND(const std::string& tagName,TagCondition*& condition) {
		std::string stringValue;
		size_t      sizeValue;
		
		Expect(27 /* ">" */);
		if (la->kind == _string) {
			STRING(stringValue);
			TagId tagId=config.RegisterTagForInternalUse(tagName);
			
			condition=new TagBinaryCondition(tagId,operatorGreater,stringValue);
			
		} else if (la->kind == _number) {
			UINT(sizeValue);
			TagId tagId=config.RegisterTagForInternalUse(tagName);
			
			condition=new TagBinaryCondition(tagId,operatorGreater,sizeValue);
			
		} else SynErr(63);
}

void Parser::TAGGREATEREQUALCOND(const std::string& tagName,TagCondition*& condition) {
		std::string stringValue;
		size_t      sizeValue;
		
		Expect(26 /* ">=" */);
		if (la->kind == _string) {
			STRING(stringValue);
			TagId tagId=config.RegisterTagForInternalUse(tagName);
			
			condition=new TagBinaryCondition(tagId,operatorGreaterEqual,stringValue);
			
		} else if (la->kind == _number) {
			UINT(sizeValue);
			TagId tagId=config.RegisterTagForInternalUse(tagName);
			
			condition=new TagBinaryCondition(tagId,operatorGreaterEqual,sizeValue);
			
		} else SynErr(64);
}

void Parser::TAGISINCOND(const std::string& tagName,TagCondition*& condition) {
		std::list<std::string> values;
		
		Expect(28 /* "IN" */);
		Expect(29 /* "[" */);
		Expect(_string);
		values.push_back(Destring(t->val)); 
		while (la->kind == 30 /* "," */) {
			Get();
			Expect(_string);
			values.push_back(Destring(t->val)); 
		}
		Expect(31 /* "]" */);
		TagId tagId=config.RegisterTagForInternalUse(tagName);
		
		if (values.size()==1) {
		 condition=new TagBinaryCondition(tagId,operatorEqual,values.front());
		}
		else {
		 TagIsInCondition *isInCondition=new TagIsInCondition(tagId);
		
		 for (std::list<std::string>::const_iterator s=values.begin();
		      s!=values.end();
		      ++s) {
		   isInCondition->AddTagValue(*s);
		 }
		
		 condition=isInCondition;
		}
		
}

void Parser::TYPEKIND(unsigned char& types) {
		if (la->kind == 33 /* "NODE" */) {
			Get();
			types|=TypeInfo::typeNode; 
		} else if (la->kind == 34 /* "WAY" */) {
			Get();
			types|=TypeInfo::typeWay; 
		} else if (la->kind == 35 /* "AREA" */) {
			Get();
			types|=TypeInfo::typeArea; 
		} else if (la->kind == 36 /* "RELATION" */) {
			Get();
			types|=TypeInfo::typeRelation; 
		} else SynErr(65);
}

void Parser::TYPEOPTION(TypeInfo& typeInfo) {
		switch (la->kind) {
		case 45 /* "ROUTE" */: {
			ROUTE(typeInfo);
			break;
		}
		case 37 /* "INDEX_LOC" */: {
			Get();
			typeInfo.SetIndexAsLocation(true); 
			break;
		}
		case 38 /* "INDEX_REGION" */: {
			Get();
			typeInfo.SetIndexAsRegion(true); 
			break;
		}
		case 39 /* "INDEX_POI" */: {
			Get();
			typeInfo.SetIndexAsPOI(true); 
			break;
		}
		case 40 /* "OPTIMIZE_LOW_ZOOM" */: {
			Get();
			typeInfo.SetOptimizeLowZoom(true); 
			break;
		}
		case 41 /* "IGNORE" */: {
			Get();
			typeInfo.SetIgnore(true); 
			break;
		}
		case 42 /* "MULTIPOLYGON" */: {
			Get();
			typeInfo.SetMultipolygon(true); 
			break;
		}
		case 43 /* "PIN_WAY" */: {
			Get();
			typeInfo.SetPinWay(true); 
			break;
		}
		case 44 /* "IGNORESEALAND" */: {
			Get();
			typeInfo.SetIgnoreSeaLand(true); 
			break;
		}
		default: SynErr(66); break;
		}
}

void Parser::ROUTE(TypeInfo& typeInfo) {
		Expect(45 /* "ROUTE" */);
		Expect(29 /* "[" */);
		if (la->kind == 46 /* "FOOT" */) {
			Get();
			typeInfo.CanRouteFoot(true);
			
		}
		if (la->kind == 47 /* "BICYCLE" */) {
			Get();
			typeInfo.CanRouteBicycle(true);
			
		}
		if (la->kind == 48 /* "CAR" */) {
			Get();
			typeInfo.CanRouteCar(true);
			
		}
		Expect(31 /* "]" */);
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
	maxT = 49;

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

	static bool set[3][51] = {
		{T,x,x,x, T,x,T,T, x,x,x,T, T,x,x,x, x,x,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,T,x, x,T,T,T, T,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,T,T,T, T,T,T,T, T,T,x,x, x,x,x}
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
			case 6: s = coco_string_create("\"GRADES\" expected"); break;
			case 7: s = coco_string_create("\"SURFACE\" expected"); break;
			case 8: s = coco_string_create("\"GRADE\" expected"); break;
			case 9: s = coco_string_create("\"{\" expected"); break;
			case 10: s = coco_string_create("\"}\" expected"); break;
			case 11: s = coco_string_create("\"TYPES\" expected"); break;
			case 12: s = coco_string_create("\"TYPE\" expected"); break;
			case 13: s = coco_string_create("\"=\" expected"); break;
			case 14: s = coco_string_create("\"(\" expected"); break;
			case 15: s = coco_string_create("\")\" expected"); break;
			case 16: s = coco_string_create("\"OR\" expected"); break;
			case 17: s = coco_string_create("\"OPTIONS\" expected"); break;
			case 18: s = coco_string_create("\"TAGS\" expected"); break;
			case 19: s = coco_string_create("\"TAG\" expected"); break;
			case 20: s = coco_string_create("\"AND\" expected"); break;
			case 21: s = coco_string_create("\"!\" expected"); break;
			case 22: s = coco_string_create("\"<\" expected"); break;
			case 23: s = coco_string_create("\"<=\" expected"); break;
			case 24: s = coco_string_create("\"==\" expected"); break;
			case 25: s = coco_string_create("\"!=\" expected"); break;
			case 26: s = coco_string_create("\">=\" expected"); break;
			case 27: s = coco_string_create("\">\" expected"); break;
			case 28: s = coco_string_create("\"IN\" expected"); break;
			case 29: s = coco_string_create("\"[\" expected"); break;
			case 30: s = coco_string_create("\",\" expected"); break;
			case 31: s = coco_string_create("\"]\" expected"); break;
			case 32: s = coco_string_create("\"EXISTS\" expected"); break;
			case 33: s = coco_string_create("\"NODE\" expected"); break;
			case 34: s = coco_string_create("\"WAY\" expected"); break;
			case 35: s = coco_string_create("\"AREA\" expected"); break;
			case 36: s = coco_string_create("\"RELATION\" expected"); break;
			case 37: s = coco_string_create("\"INDEX_LOC\" expected"); break;
			case 38: s = coco_string_create("\"INDEX_REGION\" expected"); break;
			case 39: s = coco_string_create("\"INDEX_POI\" expected"); break;
			case 40: s = coco_string_create("\"OPTIMIZE_LOW_ZOOM\" expected"); break;
			case 41: s = coco_string_create("\"IGNORE\" expected"); break;
			case 42: s = coco_string_create("\"MULTIPOLYGON\" expected"); break;
			case 43: s = coco_string_create("\"PIN_WAY\" expected"); break;
			case 44: s = coco_string_create("\"IGNORESEALAND\" expected"); break;
			case 45: s = coco_string_create("\"ROUTE\" expected"); break;
			case 46: s = coco_string_create("\"FOOT\" expected"); break;
			case 47: s = coco_string_create("\"BICYCLE\" expected"); break;
			case 48: s = coco_string_create("\"CAR\" expected"); break;
			case 49: s = coco_string_create("??? expected"); break;
			case 50: s = coco_string_create("this symbol not expected in OST"); break;
			case 51: s = coco_string_create("this symbol not expected in GRADES"); break;
			case 52: s = coco_string_create("this symbol not expected in TYPES"); break;
			case 53: s = coco_string_create("this symbol not expected in TAGS"); break;
			case 54: s = coco_string_create("this symbol not expected in GRADE"); break;
			case 55: s = coco_string_create("this symbol not expected in TYPE"); break;
			case 56: s = coco_string_create("this symbol not expected in TAG"); break;
			case 57: s = coco_string_create("invalid TAGBOOLCOND"); break;
			case 58: s = coco_string_create("invalid TAGBINCOND"); break;
			case 59: s = coco_string_create("invalid TAGLESSCOND"); break;
			case 60: s = coco_string_create("invalid TAGLESSEQUALCOND"); break;
			case 61: s = coco_string_create("invalid TAGEQUALSCOND"); break;
			case 62: s = coco_string_create("invalid TAGNOTEQUALSCOND"); break;
			case 63: s = coco_string_create("invalid TAGGREATERCOND"); break;
			case 64: s = coco_string_create("invalid TAGGREATEREQUALCOND"); break;
			case 65: s = coco_string_create("invalid TYPEKIND"); break;
			case 66: s = coco_string_create("invalid TYPEOPTION"); break;

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

