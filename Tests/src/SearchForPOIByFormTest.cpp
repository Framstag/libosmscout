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
  SECTION("Search for POI: 'Bibliothek Dortmund' (no match)")
  {
    osmscout::POIFormSearchParameter parameter;
    osmscout::LocationSearchResult   result;

    parameter.SetAdminRegionSearchString("Bibliothek Dortmund");

    bool success=locationService->SearchForPOIByForm(parameter,
                                                     result);

    REQUIRE(success);
    REQUIRE_FALSE(result.limitReached);
    REQUIRE(result.results.empty());
  }
}
