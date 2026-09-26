#ifndef OSMSCOUT_UTIL_STRINGMATCHER_H
#define OSMSCOUT_UTIL_STRINGMATCHER_H

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

#include <memory>
#include <string>
#include <vector>

#include <osmscout/lib/CoreImportExport.h>

namespace osmscout {

  class OSMSCOUT_API StringMatcher
  {
  public:
    enum Result
    {
      noMatch,
      partialMatch,
      match
    };

  public:
    virtual ~StringMatcher() = default;

    virtual Result Match(const std::string& text) const = 0;
  };

  using StringMatcherRef = std::shared_ptr<StringMatcher>;

  class OSMSCOUT_API StringMatcherCI : public StringMatcher
  {
  private:
    std::string pattern;

  public:
    explicit StringMatcherCI(const std::string& pattern);

    Result Match(const std::string& text) const override;
  };

  class OSMSCOUT_API StringMatcherFactory
  {
  public:
    virtual ~StringMatcherFactory() = default;

    virtual StringMatcherRef CreateMatcher(const std::string& pattern) const = 0;
  };

  using StringMatcherFactoryRef = std::shared_ptr<StringMatcherFactory>;

  class OSMSCOUT_API StringMatcherCIFactory : public StringMatcherFactory
  {
  public:
    StringMatcherRef CreateMatcher(const std::string& pattern) const override;
  };

  class StringMatcherTransliterate: public StringMatcher
  {
  private:
    std::string pattern;
    std::string transliteratedPattern;

  public:
    explicit StringMatcherTransliterate(const std::string &pattern);

    StringMatcher::Result Match(const std::string &text) const override;
  };

  class OSMSCOUT_API StringMatcherTransliterateFactory : public StringMatcherFactory
  {
  public:
    StringMatcherRef CreateMatcher(const std::string& pattern) const override;
  };

  /**
   * Matches like StringMatcherTransliterate and, in addition, matches the
   * pattern's consecutive words against a consecutive run of the candidate's
   * words, so a candidate whose name joins words with a separator is found by a
   * pattern that spells those words apart (and the other way round).
   *
   * Separators are whitespace, comma, hyphen, slash, backslash and the en/em
   * dash. Only a run of words that appears in the candidate in the same order as
   * in the pattern matches; a pattern whose words are reordered, or interrupted
   * by a candidate word the pattern does not carry, does not match by this rule.
   * A run covering the whole candidate is reported as `match`, a run covering
   * only part of it as `partialMatch`.
   *
   * The transliterating substring match is evaluated first and its result is
   * returned unchanged, so this matcher only adds matches; it never removes one
   * and never reports a different quality for an existing match.
   */
  class OSMSCOUT_API StringMatcherTransliterateToken : public StringMatcher
  {
  private:
    StringMatcherRef         base;
    std::vector<std::string> patternWords;

  public:
    explicit StringMatcherTransliterateToken(const std::string& pattern);

    Result Match(const std::string& text) const override;
  };

  /**
   * Creates StringMatcherTransliterateToken matchers.
   */
  class OSMSCOUT_API StringMatcherTransliterateTokenFactory : public StringMatcherFactory
  {
  public:
    StringMatcherRef CreateMatcher(const std::string& pattern) const override;
  };

}
#endif
