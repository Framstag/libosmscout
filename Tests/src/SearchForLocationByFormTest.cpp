#include "catch.hpp"

#include <osmscout/LocationService.h>

extern osmscout::LocationServiceRef locationService;
extern void DumpSeachResult(const osmscout::LocationSearchResult& result);

//
// City search
//
TEST_CASE("Form location search for city/postalArea/location/address")
{
  /*
   * Search for the city name => match
   */
  SECTION("Search for city: 'Dortmund' (match)")
  {
    osmscout::LocationFormSearchParameter parameter;
    osmscout::LocationSearchResult        result;

    parameter.SetAdminRegionSearchString("Dortmund");

    bool success=locationService->SearchForLocationByForm(parameter,
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
    osmscout::LocationFormSearchParameter parameter;
    osmscout::LocationSearchResult        result;

    parameter.SetAdminRegionSearchString("Hamburg");

    bool success=locationService->SearchForLocationByForm(parameter,
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
    osmscout::LocationFormSearchParameter parameter;
    osmscout::LocationSearchResult        result;

    parameter.SetAdminRegionSearchString("Dortm");

    bool success=locationService->SearchForLocationByForm(parameter,
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
    osmscout::LocationFormSearchParameter parameter;
    osmscout::LocationSearchResult        result;

    parameter.SetAdminRegionSearchString("Brechten");

    bool success=locationService->SearchForLocationByForm(parameter,
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
    osmscout::LocationFormSearchParameter parameter;
    osmscout::LocationSearchResult        result;

    parameter.SetAdminRegionSearchString("Brecht");

    bool success=locationService->SearchForLocationByForm(parameter,
                                                          result);

    REQUIRE(success);
    REQUIRE_FALSE(result.limitReached);
    REQUIRE(result.results.size()==1);
    REQUIRE(result.results.front().adminRegion->name=="Dortmund");
    REQUIRE(result.results.front().adminRegionMatchQuality==osmscout::LocationSearchResult::candidate);
  }
}

TEST_CASE("Form location search for city and postal code")
{
  /*
   * Search for the city name => match, postal area => match
   */
  SECTION("Search for city: '44339 Dortmund' (match)")
  {
    osmscout::LocationFormSearchParameter parameter;
    osmscout::LocationSearchResult        result;

    parameter.SetAdminRegionSearchString("Dortmund");
    parameter.SetPostalAreaSearchString("44339");

    bool success=locationService->SearchForLocationByForm(parameter,
                                                          result);

    REQUIRE(success);
    REQUIRE_FALSE(result.limitReached);
    REQUIRE(result.results.size()==1);
    REQUIRE(result.results.front().adminRegion->name=="Dortmund");
    REQUIRE(result.results.front().adminRegionMatchQuality==osmscout::LocationSearchResult::match);
    REQUIRE(result.results.front().postalArea->name=="44339");
    REQUIRE(result.results.front().postalAreaMatchQuality==osmscout::LocationSearchResult::match);
  }

  /*
   * Search for the city name => match, postal area => no match
   */
  SECTION("Search for city: '44340 Dortmund' (no match)")
  {
    osmscout::LocationFormSearchParameter parameter;
    osmscout::LocationSearchResult        result;

    parameter.SetAdminRegionSearchString("Dortmund");
    parameter.SetPostalAreaSearchString("44340");

    bool success=locationService->SearchForLocationByForm(parameter,
                                                          result);

    REQUIRE(success);
    REQUIRE_FALSE(result.limitReached);
    REQUIRE(result.results.empty());
  }

  /*
   * Search for the city name => match, postal area => match
   */
  SECTION("Search for city: '44339 Dortmund' (candidate)")
  {
    osmscout::LocationFormSearchParameter parameter;
    osmscout::LocationSearchResult        result;

    parameter.SetAdminRegionSearchString("Dortmund");
    parameter.SetPostalAreaSearchString("443");

    bool success=locationService->SearchForLocationByForm(parameter,
                                                          result);

    REQUIRE(success);
    REQUIRE_FALSE(result.limitReached);
    REQUIRE(result.results.size()==1);
    REQUIRE(result.results.front().adminRegion->name=="Dortmund");
    REQUIRE(result.results.front().adminRegionMatchQuality==osmscout::LocationSearchResult::match);
    REQUIRE(result.results.front().postalArea->name=="44339");
    REQUIRE(result.results.front().postalAreaMatchQuality==osmscout::LocationSearchResult::candidate);
  }
}

//
// City & location search
//
TEST_CASE("Form location search for city and location")
{
  /*
   * Search for location => match & city name => match
   */
  SECTION("Search for location in city: 'Am Birkenbaum Dortmund' (match)")
  {
    osmscout::LocationFormSearchParameter parameter;
    osmscout::LocationSearchResult        result;

    parameter.SetAdminRegionSearchString("Dortmund");
    parameter.SetLocationSearchString("Am Birkenbaum");

    bool success=locationService->SearchForLocationByForm(parameter,
                                                          result);

    REQUIRE(success);
    REQUIRE_FALSE(result.limitReached);
    REQUIRE(result.results.size()==1);
    REQUIRE(result.results.front().adminRegion->name=="Dortmund");
    REQUIRE(result.results.front().adminRegionMatchQuality==osmscout::LocationSearchResult::match);
    REQUIRE(result.results.front().postalArea->name=="44339");
    REQUIRE(result.results.front().postalAreaMatchQuality==osmscout::LocationSearchResult::match);
    REQUIRE(result.results.front().location->name=="Am Birkenbaum");
    REQUIRE(result.results.front().locationMatchQuality==osmscout::LocationSearchResult::match);
  }

  /*
   * Search for location => no match & city name => match
   */
  SECTION("Search for location in city: 'Trallafittistraße Dortmund' (no match)")
  {
    osmscout::LocationFormSearchParameter parameter;
    osmscout::LocationSearchResult        result;

    parameter.SetAdminRegionSearchString("Dortmund");
    parameter.SetLocationSearchString("Trallafittistraße");

    bool success=locationService->SearchForLocationByForm(parameter,
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
    osmscout::LocationFormSearchParameter parameter;
    osmscout::LocationSearchResult        result;

    parameter.SetAdminRegionSearchString("Dortmund");
    parameter.SetLocationSearchString("Am Birken");

    bool success=locationService->SearchForLocationByForm(parameter,
                                                          result);

    REQUIRE(success);
    REQUIRE_FALSE(result.limitReached);
    REQUIRE(result.results.size()==1);
    REQUIRE(result.results.front().adminRegion->name=="Dortmund");
    REQUIRE(result.results.front().adminRegionMatchQuality==osmscout::LocationSearchResult::match);
    REQUIRE(result.results.front().postalArea->name=="44339");
    REQUIRE(result.results.front().postalAreaMatchQuality==osmscout::LocationSearchResult::match);
    REQUIRE(result.results.front().location->name=="Am Birkenbaum");
    REQUIRE(result.results.front().locationMatchQuality==osmscout::LocationSearchResult::candidate);
  }
}

//
// City, location & address search
//
TEST_CASE("Form location search for city, location and address")
{
  /*
   * Search for location => match & address => match & city name => match
   */
  SECTION("Search for address and location in city: 'Am Birkenbaum 1 Dortmund' (match)")
  {
    osmscout::LocationFormSearchParameter parameter;
    osmscout::LocationSearchResult        result;

    parameter.SetAdminRegionSearchString("Dortmund");
    parameter.SetLocationSearchString("Am Birkenbaum");
    parameter.SetAddressSearchString("1");

    bool success=locationService->SearchForLocationByForm(parameter,
                                                          result);

    REQUIRE(success);
    REQUIRE_FALSE(result.limitReached);
    REQUIRE(result.results.size()==1);
    REQUIRE(result.results.front().adminRegion->name=="Dortmund");
    REQUIRE(result.results.front().adminRegionMatchQuality==osmscout::LocationSearchResult::match);
    REQUIRE(result.results.front().postalArea->name=="44339");
    REQUIRE(result.results.front().postalAreaMatchQuality==osmscout::LocationSearchResult::match);
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
    osmscout::LocationFormSearchParameter parameter;
    osmscout::LocationSearchResult        result;

    parameter.SetAdminRegionSearchString("Dortmund");
    parameter.SetLocationSearchString("Am Birkenbaum");
    parameter.SetAddressSearchString("10");

    bool success=locationService->SearchForLocationByForm(parameter,
                                                          result);

    REQUIRE(success);
    REQUIRE_FALSE(result.limitReached);
    REQUIRE(result.results.empty());
  }
}
