/*
  This source is part of the libosmscout-map library
  Copyright (C) 2026  Tim Teulings

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
 * Tests for the contract of the polygon triangulation the OpenGL backend draws with: a polygon it
 * cannot handle is rejected instead of terminating the render, input that repeats a point is
 * normalized rather than rejected, and the triangles of an accepted polygon are unchanged.
 *
 * The triangulation makes no GL call, so the cases need no database, no style sheet and no GL
 * context; the target links the OpenGL library only to reach the symbols (as OpenGLAreaVisibilityTest
 * links it to reach the visibility decision).
 */

#include <catch2/catch_test_macros.hpp>

#include <initializer_list>
#include <vector>

#include <osmscout/GeoCoord.h>
#include <osmscout/Pixel.h>
#include <osmscout/Point.h>

#include <osmscoutmapopengl/Triangulate.h>

namespace {

  using Ring = std::vector<osmscout::Vertex2D>;

  /**
   * The polygon that crashed the OpenGL backend's node path on the Dortmund database with
   * `stylesheets/standard.oss` before this change: a node whose ring has two points. Captured with the
   * diagnosis of TODO §97 (`PerformanceTest --driver opengl --start-zoom 15 --end-zoom 15
   * --icons … --font … --shaders … maps/Dortmund stylesheets/standard.oss 51.515 7.465 51.515 7.465`,
   * which exited 139 before the fix).
   */
  Ring TwoPointRing()
  {
    return Ring{{7.4811195394480308,51.523777369078736},
      {7.4811195394480308,51.523771532509144}};
  }

  /**
   * Two distinct points, each repeated: the ring has no corner at all, and the repeated points are the
   * zero-length edges the vendored triangulator asserts on.
   */
  Ring TwoValueRing()
  {
    return Ring{{0.0,0.0},
      {10.0,10.0},
      {0.0,0.0},
      {10.0,10.0}};
  }

  Ring CollinearRing()
  {
    return Ring{{0.0,0.0},
      {5.0,5.0},
      {10.0,10.0},
      {15.0,15.0}};
  }

  /**
   * The second input that crashed the OpenGL backend's node path after the degenerate rings were
   * rejected: a polygon primitive of an icon symbol mapped into lon/lat around a node, a sliver of
   * about 0.6 m by 0.05 m. Captured with the second diagnosis of TODO §97 (4 points, no repeated
   * point and no collinear corner).
   */
  Ring ThinSymbolPolygon()
  {
    return Ring{{7.4641315640120718,51.514864307506677},
      {7.4641259215149054,51.514864307506677},
      {7.4641259215149054,51.514863857012614},
      {7.4641315640120718,51.514863857012614}};
  }

  Ring Triangle()
  {
    return Ring{{0.0,0.0},
      {10.0,0.0},
      {0.0,10.0}};
  }

  Ring Square()
  {
    return Ring{{0.0,0.0},
      {10.0,0.0},
      {10.0,10.0},
      {0.0,10.0}};
  }

  Ring LShape()
  {
    return Ring{{0.0,0.0},
      {10.0,0.0},
      {10.0,4.0},
      {4.0,4.0},
      {4.0,10.0},
      {0.0,10.0}};
  }

  std::vector<osmscout::Point> Points(std::initializer_list<osmscout::GeoCoord> coords)
  {
    std::vector<osmscout::Point> points;
    uint8_t                      serial=1;

    for (const auto& coord : coords) {
      points.emplace_back(serial++,coord);
    }

    return points;
  }

  /**
   * A square given in longitude and latitude with a square hole inside it.
   */
  std::vector<std::vector<osmscout::Point>> SquareWithHole()
  {
    return {Points({osmscout::GeoCoord(0.0,0.0),
                    osmscout::GeoCoord(0.0,10.0),
                    osmscout::GeoCoord(10.0,10.0),
                    osmscout::GeoCoord(10.0,0.0)}),
            Points({osmscout::GeoCoord(4.0,4.0),
                    osmscout::GeoCoord(4.0,6.0),
                    osmscout::GeoCoord(6.0,6.0),
                    osmscout::GeoCoord(6.0,4.0)})};
  }

  size_t TriangleCount(const std::vector<GLfloat>& triangles)
  {
    return triangles.size()/6;
  }
}

TEST_CASE("A two-point ring is rejected instead of crashing","[PolygonTriangulation]")
{
  REQUIRE(TriangleCount(osmscout::Triangulate::TriangulatePolygon(TwoPointRing()))==0);
}

TEST_CASE("A ring of two distinct values repeated is rejected","[PolygonTriangulation]")
{
  REQUIRE(TriangleCount(osmscout::Triangulate::TriangulatePolygon(TwoValueRing()))==0);
}

TEST_CASE("An all-collinear ring is rejected","[PolygonTriangulation]")
{
  REQUIRE(TriangleCount(osmscout::Triangulate::TriangulatePolygon(CollinearRing()))==0);
}

TEST_CASE("A thin symbol polygon is rejected instead of crashing","[PolygonTriangulation]")
{
  REQUIRE(TriangleCount(osmscout::Triangulate::TriangulatePolygon(ThinSymbolPolygon()))==0);
}

TEST_CASE("A repeated closing point does not change the triangles","[PolygonTriangulation]")
{
  Ring               withRepeat=Square();

  osmscout::Vertex2D first=withRepeat.front();

  withRepeat.push_back(first);

  REQUIRE(osmscout::Triangulate::TriangulatePolygon(withRepeat)==
          osmscout::Triangulate::TriangulatePolygon(Square()));
}

TEST_CASE("A duplicated vertex does not change the triangles","[PolygonTriangulation]")
{
  Ring               withDuplicate=Square();

  osmscout::Vertex2D second=withDuplicate[1];

  withDuplicate.insert(withDuplicate.begin()+2,second);

  REQUIRE(osmscout::Triangulate::TriangulatePolygon(withDuplicate)==
          osmscout::Triangulate::TriangulatePolygon(Square()));
}

TEST_CASE("The triangles of an accepted polygon are unchanged","[PolygonTriangulation]")
{
  // The values the same input was triangulated into before the triangulation normalized its input
  REQUIRE(osmscout::Triangulate::TriangulatePolygon(Triangle())==
          std::vector<GLfloat> {0.0f,10.0f,0.0f,0.0f,10.0f,0.0f});

  REQUIRE(osmscout::Triangulate::TriangulatePolygon(Square())==
          std::vector<GLfloat> {0.0f,10.0f,10.0f,0.0f,10.0f,10.0f,
                                0.0f,10.0f,0.0f,0.0f,10.0f,0.0f});

  REQUIRE(osmscout::Triangulate::TriangulatePolygon(LShape())==
          std::vector<GLfloat> {0.0f,10.0f,4.0f,4.0f,4.0f,10.0f,
                                0.0f,10.0f,0.0f,0.0f,4.0f,4.0f,
                                4.0f,4.0f,0.0f,0.0f,10.0f,0.0f,
                                4.0f,4.0f,10.0f,0.0f,10.0f,4.0f});

  REQUIRE(osmscout::Triangulate::TriangulateWithHoles(SquareWithHole())==
          std::vector<GLfloat> {0.0f,10.0f,4.0f,6.0f,10.0f,10.0f,
                                0.0f,10.0f,0.0f,0.0f,4.0f,6.0f,
                                0.0f,0.0f,4.0f,4.0f,4.0f,6.0f,
                                4.0f,4.0f,0.0f,0.0f,10.0f,0.0f,
                                6.0f,4.0f,4.0f,4.0f,10.0f,0.0f,
                                6.0f,6.0f,6.0f,4.0f,10.0f,0.0f,
                                6.0f,6.0f,10.0f,0.0f,10.0f,10.0f,
                                4.0f,6.0f,6.0f,6.0f,10.0f,10.0f});
}
