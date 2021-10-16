#ifndef OSMSCOUT_IMPORT_GENMERGEAREAS_H
#define OSMSCOUT_IMPORT_GENMERGEAREAS_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2013  Tim Teulings

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
#include <unordered_map>
#include <unordered_set>

#include <osmscout/TypeInfoSet.h>

#include <osmscout/Area.h>
#include <osmscout/Way.h>

#include <osmscoutimport/ImportFeatures.h>
#include <osmscoutimport/Import.h>

#include <osmscout/util/NodeUseMap.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

  /**
   * Input for a merge job for one area type
   */
  struct AreaMergeJob
  {
    TypeInfoRef            type;
    std::unordered_set<Id> nodeUseSet;
    size_t                 areaCount;  //!< Number of areas of this type
    std::list<AreaRef>     areas;      //!< List of areas that are candidates for merging
  };

  /**
   * Result of the merge job for one area type
   */
  struct AreaMergeResult
  {
    TypeInfoRef                    type;
    std::list<AreaRef>             merges;     //!< List of areas that got merged
    std::unordered_set<FileOffset> mergedAway; //!< List of file offsets of areas, that were merged into another area
  };

  /**
   * Merges areas of the same type (and where the type is flag as mergable), which
   * "touch" each other and share the same nodes (same node id).
   */
  class MergeAreasGenerator CLASS_FINAL : public ImportModule
  {
  public:
    static const char* const AREAS2_TMP;

  private:
    /**
     * Load all areas which have at least one of the "used at least twice"
     * nodes in its nodeUseMap for all given types. If the number of areas
     * increases over a certain limit, data of some types is dropped
     * (and reloaded in the next iteration)
     *
     * @param parameter
     *   ImportParameter
     * @param progress
     *   Progress
     * @param typeConfig
     *   TypeConfiguration
     * @param candidateTypes
     *   Set of types which should be loaded
     * @param loadedTypes
     *   Set of types which have been loaded
     * @param scanner
     *   File Scanner for reading the area data
     * @param mergeJobs
     * data structure for merging areas of one type, here passed to return
     * all merge candidates in 'areas'
     */
    void GetAreas(const ImportParameter& parameter,
                  Progress& progress,
                  const TypeConfig& typeConfig,
                  const TypeInfoSet& candidateTypes,
                  TypeInfoSet& loadedTypes,
                  FileScanner& scanner,
                  std::vector<AreaMergeJob>& mergeJobs);

    void WriteMergeResult(Progress& progress,
                          const TypeConfig& typeConfig,
                          FileScanner& scanner,
                          FileWriter& writer,
                          const TypeInfoSet& loadedTypes,
                          const std::vector<AreaMergeResult>& mergeJob,
                          uint32_t& areasWritten);

  public:
    void GetDescription(const ImportParameter& parameter,
                        ImportModuleDescription& description) const override;

    bool Import(const TypeConfigRef& typeConfig,
                const ImportParameter& parameter,
                Progress& progress) override;
  };
}

#endif
