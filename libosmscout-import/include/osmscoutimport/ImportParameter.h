#ifndef OSMSCOUT_IMPORTPARAMETER_H
#define OSMSCOUT_IMPORTPARAMETER_H

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

#include <osmscoutimport/ImportErrorReporter.h>
#include <osmscoutimport/ImportImportExport.h>
#include <osmscout/util/Transformation.h>

#include <memory>

namespace osmscout {

class Preprocessor;
class PreprocessorCallback;

class OSMSCOUT_IMPORT_API PreprocessorFactory
{
public:
  virtual ~PreprocessorFactory() = default;

  virtual std::unique_ptr<Preprocessor> GetProcessor(const std::string& filename,
                                                     PreprocessorCallback& callback) const = 0;
};

using PreprocessorFactoryRef = std::shared_ptr<PreprocessorFactory>;

/**
  Collects all parameter that have influence on the import.

  TODO:
  * Add variable defining the output directory (and make all import modules
    respect this parameter).
 */
class OSMSCOUT_IMPORT_API ImportParameter CLASS_FINAL
{
public:
  /**
   * Definition of a router
   */
  class OSMSCOUT_IMPORT_API Router CLASS_FINAL
  {
  private:
    VehicleMask vehicleMask;
    std::string filenamebase;

    public:
    Router(VehicleMask vehicleMask,
    const std::string& filenamebase);

    inline VehicleMask GetVehicleMask() const
    {
      return vehicleMask;
    }

    inline std::string GetFilenamebase() const
    {
      return filenamebase;
    }

    inline std::string GetDataFilename() const
    {
      return filenamebase+".dat";
    }

    inline std::string GetVariantFilename() const
    {
      return filenamebase+"2.dat";
    }

    inline std::string GetIndexFilename() const
    {
      return filenamebase+".idx";
    }
  };

  using RouterRef = std::shared_ptr<Router>;

  enum class AssumeLandStrategy
  {
    disable   = 0, // disable land detection by database objects
    enable    = 1, // enable land detection
    automatic = 2, // disable land detection when data polygon is known
  };

private:
  std::list<std::string>       mapfiles;                 //<! Name of the files containing map data (either *.osm or *.osm.pbf)
  std::string                  typefile;                 //<! Name and path ff type definition file (map.ost.xml)
  std::string                  destinationDirectory;     //<! Name of the destination directory

  ImportErrorReporterRef       errorReporter;            //<! Class for reporting certain import errors to

  size_t                       startStep;                //<! Starting step for import
  size_t                       endStep;                  //<! End step for import
  std::string                  boundingPolygonFile;      //<! Polygon file containing the bounding polygon of the current import
  bool                         eco;                      //<! Eco modus, deletes temporary files ASAP
  std::list<Router>            router;                   //<! Definition of router

  bool                         strictAreas;              //<! Assure that areas conform to "simple" definition

  bool                         sortObjects;              //<! Sort all objects
  size_t                       sortBlockSize;            //<! Number of entries loaded in one sort iteration
  size_t                       sortTileMag;              //<! Zoom level for individual sorting cells

  size_t                       processingQueueSize;      //!< Size of the processing worker queues

  size_t                       numericIndexPageSize;     //<! Size of an numeric index page in bytes

  size_t                       rawCoordBlockSize;        //<! Number of raw coords loaded during import in one go

  bool                         rawNodeDataMemoryMaped;   //<! Use memory mapping for raw node data file access

  bool                         rawWayIndexMemoryMaped;   //<! Use memory mapping for raw way index file access
  bool                         rawWayDataMemoryMaped;    //<! Use memory mapping for raw way data file access
  size_t                       rawWayIndexCacheSize;     //<! Size of the raw way index cache
  size_t                       rawWayBlockSize;          //<! Number of ways loaded during import until nodes get resolved

  bool                         coordDataMemoryMaped;     //<! Use memory mapping for coord data file access
  size_t                       coordIndexCacheSize;      //<! Size of the coord index cache
  size_t                       coordBlockSize;           //<! Maximum number of node ids we resolve in one go

  size_t                       relMaxWays;               //<! Maximum number of ways allowed to resolve a relation
  size_t                       relMaxCoords;             //<! Maximum number of coords allowed to resolve a relation

  bool                         areaDataMemoryMaped;      //<! Use memory mapping for area data file access
  size_t                       areaDataCacheSize;        //<! Size of the area data cache

  bool                         wayDataMemoryMaped;       //<! Use memory mapping for way data file access
  size_t                       wayDataCacheSize;         //<! Size of the way data cache

  size_t                       areaAreaIndexMaxMag;      //<! Maximum depth of the index generated

  MagnificationLevel           areaNodeGridMag;          //<! Magnification level for the index grid
  uint16_t                     areaNodeSimpleListLimit;  //<! If a type has less entries, we just store them plain
  uint16_t                     areaNodeTileListLimit;    //<! If a type has less entries in a tile, we store it as list
  uint16_t                     areaNodeTileListCoordLimit;//<! If a type has less entries we store the coord in tile lists
  MagnificationLevel           areaNodeBitmapMaxMag;      //<! Maximum Magnification level for bitmap index
  uint16_t                     areaNodeBitmapLimit;       //<! All cells must have less entries for a given zoom level

  MagnificationLevel           areaWayMinMag;            //<! Minimum magnification of index for individual type
  MagnificationLevel           areaWayIndexMaxLevel;     //<! Maximum zoom level for area way index bitmap

  MagnificationLevel           areaRouteMinMag;          //<! Minimum magnification of index for individual type
  MagnificationLevel           areaRouteIndexMaxLevel;   //<! Maximum zoom level for area route index bitmap

  uint32_t                     waterIndexMinMag;         //<! Minimum level of the generated water index
  uint32_t                     waterIndexMaxMag;         //<! Maximum level of the generated water index

  size_t                       optimizationMaxWayCount;  //<! Maximum number of ways for one iteration
  MagnificationLevel           optimizationMaxMag;       //<! Maximum magnification for optimization
  MagnificationLevel           optimizationMinMag;       //<! Minimum magnification of index for individual type
  size_t                       optimizationCellSizeAverage; //<! Average entries per index cell
  size_t                       optimizationCellSizeMax;  //<! Maximum number of entries  per index cell
  TransPolygon::OptimizeMethod optimizationWayMethod;    //<! what method to use to optimize ways

  size_t                       routeNodeBlockSize;       //<! Number of route nodes loaded during import until ways get resolved
  uint32_t                     routeNodeTileMag;         //<! Size of a routing tile

  AssumeLandStrategy           assumeLand;               //<! During sea/land detection,we either trust coastlines only or make some
  //<! assumptions which tiles are sea and which are land.
  std::vector<std::string>     langOrder;                //<! languages used when parsing name[:lang] and
  //<! place_name[:lang] tags
  std::vector<std::string>     altLangOrder;             //<! the same as langOrder but for a alt (second) lang

  size_t                       maxAdminLevel;            //<! Maximum admin level that gets evalutated

  OSMId                        firstFreeOSMId;           //<! first id available for synthetic objects (parsed polygon files)
  size_t                       fillWaterArea;            //<! count of tiles around coastlines flooded by water

  PreprocessorFactoryRef       preprocessorFactory;      //<! Optional preprocessor factory to inject custom preprocessors

  public:
  ImportParameter();
  virtual ~ImportParameter();

  const std::list<std::string>& GetMapfiles() const;
  std::string GetTypefile() const;
  std::string GetDestinationDirectory() const;
  std::string GetBoundingPolygonFile() const;

  ImportErrorReporterRef GetErrorReporter() const;

  size_t GetStartStep() const;
  size_t GetEndStep() const;
  bool   IsEco() const;

  const std::list<Router>& GetRouter() const;

  bool GetStrictAreas() const;

  bool GetSortObjects() const;
  size_t GetSortBlockSize() const;
  size_t GetSortTileMag() const;

  size_t GetProcessingQueueSize() const;

  size_t GetNumericIndexPageSize() const;

  size_t GetRawCoordBlockSize() const;

  bool GetRawNodeDataMemoryMaped() const;

  bool GetRawWayIndexMemoryMaped() const;
  bool GetRawWayDataMemoryMaped() const;
  size_t GetRawWayIndexCacheSize() const;
  size_t GetRawWayBlockSize() const;

  bool GetCoordDataMemoryMaped() const;
  size_t GetCoordIndexCacheSize() const;

  size_t GetCoordBlockSize() const;

  size_t GetRelMaxWays() const;
  size_t GetRelMaxCoords() const;

  bool GetAreaDataMemoryMaped() const;
  size_t GetAreaDataCacheSize() const;

  bool GetWayDataMemoryMaped() const;
  size_t GetWayDataCacheSize() const;

  MagnificationLevel GetAreaNodeGridMag() const;
  uint16_t GetAreaNodeSimpleListLimit() const;
  uint16_t GetAreaNodeTileListLimit() const;
  uint16_t GetAreaNodeTileListCoordLimit() const;
  MagnificationLevel GetAreaNodeBitmapMaxMag() const;
  uint16_t GetAreaNodeBitmapLimit() const;

  MagnificationLevel GetAreaWayMinMag() const;
  MagnificationLevel GetAreaWayIndexMaxLevel() const;

  MagnificationLevel GetAreaRouteMinMag() const;
  MagnificationLevel GetAreaRouteIndexMaxLevel() const;

  size_t GetAreaAreaIndexMaxMag() const;

  uint32_t GetWaterIndexMinMag() const;
  uint32_t GetWaterIndexMaxMag() const;

  size_t GetOptimizationMaxWayCount() const;
  MagnificationLevel GetOptimizationMaxMag() const;
  MagnificationLevel GetOptimizationMinMag() const;
  size_t GetOptimizationCellSizeAverage() const;
  size_t GetOptimizationCellSizeMax() const;
  TransPolygon::OptimizeMethod GetOptimizationWayMethod() const;

  size_t GetRouteNodeBlockSize() const;
  uint32_t GetRouteNodeTileMag() const;

  AssumeLandStrategy GetAssumeLand() const;

  OSMId GetFirstFreeOSMId() const;

  const std::vector<std::string>& GetLangOrder () const;
  const std::vector<std::string>& GetAltLangOrder () const;

  size_t GetMaxAdminLevel() const;

  void SetMapfiles(const std::list<std::string>& mapfile);
  void SetTypefile(const std::string& typefile);
  void SetDestinationDirectory(const std::string& destinationDirectory);
  void SetBoundingPolygonFile(const std::string& boundingPolygonFile);

  void SetErrorReporter(const ImportErrorReporterRef& errorReporter);

  void SetStartStep(size_t startStep);
  void SetSteps(size_t startStep, size_t endStep);
  void SetEco(bool eco);

  void ClearRouter();
  void AddRouter(const Router& router);

  void SetStrictAreas(bool strictAreas);

  void SetSortObjects(bool sortObjects);
  void SetSortBlockSize(size_t sortBlockSize);
  void SetSortTileMag(size_t sortTileMag);

  void SetProcessingQueueSize(size_t processingQueueSize);

  void SetNumericIndexPageSize(size_t numericIndexPageSize);

  void SetRawCoordBlockSize(size_t blockSize);

  void SetRawNodeDataMemoryMaped(bool memoryMaped);

  void SetRawWayIndexMemoryMaped(bool memoryMaped);
  void SetRawWayDataMemoryMaped(bool memoryMaped);
  void SetRawWayIndexCacheSize(size_t wayIndexCacheSize);
  void SetRawWayBlockSize(size_t blockSize);

  void SetCoordDataMemoryMaped(bool memoryMaped);
  void SetCoordIndexCacheSize(size_t coordIndexCacheSize);

  void SetCoordBlockSize(size_t coordBlockSize);

  void SetRelMaxWays(size_t relMaxWays);
  void SetRelMaxCoords(size_t relMaxCoords);

  void SetAreaDataMemoryMaped(bool memoryMaped);
  void SetAreaDataCacheSize(size_t areaDataCacheSize);

  void SetWayDataMemoryMaped(bool memoryMaped);
  void SetWayDataCacheSize(size_t wayDataCacheSize);

  void SetAreaAreaIndexMaxMag(size_t areaAreaIndexMaxMag);

  void SetAreaNodeGridMag(MagnificationLevel areaNodeGridMag);
  void SetAreaNodeSimpleListLimit(uint16_t areaNodeSimpleListLimit);
  void SetAreaNodeTileListLimit(uint16_t areaNodeTileListLimit);
  void SetAreaNodeTileListCoordLimit(uint16_t areaNodeTileListCoordLimit);
  void SetAreaNodeBitmapMaxMag(const MagnificationLevel& areaNodeBitmapMaxMag);
  void SetAreaNodeBitmapLimit(uint16_t areaNodeBitmapLimit);

  void SetAreaWayMinMag(MagnificationLevel areaWayMinMag);
  void SetAreaWayIndexMaxMag(MagnificationLevel areaWayIndexMaxLevel);

  void SetAreaRouteMinMag(MagnificationLevel areaRouteMinMag);
  void SetAreaRouteIndexMaxMag(MagnificationLevel areaRouteIndexMaxLevel);

  void SetWaterIndexMinMag(uint32_t waterIndexMinMag);
  void SetWaterIndexMaxMag(uint32_t waterIndexMaxMag);

  void SetOptimizationMaxWayCount(size_t optimizationMaxWayCount);
  void SetOptimizationMaxMag(MagnificationLevel optimizationMaxMag);
  void SetOptimizationMinMag(MagnificationLevel optimizationMinMag);
  void SetOptimizationCellSizeAverage(size_t optimizationCellSizeAverage);
  void SetOptimizationCellSizeMax(size_t optimizationCellSizeMax);
  void SetOptimizationWayMethod(TransPolygon::OptimizeMethod optimizationWayMethod);

  void SetRouteNodeBlockSize(size_t blockSize);
  void SetRouteNodeTileMag(uint32_t routeNodeTileMag);

  void SetAssumeLand(AssumeLandStrategy assumeLand);

  void SetLangOrder(const std::vector<std::string>& langOrder);
  void SetAltLangOrder(const std::vector<std::string>& altLangOrder);

  void SetMaxAdminLevel(size_t maxAdminLevel);

  void SetFirstFreeOSMId(OSMId id);

  void SetFillWaterArea(size_t fillWaterArea);
  size_t GetFillWaterArea() const;

  void SetPreprocessorFactory(const PreprocessorFactoryRef& factory);

  std::unique_ptr<Preprocessor> GetPreprocessor(const std::string& filename,
                                                PreprocessorCallback& callback) const;

  void SetAreaWayIndexMaxLevel(const MagnificationLevel& areaWayIndexMaxLevel);

  static size_t GetDefaultStartStep();
  static size_t GetDefaultEndStep();
};

}

#endif //OSMSCOUT_IMPORTPARAMETER_H
