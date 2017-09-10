#ifndef OSMSCOUT_MULTIDBROUTINGSTATE_H
#define OSMSCOUT_MULTIDBROUTINGSTATE_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2016  Tim Teulings
  Copyright (C) 2017  Lukas Karas

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

#include <osmscout/DataFile.h>
#include <osmscout/ObjectVariantDataFile.h>
#include <osmscout/Database.h>
#include <osmscout/Intersection.h>

#include <osmscout/routing/DBFileOffset.h>
#include <osmscout/routing/RoutingProfile.h>

namespace osmscout {

 /**
   * \ingroup Routing
   *
   * State for MultiDBRoutingService
   */
  class MultiDBRoutingState
  {
  private:
    DatabaseId          dbId1;
    DatabaseId          dbId2;

    RoutingProfileRef   profile1;
    RoutingProfileRef   profile2;

    std::set<Id>        overlapNodes;

  public:
    MultiDBRoutingState(DatabaseId dbId1,
                        DatabaseId dbId2,
                        const RoutingProfileRef& profile1,
                        const RoutingProfileRef& profile2,
                        const std::set<Id>& overlapNodes);

    virtual ~MultiDBRoutingState() =default;

    Vehicle GetVehicle() const;

    RoutingProfileRef GetProfile(DatabaseId database) const;

    void GetOverlappingDatabases(const DatabaseId &database,
                                 const Id &nodeId,
                                 std::set<DatabaseId> &overlappingDatabases) const;
  };

}

#endif /* OSMSCOUT_MULTIDBROUTINGSTATE_H */

