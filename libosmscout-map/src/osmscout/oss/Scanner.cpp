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

#include <osmscout/oss/Scanner.h>

namespace osmscout {
namespace oss {


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
  int len = 0;
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
    std::cerr << "--- buffer out of bounds access, position: " <<  value << std::endl;
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
	maxT = 89;
	noSym = 89;
	int i;
	for (i = 65; i <= 90; ++i) start.set(i, 1);
	for (i = 95; i <= 95; ++i) start.set(i, 1);
	for (i = 97; i <= 122; ++i) start.set(i, 1);
	for (i = 48; i <= 57; ++i) start.set(i, 16);
	start.set(35, 33);
	start.set(64, 12);
	start.set(34, 14);
	start.set(61, 19);
	start.set(59, 20);
	start.set(44, 21);
	start.set(123, 22);
	start.set(125, 23);
	start.set(33, 24);
	start.set(91, 25);
	start.set(93, 26);
	start.set(45, 27);
	start.set(58, 28);
	start.set(60, 29);
	start.set(46, 30);
	start.set(40, 31);
	start.set(41, 32);
		start.set(Buffer::EoF, -1);
	keywords.set("OSS", 7);
	keywords.set("END", 8);
	keywords.set("FLAG", 9);
	keywords.set("ORDER", 12);
	keywords.set("WAYS", 13);
	keywords.set("GROUP", 14);
	keywords.set("SYMBOL", 16);
	keywords.set("POLYGON", 17);
	keywords.set("RECTANGLE", 20);
	keywords.set("x", 21);
	keywords.set("CIRCLE", 22);
	keywords.set("CONST", 23);
	keywords.set("IF", 24);
	keywords.set("COLOR", 26);
	keywords.set("MAG", 27);
	keywords.set("UINT", 28);
	keywords.set("STYLE", 29);
	keywords.set("FEATURE", 32);
	keywords.set("PATH", 33);
	keywords.set("TYPE", 34);
	keywords.set("ONEWAY", 36);
	keywords.set("BRIDGE", 37);
	keywords.set("TUNNEL", 38);
	keywords.set("SIZE", 39);
	keywords.set("m", 40);
	keywords.set("mm", 41);
	keywords.set("px", 43);
	keywords.set("NODE", 45);
	keywords.set("TEXT", 47);
	keywords.set("ICON", 49);
	keywords.set("WAY", 50);
	keywords.set("SHIELD", 51);
	keywords.set("AREA", 52);
	keywords.set("color", 53);
	keywords.set("dash", 54);
	keywords.set("gapColor", 55);
	keywords.set("displayWidth", 56);
	keywords.set("width", 57);
	keywords.set("displayOffset", 58);
	keywords.set("offset", 59);
	keywords.set("cap", 60);
	keywords.set("joinCap", 61);
	keywords.set("endCap", 62);
	keywords.set("priority", 63);
	keywords.set("pattern", 64);
	keywords.set("patternMinMag", 65);
	keywords.set("borderColor", 66);
	keywords.set("borderWidth", 67);
	keywords.set("borderDash", 68);
	keywords.set("label", 69);
	keywords.set("style", 70);
	keywords.set("size", 71);
	keywords.set("scaleMag", 72);
	keywords.set("autoSize", 73);
	keywords.set("position", 74);
	keywords.set("backgroundColor", 75);
	keywords.set("shieldSpace", 76);
	keywords.set("symbol", 77);
	keywords.set("symbolSpace", 78);
	keywords.set("name", 79);
	keywords.set("butt", 80);
	keywords.set("round", 81);
	keywords.set("square", 82);
	keywords.set("normal", 83);
	keywords.set("emphasize", 84);
	keywords.set("lighten", 85);
	keywords.set("darken", 88);


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
  TokenRef t = std::make_shared<Token>();

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
			(ch >= 9 && ch <= 10) || ch == 13 || ch == ' '
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
			if ((ch >= '0' && ch <= '9')) {AddCh(); goto case_3;}
			else {goto case_0;}
		case 3:
			case_3:
			recEnd = pos; recKind = 3;
			if ((ch >= '0' && ch <= '9')) {AddCh(); goto case_3;}
			else {t->kind = 3; break;}
		case 4:
			case_4:
			if ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f')) {AddCh(); goto case_5;}
			else {goto case_0;}
		case 5:
			case_5:
			if ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f')) {AddCh(); goto case_6;}
			else {goto case_0;}
		case 6:
			case_6:
			if ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f')) {AddCh(); goto case_7;}
			else {goto case_0;}
		case 7:
			case_7:
			if ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f')) {AddCh(); goto case_8;}
			else {goto case_0;}
		case 8:
			case_8:
			if ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f')) {AddCh(); goto case_9;}
			else {goto case_0;}
		case 9:
			case_9:
			recEnd = pos; recKind = 4;
			if ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f')) {AddCh(); goto case_10;}
			else {t->kind = 4; break;}
		case 10:
			case_10:
			if ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f')) {AddCh(); goto case_11;}
			else {goto case_0;}
		case 11:
			case_11:
			{t->kind = 4; break;}
		case 12:
			if ((ch >= 'A' && ch <= 'Z') || ch == '_' || (ch >= 'a' && ch <= 'z')) {AddCh(); goto case_13;}
			else {goto case_0;}
		case 13:
			case_13:
			recEnd = pos; recKind = 5;
			if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'Z') || ch == '_' || (ch >= 'a' && ch <= 'z')) {AddCh(); goto case_13;}
			else {t->kind = 5; break;}
		case 14:
			case_14:
			if (ch <= '!' || (ch >= '#' && ch <= '[') || (ch >= ']' && ch <= 65535)) {AddCh(); goto case_14;}
			else if (ch == '"') {AddCh(); goto case_15;}
			else if (ch == 92) {AddCh(); goto case_17;}
			else {goto case_0;}
		case 15:
			case_15:
			{t->kind = 6; break;}
		case 16:
			case_16:
			recEnd = pos; recKind = 2;
			if ((ch >= '0' && ch <= '9')) {AddCh(); goto case_16;}
			else if (ch == '.') {AddCh(); goto case_2;}
			else {t->kind = 2; break;}
		case 17:
			case_17:
			if (ch <= '!' || (ch >= '#' && ch <= '[') || (ch >= ']' && ch <= 65535)) {AddCh(); goto case_14;}
			else if (ch == 92) {AddCh(); goto case_17;}
			else if (ch == '"') {AddCh(); goto case_18;}
			else {goto case_0;}
		case 18:
			case_18:
			recEnd = pos; recKind = 6;
			if (ch <= '!' || (ch >= '#' && ch <= '[') || (ch >= ']' && ch <= 65535)) {AddCh(); goto case_14;}
			else if (ch == '"') {AddCh(); goto case_15;}
			else if (ch == 92) {AddCh(); goto case_17;}
			else {t->kind = 6; break;}
		case 19:
			{t->kind = 10; break;}
		case 20:
			{t->kind = 11; break;}
		case 21:
			{t->kind = 15; break;}
		case 22:
			{t->kind = 18; break;}
		case 23:
			{t->kind = 19; break;}
		case 24:
			{t->kind = 25; break;}
		case 25:
			{t->kind = 30; break;}
		case 26:
			{t->kind = 31; break;}
		case 27:
			{t->kind = 35; break;}
		case 28:
			{t->kind = 42; break;}
		case 29:
			{t->kind = 44; break;}
		case 30:
			{t->kind = 46; break;}
		case 31:
			{t->kind = 86; break;}
		case 32:
			{t->kind = 87; break;}
		case 33:
			recEnd = pos; recKind = 48;
			if ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f')) {AddCh(); goto case_4;}
			else {t->kind = 48; break;}

  }
  AppendVal(t);

  return t;
}

// get the next token (possibly a token already seen during peeking)
TokenRef Scanner::Scan() {
  if (!tokens->next) {
    return pt = tokens = NextToken();
  } else {
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


