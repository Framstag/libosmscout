#ifndef OSMSCOUT_MAP_MAPDATA_H
#define OSMSCOUT_MAP_MAPDATA_H

/*
  This source is part of the libosmscout-map library
  Copyright (C) 2019  Tim Teulings

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
#include <vector>

#include <osmscoutmap/MapImportExport.h>

#include <osmscout/Node.h>
#include <osmscout/Area.h>
#include <osmscout/Way.h>
#include <osmscout/Route.h>

#include <osmscout/GroundTile.h>
#include <osmscout/SRTM.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

  /**
   * \ingroup Renderer
   *
   * This is the data structure holding all to be rendered data.
   */
  class OSMSCOUT_MAP_API MapData CLASS_FINAL
  {
  public:
    std::vector<NodeRef>  nodes;        //!< Nodes as retrieved from database
    std::vector<AreaRef>  areas;        //!< Areas as retrieved from database
    std::vector<WayRef>   ways;         //!< Ways as retrieved from database
    std::vector<RouteRef> routes;       //!< Routes as retrieved from database
    std::list<NodeRef>    poiNodes;     //!< List of manually added nodes (not managed or changed by the database)
    std::list<AreaRef>    poiAreas;     //!< List of manually added areas (not managed or changed by the database)
    std::list<WayRef>     poiWays;      //!< List of manually added ways (not managed or changed by the database)
    std::list<GroundTile> groundTiles;  //!< List of ground tiles (optional)
    std::list<GroundTile> baseMapTiles; //!< List of ground tiles of base map (optional)
    SRTMDataRef           srtmTile;     //!< Optional data with height information

  public:
    void ClearDBData();
  };

  using MapDataRef = std::shared_ptr<MapData>;

  /**
   * \defgroup Renderer Map rendering
   *
   * Classes and methods related to rendering of maps.
   */
}

#endif
