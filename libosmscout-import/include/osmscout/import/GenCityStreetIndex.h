#ifndef OSMSCOUT_IMPORT_GENCITYSTREETINDEX_H
#define OSMSCOUT_IMPORT_GENCITYSTREETINDEX_H

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

#include <osmscout/Node.h>
#include <osmscout/Area.h>
#include <osmscout/Way.h>

#include <osmscout/ObjectRef.h>

#include <osmscout/import/Import.h>

namespace osmscout {

  class CityStreetIndexGenerator : public ImportModule
  {
  private:
    /**
     * An area can contain an number of location nodes. Since they do not have
     * their own area we define the nod ename as an alias for the containing
     * area, since this is the best aproximation.
     */
    struct RegionAlias
    {
      FileOffset             reference; //! Reference to the node that is the alias
      std::string            name;      //! The alias itself
    };

    /**
     * A POI within a region
     */
    struct RegionPOI
    {
      ObjectFileRef object; // Object
      std::string   name;   // Name of the POI
    };

    struct RegionAddress
    {
      ObjectFileRef          object;
      std::string            houseNr;
    };

    struct RegionLocation
    {
      std::list<ObjectFileRef> objects;   //! Objects that represent this location
      std::list<RegionAddress> addresses; //! Addresses at this location
    };

    struct Region;

    typedef Ref<Region> RegionRef;

    /**
      An area. An area is a administrative region, a city, a country, ...
      An area can have child areas (suburbs, ...).
      An area has a name and also a number of locations, which are possibly
      within the area but area currently also represented by this area.
      */
    struct Region : public Referencable
    {
      FileOffset                          offset;    //! Offset into the index file

      ObjectFileRef                       reference; //! Reference to the object this area is based on
      std::string                         name;      //! The name of this area

      std::list<RegionAlias>              aliases;   //! Location that are represented by this region
      std::vector<std::vector<GeoCoord> > areas;     //! the geometric area of this region

      double                              minlon;
      double                              minlat;
      double                              maxlon;
      double                              maxlat;

      std::list<RegionPOI>                pois;      //! A list of POIs in this region
      std::map<std::string,RegionLocation> locations; //! list of indexed objects in this region

      std::list<RegionRef>                regions;   //! A list of sub regions

      void CalculateMinMax()
      {
        bool isStart=true;

        for (size_t i=0; i<areas.size(); i++) {
          for (size_t j=0; j<areas[i].size(); j++) {
            if (isStart) {
              minlon=areas[i][j].GetLon();
              maxlon=areas[i][j].GetLon();

              minlat=areas[i][j].GetLat();
              maxlat=areas[i][j].GetLat();

              isStart=false;
            }
            else {
              minlon=std::min(minlon,areas[i][j].GetLon());
              maxlon=std::max(maxlon,areas[i][j].GetLon());

              minlat=std::min(minlat,areas[i][j].GetLat());
              maxlat=std::max(maxlat,areas[i][j].GetLat());
            }
          }
        }
      }
    };

    struct Boundary
    {
      ObjectFileRef                       reference;
      std::string                         name;
      size_t                              level;
      std::vector<std::vector<GeoCoord> > areas;
    };

    class RegionIndex
    {
    public:
      std::map<Pixel,std::list<RegionRef> > index;
      double                                cellWidth;
      double                                cellHeight;

    public:
      RegionRef GetRegionForNode(RegionRef& rootRegion,
                                 const GeoCoord& coord) const;
    };

    /**
     * Reference to an area
     */
    struct RegionReference
    {
      ObjectFileRef          reference; //! Reference of the object that
                                        //! is the alias
      FileOffset             offset;    //! Fileoffset of the area

      inline bool operator<(const RegionReference& other) const
      {
        return reference<other.reference;
      }
    };

  private:
    void DumpRegion(const Region& parent,
                    size_t indent,
                    std::ostream& out);

    bool DumpRegionTree(Progress& progress,
                        const Region& rootRegion,
                        const std::string& filename);

    void AddRegion(Region& parent,
                   const RegionRef& region);

    bool GetBoundaryAreas(const ImportParameter& parameter,
                          Progress& progress,
                          const TypeConfig& typeConfig,
                          TypeId boundaryId,
                          std::list<Boundary>& boundaryAreas);

    void SortInBoundaries(Progress& progress,
                          Region& rootRegion,
                          const std::list<Boundary>& boundaryAreas,
                          size_t level);

    bool IndexRegionAreas(const ImportParameter& parameter,
                          Progress& progress,
                          const TypeConfig& typeConfig,
                          const OSMSCOUT_HASHSET<TypeId>& regionTypes,
                          Region& rootRegion,
                          const RegionIndex& regionIndex);

    void SortInRegion(RegionRef& area,
                      std::vector<std::list<RegionRef> >& regionTree,
                      unsigned long level);

    unsigned long GetRegionTreeDepth(const Region& rootRegion);

    void IndexRegions(const std::vector<std::list<RegionRef> >& regionTree,
                      RegionIndex& regionIndex);

    void AddAliasToRegion(Region& region,
                          const RegionAlias& location,
                          const GeoCoord& node);

    bool IndexRegionNodes(const ImportParameter& parameter,
                          Progress& progress,
                          const TypeConfig& typeConfig,
                          const OSMSCOUT_HASHSET<TypeId>& regionTypes,
                          RegionRef& rootRegion,
                          const RegionIndex& regionIndex);

    bool AddLocationAreaToRegion(Region& region,
                                 const Area& area,
                                 const std::vector<GeoCoord>& nodes,
                                 const std::string& name,
                                 double minlon,
                                 double minlat,
                                 double maxlon,
                                 double maxlat);

    void AddLocationAreaToRegion(RegionRef& rootRegion,
                                 const Area& area,
                                 const Area::Ring& ring,
                                 const RegionIndex& regionIndex);

    bool IndexLocationAreas(const ImportParameter& parameter,
                            Progress& progress,
                            const OSMSCOUT_HASHSET<TypeId>& indexables,
                            RegionRef& rootRegion,
                            const RegionIndex& regionIndex);

    bool AddLocationWayToRegion(Region& region,
                                const Way& way,
                                double minlon,
                                double minlat,
                                double maxlon,
                                double maxlat);

    bool IndexLocationWays(const ImportParameter& parameter,
                           Progress& progress,
                           const OSMSCOUT_HASHSET<TypeId>& indexables,
                           RegionRef& rootRegion,
                           const RegionIndex& regionIndex);

    void AddAddressAreaToRegion(Progress& progress,
                                Region& region,
                                const Area& area,
                                const std::vector<GeoCoord>& nodes,
                                const Area::Ring& ring,
                                double minlon,
                                double minlat,
                                double maxlon,
                                double maxlat,
                                bool& added);

    void AddAddressAreaToRegion(Progress& progress,
                                RegionRef& region,
                                const Area& area,
                                const Area::Ring& ring,
                                const RegionIndex& regionIndex,
                                bool& added);

    void AddPOIAreaToRegion(Progress& progress,
                            Region& region,
                            const Area& area,
                            const std::vector<GeoCoord>& nodes,
                            const Area::Ring& ring,
                            double minlon,
                            double minlat,
                            double maxlon,
                            double maxlat,
                                bool& added);

    void AddPOIAreaToRegion(Progress& progress,
                            RegionRef& region,
                            const Area& area,
                            const Area::Ring& ring,
                            const RegionIndex& regionIndex,
                            bool& added);

    bool IndexAddressAreas(const ImportParameter& parameter,
                           Progress& progress,
                           RegionRef& rootRegion,
                           const OSMSCOUT_HASHSET<TypeId>& poiTypes,
                           const RegionIndex& regionIndex);

    bool AddAddressWayToRegion(Progress& progress,
                               Region& region,
                               const Way& way,
                               double minlon,
                               double minlat,
                               double maxlon,
                               double maxlat,
                               bool& added);

    bool AddPOIWayToRegion(Progress& progress,
                           Region& region,
                           const Way& way,
                           double minlon,
                           double minlat,
                           double maxlon,
                           double maxlat,
                           bool& added);

    bool IndexAddressWays(const ImportParameter& parameter,
                          Progress& progress,
                          RegionRef& rootRegion,
                          const OSMSCOUT_HASHSET<TypeId>& poiTypes,
                          const RegionIndex& regionIndex);

    void AddAddressNodeToRegion(Progress& progress,
                                Region& region,
                                const Node& node,
                                bool& added);

    void AddPOINodeToRegion(Progress& progress,
                            Region& region,
                            const Node& node,
                            bool& added);

    bool IndexAddressNodes(const ImportParameter& parameter,
                           Progress& progress,
                           RegionRef& rootRegion,
                           const OSMSCOUT_HASHSET<TypeId>& poiTypes,
                           const RegionIndex& regionIndex);

    bool WriteRegion(FileWriter& writer,
                     Region& region,
                     FileOffset parentOffset);

    bool WriteRegions(FileWriter& writer,
                      Region& root);

    void GetRegionRefs(const Region& region,
                       std::map<std::string,std::list<RegionReference> >& locationRefs);

    bool WriteRegionRefs(FileWriter& writer,
                         std::map<std::string,std::list<RegionReference> >& locationRefs);

  public:
    std::string GetDescription() const;
    bool Import(const ImportParameter& parameter,
                Progress& progress,
                const TypeConfig& typeConfig);
  };
}

#endif
