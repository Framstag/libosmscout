#include <osmscout/Point.h>
#include <osmscout/util/Tiling.h>

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

TEST_CASE("Test reverse calculation of coordinates from node id") {
  osmscout::Magnification magnification;
  osmscout::GeoCoord coord(51.5726193, 7.1448805);

  INFO("Coord: "+coord.GetDisplayText());

  magnification.SetLevel(24);

  osmscout::TileCalculator calculator(magnification);

  osmscout::Pixel expectedTile=calculator.GetTileId(coord);

  INFO("Expected tile: "+expectedTile.GetDisplayText());

  osmscout::Point point(0,coord);

  osmscout::GeoCoord calculatedCoord=osmscout::Point::GetCoordFromId(point.GetId());

  INFO("Reverse calculated coord: "+calculatedCoord.GetDisplayText());

  osmscout::Pixel calculatedTile=calculator.GetTileId(calculatedCoord);

  INFO("Calculated tile: "+calculatedTile.GetDisplayText());

  REQUIRE(expectedTile==calculatedTile);
}
