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


#if !defined(osmscout_ost_SCANNER_H)
#define osmscout_ost_SCANNER_H

#include <string>
#include <map>
#include <memory>

#include <osmscout/system/SystemTypes.h>

namespace osmscout {
namespace ost {


#define COCO_WCHAR_MAX 65535

// string handling, wide character
extern char* coco_string_create(const char* value);
extern char* coco_string_create(const char* value, int startIndex, int length);
extern void  coco_string_delete(char* &data);

class Token;

typedef std::shared_ptr<Token> TokenRef;

class Token
{
public:
  int      kind;    // token kind
  int      pos;     // token position in the source text (starting at 0)
  int      charPos; // token position in characters in the source text (starting at 0)
  int      col;     // token column (starting at 1)
  int      line;    // token line (starting at 1)
  char*    val;     // token value
  TokenRef next;    // ML 2005-03-11 Peek tokens are kept in linked list

  Token();
  ~Token();
};

class Buffer
{
private:
  unsigned char *buf; // input buffer
  size_t bufLen;      // length of buffer
  int    bufPos;      // current position in buffer

public:
  static const int EoF = COCO_WCHAR_MAX + 1;

  Buffer(const unsigned char* buf, size_t len);
  virtual ~Buffer();

  int Read();
  int Peek();
  //wchar_t* GetString(int beg, int end);
  int GetPos();
  void SetPos(int value);
};

//-----------------------------------------------------------------------------------
// StartStates  -- maps characters to start states of tokens
//-----------------------------------------------------------------------------------
class StartStates
{
private:
  std::map<int,int> map;

public:
  StartStates()
  {
    // no code
  }

  virtual ~StartStates()
  {
    // no code
  }

  void set(int key, int val)
  {
    map[key]=val;
  }

  int state(int key) {
    std::map<int,int>::const_iterator iter=map.find(key);

    if (iter!=map.end()) {
      return iter->second;
    }
    else {
      return 0;
    }
  }
};

//-------------------------------------------------------------------------------------------
// KeywordMap  -- maps strings to integers (identifiers to keyword kinds)
//-------------------------------------------------------------------------------------------
class KeywordMap
{
private:
  std::map<std::string,int> map;

public:
  KeywordMap()
  {
    // no code
  }

  virtual ~KeywordMap()
  {
    // no code
  }

  void set(const char* key, int val)
  {
    map[std::string(key)]=val;
  }

  int get(const char* key, int defaultVal)
  {
    std::map<std::string,int>::const_iterator iter=map.find(std::string(key));

    if (iter!=map.end()) {
      return iter->second;
    }
    else {
      return defaultVal;
    }
  }
};

class Scanner
{
private:
  unsigned char EOL;
  int           eofSym;
  int           noSym;
  int           maxT;
  StartStates   start;
  KeywordMap    keywords;

  TokenRef      t;          // current token
  char          *tval;      // text of current token
  size_t        tvalLength; // length of text of current token
  size_t        tlen;       // length of current token

  TokenRef      tokens;     // list of tokens already peeked (first token is a dummy)
  TokenRef      pt;         // current peek token

  int           ch;         // current input character

  int           pos;        // byte position of current character
  int           charPos;    // position by unicode characters starting with 0
  int           line;       // line number of current character
  int           col;        // column number of current character
  int           oldEols;    // EOLs that appeared in a comment;

  TokenRef CreateToken();
  void AppendVal(TokenRef& t);

  void Init();
  void NextCh();
  void AddCh();
	bool Comment0();
	bool Comment1();

  TokenRef NextToken();

public:
  Buffer *buffer;   // scanner buffer

  Scanner(const unsigned char* buf, size_t len);
  ~Scanner();
  TokenRef Scan();
  void SetScannerBehindT();
  TokenRef Peek();
  void ResetPeek();

}; // end Scanner

} // namespace
} // namespace


#endif

