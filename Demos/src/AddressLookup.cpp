/*
  AddressLookup - a demo program for libosmscout
  Copyright (C) 2010  Tim Teulings

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

#include <iostream>
#include <iomanip>

#include <osmscout/Database.h>

const static size_t RESULT_SET_MAX_SIZE = 1000;

int main(int argc, char* argv[])
{
  std::string                      map;
  std::string                      area;
  std::string                      location;
  std::list<osmscout::AdminRegion> areas;
  std::list<osmscout::Location>    locations;
  bool                             limitReached;

  if (argc!=3 && argc!=4) {
    std::cerr << "AddressLookup <map directory> [location] <area>" << std::endl;
    return 1;
  }

  map=argv[1];

  if (argc==4) {
    location=argv[2];
    area=argv[3];
  }
  else {
    area=argv[2];
  }


  osmscout::Database database;

  if (!database.Open(map.c_str())) {
    std::cerr << "Cannot open database" << std::endl;

    return 1;
  }

  if (location.empty()) {
    std::cout << "Looking for area '" << area << "'..." << std::endl;
  }
  else {
    std::cout << "Looking for location '" << location << "' in area '" << area << "'..." << std::endl;
  }


  if (!database.GetMatchingAdminRegions(area,areas,RESULT_SET_MAX_SIZE,limitReached,false)) {
    std::cerr << "Error while accessing database, quitting..." << std::endl;
    database.Close();
    return 1;
  }

  if (limitReached) {
    std::cerr << "To many hits for area, quitting..." << std::endl;
    database.Close();
    return 1;
  }

  for (std::list<osmscout::AdminRegion>::const_iterator area=areas.begin();
      area!=areas.end();
      ++area) {

    if (!location.empty()) {
      if (!database.GetMatchingLocations(*area,location,locations,RESULT_SET_MAX_SIZE,limitReached,false)) {
        std::cerr << "Error while accessing database, quitting..." << std::endl;
        database.Close();
        return 1;
      }

      if (limitReached) {
        std::cerr << "To many hits for area, quitting..." << std::endl;
        database.Close();
        return 1;
      }
    }

    std::cout << "+ " << area->name;

    if (!area->path.empty()) {
      std::cout << " (" << osmscout::StringListToString(area->path) << ")";
    }

    std::cout << " ~ ";

    switch (area->reference.GetType()) {
    case osmscout::refNone:
      std::cout << "<none>";
      break;
    case osmscout::refNode:
      std::cout << "Node";
      break;
    case osmscout::refWay:
      std::cout << "Way";
      break;
    case osmscout::refRelation:
      std::cout << "Relation";
      break;
    }

    std::cout << " " << area->reference.GetId() << std::endl;

    for (std::list<osmscout::Location>::const_iterator location=locations.begin();
        location!=locations.end();
        ++location) {
      std::cout <<"  => " << location->name;

      if (!location->path.empty()) {
        std::cout << " (" << osmscout::StringListToString(location->path) << ")";
      }

      std::cout << std::endl;

      for (std::list<osmscout::ObjectRef>::const_iterator reference=location->references.begin();
          reference!=location->references.end();
          ++reference) {
        std::cout << "     ~ ";

        switch (reference->GetType()) {
        case osmscout::refNone:
          std::cout << "<none>";
          break;
        case osmscout::refNode:
          std::cout << "Node";
          break;
        case osmscout::refWay:
          std::cout << "Way";
          break;
        case osmscout::refRelation:
          std::cout << "Relation";
          break;
        }

        std::cout << " " << reference->GetId() << std::endl;
      }
    }
  }

  database.Close();

  return 0;
}
