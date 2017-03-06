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
  
  assert (!osmscout::GetLineIntersection(a,a,
                                         b,b,
                                         intersection));
  assert (!osmscout::LinesIntersect(a,a,
                                    b,b));
  std::cout << "OK" << std::endl;
}

