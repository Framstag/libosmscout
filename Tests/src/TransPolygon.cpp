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

#include <TestWay.h>

#include <osmscout/projection/MercatorProjection.h>

#include <osmscout/util/Geometry.h>
#include <osmscout/util/Transformation.h>

#include <TestMain.h>

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

