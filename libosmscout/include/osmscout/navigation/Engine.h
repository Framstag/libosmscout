#ifndef OSMSCOUT_NAVIGATION_ENGINE_H
#define OSMSCOUT_NAVIGATION_ENGINE_H

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

#include <list>
#include <vector>

#include <osmscout/GeoCoord.h>

#include <osmscout/util/String.h>

namespace osmscout {

  /**
   * \defgroup NavigationEngine
   *
   * Collection of classes and methods to create a navigation engine.
   */

  /**
   * Base class for all navigation messages.
   *
   * Messages that are send to the engine should named in presense tense (e.g. "GPSUpdate").
   * Messages that signal a change in the internal state should  use past tense (e.g. "PositionChanged").
   *
   * This way NavigationEngine#Process() should only get passed present tense named messages and
   * should only return past tense named messages.
   *
   */
  struct OSMSCOUT_API NavigationMessage
  {
    const Timestamp timestamp;

    explicit NavigationMessage(const Timestamp& timestamp);
    virtual ~NavigationMessage() = default;
  };

  using NavigationMessageRef = std::shared_ptr<NavigationMessage>;

  /**
   * Message send once at the beginning to make sure everything initializes correctly
   * and to make it possible that agents can send messages on initialization.
   */
  struct OSMSCOUT_API InitializeMessage CLASS_FINAL : public NavigationMessage
  {
    explicit InitializeMessage(const Timestamp& timestamp);
  };

  /**
   * Message to pass periodically to the Engine to make sure that state changes based on
   * timeouts are triggered.
   *
   * We recommend to send a TimeTickMessage around every second.
   */
  struct OSMSCOUT_API TimeTickMessage CLASS_FINAL : public NavigationMessage
  {
    explicit TimeTickMessage(const Timestamp& timestamp);
  };

  class OSMSCOUT_API NavigationAgent
  {
  public:
    virtual ~NavigationAgent() = default;

    virtual std::list<NavigationMessageRef> Process(const NavigationMessageRef& message) = 0;
  };

  using NavigationAgentRef = std::shared_ptr<NavigationAgent>;

  class OSMSCOUT_API NavigationEngine
  {
  private:
    std::vector<NavigationAgentRef> agents;

  public:
    explicit NavigationEngine(std::initializer_list<NavigationAgentRef> agents);
    std::list<NavigationMessageRef> Process(const NavigationMessageRef& message) const;
  };
}

#endif
