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

#include <map>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include <osmscout/Node.h>
#include <osmscout/Area.h>
#include <osmscout/Way.h>

#include <osmscout/ObjectRef.h>

#include <osmscout/TypeInfoSet.h>

#include <osmscoutimport/Import.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

  namespace locidx {

    /**
     * Holds some metrics regarding names in the given region. These metrics
     * are input for some heuristics regarding searching.
     */
    struct RegionMetrics CLASS_FINAL
    {
      uint32_t minRegionChars=std::numeric_limits<uint32_t>::max();
      uint32_t maxRegionChars=0;
      uint32_t minRegionWords=std::numeric_limits<uint32_t>::max();
      uint32_t maxRegionWords=0;
      uint32_t maxPOIWords=0;
      uint32_t minLocationChars=std::numeric_limits<uint32_t>::max();
      uint32_t maxLocationChars=0;
      uint32_t minLocationWords=std::numeric_limits<uint32_t>::max();
      uint32_t maxLocationWords=0;
      uint32_t maxAddressWords=0;

      RegionMetrics() = default;
    };

    /**
     * An region can contain an number of location nodes. Since they do not have
     * their own area we define the node name as an alias for the containing
     * region, since this is the best approximation.
     */
    struct RegionAlias CLASS_FINAL
    {
      FileOffset  reference; //!< Reference to the node that is the alias
      std::string name;      //!< The alias itself
      std::string altName;
    };

    /**
     * A POI within a region
     */
    struct RegionPOI CLASS_FINAL
    {
      std::string   name;   //!< Name of the POI
      ObjectFileRef object; //!< Object

      RegionPOI(const std::string& name,
                const ObjectFileRef& object)
        : name(name),
          object(object)
      {
        // no code
      }

      bool operator<(const RegionPOI& other) const
      {
        return object.GetFileOffset()<other.object.GetFileOffset();
      }
    };

    /**
     * An address within a region (normally a house number)
     */
    struct RegionAddress CLASS_FINAL
    {
      std::string   name;       //!< The house number
      ObjectFileRef object;     //!< Object with the given address

      RegionAddress(const std::string& name,
                    const ObjectFileRef& object)
        : name(name),
          object(object)
      {
        // no code
      }

      bool operator<(const RegionAddress& other) const
      {
        return object.GetFileOffset()<other.object.GetFileOffset();
      }
    };

    /**
     * A location within a region. A location is represented by a number of objects.
     * A location holds a list of addresses.
     */
    struct RegionLocation CLASS_FINAL
    {
      std::unordered_map<std::string,
                         size_t>      names;            //!< map of names in different case used for this location and their use count
      FileOffset                      dataOffsetOffset; //!< Offset of place where the address list offset is stored
      std::list<ObjectFileRef>        objects;          //!< Objects that represent this location
      std::list<RegionAddress>        addresses;        //!< Addresses at this location

      std::string GetName() const;
    };

    /**
     * A postal area is a list of locations that are represented by an id.
     */
    struct PostalArea CLASS_FINAL
    {
      std::string                          name;             //!< name of the postal area
      FileOffset                           dataOffsetOffset; //!< Offset into the index file
      std::map<std::string,RegionLocation> locations;        //!< list of indexed objects in this region

      explicit PostalArea(const std::string& name)
        : name(name)
      {
        // no code
      }

      void AddLocationObject(const std::string& name,
                             const ObjectFileRef& objectRef);
    };

    class Region;

    using RegionRef = std::shared_ptr<Region>;

    /**
      An region is an area. An region represents a administrative region like a city, a country, ...

      The region has a name, is represented by an objects and may have a number of aliases (that also name the region
      or parts of the region).

      It may also have an admin level.

      A region consists of a number of areas that represent the region. The areas and the region itself
      have bounding boxes.

      An region can have child regions (suburbs, ...).

      The region may have a list of POIs within its area.
      An region holds a number of postal areas. There is always an (unnamed9 default postal area.
      */
    class Region CLASS_FINAL
    {
    public:
      using PostalAreaMap = std::unordered_map<std::string, PostalArea>;

    private:
      std::vector<GeoBox>                boundingBoxes;      //!< bounding box of each area building up the region (see areas)
      GeoBox                             boundingBox;        //!< Overall bounding box of all areas
      std::vector<GeoCoord>              probePoints;        //!< Points within the region that can be used to check region containment

    public:
      FileOffset                         indexOffset;        //!< Offset into the index file
      FileOffset                         dataOffset;         //!< Offset into the index file

      ObjectFileRef                      reference;          //!< Reference to the object this area is based on
      std::string                        name;               //!< The name of this area
      std::string                        altName;
      std::string                        isIn;               //!< Name of the parent region as stated in OSM (is_in tag)
      std::list<RegionAlias>             aliases;            //!< Location that are represented by this region
      int8_t                             level{-1};          //!< Admin level or -1 if not set

      std::vector<std::vector<GeoCoord>> areas;              //!< the geometric area of this region
      std::list<RegionPOI>               pois;               //!< A list of POIs in this region
      PostalAreaMap                      postalAreas;        //!< Collection of objects without a postal code
      PostalAreaMap::iterator            defaultPostalArea;  //!< PostalArea for postal code ""

      std::list<RegionRef>               regions;            //!< A list of sub regions

    public:
      Region();

      void CalculateMinMax();
      bool CouldContain(const GeoBox& boundingBox) const;
      bool CouldContain(const Region& region, bool strict) const;

      bool Contains(Region& child) const;                 //! Checks whether child is within this

      GeoBox GetBoundingBox() const
      {
        return boundingBox;
      }

      std::vector<GeoBox> GetAreaBoundingBoxes() const
      {
        return boundingBoxes;
      }

      void AddAlias(const RegionAlias& location,
                    const GeoCoord& node);

      void AddPOINode(const FileOffset& fileOffset,
                      const std::string& name,
                      bool& added);

      void AddPOIArea(const FileOffset& fileOffset,
                      const std::string& name,
                      const std::vector<Point>& nodes,
                      const GeoBox& boundingBox,
                      bool& added);

      bool AddPOIWay(const FileOffset& fileOffset,
                     const std::string& name,
                     const std::vector<Point>& nodes,
                     const GeoBox& boundingBox,
                     bool& added);

      void AddLocationObject(const std::string& name,
                             const std::string& postalCode,
                             const ObjectFileRef& objectRef);

      bool AddLocationArea(const Area& area,
                           const std::vector<Point>& nodes,
                           const std::string& name,
                           const std::string& postalCode,
                           const GeoBox& boundingBox);

      bool AddLocationWay(const Way& way,
                          const std::string& name,
                          const std::string& postalCode,
                          const GeoBox& boundingBox);

      bool AddRegion(const RegionRef& region,
                     bool assume_contains=true);

    private:
      void CalculateProbePoints();                           //! Finds probe points for this to be used for containment test
      void CalculateProbePointsForArea(size_t areaIndex,     //! Finds probe points for an area
                                       size_t refinement=0);
    };

    class RegionIndex CLASS_FINAL
    {
    private:
      double                                cellWidth;
      double                                cellHeight;
      std::map<Pixel,std::list<RegionRef> > index;

    public:
      RegionIndex(double cellWidth,
                  double cellheight);

      void IndexRegions(const std::vector<std::list<locidx::RegionRef> >& regionTree);

      RegionRef GetRegionForNode(const RegionRef& rootRegion,
                                 const GeoCoord& coord) const;
    };

  }

  class LocationIndexGenerator CLASS_FINAL : public ImportModule
  {
  public:
    static const char* const FILENAME_LOCATION_REGION_TXT;
    static const char* const FILENAME_LOCATION_FULL_TXT;
    static const char* const FILENAME_LOCATION_METRICS_TXT;

  private:
    uint8_t                bytesForNodeFileOffset;
    uint8_t                bytesForAreaFileOffset;
    uint8_t                bytesForWayFileOffset;

    ImportErrorReporterRef errorReporter;

  private:
    void Write(FileWriter& writer,
               const ObjectFileRef& object) const;

    void CalculateRegionMetrics(const locidx::Region& region,
                                locidx::RegionMetrics& metrics) const;

    void DumpRegion(const locidx::Region& parent,
                    size_t indent,
                    std::ostream& out) const;

    void DumpRegionAndData(const locidx::Region& parent,
                           size_t indent,
                           std::ostream& out) const;

    bool DumpRegionTree(Progress& progress,
                        const locidx::Region& rootRegion,
                        const std::string& filename) const;

    bool DumpLocationTree(Progress& progress,
                          const locidx::Region& rootRegion,
                          const std::string& filename) const;

    bool DumpLocationMetrics(Progress& progress,
                             const std::string& filename,
                             const locidx::RegionMetrics& metrics,
                             const std::list<std::string>& regionIgnoreTokens,
                             const std::list<std::string>& poiIgnoreTokens,
                             const std::list<std::string>& locationIgnoreTokens) const;

    bool GetBoundaryAreas(const ImportParameter& parameter,
                          Progress& progress,
                          const TypeConfigRef& typeConfig,
                          const TypeInfoSet& boundaryTypes,
                          std::vector<std::list<locidx::RegionRef>>& boundaryAreas) const;

    void SortInBoundaries(Progress& progress,
                          locidx::Region& rootRegion,
                          const std::list<locidx::RegionRef>& boundaryAreas);

    bool GetRegionAreas(const TypeConfig& typeConfig,
                        const ImportParameter& parameter,
                        Progress& progress,
                        std::list<locidx::RegionRef>& regionAreas) const;

    bool SortInRegionAreas(Progress& progress,
                           locidx::Region& rootRegion,
                           std::list<locidx::RegionRef>& regionAreas);

    void SortInRegion(const locidx::RegionRef& area,
                      std::vector<std::list<locidx::RegionRef> >& regionTree,
                      unsigned long level);

    unsigned long GetRegionTreeDepth(const locidx::Region& rootRegion) const;

    bool IndexRegionNodes(const TypeConfigRef& typeConfig,
                          const ImportParameter& parameter,
                          Progress& progress,
                          const locidx::RegionIndex& regionIndex,
                          locidx::RegionRef& rootRegion);

    bool IndexLocationAreas(const TypeConfig& typeConfig,
                            const ImportParameter& parameter,
                            Progress& progress,
                            const locidx::RegionIndex& regionIndex,
                            locidx::RegionRef& rootRegion);

    bool IndexLocationWays(const TypeConfig& typeConfig,
                           const ImportParameter& parameter,
                           Progress& progress,
                           const locidx::RegionIndex& regionIndex,
                           locidx::RegionRef& rootRegion);

    void AddLocationAreaToRegion(locidx::RegionRef& rootRegion,
                                 const Area& area,
                                 const Area::Ring& ring,
                                 const std::string& name,
                                 const std::string& postalCode,
                                 const locidx::RegionIndex& regionIndex);

    void AddAddressToRegion(Progress& progress,
                            locidx::Region& region,
                            const ObjectFileRef& object,
                            const std::string& location,
                            const std::string& address,
                            const std::string &postalCode,
                            bool allowDuplicates,
                            bool& added);

    void AddAddressAreaToRegion(Progress& progress,
                                locidx::Region& region,
                                const FileOffset& fileOffset,
                                const std::string& location,
                                const std::string& address,
                                const std::string &postalCode,
                                const std::vector<Point>& nodes,
                                const GeoBox& boundingBox,
                                bool& added);

    bool IndexAddressAreas(const TypeConfig& typeConfig,
                           const ImportParameter& parameter,
                           Progress& progress,
                           locidx::RegionRef& rootRegion,
                           const locidx::RegionIndex& regionIndex);

    bool AddAddressWayToRegion(Progress& progress,
                               locidx::Region& region,
                               const FileOffset& fileOffset,
                               const std::string& location,
                               const std::string& address,
                               const std::vector<Point>& nodes,
                               const GeoBox& boundingBox,
                               bool& added);

    bool IndexAddressWays(const TypeConfig& typeConfig,
                          const ImportParameter& parameter,
                          Progress& progress,
                          const locidx::RegionRef& rootRegion,
                          const locidx::RegionIndex& regionIndex);

    /**
     * Find location by its name in region.locations map.
     *
     * OSM data does not have strict rules how data should be organized.
     * Sometimes tag "addr:street" (`locationName` in this method) contains
     * not existent place (entry in `locations` map don't exists),
     * many times it contains typos or lowercase/uppercase errors.
     *
     * This method try to fix some simple errors at least.
     *
     * @return a valid iterator to the location else locations.end()
     */
    std::map<std::string,locidx::RegionLocation>::iterator FindLocation(Progress& progress,
                                                                const locidx::Region& region,
                                                                locidx::PostalArea& postalArea,
                                                                const std::string &locationName) const;

    void AddAddressNodeToRegion(Progress& progress,
                                locidx::Region& region,
                                const FileOffset& fileOffset,
                                const std::string& location,
                                const std::string& address,
                                const std::string& postalCode,
                                bool& added);

    bool IndexAddressNodes(const TypeConfig& typeConfig,
                           const ImportParameter& parameter,
                           Progress& progress,
                           const locidx::RegionRef& rootRegion,
                           const locidx::RegionIndex& regionIndex);

    void CleanupPostalAreasAdresses(locidx::Region& region);
    void CleanupPostalAreasLocations(locidx::Region& region);
    void CleanupPostalAreas(locidx::Region& region);

    void SortLocationTree(locidx::Region& region);

    void ValidateIsIn(locidx::Region& region);

    void WriteIgnoreTokens(FileWriter& writer,
                           const std::list<std::string>& regionIgnoreTokens,
                           const std::list<std::string>& poiIgnoreTokens,
                           const std::list<std::string>& locationIgnoreTokens) const;

    void WriteRegionMetrics(FileWriter& writer,
                            const locidx::RegionMetrics& metrics) const;

    void WriteRegionIndexEntry(FileWriter& writer,
                               const locidx::Region& parentRegion,
                               locidx::Region& region);

    void WriteRegionIndex(FileWriter& writer,
                          locidx::Region& region);

    void WriteRegionDataEntry(FileWriter& writer,
                              locidx::Region& region);

    void WriteRegionData(FileWriter& writer,
                         locidx::Region& region);

    void WritePostalArea(FileWriter& writer,
                         locidx::PostalArea& postalArea) const;

    void WriteAddressDataEntry(FileWriter& writer,
                               const locidx::Region& region);

    void WriteAddressData(FileWriter& writer,
                          const locidx::Region& region);

  public:
    void GetDescription(const ImportParameter& parameter,
                        ImportModuleDescription& description) const override;

    bool Import(const TypeConfigRef& typeConfig,
                const ImportParameter& parameter,
                Progress& progress) override;
  };
}

#endif
