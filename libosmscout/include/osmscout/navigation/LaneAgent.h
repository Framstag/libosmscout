#ifndef OSMSCOUT_LANE_AGENT_H
#define OSMSCOUT_LANE_AGENT_H

/*
 This source is part of the libosmscout library
 Copyright (C) 2020  Lukas Karas

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

class OSMSCOUT_API LaneAgent CLASS_FINAL : public NavigationAgent
{
public:
  /**
   * @see RouteDescription::LaneDescription and RouteDescription::SuggestedLaneDescription for details
   */
  struct OSMSCOUT_API Lane
  {
    bool oneway{false};
    int count{1};
    bool suggested{false};
    int suggestedFrom{0};
    int suggestedTo{0};
    std::vector<LaneTurn> turns;

    Lane() = default;

    Lane(const RouteDescription::LaneDescriptionRef laneDsc,
         const RouteDescription::SuggestedLaneDescriptionRef suggestedLaneDsc);

    Lane(const Lane &) = default;
    Lane(Lane &&) = default;

    ~Lane() = default;

    Lane &operator=(const Lane&) = default;
    Lane &operator=(Lane&&) = default;

    bool operator!=(const Lane &o) const;
  };

  struct OSMSCOUT_API LaneMessage CLASS_FINAL : public NavigationMessage
  {
    Lane lane;

    LaneMessage(const Timestamp& timestamp,
                const Lane &lane);

  };

public:
  LaneAgent() = default;

  std::list<NavigationMessageRef> Process(const NavigationMessageRef& message) override;

private:
  Lane lastLane;
};

}

#endif // OSMSCOUT_LANE_AGENT_H
