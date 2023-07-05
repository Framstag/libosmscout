/*
  This source is part of the libosmscout-map library
  Copyright (C) 2023  Tim Teulings

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

#include <osmscoutmap/MapPainterStatistics.h>

#include <osmscout/TypeInfoSet.h>

#include <osmscout/log/Logger.h>

namespace osmscout {

  std::list<MapPainterStatistics::DataStatistic> MapPainterStatistics::MapToSortedList(
    const std::unordered_map<TypeInfoRef,DataStatistic>& statistics)
  {
    std::list<DataStatistic> statisticList;

    for (const auto& [type, statistic] : statistics) {
      statisticList.push_back(statistic);
    }

    statisticList.sort([](const DataStatistic& a,
                          const DataStatistic& b)->bool {return a.objectCount>b.objectCount;});

    return statisticList;
  }

  void MapPainterStatistics::DumpMapPainterStatistics(const StyleConfig& styleConfig,
                                                      const Projection& projection,
                                                      const MapParameter& parameter,
                                                      const MapData& data)
  {
    if (parameter.IsDebugPerformance()) {
      log.Info()
        << "Data: "
        << data.nodes.size()
        << "+" << data.poiNodes.size()
        << " " << data.ways.size()
        << " " << data.areas.size();
    }

    if (parameter.GetWarningCoordCountLimit()==0 &&
        parameter.GetWarningObjectCountLimit()==0 &&
        !parameter.IsDebugData()) {
      return;
    }

    DumpDataStatistics(styleConfig,
                       projection,
                       parameter,
                       data);
  }

  void MapPainterStatistics::DumpDataStatistics(const std::list<DataStatistic>& statistics)
  {
    log.Info() << "Type|ObjectCount|NodeCount|WayCount|AreaCount|Nodes|Labels|Icons";
    for (const auto& entry : statistics) {
      log.Info() << entry.type->GetName() << " "
                 << entry.objectCount << " "
                 << entry.nodeCount << " " << entry.wayCount << " " << entry.areaCount << " "
                 << entry.coordCount << " "
                 << entry.labelCount << " "
                 << entry.iconCount;
    }
  }

  void MapPainterStatistics::DumpDataStatistics(const StyleConfig& styleConfig,
                                                const Projection& projection,
                                                const MapParameter& parameter,
                                                const MapData& data)
  {
    std::unordered_map<TypeInfoRef,DataStatistic> statistics;
    TypeInfoSet                                   types;

    // Now analyse the actual data

    for (const auto& node : data.nodes) {
      DataStatistic& entry=statistics[node->GetType()];

      CalculateStatistics(projection,
                          parameter,
                          styleConfig,
                          node,
                          entry);
    }

    for (const auto& node : data.poiNodes) {
      DataStatistic& entry=statistics[node->GetType()];

      CalculateStatistics(projection,
                          parameter,
                          styleConfig,
                          node,
                          entry);
    }

    for (const auto& way : data.ways) {
      DataStatistic& entry=statistics[way->GetType()];

      CalculateStatistics(projection,
                          parameter,
                          styleConfig,
                          way,
                          entry);
    }

    for (const auto& way : data.poiWays) {
      DataStatistic& entry=statistics[way->GetType()];

      CalculateStatistics(projection,
                          parameter,
                          styleConfig,
                          way,
                          entry);
    }

    for (const auto& area : data.areas) {
      DataStatistic& entry=statistics[area->GetType()];

      CalculateStatistics(projection,
                          parameter,
                          styleConfig,
                          area,
                          entry);
    }

    for (const auto& area : data.poiAreas) {
      DataStatistic& entry=statistics[area->GetType()];

      CalculateStatistics(projection,
                          parameter,
                          styleConfig,
                          area,
                          entry);
    }

    for (auto& [type, statistic] : statistics) {
      statistic.objectCount=statistic.nodeCount+statistic.wayCount+statistic.areaCount;
    }

    DumpStatisticWarnings(parameter,
                          statistics);

    if (parameter.IsDebugData()) {
      for (auto& [type, statistic] : statistics) {
        statistic.type=type;
      }

      DumpDataStatistics(MapToSortedList(statistics));
    }
  }

  void MapPainterStatistics::CalculateStatistics(const Projection& projection,
                                                 const MapParameter& parameter,
                                                 const StyleConfig& styleConfig,
                                                 const NodeRef& node,
                                                 DataStatistic& statistic)
  {
    ++statistic.nodeCount;
    ++statistic.coordCount;

    if (parameter.IsDebugData()) {
      IconStyleRef iconStyle=styleConfig.GetNodeIconStyle(node->GetFeatureValueBuffer(),
                                                          projection);

      if (iconStyle) {
        ++statistic.iconCount;
      }

      statistic.labelCount+=styleConfig.GetNodeTextStyleCount(node->GetFeatureValueBuffer(),
                                                              projection);

    }
  }

  void MapPainterStatistics::CalculateStatistics(const Projection& projection,
                                                 const MapParameter& parameter,
                                                 const StyleConfig& styleConfig,
                                                 const WayRef& way,
                                                 DataStatistic& statistic)
  {
    ++statistic.wayCount;
    statistic.coordCount+=way->nodes.size();

    if (parameter.IsDebugData()) {
      PathShieldStyleRef shieldStyle=styleConfig.GetWayPathShieldStyle(way->GetFeatureValueBuffer(),
                                                                       projection);
      PathTextStyleRef   pathTextStyle=styleConfig.GetWayPathTextStyle(way->GetFeatureValueBuffer(),
                                                                       projection);

      if (shieldStyle) {
        ++statistic.labelCount;
      }

      if (pathTextStyle) {
        ++statistic.labelCount;
      }
    }
  }

  void MapPainterStatistics::CalculateStatistics(const Projection& projection,
                                                 const MapParameter& parameter,
                                                 const StyleConfig& styleConfig,
                                                 const AreaRef& area,
                                                 DataStatistic& statistic)
  {
    ++statistic.areaCount;

    for (const auto& ring : area->rings) {
      statistic.coordCount+=ring.nodes.size();

      if (parameter.IsDebugData() && ring.IsMaster()) {
        IconStyleRef iconStyle=styleConfig.GetAreaIconStyle(area->GetType(),
                                                            ring.GetFeatureValueBuffer(),
                                                            projection);

        if (iconStyle) {
          ++statistic.iconCount;
        }

        statistic.labelCount+=styleConfig.GetAreaTextStyleCount(area->GetType(),
                                                                ring.GetFeatureValueBuffer(),
                                                                projection);
      }
    }
  }

  void MapPainterStatistics::DumpStatisticWarnings(const MapParameter& parameter,
                                                   const std::unordered_map<TypeInfoRef,DataStatistic>& statistics)
  {
    for (const auto& [type, statistic] : statistics) {
      if (type) {
        if (parameter.GetWarningObjectCountLimit()>0 &&
            statistic.objectCount>parameter.GetWarningObjectCountLimit()) {
          log.Warn() << "Type : " << type->GetName()
                     << " has " << statistic.objectCount
                     << " objects (performance limit: " << parameter.GetWarningObjectCountLimit() << ")";
        }

        if (parameter.GetWarningCoordCountLimit()>0 &&
            statistic.coordCount>parameter.GetWarningCoordCountLimit()) {
          log.Warn() << "Type : " << type->GetName()
                     << " has " << statistic.coordCount
                     << " coords (performance limit: " << parameter.GetWarningCoordCountLimit() << ")";
        }
      }
    }
  }
}
