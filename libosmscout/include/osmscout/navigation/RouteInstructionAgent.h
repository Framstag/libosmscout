#ifndef LIBOSMSCOUT_ROUTEINSTRUCTIONAGENT_H
#define LIBOSMSCOUT_ROUTEINSTRUCTIONAGENT_H

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
#include <osmscout/navigation/DataAgent.h>

namespace osmscout {

template <typename RouteInstruction>
struct RouteInstructionsMessage CLASS_FINAL : public NavigationMessage
{
public:
  std::list<RouteInstruction> instructions;

  inline RouteInstructionsMessage(const Timestamp& timestamp, const std::list<RouteInstruction> &instructions):
    NavigationMessage(timestamp), instructions(instructions)
  {}
};

template <typename RouteInstruction>
struct NextRouteInstructionsMessage CLASS_FINAL : public NavigationMessage
{
public:
  RouteInstruction nextRouteInstruction;

  inline NextRouteInstructionsMessage(const Timestamp& timestamp, const RouteInstruction &nextRouteInstruction):
      NavigationMessage(timestamp), nextRouteInstruction(nextRouteInstruction)
  {}
};

template <typename RouteInstruction, typename RouteInstructionBuilder>
class RouteInstructionAgent CLASS_FINAL : public NavigationAgent
{
private:
  RouteDescriptionRef prevRoute;
  std::list<RouteInstruction> instructions;

public:
  RouteInstructionAgent();

  std::list<NavigationMessageRef> Process(const NavigationMessageRef& message) override;

};

template <typename RouteInstruction, typename RouteInstructionBuilder>
RouteInstructionAgent<RouteInstruction, RouteInstructionBuilder>::RouteInstructionAgent() = default;

template <typename RouteInstruction, typename RouteInstructionBuilder>
std::list<NavigationMessageRef> RouteInstructionAgent<RouteInstruction, RouteInstructionBuilder>
    ::Process(const NavigationMessageRef& message)
{
  std::list<NavigationMessageRef> result;

  auto* positionMessage = dynamic_cast<PositionAgent::PositionMessage*>(message.get());
  if (positionMessage==nullptr) {
    return result;
  }

  if (!positionMessage->route){
    // we don't have route description yet
    return result;
  }
  if (positionMessage->position.state != PositionAgent::PositionState::OnRoute &&
      positionMessage->position.state != PositionAgent::PositionState::EstimateInTunnel) {
    // what route instruction we should show?
    return result;
  }

  RouteInstructionBuilder builder;

  Timestamp now = positionMessage->timestamp;
  if (prevRoute != positionMessage->route){
    instructions=builder.GenerateRouteInstructions(positionMessage->position.routeNode,
                                                   positionMessage->route->Nodes().end());
    result.push_back(std::make_shared<RouteInstructionsMessage<RouteInstruction>>(now,instructions));
  }

  // remove instructions behind our back (pop from the front of the list)
  bool updated=false;
  while (!instructions.empty() &&
         positionMessage->position.routeNode != positionMessage->route->Nodes().end() &&
         instructions.front().GetDistance() <= positionMessage->position.routeNode->GetDistance()){

    instructions.pop_front();
    updated=true;
  }
  if (updated){
    result.push_back(std::make_shared<RouteInstructionsMessage<RouteInstruction>>(now,instructions));
  }

  // next route instruction
  RouteInstruction nextInstruction = builder.GenerateNextRouteInstruction(positionMessage->position.routeNode,
                                                                          positionMessage->route->Nodes().end(),
                                                                          positionMessage->position.coord);
  result.push_back(std::make_shared<NextRouteInstructionsMessage<RouteInstruction>>(now,nextInstruction));

  prevRoute=positionMessage->route;

  return result;
}

}

#endif //LIBOSMSCOUT_ROUTEINSTRUCTIONAGENT_H
