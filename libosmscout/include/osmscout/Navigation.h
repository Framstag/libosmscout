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

#include "GeoCoord.h"
#include "Route.h"

namespace osmscout {
    static double one_degree_at_equator = 111320.0;
    
    double distanceToSegment(const GeoCoord &p, const GeoCoord &p1, const GeoCoord &p2, double &r);

    template<class NodeDescriptionTmpl> class OSMSCOUT_API OutputDescription {
    public:
        virtual void NextDescription(std::list<RouteDescription::Node>::const_iterator &node,
                                     std::list<RouteDescription::Node>::const_iterator end){};
        virtual NodeDescriptionTmpl GetDescription(){ return description; };
    private:
        NodeDescriptionTmpl description;
    };

    template<class NodeDescriptionTmpl> class OSMSCOUT_API Navigation {
    public:
        Navigation(OutputDescription<NodeDescriptionTmpl> *outputDescr) : route(0), outputDescription(outputDescr),snapDistanceInMeters(25.0)
        {
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
            outputDescription->NextDescription(nextWaypoint, route->Nodes().end());
            std::list<RouteDescription::Node>::const_iterator lastWaypoint = --(route->Nodes().end());
            duration = lastWaypoint->GetTime();
            distance = lastWaypoint->GetDistance();
        }
        
        void SetSnapDistance(double distance)
        {
            snapDistanceInMeters = distance;
        }

        RouteDescription::Node GetNextWaypoint()
        {
            return *nextWaypoint;
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
        
        bool UpdateCurrentLocation(double latitude, double longitude, double &minDistance)
        {
            location.Set(latitude, longitude);
            std::list<RouteDescription::Node>::const_iterator nextNode = locationOnRoute;
            std::list<RouteDescription::Node>::const_iterator foundNode = locationOnRoute;
            double abscissa = 0;
            double foundAbscissa = 0.0;
            bool found = false;
            minDistance = MAXFLOAT;
            for(std::list<RouteDescription::Node>::const_iterator node = nextNode++; node != route->Nodes().end(); node++){
                if(nextNode == route->Nodes().end()){
                    break;
                }
                double d = distanceToSegment(location, node->GetLocation(), nextNode->GetLocation(), abscissa);
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
        
            if(found){
                if(foundAbscissa < 0.0){
                    foundAbscissa = 0.0;
                } else if(foundAbscissa > 1.0){
                    foundAbscissa = 1.0;
                }
                locationOnRoute = foundNode;
                nextNode = foundNode;
                nextNode++;
                if(locationOnRoute->GetDistance() >  nextWaypoint->GetDistance() ||
                   locationOnRoute->GetDistance() == 0.0) {
                    if(foundAbscissa == 0.0){
                        nextWaypoint = locationOnRoute;
                    } else {
                        nextWaypoint = nextNode;
                    }
                    outputDescription->NextDescription(nextWaypoint, route->Nodes().end());
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
        GeoCoord                                            location;              // current location
        double                                              distanceFromStart;     // current length from the beginning of the route (in meters)
        double                                              durationFromStart;     // current (estimated) duration from the beginning of the route (in fract hours)
        double                                              distance;              // whole lenght of the route (in meters)
        double                                              duration;              // whole estimated duration of the route (in fraction of hours)
        double                                              snapDistanceInMeters;  // max distance in meters from the route path to consider being on route
    };

}

#endif
