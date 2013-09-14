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

#include <osmscout/Types.h>

#include <osmscout/Way.h>

#include <osmscout/RouteNode.h>

#include "Area.h"

namespace osmscout {

  /**
   * Abstract interface for a routing profile. A routing profile decides about the costs
   * of taking a certain way. It thus may hold information about how fast ways can be used,
   * maximum speed of the traveling device etc...
   */
  class OSMSCOUT_API RoutingProfile
  {
  public:
    virtual ~RoutingProfile();

    virtual bool CanUse(const RouteNode& currentNode,
                        size_t pathIndex) const = 0;
    virtual double GetCosts(const RouteNode& currentNode,
                            size_t pathIndex) const = 0;
    virtual double GetCosts(const Area& area,
                            double distance) const = 0;
    virtual double GetCosts(const Way& way,
                            double distance) const = 0;
    virtual double GetCosts(double distance) const = 0;
    virtual double GetTime(const Area& area,
                           double distance) const = 0;
    virtual double GetTime(const Way& way,
                           double distance) const = 0;
  };

  /**
   * Common base class for our concrete profile instantiations.
   */
  class OSMSCOUT_API AbstractRoutingProfile : public RoutingProfile
  {
  public:
    enum Vehicle
    {
      vehicleFoot,
      vehicleBicylce,
      vehicleCar
    };

  protected:
    Vehicle             vehicle;
    uint8_t             vehicleRouteNodeBit;
    std::vector<double> speeds;
    double              minSpeed;
    double              maxSpeed;
    double              vehicleMaxSpeed;

  public:
    AbstractRoutingProfile();

    void SetVehicle(Vehicle vehicle);
    void SetVehicleMaxSpeed(double maxSpeed);

    inline Vehicle GetVehicle() const
    {
      return vehicle;
    }

    void AddType(TypeId type, double speed);

    inline bool CanUse(const RouteNode& currentNode,
                       size_t pathIndex) const
    {
      if (!(currentNode.paths[pathIndex].flags & vehicleRouteNodeBit)) {
        return false;
      }

      TypeId type=currentNode.paths[pathIndex].type;

      return type<speeds.size() && speeds[type]>0.0;
    }

    inline double GetTime(const Area& area,
                          double distance) const
    {
      double speed=speeds[area.GetType()];

      speed=std::min(vehicleMaxSpeed,speed);

      return distance/speed;
    }

    inline double GetTime(const Way& way,
                          double distance) const
    {
      double speed;

      if (way.GetMaxSpeed()>0) {
        speed=way.GetMaxSpeed();
      }
      else {
        speed=speeds[way.GetType()];
      }

      speed=std::min(vehicleMaxSpeed,speed);

      return distance/speed;
    }
  };

  /**
   * Profile that defines costs in a way that the shortest way is chosen (cost==distance).
   */
  class OSMSCOUT_API ShortestPathRoutingProfile : public AbstractRoutingProfile
  {
  public:
    inline double GetCosts(const RouteNode& currentNode,
                           size_t pathIndex) const
    {
      return currentNode.paths[pathIndex].distance;
    }

    inline double GetCosts(const Area& area,
                           double distance) const
    {
      return distance;
    }

    inline double GetCosts(const Way& way,
                           double distance) const
    {
      return distance;
    }

    inline double GetCosts(double distance) const
    {
      return distance;
    }
  };

  /**
   * Profile that defines costs base of the time the traveling device needs
   * for a certain way resulting in the fastest path chosen (cost=distance/speedForWayType).
   */
  class OSMSCOUT_API FastestPathRoutingProfile : public AbstractRoutingProfile
  {
  public:
    inline double GetCosts(const RouteNode& currentNode,
                           size_t pathIndex) const
    {
      double speed;

      if (currentNode.paths[pathIndex].maxSpeed>0) {
        speed=currentNode.paths[pathIndex].maxSpeed;
      }
      else {
        speed=speeds[currentNode.paths[pathIndex].type];
      }

      speed=std::min(vehicleMaxSpeed,speed);

      return currentNode.paths[pathIndex].distance/speed;
    }

    inline double GetCosts(const Area& area,
                           double distance) const
    {
      double speed=speeds[area.GetType()];

      speed=std::min(vehicleMaxSpeed,speed);

      return distance/speed;
    }

    inline double GetCosts(const Way& way,
                           double distance) const
    {
      double speed;

      if (way.GetMaxSpeed()>0) {
        speed=way.GetMaxSpeed();
      }
      else {
        speed=speeds[way.GetType()];
      }

      speed=std::min(vehicleMaxSpeed,speed);

      return distance/speed;
    }

    inline double GetCosts(double distance) const
    {
      double speed=maxSpeed;

      speed=std::min(vehicleMaxSpeed,speed);

      return distance/speed;
    }
  };
}

#endif
