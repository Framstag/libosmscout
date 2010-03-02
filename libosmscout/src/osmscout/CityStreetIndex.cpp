/*
  TravelJinni - Openstreetmap offline viewer
  Copyright (C) 2009  Tim Teulings

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
    FileOffset  parentOffset;
    size_t      childrenCount;
    size_t      nodeCount;
    size_t      wayCount;
    size_t      areaCount;

    scanner.Read(name);
    scanner.ReadNumber(parentOffset);

    scanner.ReadNumber(childrenCount);

    for (size_t i=0; i<childrenCount; i++) {
      LoadRegion(scanner);
    }

    scanner.ReadNumber(nodeCount);
    for (size_t i=0; i<nodeCount; i++) {
      std::string name;
      size_t      idCount;
      Id          lastId=0;

      scanner.Read(name);
      scanner.ReadNumber(idCount);

      for (size_t j=0; j<idCount; j++) {
        Id id;

        scanner.ReadNumber(id);

        locations[name].nodes.push_back(id+lastId);
        lastId=id;
      }
    }

    scanner.ReadNumber(wayCount);
    for (size_t i=0; i<wayCount; i++) {
      std::string name;
      size_t      idCount;
      Id          lastId=0;

      scanner.Read(name);
      scanner.ReadNumber(idCount);

      for (size_t j=0; j<idCount; j++) {
        Id id;

        scanner.ReadNumber(id);

        locations[name].ways.push_back(id+lastId);
        lastId=id;
      }
    }

    scanner.ReadNumber(areaCount);
    for (size_t i=0; i<areaCount; i++) {
      std::string name;
      size_t      idCount;
      Id          lastId=0;

      scanner.Read(name);
      scanner.ReadNumber(idCount);

      for (size_t j=0; j<idCount; j++) {
        Id id;

        scanner.ReadNumber(id);

        locations[name].areas.push_back(id+lastId);
        lastId=id;
      }
    }

    return true;
  }

  bool CityStreetIndex::LoadRegion(FileOffset offset) const
  {
    FileScanner   scanner;
    std::string   file=path+"/"+"region.dat";
    std::list<FileOffset> offsets;

    locations.clear();

    if (!scanner.Open(file)) {
      std::cerr << "Cannot open file '" << file << "'!" << std::endl;
      return false;
    }

    scanner.SetPos(offset);

    LoadRegion(scanner);

    regionLoaded=true;

    return !scanner.HasError() && scanner.Close();
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

    size_t areaRefs;

    if (!scanner.ReadNumber(areaRefs)) {
      return false;
    }

    std::cout << areaRefs << " areas..." << std::endl;

    for (size_t i=0; i<areaRefs; i++) {
      std::string name;
      std::size_t entries;

      if (!scanner.Read(name)) {
        return false;
      }

      if (!scanner.ReadNumber(entries)) {
        return false;
      }

      for (size_t j=0; j<entries; j++) {
        AdminRegion   l;
        unsigned long type;

        l.name=name;

        if (!scanner.ReadNumber(type)) {
          return false;
        }

        l.reference.type=(RefType)type;

        if (!scanner.ReadNumber(l.reference.id)) {
          return false;
        }

        if (!scanner.ReadNumber(l.offset)) {
          return false;
        }

        // if the user has supplied a hash function then use it to generate a hash value
        if (hashFunction) {
          l.hash=(*hashFunction)(l.name);
        }

        areas.push_back(l);
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

    for (std::list<AdminRegion>::const_iterator area=this->areas.begin();
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
          std::cout << "Limit reached!!!" << std::endl;
          limitReached=true;
        }
        else {
          regions.push_back(*area);
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
            area->path=name;
          }
        }
        else {
          area->path.append("/");
          area->path.append(name);
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

    if (!regionLoaded || this->region!=region.offset) {
      if (!LoadRegion(region.offset) || !regionLoaded) {
        return false;
      }
    }

    // if the user supplied a special hash function call it and use the result
    if (hashFunction) {
      nameHash=(*hashFunction)(name);
    }

    for (std::map<std::string,Loc>::const_iterator l=this->locations.begin();
         l!=this->locations.end();
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
          std::cout << "Limit reached!!!" << std::endl;
          limitReached=true;
          return true;
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

          for (std::list<Id>::const_iterator i=l->second.areas.begin();
               i!=l->second.areas.end();
               ++i) {
            location.references.push_back(Reference(*i,refArea));
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
      memory+=l->second.areas.size()*sizeof(Id);
    }

    std::cout << "AdminRegion size " << areas.size() << ", locations size" << locations.size() << ", memory " << memory << std::endl;
  }
}

