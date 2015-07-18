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

#include <unordered_map>
#include <unordered_set>

#include <osmscout/ImportFeatures.h>

#include <osmscout/import/Import.h>

#include <osmscout/Area.h>
#include <osmscout/Way.h>

#include <osmscout/util/NodeUseMap.h>

namespace osmscout {

  /**
   * Merges areas of the same type (and where the type is flag as mergable), which
   * "touch" each other and share the same nodes (same node id).
   */
  class MergeAreasGenerator : public ImportModule
  {
  private:
    size_t GetFirstOuterRingWithId(const Area& area,
                                   Id id) const;

    void EraseAreaInCache(const NodeUseMap& nodeUseMap,
                          const AreaRef& area,
                          std::unordered_map<Id,std::set<AreaRef> >& idAreaMap);
    void EraseAreaInCache(Id currentId,
                          const NodeUseMap& nodeUseMap,
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
                         std::vector<NodeUseMap>& nodeUseMap);

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
     * @param types
     *   Set of types to load
     * @param nodeUseMap
     *   Array of NodeUseMaps
     * @param scanner
     *   File Scanner for the area data
     * @param areas
     *   actual area data returned
     * @return
     *   true, if everything is fine, else false
     */
    bool GetAreas(const ImportParameter& parameter,
                  Progress& progress,
                  const TypeConfig& typeConfig,
                  TypeInfoSet& types,
                  const std::vector<NodeUseMap>& nodeUseMap,
                  FileScanner& scanner,
                  std::vector<std::list<AreaRef> >& areas);

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
    void IndexAreasByNodeIds(const NodeUseMap& nodeUseMap,
                             const std::list<AreaRef>& areas,
                             std::unordered_map<Id,std::set<AreaRef> >& idAreaMap);

    bool TryMerge(const NodeUseMap& nodeUseMap,
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
                    const NodeUseMap& nodeUseMap,
                    std::list<AreaRef>& areas,
                    std::list<AreaRef>& merges,
                    std::unordered_set<FileOffset>& mergedAway);

  public:
    std::string GetDescription() const;
    bool Import(const TypeConfigRef& typeConfig,
                const ImportParameter& parameter,
                Progress& progress);
  };
}

#endif
