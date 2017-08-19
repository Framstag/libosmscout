#ifndef OSMSCOUT_LOCATIONINDEX_H
#define OSMSCOUT_LOCATIONINDEX_H

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

#include <list>
#include <memory>
#include <set>
#include <unordered_set>

#include <osmscout/Location.h>
#include <osmscout/TypeConfig.h>

#include <osmscout/util/FileScanner.h>

namespace osmscout {

  /**
   * \ingroup Database
   * Location index returns objects by names (the name should be changed). You
   * can currently either search for regions like 'cities' or for named locations in
   * areas like 'street in city'.
   *
   * Currently every type that has option 'INDEX' set in the map.ost file is indexed as
   * location. Areas are currently build by scanning administrative boundaries and the
   * various sized city typed locations and areas.
   */
  class OSMSCOUT_API LocationIndex
  {
  public:
    static const char* const FILENAME_LOCATION_IDX;

  private:
    std::string                     path;
    mutable uint8_t                 bytesForNodeFileOffset;
    mutable uint8_t                 bytesForAreaFileOffset;
    mutable uint8_t                 bytesForWayFileOffset;
    std::unordered_set<std::string> regionIgnoreTokens;
    std::unordered_set<std::string> locationIgnoreTokens;
    uint32_t                        maxRegionWords;
    uint32_t                        maxPOIWords;
    uint32_t                        maxLocationWords;
    uint32_t                        maxAddressWords;
    FileOffset                      indexOffset;

  private:
    void Read(FileScanner& scanner,
              ObjectFileRef& object) const;

    bool LoadAdminRegion(FileScanner& scanner,
                         AdminRegion& region) const;

    AdminRegionVisitor::Action VisitRegionEntries(const AdminRegion& region,
                                                  FileScanner& scanner,
                                                  AdminRegionVisitor& visitor) const;

    bool VisitRegionPOIs(const AdminRegion& region,
                         FileScanner& scanner,
                         POIVisitor& visitor,
                         bool recursive,
                         bool& stopped) const;

    bool VisitPostalArea(const AdminRegion& adminRegion,
                         const PostalArea& postalArea,
                         FileScanner& scanner,
                         LocationVisitor& visitor,
                         bool recursive,
                         bool& stopped) const;


    bool VisitPostalAreaLocations(const AdminRegion& adminRegion,
                                  const PostalArea& postalArea,
                                  FileScanner& scanner,
                                  LocationVisitor& visitor,
                                  bool& stopped) const;

    bool VisitLocation(FileScanner& scanner,
                       const AdminRegion& region,
                       const PostalArea& postalArea,
                       const Location& location,
                       AddressVisitor& visitor,
                       bool& stopped) const;

  public:
    LocationIndex();
    virtual ~LocationIndex();

    bool Load(const std::string& path);

    bool IsRegionIgnoreToken(const std::string& token) const;
    bool IsLocationIgnoreToken(const std::string& token) const;

    /**
     * Visit all admin regions
     */
    bool VisitAdminRegions(AdminRegionVisitor& visitor) const;

    /**
     * Visit all POIs within the given admin region
     */
    bool VisitPOIs(const AdminRegion& region,
                   POIVisitor& visitor,
                   bool recursive=true) const;

    /**
     * Visit all locations within the given admin region
     */
    bool VisitLocations(const AdminRegion& adminRegion,
                        const PostalArea& postalArea,
                        LocationVisitor& visitor,
                        bool recursive=true) const;

    /**
     * Visit all addresses for a given location (in a given AdminRegion)
     */
    bool VisitAddresses(const AdminRegion& region,
                        const PostalArea& postalArea,
                        const Location& location,
                        AddressVisitor& visitor) const;

    bool ResolveAdminRegionHierachie(const AdminRegionRef& region,
                                     std::map<FileOffset,AdminRegionRef>& refs) const;

    void DumpStatistics();
  };

  typedef std::shared_ptr<LocationIndex> LocationIndexRef;
}

#endif
