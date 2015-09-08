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
                                  const TypeSet& types,
                                  std::vector<NodeRef>& nodes) const
  {
    AreaNodeIndexRef areaNodeIndex=database->GetAreaNodeIndex();
    NodeDataFileRef  nodeDataFile=database->GetNodeDataFile();

    nodes.clear();

    if (!areaNodeIndex ||
        !nodeDataFile) {
      return false;
    }

    std::vector<FileOffset> nodeOffsets;

    if (!areaNodeIndex->GetOffsets(boundingBox.GetMinLon(),
                                   boundingBox.GetMinLat(),
                                   boundingBox.GetMaxLon(),
                                   boundingBox.GetMaxLat(),
                                   types,
                                   std::numeric_limits<size_t>::max(),
                                   nodeOffsets)) {
      log.Error() << "Error getting nodes from area node index!";
      return false;
    }

    std::sort(nodeOffsets.begin(),
              nodeOffsets.end());

    if (!nodeDataFile->GetByOffset(nodeOffsets,
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
                                  const TypeSet& types,
                                  std::vector<AreaRef>& areas) const
  {
    AreaAreaIndexRef areaAreaIndex=database->GetAreaAreaIndex();
    AreaDataFileRef  areaDataFile=database->GetAreaDataFile();

    areas.clear();

    if (!areaAreaIndex ||
        !areaDataFile) {
      return false;
    }

    std::vector<DataBlockSpan> spans;

    if (!areaAreaIndex->GetAreasInArea(database->GetTypeConfig(),
                                       boundingBox,
                                       std::numeric_limits<size_t>::max(),
                                       types,
                                       std::numeric_limits<size_t>::max(),
                                       spans)) {
      log.Error() << "Error getting ways and relations from area index!";

      return false;
    }

    if (!spans.empty()) {
      std::sort(spans.begin(),spans.end());

      if (!areaDataFile->GetByBlockSpans(spans,
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
                                 const TypeSet& types,
                                 std::vector<WayRef>& ways) const
  {
    AreaWayIndexRef  areaWayIndex=database->GetAreaWayIndex();
    WayDataFileRef   wayDataFile=database->GetWayDataFile();

    ways.clear();

    if (!areaWayIndex ||
        !wayDataFile) {
      return false;
    }

    std::vector<TypeSet>    wayTypes;
    std::vector<FileOffset> wayWayOffsets;


    wayTypes.push_back(types);


    if (!areaWayIndex->GetOffsets(boundingBox.GetMinLon(),
                                  boundingBox.GetMinLat(),
                                  boundingBox.GetMaxLon(),
                                  boundingBox.GetMaxLat(),
                                  wayTypes,
                                  std::numeric_limits<size_t>::max(),
                                  wayWayOffsets)) {
      log.Error() << "Error getting ways and relations from area way index!";

      return false;
    }

    std::sort(wayWayOffsets.begin(),wayWayOffsets.end());

    if (!wayDataFile->GetByOffset(wayWayOffsets,
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
                                 const TypeSet& nodeTypes,
                                 std::vector<NodeRef>& nodes,
                                 const TypeSet& wayTypes,
                                 std::vector<WayRef>& ways,
                                 const TypeSet& areaTypes,
                                 std::vector<AreaRef>& areas) const
  {
    bool nodesSuccess=true;
    bool waysSuccess=true;
    bool areasSuccess=true;

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
