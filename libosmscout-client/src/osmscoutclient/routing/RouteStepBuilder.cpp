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
#include <vector>

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
     * Whether a step describes a manoeuvre a driver acts on, rather than a fact of its node (a
     * name, a way type, a max speed, lanes, a POI). `WayNamePostprocessor`/`WayTypePostprocessor`
     * describe almost every route node, so the first collected step is normally no manoeuvre.
     */
    bool IsManoeuvre(const RouteStep& step)
    {
      return step.kind!=RouteStep::Kind::informative &&
             step.kind!=RouteStep::Kind::unknown;
    }

    /**
     * The index of the first manoeuvre of `steps` that lies strictly after `meters`, in route order.
     * A manoeuvre that shares the passed node's position is behind the position, not ahead of it.
     */
    std::optional<size_t> FirstManoeuvreAfter(const std::vector<RouteStep>& steps, double meters)
    {
      for (size_t index=0; index<steps.size(); ++index) {
        if (steps[index].distanceFromStart.AsMeter()>meters &&
            IsManoeuvre(steps[index])) {
          return index;
        }
      }

      return std::nullopt;
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

    const double totalMeters=std::prev(last)->GetDistance().AsMeter();
    const double remainderMeters=std::max(0.0, totalMeters-nodeMeters);

    auto collectWithin=[&](double windowMeters) {
      return collector.Collect(previous,
                               last,
                               previousDistance+Distance::Of<Meter>(windowMeters));
    };

    // Look ahead until the first manoeuvre after the position. The fixed window is only the first
    // look: a long leg without a manoeuvre would otherwise fall through to the arrival, and a
    // described node inside it would be published as the manoeuvre ahead.
    double                windowMeters=std::min(CollectWindowMeters, remainderMeters);
    RouteSteps            collected=collectWithin(windowMeters);
    std::optional<size_t> manoeuvreIndex;

    for (;;) {
      manoeuvreIndex=FirstManoeuvreAfter(collected.steps, nodeMeters);

      if (manoeuvreIndex || windowMeters>=remainderMeters) {
        break;
      }

      windowMeters=std::min(windowMeters*2.0, remainderMeters);
      collected=collectWithin(windowMeters);
    }

    double   travelledMeters=TravelledMeters(previous, last, coord, abscissa);
    Duration timeAtPosition=TimeAtPosition(previous, last, abscissa);

    RouteStep                step;
    std::optional<RouteStep> follower;

    if (manoeuvreIndex) {
      step=collected.steps[*manoeuvreIndex];

      // The follower is the next manoeuvre, not the next collected node, so the looked-ahead range
      // has to reach the announce window beyond the step ahead.
      double followerLimit=step.distanceFromStart.AsMeter()+NextNextWindowMeters;

      if (collected.steps.back().distanceFromStart.AsMeter()<followerLimit &&
          windowMeters<remainderMeters) {
        windowMeters=std::min(std::max(windowMeters, followerLimit-nodeMeters),
                              remainderMeters);
        collected=collectWithin(windowMeters);
        manoeuvreIndex=FirstManoeuvreAfter(collected.steps, nodeMeters);
        step=collected.steps[*manoeuvreIndex];
      }

      if (auto followerIndex=FirstManoeuvreAfter(collected.steps,
                                                 step.distanceFromStart.AsMeter())) {
        double gap=collected.steps[*followerIndex].distanceFromStart.AsMeter()-
                   step.distanceFromStart.AsMeter();

        if (gap>0.0 && gap<=NextNextWindowMeters) {
          follower=collected.steps[*followerIndex];
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

        step=collected.steps.back();
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
