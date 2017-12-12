#ifndef OSMSCOUT_IMPORT_IMPORT_H
#define OSMSCOUT_IMPORT_IMPORT_H

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
#include <mutex>
#include <string>

#include <osmscout/ImportFeatures.h>

#include <osmscout/private/ImportImportExport.h>

#include <osmscout/TypeConfig.h>

#include <osmscout/import/ImportErrorReporter.h>

#include <osmscout/util/Progress.h>
#include <osmscout/util/Transformation.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

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

    typedef std::shared_ptr<Router> RouterRef;

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

    bool                         areaDataMemoryMaped;      //<! Use memory mapping for area data file access
    size_t                       areaDataCacheSize;        //<! Size of the area data cache

    bool                         wayDataMemoryMaped;       //<! Use memory mapping for way data file access
    size_t                       wayDataCacheSize;         //<! Size of the way data cache

    size_t                       areaAreaIndexMaxMag;      //<! Maximum depth of the index generated

    size_t                       areaNodeMinMag;           //<! Minimum magnification of index for individual type
    double                       areaNodeIndexMinFillRate; //<! Minimum rate of filled cells in index bitmap
    size_t                       areaNodeIndexCellSizeAverage; //<! Average entries per index cell
    size_t                       areaNodeIndexCellSizeMax; //<! Maximum number of entries  per index cell

    size_t                       areaWayMinMag;            //<! Minimum magnification of index for individual type
    size_t                       areaWayIndexMaxLevel;     //<! Maximum zoom level for area way index bitmap

    size_t                       waterIndexMinMag;         //<! Minimum level of the generated water index
    size_t                       waterIndexMaxMag;         //<! Maximum level of the generated water index

    size_t                       optimizationMaxWayCount;  //<! Maximum number of ways for one iteration
    size_t                       optimizationMaxMag;       //<! Maximum magnification for optimization
    size_t                       optimizationMinMag;       //<! Minimum magnification of index for individual type
    size_t                       optimizationCellSizeAverage; //<! Average entries per index cell
    size_t                       optimizationCellSizeMax;  //<! Maximum number of entries  per index cell
    TransPolygon::OptimizeMethod optimizationWayMethod;    //<! what method to use to optimize ways

    size_t                       routeNodeBlockSize;       //<! Number of route nodes loaded during import until ways get resolved

    AssumeLandStrategy           assumeLand;               //<! During sea/land detection,we either trust coastlines only or make some
                                                           //<! assumptions which tiles are sea and which are land.
    std::vector<std::string>     langOrder;                //<! languages used when parsing name[:lang] and
                                                           //<! place_name[:lang] tags
    std::vector<std::string>     altLangOrder;             //<! the same as langOrder but for a alt (second) lang

    size_t                       maxAdminLevel;            //<! Maximum admin level that gets evalutated

    OSMId                        firstFreeOSMId;           //<! first id available for synthetic objects (parsed polygon files)
    size_t                       fillWaterArea;            //<! count of tiles around coastlines flooded by water

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

    bool GetAreaDataMemoryMaped() const;
    size_t GetAreaDataCacheSize() const;

    bool GetWayDataMemoryMaped() const;
    size_t GetWayDataCacheSize() const;

    size_t GetAreaNodeMinMag() const;
    double GetAreaNodeIndexMinFillRate() const;
    size_t GetAreaNodeIndexCellSizeAverage() const;
    size_t GetAreaNodeIndexCellSizeMax() const;

    size_t GetAreaWayMinMag() const;
    size_t GetAreaWayIndexMaxLevel() const;

    size_t GetAreaAreaIndexMaxMag() const;

    size_t GetWaterIndexMinMag() const;
    size_t GetWaterIndexMaxMag() const;

    size_t GetOptimizationMaxWayCount() const;
    size_t GetOptimizationMaxMag() const;
    size_t GetOptimizationMinMag() const;
    size_t GetOptimizationCellSizeAverage() const;
    size_t GetOptimizationCellSizeMax() const;
    TransPolygon::OptimizeMethod GetOptimizationWayMethod() const;

    size_t GetRouteNodeBlockSize() const;

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

    void SetAreaDataMemoryMaped(bool memoryMaped);
    void SetAreaDataCacheSize(size_t areaDataCacheSize);

    void SetWayDataMemoryMaped(bool memoryMaped);
    void SetWayDataCacheSize(size_t wayDataCacheSize);

    void SetAreaAreaIndexMaxMag(size_t areaAreaIndexMaxMag);

    void SetAreaNodeMinMag(size_t areaNodeMinMag);
    void SetAreaNodeIndexMinFillRate(double areaNodeIndexMinFillRate);
    void SetAreaNodeIndexCellSizeAverage(size_t areaNodeIndexCellSizeAverage);
    void SetAreaNodeIndexCellSizeMax(size_t areaNodeIndexCellSizeMax);

    void SetAreaWayMinMag(size_t areaWayMinMag);
    void SetAreaWayIndexMaxMag(size_t areaWayIndexMaxLevel);

    void SetWaterIndexMinMag(size_t waterIndexMinMag);
    void SetWaterIndexMaxMag(size_t waterIndexMaxMag);

    void SetOptimizationMaxWayCount(size_t optimizationMaxWayCount);
    void SetOptimizationMaxMag(size_t optimizationMaxMag);
    void SetOptimizationMinMag(size_t optimizationMinMag);
    void SetOptimizationCellSizeAverage(size_t optimizationCellSizeAverage);
    void SetOptimizationCellSizeMax(size_t optimizationCellSizeMax);
    void SetOptimizationWayMethod(TransPolygon::OptimizeMethod optimizationWayMethod);

    void SetRouteNodeBlockSize(size_t blockSize);

    void SetAssumeLand(AssumeLandStrategy assumeLand);

    void SetLangOrder(const std::vector<std::string>& langOrder);
    void SetAltLangOrder(const std::vector<std::string>& altLangOrder);

    void SetMaxAdminLevel(size_t maxAdminLevel);

    void SetFirstFreeOSMId(OSMId id);

    void SetFillWaterArea(size_t fillWaterArea);
    size_t GetFillWaterArea() const;
  };

  class OSMSCOUT_IMPORT_API ImportModuleDescription CLASS_FINAL
  {
  private:
    std::string            name;
    std::string            description;
    std::list<std::string> providedFiles;
    std::list<std::string> providedOptionalFiles;
    std::list<std::string> providedDebuggingFiles;
    std::list<std::string> providedTemporaryFiles;
    std::list<std::string> providedAnalysisFiles;
    std::list<std::string> requiredFiles;

  public:
    void SetName(const std::string& name);
    void SetDescription(const std::string& description);

    void AddProvidedFile(const std::string& providedFile);
    void AddProvidedOptionalFile(const std::string& providedFile);
    void AddProvidedDebuggingFile(const std::string& providedFile);
    void AddProvidedTemporaryFile(const std::string& providedFile);
    void AddProvidedAnalysisFile(const std::string& providedFile);
    void AddRequiredFile(const std::string& requiredFile);

    inline std::string GetName() const
    {
      return name;
    }

    inline std::string GetDescription() const
    {
      return description;
    }

    inline std::list<std::string> GetProvidedFiles() const
    {
      return providedFiles;
    }

    inline std::list<std::string> GetProvidedOptionalFiles() const
    {
      return providedOptionalFiles;
    }

    inline std::list<std::string> GetProvidedDebuggingFiles() const
    {
      return providedDebuggingFiles;
    }

    inline std::list<std::string> GetProvidedTemporaryFiles() const
    {
      return providedTemporaryFiles;
    }

    inline std::list<std::string> GetProvidedAnalysisFiles() const
    {
      return providedAnalysisFiles;
    }

    inline std::list<std::string> GetRequiredFiles() const
    {
      return requiredFiles;
    }
  };

  /**
    A single import module representing a single import step.

    An import consists of a number of sequentially executed steps. A step normally
    works on one object type and generates one output file (though this is just
    an suggestion). Such a step is realized by a ImportModule.
    */
  class OSMSCOUT_IMPORT_API ImportModule
  {
  public:
    virtual ~ImportModule();

    virtual void GetDescription(const ImportParameter& parameter,
                                ImportModuleDescription& description) const;

    /**
     * Do the import
     *
     * @param typeConfig
     *   Type configuration
     * @param parameter
     *   Import parameter
     * @param progress
     *   Progress object, for tracking import progress
     */
    virtual bool Import(const TypeConfigRef& typeConfig,
                        const ImportParameter& parameter,
                        Progress& progress) = 0;
  };

  typedef std::shared_ptr<ImportModule> ImportModuleRef;

  /**
    Does the import based on the given parameters. Feedback about the import progress
    is given by the indivudal import modules calling the Progress instance as appropriate.
    */
  class OSMSCOUT_IMPORT_API Importer
  {
  private:
    ImportParameter                      parameter;
    std::vector<ImportModuleRef>         modules;
    std::vector<ImportModuleDescription> moduleDescriptions;

  private:
    bool ValidateDescription(Progress& progress);
    bool ValidateParameter(Progress& progress);
    void GetModuleList(std::vector<ImportModuleRef>& modules);
    void DumpTypeConfigData(const TypeConfig& typeConfig,
                            Progress& progress);
    void DumpModuleDescription(const ImportModuleDescription& description,
                               Progress& progress);
    bool CleanupTemporaries(size_t currentStep,
                            Progress& progress);

    bool ExecuteModules(const TypeConfigRef& typeConfig,
                        Progress& progress);
  public:
    explicit Importer(const ImportParameter& parameter);
    virtual ~Importer();

    bool Import(Progress& progress);

    std::list<std::string> GetProvidedFiles() const;
    std::list<std::string> GetProvidedOptionalFiles() const;
    std::list<std::string> GetProvidedDebuggingFiles() const;
    std::list<std::string> GetProvidedTemporaryFiles() const;
    std::list<std::string> GetProvidedAnalysisFiles() const;
    std::list<std::string> GetProvidedReportFiles() const;
  };
}

#endif
