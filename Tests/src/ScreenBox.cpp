/*
  GeoBox - a test program for libosmscout
  Copyright (C) 2023 Tim Teulings

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

#include <osmscout/util/ScreenBox.h>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("GetCenter() should return the center")
{
  osmscout::ScreenBox box(osmscout::Vertex2D(-1, -1),
                          osmscout::Vertex2D(1, 1));

  REQUIRE(box.GetCenter()==osmscout::Vertex2D::ZERO);
}

TEST_CASE("GetWidth() should return width")
{
  osmscout::ScreenBox box(osmscout::Vertex2D(-2, -2),
                          osmscout::Vertex2D(1, 2));

  REQUIRE(box.GetWidth()==3.0);
}

TEST_CASE("GetHeight() should return height")
{
  osmscout::ScreenBox box(osmscout::Vertex2D(-2, -2),
                          osmscout::Vertex2D(1, 2));

  REQUIRE(box.GetHeight()==4.0);
}

TEST_CASE("Intersection check for disjunctive areas")
{
  osmscout::ScreenBox a(osmscout::Vertex2D(0, 0),
                        osmscout::Vertex2D(1, 1));
  osmscout::ScreenBox b(osmscout::Vertex2D(2, 2),
                        osmscout::Vertex2D(3, 3));

  REQUIRE_FALSE(a.Intersects(b));
}

TEST_CASE("Intersection check for containing areas")
{
  osmscout::ScreenBox a(osmscout::Vertex2D(-2, -2),
                        osmscout::Vertex2D(2, 2));
  osmscout::ScreenBox b(osmscout::Vertex2D(-1, -1),
                        osmscout::Vertex2D(1, 1));

  REQUIRE(a.Intersects(b));
  REQUIRE(b.Intersects(a));
}

TEST_CASE("Resize >0")
{
  osmscout::ScreenBox box(osmscout::Vertex2D(-1, -1),
                          osmscout::Vertex2D(1, 1));
  osmscout::ScreenBox resizedBox=box.Resize(1);

  osmscout::ScreenBox expectedResizedBox(osmscout::Vertex2D(-2, -2),
                                         osmscout::Vertex2D(2, 2));

  REQUIRE(expectedResizedBox==resizedBox);
}

TEST_CASE("Resize ==0")
{
  osmscout::ScreenBox box(osmscout::Vertex2D(-1, -1),
                          osmscout::Vertex2D(1, 1));
  osmscout::ScreenBox resizedBox=box.Resize(0);

  osmscout::ScreenBox expectedResizedBox(osmscout::Vertex2D(-1, -1),
                                         osmscout::Vertex2D(1, 1));

  REQUIRE(expectedResizedBox==resizedBox);
}

TEST_CASE("Resize empty rectangle by zero")
{
  osmscout::ScreenBox box(osmscout::Vertex2D(-1, -1),
                          osmscout::Vertex2D(-1, 1));
  osmscout::ScreenBox resizedBox=box.Resize(0);

  osmscout::ScreenBox expectedResizedBox(osmscout::Vertex2D(-1, -1),
                                         osmscout::Vertex2D(-1, 1));

  REQUIRE(expectedResizedBox==resizedBox);
}

TEST_CASE("Resize square to zero")
{
  osmscout::ScreenBox box(osmscout::Vertex2D(-1, -1),
                          osmscout::Vertex2D(1, 1));
  osmscout::ScreenBox resizedBox=box.Resize(-1);

  osmscout::ScreenBox expectedResizedBox(osmscout::Vertex2D(0, 0),
                                         osmscout::Vertex2D(0, 0));

  REQUIRE(expectedResizedBox==resizedBox);
  REQUIRE(resizedBox.IsEmpty());

  resizedBox=resizedBox.Resize(-1);

  REQUIRE(expectedResizedBox==resizedBox);
  REQUIRE(resizedBox.IsEmpty());
}
