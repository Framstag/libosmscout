/*
  This source is part of the libosmscout library
  Copyright (C) 2021  Jean-Luc Barriere

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

#include <osmscout/util/utf8helper.h>

#include <cstring>

namespace utf8helper
{

std::string UTF8ToUpper(const std::string& text) {
  return UTF8String(text, TransformUpper).ToStdString();
}

std::string UTF8ToLower(const std::string& text) {
  return UTF8String(text, TransformLower).ToStdString();
}

std::string UTF8Normalize(const std::string& text) {
  return UTF8String(text, TransformNormalize).ToStdString();
}

std::string UTF8Capitalize(const std::string& text) {
  return UTF8String(text, TransformCapitalize).ToStdString();
}

std::string UTF8Transliterate(const std::string& text) {
  return UTF8String(text, TransformTransliterate).ToStdString();
}


codepoint TransformNop(const character* ch, int context) {
  (void)context;
  return ch->code;
}

codepoint TransformUpper(const character* ch, int context) {
  (void)context;
  return ch->upper;
}

codepoint TransformLower(const character* ch, int context) {
  (void)context;
  return ch->lower;
}

codepoint TransformNormalize(const character* ch, int context) {
  if (((IsSpace | IsBreaker) & ch->category)) {
    if (((IsSpace | IsBreaker) & context))
      return NullCodepoint;
    return 0x20;
  }
  if ((ch->category & IsControl))
    return NullCodepoint;
  return ch->lower;
}

codepoint TransformCapitalize(const character* ch, int context) {
  if ((context & (IsSpace | IsBreaker | IsControl)))
    return ch->upper;
  return ch->lower;
}

codepoint TransformTransliterate(const character* ch, int context) {
  codepoint c = NullCodepoint;
  if ((IsSpace & context & ch->category))
    return c;
  const char* b = ch->translate;
  for (; *b; ++b) {
    c <<= 8;
    c |= static_cast<byte>(*b);
  }
  return c;
}


static inline char* _u_string(char* buf, codepoint u) {
  if (u < 0x100) {
    *(buf++) = (0xff & u);
  } else if (u < 0x10000) {
    *(buf++) = (0xff & (u >> 8));
    *(buf++) = (0xff & u);
  } else if (u < 0x1000000) {
    *(buf++) = (0xff & (u >> 16));
    *(buf++) = (0xff & (u >> 8));
    *(buf++) = (0xff & u);
  } else {
    *(buf++) = (0xff & (u >> 24));
    *(buf++) = (0xff & (u >> 16));
    *(buf++) = (0xff & (u >> 8));
    *(buf++) = (0xff & u);
  }
  *buf = 0;
  return buf;
}

static inline int _u_size(codepoint u) {
  if (u < 0x100) {
    return 1;
  } else if (u < 0x10000) {
    return 2;
  } else if (u < 0x1000000) {
    return 3;
  }
  return 4;
}

UTF8String::UTF8String()
: parser(utf8helper::TransformNop), rawSize(0) { }

UTF8String::UTF8String(const std::string& text)
: UTF8String() {
  Transform(text, parser.func);
}

UTF8String::UTF8String(utf8helper::Transform func)
: parser(func), rawSize(0) { }

UTF8String::UTF8String(const std::string& text, utf8helper::Transform func)
: parser(func), rawSize(0) {
  Transform(text, func);
}

void UTF8String::Clear() {
  parser.Reset();
  store.clear();
  rawSize = 0;
}

void UTF8String::WriteByte(char cc) {
  switch (parser.run(&parser, static_cast<byte>(cc))) {
  case Parser::Done:
    store.push_back(parser.u);
    rawSize += parser.u_size;
    break;
  case Parser::Continue:
    break;
  case Parser::Error:
    break;
  }
}

bool UTF8String::Remove(size_t pos, size_t n) {
  if (pos < Size()) {
    if (n > (store.size() - pos))
      n = store.size() - pos;
    size_t bc = 0;
    storage_type::const_iterator beg = store.cbegin() + pos;
    storage_type::const_iterator end = beg + n;
    for (storage_type::const_iterator it = beg; it != end; ++it)
      bc += _u_size(*it);
    store.erase(beg, end);
    rawSize -= bc;
    return true;
  }
  return false;
}

bool UTF8String::Insert(size_t pos, codepoint u, utf8helper::Transform func) {
  if (pos <= Size()) {
    Parser p(func);
    char buf[5];
    _u_string(buf, u);
    for (char* b = buf; *b; ++b) {
      switch (p.run(&p, static_cast<byte>(*b))) {
      case Parser::Done:
        store.insert(store.begin() + pos, p.u);
        rawSize += p.u_size;
        return true;
      case Parser::Continue:
        break;
      case Parser::Error:
        return false;
      }
    }
  }
  return false;
}

size_t UTF8String::Insert(size_t pos, const storage_type& data, utf8helper::Transform func) {
  size_t n = 0;
  if (pos <= Size()) {
    storage_type _data(data);
    for (const codepoint& u : _data) {
      if (Insert(pos + n, u, func)) {
        ++n;
      }
    }
  }
  return n;
}

UTF8String &UTF8String::Append(codepoint u, utf8helper::Transform func) {
  Parser p(func);
  char buf[5];
  _u_string(buf, u);
  for (char* b = buf; *b; ++b) {
    switch (p.run(&p, static_cast<byte>(*b))) {
    case Parser::Done:
      store.push_back(p.u);
      rawSize += p.u_size;
      return *this;
    case Parser::Continue:
      break;
    case Parser::Error:
      return *this;
    }
  }
  return *this;
}

UTF8String &UTF8String::Append(const UTF8String::storage_type &data, utf8helper::Transform func) {
  storage_type _data(data);
  for (const codepoint& u : _data) {
    Append(u, func);
  }
  return *this;
}

UTF8String& UTF8String::Transform(utf8helper::Transform func) {
  storage_type tmp(store);
  Clear();
  parser.func = func;
  for (const codepoint& u : tmp) {
    char buf[5];
    _u_string(buf, u);
    for (char* b = buf; *b; ++b) {
      WriteByte(*b);
    }
  }
  return *this;
}

UTF8String& UTF8String::Transform(const std::string& text, utf8helper::Transform func) {
  Clear();
  Reserve(text.size());
  parser.func = func;
  for (const char& cc : text) {
    WriteByte(cc);
  }
  return *this;
}

std::string UTF8String::ToStdString() const {
  std::string tmp(rawSize, '$');
  char* b = &tmp[0];
  for (const codepoint& u : store) {
    b = _u_string(b, u);
  }
  tmp.resize(b - &tmp[0]);
  return tmp;
}

std::string UTF8String::Substr(size_t pos, size_t n) const {
  if (pos < store.size()) {
    if (n > (store.size() - pos))
      n = store.size() - pos;
    std::string tmp(sizeof(codepoint) * n, '$');
    storage_type::const_iterator it = store.cbegin() + pos;
    char* b = &tmp[0];
    for (; it != store.cend() && n > 0; ++it, --n) {
      b = _u_string(b, *it);
    }
    tmp.resize(b - &tmp[0]);
    return tmp;
  }
  return "";
}


/**
 * 1 byte:
 * RFC 3629:#4: Valid UTF-8 matches the following syntax
 * 00-7F
 *
 * 2 bytes:
 * RFC 3629:#4: Valid UTF-8 matches the following syntax
 * C2-DF 80-BF
 *
 * 3 bytes:
 * RFC 3629:#4: Valid UTF-8 matches the following syntax
 * E0    A0-BF 80-BF
 * E1-EC 80-BF 80-BF
 * ED    80-9F 80-BF
 * EE-EF 80-BF 80-BF
 * RFC 3629:#6: [EF,BB,BF] is BOM on start, else ZERO WIDTH NO-BREAK SPACE
 *
 * 4 bytes:
 * RFC 3629:#4: Valid UTF-8 matches the following syntax
 * F0    90-BF 80-BF 80-BF
 * F1-F3 80-BF 80-BF 80-BF
 * F4    80-8F 80-BF 80-BF
 */
static Parser::Exit _p1_u2(Parser* p, byte bb);
static Parser::Exit _p1_u3(Parser* p, byte bb);
static Parser::Exit _p2_u3(Parser* p, byte bb);
static Parser::Exit _p1_u4(Parser* p, byte bb);
static Parser::Exit _p2_u4(Parser* p, byte bb);
static Parser::Exit _p3_u4(Parser* p, byte bb);

static Parser::Exit _p0(Parser* p, byte bb) {
  if (bb < 0x80) {
    const character* c = &charmap_us7ascii[bb];
    codepoint u = p->func(c, p->context);
    if (u == NullCodepoint)
      return Parser::Continue;
    p->u = u;
    p->context = c->category;
    p->u_size = 1;
    return Parser::Done;
  }
  else if (bb < 0xc2) {
    // invalid codepoint
    return Parser::Error;
  }
  else if (bb < 0xe0) {
    p->b[0] = bb;
    p->run = _p1_u2;
    return Parser::Continue;
  }
  else if (bb < 0xf0) {
    p->b[0] = bb;
    p->run = _p1_u3;
    return Parser::Continue;
  }
  else if (bb < 0xf5) {
    p->b[0] = bb;
    p->run = _p1_u4;
    return Parser::Continue;
  }
  else {
    // invalid codepoint
    return Parser::Error;
  }
}

static Parser::Exit _p1_u2(Parser* p, byte bb) {
  p->run = _p0;
  if (bb > 0x7f && bb < 0xc0) {
    const character* pg = pagemap_16[p->b[0] - 0xc0];
    if (pg) {
      const character* c = &pg[bb - 0x80];
      codepoint u = p->func(c, p->context);
      if (u == NullCodepoint)
        return Parser::Continue;
      p->u = u;
      p->context = c->category;
    } else {
      p->u = (p->b[0] << 8) | bb;
      p->context = None;
    }
    p->u_size = 2;
    return Parser::Done;
  }
  // invalid codepoint: restart
  return _p0(p, bb);
}

static Parser::Exit _p1_u3(Parser* p, byte bb) {
  if ( (p->b[0] == 0xe0 && bb > 0x9f && bb < 0xc0) ||
       (p->b[0] > 0xe0 && p->b[0] < 0xed && bb > 0x7f && bb < 0xc0) ||
       (p->b[0] == 0xed && bb > 0x7f && bb < 0xa0) ||
       (p->b[0] > 0xed && bb > 0x7f && bb < 0xc0) ) {
    p->b[1] = bb;
    p->run = _p2_u3;
    return Parser::Continue;
  }
  // invalid codepoint: restart
  p->run = _p0;
  return _p0(p, bb);
}

static Parser::Exit _p2_u3(Parser* p, byte bb) {
  p->run = _p0;
  if (bb > 0x7f && bb < 0xc0) {
    const character* pg = nullptr;
    switch (p->b[0]) {
    case 0xe1:
      pg = pagemap_24_e1[p->b[1] - 0x80];
      break;
    case 0xe2:
      pg = pagemap_24_e2[p->b[1] - 0x80];
      break;
    default:
      // no code page defined
      break;
    }
    if (pg) {
      const character* c = &pg[bb - 0x80];
      codepoint u = p->func(c, p->context);
      if (u == NullCodepoint)
        return Parser::Continue;
      p->u = u;
      p->context = c->category;
    } else {
      p->u = (p->b[0] << 16) | (p->b[1] << 8) | bb;
      p->context = None;
    }
    p->u_size = 3;
    return Parser::Done;
  }
  // invalid codepoint: restart
  return _p0(p, bb);
}

static Parser::Exit _p1_u4(Parser* p, byte bb) {
  if ( (p->b[0] == 0xf0 && bb > 0x8f && bb < 0xc0) ||
       (p->b[0] > 0xf0 && p->b[0] < 0xf4 && bb > 0x7f && bb < 0xc0) ||
       (p->b[0] == 0xf4 && bb > 0x7f && bb < 0x90) ) {
    p->b[1] = bb;
    p->run = _p2_u4;
    return Parser::Continue;
  }
  // invalid codepoint: restart
  p->run = _p0;
  return _p0(p, bb);
}

static Parser::Exit _p2_u4(Parser* p, byte bb) {
  if (bb > 0x7f && bb < 0xc0) {
    p->b[2] = bb;
    p->run = _p3_u4;
    return Parser::Continue;
  }
  // invalid codepoint: restart
  p->run = _p0;
  return _p0(p, bb);
}

static Parser::Exit _p3_u4(Parser* p, byte bb) {
  p->run = _p0;
  if (bb > 0x7f && bb < 0xc0) {
    const character* pg = nullptr;
    switch (p->b[0]) {
    case 0xf0:
      switch (p->b[1]) {
      case 0x90:
        pg = pagemap_32_f0_90[p->b[2] - 0x80];
        break;
      case 0x9e:
        pg = pagemap_32_f0_9e[p->b[2] - 0x80];
        break;
      default:
        // no code page defined
        break;
      }
      break;
    default:
      // no code page defined
      break;
    }
    if (pg) {
      const character* c = &pg[bb - 0x80];
      codepoint u = p->func(c, p->context);
      if (u == NullCodepoint)
        return Parser::Continue;
      p->u = u;
      p->context = c->category;
    } else {
      p->u = (p->b[0] << 24) | (p->b[1] << 16) | (p->b[2] << 8) | bb;
      p->context = None;
    }
    p->u_size = 4;
    return Parser::Done;
  }
  // invalid codepoint: restart
  return _p0(p, bb);
}

Parser::Parser(Transform func) : func(func) {
  Reset();
}

void Parser::Reset() {
  run = _p0;
  context = IsSpace|IsBreaker;
  u_size = 0;
  u = NullCodepoint;
}

}
