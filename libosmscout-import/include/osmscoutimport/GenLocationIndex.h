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

  class LocationIndexGenerator CLASS_FINAL : public ImportModule
  {
  public:
    static const char* const FILENAME_LOCATION_REGION_TXT;
    static const char* const FILENAME_LOCATION_FULL_TXT;
    static const char* const FILENAME_LOCATION_METRICS_TXT;

  private:
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
     * An area can contain an number of location nodes. Since they do not have
     * their own area we define the node name as an alias for the containing
     * area, since this is the best approximation.
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

    struct RegionLocation CLASS_FINAL
    {
      std::unordered_map<std::string,
                         size_t>      names;            //!< map of names in different case used for this location and their use count
      FileOffset                      dataOffsetOffset; //!< Offset of place where the address list offset is stored
      std::list<ObjectFileRef>        objects;          //!< Objects that represent this location
      std::list<RegionAddress>        addresses;        //!< Addresses at this location

      std::string GetName() const;
    };

    class Region;

    using RegionRef = std::shared_ptr<Region>;

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

    /**
      An area. An area is a administrative region, a city, a country, ...
      An area can have child areas (suburbs, ...).
      An area has a name and also a number of locations, which are possibly
      within the area but area currently also represented by this area.
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

      void AddLocationObject(const std::string& name,
                             const std::string& postalCode,
                             const ObjectFileRef& objectRef);

    protected:
      void CalculateProbePoints();                           //! Finds probe points for this to be used for containment test
      void CalculateProbePointsForArea(size_t areaIndex,     //! Finds probe points for an area
                                       size_t refinement=0);
    };

    class RegionIndex CLASS_FINAL
    {
    public:
      std::map<Pixel,std::list<RegionRef> > index;
      double                                cellWidth;
      double                                cellHeight;

    public:
      RegionRef GetRegionForNode(RegionRef& rootRegion,
                                 const GeoCoord& coord) const;
    };

  private:
    uint8_t                bytesForNodeFileOffset;
    uint8_t                bytesForAreaFileOffset;
    uint8_t                bytesForWayFileOffset;

    ImportErrorReporterRef errorReporter;

  private:
    void Write(FileWriter& writer,
               const ObjectFileRef& object) const;

    void AnalyseStringForIgnoreTokens(const std::string& string,
                                      std::unordered_map<std::string,size_t>& ignoreTokens,
                                      std::unordered_set<std::string>& blacklist);

    void CalculateRegionNameIgnoreTokens(const Region& parent,
                                         std::unordered_map<std::string,size_t>& ignoreTokens,
                                         std::unordered_set<std::string>& blacklist);

    void CalculatePOINameIgnoreTokens(const Region& parent,
                                      std::unordered_map<std::string,size_t>& ignoreTokens,
                                      std::unordered_set<std::string>& blacklist);

    void CalculateLocationNameIgnoreTokens(const Region& parent,
                                           std::unordered_map<std::string,size_t>& ignoreTokens,
                                           std::unordered_set<std::string>& blacklist);

    bool CalculateIgnoreTokens(const Region& rootRegion,
                               std::list<std::string>& regionTokens,
                               std::list<std::string>& poiTokens,
                               std::list<std::string>& locationTokens);


    void CalculateRegionMetrics(const Region& region,
                                RegionMetrics& metrics);

    void DumpRegion(const Region& parent,
                    size_t indent,
                    std::ostream& out);

    void DumpRegionAndData(const Region& parent,
                           size_t indent,
                           std::ostream& out);

    bool DumpRegionTree(Progress& progress,
                        const Region& rootRegion,
                        const std::string& filename);

    bool DumpLocationTree(Progress& progress,
                          const Region& rootRegion,
                          const std::string& filename);

    bool DumpLocationMetrics(Progress& progress,
                             const std::string& filename,
                             const LocationIndexGenerator::RegionMetrics& metrics,
                             const std::list<std::string>& regionIgnoreTokens,
                             const std::list<std::string>& poiIgnoreTokens,
                             const std::list<std::string>& locationIgnoreTokens) const;

    bool AddRegion(Region& parent,
                   const RegionRef& region,
                   bool assume_contains=true);

    bool GetBoundaryAreas(const ImportParameter& parameter,
                          Progress& progress,
                          const TypeConfigRef& typeConfig,
                          const TypeInfoSet& boundaryTypes,
                          std::vector<std::list<RegionRef>>& boundaryAreas) const;

    void SortInBoundaries(Progress& progress,
                          Region& rootRegion,
                          const std::list<RegionRef>& boundaryAreas);

    bool GetRegionAreas(const TypeConfig& typeConfig,
                        const ImportParameter& parameter,
                        Progress& progress,
                        std::list<RegionRef>& regionAreas) const;

    bool SortInRegionAreas(Progress& progress,
                           Region& rootRegion,
                           std::list<RegionRef>& regionAreas);

    void SortInRegion(const RegionRef& area,
                      std::vector<std::list<RegionRef> >& regionTree,
                      unsigned long level);

    unsigned long GetRegionTreeDepth(const Region& rootRegion) const;

    void IndexRegions(const std::vector<std::list<RegionRef> >& regionTree,
                      RegionIndex& regionIndex) const;

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
                                 const std::vector<Point>& nodes,
                                 const std::string& name,
                                 const std::string& postalCode,
                                 const GeoBox& boundingBox);

    void AddLocationAreaToRegion(RegionRef& rootRegion,
                                 const Area& area,
                                 const Area::Ring& ring,
                                 const std::string& name,
                                 const std::string& postalCode,
                                 const RegionIndex& regionIndex);

    bool IndexLocationAreas(const TypeConfig& typeConfig,
                            const ImportParameter& parameter,
                            Progress& progress,
                            RegionRef& rootRegion,
                            const RegionIndex& regionIndex);

    bool AddLocationWayToRegion(Region& region,
                                const Way& way,
                                const std::string& name,
                                const std::string& postalCode,
                                const GeoBox& boundingBox);

    bool IndexLocationWays(const TypeConfig& typeConfig,
                           const ImportParameter& parameter,
                           Progress& progress,
                           RegionRef& rootRegion,
                           const RegionIndex& regionIndex);

    void AddAddressToRegion(Progress& progress,
                            Region& region,
                            const ObjectFileRef& object,
                            const std::string& location,
                            const std::string& address,
                            const std::string &postalCode,
                            bool allowDuplicates,
                            bool& added);

    void AddAddressAreaToRegion(Progress& progress,
                                Region& region,
                                const FileOffset& fileOffset,
                                const std::string& location,
                                const std::string& address,
                                const std::string &postalCode,
                                const std::vector<Point>& nodes,
                                const GeoBox& boundingBox,
                                bool& added);

    void AddPOIAreaToRegion(Progress& progress,
                            Region& region,
                            const FileOffset& fileOffset,
                            const std::string& name,
                            const std::vector<Point>& nodes,
                            const GeoBox& boundingBox,
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
                               const std::vector<Point>& nodes,
                               const GeoBox& boundingBox,
                               bool& added);

    bool AddPOIWayToRegion(Progress& progress,
                           Region& region,
                           const FileOffset& fileOffset,
                           const std::string& name,
                           const std::vector<Point>& nodes,
                           const GeoBox& boundingBox,
                           bool& added);

    bool IndexAddressWays(const TypeConfig& typeConfig,
                          const ImportParameter& parameter,
                          Progress& progress,
                          RegionRef& rootRegion,
                          const RegionIndex& regionIndex);

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
    std::map<std::string,RegionLocation>::iterator FindLocation(Progress& progress,
                                                                const Region& region,
                                                                PostalArea& postalArea,
                                                                const std::string &locationName) const;

    void AddAddressNodeToRegion(Progress& progress,
                                Region& region,
                                const FileOffset& fileOffset,
                                const std::string& location,
                                const std::string& address,
                                const std::string& postalCode,
                                bool& added);

    void AddPOINodeToRegion(Region& region,
                            const FileOffset& fileOffset,
                            const std::string& name,
                            bool& added) const
                            ;

    bool IndexAddressNodes(const TypeConfig& typeConfig,
                           const ImportParameter& parameter,
                           Progress& progress,
                           RegionRef& rootRegion,
                           const RegionIndex& regionIndex);

    void CleanupPostalAreas(Region& region);

    void WriteIgnoreTokens(FileWriter& writer,
                           const std::list<std::string>& regionIgnoreTokens,
                           const std::list<std::string>& poiIgnoreTokens,
                           const std::list<std::string>& locationIgnoreTokens) const;

    void WriteRegionMetrics(FileWriter& writer,
                            const RegionMetrics& metrics) const;

    void WriteRegionIndexEntry(FileWriter& writer,
                               const Region& parentRegion,
                               Region& region);

    void WriteRegionIndex(FileWriter& writer,
                          Region& root);

    void WriteRegionDataEntry(FileWriter& writer,
                              Region& region);

    void WriteRegionData(FileWriter& writer,
                         Region& root);

    void WritePostalArea(FileWriter& writer,
                         PostalArea& postalArea) const;

    void WriteAddressDataEntry(FileWriter& writer,
                               Region& region);

    void WriteAddressData(FileWriter& writer,
                          const Region& root);

  public:
    void GetDescription(const ImportParameter& parameter,
                        ImportModuleDescription& description) const override;

    bool Import(const TypeConfigRef& typeConfig,
                const ImportParameter& parameter,
                Progress& progress) override;
  };
}

#endif
