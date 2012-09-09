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

#include <osmscout/util/File.h>
#include <osmscout/util/StopClock.h>

namespace osmscout {

  CityStreetIndex::LocationVisitor::LocationVisitor(FileScanner& scanner)
  : scanner(scanner)
  {
    // no code
  }

  bool CityStreetIndex::LocationVisitor::Visit(const std::string& locationName,
                                               const Loc &loc)
  {
    std::string::size_type pos;
    bool                   found;

    if (hashFunction!=NULL) {
      std::string hash=(*hashFunction)(locationName);

      if (!hash.empty()) {
        pos=hash.find(nameHash);
      }
      else {
        pos=locationName.find(name);
      }
    }
    else {
      pos=locationName.find(name);
    }

    if (startWith) {
      found=pos==0;
    }
    else {
      found=pos!=std::string::npos;
    }

    if (!found) {
      return true;
    }

    if (locations.size()>=limit) {
      limitReached=true;

      return false;
    }

    Location location;

    location.name=locationName;

    for (std::list<Id>::const_iterator i=loc.nodes.begin();
         i!=loc.nodes.end();
         ++i) {
      location.references.push_back(ObjectRef(*i,refNode));
    }

    for (std::list<Id>::const_iterator i=loc.ways.begin();
         i!=loc.ways.end();
         ++i) {
      location.references.push_back(ObjectRef(*i,refWay));
    }

    // Build up path for each hit by following
    // the parent relation up to the top of the tree.

    FileOffset currentOffset;
    FileOffset regionOffset=loc.offset;

    if (!scanner.GetPos(currentOffset)) {
      return false;
    }

    while (!scanner.HasError() &&
           regionOffset!=0) {
      std::string name;

      scanner.SetPos(regionOffset);
      scanner.Read(name);
      scanner.ReadNumber(regionOffset);

      if (location.path.empty()) {
        // We dot not want something like "'Dortmund' in 'Dortmund'"!
        if (name!=locationName) {
          location.path.push_back(name);
        }
      }
      else {
        location.path.push_back(name);
      }
    }

    locations.push_back(location);

    return scanner.SetPos(currentOffset);
  }

  CityStreetIndex::CityStreetIndex()
   : hashFunction(NULL)
  {
    // no code
  }

  CityStreetIndex::~CityStreetIndex()
  {
    // no code
  }

  bool CityStreetIndex::LoadRegion(FileScanner& scanner,
                                   LocationVisitor& visitor) const
  {
    std::string               name;
    FileOffset                offset;
    FileOffset                parentOffset;
    uint32_t                  childrenCount;
    uint32_t                  nodeCount;
    uint32_t                  wayCount;
    std::map<std::string,Loc> locations;

    if (!scanner.GetPos(offset) ||
      !scanner.Read(name) ||
      !scanner.ReadNumber(parentOffset)) {
      return false;
    }

    if (!scanner.ReadNumber(childrenCount)) {
      return false;
    }

    for (size_t i=0; i<childrenCount; i++) {
      if (!LoadRegion(scanner,visitor)) {
        return false;
      }
    }

    if (!scanner.ReadNumber(nodeCount)) {
      return false;
    }

    for (size_t i=0; i<nodeCount; i++) {
      std::string name;
      uint32_t    idCount;
      Id          lastId=0;

      if (!scanner.Read(name) ||
          !scanner.ReadNumber(idCount)) {
        return false;
      }

      locations[name].offset=offset;

      for (size_t j=0; j<idCount; j++) {
        Id id;

        if (!scanner.ReadNumber(id)) {
          return false;
        }

        locations[name].nodes.push_back(id+lastId);

        lastId=id;
      }
    }

    if (!scanner.ReadNumber(wayCount)) {
      return false;
    }

    for (size_t i=0; i<wayCount; i++) {
      std::string name;
      uint32_t    idCount;
      Id          lastId=0;

      if (!scanner.Read(name) ||
          !scanner.ReadNumber(idCount)) {
        return false;
      }

      locations[name].offset=offset;

      for (size_t j=0; j<idCount; j++) {
        Id id;

        if (!scanner.ReadNumber(id)) {
          return false;
        }

        locations[name].ways.push_back(id+lastId);

        lastId=id;
      }
    }

    for (std::map<std::string,Loc>::const_iterator l=locations.begin();
         l!=locations.end();
         ++l) {
      if (!visitor.Visit(l->first,l->second)) {
        return true;
      }
    }

    return !scanner.HasError();
  }

  bool CityStreetIndex::LoadRegion(FileScanner& scanner,
                                   FileOffset offset,
                                   LocationVisitor& visitor) const
  {
    scanner.SetPos(offset);

    return LoadRegion(scanner,visitor);
  }

  bool CityStreetIndex::Load(const std::string& path,
                             std::string (*hashFunction) (std::string))
  {
    this->path=path;
    this->hashFunction=hashFunction;

    return true;
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
    if (hashFunction!=NULL) {
      nameHash=(*hashFunction)(name);
    }

    FileScanner   scanner;
    std::string   indexFile=AppendFileToDir(path,"nameregion.idx");

    if (!scanner.Open(indexFile,true)) {
      std::cerr << "Cannot open file '" << indexFile << "'!" << std::endl;
      return false;
    }

    uint32_t areaRefs;

    if (!scanner.ReadNumber(areaRefs)) {
      return false;
    }

    for (size_t i=0; i<areaRefs; i++) {
      std::string regionName;
      uint32_t    entries;

      if (!scanner.Read(regionName)) {
        return false;
      }

      if (!scanner.ReadNumber(entries)) {
        return false;
      }

      for (size_t j=0; j<entries; j++) {
        Region   region;
        uint32_t type;

        region.name=regionName;

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


        bool                   found=false;
        std::string::size_type loc;

        // Calculate match

        if (hashFunction!=NULL) {
          std::string hash=(*hashFunction)(region.name);

          if (!hash.empty()) {
            loc=hash.find(nameHash);
          }
          else {
            loc=region.name.find(name);
          }
        }
        else {
          loc=region.name.find(name);
        }

        if (startWith) {
          found=loc==0;
        }
        else {
          found=loc!=std::string::npos;
        }

        // If match, Add to result

        if (found) {
          if (regions.size()>=limit) {
            limitReached=true;
          }
          else {
            AdminRegion adminRegion;

            adminRegion.reference=region.reference;
            adminRegion.offset=region.offset;
            adminRegion.name=region.name;

            regions.push_back(adminRegion);
          }
        }
      }
    }

    if (!scanner.Close()) {
      return false;
    }

    if (regions.empty()) {
      return true;
    }

    std::string regionFile=AppendFileToDir(path,"region.dat");

    if (!scanner.Open(regionFile,true)) {
      std::cerr << "Cannot open file '" << regionFile << "'!" << std::endl;
      return false;
    }

    // If there are results, build up path for each hit by following
    // the parent relation up to the top of the tree.

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
    FileScanner scanner;
    std::string file=AppendFileToDir(path,"region.dat");

    if (!scanner.Open(file,true)) {
      std::cerr << "Cannot open file '" << file << "'!" << std::endl;
      return false;
    }

    LocationVisitor locVisitor(scanner);

    locVisitor.name=name;
    locVisitor.startWith=startWith;
    locVisitor.limit=limit;
    locVisitor.limitReached=false;

    locVisitor.hashFunction=hashFunction;

    if (hashFunction!=NULL) {
      locVisitor.nameHash=(*hashFunction)(name);
    }


    if (!LoadRegion(locVisitor.scanner,
                    region.offset,
                    locVisitor)) {
      return false;
    }

    locations=locVisitor.locations;
    limitReached=locVisitor.limitReached;

    return true;
  }

  void CityStreetIndex::DumpStatistics()
  {
    size_t memory=0;

    std::cout << "CityStreetIndex: Memory " << memory << std::endl;
  }
}

