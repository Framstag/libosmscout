#ifndef OSMSCOUT_ROUTINGPROFILE_H
#define OSMSCOUT_ROUTINGPROFILE_H

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

#include <vector>

#include <osmscout/RouteNode.h>
#include <osmscout/Types.h>

namespace osmscout {

  class OSMSCOUT_API RoutingProfile
  {
  public:
    virtual ~RoutingProfile();

    virtual bool CanUse(const RouteNode& currentNode, size_t pathIndex) const = 0;
    virtual double GetCosts(const RouteNode& currentNode, size_t pathIndex) const = 0;
    virtual double GetCosts(TypeId type, double distance) const = 0;
    virtual double GetCosts(double distance) const = 0;
    virtual double GetTime(TypeId type, double distance) const = 0;
  };

  class OSMSCOUT_API AbstractRoutingProfile : public RoutingProfile
  {
  protected:
    std::vector<double> speeds;
    double              minSpeed;
    double              maxSpeed;

  public:
    AbstractRoutingProfile();

    void AddType(TypeId type, double speed);

    inline bool CanUse(const RouteNode& currentNode, size_t pathIndex) const
    {
      TypeId type=currentNode.paths[pathIndex].type;

      return type<speeds.size() && speeds[type]>0.0;
    }

    inline double GetTime(TypeId type, double distance) const
    {
      return distance/speeds[type];
    }
  };

  class OSMSCOUT_API ShortestPathRoutingProfile : public AbstractRoutingProfile
  {
  public:
    inline double GetCosts(const RouteNode& currentNode, size_t pathIndex) const
    {
      return currentNode.paths[pathIndex].distance;
    }

    double GetCosts(TypeId type, double distance) const
    {
      return distance;
    }

    inline double GetCosts(double distance) const
    {
      return distance;
    }
  };

  class OSMSCOUT_API FastestPathRoutingProfile : public AbstractRoutingProfile
  {
  public:
    inline double GetCosts(const RouteNode& currentNode, size_t pathIndex) const
    {
      TypeId type=currentNode.paths[pathIndex].type;
      double distance=currentNode.paths[pathIndex].distance;

      double costs=distance/speeds[type];

      return costs;
    }

    double GetCosts(TypeId type, double distance) const
    {
      double costs=distance/speeds[type];

      return costs;
    }

    inline double GetCosts(double distance) const
    {

      double costs=distance/maxSpeed;

      return costs;
    }
  };
}

#endif
