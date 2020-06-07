#ifndef OSMSCOUT_NAVIGATION_NAVIGATION_H
#define OSMSCOUT_NAVIGATION_NAVIGATION_H

/*
 This source is part of the libosmscout library
 Copyright (C) 2014  Tim Teulings, Vladimir Vyskocil

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

#include <limits>

#include <osmscout/GeoCoord.h>
#include <osmscout/routing/Route.h>

#include <osmscout/util/Geometry.h>
#include <osmscout/util/Time.h>

namespace osmscout {
  static double one_degree_at_equator=111320.0;

  template<class NodeDescriptionTmpl>
  class OutputDescription
  {
  public:
    virtual ~OutputDescription() = default;

    virtual void NextDescription(const Distance& /*distance*/,
                                 std::list<RouteDescription::Node>::const_iterator& /*node*/,
                                 std::list<RouteDescription::Node>::const_iterator /*end*/)
    {};

    virtual NodeDescriptionTmpl GetDescription()
    {return description;};

    virtual void Clear()
    {};
  protected:
    NodeDescriptionTmpl description;
  };

  template<class NodeDescriptionTmpl>
  class Navigation
  {
  private:
    /**
     * return true and set foundNode with the start node of the closest route segment from the location and foundAbscissa with the abscissa
     * of the projected point on the line, return false if there is no such point that is closer than snapDistanceInMeters
     * from the route.
     * The search start a the locationOnRoute node toward the end.
     */
    bool SearchClosestSegment(const GeoCoord& location,
                              std::list<RouteDescription::Node>::const_iterator& foundNode,
                              double& foundAbscissa,
                              double& minDistance)
    {
      auto   nextNode=locationOnRoute;
      double abscissa=0.0;
      bool   found=false;
      double qx,qy;
      minDistance=std::numeric_limits<double>::max();
      for (auto node=nextNode++; node!=route->Nodes().end(); node++) {
        if (nextNode==route->Nodes().end()) {
          break;
        }
        double d=DistanceToSegment(location.GetLon(),
                                   location.GetLat(),
                                   node->GetLocation().GetLon(),
                                   node->GetLocation().GetLat(),
                                   nextNode->GetLocation().GetLon(),
                                   nextNode->GetLocation().GetLat(),
                                   abscissa,
                                   qx,
                                   qy);
        if (minDistance>=d) {
          minDistance=d;
          if (d<=distanceInDegrees(snapDistanceInMeters,
                                   location.GetLat())) {
            foundNode    =node;
            foundAbscissa=abscissa;
            found        =true;
          }
        }
        else if (found && d>minDistance*2) {
          // Stop the search we have a good candidate
          break;
        }
        nextNode++;
      }
      return found;
    }

  public:
    /**
     * outputDescr pointer is not owned, it should not be destroyed before Navigation,
     * caller is responsible for deleting it.
     */
    explicit Navigation(OutputDescription<NodeDescriptionTmpl>* outputDescr)
      : route(nullptr),
        outputDescription(outputDescr),
        snapDistanceInMeters(Distance::Of<Meter>(25.0))
    {
    }

    static inline double distanceInDegrees(const Distance &d,
                                           double latitude)
    {
      return d.AsMeter()/(one_degree_at_equator*cos(M_PI*latitude/180));
    }

    void SetRoute(RouteDescription* newRoute)
    {
      assert(newRoute);
      route                                                         =newRoute;
      distanceFromStart                                             =Distance::Of<Meter>(0.0);
      durationFromStart                                             =Duration(osmscout::Duration::zero());
      locationOnRoute                                               =route->Nodes().begin();
      nextWaypoint                                                  =route->Nodes().begin();
      outputDescription->Clear();
      outputDescription->NextDescription(Distance::Of<Meter>(-1.0),
                                         nextWaypoint,
                                         route->Nodes().end());
      std::list<RouteDescription::Node>::const_iterator lastWaypoint=--(route->Nodes().end());
      duration=lastWaypoint->GetTime();
      distance=lastWaypoint->GetDistance();
    }

    void Clear()
    {
      route=nullptr;
    }

    bool HasRoute()
    {
      return route!=nullptr;
    }

    void SetSnapDistance(const Distance &distance)
    {
      snapDistanceInMeters=distance;
    }

    Distance GetDistanceFromStart()
    {
      return distanceFromStart;
    }

    Duration GetDurationFromStart()
    {
      return durationFromStart;
    }

    Distance GetDistance()
    {
      return distance;
    }

    Duration GetDuration()
    {
      return duration;
    }

    NodeDescriptionTmpl nextWaypointDescription()
    {
      return outputDescription->GetDescription();
    }

    const RouteDescription::Node& GetCurrentNode()
    {
      return *locationOnRoute;
    }

    bool UpdateCurrentLocation(const GeoCoord& location,
                               double& minDistance)
    {
      auto   foundNode    =locationOnRoute;
      double foundAbscissa=0.0;

      bool found=SearchClosestSegment(location,
                                      foundNode,
                                      foundAbscissa,
                                      minDistance);
      if (found) {
        locationOnRoute=foundNode;

        auto nextNode=foundNode;

        nextNode++;
        outputDescription->NextDescription(locationOnRoute->GetDistance(),
                                           nextWaypoint,
                                           route->Nodes().end());
        if (foundAbscissa<0.0) {
          foundAbscissa=0.0;
        }
        else if (foundAbscissa>1.0) {
          foundAbscissa=1.0;
        }
        distanceFromStart                                         =
          nextNode->GetDistance()*foundAbscissa+locationOnRoute->GetDistance()*(1.0-foundAbscissa);
        durationFromStart                                         =
          std::chrono::duration_cast<Duration>(
          nextNode->GetTime()*foundAbscissa+locationOnRoute->GetTime()*(1.0-foundAbscissa));
        return true;
      }

      return false;
    };

    bool ClosestPointOnRoute(const GeoCoord& location, GeoCoord& locOnRoute)
    {
      if(route == nullptr){
        return false;
      }
      std::list<RouteDescription::Node>::const_iterator nextNode = route->Nodes().begin();
      GeoCoord intersection, foundIntersection;
      double abscissa;
      double minDistance=std::numeric_limits<double>::max();
      for (auto node=nextNode++; node!=route->Nodes().end(); node++) {
        if (nextNode==route->Nodes().end()) {
          break;
        }
        double d = DistanceToSegment(location,
                                     node->GetLocation(),
                                     nextNode->GetLocation(),
                                     abscissa,
                                     intersection);
        if (minDistance > d) {
          minDistance = d;
          foundIntersection = intersection;
        }
        ++nextNode;
      }
      locOnRoute = foundIntersection;

      return true;
    }

  private:
    RouteDescription* route;                 // current route description
    std::list<RouteDescription::Node>::const_iterator locationOnRoute;       // last passed node on the route
    std::list<RouteDescription::Node>::const_iterator nextWaypoint;          // next node with routing instructions
    OutputDescription<NodeDescriptionTmpl>            * outputDescription;    // next routing instructions
    Distance                                          distanceFromStart;     // current length from the beginning of the route (in meters)
    Duration                                          durationFromStart;     // current (estimated) duration from the beginning of the route
    Distance                                          distance;              // whole lenght of the route
    Duration                                          duration;              // whole estimated duration of the route
    Distance                                          snapDistanceInMeters;  // max distance from the route path to consider being on route
  };
}

#endif
