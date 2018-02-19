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
#include <unordered_map>
#include <unordered_set>

#include <osmscout/ImportFeatures.h>

#include <osmscout/import/Import.h>

#include <osmscout/Area.h>
#include <osmscout/Way.h>

#include <osmscout/util/NodeUseMap.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

  /**
   * Merges areas of the same type (and where the type is flag as mergable), which
   * "touch" each other and share the same nodes (same node id).
   */
  class MergeAreasGenerator CLASS_FINAL : public ImportModule
  {
  public:
    static const char* AREAS2_TMP;

  private:
    /**
     * Data structure holding all information for merging
     * areas of one type
     */
    struct AreaMergeData
    {
      size_t                         areaCount;  //!< Number of areas of this type
      std::list<AreaRef>             areas;      //!< List of areas that are candidates for merging
      std::list<AreaRef>             merges;     //!< List of areas that got merged
      std::unordered_set<FileOffset> mergedAway; //!< List of file offsets of areas, that were merged into another area
    };

  private:
    /**
     * Return the index of the first outer ring that contains the given node id.
     */
    size_t GetFirstOuterRingWithId(const Area& area,
                                   Id id) const;

    void EraseAreaInCache(const std::unordered_set<Id>& nodeUseMap,
                          const AreaRef& area,
                          std::unordered_map<Id,std::set<AreaRef> >& idAreaMap);

    /**
     * Scan all areas for node ids that occur in more than one area. Only areas with
     * nodes that are used in other areas, too, are candidates for merging.
     *
     * We only take nodes in the outer rings into account.
     *
     * @param parameter
     *   ImportParameter
     * @param progress
     *   Progress
     * @param typeConfig
     *   TypeConfiguration
     * @param mergeTypes
     *   Set of types for which we do area merging
     * @param nodeUseMap
     *   special structure to track multiple use of the same node id. We have one map for each type.
     * @return
     *   if everything is fine, else false
     */
    bool ScanAreaNodeIds(Progress& progress,
                         const TypeConfig& typeConfig,
                         FileScanner& scanner,
                         const TypeInfoSet& mergeTypes,
                         std::unordered_set<Id>& nodeUseMap);

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
     * @param nodeUseMap
     *   Array of NodeUseMaps
     * @param scanner
     *   File Scanner for reading the area data
     * @param writer
     *   File writer for the writing area data which
     *   are of a type that is not marked as mergable
     * @param mergeJob
     * data structure for merging areas of one type, here passed to return
     * all merge candidates in 'areas'
     * @param areasWritten
     *   Number of areas directly written, because they
     *   are of a type that is not marked as mergable
     * @return
     *   true, if everything is fine, else false
     */
    bool GetAreas(const ImportParameter& parameter,
                  Progress& progress,
                  const TypeConfig& typeConfig,
                  const TypeInfoSet& candidateTypes,
                  TypeInfoSet& loadedTypes,
                  const std::unordered_set<Id>& nodeUseMap,
                  FileScanner& scanner,
                  FileWriter& writer,
                  std::vector<AreaMergeData>& mergeJob,
                  uint32_t& areasWritten);

    /**
     * Index areas by their "at least used twice" nodes
     * @param nodeUseMap
     *    Node use map
     * @param areas
     *    List of areas
     * @param idAreaMap
     *    Resulting index structure, mammping a node id to the
     *    list of areas that hold this node in on of their outer
     *    rings.
     */
    void IndexAreasByNodeIds(const std::unordered_set<Id>& nodeUseMap,
                             const std::list<AreaRef>& areas,
                             std::unordered_map<Id,std::set<AreaRef> >& idAreaMap);

    bool TryMerge(const std::unordered_set<Id>& nodeUseMap,
                  Area& area,
                  std::unordered_map<Id,std::set<AreaRef> >& idAreaMap,
                  std::set<Id>& finishedIds,
                  std::unordered_set<FileOffset>& mergedAway);

    /**
     * Merge area data of one type
     *
     * @param nodeUseMap
     * @param areas
     * @param merges
     * @param blacklist
     */
    void MergeAreas(Progress& progress,
                    const std::unordered_set<Id>& nodeUseMap,
                    AreaMergeData& job);

    bool WriteMergeResult(Progress& progress,
                          const TypeConfig& typeConfig,
                          FileScanner& scanner,
                          FileWriter& writer,
                          const TypeInfoSet& loadedTypes,
                          std::vector<AreaMergeData>& mergeJob,
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
