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

#include <cctype>
#include <iostream>
#include <iomanip>

#include <osmscout/Database.h>
#include <osmscout/LocationService.h>

/**
 * Examples:
 *
 * "is in Address"
 * src/LocationDescription ../maps/nordrhein-westfalen 51.57162 7.45882
 *
 * "is close to address"
 * src/LocationDescription ../maps/nordrhein-westfalen 51.57251 7.46506
 */

int main(int argc, char* argv[])
{
  std::string map;
  double      lon,lat;

  if (argc<4) {
    std::cerr << "LocationDescription <map directory> <lat> <lon>" << std::endl;
    return 1;
  }

  map=argv[1];

  if (sscanf(argv[2],"%lf",&lat)!=1) {
    std::cerr << "lon is not numeric!" << std::endl;
    return 1;
  }

  if (sscanf(argv[3],"%lf",&lon)!=1) {
    std::cerr << "lat is not numeric!" << std::endl;
    return 1;
  }
  osmscout::log.Debug(false);

  osmscout::GeoCoord location(lat,lon);

  osmscout::DatabaseParameter databaseParameter;
  osmscout::DatabaseRef       database(new osmscout::Database(databaseParameter));

  if (!database->Open(map.c_str())) {
    std::cerr << "Cannot open database" << std::endl;

    return 1;
  }

  osmscout::LocationServiceRef locationService(std::make_shared<osmscout::LocationService>(database));

  osmscout::LocationDescription description;

  if (!locationService->DescribeLocation(location,
                                         description)) {
    std::cerr << "Error during generation of location description" << std::endl;
    database->Close();

    return 1;
  }

  osmscout::LocationCoordDescriptionRef coordDescription=description.GetCoordDescription();
  osmscout::LocationAtPlaceDescriptionRef atAddressDescription=description.GetAtAddressDescription();

  if (coordDescription) {
    std::cout << "* Coordinate: " << coordDescription->GetLocation().GetDisplayText() << std::endl;
  }

  if (atAddressDescription) {
    osmscout::Place place = atAddressDescription->GetPlace();
    if (atAddressDescription->IsAtPlace()) {
      std::cout << "* Is at address: " << place.GetDisplayString() << std::endl;
    }
    else {
      std::cout.precision(1);
      std::cout << "* "  << std::fixed << atAddressDescription->GetDistance() << "m ";
      std::cout << osmscout::BearingDisplayString(atAddressDescription->GetBearing());
      std::cout << " of address: " << place.GetDisplayString() << std::endl;
    }
    
    if (place.GetPOI()) {
        std::cout << "  - POI:      " << place.GetPOI()->name << std::endl;
    }

    if (place.GetAddress()) {
        std::cout << "  - address:  " << place.GetAddress()->name << std::endl;
    }

    if (place.GetLocation()) {
        std::cout << "  - location: " << place.GetLocation()->name << std::endl;
    }

    if (place.GetAdminRegion()) {
        std::cout << "  - region:   " << place.GetAdminRegion()->name << std::endl;
    }
  }

  database->Close();

  return 0;
}
