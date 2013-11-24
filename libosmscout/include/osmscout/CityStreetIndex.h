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
     * A location is an object within an area. The location has been indexed by its name.
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

      std::list<Location>       locations;

      FileScanner&              scanner;

      LocationVisitor(FileScanner& scanner);

      bool Visit(const std::string& locationName,
                 const Loc &location);
    };

  public:
    static const char* const FILENAME_REGION_DAT;

  public:
    struct RegionAlias
    {
      std::string name;
      FileOffset  offset;
    };

    /**
     * A region is an administrative area that has a name.
     * Regions are structured in a tree. A region could be a country, a county or a city.
     */
    struct RegionEntry
    {
      FileOffset               indexOffset;       //! Offset of this entry
      FileOffset               dataOffset;        //! Offset of the data
      FileOffset               parentIndexOffset; //! Offset of the parent region index entry
      std::string              name;              //! name of the region
      ObjectFileRef            reference;         //! The object that represents this region
      std::vector<RegionAlias> aliases;           //! The list of alias for this region
    };

    class RegionVisitor
    {
    public:
      virtual ~RegionVisitor();

      virtual void Initialize();
      virtual bool Visit(const RegionEntry& region) = 0;
    };

    class RegionMatchVisitor : public RegionVisitor
    {
    private:
      std::string pattern;
      size_t      limit;

    public:
      std::list<AdminRegion> matches;
      std::list<AdminRegion> candidates;
      bool                   limitReached;

    private:
      void Match(const std::string& name,
                 bool& match,
                 bool& candidate) const;

    public:
      RegionMatchVisitor(const std::string& pattern,
                         size_t limit);

      virtual void Initialize();
      virtual bool Visit(const RegionEntry& region);
    };

  private:
    std::string path;

  private:
    bool LoadRegionEntry(FileScanner& scanner,
                         RegionEntry& region) const;

    bool VisitRegionEntries(FileScanner& scanner,
                            RegionVisitor& visitor) const;

    bool VisitRegionDataEntries(FileScanner& scanner,
                                LocationVisitor& visitor) const;

    bool LoadRegionDataEntry(FileScanner& scanner,
                             const RegionEntry& region,
                             LocationVisitor& visitor) const;

  public:
    CityStreetIndex();
    virtual ~CityStreetIndex();

    bool Load(const std::string& path);

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
