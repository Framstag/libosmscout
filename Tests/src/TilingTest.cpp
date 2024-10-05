#include <osmscout/Point.h>
#include <osmscout/util/TileId.h>
#include <osmscout/util/Tiling.h>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("TileIdBox calculation") {
  osmscout::Magnification magnification(osmscout::Magnification::magClose);
  osmscout::GeoCoord      coord1(51.5717798,7.458785);
  osmscout::GeoCoord      coord2(50.6890143,7.1360549);

  osmscout::TileId tile1=osmscout::TileId::GetTile(magnification,coord1);
  osmscout::TileId tile2=osmscout::TileId::GetTile(magnification,coord2);

  osmscout::TileIdBox box(tile1,tile2);

  REQUIRE(box.GetBoundingBox(magnification).Includes(coord1));
  REQUIRE(box.GetBoundingBox(magnification).Includes(coord2));

  osmscout::TileId min=box.GetMin();
  osmscout::TileId max=box.GetMax();

  INFO("Min "+min.GetDisplayText());
  INFO("Max "+max.GetDisplayText());

  REQUIRE(min==osmscout::TileId(8516,12805));
  REQUIRE(max==osmscout::TileId(8531,12886));

  REQUIRE(box.GetWidth()==16);
  REQUIRE(box.GetHeight()==82);

  REQUIRE(box.GetCount()==16*82);

  for (const auto& tile : box) {
    REQUIRE(tile.GetX()>=min.GetX());
    REQUIRE(tile.GetX()<=max.GetX());
    REQUIRE(tile.GetY()>=min.GetY());
    REQUIRE(tile.GetY()<=max.GetY());
  }
}

TEST_CASE("TileIdBox intersection") {
  osmscout::TileIdBox a(osmscout::TileId(0,0),
                        osmscout::TileId(9,10));
  osmscout::TileIdBox b(osmscout::TileId(5,6),
                        osmscout::TileId(15,15));

  osmscout::TileIdBox intersection = a.Intersection(b);
  REQUIRE(intersection == b.Intersection(a));
  REQUIRE(intersection.GetMin() == osmscout::TileId(5,6));
  REQUIRE(intersection.GetMax() == osmscout::TileId(9,10));
}

TEST_CASE("TileIdBox inclusion") {
  osmscout::TileIdBox a(osmscout::TileId(0,0),
                        osmscout::TileId(10,10));
  osmscout::TileIdBox b(osmscout::TileId(5,6),
                        osmscout::TileId(15,15));

  REQUIRE(a.Include(b) == b.Include(a));

  REQUIRE(a.Include(b) == osmscout::TileIdBox(osmscout::TileId(0,0),
                                              osmscout::TileId(15,15)));

  REQUIRE(a.Include(osmscout::TileId(11,12)) == osmscout::TileIdBox(osmscout::TileId(0,0),
                                                                    osmscout::TileId(11,12)));

  REQUIRE(b.Include(osmscout::TileId(1,2)) == osmscout::TileIdBox(osmscout::TileId(1,2),
                                                                  osmscout::TileId(15,15)));

}

TEST_CASE("TileIdBox intersects") {
  osmscout::TileIdBox a(osmscout::TileId(1,1),
                        osmscout::TileId(10,10));

  REQUIRE(a.Intersects(osmscout::TileIdBox(osmscout::TileId(1,1),
                                           osmscout::TileId(1,1))));
  REQUIRE(a.Intersects(osmscout::TileIdBox(osmscout::TileId(10,10),
                                           osmscout::TileId(10,10))));
  REQUIRE_FALSE(a.Intersects(osmscout::TileIdBox(osmscout::TileId(0,0),
                                                 osmscout::TileId(0,0))));
  REQUIRE_FALSE(a.Intersects(osmscout::TileIdBox(osmscout::TileId(11,11),
                                                 osmscout::TileId(11,11))));
}

TEST_CASE("OSMTileIdBox calculation") {
  osmscout::Magnification magnification(osmscout::Magnification::magClose);
  osmscout::GeoCoord      coord1(51.5717798,7.458785);
  osmscout::GeoCoord      coord2(50.6890143,7.1360549);

  osmscout::OSMTileId tile1=osmscout::OSMTileId::GetOSMTile(magnification,coord1);
  osmscout::OSMTileId tile2=osmscout::OSMTileId::GetOSMTile(magnification,coord2);

  osmscout::OSMTileIdBox box(tile1,tile2);

  osmscout::OSMTileId min=box.GetMin();
  osmscout::OSMTileId max=box.GetMax();

  INFO("Min "+min.GetDisplayText());
  INFO("Max "+max.GetDisplayText());

  REQUIRE(min==osmscout::OSMTileId(8516,5443));
  REQUIRE(max==osmscout::OSMTileId(8531,5507));

  REQUIRE(box.GetWidth()==16);
  REQUIRE(box.GetHeight()==65);

  REQUIRE(box.GetCount()==16*65);

  for (const auto& tile : box) {
    REQUIRE(tile.GetX()>=min.GetX());
    REQUIRE(tile.GetX()<=max.GetX());
    REQUIRE(tile.GetY()>=min.GetY());
    REQUIRE(tile.GetY()<=max.GetY());
  }
}

TEST_CASE("Test reverse calculation of coordinates from node id") {
  osmscout::Magnification magnification(osmscout::MagnificationLevel(24));
  osmscout::GeoCoord      coord(51.5726193, 7.1448805);

  INFO("Coord: "+coord.GetDisplayText());

  osmscout::Pixel expectedTile=osmscout::TileId::GetTile(magnification,coord).AsPixel();

  INFO("Expected tile: "+expectedTile.GetDisplayText());

  osmscout::Point point(0,coord);

  osmscout::GeoCoord calculatedCoord=osmscout::Point::GetCoordFromId(point.GetId());

  INFO("Reverse calculated coord: "+calculatedCoord.GetDisplayText());

  osmscout::Pixel calculatedTile=osmscout::TileId::GetTile(magnification,calculatedCoord).AsPixel();

  INFO("Calculated tile: "+calculatedTile.GetDisplayText());

  REQUIRE(expectedTile==calculatedTile);
}
