/*
  TransPolygon - a test program for libosmscout
  Copyright (C) 2017  Lukas Karas

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
#include <utility>

#include <TestWay.h>

#include <osmscout/projection/MercatorProjection.h>

#include <osmscout/util/Geometry.h>
#include <osmscout/util/Transformation.h>

#include <catch2/catch_test_macros.hpp>

using namespace std;

bool WayIsSimple(const std::vector<osmscout::Point> &points)
{
  if (points.size()<3) {
    return true;
  }


  for (size_t i=0; i<points.size()-1; i++) {
    size_t edgesIntersect=0;

    for (size_t j=i+1; j<points.size()-1; j++) {
      if (LinesIntersect(points[i],
                         points[i+1],
                         points[j],
                         points[j+1])) {
        edgesIntersect++;

        if (i==0) {
          if (edgesIntersect>2) {
            return false;
          }
        }
        else {
          if (edgesIntersect>1) {
            return false;
          }
        }
      }
    }
  }

  return true;
}

TEST_CASE("Testway is simple")
{
  std::vector<osmscout::Point> testWay=GetTestWay();

  REQUIRE(WayIsSimple(testWay));
}

TEST_CASE("Optimized way is still simple")
{
  std::vector<osmscout::Point> testWay=GetTestWay();

  osmscout::MercatorProjection projection;
  osmscout::Magnification      mag;
  mag.SetLevel(osmscout::Magnification::magSuburb);
  projection.Set(osmscout::GeoCoord(43.914554,
                                    8.0902544),
    /*angle*/
                 0,
                 mag,
    /*dpi*/
                 72,
    /*width*/
                 1000,
    /*height*/
                 1000);

  osmscout::TransBuffer transBuffer;
  osmscout::TransformWay(testWay,
                         transBuffer,
                         projection,
                         osmscout::TransPolygon::OptimizeMethod::quality,
                         1.0,
                         osmscout::TransPolygon::simple);

  std::vector<osmscout::Point> optimised;
  for (size_t                  p      =transBuffer.GetStart(); p<=transBuffer.GetEnd(); p++) {
    if (transBuffer.points[p].draw) {
      optimised.emplace_back(0,
                             osmscout::GeoCoord(transBuffer.points[p].x,
                                                transBuffer.points[p].y));
    }
  }

  REQUIRE(WayIsSimple(optimised));
}

TEST_CASE("Optimized area is still simple")
{
  std::vector<osmscout::Point> testWay=GetTestWay();

  osmscout::MercatorProjection projection;
  osmscout::Magnification mag;
  mag.SetLevel(osmscout::Magnification::magSuburb);
  projection.Set(osmscout::GeoCoord(43.914554, 8.0902544),
                 /*angle*/ 0,
                 mag,
                 /*dpi*/ 72,
                 /*width*/ 1000,
                 /*height*/ 1000);

  osmscout::TransBuffer transBuffer;

  // try again as area
  osmscout::TransformArea(testWay,
                          transBuffer,
                          projection,
                          osmscout::TransPolygon::OptimizeMethod::quality,
                          1.0,
                          osmscout::TransPolygon::simple);

  std::vector<osmscout::Point> optimised;
  for (size_t p=transBuffer.GetStart(); p<=transBuffer.GetEnd(); p++) {
    if (transBuffer.points[p].draw) {
      optimised.emplace_back(0, osmscout::GeoCoord(transBuffer.points[p].x, transBuffer.points[p].y));
    }
  }

  REQUIRE(AreaIsSimple(optimised));
}

static const double testRingLat=43.914554;
static const double testRingLon=8.0902544;
static const double testRingRadius=0.004;

/**
 * A ring of well-separated points around the projection centre, given as latitude and longitude
 * offsets in the order they are visited. The points are far enough apart that the optimizer keeps
 * every one of them, so the number of drawn points is the number of offsets handed in and the case
 * decides how far the optimizer's own sequence has to grow.
 */
static std::vector<osmscout::Point> GetTestRing(const std::vector<std::pair<double,double>>& offsets)
{
  std::vector<osmscout::Point> ring;

  ring.reserve(offsets.size());

  for (const auto &offset : offsets) {
    ring.emplace_back(0,
                      osmscout::GeoCoord(testRingLat+offset.first,
                                         testRingLon+offset.second));
  }

  return ring;
}

/**
 * Optimizes the given ring as an area and returns the drawn points of the result as points whose
 * coordinate carries the transformed position, the way the cases below inspect them.
 */
static std::vector<osmscout::Point> OptimizeTestRingAsArea(const std::vector<osmscout::Point>& ring)
{
  osmscout::MercatorProjection projection;
  osmscout::Magnification      mag;

  mag.SetLevel(osmscout::Magnification::magSuburb);
  projection.Set(osmscout::GeoCoord(testRingLat, testRingLon),
                 /*angle*/ 0,
                 mag,
                 /*dpi*/ 72,
                 /*width*/ 1000,
                 /*height*/ 1000);

  osmscout::TransBuffer transBuffer;

  osmscout::TransformArea(ring,
                          transBuffer,
                          projection,
                          osmscout::TransPolygon::OptimizeMethod::quality,
                          1.0,
                          osmscout::TransPolygon::simple);

  std::vector<osmscout::Point> optimised;

  for (size_t p=transBuffer.GetStart(); p<=transBuffer.GetEnd(); p++) {
    if (transBuffer.points[p].draw) {
      optimised.emplace_back(0,
                             osmscout::GeoCoord(transBuffer.points[p].x,
                                                transBuffer.points[p].y));
    }
  }

  return optimised;
}

/**
 * Four drawn points fill the optimizer's own sequence to its storage, so the append that closes the
 * ring for the simplicity decision has to move that storage. The geometry keeps all four points and
 * comes back simple as a closed ring.
 */
TEST_CASE("Optimized area keeps its geometry when the optimized sequence grows")
{
  std::vector<osmscout::Point> ring=GetTestRing({{-testRingRadius,-testRingRadius},
                                                  {-testRingRadius, testRingRadius},
                                                  { testRingRadius, testRingRadius},
                                                  { testRingRadius,-testRingRadius}});
  std::vector<osmscout::Point> optimised=OptimizeTestRingAsArea(ring);

  REQUIRE(optimised.size()==ring.size());
  REQUIRE(AreaIsSimple(optimised));
}

/**
 * Five drawn points leave the optimizer's own sequence spare storage for the append that closes the
 * ring for the simplicity decision, the other storage state that append can meet. The geometry keeps
 * all five points.
 */
TEST_CASE("Optimized area keeps its geometry when the optimized sequence has spare storage")
{
  std::vector<osmscout::Point> ring=GetTestRing({{-testRingRadius,-testRingRadius},
                                                  {-testRingRadius, testRingRadius},
                                                  {             0.0, 1.3*testRingRadius},
                                                  { testRingRadius, testRingRadius},
                                                  { testRingRadius,-testRingRadius}});
  std::vector<osmscout::Point> optimised=OptimizeTestRingAsArea(ring);

  REQUIRE(optimised.size()==ring.size());
  REQUIRE(AreaIsSimple(optimised));
}
