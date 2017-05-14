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

using namespace std;

int main(int /*argc*/, char** /*argv*/)
{
  std::cout << "Two different zero size vector can't intersects... ";

  osmscout::GeoCoord a(51,14);
  osmscout::GeoCoord b(52,15);
  osmscout::GeoCoord intersection;
  
  if (osmscout::LinesIntersect(a,a,
                               b,b) ||
      osmscout::GetLineIntersection(a,a,
                                    b,b,
                                    intersection))
  {
    std::cout << "Failure" << std::endl;
    return 1;
  }
  std::cout << "OK" << std::endl;

  std::cout << "Two horizontal vectors, one left the other, can't intersects... ";
  osmscout::GeoCoord x1(1,0);
  osmscout::GeoCoord x2(2,0);
  osmscout::GeoCoord y1(3,0);
  osmscout::GeoCoord y2(4,0);
  if (osmscout::LinesIntersect(x1,x2,
                               y1,y2) ||
      osmscout::GetLineIntersection(x1,x2,
                                    y1,y2,
                                    intersection)){
    std::cout << "Failure" << std::endl;
    return 2;
  }
  std::cout << "OK" << std::endl;

  std::cout << "Two vertical vectors, one above the other, can't intersects... ";
  x1.Set(0,1);
  x2.Set(0,2);
  y1.Set(0,3);
  y2.Set(0,4);
  if (osmscout::LinesIntersect(x1,x2,
                               y1,y2) ||
      osmscout::GetLineIntersection(x1,x2,
                                    y1,y2,
                                    intersection)){
    std::cout << "Failure" << std::endl;
    return 2;
  }
  std::cout << "OK" << std::endl;

  std::cout << "Two touching vectors should intersects... ";
  x1.Set(65.03795,179.99898);
  x2.Set(65.03846,180.0);
  y1.Set(35.61404,180.0);
  y2.Set(83.83133,180.0);
  if (!osmscout::LinesIntersect(x1,x2,
                                y1,y2)){
    std::cout << "Failure" << std::endl;
    return 2;
  }
  std::cout << "OK" << std::endl;

  return 0;
}

