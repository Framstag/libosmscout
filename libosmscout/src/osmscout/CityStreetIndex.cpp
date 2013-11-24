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

#include <iostream>

#include <osmscout/system/Assert.h>

#include <osmscout/util/File.h>
#include <osmscout/util/StopClock.h>

namespace osmscout {

  const char* const CityStreetIndex::FILENAME_REGION_DAT = "region.dat";

  CityStreetIndex::RegionVisitor::~RegionVisitor()
  {
    // no code
  }

  void CityStreetIndex::RegionVisitor::Initialize()
  {
    // no code
  }

  CityStreetIndex::RegionMatchVisitor::RegionMatchVisitor(const std::string& pattern,
                                                          size_t limit)
  : pattern(pattern),
    limit(limit),
    limitReached(false)
  {
    // no code
  }

  void CityStreetIndex::RegionMatchVisitor::Initialize()
  {
    limitReached=false;
    matches.clear();
    candidates.clear();
  }

  void CityStreetIndex::RegionMatchVisitor::Match(const std::string& name,
                                                  bool& match,
                                                  bool& candidate) const
  {
    std::string::size_type matchPosition;

    matchPosition=name.find(pattern);

    match=matchPosition==0 && name.length()==pattern.length();
    candidate=matchPosition!=std::string::npos;
  }

  bool CityStreetIndex::RegionMatchVisitor::Visit(const RegionEntry& region)
  {
    bool                   match;
    bool                   candidate;

    Match(region.name,
          match,
          candidate);

    if (match || candidate) {
      AdminRegion adminRegion;

      adminRegion.indexOffset=region.indexOffset;
      adminRegion.dataOffset=region.dataOffset;
      adminRegion.parentIndexOffset=region.parentIndexOffset;
      adminRegion.name=region.name;
      adminRegion.reference=region.reference;

      if (match) {
        matches.push_back(adminRegion);
      }
      else {
        candidates.push_back(adminRegion);
      }
    }

    for (size_t i=0; i<region.aliases.size(); i++) {
      Match(region.aliases[i].name,
            match,
            candidate);

      if (match || candidate) {
        AdminRegion adminRegion;

        adminRegion.indexOffset=region.indexOffset;
        adminRegion.dataOffset=region.dataOffset;
        adminRegion.parentIndexOffset=region.parentIndexOffset;
        adminRegion.name=region.aliases[i].name;
        adminRegion.reference=region.reference;

        if (match) {
          matches.push_back(adminRegion);
        }
        else {
          candidates.push_back(adminRegion);
        }
      }
    }

    limitReached=matches.size()+candidates.size()>=limit;

    return !limitReached;
  }

  CityStreetIndex::LocationVisitor::LocationVisitor(FileScanner& scanner)
  : startWith(false),
    limit(50),
    limitReached(false),
    scanner(scanner)
  {
    // no code
  }

  bool CityStreetIndex::LocationVisitor::Visit(const std::string& locationName,
                                               const Loc &loc)
  {
    std::string::size_type pos;
    bool                   found;

    pos=locationName.find(name);

    if (startWith) {
      found=pos==0;
    }
    else {
      found=pos!=std::string::npos;
    }

    if (!found) {
      return true;
    }

    Location location;

    location.regionOffset=loc.offset;
    location.name=locationName;

    for (std::vector<ObjectFileRef>::const_iterator object=loc.objects.begin();
         object!=loc.objects.end();
         ++object) {
      location.references.push_back(*object);
    }

    locations.push_back(location);

    limitReached=locations.size()>=limit;

    return !limitReached;
  }

  CityStreetIndex::CityStreetIndex()
  {
    // no code
  }

  CityStreetIndex::~CityStreetIndex()
  {
    // no code
  }

  bool CityStreetIndex::Load(const std::string& path)
  {
    this->path=path;

    return true;
  }

  bool CityStreetIndex::LoadRegionEntry(FileScanner& scanner,
                                        RegionEntry& region) const
  {
    uint8_t          regionReferenceType;
    FileOffset       regionReferenceOffset;
    uint32_t         aliasCount;

    if (!scanner.GetPos(region.indexOffset)) {
      return false;
    }

    if (!scanner.ReadFileOffset(region.dataOffset)) {
      return false;
    }

    if (!scanner.ReadFileOffset(region.parentIndexOffset)) {
      return false;
    }

    if (!scanner.Read(region.name)) {
      return false;
    }

    if (!scanner.Read(regionReferenceType)) {
      return false;
    }

    if (!scanner.ReadFileOffset(regionReferenceOffset)) {
      return false;
    }

    region.reference.Set(regionReferenceOffset,
                         (RefType)regionReferenceType);

    if (!scanner.ReadNumber(aliasCount)) {
      return false;
    }

    region.aliases.clear();
    region.aliases.resize(aliasCount);

    for (size_t i=0; i<aliasCount; i++) {
      if (!scanner.Read(region.aliases[i].name)) {
        return false;
      }

      if (!scanner.ReadFileOffset(region.aliases[i].offset)) {
        return false;
      }
    }

    return !scanner.HasError();
  }

  bool CityStreetIndex::VisitRegionEntries(FileScanner& scanner,
                                           RegionVisitor& visitor) const
  {
    RegionEntry region;
    uint32_t         childCount;

    if (!LoadRegionEntry(scanner,
                              region)) {
      return false;
    }

    if (!visitor.Visit(region)) {
      return true;
    }

    if (!scanner.ReadNumber(childCount)) {
      return false;
    }

    for (size_t i=0; i<childCount; i++) {
      if (!VisitRegionEntries(scanner,
                                   visitor)) {
        return false;
      }
    }

    return !scanner.HasError();
  }

  bool CityStreetIndex::LoadRegionDataEntry(FileScanner& scanner,
                                            const RegionEntry& region,
                                            LocationVisitor& visitor) const
  {
    uint32_t locationCount;

    if (!scanner.ReadNumber(locationCount)) {
      return false;
    }

    for (size_t i=0; i<locationCount; i++) {
      std::string name;
      uint32_t    objectCount;
      Loc         loc;

      if (!scanner.Read(name)) {
        return false;
      }

      loc.offset=region.indexOffset;

      if (!scanner.ReadNumber(objectCount)) {
        return false;
      }

      loc.objects.reserve(objectCount);

      FileOffset lastOffset=0;

      for (size_t j=0; j<objectCount; j++) {
        uint8_t    type;
        FileOffset offset;

        if (!scanner.Read(type)) {
          return false;
        }

        if (!scanner.ReadNumber(offset)) {
          return false;
        }

        offset+=lastOffset;

        loc.objects.push_back(ObjectFileRef(offset,(RefType)type));

        lastOffset=offset;
      }

      if (!visitor.Visit(name,loc)) {
        return true;
      }
    }

    return !scanner.HasError();
  }

  bool CityStreetIndex::VisitRegionDataEntries(FileScanner& scanner,
                                               LocationVisitor& visitor) const
  {
    RegionEntry region;
    FileOffset       childrenOffset;
    uint32_t         childCount;

    if (!LoadRegionEntry(scanner,
                              region)) {
      return false;
    }

    if (!scanner.GetPos(childrenOffset)) {
      return false;
    }

    if (!scanner.SetPos(region.dataOffset)) {
      return false;
    }

    if (!LoadRegionDataEntry(scanner,
                             region,
                             visitor)) {
      return false;
    }

    if (!scanner.SetPos(childrenOffset)) {
      return false;
    }

    if (!scanner.ReadNumber(childCount)) {
      return false;
    }

    for (size_t i=0; i<childCount; i++) {
      if (!VisitRegionDataEntries(scanner,
                                  visitor)) {
        return false;
      }
    }

    return !scanner.HasError();
  }

  bool CityStreetIndex::GetMatchingAdminRegions(const std::string& name,
                                                std::list<AdminRegion>& regions,
                                                size_t limit,
                                                bool& limitReached,
                                                bool startWith) const
  {
    RegionMatchVisitor visitor(name,
                               limit);
    FileScanner        scanner;

    visitor.Initialize();
    regions.clear();

    if (!scanner.Open(AppendFileToDir(path,
                                      FILENAME_REGION_DAT),
                      FileScanner::LowMemRandom,
                      false)) {
      std::cerr << "Cannot open file '" << scanner.GetFilename() << "'!" << std::endl;
      return false;
    }

    uint32_t regionCount;

    if (!scanner.ReadNumber(regionCount)) {
      return false;
    }

    for (size_t i=0; i<regionCount; i++) {
      if (!VisitRegionEntries(scanner,
                                   visitor)) {
        return false;
      }
    }

    for (std::list<AdminRegion>::const_iterator region=visitor.matches.begin();
        region!=visitor.matches.end();
        ++region) {
      regions.push_back(*region);
    }

    for (std::list<AdminRegion>::const_iterator region=visitor.candidates.begin();
        region!=visitor.candidates.end();
        ++region) {
      regions.push_back(*region);
    }

    if (regions.empty()) {
      return scanner.Close();
    }

    // If there are results, build up a path for each hit by following
    // the parent relation up to the top of the tree.

    for (std::list<AdminRegion>::iterator area=regions.begin();
         area!=regions.end();
         ++area) {
      FileOffset       offset=area->indexOffset;
      RegionEntry region;

      // Resolve all parent to build up the complete region path
      while (offset!=0) {
        RegionEntry region;

        if (!scanner.SetPos(offset)) {
          return false;
        }

        if (!LoadRegionEntry(scanner,
                                  region)) {
          return false;
        }

        if (area->path.empty()) {
          // if the found name is an alias of the region,
          // we add the real name of the region also,
          // else we skip it
          if (region.name!=area->name) {
            area->path.push_back(region.name);
          }
        }
        else {
          area->path.push_back(region.name);
        }

        offset=region.parentIndexOffset;
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

    if (!scanner.Open(AppendFileToDir(path,
                                      FILENAME_REGION_DAT),
                      FileScanner::LowMemRandom,
                      true)) {
      std::cerr << "Cannot open file '" << scanner.GetFilename() << "'!" << std::endl;
      return false;
    }

    LocationVisitor locVisitor(scanner);

    locVisitor.name=name;
    locVisitor.startWith=startWith;
    locVisitor.limit=limit;
    locVisitor.limitReached=false;

    if (!scanner.SetPos(region.indexOffset)) {
      return false;
    }

    if (!VisitRegionDataEntries(scanner,
                                locVisitor)) {
      return false;
    }

    locations=locVisitor.locations;
    limitReached=locVisitor.limitReached;

    // If there are results, build up a path for each hit by following
    // the parent relation up to the top of the tree.

    for (std::list<Location>::iterator location=locations.begin();
         location!=locations.end();
         ++location) {
      FileOffset       offset=location->regionOffset;
      RegionEntry region;

      // Resolve all parent to build up the complete region path
      while (offset!=0) {
        RegionEntry region;

        if (!scanner.SetPos(offset)) {
          return false;
        }

        if (!LoadRegionEntry(scanner,
                                  region)) {
          return false;
        }

        if (location->path.empty()) {
          // if the found name is an alias of the region,
          // we add the real name of the region also,
          // else we skip it
          if (region.name!=location->name) {
            location->path.push_back(region.name);
          }
        }
        else {
          location->path.push_back(region.name);
        }

        offset=region.parentIndexOffset;
      }
    }


    return true;
  }

  void CityStreetIndex::DumpStatistics()
  {
    size_t memory=0;

    std::cout << "CityStreetIndex: Memory " << memory << std::endl;
  }
}

