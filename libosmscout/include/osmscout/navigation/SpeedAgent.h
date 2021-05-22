#ifndef OSMSCOUT_NAVIGATION_SPEED_AGENT_H
#define OSMSCOUT_NAVIGATION_SPEED_AGENT_H

/*
 This source is part of the libosmscout library
 Copyright (C) 2019  Lukas Karas

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

#include <osmscout/navigation/Engine.h>
#include <osmscout/navigation/Agents.h>
#include <osmscout/navigation/PositionAgent.h>

namespace osmscout {

struct OSMSCOUT_API CurrentSpeedMessage CLASS_FINAL : public NavigationMessage
{
  double speed; // km/h

  CurrentSpeedMessage(const Timestamp& timestamp,
                      double speed);
};

struct OSMSCOUT_API MaxAllowedSpeedMessage CLASS_FINAL : public NavigationMessage
{
  double maxAllowedSpeed; // km/h
  bool defined;

  MaxAllowedSpeedMessage(const Timestamp& timestamp,
                         double maxAllowedSpeed,
                         bool defined);
};

class OSMSCOUT_API SpeedAgent CLASS_FINAL : public NavigationAgent
{
private:
  struct TrackSegment
  {
    Distance distance;
    Timestamp::duration duration{Timestamp::duration::zero()};

    inline TrackSegment(const Distance &distance, const Timestamp::duration &duration):
      distance(distance), duration(duration)
    {}
  };
  std::list<TrackSegment> segmentFifo; // buffer of segments for 5 seconds used for speed computation

  struct Position
  {
    GeoCoord coord;
    Timestamp time;

    inline operator bool() const
    {
      return time.time_since_epoch()!=Timestamp::duration::zero();
    }
  };
  Position lastPosition;

  double lastReportedMaxSpeed{-1};

public:
  explicit SpeedAgent() = default;
  std::list<NavigationMessageRef> Process(const NavigationMessageRef& message) override;
};
}

#endif //OSMSCOUT_NAVIGATION_SPEED_AGENT_H

