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

bool GetAdminRegionHierachie(const osmscout::Database &database,
                             const osmscout::AdminRegionRef& adminRegion,
                             std::map<osmscout::FileOffset,osmscout::AdminRegionRef>& adminRegionMap,
                             std::string& path)
{
  if (!database.ResolveAdminRegionHierachie(adminRegion,
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

std::string GetAdminRegionLabel(const osmscout::Database &database,
                                std::map<osmscout::FileOffset,osmscout::AdminRegionRef>& adminRegionMap,
                                const osmscout::LocationSearchResult::Entry& entry)
{
  std::string label;
  std::string path;


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

  if (!GetAdminRegionHierachie(database,
                               entry.adminRegion,
                               adminRegionMap,
                               path)) {
    return false;
  }

  if (!path.empty()) {
    label.append(" (");
    label.append(path);
    label.append(")");
  }

  return label;
}

int main(int argc, char* argv[])
{
  std::string map;
  std::string areaPattern;
  std::string locationPattern;
  std::string addressPattern;

  if (argc!=3 && argc!=4 && argc!=5) {
    std::cerr << "AddressLookup <map directory> [location [address]] <area>" << std::endl;
    return 1;
  }

  map=argv[1];

  if (argc==5) {
    locationPattern=argv[2];
    addressPattern=argv[3];
    areaPattern=argv[4];
  }
  else if (argc==4) {
    locationPattern=argv[2];
    areaPattern=argv[3];
  }
  else {
    areaPattern=argv[2];
  }


  osmscout::DatabaseParameter databaseParameter;
  osmscout::Database          database(databaseParameter);

  if (!database.Open(map.c_str())) {
    std::cerr << "Cannot open database" << std::endl;

    return 1;
  }

  osmscout::LocationSearch                                search;
  osmscout::LocationSearchResult                          searchResult;
  std::map<osmscout::FileOffset,osmscout::AdminRegionRef> adminRegionMap;
  std::string                                             path;

  search.regionPattern=areaPattern;
  search.locationPattern=locationPattern;
  search.addressPattern=addressPattern;
  search.limit=50;

  if (!database.SearchForLocations(search,
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

      std::cout << entry->address->name;

      std::cout << " " << GetAdminRegionLabel(database,
                                              adminRegionMap,
                                              *entry);

      std::cout << " " << entry->address->object.GetTypeName() << " " << entry->address->object.GetFileOffset();

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

      std::cout << entry->location->name;

      std::cout << " " << GetAdminRegionLabel(database,
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

      std::cout << entry->poi->name;

      std::cout << " " << GetAdminRegionLabel(database,
                                              adminRegionMap,
                                              *entry);

      std::cout << " " << entry->poi->object.GetTypeName() << " " << entry->poi->object.GetFileOffset();

      std::cout << std::endl;
    }
    else if (entry->adminRegion.Valid()) {
      std::cout << " " << GetAdminRegionLabel(database,
                                              adminRegionMap,
                                              *entry);

      if (entry->adminRegion->aliasReference.Valid()) {
        std::cout << " " << entry->adminRegion->aliasReference.GetTypeName() << " " << entry->adminRegion->aliasReference.GetFileOffset();
      }
      else {
        std::cout << " " << entry->adminRegion->object.GetTypeName() << " " << entry->adminRegion->object.GetFileOffset();
      }

      std::cout << std::endl;
    }
  }

  if (searchResult.limitReached) {
    std::cout << "<limit reached!>" << std::endl;
  }

  database.Close();

  return 0;
}
