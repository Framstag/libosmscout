/*
  This source is part of the libosmscout library
  Copyright (C) 2009  Tim Teulings

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#include <osmscout/CityStreetIndex.h>

#include <cassert>
#include <iostream>

#include <osmscout/FileScanner.h>
#include <osmscout/Util.h>

namespace osmscout {

  CityStreetIndex::CityStreetIndex()
   : regionLoaded(false),
     hashFunction(NULL)
  {
    // no code
  }

  CityStreetIndex::~CityStreetIndex()
  {
    // no code
  }

  bool CityStreetIndex::LoadRegion(FileScanner& scanner) const
  {
    std::string name;
    FileOffset  offset;
    FileOffset  parentOffset;
    uint32_t    childrenCount;
    uint32_t    nodeCount;
    uint32_t    wayCount;

    if (!scanner.GetPos(offset)) {
      return false;
    }

    scanner.Read(name);
    scanner.ReadNumber(parentOffset);

    scanner.ReadNumber(childrenCount);
    for (size_t i=0; i<childrenCount; i++) {
      if (!LoadRegion(scanner)) {
        return false;
      }
    }

    scanner.ReadNumber(nodeCount);
    for (size_t i=0; i<nodeCount; i++) {
      std::string name;
      uint32_t    idCount;
      Id          lastId=0;

      scanner.Read(name);
      scanner.ReadNumber(idCount);

      for (size_t j=0; j<idCount; j++) {
        Id id;

        scanner.ReadNumber(id);

        locations[name].offset=offset;
        locations[name].nodes.push_back(id+lastId);
        lastId=id;
      }
    }

    scanner.ReadNumber(wayCount);
    for (size_t i=0; i<wayCount; i++) {
      std::string name;
      uint32_t    idCount;
      Id          lastId=0;

      scanner.Read(name);
      scanner.ReadNumber(idCount);

      for (size_t j=0; j<idCount; j++) {
        Id id;

        scanner.ReadNumber(id);

        locations[name].offset=offset;
        locations[name].ways.push_back(id+lastId);
        lastId=id;
      }
    }

    return true;
  }

  bool CityStreetIndex::LoadRegion(FileScanner& scanner,
                                   FileOffset offset) const
  {
    locations.clear();

    scanner.SetPos(offset);

    LoadRegion(scanner);

    regionLoaded=true;

    return !scanner.HasError();
  }

  bool CityStreetIndex::Load(const std::string& path,
                             std::string (*hashFunction) (std::string))
  {
    FileScanner   scanner;
    std::string   file=path+"/"+"nameregion.idx";

    this->path=path;
    this->hashFunction=hashFunction;

    if (!scanner.Open(file)) {
      std::cerr << "Cannot open file '" << file << "'!" << std::endl;
      return false;
    }

    uint32_t areaRefs;

    if (!scanner.ReadNumber(areaRefs)) {
      return false;
    }

    for (size_t i=0; i<areaRefs; i++) {
      std::string name;
      uint32_t    entries;

      if (!scanner.Read(name)) {
        return false;
      }

      if (!scanner.ReadNumber(entries)) {
        return false;
      }

      for (size_t j=0; j<entries; j++) {
        Region   region;
        uint32_t type;

        region.name=name;

        if (!scanner.ReadNumber(type)) {
          return false;
        }

        region.reference.type=(RefType)type;

        if (!scanner.ReadNumber(region.reference.id)) {
          return false;
        }

        if (!scanner.ReadNumber(region.offset)) {
          return false;
        }

        // if the user has supplied a hash function then use it to generate a hash value
        if (hashFunction) {
          region.hash=(*hashFunction)(region.name);
        }

        areas.push_back(region);
      }
    }

    return !scanner.HasError() && scanner.Close();
  }


  bool CityStreetIndex::GetMatchingAdminRegions(const std::string& name,
                                                std::list<AdminRegion>& regions,
                                                size_t limit,
                                                bool& limitReached,
                                                bool startWith) const
  {
    std::string nameHash;

    limitReached=false;
    regions.clear();

    // if the user supplied a special hash function call it and use the result
    if (hashFunction) {
      nameHash=(*hashFunction)(name);
    }

    for (std::list<Region>::const_iterator area=this->areas.begin();
         area!=this->areas.end() && !limitReached;
         ++area) {
      bool                   found=false;
      std::string::size_type loc;

      if (hashFunction && !area->hash.empty()) {
        loc=area->hash.find(nameHash);
      }
      else {
        loc=area->name.find(name);
      }

      if (startWith) {
        found=loc==0;
      }
      else {
        found=loc!=std::string::npos;
      }

      if (found) {
        if (regions.size()>=limit) {
          limitReached=true;
        }
        else {
          AdminRegion adminRegion;

          adminRegion.reference=area->reference;
          adminRegion.offset=area->offset;
          adminRegion.name=area->name;
          adminRegion.hash=area->hash;

          regions.push_back(adminRegion);
        }
      }
    }

    if (regions.size()==0) {
      return true;
    }

    FileScanner scanner;
    std::string file=path+"/"+"region.dat";

    if (!scanner.Open(file)) {
      std::cerr << "Cannot open file '" << file << "'!" << std::endl;
      return false;
    }

    for (std::list<AdminRegion>::iterator area=regions.begin();
         area!=regions.end();
         ++area) {
      FileOffset offset=area->offset;

      while (offset!=0) {
        std::string name;

        scanner.SetPos(offset);
        scanner.Read(name);
        scanner.ReadNumber(offset);

        if (area->path.empty()) {
          if (name!=area->name) {
            area->path.push_back(name);
          }
        }
        else {

          area->path.push_back(name);
        }
      }
    }

    return !scanner.HasError() && scanner.Close();
  }

  bool CityStreetIndex::GetMatchingLocations(const AdminRegion& region,
                                             const std::string& name,
                                             std::list<Location>& locations,
                                             size_t limit,
                                             bool& limitReached,
                                             bool startWith) const
  {
    std::string nameHash;

    limitReached=false;
    locations.clear();

    FileScanner scanner;
    std::string file=path+"/"+"region.dat";

    if (!scanner.Open(file)) {
      std::cerr << "Cannot open file '" << file << "'!" << std::endl;
      return false;
    }

    if (!regionLoaded || this->region!=region.offset) {
      if (!LoadRegion(scanner,region.offset) || !regionLoaded) {
        return false;
      }
    }

    // if the user supplied a special hash function call it and use the result
    if (hashFunction) {
      nameHash=(*hashFunction)(name);
    }

    for (std::map<std::string,Loc>::const_iterator l=this->locations.begin();
         l!=this->locations.end() && !limitReached;
         ++l) {
      bool                   found=false;
      std::string::size_type pos;
      std::string            hash;

      if (hashFunction) {
          hash=(*hashFunction)(l->first);
        }

      if (hashFunction && !hash.empty()) {
        pos=hash.find(nameHash);
      }
      else {
        pos=l->first.find(name);
      }

      if (startWith) {
        found=pos==0;
      }
      else {
        found=pos!=std::string::npos;
      }

      if (found) {
        if (locations.size()>=limit) {
          limitReached=true;
        }
        else {
          Location location;

          location.name=l->first;

          for (std::list<Id>::const_iterator i=l->second.nodes.begin();
               i!=l->second.nodes.end();
               ++i) {
            location.references.push_back(Reference(*i,refNode));
          }

          for (std::list<Id>::const_iterator i=l->second.ways.begin();
               i!=l->second.ways.end();
               ++i) {
            location.references.push_back(Reference(*i,refWay));
          }

          FileOffset offset=l->second.offset;

          while (offset!=0) {
            std::string name;

            scanner.SetPos(offset);
            scanner.Read(name);
            scanner.ReadNumber(offset);

            if (location.path.empty()) {
              if (name!=l->first) {
                location.path.push_back(name);
              }
            }
            else {
              location.path.push_back(name);
            }
          }

          locations.push_back(location);
        }
      }
    }

    return true;
  }

  void CityStreetIndex::DumpStatistics()
  {
    size_t memory=0;

    memory+=areas.size()*sizeof(AdminRegion);
    memory+=locations.size()*sizeof(Location);

    for (std::map<std::string,Loc>::const_iterator l=this->locations.begin();
         l!=this->locations.end();
         ++l) {
      memory+=l->second.nodes.size()*sizeof(Id);
      memory+=l->second.ways.size()*sizeof(Id);
    }

    std::cout << "AdminRegion size " << areas.size() << ", locations size" << locations.size() << ", memory " << memory << std::endl;
  }
}

