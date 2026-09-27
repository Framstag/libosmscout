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

#include <string>

#include <osmscout/util/StringMatcher.h>

#include <catch2/catch_test_macros.hpp>

//
// Word matching across separators
//

TEST_CASE("Separator in the name, words in the query")
{
  // The candidate joins "Hilpert" and "Theater" with a hyphen; the query spells
  // them apart. The name carries a further word the query does not, so the
  // result is a partial match.
  osmscout::StringMatcherTransliterateToken matcher("Hilpert Theater");

  REQUIRE(matcher.Match("Heinz-Hilpert-Theater Lünen")==osmscout::StringMatcher::partialMatch);
}

TEST_CASE("Separator in the query, words in the name")
{
  osmscout::StringMatcherTransliterateToken matcher("Cafe-Central");

  REQUIRE(matcher.Match("Café Central")==osmscout::StringMatcher::match);
}

TEST_CASE("Slash-separated name")
{
  osmscout::StringMatcherTransliterateToken matcher("Sparkasse Hagen Herdecke");

  REQUIRE(matcher.Match("Sparkasse Hagen/Herdecke")==osmscout::StringMatcher::match);
}

TEST_CASE("En-dash separated name")
{
  osmscout::StringMatcherTransliterateToken matcher("Bahnhof Straße");

  REQUIRE(matcher.Match("Bahnhof\u2013Straße")==osmscout::StringMatcher::match);
}

TEST_CASE("Transliteration across the separator")
{
  SECTION("Sharp s")
  {
    osmscout::StringMatcherTransliterateToken matcher("Bahnhofs Strasse");

    REQUIRE(matcher.Match("Bahnhofs-Straße")==osmscout::StringMatcher::match);
  }

  SECTION("Diacritics")
  {
    osmscout::StringMatcherTransliterateToken matcher("Gunnemann Hof");

    REQUIRE(matcher.Match("Günnemann-Hof")==osmscout::StringMatcher::match);
  }
}

TEST_CASE("Complete word coverage of a separator-joined name")
{
  osmscout::StringMatcherTransliterateToken matcher("August Warkner Platz");

  REQUIRE(matcher.Match("August-Warkner-Platz")==osmscout::StringMatcher::match);
}

TEST_CASE("A word run at the end of the name matches partially")
{
  osmscout::StringMatcherTransliterateToken matcher("Theater Lünen");

  REQUIRE(matcher.Match("Heinz-Hilpert-Theater Lünen")==osmscout::StringMatcher::partialMatch);
}

//
// Boundaries: only consecutive words in query order
//

TEST_CASE("Reordered words do not match")
{
  osmscout::StringMatcherTransliterateToken matcher("Theater Hilpert");

  REQUIRE(matcher.Match("Heinz-Hilpert-Theater Lünen")==osmscout::StringMatcher::noMatch);
}

TEST_CASE("An interrupted word run does not match")
{
  // "Theater" sits between the query's two words in the candidate name.
  osmscout::StringMatcherTransliterateToken matcher("Hilpert Lünen");

  REQUIRE(matcher.Match("Heinz-Hilpert-Theater Lünen")==osmscout::StringMatcher::noMatch);
}

TEST_CASE("Words joined without a separator do not match")
{
  // Word-internal splitting is not part of the rule: "Bahnhof Straße" is not
  // "Bahnhofstraße".
  osmscout::StringMatcherTransliterateToken matcher("Bahnhof Straße");

  REQUIRE(matcher.Match("Bahnhofstraße")==osmscout::StringMatcher::noMatch);
}

TEST_CASE("Unrelated text does not match")
{
  osmscout::StringMatcherTransliterateToken matcher("Trallafitti");

  REQUIRE(matcher.Match("Am Birkenbaum")==osmscout::StringMatcher::noMatch);
}

//
// Unchanged substring behavior
//

TEST_CASE("Substring matching is unchanged")
{
  /*
   * The first section is the discriminating one: the word matching alone would
   * report a non-match, because the candidate's second word is "Birkenbaum"
   * where the pattern's is "Birken", and the candidate offers no other word run.
   * The result can therefore only come from the transliterating substring match,
   * which pins that the substring decision is taken first and returned unchanged.
   */
  SECTION("Word prefix of the candidate")
  {
    osmscout::StringMatcherTransliterateToken matcher("Am Birken");

    REQUIRE(matcher.Match("Am Birkenbaum")==osmscout::StringMatcher::partialMatch);
  }

  SECTION("Inner substring of the candidate")
  {
    osmscout::StringMatcherTransliterateToken matcher("Birkenbaum");

    REQUIRE(matcher.Match("Am Birkenbaum")==osmscout::StringMatcher::partialMatch);
  }

  SECTION("Whole candidate")
  {
    osmscout::StringMatcherTransliterateToken matcher("Am Birkenbaum");

    REQUIRE(matcher.Match("Am Birkenbaum")==osmscout::StringMatcher::match);
  }

  SECTION("Single word against a separator-joined candidate")
  {
    osmscout::StringMatcherTransliterateToken matcher("Hilpert");

    REQUIRE(matcher.Match("Heinz-Hilpert-Theater")==osmscout::StringMatcher::partialMatch);
  }
}

TEST_CASE("A single-word pattern keeps the substring decision")
{
  // The word matching declines patterns of one word, so this result can only
  // come from the substring match. Removing that match turns the result into
  // noMatch, which fails this assertion.
  osmscout::StringMatcherTransliterateToken matcher("Test");

  REQUIRE(matcher.Match("Test-Theater")==osmscout::StringMatcher::partialMatch);
}

/*
 * The evaluation order is pinned by the cases above: "Substring matching is
 * unchanged" and "A single-word pattern keeps the substring decision" both
 * assert a result the word matching alone cannot produce, so a matcher that
 * consulted the words first would fail them. That the word matching does not
 * even run after a substring hit - the fast path's cost claim - is deliberately
 * not asserted: observing single allocations needs the global allocation
 * functions of this test binary replaced, and that neither links against the
 * memory sanitizer runtime nor is observed at all on MinGW, where the counter
 * stayed at zero for both paths.
 */
