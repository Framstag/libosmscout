#include "catch.hpp"

#include <osmscout/LocationService.h>

extern osmscout::LocationServiceRef locationService;

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
}
