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

#include <string.h>
#include <stdlib.h>

#include <iostream>

#include <osmscout/ost/Scanner.h>

namespace osmscout {
namespace ost {


// string handling, wide character

char* coco_string_create(const char* value) {
  char* data;
  size_t len = 0;
  if (value) { len = strlen(value); }
  data = new char[len + 1];
  strncpy(data, value, len);
  data[len] = 0;
  return data;
}

char* coco_string_create(const char *value , int startIndex, size_t length) {
  size_t len = 0;
  char* data;

  if (value) { len = length; }
  data = new char[len + 1];
  strncpy(data, &(value[startIndex]), len);
  data[len] = 0;

  return data;
}

void coco_string_delete(char* &data) {
  delete [] data;
  data = NULL;
}

Token::Token()
: kind(0),
  pos(0),
  col(0),
  line(0),
  val(NULL),
  next(NULL)
{
 // no code
}

Token::~Token()
{
  delete [] val;
}

Buffer::Buffer(const unsigned char* buf, size_t len)
{
  this->buf = new unsigned char[len];
  memcpy(this->buf, buf, len*sizeof(unsigned char));
  bufLen = len;
  bufPos = 0;
}

Buffer::~Buffer()
{
  delete [] buf;
}

int Buffer::Peek()
{
  size_t curPos = GetPos();
  int ch = Read();
  SetPos(curPos);
  return ch;
}

size_t Buffer::GetPos()
{
  return bufPos;
}

void Buffer::SetPos(size_t value) {
  if (value > bufLen) {
    std::cerr << "--- buffer out of bounds access, position: " << value << std::endl;
    exit(1);
  }

  bufPos = value;
}

int Buffer::Read()
{
  if (bufPos < bufLen) {
    return buf[bufPos++];
  } else {
    return EoF;
  }
}

Scanner::Scanner(const unsigned char* buf, size_t len) {
  buffer = new Buffer(buf, len);
  Init();
}

Scanner::~Scanner() {
  delete [] tval;
  delete buffer;
}

void Scanner::Init() {
  EOL    = '\n';
  eofSym = 0;
	maxT = 57;
	noSym = 57;
	int i;
	for (i = 65; i <= 90; ++i) start.set(i, 1);
	for (i = 97; i <= 106; ++i) start.set(i, 1);
	for (i = 108; i <= 122; ++i) start.set(i, 1);
	for (i = 48; i <= 57; ++i) start.set(i, 2);
	start.set(34, 3);
	start.set(61, 21);
	start.set(107, 22);
	start.set(123, 9);
	start.set(125, 10);
	start.set(58, 11);
	start.set(40, 12);
	start.set(41, 13);
	start.set(44, 14);
	start.set(33, 23);
	start.set(60, 24);
	start.set(62, 25);
	start.set(91, 19);
	start.set(93, 20);
		start.set(Buffer::EoF, -1);
	keywords.set("OST", 4);
	keywords.set("END", 5);
	keywords.set("MAX", 6);
	keywords.set("SPEEDS", 7);
	keywords.set("SPEED", 8);
	keywords.set("GRADES", 11);
	keywords.set("SURFACE", 12);
	keywords.set("GRADE", 13);
	keywords.set("FEATURES", 16);
	keywords.set("FEATURE", 17);
	keywords.set("DESC", 18);
	keywords.set("TYPES", 20);
	keywords.set("TYPE", 21);
	keywords.set("IGNORE", 22);
	keywords.set("OR", 25);
	keywords.set("AND", 27);
	keywords.set("IN", 35);
	keywords.set("EXISTS", 38);
	keywords.set("NODE", 39);
	keywords.set("WAY", 40);
	keywords.set("AREA", 41);
	keywords.set("RELATION", 42);
	keywords.set("LOCATION", 43);
	keywords.set("ADMIN_REGION", 44);
	keywords.set("ADDRESS", 45);
	keywords.set("POI", 46);
	keywords.set("OPTIMIZE_LOW_ZOOM", 47);
	keywords.set("MULTIPOLYGON", 48);
	keywords.set("PIN_WAY", 49);
	keywords.set("MERGE_AREAS", 50);
	keywords.set("IGNORESEALAND", 51);
	keywords.set("PATH", 52);
	keywords.set("FOOT", 53);
	keywords.set("BICYCLE", 54);
	keywords.set("CAR", 55);
	keywords.set("GROUP", 56);


  tvalLength = 128;
  tval = new char[tvalLength]; // text of current token

  pos = -1; line = 1; col = 0;
  oldEols = 0;
  NextCh();
  if (ch == 0xEF) { // check optional byte order mark for UTF-8
    NextCh(); int ch1 = ch;
    NextCh(); int ch2 = ch;
    if (ch1 != 0xBB || ch2 != 0xBF) {
      std::cerr << "Illegal byte order mark at start of file" << std::endl;
      exit(1);
    }
    NextCh();
  }


  pt = tokens = CreateToken(); // first token is a dummy
}

void Scanner::NextCh() {
  if (oldEols > 0) {
    ch = EOL;
    oldEols--;
  }
  else {
    pos = buffer->GetPos();
    ch = buffer->Read(); col++;
    // replace isolated '\r' by '\n' in order to make
    // eol handling uniform across Windows, Unix and Mac
    if (ch == '\r' && buffer->Peek() != '\n') {
      ch = EOL;
    }
    if (ch == EOL) {
      line++;
      col = 0;
    }
  }

}

void Scanner::AddCh() {
  if (tlen >= tvalLength) {
    tvalLength *= 2;
    char *newBuf = new char[tvalLength];
    memcpy(newBuf, tval, tlen*sizeof(char));
    delete [] tval;
    tval = newBuf;
  }
  if (ch != Buffer::EoF) {
		tval[tlen++] = ch;
    NextCh();
  }
}


bool Scanner::Comment0() {
	int level = 1, pos0 = pos, line0 = line, col0 = col, charPos0 = charPos;
	NextCh();
	if (ch == '/') {
		NextCh();
		for(;;) {
			if (ch == 10) {
				level--;
				if (level == 0) { oldEols = line - line0; NextCh(); return true; }
				NextCh();
			} else if (ch == buffer->EoF) return false;
			else NextCh();
		}
	} else {
		buffer->SetPos(pos0); NextCh(); line = line0; col = col0; charPos = charPos0;
	}
	return false;
}

bool Scanner::Comment1() {
	int level = 1, pos0 = pos, line0 = line, col0 = col, charPos0 = charPos;
	NextCh();
	if (ch == '*') {
		NextCh();
		for(;;) {
			if (ch == '*') {
				NextCh();
				if (ch == '/') {
					level--;
					if (level == 0) { oldEols = line - line0; NextCh(); return true; }
					NextCh();
				}
			} else if (ch == '/') {
				NextCh();
				if (ch == '*') {
					level++; NextCh();
				}
			} else if (ch == buffer->EoF) return false;
			else NextCh();
		}
	} else {
		buffer->SetPos(pos0); NextCh(); line = line0; col = col0; charPos = charPos0;
	}
	return false;
}


TokenRef Scanner::CreateToken() {
  TokenRef t=std::make_shared<Token>();

  return t;
}

void Scanner::AppendVal(TokenRef& t) {
  delete [] t->val;
  t->val = new char[tlen+1];

  strncpy(t->val, tval, tlen);
  t->val[tlen] = '\0';
}

TokenRef Scanner::NextToken() {
  while (ch == ' ' ||
			(ch >= 9 && ch <= 10) || ch == 13
  ) NextCh();
	if ((ch == '/' && Comment0()) || (ch == '/' && Comment1())) return NextToken();
  int recKind = noSym;
  size_t recEnd = pos;

  t = CreateToken();
  t->pos = pos; t->col = col; t->line = line; t->charPos = charPos;
  int state = start.state(ch);
  tlen = 0; AddCh();

  switch (state) {
  case -1: { t->kind = eofSym; break; } // NextCh already done
  case 0: {
    case_0:
    if (recKind != noSym) {
      tlen = recEnd - t->pos;
      SetScannerBehindT();
    }

    t->kind = recKind; break;
  } // NextCh already done
		case 1:
			case_1:
			recEnd = pos; recKind = 1;
			if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'Z') || ch == '_' || (ch >= 'a' && ch <= 'z')) {AddCh(); goto case_1;}
			else {t->kind = 1; char *literal = coco_string_create(tval, 0, tlen); t->kind = keywords.get(literal, t->kind); coco_string_delete(literal); break;}
		case 2:
			case_2:
			recEnd = pos; recKind = 2;
			if ((ch >= '0' && ch <= '9')) {AddCh(); goto case_2;}
			else {t->kind = 2; break;}
		case 3:
			case_3:
			if (ch <= '!' || (ch >= '#' && ch <= '[') || (ch >= ']' && ch <= 65535)) {AddCh(); goto case_3;}
			else if (ch == '"') {AddCh(); goto case_4;}
			else if (ch == 92) {AddCh(); goto case_5;}
			else {goto case_0;}
		case 4:
			case_4:
			{t->kind = 3; break;}
		case 5:
			case_5:
			if (ch <= '!' || (ch >= '#' && ch <= '[') || (ch >= ']' && ch <= 65535)) {AddCh(); goto case_3;}
			else if (ch == 92) {AddCh(); goto case_5;}
			else if (ch == '"') {AddCh(); goto case_6;}
			else {goto case_0;}
		case 6:
			case_6:
			recEnd = pos; recKind = 3;
			if (ch <= '!' || (ch >= '#' && ch <= '[') || (ch >= ']' && ch <= 65535)) {AddCh(); goto case_3;}
			else if (ch == '"') {AddCh(); goto case_4;}
			else if (ch == 92) {AddCh(); goto case_5;}
			else {t->kind = 3; break;}
		case 7:
			case_7:
			if (ch == 'h') {AddCh(); goto case_8;}
			else {goto case_0;}
		case 8:
			case_8:
			{t->kind = 10; break;}
		case 9:
			{t->kind = 14; break;}
		case 10:
			{t->kind = 15; break;}
		case 11:
			{t->kind = 19; break;}
		case 12:
			{t->kind = 23; break;}
		case 13:
			{t->kind = 24; break;}
		case 14:
			{t->kind = 26; break;}
		case 15:
			case_15:
			{t->kind = 30; break;}
		case 16:
			case_16:
			{t->kind = 31; break;}
		case 17:
			case_17:
			{t->kind = 32; break;}
		case 18:
			case_18:
			{t->kind = 33; break;}
		case 19:
			{t->kind = 36; break;}
		case 20:
			{t->kind = 37; break;}
		case 21:
			recEnd = pos; recKind = 9;
			if (ch == '=') {AddCh(); goto case_16;}
			else {t->kind = 9; break;}
		case 22:
			recEnd = pos; recKind = 1;
			if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'Z') || ch == '_' || (ch >= 'a' && ch <= 'l') || (ch >= 'n' && ch <= 'z')) {AddCh(); goto case_1;}
			else if (ch == 'm') {AddCh(); goto case_26;}
			else {t->kind = 1; char *literal = coco_string_create(tval, 0, tlen); t->kind = keywords.get(literal, t->kind); coco_string_delete(literal); break;}
		case 23:
			recEnd = pos; recKind = 28;
			if (ch == '=') {AddCh(); goto case_17;}
			else {t->kind = 28; break;}
		case 24:
			recEnd = pos; recKind = 29;
			if (ch == '=') {AddCh(); goto case_15;}
			else {t->kind = 29; break;}
		case 25:
			recEnd = pos; recKind = 34;
			if (ch == '=') {AddCh(); goto case_18;}
			else {t->kind = 34; break;}
		case 26:
			case_26:
			recEnd = pos; recKind = 1;
			if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'Z') || ch == '_' || (ch >= 'a' && ch <= 'z')) {AddCh(); goto case_1;}
			else if (ch == '/') {AddCh(); goto case_7;}
			else {t->kind = 1; char *literal = coco_string_create(tval, 0, tlen); t->kind = keywords.get(literal, t->kind); coco_string_delete(literal); break;}

  }
  AppendVal(t);

  return t;
}

// get the next token (possibly a token already seen during peeking)
TokenRef Scanner::Scan() {
  if (!tokens->next) {
    return pt = tokens = NextToken();
  }
  else {
    pt = tokens = tokens->next;
    return tokens;
  }
}

void Scanner::SetScannerBehindT()
{
  buffer->SetPos(t->pos);
  NextCh();
  line = t->line; col = t->col; charPos = t->charPos;
  for (size_t i = 0; i < tlen; i++) NextCh();
}


// peek for the next token, ignore pragmas
TokenRef Scanner::Peek() {
  do {
    if (!pt->next) {
      pt->next = NextToken();
    }
    pt = pt->next;
  } while (pt->kind > maxT); // skip pragmas

  return pt;
}

// make sure that peeking starts at the current scan position
void Scanner::ResetPeek() {
  pt = tokens;
}

} // namespace
} // namespace


