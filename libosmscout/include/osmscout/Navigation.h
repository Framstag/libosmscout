#ifndef OSMSCOUT_NAVIGATION_H
#define OSMSCOUT_NAVIGATION_H

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

namespace osmscout {
    static double one_degree_at_equator = 111320.0;

    template<class NodeDescriptionTmpl> class OSMSCOUT_API OutputDescription {
    public:
        virtual void NextDescription(double distance,
                                     std::list<RouteDescription::Node>::const_iterator &node,
                                     std::list<RouteDescription::Node>::const_iterator end){};
        virtual NodeDescriptionTmpl GetDescription(){ return description; };
        virtual void Clear(){};
    private:
        NodeDescriptionTmpl description;
    };

    template<class NodeDescriptionTmpl> class OSMSCOUT_API Navigation {
    private:
        /**
         * return true and set foundNode with the start node of the closest route segment from the location and foundAbscissa with the abscissa
         * of the projected point on the line, return false if there is no such point that is closer than snapDistanceInMeters
         * from the route.
         * The search start a the locationOnRoute node toward the end.
         */
        bool SearchClosestSegment(const GeoCoord &location, std::list<RouteDescription::Node>::const_iterator &foundNode, double &foundAbscissa,
                                  double &minDistance)
        {
            std::list<RouteDescription::Node>::const_iterator nextNode = locationOnRoute;
            double abscissa = 0.0;
            bool found = false;
            double qx, qy;
            minDistance = std::numeric_limits<double>::max();
            for(std::list<RouteDescription::Node>::const_iterator node = nextNode++; node != route->Nodes().end(); node++){
                if(nextNode == route->Nodes().end()){
                    break;
                }
                double d =DistanceToSegment(location.GetLon(),location.GetLat(),
                                            node->GetLocation().GetLon(),node->GetLocation().GetLat(),
                                            nextNode->GetLocation().GetLon(),nextNode->GetLocation().GetLat(),
                                            abscissa,qx,qy);
                if(minDistance>=d){
                    minDistance = d;
                    if(d <= distanceInDegrees(snapDistanceInMeters, location.GetLat())){
                        foundNode = node;
                        foundAbscissa = abscissa;
                        found = true;
                    }
                }  else if(found && d>minDistance*2){
                    // Stop the search we have a good candidate
                    break;
                }
                nextNode++;
            }
            return found;
        }


    public:
        Navigation(OutputDescription<NodeDescriptionTmpl> *outputDescr) : route(0), outputDescription(outputDescr),snapDistanceInMeters(25.0)        {
        }

        static inline double distanceInDegrees(double d, double latitude)
        {
            return d / (one_degree_at_equator * cos(M_PI * latitude / 180));
        }

        void SetRoute(RouteDescription *newRoute)
        {
            route = newRoute;
            distanceFromStart = 0.0;
            durationFromStart = 0.0;
            locationOnRoute = route->Nodes().begin();
            nextWaypoint = route->Nodes().begin();
            outputDescription->Clear();
            outputDescription->NextDescription(-1.0, nextWaypoint, route->Nodes().end());
            std::list<RouteDescription::Node>::const_iterator lastWaypoint = --(route->Nodes().end());
            duration = lastWaypoint->GetTime();
            distance = lastWaypoint->GetDistance();
        }

        void SetSnapDistance(double distance)
        {
            snapDistanceInMeters = distance;
        }

        double GetDistanceFromStart()
        {
            return distanceFromStart;
        }

        double GetDurationFromStart()
        {
            return durationFromStart;
        }

        double GetDistance()
        {
            return distance;
        }

        double GetDuration()
        {
            return duration;
        }

        NodeDescriptionTmpl nextWaypointDescription()
        {
            return outputDescription->GetDescription();
        }

        const RouteDescription::Node &GetCurrentNode(){
            return *locationOnRoute;
        }

        bool UpdateCurrentLocation(const GeoCoord &location, double &minDistance)
        {
            std::list<RouteDescription::Node>::const_iterator foundNode = locationOnRoute;
            double foundAbscissa = 0.0;

            bool found = SearchClosestSegment(location, foundNode, foundAbscissa, minDistance);
            if(found){
                locationOnRoute = foundNode;
                std::list<RouteDescription::Node>::const_iterator nextNode = foundNode;
                nextNode++;
                outputDescription->NextDescription(locationOnRoute->GetDistance(), nextWaypoint, route->Nodes().end());
                if(foundAbscissa < 0.0){
                    foundAbscissa = 0.0;
                } else if(foundAbscissa > 1.0){
                    foundAbscissa = 1.0;
                }
                distanceFromStart =  nextNode->GetDistance() * foundAbscissa +  locationOnRoute->GetDistance() * (1.0 - foundAbscissa);
                durationFromStart =  nextNode->GetTime() * foundAbscissa + locationOnRoute->GetTime() * (1.0 - foundAbscissa);
                return true;
            } else {
                return false;
            }
        };

    private:
        RouteDescription*                                   route;                 // current route description
        std::list<RouteDescription::Node>::const_iterator   locationOnRoute;       // last passed node on the route
        std::list<RouteDescription::Node>::const_iterator   nextWaypoint;          // next node with routing instructions
        OutputDescription<NodeDescriptionTmpl>              *outputDescription;    // next routing instructions
        double                                              distanceFromStart;     // current length from the beginning of the route (in meters)
        double                                              durationFromStart;     // current (estimated) duration from the beginning of the route (in fract hours)
        double                                              distance;              // whole lenght of the route (in meters)
        double                                              duration;              // whole estimated duration of the route (in fraction of hours)
        double                                              snapDistanceInMeters;  // max distance in meters from the route path to consider being on route
    };

}

#endif
