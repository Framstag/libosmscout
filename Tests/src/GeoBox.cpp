/*
  GeoBox - a test program for libosmscout
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

#include <osmscout/util/GeoBox.h>

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

using namespace std;

TEST_CASE("Invalid boxes should not intersects")
{
  osmscout::GeoBox a(osmscout::GeoCoord(0, 0), osmscout::GeoCoord(1, 1));
  osmscout::GeoBox b;

  REQUIRE(a.IsValid());
  REQUIRE_FALSE(b.IsValid());
  REQUIRE_FALSE(a.Intersects(b));
}

TEST_CASE("Intersection of touching boxes is not commutative by default")
{
  osmscout::GeoBox a(osmscout::GeoCoord(0, 0), osmscout::GeoCoord(1, 1));
  osmscout::GeoBox b(osmscout::GeoCoord(0, 1), osmscout::GeoCoord(1, 2));

  REQUIRE_FALSE(a.Intersects(b));
  REQUIRE(b.Intersects(a));
}

TEST_CASE("Intersection of touching boxes is commutative with close interval")
{
  osmscout::GeoBox a(osmscout::GeoCoord(0,0),osmscout::GeoCoord(1,1));
  osmscout::GeoBox b(osmscout::GeoCoord(0,1),osmscout::GeoCoord(1,2));
  REQUIRE(a.Intersects(b,/*openInterval*/false));
  REQUIRE(b.Intersects(a,/*openInterval*/false));
}
