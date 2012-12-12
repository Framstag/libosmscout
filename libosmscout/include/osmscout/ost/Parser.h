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


#if !defined(osmscout_ost_PARSER_H)
#define osmscout_ost_PARSER_H

#include <cassert>
#include <iostream>

#include <osmscout/TypeConfig.h>


#include <osmscout/TypeConfig.h>

#include <osmscout/ost/Scanner.h>

namespace osmscout {
namespace ost {


class Errors
{
public:
  class Err
  {
  public:
    enum Type {
      Symbol,
      Error,
      Warning,
      Exception
    };

  public:
    Type        type;
    int         line;
    int         column;
    std::string text;
  };

public:
  std::list<Err> errors;
  bool           hasErrors;

public:
  Errors();
  void SynErr(int line, int col, int n);
  void Error(int line, int col, const char *s);
  void Warning(int line, int col, const char *s);
  void Warning(const char *s);
  void Exception(const char *s);
};

class Parser
{
private:
	enum {
		_EOF=0,
		_ident=1,
		_number=2,
		_string=3
	};
	int maxT;

  TokenRef dummyToken;
  int errDist;
  int minErrDist;

  void SynErr(int n);
  void Get();
  void Expect(int n);
  bool StartOf(int s);
  void ExpectWeak(int n, int follow);
  bool WeakSeparator(int n, int syFol, int repFol);

  Scanner *scanner;

  TokenRef t;  // last recognized token
  TokenRef la; // lookahead token

  TypeConfig& config;

public:
  Errors  *errors;

std::string Destring(const char* str)
{
  std::string result(str);

  if (result.length()>=2 &&
      result[0]=='"' &&
      result[result.length()-1]=='"') {
    result=result.substr(1,result.length()-2);
  }

  return result;
}



  Parser(Scanner *scanner,
         TypeConfig& config);
  ~Parser();
  void SemErr(const char* msg);

	void OST();
	void TYPES();
	void TAGS();
	void TYPE();
	void IDENT(std::string& value);
	void TYPEKINDS(unsigned char& types);
	void CONDITION(Condition*& condition);
	void TYPEOPTIONS(TypeInfo& typeInfo);
	void TAG();
	void ANDCOND(Condition*& condition);
	void BOOLCOND(Condition*& condition);
	void BINARYCOND(Condition*& condition);
	void EXISTSCOND(Condition*& condition);
	void EQUALSCOND(const std::string& tagName,Condition*& condition);
	void NOTEQUALSCOND(const std::string& tagName,Condition*& condition);
	void ISINCOND(const std::string& tagName,Condition*& condition);
	void TYPEKIND(unsigned char& types);
	void TYPEOPTION(TypeInfo& typeInfo);

  void Parse();
};

} // namespace
} // namespace


#endif // !defined(COCO_PARSER_H__)

