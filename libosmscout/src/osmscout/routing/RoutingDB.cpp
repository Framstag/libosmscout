/*
  This source is part of the libosmscout library
  Copyright (C) 2018  Tim Teulings

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

#include <osmscout/routing/RoutingDB.h>

#include <osmscout/routing/RoutingService.h>

namespace osmscout {

  RoutingDatabase::RoutingDatabase()
    :
    routeNodeDataFile(RoutingService::GetDataFilename(osmscout::RoutingService::DEFAULT_FILENAME_BASE),
                      RoutingService::GetIndexFilename(osmscout::RoutingService::DEFAULT_FILENAME_BASE),
                      /*indexCacheSize*/
                      12000,
                      /*dataCacheSize*/
                      1000),
    junctionDataFile(RoutingService::FILENAME_INTERSECTIONS_DAT,
                     RoutingService::FILENAME_INTERSECTIONS_IDX,
                     /*indexCacheSize*/
                     10000,
                     /*dataCacheSize*/
                     1000)
  {
  }

  bool RoutingDatabase::Open(const DatabaseRef& database)
  {
    typeConfig=database->GetTypeConfig();
    path=database->GetPath();

    if (!routeNodeDataFile.Open(database->GetTypeConfig(),
                                database->GetPath(),
                                true,
                                database->GetParameter().GetRouterDataMMap())) {
      log.Error() << "Cannot open '" << database->GetPath() << "'!";
      return false;
    }

    return objectVariantDataFile.Load(*(database->GetTypeConfig()),
                                      AppendFileToDir(database->GetPath(),
                                                      RoutingService::GetData2Filename(osmscout::RoutingService::DEFAULT_FILENAME_BASE)));
  }

  void RoutingDatabase::Close()
  {
    routeNodeDataFile.Close();
    junctionDataFile.Close();

    typeConfig.reset();
    path.clear();
  }

  bool RoutingDatabase::GetJunctions(const std::set<Id>& ids,
                                     std::vector<JunctionRef>& junctions)
  {
    if (!junctionDataFile.IsOpen()) {
      if (!junctionDataFile.Open(typeConfig,
                                 path,
                                 false,
                                 false)) {
        return false;
      }
    }

    bool result=junctionDataFile.Get(ids,
                                     junctions);

    if (!junctionDataFile.Close()) {
      result=false;
    }

    return result;
  }
}
