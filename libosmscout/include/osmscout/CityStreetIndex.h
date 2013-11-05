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

#include <osmscout/AdminRegion.h>
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
  private:
    /**
     * A region is an administrative area that has a name.
     * Regions are structured in a tree. A region could be a country, a county or a city.
     */
    struct Region
    {
      ObjectFileRef reference;       //! Reference to the object defining the region
      FileOffset    offset;          //! Offset into the region datafile
      std::string   name;            //! name of the region
    };

    /**
     * A location is an object within an area. The location has been index by its name.
     */
    struct Loc
    {
      FileOffset                 offset;  //! Offset of the admin region this location is in
      std::vector<ObjectFileRef> objects; //! List of objects that belong to this location
    };

    /**
     * Visitor that gets called for every location found in the given area.
     * It is the task of the visitor to decide if a locations matches the given criteria.
     */
    struct LocationVisitor
    {
      std::string               name;
      bool                      startWith;
      size_t                    limit;
      bool                      limitReached;

      std::string               nameHash;
      std::string               (*hashFunction)(std::string);

      std::list<Location>       locations;

      FileScanner&              scanner;

      LocationVisitor(FileScanner& scanner);

      bool Visit(const std::string& locationName,
                 const Loc &location);
    };

  public:
    static const char* const FILENAME_REGION_DAT;
    static const char* const FILENAME_NAMEREGION_IDX;

  private:
    std::string path;
    std::string (*hashFunction)(std::string);

  private:
    bool LoadRegion(FileScanner& scanner,
                    LocationVisitor& visitor) const;
    bool LoadRegion(FileScanner& scanner,
                    FileOffset offset,
                    LocationVisitor& visitor) const;

  public:
    CityStreetIndex();
    virtual ~CityStreetIndex();

    bool Load(const std::string& path,
              std::string (*hashFunction) (std::string) = NULL);

    /**
     * Get a list of AdminRegions that match the given name.
     */
    bool GetMatchingAdminRegions(const std::string& name,
                                 std::list<AdminRegion>& regions,
                                 size_t limit,
                                 bool& limitReached,
                                 bool startWith) const;

    /**
     * Get a list of locations matching the given name within the given
     * AdminRegion or its child regions.
     */
    bool GetMatchingLocations(const AdminRegion& region,
                              const std::string& name,
                              std::list<Location>& locations,
                              size_t limit,
                              bool& limitReached,
                              bool startWith) const;

    void DumpStatistics();
  };
}

#endif
