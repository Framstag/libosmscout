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

#include <string>

#include <osmscout/private/ImportImportExport.h>

#include <osmscout/TypeConfig.h>

#include <osmscout/util/Progress.h>

#include <osmscout/util/Transformation.h>

namespace osmscout {

  /**
    Collects all parameter that have influence on the import.

    TODO:
    * Add variable defining the output directory (and make all import modules
      respect this parameter).
    */
  class OSMSCOUT_IMPORT_API ImportParameter
  {
  private:
    std::string                  mapfile;                  //! Name of the file containing the map (either *.osm or *.osm.pbf)
    std::string                  typefile;                 //! Name and path ff type definition file (map.ost.xml)
    std::string                  destinationDirectory;     //! Name of the destination directory
    size_t                       startStep;                //! Starting step for import
    size_t                       endStep;                  //! End step for import

    bool                         renumberIds;              //! Renumber all Ids
    size_t                       renumberBlockSize;        //! Number of entries loaded in one renumber iterations
    size_t                       renumberMag;              //! Zoom level for individual renumber sorting cells

    size_t                       numericIndexPageSize;     //! Size of an numeric index page in bytes

    bool                         rawNodeIndexMemoryMaped;  //! Use memory mapping for raw node index file access
    bool                         rawNodeDataMemoryMaped;   //! Use memory mapping for raw node data file access
    size_t                       rawNodeDataCacheSize;     //! Size of the raw node data cache
    size_t                       rawNodeIndexCacheSize;    //! Size of the raw node index cache

    bool                         rawWayIndexMemoryMaped;   //! Use memory mapping for raw way index file access
    bool                         rawWayDataMemoryMaped;    //! Use memory mapping for raw way data file access
    size_t                       rawWayDataCacheSize;      //! Size of the raw way data cache
    size_t                       rawWayIndexCacheSize;     //! Size of the raw way index cache
    size_t                       rawWayBlockSize;          //! Number of ways loaded during import until nodes get resolved

    bool                         wayIndexMemoryMaped;      //! Use memory mapping for way index file access
    bool                         wayDataMemoryMaped;       //! Use memory mapping for way data file access
    size_t                       wayDataCacheSize;         //! Size of the way data cache
    size_t                       wayIndexCacheSize;        //! Size of the way index cache
    size_t                       waysLoadSize;             //! Maximum number of ways loaded into memory in one go

    size_t                       areaAreaIndexMaxMag;      //! Maximum depth of the index generated

    size_t                       areaWayMinMag;            //! Minimum magnification of index for individual type
    size_t                       areaWayIndexCellSizeAverage; //! Average entries per index cell
    size_t                       areaWayIndexCellSizeMax;  //! Maximum number of entries  per index cell

    size_t                       areaNodeMinMag;           //! Minimum magnification of index for individual type
    size_t                       areaNodeIndexCellSizeAverage; //! Average entries per index cell
    size_t                       areaNodeIndexCellSizeMax; //! Maximum number of entries  per index cell

    size_t                       waterIndexMinMag;         //! Minimum level of the generated water index
    size_t                       waterIndexMaxMag;         //! Maximum level of the generated water index

    size_t                       optimizationMaxMag;       //! Maximum magnification for optimization
    size_t                       optimizationMinMag;       //! Minimum magnification of index for individual type
    size_t                       optimizationCellSizeAverage; //! Average entries per index cell
    size_t                       optimizationCellSizeMax;  //! Maximum number of entries  per index cell
    TransPolygon::OptimizeMethod optimizationWayMethod; //! what method to use to optimize ways

    size_t                       routeNodeBlockSize;       //! Number of route nodes loaded during import until ways get resolved

    bool                         assumeLand;               //! During sea/land detection,we either trust coastlines only or make some
                                                           //! assumptions which tiles are sea and which are land.

  public:
    ImportParameter();

    std::string GetMapfile() const;
    std::string GetTypefile() const;
    std::string GetDestinationDirectory() const;

    size_t GetStartStep() const;
    size_t GetEndStep() const;

    bool GetRenumberIds() const;
    size_t GetRenumberBlockSize() const;
    size_t GetRenumberMag() const;

    size_t GetNumericIndexPageSize() const;

    bool GetRawNodeIndexMemoryMaped() const;
    bool GetRawNodeDataMemoryMaped() const;
    size_t GetRawNodeDataCacheSize() const;
    size_t GetRawNodeIndexCacheSize() const;

    bool GetRawWayIndexMemoryMaped() const;
    bool GetRawWayDataMemoryMaped() const;
    size_t GetRawWayDataCacheSize() const;
    size_t GetRawWayIndexCacheSize() const;
    size_t GetRawWayBlockSize() const;


    bool GetWayIndexMemoryMaped() const;
    bool GetWayDataMemoryMaped() const;
    size_t GetWayDataCacheSize() const;
    size_t GetWayIndexCacheSize() const;

    size_t GetAreaNodeMinMag() const;
    size_t GetAreaNodeIndexCellSizeAverage() const;
    size_t GetAreaNodeIndexCellSizeMax() const;

    size_t GetAreaWayMinMag() const;
    size_t GetAreaWayIndexCellSizeAverage() const;
    size_t GetAreaWayIndexCellSizeMax() const;

    size_t GetAreaAreaIndexMaxMag() const;

    size_t GetWaterIndexMinMag() const;
    size_t GetWaterIndexMaxMag() const;

    size_t GetOptimizationMaxMag() const;
    size_t GetOptimizationMinMag() const;
    size_t GetOptimizationCellSizeAverage() const;
    size_t GetOptimizationCellSizeMax() const;
    TransPolygon::OptimizeMethod GetOptimizationWayMethod() const;

    size_t GetRouteNodeBlockSize() const;

    bool GetAssumeLand() const;

    void SetMapfile(const std::string& mapfile);
    void SetTypefile(const std::string& typefile);
    void SetDestinationDirectory(const std::string& destinationDirectory);

    void SetStartStep(size_t startStep);
    void SetSteps(size_t startStep, size_t endStep);

    void SetRenumberIds(bool renumberIds);
    void SetRenumberBlockSize(size_t renumberBlockSize);
    void SetRenumberMag(size_t renumberMag);

    void SetNumericIndexPageSize(size_t numericIndexPageSize);

    void SetRawNodeIndexMemoryMaped(bool memoryMaped);
    void SetRawNodeDataMemoryMaped(bool memoryMaped);
    void SetRawNodeDataCacheSize(size_t nodeDataCacheSize);
    void SetRawNodeIndexCacheSize(size_t nodeIndexCacheSize);

    void SetRawWayIndexMemoryMaped(bool memoryMaped);
    void SetRawWayDataMemoryMaped(bool memoryMaped);
    void SetRawWayDataCacheSize(size_t wayDataCacheSize);
    void SetRawWayIndexCacheSize(size_t wayIndexCacheSize);
    void SetRawWayBlockSize(size_t blockSize);


    void SetWayIndexMemoryMaped(bool memoryMaped);
    void SetWayDataMemoryMaped(bool memoryMaped);
    void SetWayDataCacheSize(size_t wayDataCacheSize);
    void SetWayIndexCacheSize(size_t wayIndexCacheSize);
    void SetWaysLoadSize(size_t waysLoadSize);

    void SetAreaAreaIndexMaxMag(size_t areaAreaIndexMaxMag);

    void SetAreaWayMinMag(size_t areaWayMinMag);
    void SetAreaWayIndexCellSizeAverage(size_t areaWayIndexCellSizeAverage);
    void SetAreaWayIndexCellSizeMax(size_t areaWayIndexCellSizeMax);

    void SetAreaNodeMinMag(size_t areaNodeMinMag);
    void SetAreaNodeIndexCellSizeAverage(size_t areaNodeIndexCellSizeAverage);
    void SetAreaNodeIndexCellSizeMax(size_t areaNodeIndexCellSizeMax);

    void SetWaterIndexMinMag(size_t waterIndexMinMag);
    void SetWaterIndexMaxMag(size_t waterIndexMaxMag);

    void SetOptimizationMaxMag(size_t optimizationMaxMag);
    void SetOptimizationMinMag(size_t optimizationMinMag);
    void SetOptimizationCellSizeAverage(size_t optimizationCellSizeAverage);
    void SetOptimizationCellSizeMax(size_t optimizationCellSizeMax);
    void SetOptimizationWayMethod(TransPolygon::OptimizeMethod optimizationWayMethod);

    void SetRouteNodeBlockSize(size_t blockSize);

    void SetAssumeLand(bool assumeLand);
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
    virtual std::string GetDescription() const = 0;
    virtual bool Import(const ImportParameter& parameter,
                        Progress& progress,
                        const TypeConfig& typeConfig) = 0;
  };

  /**
    Does the import based on the given parameters. Feedback about the import progress
    is given by the indivudal import modules calling the Progress instance as appropriate.
    */
  extern OSMSCOUT_IMPORT_API bool Import(const ImportParameter& parameter,
                                         Progress& progress);
}

#endif
