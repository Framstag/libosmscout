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

#include <osmscout/poi/POIService.h>

#include <algorithm>
#include <future>

#include <osmscout/log/Logger.h>

namespace osmscout {

  POIService::POIService(const DatabaseRef& database)
  : database(database)
  {
    // no code
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
   * @exception
   *    OSMScoutException in case of errors
   */
  void POIService::GetPOIsInArea(const GeoBox& boundingBox,
                                 const TypeInfoSet& nodeTypes,
                                 std::vector<NodeRef>& nodes,
                                 const TypeInfoSet& wayTypes,
                                 std::vector<WayRef>& ways,
                                 const TypeInfoSet& areaTypes,
                                 std::vector<AreaRef>& areas) const
  {
    nodes.clear();
    areas.clear();
    ways.clear();

    auto nodeResult=std::async(std::launch::async,
                               &Database::LoadNodesInArea,database,
                               std::ref(nodeTypes),
                               std::ref(boundingBox));

    auto wayResult=std::async(std::launch::async,
                              &Database::LoadWaysInArea,database,
                              std::ref(wayTypes),
                              std::ref(boundingBox));

    auto areaResult=std::async(std::launch::async,
                               &Database::LoadAreasInArea,database,
                               std::ref(areaTypes),
                               std::ref(boundingBox));

    auto nodeResultData=nodeResult.get();

    nodes.reserve(nodeResultData.GetNodeResults().size());

    for (const auto& entry : nodeResultData.GetNodeResults()) {
      nodes.push_back(entry.GetNode());
    }

    auto wayResultData=wayResult.get();

    ways.reserve(wayResultData.GetWayResults().size());

    for (const auto& entry : wayResultData.GetWayResults()) {
      ways.push_back(entry.GetWay());
    }

    auto areaResultData=areaResult.get();

    areas.reserve(areaResultData.GetAreaResults().size());

    for (const auto& entry : areaResultData.GetAreaResults()) {
      areas.push_back(entry.GetArea());
    }
  }

  /**
   * Returns all objects with the given max distance from the given location that have one of the given types.
   *
   * @param location
   *    Center of the radius
   * @param maxDistance
   *    Maximum radius form the location to search in
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
   * @exception
   *    OSMScoutException in case of errors
   */
  void POIService::GetPOIsInRadius(const GeoCoord& location,
                                   const Distance& maxDistance,
                                   const TypeInfoSet& nodeTypes,
                                   std::vector<NodeRef>& nodes,
                                   const TypeInfoSet& wayTypes,
                                   std::vector<WayRef>& ways,
                                   const TypeInfoSet& areaTypes,
                                   std::vector<AreaRef>& areas) const
  {
    nodes.clear();
    areas.clear();
    ways.clear();

    auto nodeResult=std::async(std::launch::async,
                               &Database::LoadNodesInRadius,database,
                               std::ref(location),
                               std::ref(nodeTypes),
                               std::ref(maxDistance));

    auto wayResult=std::async(std::launch::async,
                              &Database::LoadWaysInRadius,database,
                              std::ref(location),
                              std::ref(wayTypes),
                              std::ref(maxDistance));

    auto areaResult=std::async(std::launch::async,
                               &Database::LoadAreasInRadius,database,
                               std::ref(location),
                               std::ref(areaTypes),
                               std::ref(maxDistance));

    auto nodeResultData=nodeResult.get();

    nodes.reserve(nodeResultData.GetNodeResults().size());

    for (const auto& entry : nodeResultData.GetNodeResults()) {
      nodes.push_back(entry.GetNode());
    }

    auto wayResultData=wayResult.get();

    ways.reserve(wayResultData.GetWayResults().size());

    for (const auto& entry : wayResultData.GetWayResults()) {
      ways.push_back(entry.GetWay());
    }

    auto areaResultData=areaResult.get();

    areas.reserve(areaResultData.GetAreaResults().size());

    for (const auto& entry : areaResultData.GetAreaResults()) {
      areas.push_back(entry.GetArea());
    }
  }
}
