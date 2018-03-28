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

#include <osmscout/POIService.h>

#include <algorithm>

#include <osmscout/util/Logger.h>

#if _OPENMP
#include <omp.h>
#endif

namespace osmscout {

  POIService::POIService(const DatabaseRef& database)
  : database(database)
  {
    // no code
  }

  POIService::~POIService()
  {
    // no code
  }

  /**
   * Return all nodes in the given bounding box with the given type
   * @param boundingBox
   *    Bounding box, objects must be in
   * @param types
   *    The resulting nodes must be of one of these types
   * @param nodes
   *    Result of the query, in case the query succeeded. In case of errors
   *    the result is empty.
   * @return
   *    True, if success, else false
   */
  bool POIService::GetNodesInArea(const GeoBox& boundingBox,
                                  const TypeInfoSet& types,
                                  std::vector<NodeRef>& nodes) const
  {
    AreaNodeIndexRef areaNodeIndex=database->GetAreaNodeIndex();
    NodeDataFileRef  nodeDataFile=database->GetNodeDataFile();
    TypeInfoSet      loadedTypes;

    nodes.clear();

    if (!areaNodeIndex ||
        !nodeDataFile) {
      return false;
    }

    std::vector<FileOffset> offsets;

    if (!areaNodeIndex->GetOffsets(boundingBox,
                                   types,
                                   offsets,
                                   loadedTypes)) {
      log.Error() << "Error getting nodes from area node index!";

      return false;
    }

    std::sort(offsets.begin(),
              offsets.end());

    if (!nodeDataFile->GetByOffset(offsets.begin(),
                                   offsets.end(),
                                   offsets.size(),
                                   nodes)) {
      log.Error() << "Error reading nodes in area!";

      return false;
    }

    return true;
  }

  /**
   * Return all areas in the given bounding box with the given type
   * @param boundingBox
   *    Bounding box, objects must be in
   * @param types
   *    The resulting areas must be of one of these types
   * @param nodes
   *    Result of the query, in case the query succeeded. In case of errors
   *    the result is empty.
   * @return
   *    True, if success, else false
   */
  bool POIService::GetAreasInArea(const GeoBox& boundingBox,
                                  const TypeInfoSet& types,
                                  std::vector<AreaRef>& areas) const
  {
    AreaAreaIndexRef areaAreaIndex=database->GetAreaAreaIndex();
    AreaDataFileRef  areaDataFile=database->GetAreaDataFile();
    TypeInfoSet      loadedTypes;

    areas.clear();

    if (!areaAreaIndex ||
        !areaDataFile) {
      return false;
    }

    std::vector<DataBlockSpan> spans;

    if (!areaAreaIndex->GetAreasInArea(*database->GetTypeConfig(),
                                       boundingBox,
                                       std::numeric_limits<size_t>::max(),
                                       types,
                                       spans,
                                       loadedTypes)) {
      log.Error() << "Error getting ways and relations from area index!";

      return false;
    }

    if (!spans.empty()) {
      std::sort(spans.begin(),spans.end());

      if (!areaDataFile->GetByBlockSpans(spans.begin(),
                                         spans.end(),
                                         areas)) {
        log.Error() << "Error reading areas in area!";

        return false;
      }
    }

    return true;
  }

  /**
   * Return all ways in the given bounding box with the given type
   * @param boundingBox
   *    Bounding box, objects must be in
   * @param types
   *    The resulting ways must be of one of these types
   * @param nodes
   *    Result of the query, in case the query succeeded. In case of errors
   *    the result is empty.
   * @return
   *    True, if success, else false
   */
  bool POIService::GetWaysInArea(const GeoBox& boundingBox,
                                 const TypeInfoSet& types,
                                 std::vector<WayRef>& ways) const
  {
    AreaWayIndexRef areaWayIndex=database->GetAreaWayIndex();
    WayDataFileRef  wayDataFile=database->GetWayDataFile();

    ways.clear();

    if (!areaWayIndex ||
        !wayDataFile) {
      return false;
    }

    std::vector<FileOffset> offsets;
    TypeInfoSet             loadedWayTypes;


    if (!areaWayIndex->GetOffsets(boundingBox,
                                  types,
                                  offsets,
                                  loadedWayTypes)) {
      log.Error() << "Error getting ways and relations from area way index!";

      return false;
    }

    std::sort(offsets.begin(),offsets.end());

    if (!wayDataFile->GetByOffset(offsets.begin(),
                                  offsets.end(),
                                  offsets.size(),
                                  ways)) {
      log.Error() << "Error reading ways in area!";

      return true;
    }

    return true;
  }

  /**
   * Returns all objects in the given boundary that have one of the given types.
   *
   * @param boundingBox
   *    Bounding box, objects must be in
   * @param types
   *    The resulting nodes, ways and areas must be of one of these types
   * @param nodes
   *    Result of the query, in case the query succeeded. In case of errors
   *    the result is empty.
   * @param ways
   *    Result of the query, in case the query succeeded. In case of errors
   *    the result is empty.
   * @param areas
   *    Result of the query, in case the query succeeded. In case of errors
   *    the result is empty.
   * @return
   *    True, if success, else false
   */
  bool POIService::GetPOIsInArea(const GeoBox& boundingBox,
                                 const TypeInfoSet& nodeTypes,
                                 std::vector<NodeRef>& nodes,
                                 const TypeInfoSet& wayTypes,
                                 std::vector<WayRef>& ways,
                                 const TypeInfoSet& areaTypes,
                                 std::vector<AreaRef>& areas) const
  {
    bool nodesSuccess;
    bool waysSuccess;
    bool areasSuccess;

#pragma omp parallel
#pragma omp sections
    {
#pragma omp section
      nodesSuccess=GetNodesInArea(boundingBox,
                                  nodeTypes,
                                  nodes);

#pragma omp section

      areasSuccess=GetAreasInArea(boundingBox,
                                  areaTypes,
                                  areas);

#pragma omp section
      waysSuccess=GetWaysInArea(boundingBox,
                                wayTypes,
                                ways);
    }

    if (!nodesSuccess ||
        !waysSuccess ||
        !areasSuccess) {
      nodes.clear();
      areas.clear();
      ways.clear();

      return false;
    }

    return true;
  }
}
