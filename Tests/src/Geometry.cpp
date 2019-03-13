/*
  CachePerformance - a test program for libosmscout
  Copyright (C) 2017 Lukas Karas

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <cstdlib>
#include <iostream>

#include <osmscout/util/Geometry.h>
#include <osmscout/GeoCoord.h>

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

using namespace std;

TEST_CASE("Two different zero size vector can't intersects")
{
  osmscout::GeoCoord a(51, 14);
  osmscout::GeoCoord b(52, 15);
  osmscout::GeoCoord intersection;

  REQUIRE_FALSE(osmscout::LinesIntersect(a, a, b, b));
  REQUIRE_FALSE(osmscout::GetLineIntersection(a, a, b, b, intersection));
}

TEST_CASE("Two horizontal vectors, one left the other, can't intersects")
{
  osmscout::GeoCoord x1(1, 0);
  osmscout::GeoCoord x2(2, 0);
  osmscout::GeoCoord y1(3, 0);
  osmscout::GeoCoord y2(4, 0);
  osmscout::GeoCoord intersection;

  REQUIRE_FALSE(osmscout::LinesIntersect(x1, x2, y1, y2));
  REQUIRE_FALSE(osmscout::GetLineIntersection(x1, x2, y1, y2, intersection));
}

TEST_CASE("Two vertical vectors, one above the other, can't intersects")
{
  osmscout::GeoCoord x1(0, 1);
  osmscout::GeoCoord x2(0, 2);
  osmscout::GeoCoord y1(0, 3);
  osmscout::GeoCoord y2(0, 4);
  osmscout::GeoCoord intersection;

  REQUIRE_FALSE(osmscout::LinesIntersect(x1, x2, y1, y2));
  REQUIRE_FALSE(osmscout::GetLineIntersection(x1, x2, y1, y2, intersection));
}

TEST_CASE("Two touching vectors should intersects")
{
  osmscout::GeoCoord x1(65.03795,179.99898);
  osmscout::GeoCoord x2(65.03846,180.0);
  osmscout::GeoCoord y1(35.61404,180.0);
  osmscout::GeoCoord y2(83.83133,180.0);
  osmscout::GeoCoord intersection;

  REQUIRE(osmscout::LinesIntersect(x1, x2, y1, y2));
  REQUIRE(osmscout::GetLineIntersection(x1, x2, y1, y2, intersection));
}

static const double areaCoords[] = {
    1,1,
    1,10,
    5,5,
    10,10,
    10,1,
    5,4,
    0
};

TEST_CASE("Check node in area")
{
  std::vector<osmscout::Point> area;

  for (int i=0;;i+=2){
    double lat = areaCoords[i];
    if (lat == 0.0)
      break;
    double lon = areaCoords[i+1];
    area.push_back(osmscout::Point(0, osmscout::GeoCoord(lat, lon)));
  }

  SECTION("Nodes outside bounding box"){
    REQUIRE_FALSE(IsCoordInArea(osmscout::GeoCoord(0,0), area));
    REQUIRE_FALSE(IsCoordInArea(osmscout::GeoCoord(1,11), area));
    REQUIRE_FALSE(IsCoordInArea(osmscout::GeoCoord(11,5), area));
    REQUIRE_FALSE(IsCoordInArea(osmscout::GeoCoord(0.9,5), area));

    REQUIRE(GetRelationOfPointToArea(osmscout::GeoCoord(0,0), area) == -1);
    REQUIRE(GetRelationOfPointToArea(osmscout::GeoCoord(1,11), area) == -1);
    REQUIRE(GetRelationOfPointToArea(osmscout::GeoCoord(11,5), area) == -1);
    REQUIRE(GetRelationOfPointToArea(osmscout::GeoCoord(0.9,5), area) == -1);
  }
  SECTION("Nodes outside"){
    REQUIRE_FALSE(IsCoordInArea(osmscout::GeoCoord(5,3), area));
    REQUIRE_FALSE(IsCoordInArea(osmscout::GeoCoord(5,6), area));

    REQUIRE(GetRelationOfPointToArea(osmscout::GeoCoord(5,3), area) == -1);
    REQUIRE(GetRelationOfPointToArea(osmscout::GeoCoord(5,6), area) == -1);
  }
  SECTION("Nodes inside"){
    REQUIRE(IsCoordInArea(osmscout::GeoCoord(5,4.1), area));
    REQUIRE(IsCoordInArea(osmscout::GeoCoord(7,6), area));

    REQUIRE(GetRelationOfPointToArea(osmscout::GeoCoord(5,4.1), area) == 1);
    REQUIRE(GetRelationOfPointToArea(osmscout::GeoCoord(7,6), area) == 1);
  }
  SECTION("Nodes on boundary"){
    REQUIRE(IsCoordInArea(osmscout::GeoCoord(1,4), area));
    //REQUIRE(GetRelationOfPointToArea(osmscout::GeoCoord(1,4), area) == 0);
  }
}

bool IsSame(double a, double b, double tolerance = 1e-9)
{
  return std::abs(a - b) < tolerance;
}

bool IsSame(const osmscout::Distance &d1, const osmscout::Distance &d2, double tolerance = 1e-9)
{
  return IsSame(d1.AsMeter(), d2.AsMeter(), tolerance);
}

TEST_CASE("Distance computation")
{
  osmscout::GeoCoord location1(51.43170928, 6.80131361);
  osmscout::GeoCoord location2(51.48510151, 7.4160216);
  osmscout::Distance distance = location2 - location1;

  REQUIRE(IsSame(distance, osmscout::Distance::Of<osmscout::Kilometer>(43.135331925602415)));
  REQUIRE(IsSame(distance, osmscout::GetEllipsoidalDistance(location1.GetLon(), location1.GetLat(),
                                                               location2.GetLon(), location2.GetLat())));
  REQUIRE(IsSame(distance, osmscout::GetEllipsoidalDistance(location1, location2)));
}

bool IsSame(const osmscout::GeoCoord &a,
            const osmscout::GeoCoord &b)
{
  return std::abs(a.GetLat() - b.GetLat()) < 1e-6
      && std::abs(a.GetLon() - b.GetLon()) < 1e-6;
}

TEST_CASE("Target computation from bearing and distance")
{
  osmscout::GeoCoord location1(51.43170928, 6.80131361);

  // Target:
  //    Latitude: 51°27'48" N (51.463397)
  //    Longitude: 7°0'22" E (7.006078)
  osmscout::Distance distance = osmscout::Distance::Of<osmscout::Meter>(14665.298166863819);
  auto angle = osmscout::Bearing::Degrees(76.010085273091411718093847668127);

  osmscout::GeoCoord target = location1.Add(angle, distance);
  REQUIRE(IsSame(target, osmscout::GeoCoord(51.463397, 7.006078)));

  auto location2=osmscout::GetEllipsoidalDistance(location1,
                                                  angle,
                                                  distance);
  REQUIRE(IsSame(target, location2));
  REQUIRE(IsSame(distance, location1 - location2,1e-6));
}

TEST_CASE("Ellipsoidal distance up")
{
  osmscout::GeoCoord location(51.57178,7.45879);
  osmscout::Distance distance=osmscout::Distance::Of<osmscout::Kilometer>(5);
  auto               angle=osmscout::Bearing::Degrees(0);

  osmscout::GeoCoord target=location.Add(angle,distance);

  REQUIRE(target.GetLat()>location.GetLat());
  REQUIRE(IsSame(target.GetLon(),location.GetLon(),1e-4));
}

TEST_CASE("Ellipsoidal distance right")
{
  osmscout::GeoCoord location(51.5718,7.45879);
  osmscout::Distance distance=osmscout::Distance::Of<osmscout::Kilometer>(5);
  auto               angle=osmscout::Bearing::Degrees(90);

  osmscout::GeoCoord target=location.Add(angle,distance);

  INFO(target.GetDisplayText())

  REQUIRE(target.GetLon()>location.GetLon());
  REQUIRE(IsSame(target.GetLat(),location.GetLat(),1e-4));
}

TEST_CASE("Ellipsoidal distance down")
{
  osmscout::GeoCoord location(51.57178,7.45879);
  osmscout::Distance distance=osmscout::Distance::Of<osmscout::Kilometer>(5);
  auto               angle=osmscout::Bearing::Degrees(180);

  osmscout::GeoCoord target=location.Add(angle,distance);

  REQUIRE(target.GetLat()<location.GetLat());
  REQUIRE(IsSame(target.GetLon(),location.GetLon(),1e-4));
}

TEST_CASE("Ellipsoidal distance left")
{
  osmscout::GeoCoord location(51.57178,7.45879);
  osmscout::Distance distance=osmscout::Distance::Of<osmscout::Kilometer>(5);
  auto               angle=osmscout::Bearing::Degrees(270);

  osmscout::GeoCoord target=location.Add(angle,distance);

  INFO(target.GetDisplayText())

  REQUIRE(target.GetLon()<location.GetLon());
  REQUIRE(IsSame(target.GetLat(),location.GetLat(),1e-4));
}

TEST_CASE("Angle diff")
{
  double step = osmscout::DegToRad(10);
  for (double a = -M_PI; a < M_PI; a += step) {
    for (double diff = -M_PI; diff < M_PI - step; diff += step) {
      double b = a + diff;
      if (b < -M_PI)
        b += 2*M_PI;
      if (b > M_PI)
        b -= 2*M_PI;

      // std::cout << osmscout::RadToDeg(a) << ", " <<
      //           osmscout::RadToDeg(b) << "; " <<
      //           osmscout::RadToDeg(osmscout::AngleDiff(a, b)) << " ?= " <<
      //           osmscout::RadToDeg(std::abs(diff)) << std::endl;

      REQUIRE(IsSame(osmscout::AngleDiff(a, b), std::abs(diff)));
    }
  }
}
