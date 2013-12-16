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

#include <osmscout/LocationIndex.h>

#include <iostream>

#include <osmscout/system/Assert.h>

#include <osmscout/util/File.h>
#include <osmscout/util/StopClock.h>

namespace osmscout {

  const char* const LocationIndex::FILENAME_LOCATION_IDX = "location.idx";

  LocationIndex::LocationIndex()
  {
    // no code
  }

  LocationIndex::~LocationIndex()
  {
    // no code
  }

  bool LocationIndex::Load(const std::string& path)
  {
    this->path=path;

    return true;
  }

  bool LocationIndex::LoadAdminRegion(FileScanner& scanner,
                                      AdminRegion& region) const
  {
    uint8_t          regionReferenceType;
    FileOffset       regionReferenceOffset;
    uint32_t         aliasCount;

    if (!scanner.GetPos(region.regionOffset)) {
      return false;
    }

    if (!scanner.ReadFileOffset(region.dataOffset)) {
      return false;
    }

    if (!scanner.ReadFileOffset(region.parentRegionOffset)) {
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

    region.object.Set(regionReferenceOffset,
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

      if (!scanner.ReadFileOffset(region.aliases[i].objectOffset)) {
        return false;
      }
    }

    return !scanner.HasError();
  }

  bool LocationIndex::VisitRegionEntries(FileScanner& scanner,
                                         AdminRegionVisitor& visitor) const
  {
    AdminRegion region;
    uint32_t         childCount;

    if (!LoadAdminRegion(scanner,
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

  bool LocationIndex::LoadRegionDataEntry(FileScanner& scanner,
                                          const AdminRegion& adminRegion,
                                          LocationVisitor& visitor) const
  {
    uint32_t poiCount;
    uint32_t locationCount;

    if (!scanner.ReadNumber(poiCount)) {
      return false;
    }

    for (size_t i=0; i<poiCount; i++) {
      POI        poi;
      uint8_t    type;
      FileOffset offset;

      poi.regionOffset=adminRegion.regionOffset;

      if (!scanner.Read(poi.name)) {
        return false;
      }

      if (!scanner.Read(type)) {
        return false;
      }

      if (!scanner.ReadNumber(offset)) {
        return false;
      }

      poi.object.Set(offset,(RefType)type);

      if (!visitor.Visit(adminRegion,
                         poi)) {
        return true;
      }
    }

    if (!scanner.ReadNumber(locationCount)) {
      return false;
    }

    for (size_t i=0; i<locationCount; i++) {
      Location location;
      uint32_t  objectCount;

      if (!scanner.GetPos(location.locationOffset)) {
        return false;
      }

      if (!scanner.Read(location.name)) {
        return false;
      }

      location.regionOffset=adminRegion.regionOffset;

      if (!scanner.ReadNumber(objectCount)) {
        return false;
      }

      location.objects.reserve(objectCount);

      bool hasAddresses;

      if (!scanner.Read(hasAddresses)) {
        return false;
      }

      if (hasAddresses) {
        if (!scanner.ReadFileOffset(location.addressesOffset)) {
          return false;
        }
      }
      else {
        location.addressesOffset=0;
      }

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

        location.objects.push_back(ObjectFileRef(offset,(RefType)type));

        lastOffset=offset;
      }

      if (!visitor.Visit(adminRegion,
                         location)) {
        return true;
      }
    }

    return !scanner.HasError();
  }

  bool LocationIndex::VisitRegionLocationEntries(FileScanner& scanner,
                                                 LocationVisitor& visitor) const
  {
    AdminRegion region;
    FileOffset       childrenOffset;
    uint32_t         childCount;

    if (!LoadAdminRegion(scanner,
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
      if (!VisitRegionLocationEntries(scanner,
                                      visitor)) {
        return false;
      }
    }

    return !scanner.HasError();
  }

  bool LocationIndex::VisitLocationAddressEntries(FileScanner& scanner,
                                                  const Location& location,
                                                  AddressVisitor& visitor) const
  {
    uint32_t addressCount;

    if (!scanner.SetPos(location.addressesOffset)) {
      return false;
    }

    if (!scanner.ReadNumber(addressCount)) {
      return false;
    }

    FileOffset lastOffset=0;

    for (size_t i=0; i<addressCount; i++) {
      Address     address;
      uint8_t     type;
      FileOffset  offset;

      if (!scanner.GetPos(address.addressOffset)) {
        return false;
      }

      address.locationOffset=location.locationOffset;
      address.regionOffset=location.regionOffset;

      if (!scanner.Read(address.name)) {
        return false;
      }

      if (!scanner.Read(type)) {
        return false;
      }


      if (!scanner.ReadNumber(offset)) {
        return false;
      }

      offset+=lastOffset;

      address.object.Set(offset,(RefType)type);

      lastOffset=offset;

      visitor.Visit(location,
                    address);
    }

    return !scanner.HasError();
  }

  bool LocationIndex::VisitAdminRegions(AdminRegionVisitor& visitor) const
  {
    FileScanner scanner;

    if (!scanner.Open(AppendFileToDir(path,
                                      FILENAME_LOCATION_IDX),
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

    return !scanner.HasError() && scanner.Close();
  }

  bool LocationIndex::VisitAdminRegionLocations(const AdminRegion& region,
                                                LocationVisitor& visitor) const
  {
    FileScanner scanner;

    if (!scanner.Open(AppendFileToDir(path,
                                      FILENAME_LOCATION_IDX),
                      FileScanner::LowMemRandom,
                      true)) {
      std::cerr << "Cannot open file '" << scanner.GetFilename() << "'!" << std::endl;
      return false;
    }

    if (!scanner.SetPos(region.regionOffset)) {
      return false;
    }

    if (!VisitRegionLocationEntries(scanner,
                                    visitor)) {
      return false;
    }

    return !scanner.HasError() && scanner.Close();
  }

  bool LocationIndex::VisitLocationAddresses(const Location& location,
                                             AddressVisitor& visitor) const
  {
    FileScanner scanner;

    if (!scanner.Open(AppendFileToDir(path,
                                      FILENAME_LOCATION_IDX),
                      FileScanner::LowMemRandom,
                      true)) {
      std::cerr << "Cannot open file '" << scanner.GetFilename() << "'!" << std::endl;
      return false;
    }

    if (!VisitLocationAddressEntries(scanner,
                                     location,
                                     visitor)) {
      return false;
    }

    return !scanner.HasError() && scanner.Close();
  }

  bool LocationIndex::ResolveAdminRegionHierachie(const AdminRegionRef& adminRegion,
                                                  std::map<FileOffset,AdminRegionRef >& refs) const
  {
    FileScanner scanner;

    if (!scanner.Open(AppendFileToDir(path,
                                      FILENAME_LOCATION_IDX),
                      FileScanner::LowMemRandom,
                      true)) {
      std::cerr << "Cannot open file '" << scanner.GetFilename() << "'!" << std::endl;
      return false;
    }

    std::list<FileOffset> offsets;

    refs[adminRegion->regionOffset]=adminRegion;

    if (adminRegion->parentRegionOffset!=0) {
      offsets.push_back(adminRegion->parentRegionOffset);
    }

    while (!offsets.empty()) {
      std::list<FileOffset> newOffsets;

      for (std::list<FileOffset>::const_iterator offset=offsets.begin();
          offset!=offsets.end();
          ++offset) {
        if (refs.find(*offset)!=refs.end()) {
          continue;
        }

        if (!scanner.SetPos(*offset)) {
          return false;
        }

        AdminRegion adminRegion;

        if (!LoadAdminRegion(scanner,
                             adminRegion)) {
          return false;
        }

        refs[adminRegion.regionOffset]=new AdminRegion(adminRegion);

        if (adminRegion.parentRegionOffset!=0) {
          newOffsets.push_back(adminRegion.parentRegionOffset);
        }

      }
      offsets.clear();

      std::swap(offsets,
                newOffsets);
    }

    return !scanner.HasError() && scanner.Close();
  }

  void LocationIndex::DumpStatistics()
  {
    size_t memory=0;

    std::cout << "CityStreetIndex: Memory " << memory << std::endl;
  }
}

