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
 *
 * "is close to POI"
 * src/LocationDescription ../maps/nordrhein-westfalen 51.5142273, 7.4652789
 *
 * "is close/at named object"
 * src/LocationDescription ../maps/nordrhein-westfalen 51.49300 7.48255
 */

void DumpLocationAtPlaceDescription(osmscout::LocationAtPlaceDescription& description)
{
  osmscout::Place place = description.GetPlace();
  if (description.IsAtPlace()) {
    std::cout << "* Is at address: " << place.GetDisplayString() << std::endl;
  }
  else {
    std::cout.precision(1);
    std::cout << "* "  << std::fixed << description.GetDistance() << "m ";
    std::cout << osmscout::BearingDisplayString(description.GetBearing());
    std::cout << " of address: " << place.GetDisplayString() << std::endl;
  }

  if (place.GetPOI()) {
    std::cout << "  - POI:      " << place.GetPOI()->name << std::endl;
    if (place.GetObjectFeatures()){
      std::cout << "  - type:     " << place.GetObjectFeatures()->GetType()->GetName() << std::endl;
    }
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
  
  // print all features of this place
  std::cout << std::endl;
  if (place.GetObjectFeatures()){
    for (auto featureInstance :place.GetObjectFeatures()->GetType()->GetFeatures()){
      if (place.GetObjectFeatures()->HasFeature(featureInstance.GetIndex())){
        osmscout::FeatureRef feature=featureInstance.GetFeature();
        if (feature->HasValue() && feature->HasLabel()){
          osmscout::FeatureValue *value=place.GetObjectFeatures()->GetValue(featureInstance.GetIndex());
          if (value->GetLabel().empty()){
            std::cout << "  + feature " << feature->GetName() << std::endl;
          }else{
            std::cout << "  + feature " << feature->GetName() << ": " << value->GetLabel() << std::endl;
          }
        }else{
          std::cout << "  + feature " << feature->GetName() << std::endl;
        }
      }
    }
  }
}

void DumpParentAdminRegions(const osmscout::LocationServiceRef& locationService,
                            const osmscout::AdminRegionRef& adminRegion)
{
  if (!adminRegion){
    return;
  }

  std::cout << std::endl;
  std::map<osmscout::FileOffset,osmscout::AdminRegionRef> regions;
  locationService->ResolveAdminRegionHierachie(adminRegion, regions);
  osmscout::FileOffset parentOffset = adminRegion->parentRegionOffset;
  while (parentOffset != 0){
    osmscout::AdminRegionRef region = regions[parentOffset];
    std::cout << "  > parent region: " << region->name << std::endl;
    parentOffset = region->parentRegionOffset;
  }
}

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
  osmscout::LocationAtPlaceDescriptionRef atNameDescription=description.GetAtNameDescription();
  osmscout::LocationAtPlaceDescriptionRef atAddressDescription=description.GetAtAddressDescription();
  osmscout::LocationAtPlaceDescriptionRef atPOIDescription=description.GetAtPOIDescription();

  if (coordDescription) {
    std::cout << "* Coordinate: " << coordDescription->GetLocation().GetDisplayText() << std::endl;
  }

  if (atNameDescription) {
    DumpLocationAtPlaceDescription(*atNameDescription);
    DumpParentAdminRegions(locationService, atNameDescription->GetPlace().GetAdminRegion());
  }

  if (atAddressDescription) {
    DumpLocationAtPlaceDescription(*atAddressDescription);
    DumpParentAdminRegions(locationService, atAddressDescription->GetPlace().GetAdminRegion());
  }

  if (atPOIDescription) {
    DumpLocationAtPlaceDescription(*atPOIDescription);
    DumpParentAdminRegions(locationService, atPOIDescription->GetPlace().GetAdminRegion());
  }

  database->Close();

  return 0;
}
