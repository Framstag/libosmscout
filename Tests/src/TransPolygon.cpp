/*
  DrawTextQt - a test program for libosmscout
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
#include <TestWay.h>

#include <osmscout/util/Geometry.h>
#include <osmscout/util/Transformation.h>

using namespace std;

bool WayIsSimple(const std::vector<osmscout::Point> &points)
{
  if (points.size()<3) {
    return true;
  }

  size_t edgesIntersect=0;

  for (size_t i=0; i<points.size()-1; i++) {
    edgesIntersect=0;

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

int main(int /*argc*/, char** /*argv*/)
{
  std::vector<osmscout::Point> testWay=GetTestWay();

  if (!WayIsSimple(testWay)){
    std::cout << "Test way is not simple" << std::endl;
    return 1;
  }

  osmscout::MercatorProjection projection;
  osmscout::Magnification mag;
  mag.SetMagnification(osmscout::Magnification::magSuburb);
  projection.Set(osmscout::GeoCoord(43.914554, 8.0902544),
                 /*angle*/ 0,
                 mag,
                 /*dpi*/ 72,
                 /*width*/ 1000,
                 /*height*/ 1000);

  osmscout::TransPolygon polygon;
  polygon.TransformWay(projection,
                       osmscout::TransPolygon::OptimizeMethod::quality,
                       testWay,
                       /*optimizeErrorTolerance*/1.0,
                       osmscout::TransPolygon::simple);

  std::vector<osmscout::Point> optimised;
  for (size_t p=polygon.GetStart(); p<=polygon.GetEnd(); p++) {
    if (polygon.points[p].draw) {
      optimised.push_back(osmscout::Point(0, osmscout::GeoCoord(polygon.points[p].x, polygon.points[p].y)));
    }
  }

  if (!WayIsSimple(optimised)){
    std::cout << "Optimised way is not simple" << std::endl;
    return 2;
  }

  // try again as area
  polygon.TransformArea(projection,
                        osmscout::TransPolygon::OptimizeMethod::quality,
                        testWay,
                        /*optimizeErrorTolerance*/1.0,
                        osmscout::TransPolygon::simple);

  optimised.clear();
  for (size_t p=polygon.GetStart(); p<=polygon.GetEnd(); p++) {
    if (polygon.points[p].draw) {
      optimised.push_back(osmscout::Point(0, osmscout::GeoCoord(polygon.points[p].x, polygon.points[p].y)));
    }
  }

  if (!AreaIsSimple(optimised)){
    std::cout << "Optimised area is not simple" << std::endl;
    return 3;
  }

  std::cout << "OK" << std::endl;

  return 0;
}

