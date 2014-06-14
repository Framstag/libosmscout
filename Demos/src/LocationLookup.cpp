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

#include <osmscout/util/String.h>

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

    path.append(adminRegion->aliasName);
  }

  if (!path.empty()) {
    path.append("/");
  }

  path.append(adminRegion->name);

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

    path.append(parentRegion->name);

    parentRegionOffset=parentRegion->parentRegionOffset;
  }

  return true;
}

std::string GetAdminRegion(const osmscout::LocationServiceRef& locationService,
                           const osmscout::LocationSearchResult::Entry& entry)
{
  std::string label;


  if (entry.adminRegionMatchQuality==osmscout::LocationSearchResult::match) {
    label.append("= ");
  }
  else {
    label.append("~ ");
  }

  if (!entry.adminRegion->aliasName.empty()) {
    label.append(entry.adminRegion->aliasName);
  }
  else {
    label.append(entry.adminRegion->name);
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
  std::string map;
  std::string areaPattern;
  std::string locationPattern;
  std::string addressPattern;

  if (argc<3) {
    std::cerr << "AddressLookup <map directory> [location [address]] <area>" << std::endl;
    return 1;
  }

  map=argv[1];

  std::string searchPattern;

  for (int i=2; i<argc; i++) {
    if (!searchPattern.empty()) {
      searchPattern.append(" ");
    }

    searchPattern.append(argv[i]);
  }

  osmscout::DatabaseParameter databaseParameter;
  osmscout::DatabaseRef       database(new osmscout::Database(databaseParameter));

  if (!database->Open(map.c_str())) {
    std::cerr << "Cannot open database" << std::endl;

    return 1;
  }

  osmscout::LocationServiceRef                            locationService(new osmscout::LocationService(database));
  osmscout::LocationSearch                                search;
  osmscout::LocationSearchResult                          searchResult;
  std::map<osmscout::FileOffset,osmscout::AdminRegionRef> adminRegionMap;
  std::string                                             path;

  search.limit=50;

  search.InitializeSearchEntries(searchPattern);

  if (!locationService->SearchForLocations(search,
                                           searchResult)) {
    std::cerr << "Error while searching for location" << std::endl;
    return false;
  }


  for (std::list<osmscout::LocationSearchResult::Entry>::const_iterator entry=searchResult.results.begin();
      entry!=searchResult.results.end();
      ++entry) {
    if (entry->adminRegion.Valid() &&
        entry->location.Valid() &&
        entry->address.Valid()) {
      if (entry->locationMatchQuality==osmscout::LocationSearchResult::match) {
        std::cout << " = ";
      }
      else {
        std::cout << " ~ ";
      }

      std::cout << entry->location->name;

      if (entry->addressMatchQuality==osmscout::LocationSearchResult::match) {
        std::cout << " = ";
      }
      else {
        std::cout << " ~ ";
      }

      std::cout << entry->address->name << " " << GetAdminRegion(locationService,*entry) << std::endl;

      std::cout << "   * " << GetAdminRegionHierachie(locationService,
                                                      adminRegionMap,
                                                      *entry);
      std::cout << std::endl;

      std::cout << "   - " << entry->address->object.GetTypeName() << " " << entry->address->object.GetFileOffset();

      std::cout << std::endl;
    }
    else if (entry->adminRegion.Valid() &&
             entry->location.Valid()) {
      if (entry->locationMatchQuality==osmscout::LocationSearchResult::match) {
        std::cout << " = ";
      }
      else {
        std::cout << " ~ ";
      }

      std::cout << entry->location->name << " " << GetAdminRegion(locationService,*entry) << std::endl;

      std::cout << "   * " << GetAdminRegionHierachie(locationService,
                                                      adminRegionMap,
                                                      *entry);

      std::cout << std::endl;

      for (std::vector<osmscout::ObjectFileRef>::const_iterator object=entry->location->objects.begin();
          object!=entry->location->objects.end();
          ++object) {
        std::cout << "   - " << object->GetTypeName() << " " << object->GetFileOffset() << std::endl;
      }
    }
    else if (entry->adminRegion.Valid() &&
             entry->poi.Valid()) {
      if (entry->poiMatchQuality==osmscout::LocationSearchResult::match) {
        std::cout << " = ";
      }
      else {
        std::cout << " ~ ";
      }

      std::cout << entry->poi->name << " " << GetAdminRegion(locationService,*entry) << std::endl;

      std::cout << "   * " << GetAdminRegionHierachie(locationService,
                                                      adminRegionMap,
                                                      *entry);

      std::cout << std::endl;

      std::cout << "   - " << entry->poi->object.GetTypeName() << " " << entry->poi->object.GetFileOffset();

      std::cout << std::endl;
    }
    else if (entry->adminRegion.Valid()) {
      std::cout << GetAdminRegion(locationService,*entry) << std::endl;

      std::cout << "   * " << GetAdminRegionHierachie(locationService,
                                                      adminRegionMap,
                                                      *entry);

      std::cout << std::endl;

      if (entry->adminRegion->aliasObject.Valid()) {
        std::cout << "   - " << entry->adminRegion->aliasObject.GetTypeName() << " " << entry->adminRegion->aliasObject.GetFileOffset();
      }
      else {
        std::cout << "   - " << entry->adminRegion->object.GetTypeName() << " " << entry->adminRegion->object.GetFileOffset();
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
