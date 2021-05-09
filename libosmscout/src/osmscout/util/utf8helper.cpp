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
#include <osmscout/util/utf8helper_charmap.h>

#include <string>
#include <cstring>

namespace utf8helper
{

/**
 * Parse and transform an UTF8 string
 *
 * See https://tools.ietf.org/html/rfc3629
 *
 * UTF8 encoding standard provides backward compatibility with the ASCII string.
 * Illegal sequences will be discarded for security reason: see RFC 3629 #10.
 * For each valid sequence, It picks the corresponding predefined table, If none
 * has been defined, then the sequence is dumped as it is. Otherwise, the functor
 * is called for the found character and the new sequence will be dumped instead.
 *
 * The functor has 2 arguments:
 *   - The character matching the current code point
 *   - The context, it is the category of the preceding sequence
 * It must return the new code point to be dumped instead, or NullCodepoint to
 * discard the sequence.
 *
 * @param text
 *    Text to get processed
 * @param func
 *    The functor of the desired transformation
 * @return
 *    Processed text
 */
static std::string parse(const std::string& text, codepoint (*func)(const character*, int context)){
  size_t len = text.size();
  std::string result;
  result.reserve(len);
  const byte * b = reinterpret_cast<const byte*>(text.c_str());
  const byte * e = b + len;

  int context = IsSpace | IsBreaker;

  for (; *b; ++b) {
    size_t rb = e - b;
    // 1 byte
    // RFC 3629:#4: Valid UTF-8 matches the following syntax
    // 00-7F
    if (*b < 0x80) {
      const character* c = &charmap_us7ascii[*b];
      codepoint u = func(c, context);
      if (u != NullCodepoint) {
        if (u > 0xffffff)
          result.push_back(0xff & (u >> 24));
        if (u > 0xffff)
          result.push_back(0xff & (u >> 16));
        if (u > 0xff)
          result.push_back(0xff & (u >> 8));
        result.push_back(0xff & u);
        context = c->category;
      }
    }
    else if (*b < 0xc2) {
      // invalid codepoint
      context = None;
    }
    // 2 bytes
    // RFC 3629:#4: Valid UTF-8 matches the following syntax
    // C2-DF 80-BF
    else if (*b < 0xe0 && rb > 1) {
      byte b0 = *b;
      if (*(++b) > 0x7f && *b < 0xc0) {
        const character* pg = pagemap_16[b0 - 0xc0];
        if (pg) {
          const character* c = &pg[*b - 0x80];
          codepoint u = func(c, context);
          if (u != NullCodepoint) {
            if (u > 0xffffff)
              result.push_back(0xff & (u >> 24));
            if (u > 0xffff)
              result.push_back(0xff & (u >> 16));
            if (u > 0xff)
              result.push_back(0xff & (u >> 8));
            result.push_back(0xff & u);
            context = c->category;
          }
        } else {
          result.push_back(b0);
          result.push_back(*b);
          context = None;
        }
      } else {
        // invalid codepoint: restart
        --b;
        context = None;
      }
    }
    // 3 bytes
    // RFC 3629:#4: Valid UTF-8 matches the following syntax
    // E0    A0-BF 80-BF
    // E1-EC 80-BF 80-BF
    // ED    80-9F 80-BF
    // EE-EF 80-BF 80-BF
    // RFC 3629:#6: [EF,BB,BF] is BOM on start, else ZERO WIDTH NO-BREAK SPACE
    else if (*b < 0xf0 && rb > 2) {
      byte b0 = *b;
      byte b1 = *(++b);
      if ( (b0 == 0xe0 && b1 > 0x9f && b1 < 0xc0) ||
           (b0 > 0xe0 && b0 < 0xed && b1 > 0x7f && b1 < 0xc0) ||
           (b0 == 0xed && b1 > 0x7f && b1 < 0xa0) ||
           (b0 > 0xed && b1 > 0x7f && b1 < 0xc0) ) {
        if (*(++b) > 0x7f && *b < 0xc0) {
          const character* pg = nullptr;
          switch (b0) {
          case 0xe1:
            pg = pagemap_24_e1[b1 - 0x80];
            break;
          case 0xe2:
            pg = pagemap_24_e2[b1 - 0x80];
            break;
          default:
            // no code page defined
            break;
          }
          if (pg) {
            const character* c = &pg[*b - 0x80];
            codepoint u = func(c, context);
            if (u != NullCodepoint) {
              if (u > 0xffffff)
                result.push_back(0xff & (u >> 24));
              if (u > 0xffff)
                result.push_back(0xff & (u >> 16));
              if (u > 0xff)
                result.push_back(0xff & (u >> 8));
              result.push_back(0xff & u);
              context = c->category;
            }
          } else {
            result.push_back(b0);
            result.push_back(b1);
            result.push_back(*b);
            context = None;
          }
        } else {
          // invalid codepoint: restart
          b -= 2;
          context = None;
        }
      } else {
        // invalid codepoint: restart
        --b;
        context = None;
      }
    }
    // 4 bytes
    // RFC 3629:#4: Valid UTF-8 matches the following syntax
    // F0    90-BF 80-BF 80-BF
    // F1-F3 80-BF 80-BF 80-BF
    // F4    80-8F 80-BF 80-BF
    else if (*b < 0xf5 && rb > 3) {
      byte b0 = *b;
      byte b1 = *(++b);
      if ( (b0 == 0xf0 && b1 > 0x8f && b1 < 0xc0) ||
           (b0 > 0xf0 && b0 < 0xf4 && b1 > 0x7f && b1 < 0xc0) ||
           (b0 == 0xf4 && b1 > 0x7f && b1 < 0x90) ) {
        if (*(++b) > 0x7f && *b < 0xc0) {
          byte b2 = *b;
          if (*(++b) > 0x7f && *b < 0xc0) {
            const character* pg = nullptr;
            switch (b0) {
            case 0xf0:
              switch (b1) {
              case 0x90:
                pg = pagemap_32_f0_90[b2 - 0x80];
                break;
              case 0x9e:
                pg = pagemap_32_f0_9e[b2 - 0x80];
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
              const character* c = &pg[*b - 0x80];
              codepoint u = func(c, context);
              if (u != NullCodepoint) {
                if (u > 0xffffff)
                  result.push_back(0xff & (u >> 24));
                if (u > 0xffff)
                  result.push_back(0xff & (u >> 16));
                if (u > 0xff)
                  result.push_back(0xff & (u >> 8));
                result.push_back(0xff & u);
                context = c->category;
              }
            } else {
              result.push_back(b0);
              result.push_back(b1);
              result.push_back(b2);
              result.push_back(*b);
              context = None;
            }
          } else {
            // invalid codepoint: restart
            b -= 3;
            context = None;
          }
        } else {
          // invalid codepoint: restart
          b -= 2;
          context = None;
        }
      } else {
        // invalid codepoint: restart
        --b;
        context = None;
      }
    }
    else {
      // invalid codepoint
      context = None;
    }
  }
  return result;
}

static codepoint toUpper(const character* ch, int context) {
  (void)context;
  return ch->upper;
}

static codepoint toLower(const character* ch, int context) {
  (void)context;
  return ch->lower;
}

static codepoint normalize(const character* ch, int context) {
  if ((IsSpace & ch->category & context))
    return NullCodepoint;
  if ((IsBreaker & ch->category & context))
    return NullCodepoint;
  if ((ch->category & IsControl))
    return NullCodepoint;
  return ch->lower;
}

static codepoint capitalize(const character* ch, int context) {
  if ((context & (IsSpace | IsBreaker | IsControl)))
    return ch->upper;
  return ch->lower;
}

static codepoint transliterate(const character* ch, int context) {
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

std::string UTF8ToUpper(const std::string& text) {
  return parse(text, toUpper);
}

std::string UTF8ToLower(const std::string& text) {
  return parse(text, toLower);
}

std::string UTF8Normalize(const std::string& text) {
  return parse(text, normalize);
}

std::string UTF8Capitalize(const std::string& text) {
  return parse(text, capitalize);
}

std::string UTF8Transliterate(const std::string& text) {
  return parse(text, transliterate);
}

}
