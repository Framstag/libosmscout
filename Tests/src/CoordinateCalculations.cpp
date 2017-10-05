/*
  CoordinateCalculations - a test program for libosmscout
  Copyright (C) 2015  Tim Teulings

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
#include <limits>
#include <vector>

#include <osmscout/GeoCoord.h>

int main(int /*argc*/, char** /*argv*/)
{
  // Distance:
  //    43135.331925721744 m
  //    43.135331925721744 km
  //    23.291215942614333 nm
  osmscout::GeoCoord location1(51.43170928, 6.80131361);
  osmscout::GeoCoord location2(51.48510151, 7.4160216);
  double distance_km = location2 - location1;
  std::cout << "Distance between " << location1.GetDisplayText() << " and " << location2.GetDisplayText() << ": " << distance_km << " km" << std::endl;

  // Target:
  //    Latitude: 51°27'48" N (51.463397)
  //    Longitude: 7°0'22" E (7.006078)
  double distance = 14665.298166863819; // [m]
  double angle = 76.010085273091411718093847668127; // [deg]
  osmscout::GeoCoord target = location1.Add(angle, distance);
  std::cout << "Go " << distance << " m in direction " << angle << " degree: " << target.GetDisplayText() << std::endl;

  return 0;
}
