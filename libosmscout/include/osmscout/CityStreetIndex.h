#ifndef OSMSCOUT_CITYSTREETINDEX_H
#define OSMSCOUT_CITYSTREETINDEX_H

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
#include <set>

#include <osmscout/Location.h>
#include <osmscout/TypeConfig.h>

#include <osmscout/util/FileScanner.h>

namespace osmscout {

  /**
   * City street index returns objects by names (the name should be changed). You
   * can currently either search for regions like 'cities' or for named locations in
   * areas like 'street in city'.
   *
   * Currently every type that has option 'INDEX' set in the map.ost file is indexed as
   * location. Areas are currently build by scanning administrative boundaries and the
   * various sized city typed locations and areas.
   */
  class OSMSCOUT_API CityStreetIndex
  {
  public:
    static const char* const FILENAME_REGION_DAT;

  private:
    std::string path;

  private:
    bool LoadAdminRegion(FileScanner& scanner,
                         AdminRegion& region) const;

    bool VisitRegionEntries(FileScanner& scanner,
                            AdminRegionVisitor& visitor) const;

    bool VisitRegionLocationEntries(FileScanner& scanner,
                                    LocationVisitor& visitor) const;

    bool LoadRegionDataEntry(FileScanner& scanner,
                             const AdminRegion& region,
                             LocationVisitor& visitor) const;

    bool VisitLocationAddressEntries(FileScanner& scanner,
                                     const Location& location,
                                     AddressVisitor& visitor) const;

  public:
    CityStreetIndex();
    virtual ~CityStreetIndex();

    bool Load(const std::string& path);

    /**
     * Visit all admin regions
     */
    bool VisitAdminRegions(AdminRegionVisitor& visitor) const;

    /**
     * Visit all locations within the given admin region
     */
    bool VisitAdminRegionLocations(const AdminRegion& region,
                                   LocationVisitor& visitor) const;

    /**
     * Visit all addresses for a given location (in a given AdminRegion)
     */
    bool VisitLocationAddresses(const Location& location,
                                AddressVisitor& visitor) const;

    bool ResolveAdminRegionHierachie(const AdminRegionRef& region,
                                     std::map<FileOffset,AdminRegionRef>& refs) const;

    void DumpStatistics();
  };
}

#endif
