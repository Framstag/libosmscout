#ifndef OSMSCOUT_ROUTENODE_H
#define OSMSCOUT_ROUTENODE_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2012  Tim Teulings

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

#include <vector>

#include <osmscout/TypeConfig.h>
#include <osmscout/Way.h>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>

namespace osmscout {

  class OSMSCOUT_API RouteNode : public Referencable
  {
  public:
    const static uint8_t hasAccess = 1 <<  0; //! We do have access rights to this way/area

    struct OSMSCOUT_API RouteExclude
    {
      Id       sourceWay;
      uint32_t targetPath;
    };

    struct OSMSCOUT_API RoutePath
    {
      Id      id;
      Id      wayId;
      TypeId  type;
      uint8_t maxSpeed;
      uint8_t flags;
      double  distance;
      double  lat;
      double  lon;

      inline bool HasAccess() const
      {
        return (flags & hasAccess) != 0;
      }
    };

  public:
    Id                        id;
    std::vector<RoutePath>    paths;
    std::vector<RouteExclude> excludes;

    inline Id GetId() const
    {
      return id;
    }

    bool Read(FileScanner& scanner);
    bool Write(FileWriter& writer) const;
  };

  typedef Ref<RouteNode> RouteNodeRef;
}

#endif
