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

#include <osmscout/navigation/ArrivalEstimateAgent.h>

namespace osmscout {

ArrivalEstimateMessage::ArrivalEstimateMessage(const Timestamp& timestamp,
                                               const Timestamp& arrivalEstimate,
                                               const Distance& remainingDistance):
    NavigationMessage(timestamp),
    arrivalEstimate(arrivalEstimate),
    remainingDistance(remainingDistance)
{

}

std::list<NavigationMessageRef> ArrivalEstimateAgent::Process(const NavigationMessageRef &message)
{
  std::list<NavigationMessageRef> result;

  const auto *possitionMsg = dynamic_cast<osmscout::PositionAgent::PositionMessage *>(message.get());
  if (possitionMsg==nullptr) {
    return result;
  }
  if (!possitionMsg->route && !possitionMsg->route->Nodes().empty()){
    return result;
  }
  if (possitionMsg->position.state==PositionAgent::OffRoute ||
      possitionMsg->position.state==PositionAgent::NoGpsSignal){
    return result;
  }
  auto routeNode = possitionMsg->position.routeNode;
  if (routeNode==possitionMsg->route->Nodes().end()){
    return result;
  }
  auto nextRouteNode = routeNode;
  nextRouteNode++;
  if (nextRouteNode==possitionMsg->route->Nodes().end()){
    return result;
  }
  auto lastNode = possitionMsg->route->Nodes().back();
  Distance distanceFromNode=GetSphericalDistance(routeNode->GetLocation(), possitionMsg->position.coord);
  Distance distanceBetweenNodes=GetSphericalDistance(routeNode->GetLocation(), nextRouteNode->GetLocation());
  if (distanceFromNode>distanceBetweenNodes){
    log.Warn() << "Distance from previous node (" << distanceFromNode << ") is greater than distance between nodes (" << distanceBetweenNodes << ")";
    return result;
  }

  Distance remainingDistance=(lastNode.GetDistance()-routeNode->GetDistance())-distanceFromNode;
  if (remainingDistance < Meters(0)){
    log.Warn() << "Remaining distance (" << remainingDistance << ") is negative!";
    return result;
  }

  Duration timeBetweenNodes = nextRouteNode->GetTime() - routeNode->GetTime();
  double fractionToNext = 1 - (distanceBetweenNodes==Distance::Zero() ? 1: (distanceFromNode.AsMeter() / distanceBetweenNodes.AsMeter()));
  Duration timeToNext = std::chrono::duration_cast<Duration>(timeBetweenNodes * fractionToNext);
  Duration timeFromNextToLast = lastNode.GetTime()-nextRouteNode->GetTime();
  Timestamp arrivalEstimate = possitionMsg->timestamp + timeFromNextToLast + timeToNext;

  result.push_back(std::make_shared<ArrivalEstimateMessage>(possitionMsg->timestamp, arrivalEstimate, remainingDistance));

  return result;
}

}
