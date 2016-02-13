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

    FileScanner scanner;

    try {
      scanner.Open(AppendFileToDir(path,
                                   FILENAME_LOCATION_IDX),
                   FileScanner::LowMemRandom,
                   true);

      if (!(scanner.Read(bytesForNodeFileOffset) &&
            scanner.Read(bytesForAreaFileOffset) &&
            scanner.Read(bytesForWayFileOffset))) {
        return false;
      }

      uint32_t ignoreTokenCount;

      if (!scanner.ReadNumber(ignoreTokenCount)) {
        return false;
      }

      for (size_t i=0; i<ignoreTokenCount; i++) {
        std::string token;

        if (!scanner.Read(token)) {
          return false;
        }

        regionIgnoreTokens.insert(token);
      }

      if (!scanner.ReadNumber(ignoreTokenCount)) {
        return false;
      }

      for (size_t i=0; i<ignoreTokenCount; i++) {
        std::string token;

        if (!scanner.Read(token)) {
          return false;
        }

        locationIgnoreTokens.insert(token);
      }

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
    return regionIgnoreTokens.find(token)!=regionIgnoreTokens.end();
  }

  bool LocationIndex::IsLocationIgnoreToken(const std::string& token) const
  {
    return locationIgnoreTokens.find(token)!=locationIgnoreTokens.end();
  }

  bool LocationIndex::Read(FileScanner& scanner,
                           ObjectFileRef& object) const
  {
    uint8_t    type;
    FileOffset fileOffset;

    if (!scanner.Read(type)) {
      return false;
    }

    switch (type) {
    case refNode:
      if (!scanner.ReadFileOffset(fileOffset,
                                  bytesForNodeFileOffset)) {
        return false;
      }
      break;
    case refArea:
      if (!scanner.ReadFileOffset(fileOffset,
                                  bytesForAreaFileOffset)) {
        return false;
      }
      break;
    case refWay:
      if (!scanner.ReadFileOffset(fileOffset,
                                  bytesForWayFileOffset)) {
        return false;
      }
      break;
    default:
      return false;
    }

    object.Set(fileOffset,
               (RefType)type);

    return true;
  }

  bool LocationIndex::LoadAdminRegion(FileScanner& scanner,
                                      AdminRegion& region) const
  {
    uint32_t aliasCount;

    region.regionOffset=scanner.GetPos();

    if (!scanner.ReadFileOffset(region.dataOffset)) {
      return false;
    }

    if (!scanner.ReadFileOffset(region.parentRegionOffset)) {
      return false;
    }

    if (!scanner.Read(region.name)) {
      return false;
    }

    if (!Read(scanner,
              region.object)) {
      return false;
    }

    if (!scanner.ReadNumber(aliasCount)) {
      return false;
    }

    region.aliases.clear();

    if (aliasCount>0) {
      region.aliases.resize(aliasCount);

      for (size_t i=0; i<aliasCount; i++) {
        if (!scanner.Read(region.aliases[i].name)) {
          return false;
        }

        if (!scanner.ReadFileOffset(region.aliases[i].objectOffset,
                                    bytesForNodeFileOffset)) {
          return false;
        }
      }
    }

    return !scanner.HasError();
  }

  AdminRegionVisitor::Action LocationIndex::VisitRegionEntries(FileScanner& scanner,
                                                               AdminRegionVisitor& visitor) const
  {
    AdminRegion region;
    uint32_t    childCount;

    if (!LoadAdminRegion(scanner,
                         region)) {
      return AdminRegionVisitor::error;
    }

    AdminRegionVisitor::Action action=visitor.Visit(region);

    switch (action) {
    case AdminRegionVisitor::stop:
      return action;
    case AdminRegionVisitor::error:
      return action;
    case AdminRegionVisitor::skipChildren:
      return AdminRegionVisitor::skipChildren;
    case AdminRegionVisitor::visitChildren:
      // just continue...
      break;
    }

    try {
      if (!scanner.ReadNumber(childCount)) {
        return AdminRegionVisitor::error;
      }

      for (size_t i=0; i<childCount; i++) {
        FileOffset nextChildOffset;

        if (!scanner.ReadFileOffset(nextChildOffset)) {
          return AdminRegionVisitor::error;
        }

        action=VisitRegionEntries(scanner,
                                  visitor);

        if (action==AdminRegionVisitor::stop ||
            action==AdminRegionVisitor::error) {
          return action;
        }
        else if (action==AdminRegionVisitor::skipChildren) {
          if (i+1<childCount) {
            scanner.SetPos(nextChildOffset);
          }
          else {
            return AdminRegionVisitor::skipChildren;
          }
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

  bool LocationIndex::LoadRegionDataEntry(FileScanner& scanner,
                                          const AdminRegion& adminRegion,
                                          LocationVisitor& visitor,
                                          bool& stopped) const
  {
    uint32_t poiCount;
    uint32_t locationCount;

    if (!scanner.ReadNumber(poiCount)) {
      return false;
    }

    ObjectFileRefStreamReader objectFileRefReader(scanner);

    for (size_t i=0; i<poiCount; i++) {
      POI poi;

      poi.regionOffset=adminRegion.regionOffset;

      if (!scanner.Read(poi.name)) {
        return false;
      }

      if (!objectFileRefReader.Read(poi.object)) {
        return false;
      }

      if (!visitor.Visit(adminRegion,
                         poi)) {
        stopped=true;

        return true;
      }
    }

    if (!scanner.ReadNumber(locationCount)) {
      return false;
    }

    for (size_t i=0; i<locationCount; i++) {
      Location location;
      uint32_t  objectCount;

      location.locationOffset=scanner.GetPos();

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

      objectFileRefReader.Reset();

      for (size_t j=0; j<objectCount; j++) {
        ObjectFileRef ref;

        if (!objectFileRefReader.Read(ref)) {
          return false;
        }

        location.objects.push_back(ref);
      }

      if (!visitor.Visit(adminRegion,
                         location)) {
        stopped=true;

        return true;
      }
    }

    return !scanner.HasError();
  }

  bool LocationIndex::VisitRegionLocationEntries(FileScanner& scanner,
                                                 LocationVisitor& visitor,
                                                 bool recursive,
                                                 bool& stopped) const
  {
    AdminRegion region;
    FileOffset       childrenOffset;
    uint32_t         childCount;

    if (!LoadAdminRegion(scanner,
                         region)) {
      return false;
    }

    childrenOffset=scanner.GetPos();

    scanner.SetPos(region.dataOffset);

    if (!LoadRegionDataEntry(scanner,
                             region,
                             visitor,
                             stopped)) {
      return false;
    }

    if (stopped || !recursive) {
      return !scanner.HasError();
    }

    scanner.SetPos(childrenOffset);

    if (!scanner.ReadNumber(childCount)) {
      return false;
    }

    for (size_t i=0; i<childCount; i++) {
      FileOffset nextChildOffset;

      if (!scanner.ReadFileOffset(nextChildOffset)) {
        return false;
      }

      if (!VisitRegionLocationEntries(scanner,
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

  bool LocationIndex::VisitLocationAddressEntries(FileScanner& scanner,
                                                  const AdminRegion& region,
                                                  const Location& location,
                                                  AddressVisitor& visitor,
                                                  bool& stopped) const
  {
    uint32_t addressCount;

    scanner.SetPos(location.addressesOffset);

    if (!scanner.ReadNumber(addressCount)) {
      return false;
    }

    ObjectFileRefStreamReader objectFileRefReader(scanner);

    for (size_t i=0; i<addressCount; i++) {
      Address address;

      address.addressOffset=scanner.GetPos();

      address.locationOffset=location.locationOffset;
      address.regionOffset=location.regionOffset;

      if (!scanner.Read(address.name)) {
        return false;
      }

      if (!objectFileRefReader.Read(address.object)) {
        return false;
      }

      if (!visitor.Visit(region,
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

      uint32_t regionCount;

      if (!scanner.ReadNumber(regionCount)) {
        return false;
      }

      for (size_t i=0; i<regionCount; i++) {
        AdminRegionVisitor::Action action;
        FileOffset nextChildOffset;

        if (!scanner.ReadFileOffset(nextChildOffset)) {
          return false;
        }

        action=VisitRegionEntries(scanner,
                                  visitor);

        if (action==AdminRegionVisitor::error) {
          return false;
        }
        else if (action==AdminRegionVisitor::stop) {
          return true;
        }
        else if (action==AdminRegionVisitor::skipChildren) {
          if (i+1<regionCount) {
            scanner.SetPos(nextChildOffset);
          }
        }
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

  bool LocationIndex::VisitAdminRegionLocations(const AdminRegion& region,
                                                LocationVisitor& visitor,
                                                bool recursive) const
  {
    FileScanner scanner;
    bool        stopped=false;

    try {
      scanner.Open(AppendFileToDir(path,
                                   FILENAME_LOCATION_IDX),
                   FileScanner::LowMemRandom,
                   true);

      scanner.SetPos(indexOffset);
      scanner.SetPos(region.regionOffset);

      if (!VisitRegionLocationEntries(scanner,
                                      visitor,
                                      recursive,
                                      stopped)) {
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

  bool LocationIndex::VisitLocationAddresses(const AdminRegion& region,
                                             const Location& location,
                                             AddressVisitor& visitor) const
  {
    FileScanner scanner;
    bool        stopped=false;

    try {
      scanner.Open(AppendFileToDir(path,
                                   FILENAME_LOCATION_IDX),
                   FileScanner::LowMemRandom,
                   true);

      scanner.SetPos(indexOffset);

      if (!VisitLocationAddressEntries(scanner,
                                       region,
                                       location,
                                       visitor,
                                       stopped)) {
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

