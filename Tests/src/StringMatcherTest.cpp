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

#include <cstdlib>
#include <new>
#include <string>

#include <osmscout/util/StringMatcher.h>

#include <catch2/catch_test_macros.hpp>

/*
 * Sanitizer runtimes provide their own operator new/delete, which collide with
 * the counter's definitions at link time (MemorySanitizer's libclang_rt.msan_cxx
 * does), so the counter is compiled out there - the same guard the other
 * allocation-counting tests use.
 */
#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__)
#define STRINGMATCHER_HAVE_ALLOCATION_COUNTER 0
#elif defined(__has_feature)
#if __has_feature(memory_sanitizer) || __has_feature(address_sanitizer) || __has_feature(thread_sanitizer)
#define STRINGMATCHER_HAVE_ALLOCATION_COUNTER 0
#else
#define STRINGMATCHER_HAVE_ALLOCATION_COUNTER 1
#endif
#else
#define STRINGMATCHER_HAVE_ALLOCATION_COUNTER 1
#endif

namespace {

  /*
   * Allocation counter, used to pin the cost claim of the word matching: a
   * substring hit must not pay for splitting the candidate into words. The
   * count is comparative (same candidate text, same pattern length) because the
   * transliterating base matcher allocates for its case-folded copies in both
   * paths.
   *
   * The counter replaces the global operator new/delete, so it only observes the
   * allocations of the test executable itself. Where the library is built as a
   * shared library and its runtime owns the allocation operators (the MinGW DLL
   * build; the PE/COFF format binds imports to the runtime that provided them,
   * while ELF and Mach-O let the executable's definition win), the matcher
   * allocates through that runtime and the counter stays at zero - which the cost
   * test below reports as "not measurable" and skips, instead of reading it as
   * "the matcher does not allocate at all".
   */

  bool          counting=false;
  unsigned long allocations=0;

  size_t CountAllocations(const osmscout::StringMatcher& matcher,
                          const std::string& text)
  {
    allocations=0;
    counting=true;

    osmscout::StringMatcher::Result result=matcher.Match(text);

    counting=false;

    REQUIRE(result!=osmscout::StringMatcher::noMatch);

    return allocations;
  }
}

#if STRINGMATCHER_HAVE_ALLOCATION_COUNTER

void* operator new(std::size_t size)
{
  if (counting) {
    allocations++;
  }

  if (size==0) {
    size=1;
  }

  void* memory=std::malloc(size);

  if (memory==nullptr) {
    throw std::bad_alloc();
  }

  return memory;
}

void operator delete(void* memory) noexcept
{
  std::free(memory);
}

void operator delete(void* memory,
                     std::size_t /*size*/) noexcept
{
  std::free(memory);
}

void* operator new[](std::size_t size)
{
  if (counting) {
    allocations++;
  }

  if (size==0) {
    size=1;
  }

  void* memory=std::malloc(size);

  if (memory==nullptr) {
    throw std::bad_alloc();
  }

  return memory;
}

void operator delete[](void* memory) noexcept
{
  std::free(memory);
}

void operator delete[](void* memory,
                       std::size_t /*size*/) noexcept
{
  std::free(memory);
}

#endif

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

//
// Cost: the word matching only runs when the substring match misses
//

TEST_CASE("A single-word pattern keeps the substring decision")
{
  // The word matching declines patterns of one word, so this result can only
  // come from the substring match. Removing that match turns the result into
  // noMatch, which fails this assertion.
  osmscout::StringMatcherTransliterateToken matcher("Test");

  REQUIRE(matcher.Match("Test-Theater")==osmscout::StringMatcher::partialMatch);
}

TEST_CASE("A substring hit does not split the candidate into words")
{
  // Long enough that the case-folded copies use the heap, so the comparison
  // measures the matcher's own work and not small-string-buffer effects.
  const std::string candidate="Heinz-Hilpert-Theater an der Kurt-Schumacher-Strasse in Luenen";

  osmscout::StringMatcherTransliterateToken fastPath("Hilpert-Theater");
  osmscout::StringMatcherTransliterateToken wordPath("Hilpert Theater");

#if STRINGMATCHER_HAVE_ALLOCATION_COUNTER
  size_t fastPathAllocations=CountAllocations(fastPath,
                                              candidate);
  size_t wordPathAllocations=CountAllocations(wordPath,
                                              candidate);

  if (fastPathAllocations==0 && wordPathAllocations==0) {
    // Both paths allocate for the case-folded copies, so a zero count for both
    // means the counter is blind here (the library allocates through its own
    // runtime), not that the matcher is free of allocations.
    SKIP("the allocation counter cannot observe the matcher's allocations in this build (shared library with its own allocation runtime, e.g. the MinGW DLL build), so the cost claim is not measurable here");
  }

  REQUIRE(fastPathAllocations<wordPathAllocations);
#else
  // The sanitizer runtime owns the allocation operators, so only the matcher's
  // own assertions run here; the cost claim cannot be measured.
  CountAllocations(fastPath,
                   candidate);
  CountAllocations(wordPath,
                   candidate);

  SKIP("the allocation counter is compiled out in sanitizer builds, whose runtime provides the allocation operators");
#endif
}
