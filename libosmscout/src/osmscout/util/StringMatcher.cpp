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
          transliteratedPattern(UTF8Transliterate(pattern))
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

      auto transliterated=UTF8Transliterate(transformedText);
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

}
