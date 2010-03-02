#ifndef OSMSCOUT_ROUTE_H
#define OSMSCOUT_ROUTE_H

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

#include <list>

#include <osmscout/TypeConfig.h>

namespace osmscout {

  class RouteData
  {
  public:
    class RouteEntry
    {
    private:
      Id wayId;
      Id nodeId;

    public:
      RouteEntry(Id wayId, Id routeId);

      inline Id GetWayId() const
      {
        return wayId;
      }

      inline Id GetNodeId() const
      {
        return nodeId;
      }
    };

  private:
    std::list<RouteEntry> entries;

  public:
    RouteData();

    void Clear();

    void AddEntry(Id wayId, Id nodeId);

    inline const std::list<RouteEntry>& Entries() const
    {
      return entries;
    }
  };

  class RouteDescription
  {
  public:
    enum Action
    {
      start,
      drive,
      switchRoad, // TODO: make it more precise
      pass,       // TODO: make it more precise
      reachTarget
    };

    class RouteStep
    {
    private:
      double      distance;
      Action      action;
      std::string name;
      std::string refName;

    public:
      RouteStep(double distance,
                Action action,
                const std::string& name,
                const std::string& refName);

      inline double GetDistance() const
      {
        return distance;
      }

      inline Action GetAction() const
      {
        return action;
      }

      inline std::string GetName() const
      {
        return name;
      }

      inline std::string GetRefName() const
      {
        return refName;
      }
    };

  private:
    std::list<RouteStep> steps;

  public:
    RouteDescription();

    void Clear();

    void AddStep(double distance,
                 Action action,
                 const std::string& name,
                 const std::string& refName);

    inline const std::list<RouteStep>& Steps() const
    {
      return steps;
    }
  };
}

#endif
