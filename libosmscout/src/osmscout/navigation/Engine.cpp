/*
  This source is part of the libosmscout library
  Copyright (C) 2018  Tim Teulings

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

namespace osmscout {

  NavigationMessage::NavigationMessage(const Timestamp& timestamp)
  : timestamp(timestamp)
  {
  }

  NavigationMessage::~NavigationMessage()
  {
  }


  NavigationAgent::~NavigationAgent()
  {
  }

  InitializeMessage::InitializeMessage(const osmscout::Timestamp& timestamp)
    : NavigationMessage(timestamp)
  {
  }

  TimeTickMessage::TimeTickMessage(const osmscout::Timestamp& timestamp)
  : NavigationMessage(timestamp)
  {
  }

  NavigationEngine::NavigationEngine(std::initializer_list<NavigationAgentRef> agents)
  : agents(agents)
  {
  }

  std::list<NavigationMessageRef> NavigationEngine::Process(const NavigationMessageRef& message)
  {
    std::list<NavigationMessageRef> messageQueue{message};
    std::list<NavigationMessageRef> result;

    while (!messageQueue.empty()) {
      auto messageToProcess=messageQueue.front();

      messageQueue.pop_front();

      for (const auto& agent : agents) {
        auto resultMessages=agent->Process(messageToProcess);

        messageQueue.insert(messageQueue.end(),
                            resultMessages.begin(),
                            resultMessages.end());

        result.insert(result.end(),
                      resultMessages.begin(),
                      resultMessages.end());
      }
    }

    return result;
  }
}
