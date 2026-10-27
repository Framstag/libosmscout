/*
  This source is part of the libosmscout library
  Copyright (C) 2026  Tim Teulings

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

#include <osmscoutclient/routing/RouteStepBuilder.h>

#include <osmscoutclient/routing/RouteStepCollector.h>

#include <osmscout/util/Distance.h>
#include <osmscout/util/Geometry.h>
#include <osmscout/util/Time.h>

#include <algorithm>
#include <chrono>
#include <memory>
#include <optional>
#include <utility>

namespace osmscout::client {

  namespace {

    Duration SecondsToDuration(double seconds)
    {
      return std::chrono::duration_cast<Duration>(std::chrono::duration<double>(seconds));
    }

    /**
     * The distance the position has progressed beyond the previous route node.
     */
    double TravelledMeters(const RouteDescription::NodeIterator& previous,
                           const RouteDescription::NodeIterator& last,
                           const GeoCoord& coord,
                           double abscissa)
    {
      double travelled=GetEllipsoidalDistance(coord, previous->GetLocation()).AsMeter();

      auto   nextNode=previous;

      ++nextNode;

      if (abscissa>0.0 && nextNode!=last) {
        double segmentLength=GetEllipsoidalDistance(previous->GetLocation(),
                                                    nextNode->GetLocation()).AsMeter();

        if (segmentLength>0.0) {
          travelled=std::min(segmentLength*abscissa, segmentLength);
        }
      }

      return travelled;
    }

    /**
     * The time from the route start at the position.
     */
    Duration TimeAtPosition(const RouteDescription::NodeIterator& previous,
                            const RouteDescription::NodeIterator& last,
                            double abscissa)
    {
      Duration timeAtPosition=previous->GetTime();

      auto     nextNode=previous;

      ++nextNode;

      if (abscissa>0.0 && nextNode!=last) {
        double nodeTime=DurationAsSeconds(timeAtPosition);
        double nextNodeTime=DurationAsSeconds(nextNode->GetTime());

        timeAtPosition=SecondsToDuration(nodeTime+
                                         std::min(abscissa, 1.0)*(nextNodeTime-nodeTime));
      }

      return timeAtPosition;
    }

    /**
     * The progress facts of the step ahead: how far and how long it still is from the position.
     */
    void FillProgress(RouteStep& step,
                      double nodeMeters,
                      double travelledMeters,
                      const Duration& timeAtPosition)
    {
      step.remainingDistance=Distance::Of<Meter>(std::max(0.0,
                                                          step.distanceFromStart.AsMeter()-
                                                          nodeMeters-
                                                          travelledMeters));
      step.remainingTime=SecondsToDuration(std::max(0.0,
                                                    DurationAsSeconds(step.timeFromStart-
                                                                      timeAtPosition)));
    }
  }

  std::list<RouteStep> RouteStepBuilder::GenerateRouteInstructions(const RouteDescription::NodeIterator& first,
                                                                   const RouteDescription::NodeIterator& last) const
  {
    RouteStepCollector   collector;

    RouteSteps           collected=collector.Collect(first, last);

    std::list<RouteStep> result;

    for (auto& step : collected.steps) {
      result.push_back(std::move(step));
    }

    return result;
  }

  RouteStep RouteStepBuilder::GenerateNextRouteInstruction(const RouteDescription::NodeIterator& previous,
                                                           const RouteDescription::NodeIterator& last,
                                                           const GeoCoord& coord) const
  {
    return GenerateNextRouteInstruction(previous, last, coord, 0.0);
  }

  RouteStep RouteStepBuilder::GenerateNextRouteInstruction(const RouteDescription::NodeIterator& previous,
                                                           const RouteDescription::NodeIterator& last,
                                                           const GeoCoord& coord,
                                                           double abscissa) const
  {
    if (previous==last) {
      return RouteStep();
    }

    Distance           previousDistance=previous->GetDistance();
    double             nodeMeters=previousDistance.AsMeter();

    RouteStepCollector collector;

    RouteSteps         collected=collector.Collect(previous,
                                                   last,
                                                   previousDistance+
                                                   Distance::Of<Meter>(CollectWindowMeters));

    double   travelledMeters=TravelledMeters(previous, last, coord, abscissa);
    Duration timeAtPosition=TimeAtPosition(previous, last, abscissa);

    // The first step that still lies ahead of the position.
    size_t index=0;

    while (index<collected.steps.size() &&
           collected.steps[index].distanceFromStart.AsMeter()<=nodeMeters) {
      ++index;
    }

    RouteStep                step;
    std::optional<RouteStep> follower;

    if (index<collected.steps.size()) {
      step=collected.steps[index];

      if (index+1<collected.steps.size()) {
        double gap=collected.steps[index+1].distanceFromStart.AsMeter()-
                    step.distanceFromStart.AsMeter();

        if (gap>0.0 && gap<=NextNextWindowMeters) {
          follower=collected.steps[index+1];
        }
      }
    }
    else {
      // Nothing described within the window: the manoeuvre ahead is the arrival.
      auto arrivalNode=last;

      --arrivalNode;

      auto arrival=RouteStepCollector::BuildNodeStep(*arrivalNode);

      if (!arrival) {
        if (collected.steps.empty()) {
          return RouteStep();
        }

        index=collected.steps.size()-1;
        step=collected.steps[index];
      }
      else {
        step=*arrival;
      }
    }

    FillProgress(step, nodeMeters, travelledMeters, timeAtPosition);

    // The following manoeuvre, when it is close enough to be announced.
    if (follower) {
      step.nextNext=std::make_shared<RouteStep>(std::move(*follower));
    }

    return step;
  }
} // namespace osmscout::client
