#include <osmscout/util/Geometry.h>

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

TEST_CASE("Simple point") {
  std::vector<osmscout::ScanCell> cells;

  osmscout::ScanConvertLine(0,0,0,0,cells);

  REQUIRE(cells.size()==1);
  REQUIRE(cells[0].x==0);
  REQUIRE(cells[0].y==0);
}

TEST_CASE("Simple diagonal") {
  std::vector<osmscout::ScanCell> cells;

  osmscout::ScanConvertLine(0,0,1,1,cells);

  REQUIRE(cells.size()==2);
  REQUIRE(cells[0].x==0);
  REQUIRE(cells[0].y==0);
  REQUIRE(cells[1].x==1);
  REQUIRE(cells[1].y==1);
}

TEST_CASE("Simple horizontal") {
  std::vector<osmscout::ScanCell> cells;

  osmscout::ScanConvertLine(0,0,10,0,cells);

  REQUIRE(cells.size()==11);
  REQUIRE(cells[0].x==0);
  REQUIRE(cells[0].y==0);
  REQUIRE(cells[10].x==10);
  REQUIRE(cells[10].y==0);
}

TEST_CASE("Simple vertical") {
  std::vector<osmscout::ScanCell> cells;

  osmscout::ScanConvertLine(0,0,0,10,cells);

  REQUIRE(cells.size()==11);
  REQUIRE(cells[0].x==0);
  REQUIRE(cells[0].y==0);
  REQUIRE(cells[10].x==0);
  REQUIRE(cells[10].y==10);
}
