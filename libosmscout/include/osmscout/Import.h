#ifndef OSMSCOUT_IMPORT_H
#define OSMSCOUT_IMPORT_H

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

#include <osmscout/Progress.h>
#include <osmscout/TypeConfig.h>

namespace osmscout {

  /**
    Collects all parameter that have influence on the import.

    TODO:
    * Add variable defining the output directory (and make all import modules
      respect this parameter).
    */
  class OSMSCOUT_API ImportParameter
  {
  private:
    std::string mapfile;               //! Name of the file containing the map (either *.osm or *.osm.pbf)
    std::string typefile;              //! Name and path ff type definition file (map.ost.xml)
    std::string destinationDirectory;  //! Name of the destination directory
    size_t      startStep;             //! Starting step for import
    size_t      endStep;               //! End step for mport
    size_t      numericIndexPageSize;  //! Size of an numeric index page in bytes
    size_t      nodesLoadSize;         //! Maximum number of nodes loaded into memory in one go
    size_t      nodeIndexIntervalSize; //! The size of the index interval of the noduse index
    size_t      nodeDataCacheSize;     //! Size of the node data cache
    size_t      nodeIndexCacheSize;    //! Size of the node index cache
    size_t      waysLoadSize;          //! Maximum number of ways loaded into memory in one go
    size_t      wayDataCacheSize;      //! Size of the way data cache
    size_t      wayIndexCacheSize;     //! Size of the way index cache
    size_t      areaAreaIndexMaxMag;   //! Maximum depth of the index generated
    size_t      areaAreaRelIndexMaxMag;//! Maximum depth of the index generated
    size_t      areaWayIndexMaxMag;    //! Maximum depth of the index generated
    size_t      areaWayRelIndexMaxMag; //! Maximum depth of the index generated
    size_t      waterIndexMaxMag;      //! Maximum depth of the index generated

  public:
    ImportParameter();

    std::string GetMapfile() const;
    std::string GetTypefile() const;
    std::string GetDestinationDirectory() const;

    size_t GetStartStep() const;
    size_t GetEndStep() const;

    size_t GetNumericIndexPageSize() const;
    size_t GetNodesLoadSize() const;
    size_t GetNodeIndexIntervalSize() const;
    size_t GetNodeDataCacheSize() const;
    size_t GetNodeIndexCacheSize() const;
    size_t GetWaysLoadSize() const;
    size_t GetWayDataCacheSize() const;
    size_t GetWayIndexCacheSize() const;
    size_t GetAreaAreaIndexMaxMag() const;
    size_t GetAreaAreaRelIndexMaxMag() const;
    size_t GetAreaWayIndexMaxMag() const;
    size_t GetAreaWayRelIndexMaxMag() const;
    size_t GetWaterIndexMaxMag() const;

    void SetMapfile(const std::string& mapfile);
    void SetTypefile(const std::string& typefile);
    void SetDestinationDirectory(const std::string& destinationDirectory);

    void SetStartStep(size_t startStep);
    void SetSteps(size_t startStep, size_t endStep);

    void SetNumericIndexPageSize(size_t numericIndexPageSize);
    void SetNodesLoadSize(size_t nodesLoadSize);
    void SetNodeIndexIntervalSize(size_t nodeIndexIntervalSize);
    void SetNodeDataCacheSize(size_t nodeDataCacheSize);
    void SetNodeIndexCacheSize(size_t nodeIndexCacheSize);
    void SetWaysLoadSize(size_t waysLoadSize);
    void SetWayDataCacheSize(size_t wayDataCacheSize);
    void SetWayIndexCacheSize(size_t wayIndexCacheSize);
  };

  /**
    A single import module representing a single import step.

    An import consists of a number of sequentially executed steps. A step normally
    works on one object type and generates one output file (though this is just
    an suggestion). Such a step is realized by a ImportModule.
    */
  class OSMSCOUT_API ImportModule
  {
  public:
    virtual std::string GetDescription() const = 0;
    virtual bool Import(const ImportParameter& parameter,
                        Progress& progress,
                        const TypeConfig& typeConfig) = 0;
  };

  /**
    Does the import based on the given parameters. Feedback about the import progress
    is given by the indivudal import modules calling the Progress instance as appropriate.
    */
  extern OSMSCOUT_API bool Import(const ImportParameter& parameter,
                                  Progress& progress);
}

#endif
