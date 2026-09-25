#include <memory>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <osmscout/location/LocationService.h>

extern osmscout::LocationServiceRef locationService;

namespace {
  /*
   * Run one string search and return its result, asserting the invariants every
   * matching section shares: the search succeeds and does not hit the candidate
   * limit. The section asserts its own expectations on the returned result —
   * one place for the boilerplate instead of a copy per section.
   */
  osmscout::LocationSearchResult SearchForString(const std::string& query,
                                                 bool partialMatch=false)
  {
    osmscout::LocationStringSearchParameter parameter(query);

    parameter.SetPartialMatch(partialMatch);

    osmscout::LocationSearchResult result;

    REQUIRE(locationService->SearchForLocationByString(parameter,result));
    REQUIRE_FALSE(result.limitReached);

    return result;
  }

  /*
   * The components of a result entry as text. Two searches build their own
   * copies of the index objects, so entries are compared by content and not by
   * pointer.
   */
  std::string EntrySignature(const osmscout::LocationSearchResult::Entry& entry)
  {
    std::string signature;

    signature+=entry.adminRegion ? entry.adminRegion->name : "-";
    signature+='|';
    signature+=entry.postalArea ? entry.postalArea->name : "-";
    signature+='|';
    signature+=entry.location ? entry.location->name : "-";
    signature+='|';
    signature+=entry.address ? entry.address->name : "-";
    signature+='|';
    signature+=entry.poi ? entry.poi->name : "-";

    return signature;
  }
}

//
// City search
//

TEST_CASE("String search for city")
{
  /*
   * Search for the city name => match
   */
  SECTION("Search for city: 'Dortmund' (match)")
  {
    osmscout::LocationStringSearchParameter parameter("Dortmund");
    osmscout::LocationSearchResult          result;

    bool success=locationService->SearchForLocationByString(parameter,
                                                            result);

    REQUIRE(success);
    REQUIRE_FALSE(result.limitReached);
    REQUIRE(result.results.size()==1);
    REQUIRE(result.results.front().adminRegion->name=="Dortmund");
    REQUIRE(result.results.front().adminRegionMatchQuality==osmscout::LocationSearchResult::match);
  }

  /*
   * Search for the city name => no match
   */
  SECTION("Search for city: 'Hamburg' (no match)")
  {
    osmscout::LocationStringSearchParameter parameter("Hamburg");
    osmscout::LocationSearchResult          result;

    bool success=locationService->SearchForLocationByString(parameter,
                                                            result);

    REQUIRE(success);
    REQUIRE_FALSE(result.limitReached);
    REQUIRE(result.results.empty());
  }

  /*
   * Search city name => candidate (substring)
   */
  SECTION("Search for city: 'Dortm' (candidate)")
  {
    osmscout::LocationStringSearchParameter parameter("Dortm");
    osmscout::LocationSearchResult          result;

    bool success=locationService->SearchForLocationByString(parameter,
                                                            result);

    REQUIRE(success);
    REQUIRE_FALSE(result.limitReached);
    REQUIRE(result.results.size()==1);
    REQUIRE(result.results.front().adminRegion->name=="Dortmund");
    REQUIRE(result.results.front().adminRegionMatchQuality==osmscout::LocationSearchResult::candidate);
  }

  /*
   * Search for alias of the city => match
   */
  SECTION("Search for city: 'Brechten' (alias of Dortmund)")
  {
    osmscout::LocationStringSearchParameter parameter("Brechten");
    osmscout::LocationSearchResult          result;

    bool success=locationService->SearchForLocationByString(parameter,
                                                            result);

    REQUIRE(success);
    REQUIRE_FALSE(result.limitReached);
    REQUIRE(result.results.size()==1);
    REQUIRE(result.results.front().adminRegion->name=="Dortmund");
    REQUIRE(result.results.front().adminRegionMatchQuality==osmscout::LocationSearchResult::match);
  }

  /*
   * Substring of alias of the city => candiate (substring)
   */
  SECTION("Search for city: 'Brecht' (alias of Dortmund)")
  {
    osmscout::LocationStringSearchParameter parameter("Brecht");
    osmscout::LocationSearchResult          result;

    bool success=locationService->SearchForLocationByString(parameter,
                                                            result);

    REQUIRE(success);
    REQUIRE_FALSE(result.limitReached);
    REQUIRE(result.results.size()==1);
    REQUIRE(result.results.front().adminRegion->name=="Dortmund");
    REQUIRE(result.results.front().adminRegionMatchQuality==osmscout::LocationSearchResult::candidate);
  }
}

//
// City & location search
//

TEST_CASE("String search for city and location")
{
  /*
   * Search for location => match & city name => match
   */
  SECTION("Search for location in city: 'Am Birkenbaum Dortmund' (match)")
  {
    osmscout::LocationStringSearchParameter parameter("Am Birkenbaum Dortmund");
    osmscout::LocationSearchResult          result;

    bool success=locationService->SearchForLocationByString(parameter,
                                                            result);

    REQUIRE(success);
    REQUIRE_FALSE(result.limitReached);
    REQUIRE(result.results.size()==1);
    REQUIRE(result.results.front().adminRegion->name=="Dortmund");
    REQUIRE(result.results.front().adminRegionMatchQuality==osmscout::LocationSearchResult::match);
    REQUIRE(result.results.front().location->name=="Am Birkenbaum");
    REQUIRE(result.results.front().locationMatchQuality==osmscout::LocationSearchResult::match);
  }

  /*
   * Search for location => no match & city name => match
   */
  SECTION("Search for location in city: 'Trallafittistraße Dortmund' (no match)")
  {
    osmscout::LocationStringSearchParameter parameter("Trallafittistraße Dortmund");
    osmscout::LocationSearchResult          result;

    bool success=locationService->SearchForLocationByString(parameter,
                                                            result);

    REQUIRE(success);
    REQUIRE_FALSE(result.limitReached);
    REQUIRE(result.results.empty());
  }

  /*
   * Search for location => candidate (substring) & city name => match
   */
  SECTION("Search for location in city: 'Am Birken Dortmund' (location substring)")
  {
    osmscout::LocationStringSearchParameter parameter("Am Birken Dortmund");
    osmscout::LocationSearchResult          result;

    bool success=locationService->SearchForLocationByString(parameter,
                                                            result);

    REQUIRE(success);
    REQUIRE_FALSE(result.limitReached);
    REQUIRE(result.results.size()==1);
    REQUIRE(result.results.front().adminRegion->name=="Dortmund");
    REQUIRE(result.results.front().adminRegionMatchQuality==osmscout::LocationSearchResult::match);
    REQUIRE(result.results.front().location->name=="Am Birkenbaum");
    REQUIRE(result.results.front().locationMatchQuality==osmscout::LocationSearchResult::candidate);
  }

  /*
   * Search for city name => match & location => candidate (substring)
   */
  SECTION("Search for location in city: 'Dortmund Am Birken ' (location substring)")
  {
    osmscout::LocationStringSearchParameter parameter("Dortmund Am Birken");
    osmscout::LocationSearchResult          result;

    bool success=locationService->SearchForLocationByString(parameter,
                                                            result);

    REQUIRE(success);
    REQUIRE_FALSE(result.limitReached);
    REQUIRE(result.results.size()==1);
    REQUIRE(result.results.front().adminRegion->name=="Dortmund");
    REQUIRE(result.results.front().adminRegionMatchQuality==osmscout::LocationSearchResult::match);
    REQUIRE(result.results.front().location->name=="Am Birkenbaum");
    REQUIRE(result.results.front().locationMatchQuality==osmscout::LocationSearchResult::candidate);
  }

  /*
   * Words that the index joins with a hyphen are matched when the query spells
   * them apart (fix-compound-name-matching). The pattern covers the whole
   * location name, so the location itself is the match.
   */
  SECTION("Search for hyphen-joined location: 'August Warkner Platz Eving' (match)")
  {
    osmscout::LocationStringSearchParameter parameter("August Warkner Platz Eving");
    osmscout::LocationSearchResult          result;

    parameter.SetStringMatcherFactory(
        std::make_shared<osmscout::StringMatcherTransliterateTokenFactory>());

    bool success=locationService->SearchForLocationByString(parameter,
                                                            result);

    REQUIRE(success);
    REQUIRE_FALSE(result.limitReached);
    REQUIRE(result.results.size()==1);
    REQUIRE(result.results.front().location!=nullptr);
    REQUIRE(result.results.front().location->name=="August-Warkner-Platz");
    REQUIRE(result.results.front().locationMatchQuality==osmscout::LocationSearchResult::match);
  }

  /*
   * A word of the joined name is missing from the query, so the query's words are
   * no consecutive run of the name's words (spec: search-name-matching).
   */
  SECTION("Search for hyphen-joined location: 'August Platz Dortmund' (interrupted word run, no match)")
  {
    osmscout::LocationStringSearchParameter parameter("August Platz Dortmund");
    osmscout::LocationSearchResult          result;

    parameter.SetStringMatcherFactory(
        std::make_shared<osmscout::StringMatcherTransliterateTokenFactory>());

    bool success=locationService->SearchForLocationByString(parameter,
                                                            result);

    REQUIRE(success);
    REQUIRE_FALSE(result.limitReached);
    REQUIRE(result.results.empty());
  }
}

//
// POI search
//

TEST_CASE("String search for POI")
{
  /*
   * The hyphen in the query keeps the name's first word a substring of the
   * indexed name, so the POI is reached even without word matching. It also
   * proves the test data's POI is indexed at all.
   */
  SECTION("Search for POI: 'Test-Theater Eving' (hyphen in name and query)")
  {
    osmscout::LocationStringSearchParameter parameter("Test-Theater Eving");
    osmscout::LocationSearchResult          result;

    parameter.SetStringMatcherFactory(
        std::make_shared<osmscout::StringMatcherTransliterateTokenFactory>());

    bool success=locationService->SearchForLocationByString(parameter,
                                                            result);

    REQUIRE(success);
    REQUIRE_FALSE(result.limitReached);
    REQUIRE(result.results.size()==1);
    REQUIRE(result.results.front().poi!=nullptr);
    REQUIRE(result.results.front().poi->name=="Test-Theater Eving");
    REQUIRE(result.results.front().poiMatchQuality==osmscout::LocationSearchResult::candidate);
  }

  /*
   * Words that the index joins with a hyphen are matched when the query spells
   * them apart (fix-compound-name-matching). The name carries a third word the
   * query does not, so the POI is a candidate, not a full match.
   */
  SECTION("Search for POI: 'Test Theater Eving' (words spelled apart)")
  {
    osmscout::LocationStringSearchParameter parameter("Test Theater Eving");
    osmscout::LocationSearchResult          result;

    parameter.SetStringMatcherFactory(
        std::make_shared<osmscout::StringMatcherTransliterateTokenFactory>());

    bool success=locationService->SearchForLocationByString(parameter,
                                                            result);

    REQUIRE(success);
    REQUIRE_FALSE(result.limitReached);
    REQUIRE(result.results.size()==1);
    REQUIRE(result.results.front().poi!=nullptr);
    REQUIRE(result.results.front().poi->name=="Test-Theater Eving");
    REQUIRE(result.results.front().poiMatchQuality==osmscout::LocationSearchResult::candidate);
  }

  /*
   * The word matching accepts only consecutive words in query order, so a
   * reordered query does not reach the POI (spec: search-name-matching).
   */
  SECTION("Search for POI: 'Theater Test Eving' (reordered words, no match)")
  {
    osmscout::LocationStringSearchParameter parameter("Theater Test Eving");
    osmscout::LocationSearchResult          result;

    parameter.SetStringMatcherFactory(
        std::make_shared<osmscout::StringMatcherTransliterateTokenFactory>());

    bool success=locationService->SearchForLocationByString(parameter,
                                                            result);

    REQUIRE(success);
    REQUIRE_FALSE(result.limitReached);
    REQUIRE(result.results.empty());
  }
}

//
// City, location & address search
//

TEST_CASE("String search for city, location and address")
{
  /*
   * Search for location => match & address => match & city name => match
   */
  SECTION("Search for address and location in city: 'Am Birkenbaum 1 Dortmund' (match)")
  {
    osmscout::LocationStringSearchParameter parameter("Am Birkenbaum 1 Dortmund");
    osmscout::LocationSearchResult          result;

    bool success=locationService->SearchForLocationByString(parameter,
                                                            result);

    REQUIRE(success);
    REQUIRE_FALSE(result.limitReached);
    REQUIRE(result.results.size()==1);
    REQUIRE(result.results.front().adminRegion->name=="Dortmund");
    REQUIRE(result.results.front().adminRegionMatchQuality==osmscout::LocationSearchResult::match);
    REQUIRE(result.results.front().location->name=="Am Birkenbaum");
    REQUIRE(result.results.front().locationMatchQuality==osmscout::LocationSearchResult::match);
    REQUIRE(result.results.front().address->name=="1");
    REQUIRE(result.results.front().addressMatchQuality==osmscout::LocationSearchResult::match);
  }

  /*
   * Search for location => match & address => match & city name => match
   */
  SECTION("Search for address and location in city: 'Am Birkenbaum 10 Dortmund' (no match)")
  {
    osmscout::LocationStringSearchParameter parameter("Am Birkenbaum 10 Dortmund");
    osmscout::LocationSearchResult          result;

    bool success=locationService->SearchForLocationByString(parameter,
                                                            result);

    REQUIRE(success);
    REQUIRE_FALSE(result.limitReached);
    REQUIRE(result.results.empty());
  }

  /*
   * Search for city name => match & location => match & addess => match
   */
  SECTION("Search for address and location in city: 'Dortmund Am Birkenbaum 1' (match)")
  {
    osmscout::LocationStringSearchParameter parameter("Dortmund Am Birkenbaum 1");
    osmscout::LocationSearchResult          result;

    bool success=locationService->SearchForLocationByString(parameter,
                                                            result);

    REQUIRE(success);
    REQUIRE_FALSE(result.limitReached);
    REQUIRE(result.results.size()==1);
    REQUIRE(result.results.front().adminRegion->name=="Dortmund");
    REQUIRE(result.results.front().adminRegionMatchQuality==osmscout::LocationSearchResult::match);
    REQUIRE(result.results.front().location->name=="Am Birkenbaum");
    REQUIRE(result.results.front().locationMatchQuality==osmscout::LocationSearchResult::match);
    REQUIRE(result.results.front().address->name=="1");
    REQUIRE(result.results.front().addressMatchQuality==osmscout::LocationSearchResult::match);
  }

  /*
   * A surplus postal-code token between the house number and the city used
   * to zero out the result set (every token must be consumed). With partial
   * match enabled the search falls back to the street-level candidate instead
   * (fix-address-lookup-accuracy).
   */
  SECTION("Search for address with surplus postal token: 'Am Birkenbaum 1 44339 Dortmund' (partial fallback)")
  {
    auto result=SearchForString("Am Birkenbaum 1 44339 Dortmund",true);

    REQUIRE_FALSE(result.results.empty());
    REQUIRE(result.results.front().adminRegion->name=="Dortmund");
    REQUIRE(result.results.front().location->name=="Am Birkenbaum");
  }
}

//
// Word matching only adds matches
//

TEST_CASE("Name matching with and without word matching")
{
  /*
   * Queries that already matched must return the same entries with the matcher
   * the search bridge uses as with the previous matcher: the word matching only
   * adds matches, it never removes one nor changes its quality
   * (spec: search-name-matching).
   */
  const std::vector<std::string> queries{
      "Dortmund",
      "Brechten",
      "Am Birkenbaum Dortmund",
      "Am Birken Dortmund",
      "Am Birkenbaum 1 Dortmund",
      "Am Birkenbaum 1 44339 Dortmund",
      "August-Warkner-Platz"};

  for (const auto& query : queries) {
    osmscout::LocationStringSearchParameter previousMatcher(query);
    previousMatcher.SetPartialMatch(true);
    previousMatcher.SetStringMatcherFactory(
        std::make_shared<osmscout::StringMatcherTransliterateFactory>());

    osmscout::LocationSearchResult previousResult;

    REQUIRE(locationService->SearchForLocationByString(previousMatcher,
                                                       previousResult));

    osmscout::LocationStringSearchParameter wordMatcher(query);
    wordMatcher.SetPartialMatch(true);
    wordMatcher.SetStringMatcherFactory(
        std::make_shared<osmscout::StringMatcherTransliterateTokenFactory>());

    osmscout::LocationSearchResult wordResult;

    REQUIRE(locationService->SearchForLocationByString(wordMatcher,
                                                       wordResult));

    INFO("query: " << query);

    REQUIRE(wordResult.results.size()==previousResult.results.size());

    auto previousEntry=previousResult.results.begin();
    auto wordEntry=wordResult.results.begin();

    for (; previousEntry!=previousResult.results.end() && wordEntry!=wordResult.results.end();
           ++previousEntry,++wordEntry) {
      REQUIRE(EntrySignature(*previousEntry)==EntrySignature(*wordEntry));
      REQUIRE(previousEntry->adminRegionMatchQuality==wordEntry->adminRegionMatchQuality);
      REQUIRE(previousEntry->locationMatchQuality==wordEntry->locationMatchQuality);
      REQUIRE(previousEntry->poiMatchQuality==wordEntry->poiMatchQuality);
      REQUIRE(previousEntry->addressMatchQuality==wordEntry->addressMatchQuality);
    }
  }
}
