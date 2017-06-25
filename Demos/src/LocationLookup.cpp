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
#include <locale>

#include <osmscout/Database.h>
#include <osmscout/LocationService.h>

#include <osmscout/util/CmdLineParsing.h>
#include <osmscout/util/String.h>

struct Arguments
{
  bool                   help;
  std::string            databaseDirectory;
  std::list<std::string> location;

  Arguments()
    : help(false)
  {
    // no code
  }
};

bool GetAdminRegionHierachie(const osmscout::LocationServiceRef& locationService,
                             const osmscout::AdminRegionRef& adminRegion,
                             std::map<osmscout::FileOffset,osmscout::AdminRegionRef>& adminRegionMap,
                             std::string& path)
{
  if (!locationService->ResolveAdminRegionHierachie(adminRegion,
                                                    adminRegionMap)) {
    return false;
  }

  if (!adminRegion->aliasName.empty()) {
    if (!path.empty()) {
      path.append("/");
    }

    path.append(osmscout::UTF8StringToLocaleString(adminRegion->aliasName));
  }

  if (!path.empty()) {
    path.append("/");
  }

  path.append(osmscout::UTF8StringToLocaleString(adminRegion->name));

  osmscout::FileOffset parentRegionOffset=adminRegion->parentRegionOffset;

  while (parentRegionOffset!=0) {
    std::map<osmscout::FileOffset,osmscout::AdminRegionRef>::const_iterator entry=adminRegionMap.find(parentRegionOffset);

    if (entry==adminRegionMap.end()) {
      break;
    }

    osmscout::AdminRegionRef parentRegion=entry->second;

    if (!path.empty()) {
      path.append("/");
    }

    path.append(osmscout::UTF8StringToLocaleString(parentRegion->name));

    parentRegionOffset=parentRegion->parentRegionOffset;
  }

  return true;
}

std::string GetAddress(const osmscout::LocationSearchResult::Entry& entry)
{
  std::string label;

  if (entry.addressMatchQuality==osmscout::LocationSearchResult::match) {
    label="= ";
  }
  else {
    label="~ ";
  }

  label+="Address ("+osmscout::UTF8StringToLocaleString(entry.address->name)+")";

  return label;
}

std::string GetLocation(const osmscout::LocationSearchResult::Entry& entry)
{
  std::string label;

  if (entry.locationMatchQuality==osmscout::LocationSearchResult::match) {
    label="= ";
  }
  else {
    label="~ ";
  }

  label+="Location ("+osmscout::UTF8StringToLocaleString(entry.location->name)+")";

  return label;
}

std::string GetPOI(const osmscout::LocationSearchResult::Entry& entry)
{
  std::string label;

  if (entry.poiMatchQuality==osmscout::LocationSearchResult::match) {
    label="= ";
  }
  else {
    label=" ~ ";
  }

  label+="POI ("+osmscout::UTF8StringToLocaleString(entry.poi->name)+")";

  return label;
}

std::string GetPostalArea(const osmscout::LocationSearchResult::Entry& entry)
{
  std::string label;

  if (entry.postalAreaMatchQuality==osmscout::LocationSearchResult::match) {
    label="= ";
  }
  else {
    label="~ ";
  }

  label+="PostalArea ("+osmscout::UTF8StringToLocaleString(entry.postalArea->name)+")";

  return label;
}

std::string GetAdminRegion(const osmscout::LocationSearchResult::Entry& entry)
{
  std::string label;

  if (entry.adminRegionMatchQuality==osmscout::LocationSearchResult::match) {
    label.append("= ");
  }
  else {
    label.append("~ ");
  }

  if (!entry.adminRegion->aliasName.empty()) {
    label.append(osmscout::UTF8StringToLocaleString(entry.adminRegion->aliasName));
  }
  else {
    label.append(osmscout::UTF8StringToLocaleString(entry.adminRegion->name));
  }

  return label;
}

std::string GetObject(const osmscout::DatabaseRef& database,
                      const osmscout::ObjectFileRef& object)
{
  std::string label;

  label=object.GetTypeName();
  label+=" ";
  label+=osmscout::NumberToString(object.GetFileOffset());

  if (object.GetType()==osmscout::RefType::refNode) {
    osmscout::NodeRef node;

    if (database->GetNodeByOffset(object.GetFileOffset(),
                                  node)) {
      label+=" ";
      label+=node->GetType()->GetName();
    }
  }
  else if (object.GetType()==osmscout::RefType::refArea) {
    osmscout::AreaRef area;

    if (database->GetAreaByOffset(object.GetFileOffset(),
                                  area)) {
      label+=" ";
      label+=area->GetType()->GetName();
    }
  }
  else if (object.GetType()==osmscout::RefType::refWay) {
    osmscout::WayRef way;

    if (database->GetWayByOffset(object.GetFileOffset(),
                                 way)) {
      label+=" ";
      label+=way->GetType()->GetName();
    }
  }

  return label;
}

std::string GetAdminRegionHierachie(const osmscout::LocationServiceRef& locationService,
                                    std::map<osmscout::FileOffset,osmscout::AdminRegionRef>& adminRegionMap,
                                    const osmscout::LocationSearchResult::Entry& entry)
{
  std::string path;

  if (!GetAdminRegionHierachie(locationService,
                               entry.adminRegion,
                               adminRegionMap,
                               path)) {
    return "";
  }

  return path;
}

int main(int argc, char* argv[])
{
  osmscout::CmdLineParser   argParser("LocationLookup",
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

  argParser.AddPositional(osmscout::CmdLineStringListOption([&args](const std::string& value) {
                            args.location.push_back(value);
                          }),
                          "LOCATION",
                          "list of location search attributes");

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

  /*
  osmscout::log.Debug(true);
  osmscout::log.Info(true);
  osmscout::log.Warn(true);
  osmscout::log.Error(true);*/

  try {
    std::locale::global(std::locale(""));
  }
  catch (const std::runtime_error& e) {
    std::cerr << "Cannot set locale: \"" << e.what() << "\"" << std::endl;
  }

  std::string searchPattern;

  for (const auto& location : args.location) {
    if (!searchPattern.empty()) {
      searchPattern.append(" ");
    }

    searchPattern.append(location);
  }

  std::cout << "Searching for pattern \"" <<searchPattern  << "\"" << std::endl;

  osmscout::DatabaseParameter databaseParameter;
  osmscout::DatabaseRef       database(new osmscout::Database(databaseParameter));

  if (!database->Open(args.databaseDirectory)) {
    std::cerr << "Cannot open database" << std::endl;

    return 1;
  }

  osmscout::LocationServiceRef                            locationService=std::make_shared<osmscout::LocationService>(database);
  osmscout::LocationSearch                                search;
  osmscout::LocationSearchResult                          searchResult;
  std::map<osmscout::FileOffset,osmscout::AdminRegionRef> adminRegionMap;

  search.limit=50;

  if (!locationService->InitializeLocationSearchEntries(osmscout::LocaleStringToUTF8String(searchPattern),
                                                        search)) {
    std::cerr << "Error while parsing search string" << std::endl;
    return false;
  }

  if (!locationService->SearchForLocations(search,
                                           searchResult)) {
    std::cerr << "Error while searching for location" << std::endl;
    return false;
  }


  for (const auto &entry : searchResult.results) {
    if (entry.adminRegion &&
        entry.location &&
        entry.address) {
      std::cout << GetLocation(entry) << " ";
      std::cout << GetAddress(entry) << " ";
      std::cout << GetPostalArea(entry) << " ";
      std::cout << GetAdminRegion(entry) << std::endl;

      std::cout << "   * " << GetAdminRegionHierachie(locationService,
                                                      adminRegionMap,
                                                      entry);
      std::cout << std::endl;

      std::cout << "   - " << GetObject(database,entry.address->object);

      std::cout << std::endl;
    }
    else if (entry.adminRegion &&
             entry.location) {
      std::cout << GetLocation(entry) << " ";
      std::cout << GetPostalArea(entry) << " ";
      std::cout << GetAdminRegion(entry) << std::endl;

      std::cout << "   * " << GetAdminRegionHierachie(locationService,
                                                      adminRegionMap,
                                                      entry);

      std::cout << std::endl;

      for (const auto &object : entry.location->objects) {
        std::cout << "   - " << GetObject(database,object) << std::endl;
      }
    }
    else if (entry.adminRegion &&
             entry.poi) {
      std::cout << GetPOI(entry) << " ";
      std::cout << GetAdminRegion(entry) << std::endl;

      std::cout << "   * " << GetAdminRegionHierachie(locationService,
                                                      adminRegionMap,
                                                      entry);

      std::cout << std::endl;

      std::cout << "   - " << GetObject(database,entry.poi->object);

      std::cout << std::endl;
    }
    else if (entry.adminRegion) {
      std::cout << GetAdminRegion(entry) << std::endl;

      std::cout << "   * " << GetAdminRegionHierachie(locationService,
                                                      adminRegionMap,
                                                      entry);

      std::cout << std::endl;

      if (entry.adminRegion->aliasObject.Valid()) {
        std::cout << "   - " << GetObject(database,entry.adminRegion->aliasObject);
      }
      else {
        std::cout << "   - " << GetObject(database,entry.adminRegion->object);
      }

      std::cout << std::endl;
    }
  }

  if (searchResult.limitReached) {
    std::cout << "<limit reached!>" << std::endl;
  }

  database->Close();

  return 0;
}
