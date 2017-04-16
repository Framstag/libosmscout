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
#include <iostream>

#include <osmscout/util/GeoBox.h>

using namespace std;

int main(int /*argc*/, char** /*argv*/)
{
  osmscout::GeoBox a(osmscout::GeoCoord(0,0),osmscout::GeoCoord(1,1));
  osmscout::GeoBox b;  
  
  std::cout << "Invalid boxes should not intersects... ";
  if (!a.IsValid() ||
      b.IsValid() ||
      a.Intersects(b)){
    std::cout << "Failure" << std::endl;
    return 1;
  }
  std::cout << "OK" << std::endl;

  std::cout << "Intersection of touching boxes is not commutative by default... ";
  b.Set(osmscout::GeoCoord(0,1),osmscout::GeoCoord(1,2));
  if (a.Intersects(b) ||
      !b.Intersects(a)){
    std::cout << "Failure" << std::endl;
    return 2;
  }
  std::cout << "OK" << std::endl;

  std::cout << "Intersection of touching boxes is commutative with close interval... ";
  if (!a.Intersects(b,/*openInterval*/false) ||
      !b.Intersects(a,/*openInterval*/false)){
    std::cout << "Failure" << std::endl;
    return 3;
  }
  std::cout << "OK" << std::endl;

  return 0;
}
