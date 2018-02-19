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
		while (!(la->kind == _EOF || la->kind == 4 /* "OST" */)) {SynErr(58); Get();}
		Expect(4 /* "OST" */);
		if (la->kind == 6 /* "MAX" */) {
			MAXSPEEDS();
		}
		if (la->kind == 11 /* "GRADES" */) {
			GRADES();
		}
		if (la->kind == 16 /* "FEATURES" */) {
			FEATURES();
		}
		if (la->kind == 20 /* "TYPES" */) {
			TYPES();
		}
		Expect(5 /* "END" */);
}

void Parser::MAXSPEEDS() {
		while (!(la->kind == _EOF || la->kind == 6 /* "MAX" */)) {SynErr(59); Get();}
		Expect(6 /* "MAX" */);
		Expect(7 /* "SPEEDS" */);
		MAXSPEED();
		while (la->kind == 8 /* "SPEED" */) {
			MAXSPEED();
		}
}

void Parser::GRADES() {
		while (!(la->kind == _EOF || la->kind == 11 /* "GRADES" */)) {SynErr(60); Get();}
		Expect(11 /* "GRADES" */);
		GRADE();
		while (la->kind == 12 /* "SURFACE" */) {
			GRADE();
		}
}

void Parser::FEATURES() {
		while (!(la->kind == _EOF || la->kind == 16 /* "FEATURES" */)) {SynErr(61); Get();}
		Expect(16 /* "FEATURES" */);
		FEATURE();
		while (la->kind == 17 /* "FEATURE" */) {
			FEATURE();
		}
}

void Parser::TYPES() {
		while (!(la->kind == _EOF || la->kind == 20 /* "TYPES" */)) {SynErr(62); Get();}
		Expect(20 /* "TYPES" */);
		TYPE();
		while (la->kind == 21 /* "TYPE" */) {
			TYPE();
		}
}

void Parser::MAXSPEED() {
		std::string alias;
		size_t      speed;
		
		while (!(la->kind == _EOF || la->kind == 8 /* "SPEED" */)) {SynErr(63); Get();}
		Expect(8 /* "SPEED" */);
		STRING(alias);
		Expect(9 /* "=" */);
		UINT(speed);
		if (speed>0 && speed<256) {
		 config.RegisterMaxSpeedAlias(alias,
		                              (uint8_t)speed);
		}
		else {
		    std::string e="Speed value not in the allowed range (]0..255] km/h)";
		
		    SemErr(e.c_str());
		}
		
		if (la->kind == 10 /* "km/h" */) {
			Get();
		}
}

void Parser::STRING(std::string& value) {
		Expect(_string);
		value=Destring(t->val);
		
}

void Parser::UINT(size_t& value) {
		Expect(_number);
		if (!StringToNumber(t->val,value)) {
		 std::string e="Cannot parse number '"+std::string(t->val)+"'";
		
		 SemErr(e.c_str());
		}
		
}

void Parser::GRADE() {
		while (!(la->kind == _EOF || la->kind == 12 /* "SURFACE" */)) {SynErr(64); Get();}
		Expect(12 /* "SURFACE" */);
		Expect(13 /* "GRADE" */);
		size_t grade;
		
		UINT(grade);
		Expect(14 /* "{" */);
		while (la->kind == _string) {
			std::string surface;
			
			STRING(surface);
			if (grade>=1 && grade<=5) {
			 config.RegisterSurfaceToGradeMapping(surface,
			                                      grade);
			}
			else {
			 std::string e="Not a valid grade level: "+std::to_string(grade);
			
			 SemErr(e.c_str());
			}
			
		}
		Expect(15 /* "}" */);
}

void Parser::FEATURE() {
		while (!(la->kind == _EOF || la->kind == 17 /* "FEATURE" */)) {SynErr(65); Get();}
		Expect(17 /* "FEATURE" */);
		std::string          featureName;
		osmscout::FeatureRef feature;
		
		IDENT(featureName);
		feature=config.GetFeature(featureName);
		
		if (!feature) {
		 std::string e="Unknown feature '"+featureName+"'";
		
		 SemErr(e.c_str());
		
		 // Avoid NULL-pointer further on
		 feature=std::make_shared<osmscout::NameFeature>();
		}
		
		if (la->kind == 18 /* "DESC" */) {
			FEATUREDESCS(*feature);
		}
}

void Parser::IDENT(std::string& value) {
		Expect(_ident);
		value=t->val;
		
}

void Parser::FEATUREDESCS(Feature& feature) {
		Expect(18 /* "DESC" */);
		std::string languageCode;
		std::string description;
		
		IDENT(languageCode);
		Expect(19 /* ":" */);
		STRING(description);
		feature.AddDescription(languageCode,description); 
		while (la->kind == _ident) {
			IDENT(languageCode);
			Expect(19 /* ":" */);
			STRING(description);
			feature.AddDescription(languageCode,description); 
		}
}

void Parser::TYPE() {
		std::string     name;
		TagConditionRef condition=NULL;
		TypeInfoRef     typeInfo;
		unsigned char   types;
		
		while (!(la->kind == _EOF || la->kind == 21 /* "TYPE" */)) {SynErr(66); Get();}
		Expect(21 /* "TYPE" */);
		IDENT(name);
		typeInfo=std::make_shared<TypeInfo>(name); 
		if (la->kind == 22 /* "IGNORE" */) {
			Get();
			typeInfo->SetIgnore(true); 
		}
		Expect(9 /* "=" */);
		TYPEKINDS(types);
		Expect(23 /* "(" */);
		TAGCONDITION(condition);
		Expect(24 /* ")" */);
		typeInfo->AddCondition(types,condition); 
		while (la->kind == 25 /* "OR" */) {
			Get();
			TYPEKINDS(types);
			Expect(23 /* "(" */);
			TAGCONDITION(condition);
			Expect(24 /* ")" */);
			typeInfo->AddCondition(types,condition); 
		}
		if (la->kind == 14 /* "{" */) {
			Get();
			if (la->kind == _ident) {
				TYPEFEATURE(*typeInfo);
				while (la->kind == 26 /* "," */) {
					Get();
					TYPEFEATURE(*typeInfo);
				}
			}
			Expect(15 /* "}" */);
		}
		if (StartOf(1)) {
			TYPEOPTIONS(*typeInfo);
		}
		if (la->kind == 56 /* "GROUP" */) {
			GROUPS(*typeInfo);
		}
		if (la->kind == 18 /* "DESC" */) {
			TYPEDESCS(*typeInfo);
		}
		config.RegisterType(typeInfo);
		
}

void Parser::TYPEKINDS(unsigned char& types) {
		types=0;
		
		TYPEKIND(types);
		while (StartOf(2)) {
			if (la->kind == 26 /* "," */) {
				Get();
			}
			TYPEKIND(types);
		}
}

void Parser::TAGCONDITION(TagConditionRef& condition) {
		std::list<TagConditionRef> conditions;
		TagConditionRef            subCond;
		
		TAGANDCOND(subCond);
		conditions.push_back(subCond); 
		while (la->kind == 25 /* "OR" */) {
			Get();
			TAGANDCOND(subCond);
			conditions.push_back(subCond); 
		}
		if (conditions.size()==1) {
		 condition=conditions.front();
		}
		else {
		 TagBoolConditionRef orCondition=std::make_shared<TagBoolCondition>(TagBoolCondition::boolOr);
		
		 for (const auto& c : conditions) {
		   orCondition->AddCondition(c);
		 }
		
		 condition=orCondition;
		}
		
}

void Parser::TYPEFEATURE(TypeInfo& typeInfo) {
		std::string name; 
		IDENT(name);
		FeatureRef feature=config.GetFeature(name);
		
		if (feature) {
		 if (!typeInfo.HasFeature(name)) {
		   typeInfo.AddFeature(feature);
		 }
		 else {
		 std::string e="Feature '"+name+"' has been already assigned to type";
		
		 SemErr(e.c_str());
		 }
		}
		else {
		 std::string e="Feature '"+name+"' is unknown";
		
		 SemErr(e.c_str());
		}
		
}

void Parser::TYPEOPTIONS(TypeInfo& typeInfo) {
		TYPEOPTION(typeInfo);
		while (StartOf(1)) {
			TYPEOPTION(typeInfo);
		}
}

void Parser::GROUPS(TypeInfo& typeInfo) {
		Expect(56 /* "GROUP" */);
		std::string groupName; 
		IDENT(groupName);
		typeInfo.AddGroup(groupName); 
		while (la->kind == 26 /* "," */) {
			Get();
			IDENT(groupName);
			typeInfo.AddGroup(groupName); 
		}
}

void Parser::TYPEDESCS(TypeInfo& typeInfo) {
		Expect(18 /* "DESC" */);
		std::string languageCode;
		std::string description;
		
		IDENT(languageCode);
		Expect(19 /* ":" */);
		STRING(description);
		typeInfo.AddDescription(languageCode,description); 
		while (la->kind == _ident) {
			IDENT(languageCode);
			Expect(19 /* ":" */);
			STRING(description);
			typeInfo.AddDescription(languageCode,description); 
		}
}

void Parser::TAGANDCOND(TagConditionRef& condition) {
		std::list<TagConditionRef> conditions;
		TagConditionRef            subCond;
		
		TAGBOOLCOND(subCond);
		conditions.push_back(subCond); 
		while (la->kind == 27 /* "AND" */) {
			Get();
			TAGBOOLCOND(subCond);
			conditions.push_back(subCond); 
		}
		if (conditions.size()==1) {
		 condition=conditions.front();
		}
		else {
		 TagBoolConditionRef andCondition=std::make_shared<TagBoolCondition>(TagBoolCondition::boolAnd);
		
		 for (const auto& c : conditions) {
		   andCondition->AddCondition(c);
		 }
		
		 condition=andCondition;
		}
		
}

void Parser::TAGBOOLCOND(TagConditionRef& condition) {
		if (la->kind == _string) {
			TAGBINCOND(condition);
		} else if (la->kind == 38 /* "EXISTS" */) {
			TAGEXISTSCOND(condition);
		} else if (la->kind == 23 /* "(" */) {
			Get();
			TAGCONDITION(condition);
			Expect(24 /* ")" */);
		} else if (la->kind == 28 /* "!" */) {
			Get();
			TAGBOOLCOND(condition);
			condition=std::make_shared<TagNotCondition>(condition); 
		} else SynErr(67);
}

void Parser::TAGBINCOND(TagConditionRef& condition) {
		std::string nameValue;
		
		Expect(_string);
		nameValue=Destring(t->val); 
		switch (la->kind) {
		case 29 /* "<" */: {
			TAGLESSCOND(nameValue,condition);
			break;
		}
		case 30 /* "<=" */: {
			TAGLESSEQUALCOND(nameValue,condition);
			break;
		}
		case 31 /* "==" */: {
			TAGEQUALSCOND(nameValue,condition);
			break;
		}
		case 32 /* "!=" */: {
			TAGNOTEQUALSCOND(nameValue,condition);
			break;
		}
		case 34 /* ">" */: {
			TAGGREATERCOND(nameValue,condition);
			break;
		}
		case 33 /* ">=" */: {
			TAGGREATEREQUALCOND(nameValue,condition);
			break;
		}
		case 35 /* "IN" */: {
			TAGISINCOND(nameValue,condition);
			break;
		}
		default: SynErr(68); break;
		}
}

void Parser::TAGEXISTSCOND(TagConditionRef& condition) {
		Expect(38 /* "EXISTS" */);
		Expect(_string);
		condition=std::make_shared<TagExistsCondition>(config.RegisterTag(Destring(t->val)));
		
}

void Parser::TAGLESSCOND(const std::string& tagName,TagConditionRef& condition) {
		std::string stringValue;
		size_t      sizeValue;
		
		Expect(29 /* "<" */);
		if (la->kind == _string) {
			STRING(stringValue);
			TagId tagId=config.RegisterTag(tagName);
			
			condition=std::make_shared<TagBinaryCondition>(tagId,operatorLess,stringValue);
			
		} else if (la->kind == _number) {
			UINT(sizeValue);
			TagId tagId=config.RegisterTag(tagName);
			
			condition=std::make_shared<TagBinaryCondition>(tagId,operatorLess,sizeValue);
			
		} else SynErr(69);
}

void Parser::TAGLESSEQUALCOND(const std::string& tagName,TagConditionRef& condition) {
		std::string stringValue;
		size_t      sizeValue;
		
		Expect(30 /* "<=" */);
		if (la->kind == _string) {
			STRING(stringValue);
			TagId tagId=config.RegisterTag(tagName);
			
			condition=std::make_shared<TagBinaryCondition>(tagId,operatorLessEqual,stringValue);
			
		} else if (la->kind == _number) {
			UINT(sizeValue);
			TagId tagId=config.RegisterTag(tagName);
			
			condition=std::make_shared<TagBinaryCondition>(tagId,operatorLessEqual,sizeValue);
			
		} else SynErr(70);
}

void Parser::TAGEQUALSCOND(const std::string& tagName,TagConditionRef& condition) {
		std::string stringValue;
		size_t      sizeValue;
		
		Expect(31 /* "==" */);
		if (la->kind == _string) {
			STRING(stringValue);
			TagId tagId=config.RegisterTag(tagName);
			
			condition=std::make_shared<TagBinaryCondition>(tagId,operatorEqual,stringValue);
			
		} else if (la->kind == _number) {
			UINT(sizeValue);
			TagId tagId=config.RegisterTag(tagName);
			
			condition=std::make_shared<TagBinaryCondition>(tagId,operatorEqual,sizeValue);
			
		} else SynErr(71);
}

void Parser::TAGNOTEQUALSCOND(const std::string& tagName,TagConditionRef& condition) {
		std::string stringValue;
		size_t      sizeValue;
		
		Expect(32 /* "!=" */);
		if (la->kind == _string) {
			STRING(stringValue);
			TagId tagId=config.RegisterTag(tagName);
			
			condition=std::make_shared<TagBinaryCondition>(tagId,operatorNotEqual,stringValue);
			
		} else if (la->kind == _number) {
			UINT(sizeValue);
			TagId tagId=config.RegisterTag(tagName);
			
			condition=std::make_shared<TagBinaryCondition>(tagId,operatorNotEqual,sizeValue);
			
		} else SynErr(72);
}

void Parser::TAGGREATERCOND(const std::string& tagName,TagConditionRef& condition) {
		std::string stringValue;
		size_t      sizeValue;
		
		Expect(34 /* ">" */);
		if (la->kind == _string) {
			STRING(stringValue);
			TagId tagId=config.RegisterTag(tagName);
			
			condition=std::make_shared<TagBinaryCondition>(tagId,operatorGreater,stringValue);
			
		} else if (la->kind == _number) {
			UINT(sizeValue);
			TagId tagId=config.RegisterTag(tagName);
			
			condition=std::make_shared<TagBinaryCondition>(tagId,operatorGreater,sizeValue);
			
		} else SynErr(73);
}

void Parser::TAGGREATEREQUALCOND(const std::string& tagName,TagConditionRef& condition) {
		std::string stringValue;
		size_t      sizeValue;
		
		Expect(33 /* ">=" */);
		if (la->kind == _string) {
			STRING(stringValue);
			TagId tagId=config.RegisterTag(tagName);
			
			condition=std::make_shared<TagBinaryCondition>(tagId,operatorGreaterEqual,stringValue);
			
		} else if (la->kind == _number) {
			UINT(sizeValue);
			TagId tagId=config.RegisterTag(tagName);
			
			condition=std::make_shared<TagBinaryCondition>(tagId,operatorGreaterEqual,sizeValue);
			
		} else SynErr(74);
}

void Parser::TAGISINCOND(const std::string& tagName,TagConditionRef& condition) {
		std::list<std::string> values;
		
		Expect(35 /* "IN" */);
		Expect(36 /* "[" */);
		Expect(_string);
		values.push_back(Destring(t->val)); 
		while (la->kind == 26 /* "," */) {
			Get();
			Expect(_string);
			values.push_back(Destring(t->val)); 
		}
		Expect(37 /* "]" */);
		TagId tagId=config.RegisterTag(tagName);
		
		if (values.size()==1) {
		 condition=std::make_shared<TagBinaryCondition>(tagId,operatorEqual,values.front());
		}
		else {
		 TagIsInConditionRef isInCondition=std::make_shared<TagIsInCondition>(tagId);
		
		 for (const auto& s : values) {
		   isInCondition->AddTagValue(s);
		 }
		
		 condition=isInCondition;
		}
		
}

void Parser::TYPEKIND(unsigned char& types) {
		if (la->kind == 39 /* "NODE" */) {
			Get();
			types|=TypeInfo::typeNode; 
		} else if (la->kind == 40 /* "WAY" */) {
			Get();
			types|=TypeInfo::typeWay; 
		} else if (la->kind == 41 /* "AREA" */) {
			Get();
			types|=TypeInfo::typeArea; 
		} else if (la->kind == 42 /* "RELATION" */) {
			Get();
			types|=TypeInfo::typeRelation; 
		} else SynErr(75);
}

void Parser::TYPEOPTION(TypeInfo& typeInfo) {
		switch (la->kind) {
		case 52 /* "PATH" */: {
			PATH(typeInfo);
			break;
		}
		case 43 /* "LOCATION" */: {
			Get();
			typeInfo.SetIndexAsLocation(true); 
			break;
		}
		case 44 /* "ADMIN_REGION" */: {
			Get();
			typeInfo.SetIndexAsRegion(true); 
			break;
		}
		case 45 /* "ADDRESS" */: {
			Get();
			typeInfo.SetIndexAsAddress(true); 
			break;
		}
		case 46 /* "POI" */: {
			Get();
			typeInfo.SetIndexAsPOI(true); 
			break;
		}
		case 47 /* "OPTIMIZE_LOW_ZOOM" */: {
			Get();
			typeInfo.SetOptimizeLowZoom(true); 
			break;
		}
		case 48 /* "MULTIPOLYGON" */: {
			Get();
			typeInfo.SetMultipolygon(true); 
			break;
		}
		case 49 /* "PIN_WAY" */: {
			Get();
			typeInfo.SetPinWay(true); 
			break;
		}
		case 50 /* "MERGE_AREAS" */: {
			Get();
			typeInfo.SetMergeAreas(true); 
			break;
		}
		case 51 /* "IGNORESEALAND" */: {
			Get();
			typeInfo.SetIgnoreSeaLand(true); 
			break;
		}
		default: SynErr(76); break;
		}
}

void Parser::PATH(TypeInfo& typeInfo) {
		Expect(52 /* "PATH" */);
		typeInfo.SetIsPath(true); 
		if (la->kind == 36 /* "[" */) {
			Get();
			if (la->kind == 53 /* "FOOT" */) {
				Get();
				typeInfo.CanRouteFoot(true);
				
			}
			if (la->kind == 54 /* "BICYCLE" */) {
				Get();
				typeInfo.CanRouteBicycle(true);
				
			}
			if (la->kind == 55 /* "CAR" */) {
				Get();
				typeInfo.CanRouteCar(true);
				
			}
			Expect(37 /* "]" */);
		}
}



void Parser::Parse()
{
  t = NULL;
  la = dummyToken = std::make_shared<Token>();
  la->val = coco_string_create("Dummy Token");
  Get();
	OST();
	Expect(0);
}

Parser::Parser(Scanner *scanner,
               TypeConfig& config)
 : config(config)
{
	maxT = 57;

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

	static bool set[3][59] = {
		{T,x,x,x, T,x,T,x, T,x,x,T, T,x,x,x, T,T,x,x, T,T,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,T, T,T,T,T, T,T,T,T, T,x,x,x, x,x,x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,T,x, x,x,x,x, x,x,x,x, x,x,x,T, T,T,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x}
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
			case 6: s = coco_string_create("\"MAX\" expected"); break;
			case 7: s = coco_string_create("\"SPEEDS\" expected"); break;
			case 8: s = coco_string_create("\"SPEED\" expected"); break;
			case 9: s = coco_string_create("\"=\" expected"); break;
			case 10: s = coco_string_create("\"km/h\" expected"); break;
			case 11: s = coco_string_create("\"GRADES\" expected"); break;
			case 12: s = coco_string_create("\"SURFACE\" expected"); break;
			case 13: s = coco_string_create("\"GRADE\" expected"); break;
			case 14: s = coco_string_create("\"{\" expected"); break;
			case 15: s = coco_string_create("\"}\" expected"); break;
			case 16: s = coco_string_create("\"FEATURES\" expected"); break;
			case 17: s = coco_string_create("\"FEATURE\" expected"); break;
			case 18: s = coco_string_create("\"DESC\" expected"); break;
			case 19: s = coco_string_create("\":\" expected"); break;
			case 20: s = coco_string_create("\"TYPES\" expected"); break;
			case 21: s = coco_string_create("\"TYPE\" expected"); break;
			case 22: s = coco_string_create("\"IGNORE\" expected"); break;
			case 23: s = coco_string_create("\"(\" expected"); break;
			case 24: s = coco_string_create("\")\" expected"); break;
			case 25: s = coco_string_create("\"OR\" expected"); break;
			case 26: s = coco_string_create("\",\" expected"); break;
			case 27: s = coco_string_create("\"AND\" expected"); break;
			case 28: s = coco_string_create("\"!\" expected"); break;
			case 29: s = coco_string_create("\"<\" expected"); break;
			case 30: s = coco_string_create("\"<=\" expected"); break;
			case 31: s = coco_string_create("\"==\" expected"); break;
			case 32: s = coco_string_create("\"!=\" expected"); break;
			case 33: s = coco_string_create("\">=\" expected"); break;
			case 34: s = coco_string_create("\">\" expected"); break;
			case 35: s = coco_string_create("\"IN\" expected"); break;
			case 36: s = coco_string_create("\"[\" expected"); break;
			case 37: s = coco_string_create("\"]\" expected"); break;
			case 38: s = coco_string_create("\"EXISTS\" expected"); break;
			case 39: s = coco_string_create("\"NODE\" expected"); break;
			case 40: s = coco_string_create("\"WAY\" expected"); break;
			case 41: s = coco_string_create("\"AREA\" expected"); break;
			case 42: s = coco_string_create("\"RELATION\" expected"); break;
			case 43: s = coco_string_create("\"LOCATION\" expected"); break;
			case 44: s = coco_string_create("\"ADMIN_REGION\" expected"); break;
			case 45: s = coco_string_create("\"ADDRESS\" expected"); break;
			case 46: s = coco_string_create("\"POI\" expected"); break;
			case 47: s = coco_string_create("\"OPTIMIZE_LOW_ZOOM\" expected"); break;
			case 48: s = coco_string_create("\"MULTIPOLYGON\" expected"); break;
			case 49: s = coco_string_create("\"PIN_WAY\" expected"); break;
			case 50: s = coco_string_create("\"MERGE_AREAS\" expected"); break;
			case 51: s = coco_string_create("\"IGNORESEALAND\" expected"); break;
			case 52: s = coco_string_create("\"PATH\" expected"); break;
			case 53: s = coco_string_create("\"FOOT\" expected"); break;
			case 54: s = coco_string_create("\"BICYCLE\" expected"); break;
			case 55: s = coco_string_create("\"CAR\" expected"); break;
			case 56: s = coco_string_create("\"GROUP\" expected"); break;
			case 57: s = coco_string_create("??? expected"); break;
			case 58: s = coco_string_create("this symbol not expected in OST"); break;
			case 59: s = coco_string_create("this symbol not expected in MAXSPEEDS"); break;
			case 60: s = coco_string_create("this symbol not expected in GRADES"); break;
			case 61: s = coco_string_create("this symbol not expected in FEATURES"); break;
			case 62: s = coco_string_create("this symbol not expected in TYPES"); break;
			case 63: s = coco_string_create("this symbol not expected in MAXSPEED"); break;
			case 64: s = coco_string_create("this symbol not expected in GRADE"); break;
			case 65: s = coco_string_create("this symbol not expected in FEATURE"); break;
			case 66: s = coco_string_create("this symbol not expected in TYPE"); break;
			case 67: s = coco_string_create("invalid TAGBOOLCOND"); break;
			case 68: s = coco_string_create("invalid TAGBINCOND"); break;
			case 69: s = coco_string_create("invalid TAGLESSCOND"); break;
			case 70: s = coco_string_create("invalid TAGLESSEQUALCOND"); break;
			case 71: s = coco_string_create("invalid TAGEQUALSCOND"); break;
			case 72: s = coco_string_create("invalid TAGNOTEQUALSCOND"); break;
			case 73: s = coco_string_create("invalid TAGGREATERCOND"); break;
			case 74: s = coco_string_create("invalid TAGGREATEREQUALCOND"); break;
			case 75: s = coco_string_create("invalid TYPEKIND"); break;
			case 76: s = coco_string_create("invalid TYPEOPTION"); break;

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

