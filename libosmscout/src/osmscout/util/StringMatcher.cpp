/*
  This source is part of the libosmscout library
  Copyright (C) 2017  Tim Teulings

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

#include <osmscout/util/StringMatcher.h>

#include <osmscout/util/String.h>

#include <algorithm>

namespace osmscout {

  StringMatcherCI::StringMatcherCI(const std::string& pattern)
    : pattern(UTF8StringToUpper(pattern))
  {
    // no code
  }

  StringMatcher::Result StringMatcherCI::Match(const std::string& text) const
  {
    auto transformedText=UTF8StringToUpper(text);
    auto pos            =transformedText.find(pattern);

    if (pos==std::string::npos) {
      return noMatch;
    }

    if (pos==0 && pattern.length()==transformedText.length()) {
      return match;
    }

    return partialMatch;
  }

  StringMatcherRef StringMatcherCIFactory::CreateMatcher(const std::string& pattern) const
  {
    return std::make_shared<StringMatcherCI>(pattern);
  }

  StringMatcherTransliterate::StringMatcherTransliterate(const std::string& patternArg)
        : pattern(UTF8StringToUpper(patternArg)),
          transliteratedPattern(UTF8StringToUpper(UTF8Transliterate(pattern)))
  {
  }

  StringMatcher::Result StringMatcherTransliterate::Match(const std::string& text) const
  {
    auto transformedText=UTF8StringToUpper(text);
    auto pos            =transformedText.find(pattern);

    if (pos==std::string::npos) {
      if (transliteratedPattern.empty()){
        return noMatch;
      }

      // Transliteration can introduce characters whose case does not follow the
      // case of the surrounding text: the sharp s transliterates to lower case
      // "ss" while text and pattern are upper cased here, so both transliterated
      // forms are compared case-normalized (otherwise "straße" would never match
      // "strasse" and vice versa).
      auto transliterated=UTF8StringToUpper(UTF8Transliterate(transformedText));
      pos=transliterated.find(transliteratedPattern);

      if (pos==std::string::npos) {
        return noMatch;
      }
      if (pos==0 && transliteratedPattern.length()==transliterated.length()) {
        return match;
      }

      return partialMatch;
    }

    if (pos==0 && pattern.length()==transformedText.length()) {
      return match;
    }

    return partialMatch;
  }

  StringMatcherRef StringMatcherTransliterateFactory::CreateMatcher(const std::string& pattern) const
  {
    return std::make_shared<StringMatcherTransliterate>(pattern);
  }

  namespace {

    /**
     * Appends the words of [text] to [words], separated by whitespace, comma,
     * hyphen, slash, backslash and the en/em dash. Repeated, leading and trailing
     * separators produce no empty word. [text] is expected to be case-folded and
     * transliterated by the caller.
     *
     * UTF-8 continuation bytes never collide with the single-byte separators, so
     * the scan can stay byte-oriented while advancing by the character width of
     * every multi-byte character.
     */
    void SplitIntoWords(const std::string& text,
                        std::vector<std::string>& words)
    {
      std::string word;
      size_t      pos=0;

      while (pos<text.length()) {
        auto   byte=static_cast<unsigned char>(text[pos]);
        size_t width=1;
        bool   separator=false;

        if ((byte&0xe0)==0xc0) {
          width=2;
        }
        else if ((byte&0xf0)==0xe0) {
          width=3;
        }
        else if ((byte&0xf8)==0xf0) {
          width=4;
        }

        if (width==1) {
          separator=byte<=' ' ||
                     byte==',' ||
                     byte=='-' ||
                     byte=='/' ||
                     byte=='\\';
        }
        else if (width==3 && byte==0xe2 && pos+2<text.length() &&
                 static_cast<unsigned char>(text[pos+1])==0x80 &&
                 (static_cast<unsigned char>(text[pos+2])==0x93 ||
                  static_cast<unsigned char>(text[pos+2])==0x94)) {
          // En dash (U+2013) and em dash (U+2014)
          separator=true;
        }

        if (separator) {
          if (!word.empty()) {
            words.push_back(word);
            word.clear();
          }
        }
        else {
          word.append(text,pos,width);
        }

        pos+=width;
      }

      if (!word.empty()) {
        words.push_back(word);
      }
    }
  }

  StringMatcherTransliterateToken::StringMatcherTransliterateToken(const std::string& patternArg)
  : base(StringMatcherTransliterateFactory().CreateMatcher(patternArg))
  {
    SplitIntoWords(UTF8StringToUpper(UTF8Transliterate(patternArg)),
                   patternWords);
  }

  StringMatcher::Result StringMatcherTransliterateToken::Match(const std::string& text) const
  {
    // Fast path: the transliterating substring match decides first, so every
    // match that existed before is reported with the same quality.
    StringMatcher::Result result=base->Match(text);

    if (result!=noMatch) {
      return result;
    }

    // A pattern of one word cannot gain from word matching: a single word either
    // occurs in the candidate (fast path) or does not occur at all.
    if (patternWords.size()<2) {
      return noMatch;
    }

    std::vector<std::string> nameWords;

    SplitIntoWords(UTF8StringToUpper(UTF8Transliterate(text)),
                   nameWords);

    if (nameWords.size()<patternWords.size()) {
      return noMatch;
    }

    for (size_t start=0; start+patternWords.size()<=nameWords.size(); start++) {
      size_t wordIndex=0;

      while (wordIndex<patternWords.size() &&
             nameWords[start+wordIndex]==patternWords[wordIndex]) {
        wordIndex++;
      }

      if (wordIndex==patternWords.size()) {
        // Match only when the run covers every word of the candidate.
        return nameWords.size()==patternWords.size() ? match : partialMatch;
      }
    }

    return noMatch;
  }

  StringMatcherRef StringMatcherTransliterateTokenFactory::CreateMatcher(const std::string& pattern) const
  {
    return std::make_shared<StringMatcherTransliterateToken>(pattern);
  }

}
