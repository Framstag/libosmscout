#ifndef OSMSCOUT_MAP_MAPPAINTERSTATISTICS_H
#define OSMSCOUT_MAP_MAPPAINTERSTATISTICS_H

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

#include <osmscoutmap/MapImportExport.h>

#include <osmscout/TypeConfig.h>

#include <osmscout/projection/Projection.h>

#include <osmscoutmap/MapParameter.h>
#include <osmscoutmap/MapData.h>
#include <osmscoutmap/StyleConfig.h>

namespace osmscout {

  class OSMSCOUT_MAP_API MapPainterStatistics
  {
  private:
    /**
     * Structure used for internal statistic collection
     */
    struct OSMSCOUT_MAP_API DataStatistic
    {
      TypeInfoRef type;        //!< Type
      size_t      objectCount=0; //!< Sum of nodeCount, wayCount, areaCont
      size_t      nodeCount  =0;   //!< Number of Node objects
      size_t      wayCount   =0;    //!< Number of Way objects
      size_t      areaCount  =0;   //!< Number of Area objects
      size_t      coordCount =0;  //!< Number of coordinates
      size_t      labelCount =0;  //!< Number of labels
      size_t      iconCount  =0;   //!< Number of icons

      DataStatistic() = default;
    };

  private:
    void DumpDataStatistics(const std::list<DataStatistic>& statistics);

    std::list<DataStatistic> MapToSortedList(const std::unordered_map<TypeInfoRef,DataStatistic>& statistics);

    void DumpDataStatistics(const StyleConfig& styleConfig,
                            const Projection& projection,
                            const MapParameter& parameter,
                            const MapData& data);

    void CalculateStatistics(const Projection& projection,
                             const MapParameter& parameter,
                             const StyleConfig& styleConfig,
                             const NodeRef& node,
                             DataStatistic& statistic);

    void CalculateStatistics(const Projection& projection,
                             const MapParameter& parameter,
                             const StyleConfig& styleConfig,
                             const WayRef& way,
                             DataStatistic& statistic);

    void CalculateStatistics(const Projection& projection,
                             const MapParameter& parameter,
                             const StyleConfig& styleConfig,
                             const AreaRef& area,
                             DataStatistic& statistic);

    void DumpStatisticWarnings(const MapParameter& parameter,
                               const std::unordered_map<TypeInfoRef,DataStatistic>& statistics);

  public:
    MapPainterStatistics() = default;

    void DumpMapPainterStatistics(const StyleConfig& styleConfig,
                                  const Projection& projection,
                                  const MapParameter& parameter,
                                  const MapData& data);
  };
}
#endif
