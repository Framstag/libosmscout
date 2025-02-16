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

#include <osmscout/navigation/LaneAgent.h>

namespace osmscout{

LaneAgent::LaneMessage::LaneMessage(const Timestamp& timestamp,
                                    const LaneAgent::Lane &lane):
    NavigationMessage(timestamp),
    lane(lane)
{}

LaneAgent::Lane::Lane(const RouteDescription::LaneDescriptionRef laneDsc,
                      const RouteDescription::SuggestedLaneDescriptionRef suggestedLaneDsc)
{
  if (laneDsc){
    oneway = laneDsc->IsOneway();
    count = laneDsc->GetLaneCount();
    turns = laneDsc->GetLaneTurns();
  }
  if (suggestedLaneDsc){
    suggested = true;
    suggestedFrom = suggestedLaneDsc->GetFrom();
    suggestedTo = suggestedLaneDsc->GetTo();
    turn = suggestedLaneDsc->GetTurn();
  } else {
    suggestedTo = count-1;
    turn = LaneTurn::None;
  }
}

bool LaneAgent::Lane::operator!=(const LaneAgent::Lane &o) const
{
  return oneway!=o.oneway ||
         count!=o.count ||
         suggested!=o.suggested ||
         suggestedFrom!=o.suggestedFrom ||
         suggestedTo!=o.suggestedTo ||
         turns!=o.turns ||
         turn!=o.turn;
}

std::list<NavigationMessageRef> LaneAgent::Process(const NavigationMessageRef& message)
{
  std::list<NavigationMessageRef> result;

  auto positionMsg = dynamic_cast<osmscout::PositionAgent::PositionMessage *>(message.get());
  if (positionMsg){

    Lane updated(positionMsg->GetRouteDescription<RouteDescription::LaneDescription>(RouteDescription::LANES_DESC),
                 positionMsg->GetRouteDescription<RouteDescription::SuggestedLaneDescription>(RouteDescription::SUGGESTED_LANES_DESC));

    if (lastLane != updated) {
      lastLane=updated;
      result.push_back(std::make_shared<LaneMessage>(positionMsg->timestamp,updated));
    }
  }

  return result;
}

}
