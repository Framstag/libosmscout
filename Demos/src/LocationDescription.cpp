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
#include <osmscout/TypeFeatures.h>

#include <osmscout/util/CmdLineParsing.h>

struct Arguments
{
  bool               help;
  std::string        databaseDirectory;
  osmscout::GeoCoord location;

  Arguments()
  : help(false)
  {
    // no code
  }
};

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

void DumpFeatures(const osmscout::FeatureValueBuffer &features, std::string indent="  ")
{
    for (auto featureInstance :features.GetType()->GetFeatures()) {
      if (features.HasFeature(featureInstance.GetIndex())) {
        osmscout::FeatureRef feature=featureInstance.GetFeature();
        std::cout << indent << "+ feature " << feature->GetName();

        if (feature->HasValue()) {
          osmscout::FeatureValue *value=features.GetValue(featureInstance.GetIndex());
          if (feature->HasLabel()) {
            if (!value->GetLabel().empty()) {
              std::cout << ": " << osmscout::UTF8StringToLocaleString(value->GetLabel());
            }
          }
          else {
            // print other values without defined label
            const auto *adminLevel=dynamic_cast<const osmscout::AdminLevelFeatureValue*>(value);
            if (adminLevel!=NULL) {
              std::cout << ": " << (int)adminLevel->GetAdminLevel();
            }
          }
        }
        std::cout << std::endl;
      }
    }
}

void DumpLocationAtPlaceDescription(const std::string& label,
                                    osmscout::LocationAtPlaceDescription& description)
{
  std::cout << label << ":" << std::endl;
  osmscout::Place place = description.GetPlace();
  if (description.IsAtPlace()) {
    std::cout << "  * You are at '" << place.GetDisplayString() << "' (" << place.GetObject().GetTypeName() << ")" << std::endl;
  }
  else {
    std::cout.precision(1);
    std::cout << "  * You are "  << std::fixed << description.GetDistance() << "m ";
    std::cout << osmscout::BearingDisplayString(description.GetBearing());
    std::cout << " of '" << place.GetDisplayString() << "' (" << place.GetObject().GetTypeName() << ")" <<std::endl;
  }

  if (place.GetPOI()) {
    std::cout << "  - POI:      " << osmscout::UTF8StringToLocaleString(place.GetPOI()->name) << std::endl;
    if (place.GetObjectFeatures()){
      std::cout << "  - type:     " << place.GetObjectFeatures()->GetType()->GetName() << std::endl;
    }
  }

  if (place.GetAddress()) {
    std::cout << "  - address:  " << osmscout::UTF8StringToLocaleString(place.GetAddress()->name) << std::endl;
  }

  if (place.GetLocation()) {
    std::cout << "  - location: " << osmscout::UTF8StringToLocaleString(place.GetLocation()->name) << std::endl;
  }

  if (place.GetPostalArea()) {
    std::cout << "  - postal area:  " << osmscout::UTF8StringToLocaleString(place.GetPostalArea()->name) << std::endl;
  }

  if (place.GetAdminRegion()) {
    std::cout << "  - region:   " << osmscout::UTF8StringToLocaleString(place.GetAdminRegion()->name) << std::endl;
  }

  // print all features of this place
  std::cout << std::endl;
  if (place.GetObjectFeatures()) {
    DumpFeatures(*place.GetObjectFeatures());
  }
}

void DumpWayDescription(const std::string& label,
                        osmscout::LocationWayDescription& description)
{
  std::cout << label << ":" << std::endl;

  std::cout.precision(1);
  std::cout << "  * Your are "  << std::fixed << description.GetDistance() << "m";
  std::cout << " away from way:"  << std::endl;

  std::cout << "  - " << description.GetWay().GetDisplayString() << " " << description.GetWay().GetObject().GetName() << std::endl;
  
  // print all features of this place
  std::cout << std::endl;
  if (description.GetWay().GetObjectFeatures()) {
    DumpFeatures(*description.GetWay().GetObjectFeatures());
  }
 
}

void DumpCrossingDescription(const std::string& label,
                             osmscout::LocationCrossingDescription& description)
{
  std::cout << label << ":" << std::endl;
  if (description.IsAtPlace()) {
    std::cout << "  * You are at crossing:" << std::endl;
  }
  else {
    std::cout.precision(1);
    std::cout << "  * Your are "  << std::fixed << description.GetDistance() << "m ";
    std::cout << osmscout::BearingDisplayString(description.GetBearing());
    std::cout << " of crossing:"  << std::endl;
  }

  for (const auto& wayPlace : description.GetWays()) {
    std::cout << "  - " << wayPlace.GetDisplayString() << " " << wayPlace.GetObject().GetName() << std::endl;
  }
}

void DumpParentAdminRegions(const osmscout::LocationServiceRef& locationService,
                            const osmscout::DatabaseRef &database,
                            const osmscout::AdminRegionRef& adminRegion)
{
  if (!adminRegion){
    return;
  }

  std::cout << std::endl;
  std::map<osmscout::FileOffset,osmscout::AdminRegionRef> regions;
  locationService->ResolveAdminRegionHierachie(adminRegion, regions);
  osmscout::FileOffset offset = adminRegion->regionOffset;
  while (offset != 0){
    osmscout::AdminRegionRef region = regions[offset];
    std::cout << "  > ";
    if (offset!=adminRegion->regionOffset){
      std::cout << "parent ";
    }
    std::cout << "region: " << osmscout::UTF8StringToLocaleString(region->name) << std::endl;

    if(region->object.type==osmscout::RefType::refArea){
      osmscout::AreaRef area;
      database->GetAreaByOffset(region->object.offset, area);
      DumpFeatures(area->GetFeatureValueBuffer(), "    ");
    }

    offset = region->parentRegionOffset;
  }
}

int main(int argc, char* argv[])
{
  osmscout::CmdLineParser   argParser("LocationDescription",
                                      argc,argv);
  std::vector<std::string>  helpArgs{"h","help"};
  Arguments                 args;

  argParser.AddOption(osmscout::CmdLineFlag([&args](const bool& value) {
                        args.help=value;
                      }),
                      helpArgs,
                      "Return argument help",
                      true);

  argParser.AddPositional(osmscout::CmdLineStringOption([&args](const std::string& value) {
                            args.databaseDirectory=value;
                          }),
                          "DATABASE",
                          "Directory of the database to use");

  argParser.AddPositional(osmscout::CmdLineGeoCoordOption([&args](const osmscout::GeoCoord& value) {
                            args.location=value;
                          }),
                          "LOCATION",
                          "Geographic coordinate to describe");

  osmscout::CmdLineParseResult result=argParser.Parse();

  if (result.HasError()) {
    std::cerr << "ERROR: " << result.GetErrorDescription() << std::endl;
    std::cout << argParser.GetHelp() << std::endl;
    return 1;
  }
  else if (args.help) {
    std::cout << argParser.GetHelp() << std::endl;
    return 0;
  }

  try {
    std::locale::global(std::locale(""));
  }
  catch (const std::runtime_error& e) {
    std::cerr << "Cannot set locale: \"" << e.what() << "\"" << std::endl;
  }

  osmscout::log.Debug(false);

  osmscout::DatabaseParameter databaseParameter;
  osmscout::DatabaseRef       database(new osmscout::Database(databaseParameter));

  if (!database->Open(args.databaseDirectory)) {
    std::cerr << "Cannot open database" << std::endl;

    return 1;
  }

  osmscout::LocationServiceRef locationService(std::make_shared<osmscout::LocationService>(database));

  osmscout::LocationDescription description;

  if (!locationService->DescribeLocation(args.location,
                                         description)) {
    std::cerr << "Error during generation of location description" << std::endl;
    database->Close();

    return 1;
  }

  osmscout::LocationCoordDescriptionRef    coordDescription=description.GetCoordDescription();
  osmscout::LocationAtPlaceDescriptionRef  atNameDescription=description.GetAtNameDescription();
  osmscout::LocationAtPlaceDescriptionRef  atAddressDescription=description.GetAtAddressDescription();
  osmscout::LocationAtPlaceDescriptionRef  atPOIDescription=description.GetAtPOIDescription();
  osmscout::LocationWayDescriptionRef      wayDescription=description.GetWayDescription();
  osmscout::LocationCrossingDescriptionRef crossingDescription=description.GetCrossingDescription();

  if (coordDescription) {
    std::cout << "* Coordinate: " << coordDescription->GetLocation().GetDisplayText() << std::endl;
  }

  if (atNameDescription) {
    std::cout << std::endl;
    DumpLocationAtPlaceDescription("Nearest namable object",*atNameDescription);
    DumpParentAdminRegions(locationService, database, atNameDescription->GetPlace().GetAdminRegion());
  }

  if (atAddressDescription) {
    std::cout << std::endl;
    DumpLocationAtPlaceDescription("Nearest address",*atAddressDescription);
    DumpParentAdminRegions(locationService, database, atAddressDescription->GetPlace().GetAdminRegion());
  }

  if (atPOIDescription) {
    std::cout << std::endl;
    DumpLocationAtPlaceDescription("Nearest POI",*atPOIDescription);
    DumpParentAdminRegions(locationService, database, atPOIDescription->GetPlace().GetAdminRegion());
  }

  if (wayDescription) {
    std::cout << std::endl;
    DumpWayDescription("Nearest way",*wayDescription);
    DumpParentAdminRegions(locationService, database, wayDescription->GetWay().GetAdminRegion());
  }
  
  if (crossingDescription) {
    std::cout << std::endl;
    DumpCrossingDescription("Nearest crossing",*crossingDescription);
    DumpParentAdminRegions(locationService, database, crossingDescription->GetWays().front().GetAdminRegion());
  }

  database->Close();

  return 0;
}
