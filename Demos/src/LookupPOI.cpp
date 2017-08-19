/*
  DrawMap - a demo program for libosmscout
  Copyright (C) 2011  Tim Teulings

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

#include <osmscout/Database.h>

#include <osmscout/POIService.h>

#include <osmscout/TypeFeatures.h>

#include <osmscout/util/CmdLineParsing.h>
#include <osmscout/util/GeoBox.h>

struct Arguments
{
  bool                   help;
  std::string            databaseDirectory;
  osmscout::GeoCoord     topLeft;
  osmscout::GeoCoord     bottomRight;
  std::list<std::string> typeNames;

  Arguments()
    : help(false)
  {
    // no code
  }
};


/*
  Example for the nordrhein-westfalen.osm (to be executed in the Demos top
  level directory):

  src/LookupPOI ../TravelJinni/ 51.2 6.5 51.7 8 amenity_hospital amenity_hospital_building
  src/LookupPOI ../TravelJinni/ 51.2 6.5 51.7 8 shop
*/

int main(int argc, char* argv[])
{
  osmscout::CmdLineParser   argParser("LookupPOI",
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
                            args.topLeft=value;
                          }),
                          "TOP_LEFT",
                          "Top left coordinate of search bounding box");

  argParser.AddPositional(osmscout::CmdLineGeoCoordOption([&args](const osmscout::GeoCoord& value) {
                            args.bottomRight=value;
                          }),
                          "BOTTOM_RIGHT",
                          "Bottom right coordinate of search bounding box");

  argParser.AddPositional(osmscout::CmdLineStringListOption([&args](const std::string& value) {
                            args.typeNames.push_back(value);
                          }),
                          "TYPE",
                          "list of search types");

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

  osmscout::DatabaseParameter databaseParameter;
  osmscout::DatabaseRef       database(new osmscout::Database(databaseParameter));
  osmscout::POIServiceRef     poiService(new osmscout::POIService(database));
  osmscout::GeoBox            boundingBox(args.topLeft,
                                          args.bottomRight);

  if (!database->Open(args.databaseDirectory)) {
    std::cerr << "Cannot open database" << std::endl;

    return 1;
  }

  std::cout << "- Search area: " << boundingBox.GetDisplayText() << std::endl;

  osmscout::TypeConfigRef          typeConfig(database->GetTypeConfig());
  osmscout::TypeInfoSet            nodeTypes(*typeConfig);
  osmscout::TypeInfoSet            wayTypes(*typeConfig);
  osmscout::TypeInfoSet            areaTypes(*typeConfig);
  osmscout::NameFeatureLabelReader nameLabelReader(*typeConfig);

  for (const auto &typeName : args.typeNames) {
    osmscout::TypeInfoRef type=typeConfig->GetTypeInfo(typeName);

    if (type->GetIgnore()) {
      std::cerr << "Cannot resolve type name '" << typeName << "'" << std::endl;
      continue;
    }

    std::cout << "- Searching for '" << typeName << "' as";


    if (!type->GetIgnore()) {
      if (type->CanBeNode()) {
        std::cout << " node";
        nodeTypes.Set(type);
      }

      if (type->CanBeWay()) {
        std::cout << " way";
        wayTypes.Set(type);
      }

      if (type->CanBeArea()) {
        std::cout << " area";
        areaTypes.Set(type);
      }
    }

    std::cout << std::endl;
  }

  std::vector<osmscout::NodeRef> nodes;
  std::vector<osmscout::WayRef>  ways;
  std::vector<osmscout::AreaRef> areas;

  if (!poiService->GetPOIsInArea(boundingBox,
                                 nodeTypes,
                                 nodes,
                                 wayTypes,
                                 ways,
                                 areaTypes,
                                 areas)) {
    std::cerr << "Cannot load data from database" << std::endl;

    return 1;
  }

  for (const auto &node : nodes) {
    std::cout << "+ Node " << node->GetFileOffset();
    std::cout << " " << node->GetType()->GetName();
    std::cout << " " << osmscout::UTF8StringToLocaleString(nameLabelReader.GetLabel((node->GetFeatureValueBuffer()))) << std::endl;
  }

  for (const auto &way :ways) {
    std::cout << "+ Way " << way->GetFileOffset();
    std::cout << " " << way->GetType()->GetName();
    std::cout << " " << osmscout::UTF8StringToLocaleString(nameLabelReader.GetLabel(way->GetFeatureValueBuffer())) << std::endl;
  }

  for (const auto &area : areas) {
    std::cout << "+ Area " << area->GetFileOffset();
    std::cout << " " << area->GetType()->GetName();
    std::cout << " " << osmscout::UTF8StringToLocaleString(nameLabelReader.GetLabel(area->rings.front().GetFeatureValueBuffer())) << std::endl;
  }

  return 0;
}
