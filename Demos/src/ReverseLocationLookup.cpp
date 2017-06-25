/*
  ReverseLocationLookup - a demo program for libosmscout
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
#include <cstring>
#include <iostream>

#include <osmscout/Database.h>
#include <osmscout/LocationService.h>

#include <osmscout/util/String.h>

int main(int argc, char* argv[])
{
  std::string                        map;
  std::list<osmscout::ObjectFileRef> objects;

  if (argc<4 || argc%2!=0) {
    std::cerr << "ReverseLocationLookup <map directory> <ObjectType> <FileOffset>..." << std::endl;
    return 1;
  }

  map=argv[1];

  std::string searchPattern;

  int argIndex=2;
  while (argIndex<argc) {
    osmscout::RefType    objectType=osmscout::refNone;
    osmscout::FileOffset offset=0;

    if (strcmp("Node",argv[argIndex])==0) {
      objectType=osmscout::refNode;
    }
    else if (strcmp("Area",argv[argIndex])==0) {
      objectType=osmscout::refArea;
    }
    else if (strcmp("Way",argv[argIndex])==0) {
      objectType=osmscout::refWay;
    }
    else {
      std::cerr << "Error: ObjectType must be one of 'Node', 'Area' or 'Way'" << std::endl;

      return 1;
    }

    argIndex++;

    if (!osmscout::StringToNumber(argv[argIndex],
                                  offset)) {
      std::cerr << "Error: '" << argv[argIndex] << "' cannot be parsed to a file offset" << std::endl;

      return 1;
    }

    argIndex++;

    objects.push_back(osmscout::ObjectFileRef(offset,
                                              objectType));
  }

  osmscout::DatabaseParameter databaseParameter;
  osmscout::DatabaseRef       database=std::make_shared<osmscout::Database>(databaseParameter);

  if (!database->Open(map.c_str())) {
    std::cerr << "Cannot open database" << std::endl;

    return 1;
  }

  osmscout::LocationServiceRef locationService=std::make_shared<osmscout::LocationService>(database);

  std::list<osmscout::LocationService::ReverseLookupResult> result;

  if (locationService->ReverseLookupObjects(objects,
                                            result)) {
    for (const auto& entry : result) {
      std::cout << entry.object.GetTypeName() << " " << entry.object.GetFileOffset() << " matches";

      if (entry.adminRegion) {
        std::cout << " region";

        if (entry.postalArea) {
          std::cout << " " << entry.postalArea->name;
        }

        std::cout << " '" << entry.adminRegion->name << "'";
      }

      if (entry.poi) {
        std::cout << " poi '" << entry.poi->name << "'";
      }

      if (entry.location) {
        std::cout << " location '" << entry.location->name << "'";
      }

      if (entry.address) {
        std::cout << " address '" << entry.address->name << "'";
      }

      std::cout << std::endl;
    }
  }
  else {
    std::cerr << "Error while reverse lookup" << std::endl;
  }

  database->Close();

  return 0;
}
