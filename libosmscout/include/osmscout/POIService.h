#ifndef OSMSCOUT_POISERVICE_H
#define OSMSCOUT_POISERVICE_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2014  Tim Teulings

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

#include <memory>
#include <vector>

#include <osmscout/Database.h>

#include <osmscout/TypeInfoSet.h>

#include <osmscout/util/GeoBox.h>

namespace osmscout {

  /**
   * \ingroup Service
   *
   * The POIServices offers methods special to working with POIs.
   *
   * Currently this includes the following functionality:
   * - Locating POIs of given types in a given area
   */
  class OSMSCOUT_API POIService final
  {
  private:
    DatabaseRef database;

  public:
    explicit POIService(const DatabaseRef& database);

    void GetPOIsInArea(const GeoBox& boundingBox,
                       const TypeInfoSet& nodeTypes,
                       std::vector<NodeRef>& nodes,
                       const TypeInfoSet& wayTypes,
                       std::vector<WayRef>& ways,
                       const TypeInfoSet& areaTypes,
                       std::vector<AreaRef>& areas) const;

    void GetPOIsInRadius(const GeoCoord& location,
                         const Distance& maxDistance,
                         const TypeInfoSet& nodeTypes,
                         std::vector<NodeRef>& nodes,
                         const TypeInfoSet& wayTypes,
                         std::vector<WayRef>& ways,
                         const TypeInfoSet& areaTypes,
                         std::vector<AreaRef>& areas) const;
  };

  //! \ingroup Service
  //! Reference counted reference to a POI service instance
  using POIServiceRef = std::shared_ptr<POIService>;
}

#endif
