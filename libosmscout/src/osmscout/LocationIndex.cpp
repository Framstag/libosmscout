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

#include <osmscout/system/Assert.h>

#include <osmscout/util/File.h>
#include <osmscout/util/Logger.h>
#include <osmscout/util/StopClock.h>
#include <iostream>
namespace osmscout {

  const char* const LocationIndex::FILENAME_LOCATION_IDX = "location.idx";

  bool LocationIndex::Load(const std::string& path, bool memoryMappedData)
  {
    this->path=path;

    FileScanner scanner;

    try {
      scanner.Open(AppendFileToDir(path,
                                   FILENAME_LOCATION_IDX),
                   FileScanner::LowMemRandom,
                   memoryMappedData);

      scanner.Read(bytesForNodeFileOffset);
      scanner.Read(bytesForAreaFileOffset);
      scanner.Read(bytesForWayFileOffset);

      uint32_t ignoreTokenCount;

      scanner.ReadNumber(ignoreTokenCount);
      regionIgnoreTokens.reserve(ignoreTokenCount);

      for (size_t i=0; i<ignoreTokenCount; i++) {
        std::string token;

        scanner.Read(token);

        regionIgnoreTokens.push_back(token);
        regionIgnoreTokenSet.insert(token);
      }

      scanner.ReadNumber(ignoreTokenCount);
      poiIgnoreTokens.reserve(ignoreTokenCount);

      for (size_t i=0; i<ignoreTokenCount; i++) {
        std::string token;

        scanner.Read(token);

        poiIgnoreTokens.push_back(token);
        poiIgnoreTokenSet.insert(token);
      }

      scanner.ReadNumber(ignoreTokenCount);
      locationIgnoreTokens.reserve(ignoreTokenCount);

      for (size_t i=0; i<ignoreTokenCount; i++) {
        std::string token;

        scanner.Read(token);

        locationIgnoreTokens.push_back(token);
        locationIgnoreTokenSet.insert(token);
      }

      scanner.ReadNumber(minRegionChars);
      scanner.ReadNumber(maxRegionChars);
      scanner.ReadNumber(minRegionWords);
      scanner.ReadNumber(maxRegionWords);
      scanner.ReadNumber(maxPOIWords);
      scanner.ReadNumber(minLocationChars);
      scanner.ReadNumber(maxLocationChars);
      scanner.ReadNumber(minLocationWords);
      scanner.ReadNumber(maxLocationWords);
      scanner.ReadNumber(maxAddressWords);

      indexOffset=scanner.GetPos();

      scanner.Close();

      return true;
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      scanner.CloseFailsafe();
      return false;
    }
  }

  bool LocationIndex::IsRegionIgnoreToken(const std::string& token) const
  {
    return regionIgnoreTokenSet.find(token)!=regionIgnoreTokenSet.end();
  }

  bool LocationIndex::IsLocationIgnoreToken(const std::string& token) const
  {
    return locationIgnoreTokenSet.find(token)!=locationIgnoreTokenSet.end();
  }

  void LocationIndex::Read(FileScanner& scanner,
                           ObjectFileRef& object) const
  {
    uint8_t    type;
    FileOffset fileOffset;

    scanner.Read(type);

    switch (type) {
    case refNone:
      break;
    case refNode:
      scanner.ReadFileOffset(fileOffset,
                             bytesForNodeFileOffset);
      object.Set(fileOffset,
                 refNode);
      break;
    case refArea:
      scanner.ReadFileOffset(fileOffset,
                             bytesForAreaFileOffset);
      object.Set(fileOffset,
                 refArea);
      break;
    case refWay:
      scanner.ReadFileOffset(fileOffset,
                             bytesForWayFileOffset);
      object.Set(fileOffset,
                 refWay);
      break;
    default:
      //std::cout << "type: " << (int) type << std::endl;
      throw IOException(scanner.GetFilename(),
                        "Cannot read ObjectFileRef",
                        "Unknown object file type");
    }
  }

  bool LocationIndex::LoadAdminRegion(FileScanner& scanner,
                                      AdminRegion& region) const
  {
    uint32_t aliasCount;
    uint32_t postalAreasCount;
    uint32_t childrenOffsetsCount;

    region.regionOffset=scanner.GetPos();

    scanner.ReadFileOffset(region.dataOffset);
    scanner.ReadFileOffset(region.parentRegionOffset);
    scanner.Read(region.name);

    Read(scanner,
         region.object);

    scanner.ReadNumber(aliasCount);

    region.aliases.clear();

    if (aliasCount>0) {
      region.aliases.resize(aliasCount);

      for (size_t i=0; i<aliasCount; i++) {
        scanner.Read(region.aliases[i].name);
        scanner.ReadFileOffset(region.aliases[i].objectOffset,
                               bytesForNodeFileOffset);
      }
    }

    scanner.ReadNumber(postalAreasCount);

    region.postalAreas.clear();

    if (postalAreasCount>0) {
      region.postalAreas.resize(postalAreasCount);

      for (size_t i=0; i<postalAreasCount; i++) {
        scanner.Read(region.postalAreas[i].name);
        scanner.ReadFileOffset(region.postalAreas[i].objectOffset);
      }
    }

    scanner.ReadNumber(childrenOffsetsCount);

    region.childrenOffsets.clear();

    if (childrenOffsetsCount>0) {
      region.childrenOffsets.resize(childrenOffsetsCount);

      for (size_t i=0; i<childrenOffsetsCount; i++) {
        scanner.ReadFileOffset(region.childrenOffsets[i]);
      }
    }

    return !scanner.HasError();
  }

  AdminRegionVisitor::Action LocationIndex::VisitRegionEntries(const AdminRegion& region,
                                                               FileScanner& scanner,
                                                               AdminRegionVisitor& visitor) const
  {
    AdminRegionVisitor::Action action=visitor.Visit(region);

    switch (action) {
    case AdminRegionVisitor::stop:
      return action;
    case AdminRegionVisitor::error:
      return action;
    case AdminRegionVisitor::skipChildren:
      return AdminRegionVisitor::visitChildren;
    case AdminRegionVisitor::visitChildren:
      // just continue...
      break;
    }

    try {
      for (auto childOffset : region.childrenOffsets) {
        AdminRegion childRegion;

        scanner.SetPos(childOffset);

        if (!LoadAdminRegion(scanner,
                             childRegion)) {
          return AdminRegionVisitor::error;
        }

        action=VisitRegionEntries(childRegion,
                                  scanner,
                                  visitor);

        if (action==AdminRegionVisitor::stop ||
            action==AdminRegionVisitor::error) {
          return action;
        }
        else if (action==AdminRegionVisitor::skipChildren) {
          return AdminRegionVisitor::visitChildren;
        }
      }
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      return AdminRegionVisitor::error;
    }

    if (scanner.HasError()) {
      return AdminRegionVisitor::error;
    }
    else {
      return AdminRegionVisitor::visitChildren;
    }
  }

  bool LocationIndex::VisitLocations(const AdminRegion& adminRegion,
                                     FileScanner& scanner,
                                     LocationVisitor& visitor,
                                     bool& stopped) const
  {
    //std::cout << "Visiting locations for " << adminRegion.name << std::endl;
    uint32_t                  locationCount;
    ObjectFileRefStreamReader objectFileRefReader(scanner);

    for (const auto& postalArea : adminRegion.postalAreas) {
      scanner.SetPos(postalArea.objectOffset);
      scanner.ReadNumber(locationCount);

      for (size_t i=0; i<locationCount; i++) {
        Location location;
        uint32_t objectCount;

        location.locationOffset=scanner.GetPos();

        scanner.Read(location.name);

        location.regionOffset=adminRegion.regionOffset;

        scanner.ReadNumber(objectCount);

        location.objects.reserve(objectCount);

        bool hasAddresses;

        scanner.Read(hasAddresses);

        if (hasAddresses) {
          scanner.ReadFileOffset(location.addressesOffset);
        }
        else {
          location.addressesOffset=0;
        }

        objectFileRefReader.Reset();

        for (size_t j=0; j<objectCount; j++) {
          ObjectFileRef ref;

          objectFileRefReader.Read(ref);

          location.objects.push_back(ref);
        }

        //std::cout << "Passing location " << location.name << " " << postalArea.name << " " << adminRegion.name << " to visitor" << std::endl;

        if (!visitor.Visit(adminRegion,
                           postalArea,
                           location)) {
          stopped=true;

          return true;
        }
      }
    }

    for (const auto offset : adminRegion.childrenOffsets) {
      AdminRegion childRegion;

      scanner.SetPos(offset);

      if (!LoadAdminRegion(scanner,
                           childRegion)) {
        return false;
      }

      //std::cout << "Visiting child region " << childRegion.name << " of " << adminRegion.name << std::endl;

      if (!VisitLocations(childRegion,
                          scanner,
                          visitor,
                          stopped)) {
        return false;
      }

      if (stopped) {
        break;
      }
    }

    return !scanner.HasError();
  }

  bool LocationIndex::VisitPostalAreaLocations(const AdminRegion& adminRegion,
                                               const PostalArea& postalArea,
                                               FileScanner& scanner,
                                               LocationVisitor& visitor,
                                               bool& stopped) const
  {
    //std::cout << "Visiting locations for " << postalArea.name << " " << adminRegion.name << std::endl;
    uint32_t                  locationCount;
    ObjectFileRefStreamReader objectFileRefReader(scanner);

    scanner.SetPos(postalArea.objectOffset);
    scanner.ReadNumber(locationCount);

    for (size_t i=0; i<locationCount; i++) {
      Location location;
      uint32_t objectCount;

      location.locationOffset=scanner.GetPos();

      scanner.Read(location.name);

      location.regionOffset=adminRegion.regionOffset;

      scanner.ReadNumber(objectCount);

      location.objects.reserve(objectCount);

      bool hasAddresses;

      scanner.Read(hasAddresses);

      if (hasAddresses) {
        scanner.ReadFileOffset(location.addressesOffset);
      }
      else {
        location.addressesOffset=0;
      }

      objectFileRefReader.Reset();

      for (size_t j=0; j<objectCount; j++) {
        ObjectFileRef ref;

        objectFileRefReader.Read(ref);

        location.objects.push_back(ref);
      }

      //std::cout << "Passing location " << location.name << " " << postalArea.name << " " << adminRegion.name << " to visitor" << std::endl;

      if (!visitor.Visit(adminRegion,
                         postalArea,
                         location)) {
        stopped=true;

        return true;
      }
    }

    return !scanner.HasError();
  }

  bool LocationIndex::VisitRegionPOIs(const AdminRegion& region,
                                      FileScanner& scanner,
                                      POIVisitor& visitor,
                                      bool recursive,
                                      bool& stopped) const
  {
    scanner.SetPos(region.dataOffset);

    uint32_t                  poiCount;
    ObjectFileRefStreamReader objectFileRefReader(scanner);

    scanner.ReadNumber(poiCount);

    for (size_t i=0; i<poiCount; i++) {
      POI poi;

      poi.regionOffset=region.regionOffset;

      scanner.Read(poi.name);
      objectFileRefReader.Read(poi.object);

      if (!visitor.Visit(region,
                         poi)) {
        stopped=true;
        break;
      }
    }

    if (stopped || !recursive) {
      return !scanner.HasError();
    }

    for (const auto offset : region.childrenOffsets) {
      AdminRegion childRegion;

      scanner.SetPos(offset);

      if (!LoadAdminRegion(scanner,
                           childRegion)) {
        return false;
      }

      if (!VisitRegionPOIs(childRegion,
                           scanner,
                           visitor,
                           recursive,
                           stopped)) {
        return false;
      }

      if (stopped) {
        break;
      }
    }

    return !scanner.HasError();
  }

  bool LocationIndex::VisitPostalArea(const AdminRegion& adminRegion,
                                      const PostalArea& postalArea,
                                      FileScanner& scanner,
                                      LocationVisitor& visitor,
                                      bool recursive,
                                      bool& stopped) const
  {
    scanner.SetPos(postalArea.objectOffset);

    if (!VisitPostalAreaLocations(adminRegion,
                                  postalArea,
                                  scanner,
                                  visitor,
                                  stopped)) {
      return false;
    }

    if (stopped || !recursive) {
      return !scanner.HasError();
    }

    for (const auto offset : adminRegion.childrenOffsets) {
      AdminRegion childRegion;

      scanner.SetPos(offset);

      if (!LoadAdminRegion(scanner,
                           childRegion)) {
        return false;
      }

      //std::cout << "Visiting child region " << childRegion.name << " of " << postalArea.name << " " << adminRegion.name << std::endl;

      for (const auto& childPostalArea : childRegion.postalAreas) {
        //std::cout << "Visiting child region " << childPostalArea.name << " " << childRegion.name << " of " << postalArea.name << " " << adminRegion.name << std::endl;
        if (!VisitPostalArea(childRegion,
                             childPostalArea,
                             scanner,
                             visitor,
                             recursive,
                             stopped)) {
          return false;
        }

        if (stopped) {
          break;
        }
      }

      if (stopped) {
        break;
      }
    }

    return !scanner.HasError();
  }

  bool LocationIndex::VisitLocation(FileScanner& scanner,
                                    const AdminRegion& region,
                                    const PostalArea& postalArea,
                                    const Location& location,
                                    AddressVisitor& visitor,
                                    bool& stopped) const
  {
    if (location.addressesOffset==0) {
      // see LocationIndex::LoadRegionDataEntry
      // if location don't have addresses, offset is set to 0
      return true;
    }

    uint32_t addressCount;

    scanner.SetPos(location.addressesOffset);

    scanner.ReadNumber(addressCount);

    ObjectFileRefStreamReader objectFileRefReader(scanner);

    for (size_t i=0; i<addressCount; i++) {
      Address address;

      address.addressOffset=scanner.GetPos();
      address.locationOffset=location.locationOffset;
      address.regionOffset=location.regionOffset;

      scanner.Read(address.name);

      objectFileRefReader.Read(address.object);

      if (!visitor.Visit(region,
                         postalArea,
                         location,
                         address)) {
        stopped=true;

        break;
      }
    }

    return !scanner.HasError();
  }

  bool LocationIndex::VisitAdminRegions(AdminRegionVisitor& visitor) const
  {
    FileScanner scanner;

    try {
      scanner.Open(AppendFileToDir(path,
                                   FILENAME_LOCATION_IDX),
                   FileScanner::LowMemRandom,
                   false);

      scanner.SetPos(indexOffset);

      uint32_t                regionCount;
      std::vector<FileOffset> rootRegionOffsets;

      scanner.ReadNumber(regionCount);
      rootRegionOffsets.resize(regionCount);

      for (size_t i=0; i<regionCount; i++) {
        scanner.ReadFileOffset(rootRegionOffsets[i]);
      }

      for (size_t i=0; i<regionCount; i++) {
        AdminRegion region;

        scanner.SetPos(rootRegionOffsets[i]);

        if (!LoadAdminRegion(scanner,
                             region)) {
          scanner.Close();
          return false;
        }

        AdminRegionVisitor::Action action;
        FileOffset                 currentPos=scanner.GetPos();


        action=VisitRegionEntries(region,
                                  scanner,
                                  visitor);

        if (action==AdminRegionVisitor::error) {
          scanner.Close();
          return false;
        }
        else if (action==AdminRegionVisitor::stop) {
          scanner.Close();
          return true;
        }

        scanner.SetPos(currentPos);
      }

      scanner.Close();

      return true;
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      scanner.CloseFailsafe();
      return false;
    }
  }

  bool LocationIndex::VisitAdminRegions(const AdminRegion& adminRegion,
                                        AdminRegionVisitor& visitor) const
  {
    FileScanner scanner;

    try {
      scanner.Open(AppendFileToDir(path,
                                   FILENAME_LOCATION_IDX),
                   FileScanner::LowMemRandom,
                   true);

      if (!VisitRegionEntries(adminRegion,
                              scanner,
                              visitor)) {
        scanner.Close();
        return false;
      }

      scanner.Close();

      return true;
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      scanner.CloseFailsafe();
      return false;
    }
  }

  bool LocationIndex::VisitPOIs(const AdminRegion& region,
                                POIVisitor& visitor,
                                bool recursive) const
  {
    FileScanner scanner;

    try {
      bool stopped=false;

      scanner.Open(AppendFileToDir(path,
                                   FILENAME_LOCATION_IDX),
                   FileScanner::LowMemRandom,
                   true);

      scanner.SetPos(region.regionOffset);

      if (!VisitRegionPOIs(region,
                           scanner,
                           visitor,
                           recursive,
                           stopped)) {
        scanner.Close();
        return false;
      }

      scanner.Close();

      return true;
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      scanner.CloseFailsafe();
      return false;
    }
  }

  bool LocationIndex::VisitLocations(const AdminRegion& adminRegion,
                                     LocationVisitor& visitor) const
  {
    FileScanner scanner;

    try {
      bool stopped=false;

      scanner.Open(AppendFileToDir(path,
                                   FILENAME_LOCATION_IDX),
                   FileScanner::LowMemRandom,
                   true);

      if (!VisitLocations(adminRegion,
                          scanner,
                          visitor,
                          stopped)) {
        scanner.Close();
        return false;
      }

      scanner.Close();

      return true;
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      scanner.CloseFailsafe();
      return false;
    }
  }

  bool LocationIndex::VisitLocations(const AdminRegion& adminRegion,
                                     const PostalArea& postalArea,
                                     LocationVisitor& visitor,
                                     bool recursive) const
  {
    FileScanner scanner;

    try {
      bool stopped=false;

      scanner.Open(AppendFileToDir(path,
                                   FILENAME_LOCATION_IDX),
                   FileScanner::LowMemRandom,
                   true);

      if (!VisitPostalArea(adminRegion,
                           postalArea,
                           scanner,
                           visitor,
                           recursive,
                           stopped)) {
        scanner.Close();
        return false;
      }

      scanner.Close();

      return true;
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      scanner.CloseFailsafe();
      return false;
    }
  }

  bool LocationIndex::VisitAddresses(const AdminRegion& region,
                                     const PostalArea& postalArea,
                                     const Location& location,
                                     AddressVisitor& visitor) const
  {
    FileScanner scanner;

    try {
      bool stopped=false;

      scanner.Open(AppendFileToDir(path,
                                   FILENAME_LOCATION_IDX),
                   FileScanner::LowMemRandom,
                   true);

      if (!VisitLocation(scanner,
                         region,
                         postalArea,
                         location,
                         visitor,
                         stopped)) {
        scanner.Close();
        return false;
      }

      scanner.Close();

      return true;
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      scanner.CloseFailsafe();
      return false;
    }
  }

  bool LocationIndex::ResolveAdminRegionHierachie(const AdminRegionRef& adminRegion,
                                                  std::map<FileOffset,AdminRegionRef >& refs) const
  {
    FileScanner scanner;

    try  {
      scanner.Open(AppendFileToDir(path,
                                   FILENAME_LOCATION_IDX),
                   FileScanner::LowMemRandom,
                   true);

      scanner.SetPos(indexOffset);

      std::list<FileOffset> offsets;

      refs[adminRegion->regionOffset]=adminRegion;

      if (adminRegion->parentRegionOffset!=0) {
        offsets.push_back(adminRegion->parentRegionOffset);
      }

      while (!offsets.empty()) {
        std::list<FileOffset> newOffsets;

        for (const auto& offset : offsets) {
          if (refs.find(offset)!=refs.end()) {
            continue;
          }

          scanner.SetPos(offset);

          AdminRegion adminRegion;

          if (!LoadAdminRegion(scanner,
                               adminRegion)) {
            return false;
          }

          refs[adminRegion.regionOffset]=std::make_shared<AdminRegion>(adminRegion);

          if (adminRegion.parentRegionOffset!=0) {
            newOffsets.push_back(adminRegion.parentRegionOffset);
          }

        }
        offsets.clear();

        std::swap(offsets,
                  newOffsets);
      }

      scanner.Close();

      return true;
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      scanner.CloseFailsafe();
      return false;
    }
  }

  void LocationIndex::DumpStatistics()
  {
    size_t memory=0;

    log.Info() << "CityStreetIndex: Memory " << memory;
  }
}

