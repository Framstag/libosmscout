#include "catch.hpp"

#include <osmscout/LocationService.h>

extern osmscout::LocationServiceRef locationService;
extern void DumpSeachResult(const osmscout::LocationSearchResult& result);

//
// City search
//
TEST_CASE("Form POI search")
{
  /*
   * Search for POI in City
   */
  SECTION("Search for POI: 'Bücherei Dortmund' (no match)")
  {
    osmscout::POIFormSearchParameter parameter;
    osmscout::LocationSearchResult   result;

    parameter.SetPartialMatch(false);
    parameter.SetAdminRegionSearchString("Dortmund");
    parameter.SetPOISearchString("Bücherei");

    bool success=locationService->SearchForPOIByForm(parameter,
                                                     result);

    REQUIRE(success);
    REQUIRE_FALSE(result.limitReached);
    REQUIRE(result.results.empty());
  }

  SECTION("Search for POI: 'Stadtteilbibliothek Eving Dortmund' (match)")
  {
    osmscout::POIFormSearchParameter parameter;
    osmscout::LocationSearchResult   result;

    parameter.SetPartialMatch(false);
    parameter.SetAdminRegionSearchString("Dortmund");
    parameter.SetPOISearchString("Stadtteilbibliothek Eving");

    bool success=locationService->SearchForPOIByForm(parameter,
                                                     result);

    REQUIRE(success);
    REQUIRE_FALSE(result.limitReached);
    REQUIRE(result.results.size()==1);
    REQUIRE(result.results.front().adminRegion->name=="Dortmund");
    REQUIRE(result.results.front().adminRegionMatchQuality==osmscout::LocationSearchResult::match);
    REQUIRE(result.results.front().poi->name=="Stadtteilbibliothek Eving");
    REQUIRE(result.results.front().poiMatchQuality==osmscout::LocationSearchResult::match);
    REQUIRE(result.results.front().postalAreaMatchQuality==osmscout::LocationSearchResult::none);
    REQUIRE(result.results.front().locationMatchQuality==osmscout::LocationSearchResult::none);
    REQUIRE(result.results.front().addressMatchQuality==osmscout::LocationSearchResult::none);
  }

  SECTION("Search for POI: 'Stadtteilbibliothek Dortmund' (candidate)")
  {
    osmscout::POIFormSearchParameter parameter;
    osmscout::LocationSearchResult   result;

    parameter.SetPartialMatch(false);
    parameter.SetAdminRegionSearchString("Dortmund");
    parameter.SetPOISearchString("Stadtteilbibliothek");

    bool success=locationService->SearchForPOIByForm(parameter,
                                                     result);

    REQUIRE(success);
    REQUIRE_FALSE(result.limitReached);
    REQUIRE(result.results.size()==1);
    REQUIRE(result.results.front().adminRegion->name=="Dortmund");
    REQUIRE(result.results.front().adminRegionMatchQuality==osmscout::LocationSearchResult::match);
    REQUIRE(result.results.front().poi->name=="Stadtteilbibliothek Eving");
    REQUIRE(result.results.front().poiMatchQuality==osmscout::LocationSearchResult::candidate);
    REQUIRE(result.results.front().postalAreaMatchQuality==osmscout::LocationSearchResult::none);
    REQUIRE(result.results.front().locationMatchQuality==osmscout::LocationSearchResult::none);
    REQUIRE(result.results.front().addressMatchQuality==osmscout::LocationSearchResult::none);
  }

}
