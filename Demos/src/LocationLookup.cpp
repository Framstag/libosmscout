/*
  LocationLookup - a demo program for libosmscout
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
#include <osmscout/LocationDescriptionService.h>

#include <osmscout/util/CmdLineParsing.h>
#include <osmscout/util/String.h>

struct Arguments
{
  bool                   help=false;
  std::string            databaseDirectory;
  std::string            defaultAdminRegion;
  bool                   searchForLocation=true;
  bool                   searchForPOI=true;
  bool                   adminRegionOnlyMatch=false;
  bool                   poiOnlyMatch=false;
  bool                   locationOnlyMatch=false;
  bool                   addressOnlyMatch=true;
  bool                   partialMatch=false;
  size_t                 limit=30;
  size_t                 repeat=1;
  std::list<std::string> location;
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
    auto entry=adminRegionMap.find(parentRegionOffset);

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
    label="~ ";
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
    label+="Region ("+osmscout::UTF8StringToLocaleString(entry.adminRegion->aliasName)+")";
  }
  else {
    label+="Region ("+osmscout::UTF8StringToLocaleString(entry.adminRegion->name)+")";
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
                                    const osmscout::LocationSearchResult::Entry& entry,
                                    std::map<osmscout::FileOffset,osmscout::AdminRegionRef>& adminRegionMap)
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

void DumpResult(const osmscout::DatabaseRef& database,
                const osmscout::LocationServiceRef& locationService,
                const osmscout::LocationSearchResult& searchResult)
{
  std::map<osmscout::FileOffset,osmscout::AdminRegionRef> adminRegionMap;

  for (const auto&entry : searchResult.results) {
    if (entry.adminRegion &&
        entry.location &&
        entry.address) {
      std::cout << GetLocation(entry) << " ";
      std::cout << GetAddress(entry) << " ";
      std::cout << GetPostalArea(entry) << " ";
      std::cout << GetAdminRegion(entry) << std::endl;

      std::cout << "   * " << GetAdminRegionHierachie(locationService,
                                                      entry,
                                                      adminRegionMap);
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
                                                      entry,
                                                      adminRegionMap);

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
                                                      entry,
                                                      adminRegionMap);

      std::cout << std::endl;

      std::cout << "   - " << GetObject(database,entry.poi->object);

      std::cout << std::endl;
    }
    else if (entry.adminRegion) {
      std::cout << GetAdminRegion(entry) << std::endl;

      std::cout << "   * " << GetAdminRegionHierachie(locationService,
                                                      entry,
                                                      adminRegionMap);

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

  argParser.AddOption(osmscout::CmdLineBoolOption([&args](bool value) {
                        args.searchForLocation=value;
                      }),
                      "location",
                      "Search for a location");

  argParser.AddOption(osmscout::CmdLineBoolOption([&args](bool value) {
                        args.searchForPOI=value;
                      }),
                      "poi",
                      "Search for a point of interest (POI)");

  argParser.AddOption(osmscout::CmdLineStringOption([&args](const std::string& value) {
                        args.defaultAdminRegion=value;
                      }),
                      "adminRegion",
                      "The default admin region to search in");

  argParser.AddOption(osmscout::CmdLineBoolOption([&args](bool value) {
                        args.adminRegionOnlyMatch=value;
                      }),
                      "adminRegionOnlyMatch",
                      "Return only exact matches for the admin region");

  argParser.AddOption(osmscout::CmdLineBoolOption([&args](bool value) {
                        args.poiOnlyMatch=value;
                      }),
                      "poiOnlyMatch",
                      "Return only exact matches for the POI");

  argParser.AddOption(osmscout::CmdLineBoolOption([&args](bool value) {
                        args.locationOnlyMatch=value;
                      }),
                      "locationOnlyMatch",
                      "Return only exact matches for the location");

  argParser.AddOption(osmscout::CmdLineBoolOption([&args](bool value) {
                        args.addressOnlyMatch=value;
                      }),
                      "addressOnlyMatch",
                      "Return only exact matches for the address");

  argParser.AddOption(osmscout::CmdLineBoolOption([&args](bool value) {
                        args.partialMatch=value;
                      }),
                      "partialMatch",
                      "Return only matches that match the complete search string");

  argParser.AddOption(osmscout::CmdLineSizeTOption([&args](size_t value) {
                        args.limit=value;
                      }),
                      "limit",
                      "Maximum number of results");

  argParser.AddOption(osmscout::CmdLineSizeTOption([&args](size_t value) {
                        args.repeat=value;
                      }),
                      "repeat",
                      "Cout of repeat for performance test");

  argParser.AddPositional(osmscout::CmdLineStringOption([&args](const std::string& value) {
                            args.databaseDirectory=value;
                          }),
                          "DATABASE",
                          "Directory of the database to use");

  argParser.AddPositional(osmscout::CmdLineStringListOption([&args](const std::string& value) {
                            args.location.push_back(value);
                          }),
                          "LOCATION",
                          "List of location search attributes");

  osmscout::CmdLineParseResult result=argParser.Parse();

  if (result.HasError()) {
    std::cerr << "ERROR: " << result.GetErrorDescription() << std::endl;
    std::cout << argParser.GetHelp() << std::endl;
    return 1;
  }

  if (args.help) {
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

  osmscout::DatabaseParameter databaseParameter;
  osmscout::DatabaseRef       database(new osmscout::Database(databaseParameter));

  if (!database->Open(args.databaseDirectory)) {
    std::cerr << "Cannot open database" << std::endl;

    return 1;
  }

  osmscout::StringMatcherFactoryRef matcherFactory=std::make_shared<osmscout::StringMatcherCIFactory>();
  osmscout::LocationServiceRef     locationService=std::make_shared<osmscout::LocationService>(database);

  osmscout::LocationStringSearchParameter searchParameter(osmscout::LocaleStringToUTF8String(searchPattern));

  searchParameter.SetSearchForLocation(args.searchForLocation);
  searchParameter.SetSearchForPOI(args.searchForPOI);
  searchParameter.SetAdminRegionOnlyMatch(args.adminRegionOnlyMatch);
  searchParameter.SetPOIOnlyMatch(args.poiOnlyMatch);
  searchParameter.SetLocationOnlyMatch(args.locationOnlyMatch);
  searchParameter.SetAddressOnlyMatch(args.addressOnlyMatch);
  searchParameter.SetPartialMatch(args.partialMatch);
  searchParameter.SetStringMatcherFactory(matcherFactory);
  searchParameter.SetLimit(args.limit);

  if (!args.defaultAdminRegion.empty()) {
    osmscout::StopClock                   adminRegionSearchTime;
    osmscout::LocationFormSearchParameter patternSearchParams;
    osmscout::LocationSearchResult        adminRegionSearchResult;

    patternSearchParams.SetStringMatcherFactory(matcherFactory);
    patternSearchParams.SetAdminRegionSearchString(args.defaultAdminRegion);
    patternSearchParams.SetLimit(50);

    if (!locationService->SearchForLocationByForm(patternSearchParams,
                                                  adminRegionSearchResult)) {
      std::cerr << "Error while resolving default admin region" << std::endl;
      return 1;
    }

    for (const auto& adminRegionResult : adminRegionSearchResult.results) {
      if (adminRegionResult.adminRegion &&
          adminRegionResult.adminRegionMatchQuality==osmscout::LocationSearchResult::match) {
        searchParameter.SetAdminRegionOnlyMatch(true);
        searchParameter.SetDefaultAdminRegion(adminRegionResult.adminRegion);
        break;
      }
    }

    adminRegionSearchTime.Stop();

    std::cout << "Admin region search time: " << adminRegionSearchTime.ResultString() << std::endl;
    std::cout << std::endl;
  }

  std::cout << "Database:                " << args.databaseDirectory << std::endl;
  std::cout << "Search pattern:          " << searchParameter.GetSearchString() << std::endl;
  std::cout << "Search for location:     " << (searchParameter.GetSearchForLocation() ? "true" : "false") << std::endl;
  std::cout << "Search for POI:          " << (searchParameter.GetSearchForPOI() ? "true" : "false") << std::endl;
  std::cout << "Admin region only match: " << (searchParameter.GetAdminRegionOnlyMatch() ? "true" : "false") << std::endl;
  std::cout << "POI only match:          " << (searchParameter.GetPOIOnlyMatch() ? "true" : "false") << std::endl;
  std::cout << "Location only match:     " << (searchParameter.GetLocationOnlyMatch() ? "true" : "false") << std::endl;
  std::cout << "Address only match:      " << (searchParameter.GetAddressOnlyMatch() ? "true" : "false") << std::endl;
  std::cout << "Partial match:           " << (searchParameter.GetPartialMatch() ? "true" : "false") << std::endl;

  if (searchParameter.GetDefaultAdminRegion()) {
    std::cout << "Default admin region:    " << searchParameter.GetDefaultAdminRegion()->name << " (" << searchParameter.GetDefaultAdminRegion()->object.GetName() << ")" << std::endl;
  }

  std::cout << "Limit:                   " << searchParameter.GetLimit() << std::endl;

  std::cout << std::endl;

  osmscout::StopClock locationSearchTime;

  for (size_t i=0; i<args.repeat; i++) {
    osmscout::LocationSearchResult   searchResult;

    if (!locationService->SearchForLocationByString(searchParameter,
                                                    searchResult)) {
      std::cerr << "Error while searching for location" << std::endl;
      return 1;
    }

    if (args.repeat==1) {
      DumpResult(database,
                 locationService,
                 searchResult);
    }
  }

  locationSearchTime.Stop();
  std::cout << std::endl;
  std::cout << "Location search time: " << locationSearchTime.ResultString() << std::endl;

  database->Close();

  return 0;
}

