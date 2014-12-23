#ifndef OSMSCOUT_IMPORT_GENLOCATIONINDEX_H
#define OSMSCOUT_IMPORT_GENLOCATIONINDEX_H

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

  class LocationIndexGenerator : public ImportModule
  {
  private:
    /**
     * An area can contain an number of location nodes. Since they do not have
     * their own area we define the node name as an alias for the containing
     * area, since this is the best approximation.
     */
    struct RegionAlias
    {
      FileOffset  reference; //<! Reference to the node that is the alias
      std::string name;      //<! The alias itself
    };

    /**
     * A POI within a region
     */
    struct RegionPOI
    {
      ObjectFileRef object; //<! Object
      std::string   name;   //<! Name of the POI

      bool operator<(const RegionPOI& other) const
      {
        return object.GetFileOffset()<other.object.GetFileOffset();
      }
    };

    struct RegionAddress
    {
      ObjectFileRef object; //<! Object with the given address
      std::string   name;   //<! The house number

      bool operator<(const RegionAddress& other) const
      {
        return object.GetFileOffset()<other.object.GetFileOffset();
      }
    };

    struct RegionLocation
    {
      FileOffset               addressOffset; //<! Offset of place where the address list offset is stored
      std::list<ObjectFileRef> objects;       //<! Objects that represent this location
      std::list<RegionAddress> addresses;     //<! Addresses at this location
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
      FileOffset                           indexOffset; //<! Offset into the index file
      FileOffset                           dataOffset;  //<! Offset into the index file

      ObjectFileRef                        reference;   //<! Reference to the object this area is based on
      std::string                          name;        //<! The name of this area

      std::list<RegionAlias>               aliases;     //<! Location that are represented by this region
      std::vector<std::vector<GeoCoord> >  areas;       //<! the geometric area of this region

      double                               minlon;
      double                               minlat;
      double                               maxlon;
      double                               maxlat;

      std::list<RegionPOI>                 pois;        //<! A list of POIs in this region
      std::map<std::string,RegionLocation> locations;   //<! list of indexed objects in this region

      std::list<RegionRef>                 regions;     //<! A list of sub regions

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
      ObjectFileRef          reference; //<! Reference of the object that
                                        //<! is the alias
      FileOffset             offset;    //<! File offset of the area

      inline bool operator<(const RegionReference& other) const
      {
        return reference<other.reference;
      }
    };

  private:
    uint8_t bytesForNodeFileOffset;
    uint8_t bytesForAreaFileOffset;
    uint8_t bytesForWayFileOffset;

  private:
    bool Write(FileWriter& writer,
               const ObjectFileRef& object);

    void AnalyseStringForIgnoreTokens(const std::string& string,
                                      OSMSCOUT_HASHMAP<std::string,size_t>& ignoreTokens,
                                      OSMSCOUT_HASHSET<std::string>& blacklist);

    void CalculateRegionNameIgnoreTokens(const Region& parent,
                                         OSMSCOUT_HASHMAP<std::string,size_t>& ignoreTokens,
                                         OSMSCOUT_HASHSET<std::string>& blacklist);

    void CalculateLocationNameIgnoreTokens(const Region& parent,
                                           OSMSCOUT_HASHMAP<std::string,size_t>& ignoreTokens,
                                           OSMSCOUT_HASHSET<std::string>& blacklist);

    bool CalculateIgnoreTokens(const Region& rootRegion,
                               std::list<std::string>& regionTokens,
                               std::list<std::string>& locationTokens);


    void DumpRegion(const Region& parent,
                    size_t indent,
                    std::ostream& out);

    bool DumpLocationTree(Progress& progress,
                          const Region& rootRegion,
                          const std::string& filename);

    void AddRegion(Region& parent,
                   const RegionRef& region);

    bool GetBoundaryAreas(const ImportParameter& parameter,
                          Progress& progress,
                          const TypeConfigRef& typeConfig,
                          const TypeInfoRef& boundaryType,
                          std::list<Boundary>& boundaryAreas);

    void SortInBoundaries(Progress& progress,
                          Region& rootRegion,
                          const std::list<Boundary>& boundaryAreas,
                          size_t level);

    bool IndexRegionAreas(const TypeConfig& typeConfig,
                          const ImportParameter& parameter,
                          Progress& progress,
                          Region& rootRegion);

    void SortInRegion(RegionRef& area,
                      std::vector<std::list<RegionRef> >& regionTree,
                      unsigned long level);

    unsigned long GetRegionTreeDepth(const Region& rootRegion);

    void IndexRegions(const std::vector<std::list<RegionRef> >& regionTree,
                      RegionIndex& regionIndex);

    void AddAliasToRegion(Region& region,
                          const RegionAlias& location,
                          const GeoCoord& node);

    bool IndexRegionNodes(const TypeConfigRef& typeConfig,
                          const ImportParameter& parameter,
                          Progress& progress,
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
                                 const std::string& name,
                                 const RegionIndex& regionIndex);

    bool IndexLocationAreas(const TypeConfig& typeConfig,
                            const ImportParameter& parameter,
                            Progress& progress,
                            RegionRef& rootRegion,
                            const RegionIndex& regionIndex);

    bool AddLocationWayToRegion(Region& region,
                                const Way& way,
                                const std::string& name,
                                double minlon,
                                double minlat,
                                double maxlon,
                                double maxlat);

    bool IndexLocationWays(const TypeConfigRef& typeConfig,
                           const ImportParameter& parameter,
                           Progress& progress,
                           RegionRef& rootRegion,
                           const RegionIndex& regionIndex);

    void AddAddressAreaToRegion(Progress& progress,
                                Region& region,
                                const FileOffset& fileOffset,
                                const std::string& location,
                                const std::string& address,
                                const std::vector<GeoCoord>& nodes,
                                double minlon,
                                double minlat,
                                double maxlon,
                                double maxlat,
                                bool& added);

    void AddPOIAreaToRegion(Progress& progress,
                            Region& region,
                            const FileOffset& fileOffset,
                            const std::string& name,
                            const std::vector<GeoCoord>& nodes,
                            double minlon,
                            double minlat,
                            double maxlon,
                            double maxlat,
                            bool& added);

    bool IndexAddressAreas(const TypeConfig& typeConfig,
                           const ImportParameter& parameter,
                           Progress& progress,
                           RegionRef& rootRegion,
                           const RegionIndex& regionIndex);

    bool AddAddressWayToRegion(Progress& progress,
                               Region& region,
                               const FileOffset& fileOffset,
                               const std::string& location,
                               const std::string& address,
                               const std::vector<GeoCoord>& nodes,
                               double minlon,
                               double minlat,
                               double maxlon,
                               double maxlat,
                               bool& added);

    bool AddPOIWayToRegion(Progress& progress,
                           Region& region,
                           const FileOffset& fileOffset,
                           const std::string& name,
                           const std::vector<GeoCoord>& nodes,
                           double minlon,
                           double minlat,
                           double maxlon,
                           double maxlat,
                           bool& added);

    bool IndexAddressWays(const TypeConfig& typeConfig,
                          const ImportParameter& parameter,
                          Progress& progress,
                          RegionRef& rootRegion,
                          const RegionIndex& regionIndex);

    void AddAddressNodeToRegion(Progress& progress,
                                Region& region,
                                const FileOffset& fileOffset,
                                const std::string& location,
                                const std::string& address,
                                bool& added);

    void AddPOINodeToRegion(Region& region,
                            const FileOffset& fileOffset,
                            const std::string& name,
                            bool& added);

    bool IndexAddressNodes(const TypeConfig& typeConfig,
                           const ImportParameter& parameter,
                           Progress& progress,
                           RegionRef& rootRegion,
                           const RegionIndex& regionIndex);

    bool WriteIgnoreTokens(FileWriter& writer,
                           const std::list<std::string>& regionIgnoreTokens,
                           const std::list<std::string>& locationIgnoreTokens);

    bool WriteRegionIndexEntry(FileWriter& writer,
                               const Region& parentRegion,
                               Region& region);

    bool WriteRegionIndex(FileWriter& writer,
                          Region& root);

    bool WriteRegionDataEntry(FileWriter& writer,
                              Region& region);

    bool WriteRegionData(FileWriter& writer,
                         Region& root);

    bool WriteAddressDataEntry(FileWriter& writer,
                               Region& region);

    bool WriteAddressData(FileWriter& writer,
                          Region& root);

  public:
    std::string GetDescription() const;
    bool Import(const TypeConfigRef& typeConfig,
                const ImportParameter& parameter,
                Progress& progress);
  };
}

#endif
