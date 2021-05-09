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

#ifndef UTF8HELPER_H
#define UTF8HELPER_H

#include <osmscout/util/utf8helper_charmap.h>

#include <string>
#include <vector>

namespace utf8helper
{

extern std::string UTF8ToUpper(const std::string& text);

extern std::string UTF8ToLower(const std::string& text);

extern std::string UTF8Normalize(const std::string& text);

extern std::string UTF8Capitalize(const std::string& text);

extern std::string UTF8Transliterate(const std::string& text);

/**
 * @brief functor implements desired transformation of the character
 * It has 2 arguments:
 * - The character struct matching the current code point
 * - The context, it is the category of the preceding sequence
 * It must return the new code point to be dumped instead, or NullCodepoint to
 * discard the sequence.
 */
using Transform = codepoint (*)(const character*, int context);

extern codepoint TransformNop(const character*, int);
extern codepoint TransformUpper(const character*, int);
extern codepoint TransformLower(const character*, int);
extern codepoint TransformCapitalize(const character*, int);
extern codepoint TransformNormalize(const character*, int);
extern codepoint TransformTransliterate(const character*, int);

/**
 * @brief Parse and transform an UTF8 string
 *
 * See https://tools.ietf.org/html/rfc3629
 *
 * UTF8 encoding standard provides backward compatibility with the ASCII string.
 * Illegal sequences will be discarded for security reason: see RFC 3629 #10.
 * For each valid sequence, It picks the corresponding predefined table, If none
 * has been defined, then the sequence is dumped as it is. Otherwise, the functor
 * is called for the found character and the new sequence will be dumped instead.
 */
struct Parser {
  enum Exit { Done = 0, Continue, Error };
  utf8helper::Transform func;
  Exit (*run)(Parser*, byte);
  int context;
  byte b[3];
  char u_size;
  codepoint u;
  explicit Parser(utf8helper::Transform func);
  Parser(const Parser&) = default;
  void Reset();
};


class UTF8String {
  using storage_type = std::vector<codepoint>;
public:
  /**
   * @brief Contructs empty UTF8String object with transformation nop.
   */
  UTF8String();

  /**
   * @brief Contructs UTF8String object containing the text.
   */
  explicit UTF8String(const std::string& text);

  /**
   * @brief Contructs empty UTF8String object with the specified transformation.
   */
  explicit UTF8String(utf8helper::Transform);

  /**
   * @brief Contructs UTF8String object containing the text with specified transformation.
   */
  UTF8String(const std::string& text, utf8helper::Transform);

  UTF8String(const UTF8String&) = default;

  /**
   * @return true if the string has no characters,
   *         otherwise returns false
   */
  bool Empty() const { return store.empty(); }

  /**
   * @brief Return the number of character of this string.
   * @return count of characters
   */
  size_t Size() const { return store.size(); }

  /**
   * @brief Returns the number of bytes of this string.
   * @return count of bytes
   */
  size_t RawSize() const { return rawSize; }

  /**
   * @brief Attempts to allocate memory for at least sz characters.
   * @param sz
   */
  void Reserve(size_t sz) { store.reserve(sz); }

  /**
   * @brief Clears the contents of the string and makes it null.
   */
  void Clear();

  /**
   * @brief Push one more byte into this string.
   *        A character is added to this string on each valid sequence.
   * @param cc
   * @see operator \<\<(char)
   */
  void WriteByte(char cc);

  /**
   * A convenience method for writeByte(char)
   * @param cc
   * @return this
   * @see writeByte(char)
   */
  UTF8String& operator<<(char cc) {
    WriteByte(cc);
    return *this;
  }

  /**
   * @brief Returns the character at the position pos
   * @param pos
   * @return the character
   */
  codepoint operator[](size_t pos) const { return store[pos]; }

  /**
   * @brief Returns the characters contained in this string.
   * @return the list of characters
   */
  const storage_type& Data() const { return store; }

  /**
   * @brief Remove n character from the position pos
   * @param pos
   * @param n
   * @return false if pos is out of bound, otherwise returns true
   */
  bool Remove(size_t pos, size_t n = 1);

  /**
   * @brief Insert the character u at the position pos
   * @param pos
   * @param u
   * @param transform function = nop
   * @return false if pos is out of bound or the character is invalid,
   *         otherwise returns true
   */
  bool Insert(size_t pos, codepoint u, utf8helper::Transform func = utf8helper::TransformNop);

  /**
   * @brief Insert characters at the position pos
   * @param pos
   * @param data
   * @param transform function = nop
   * @return the number of valid character inserted
   */
  size_t Insert(size_t pos, const storage_type& data, utf8helper::Transform func = utf8helper::TransformNop);

  /**
   * @brief Add character to the string
   * @param u
   * @param transform function = nop
   * @return this
   */
  UTF8String& Append(codepoint u, utf8helper::Transform func = utf8helper::TransformNop);

  /**
   * @brief Add characters to the string
   * @param data
   * @param transform function = nop
   * @return this
   */
  UTF8String& Append(const storage_type& data, utf8helper::Transform func = utf8helper::TransformNop);

  /**
   * @brief Apply transformation to the string
   * @param transform function
   * @return this
   */
  UTF8String& Transform(utf8helper::Transform);

  /**
   * @brief Transform and assign the text to the string
   * @param text
   * @param transform function
   * @return this
   */
  UTF8String& Transform(const std::string& text, utf8helper::Transform);

  /**
   * @brief Returns a std::string object with the data contained in this string.
   * @return the std::string object
   */
  std::string ToStdString() const;

  /**
   * @brief Returns a std::string object with the substring of n characters of this string,
   *        starting at the position pos.
   * @param pos
   * @param n
   * @return the std::string object
   */
  std::string Substr(size_t pos, size_t n = (-1)) const;

private:
  Parser parser;
  storage_type store;
  size_t rawSize;
};

}

#endif // UTF8HELPER_H
