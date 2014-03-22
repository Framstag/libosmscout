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

  bool POIService::GetNodesInArea(double lonMin, double latMin,
                                  double lonMax, double latMax,
                                  const TypeSet& types,
                                  std::vector<NodeRef>& nodes) const
  {
    AreaNodeIndexRef areaNodeIndex=database->GetAreaNodeIndex();
    NodeDataFileRef  nodeDataFile=database->GetNodeDataFile();

    nodes.clear();

    if (areaNodeIndex.Invalid() ||
        nodeDataFile.Invalid()) {
      return false;
    }

    std::vector<FileOffset> nodeOffsets;

    if (!areaNodeIndex->GetOffsets(lonMin,
                                   latMin,
                                   lonMax,
                                   latMax,
                                   types,
                                   std::numeric_limits<size_t>::max(),
                                   nodeOffsets)) {
      std::cout << "Error getting nodes from area node index!" << std::endl;
      return false;
    }

    std::sort(nodeOffsets.begin(),nodeOffsets.end());

    if (!nodeDataFile->GetByOffset(nodeOffsets,
                                   nodes)) {
      std::cout << "Error reading nodes in area!" << std::endl;

      return false;
    }

    return true;
  }

  bool POIService::GetAreasInArea(double lonMin, double latMin,
                                  double lonMax, double latMax,
                                  const TypeSet& types,
                                  std::vector<AreaRef>& areas) const
  {
    AreaAreaIndexRef areaAreaIndex=database->GetAreaAreaIndex();
    AreaDataFileRef  areaDataFile=database->GetAreaDataFile();

    areas.clear();

    if (areaAreaIndex.Invalid() ||
        areaDataFile.Invalid()) {
      return false;
    }

    std::vector<FileOffset> wayAreaOffsets;

    if (!areaAreaIndex->GetOffsets(lonMin,
                                   latMin,
                                   lonMax,
                                   latMax,
                                   std::numeric_limits<size_t>::max(),
                                   types,
                                   std::numeric_limits<size_t>::max(),
                                   wayAreaOffsets)) {
      std::cout << "Error getting ways and relations from area index!" << std::endl;

      return false;
    }

    std::sort(wayAreaOffsets.begin(),wayAreaOffsets.end());

    if (!areaDataFile->GetByOffset(wayAreaOffsets,
                                   areas)) {
      std::cout << "Error reading areas in area!" << std::endl;

      return false;
    }

    return true;
  }

  bool POIService::GetWaysInArea(double lonMin, double latMin,
                                 double lonMax, double latMax,
                                 const TypeSet& types,
                                 std::vector<WayRef>& ways) const
  {
    AreaWayIndexRef  areaWayIndex=database->GetAreaWayIndex();
    WayDataFileRef   wayDataFile=database->GetWayDataFile();

    ways.clear();

    if (areaWayIndex.Invalid() ||
        wayDataFile.Invalid()) {
      return false;
    }

    std::vector<TypeSet>    wayTypes;
    std::vector<FileOffset> wayWayOffsets;


    wayTypes.push_back(types);


    if (!areaWayIndex->GetOffsets(lonMin,
                                  latMin,
                                  lonMax,
                                  latMax,
                                  wayTypes,
                                  std::numeric_limits<size_t>::max(),
                                  wayWayOffsets)) {
      std::cout << "Error getting ways and relations from area way index!" << std::endl;

      return false;
    }

    std::sort(wayWayOffsets.begin(),wayWayOffsets.end());

    if (!wayDataFile->GetByOffset(wayWayOffsets,
                                  ways)) {
      std::cout << "Error reading ways in area!" << std::endl;

      return true;
    }

    return true;
  }

  /**
   * Returns all objects in the given boundary that have one of the given types.
   * @param lonMin
   *    Boundary coordinate
   * @param latMin
   *    Boundary coordinate
   * @param lonMax
   *    Boundary coordinate
   * @param latMax
   *    Boundary coordinate
   * @param types
   *    Requested object types
   * @param nodes
   *    Returns list of nodes
   * @param ways
   *    Returns list of ways
   * @param areas
   *    Returns list of areas
   * @return
   *    True, if there was no error
   */
  bool POIService::GetPOIsInArea(double lonMin, double latMin,
                                 double lonMax, double latMax,
                                 const TypeSet& types,
                                 std::vector<NodeRef>& nodes,
                                 std::vector<WayRef>& ways,
                                 std::vector<AreaRef>& areas) const
  {
    bool nodesSuccess=true;
    bool waysSuccess=true;
    bool areasSuccess=true;

#pragma omp parallel
#pragma omp sections
    {
#pragma omp section
      nodesSuccess=GetNodesInArea(lonMin,
                                  latMin,
                                  lonMax,
                                  latMax,
                                  types,
                                  nodes);

#pragma omp section

      areasSuccess=GetAreasInArea(lonMin,
                                  latMin,
                                  lonMax,
                                  latMax,
                                  types,
                                  areas);

#pragma omp section
      waysSuccess=GetWaysInArea(lonMin,
                                latMin,
                                lonMax,
                                latMax,
                                types,
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
