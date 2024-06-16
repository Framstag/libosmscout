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

#include <osmscout/db/LocationIndex.h>

#include <osmscout/io/File.h>

#include <osmscout/log/Logger.h>

namespace osmscout {

  const char* const LocationIndex::FILENAME_LOCATION_IDX = "location.idx";

  bool LocationIndex::Load(const std::string& path, bool memoryMappedData)
  {
    fileScannerPool.path=path;
    fileScannerPool.memoryMappedData=memoryMappedData;

    try {
      FileScannerPtr scanner=fileScannerPool.Borrow();
      if (!scanner){
        return false;
      }

      bytesForNodeFileOffset=scanner->ReadUInt8();
      bytesForAreaFileOffset=scanner->ReadUInt8();
      bytesForWayFileOffset=scanner->ReadUInt8();

      uint32_t ignoreTokenCount=scanner->ReadUInt32Number();

      regionIgnoreTokens.reserve(ignoreTokenCount);
      for (uint32_t i=0; i<ignoreTokenCount; i++) {
        std::string token=scanner->ReadString();

        regionIgnoreTokens.push_back(token);
        regionIgnoreTokenSet.insert(token);
      }

      ignoreTokenCount=scanner->ReadUInt32Number();

      poiIgnoreTokens.reserve(ignoreTokenCount);
      for (uint32_t i=0; i<ignoreTokenCount; i++) {
        std::string token=scanner->ReadString();

        poiIgnoreTokens.push_back(token);
        poiIgnoreTokenSet.insert(token);
      }

      ignoreTokenCount=scanner->ReadUInt32Number();

      locationIgnoreTokens.reserve(ignoreTokenCount);
      for (size_t i=0; i<ignoreTokenCount; i++) {
        std::string token=scanner->ReadString();

        locationIgnoreTokens.push_back(token);
        locationIgnoreTokenSet.insert(token);
      }

      minRegionChars=scanner->ReadUInt32Number();
      maxRegionChars=scanner->ReadUInt32Number();
      minRegionWords=scanner->ReadUInt32Number();
      maxRegionWords=scanner->ReadUInt32Number();
      maxPOIWords=scanner->ReadUInt32Number();
      minLocationChars=scanner->ReadUInt32Number();
      maxLocationChars=scanner->ReadUInt32Number();
      minLocationWords=scanner->ReadUInt32Number();
      maxLocationWords=scanner->ReadUInt32Number();
      maxAddressWords=scanner->ReadUInt32Number();

      indexOffset=scanner->GetPos();

    }
    catch (const IOException& e) {
      log.Error() << e.GetDescription();
      fileScannerPool.Clear();
      return false;
    }
    fileScannerPool.Clear();
    return true;
  }

  bool LocationIndex::IsRegionIgnoreToken(const std::string& token) const
  {
    return regionIgnoreTokenSet.contains(token);
  }

  bool LocationIndex::IsLocationIgnoreToken(const std::string& token) const
  {
    return locationIgnoreTokenSet.contains(token);
  }

  LocationIndex::FileScannerPtr LocationIndex::FileScannerPool::Borrow()
  {
    auto scanner=ObjectPool<FileScanner>::Borrow();
    if (scanner){
      scanner->SetPos(0);
    }
    return scanner;
  }

  FileScanner *LocationIndex::FileScannerPool::MakeNew() noexcept
  {
    FileScanner *scanner=new FileScanner();
    try {
      scanner->Open(AppendFileToDir(path,
                                    FILENAME_LOCATION_IDX),
                    FileScanner::LowMemRandom,
                    memoryMappedData);
      return scanner;
    } catch (const IOException& e) {
      log.Error() << e.GetDescription();
      scanner->CloseFailsafe();
      delete scanner;
      return nullptr;
    }
  }

  void LocationIndex::FileScannerPool::Destroy(FileScanner* o) noexcept
  {
    try{
      o->Close();
    } catch (const IOException& e) {
      log.Error() << e.GetDescription();
      o->CloseFailsafe();
    }
    delete o;
  }

  bool LocationIndex::FileScannerPool::IsValid(FileScanner* o) noexcept
  {
    return o->IsOpen() && !o->HasError();
  }

  void LocationIndex::Read(FileScanner& scanner,
                           ObjectFileRef& object) const
  {
    FileOffset fileOffset;

    uint8_t    type=scanner.ReadUInt8();

    switch (type) {
    case refNone:
      break;
    case refNode:
      fileOffset=scanner.ReadFileOffset(bytesForNodeFileOffset);
      object.Set(fileOffset,
                 refNode);
      break;
    case refArea:
      fileOffset=scanner.ReadFileOffset(bytesForAreaFileOffset);
      object.Set(fileOffset,
                 refArea);
      break;
    case refWay:
      fileOffset=scanner.ReadFileOffset(bytesForWayFileOffset);
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

    region.dataOffset=scanner.ReadFileOffset();
    region.parentRegionOffset=scanner.ReadFileOffset();
    region.name=scanner.ReadString();
    region.altName=scanner.ReadString();

    Read(scanner,
         region.object);

    aliasCount=scanner.ReadUInt32Number();

    region.aliases.clear();

    if (aliasCount>0) {
      region.aliases.resize(aliasCount);

      for (uint32_t i=0; i<aliasCount; i++) {
        region.aliases[i].name=scanner.ReadString();
        region.aliases[i].altName=scanner.ReadString();
        region.aliases[i].objectOffset=scanner.ReadFileOffset(bytesForNodeFileOffset);
      }
    }

    postalAreasCount=scanner.ReadUInt32Number();

    region.postalAreas.clear();

    if (postalAreasCount>0) {
      region.postalAreas.resize(postalAreasCount);

      for (uint32_t i=0; i<postalAreasCount; i++) {
        region.postalAreas[i].name=scanner.ReadString();
        region.postalAreas[i].objectOffset=scanner.ReadFileOffset();
      }
    }

    childrenOffsetsCount=scanner.ReadUInt32Number();

    region.childrenOffsets.clear();

    if (childrenOffsetsCount>0) {
      region.childrenOffsets.resize(childrenOffsetsCount);

      for (uint32_t i=0; i<childrenOffsetsCount; i++) {
        region.childrenOffsets[i]=scanner.ReadFileOffset();
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

        if (action==AdminRegionVisitor::skipChildren) {
          return AdminRegionVisitor::visitChildren;
        }
      }
    }
    catch (const IOException& e) {
      log.Error() << e.GetDescription();
      return AdminRegionVisitor::error;
    }

    if (scanner.HasError()) {
      return AdminRegionVisitor::error;
    }

    return AdminRegionVisitor::visitChildren;
  }

  bool LocationIndex::VisitLocations(const AdminRegion& adminRegion,
                                     FileScanner& scanner,
                                     LocationVisitor& visitor,
                                     bool recursive,
                                     bool& stopped) const
  {
    //std::cout << "Visiting locations for " << adminRegion.name << std::endl;

    for (const auto& postalArea : adminRegion.postalAreas) {
      scanner.SetPos(postalArea.objectOffset);

      uint32_t locationCount=scanner.ReadUInt32Number();

      for (uint32_t i=0; i<locationCount; i++) {
        Location location;

        location.locationOffset=scanner.GetPos();
        location.name=scanner.ReadString();
        location.regionOffset=adminRegion.regionOffset;

        uint32_t objectCount=scanner.ReadUInt32Number();
        bool     hasAddresses=scanner.ReadBool();

        if (hasAddresses) {
          location.addressesOffset=scanner.ReadFileOffset();
        }
        else {
          location.addressesOffset=0;
        }

        location.objects=scanner.ReadObjectFileRefs(objectCount);

        //std::cout << "Passing location " << location.name << " " << postalArea.name << " " << adminRegion.name << " to visitor" << std::endl;

        if (!visitor.Visit(adminRegion,
                           postalArea,
                           location)) {
          stopped=true;

          return true;
        }
      }
    }

    if (!recursive) {
      return true;
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

  bool LocationIndex::VisitPostalAreaLocations(const AdminRegion& adminRegion,
                                               const PostalArea& postalArea,
                                               FileScanner& scanner,
                                               LocationVisitor& visitor,
                                               bool& stopped) const
  {
    //std::cout << "Visiting locations for " << postalArea.name << " " << adminRegion.name << std::endl;

    scanner.SetPos(postalArea.objectOffset);

    uint32_t locationCount=scanner.ReadUInt32Number();

    for (uint32_t i=0; i<locationCount; i++) {
      Location location;

      location.locationOffset=scanner.GetPos();
      location.name=scanner.ReadString();
      location.regionOffset=adminRegion.regionOffset;

      uint32_t objectCount=scanner.ReadUInt32Number();
      bool     hasAddresses=scanner.ReadBool();

      if (hasAddresses) {
        location.addressesOffset=scanner.ReadFileOffset();
      }
      else {
        location.addressesOffset=0;
      }

      location.objects=scanner.ReadObjectFileRefs(objectCount);

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

    ObjectFileRefStreamReader objectFileRefReader(scanner);

    uint32_t poiCount=scanner.ReadUInt32Number();

    for (uint32_t i=0; i<poiCount; i++) {
      POI poi;

      poi.regionOffset=region.regionOffset;

      poi.name=scanner.ReadString();
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


    scanner.SetPos(location.addressesOffset);

    uint32_t addressCount=scanner.ReadUInt32Number();

    ObjectFileRefStreamReader objectFileRefReader(scanner);

    for (size_t i=0; i<addressCount; i++) {
      Address address;

      address.addressOffset=scanner.GetPos();
      address.locationOffset=location.locationOffset;
      address.regionOffset=location.regionOffset;

      address.name=scanner.ReadString();

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
    try {
      FileScannerPtr scanner=fileScannerPool.Borrow();
      if (!scanner){
        return false;
      }

      scanner->SetPos(indexOffset);

      std::vector<FileOffset> rootRegionOffsets;

      uint32_t regionCount=scanner->ReadUInt32Number();

      rootRegionOffsets.resize(regionCount);

      for (uint32_t i=0; i<regionCount; i++) {
        rootRegionOffsets[i]=scanner->ReadFileOffset();
      }

      for (uint32_t i=0; i<regionCount; i++) {
        AdminRegion region;

        scanner->SetPos(rootRegionOffsets[i]);

        if (!LoadAdminRegion(*scanner,
                             region)) {
          scanner->Close();
          return false;
        }

        AdminRegionVisitor::Action action;
        FileOffset                 currentPos=scanner->GetPos();


        action=VisitRegionEntries(region,
                                  *scanner,
                                  visitor);

        if (action==AdminRegionVisitor::error) {
          scanner->Close();
          return false;
        }

        if (action==AdminRegionVisitor::stop) {
          scanner->Close();
          return true;
        }

        scanner->SetPos(currentPos);
      }

      return true;
    }
    catch (const IOException& e) {
      log.Error() << e.GetDescription();
      return false;
    }
  }

  bool LocationIndex::VisitAdminRegions(const AdminRegion& adminRegion,
                                        AdminRegionVisitor& visitor) const
  {

    try {
      FileScannerPtr scanner=fileScannerPool.Borrow();
      if (!scanner){
        return false;
      }

      if (!VisitRegionEntries(adminRegion,
                              *scanner,
                              visitor)) {
        scanner->Close();
        return false;
      }

      return true;
    }
    catch (const IOException& e) {
      log.Error() << e.GetDescription();
      return false;
    }
  }

  bool LocationIndex::VisitPOIs(const AdminRegion& region,
                                POIVisitor& visitor,
                                bool recursive) const
  {
    try {
      bool stopped=false;

      FileScannerPtr scanner=fileScannerPool.Borrow();
      if (!scanner){
        return false;
      }

      scanner->SetPos(region.regionOffset);

      if (!VisitRegionPOIs(region,
                           *scanner,
                           visitor,
                           recursive,
                           stopped)) {
        scanner->Close();
        return false;
      }

      return true;
    }
    catch (const IOException& e) {
      log.Error() << e.GetDescription();
      return false;
    }
  }

  bool LocationIndex::VisitLocations(const AdminRegion& adminRegion,
                                     LocationVisitor& visitor,
                                     bool recursive) const
  {
    try {
      bool stopped=false;

      FileScannerPtr scanner=fileScannerPool.Borrow();
      if (!scanner){
        return false;
      }

      if (!VisitLocations(adminRegion,
                          *scanner,
                          visitor,
                          recursive,
                          stopped)) {
        scanner->Close();
        return false;
      }

      return true;
    }
    catch (const IOException& e) {
      log.Error() << e.GetDescription();
      return false;
    }
  }

  bool LocationIndex::VisitLocations(const AdminRegion& adminRegion,
                                     const PostalArea& postalArea,
                                     LocationVisitor& visitor,
                                     bool recursive) const
  {
    try {
      bool stopped=false;

      FileScannerPtr scanner=fileScannerPool.Borrow();
      if (!scanner){
        return false;
      }

      if (!VisitPostalArea(adminRegion,
                           postalArea,
                           *scanner,
                           visitor,
                           recursive,
                           stopped)) {
        scanner->Close();
        return false;
      }

      return true;
    }
    catch (const IOException& e) {
      log.Error() << e.GetDescription();
      return false;
    }
  }

  bool LocationIndex::VisitAddresses(const AdminRegion& region,
                                     const PostalArea& postalArea,
                                     const Location& location,
                                     AddressVisitor& visitor) const
  {
    try {
      bool stopped=false;

      FileScannerPtr scanner=fileScannerPool.Borrow();
      if (!scanner){
        return false;
      }

      if (!VisitLocation(*scanner,
                         region,
                         postalArea,
                         location,
                         visitor,
                         stopped)) {
        scanner->Close();
        return false;
      }

      return true;
    }
    catch (const IOException& e) {
      log.Error() << e.GetDescription();
      return false;
    }
  }

  bool LocationIndex::ResolveAdminRegionHierachie(const AdminRegionRef& adminRegion,
                                                  std::map<FileOffset,AdminRegionRef >& refs) const
  {
    try  {
      FileScannerPtr scanner=fileScannerPool.Borrow();
      if (!scanner){
        return false;
      }

      scanner->SetPos(indexOffset);

      std::list<FileOffset> offsets;

      refs[adminRegion->regionOffset]=adminRegion;

      if (adminRegion->parentRegionOffset!=0) {
        offsets.push_back(adminRegion->parentRegionOffset);
      }

      while (!offsets.empty()) {
        std::list<FileOffset> newOffsets;

        for (const auto& offset : offsets) {
          if (refs.contains(offset)) {
            continue;
          }

          scanner->SetPos(offset);

          AdminRegion currentAdminRegion;

          if (!LoadAdminRegion(*scanner,
                               currentAdminRegion)) {
            return false;
          }

          refs[currentAdminRegion.regionOffset]=std::make_shared<AdminRegion>(currentAdminRegion);

          if (currentAdminRegion.parentRegionOffset!=0) {
            newOffsets.push_back(currentAdminRegion.parentRegionOffset);
          }

        }
        offsets.clear();

        std::swap(offsets,
                  newOffsets);
      }

      return true;
    }
    catch (const IOException& e) {
      log.Error() << e.GetDescription();
      return false;
    }
  }

  void LocationIndex::DumpStatistics() const
  {
    size_t memory=0;

    log.Info() << "CityStreetIndex: Memory " << memory;
  }

  void LocationIndex::FlushCache() const
  {
    fileScannerPool.Clear();
  }
}

