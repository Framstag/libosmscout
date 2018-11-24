#ifndef OSMSCOUT_NAVIGATION_AGENTS_H
#define OSMSCOUT_NAVIGATION_AGENTS_H

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

#include <osmscout/LocationDescriptionService.h>

namespace osmscout {
  /**
   * Message to pass to the NavigationEngine each time gps state has changed.
   *
   * TODO: Pass information about the accuracy of the signal
   */
  struct OSMSCOUT_API GPSUpdateMessage : public NavigationMessage
  {
    const bool     hasPositionLock;
    const GeoCoord currentPosition;
    const double   currentSpeed;

    GPSUpdateMessage(const Timestamp& timestamp,
                     bool hasPositionLock,
                     const GeoCoord& currentPosition,
                     double currentSpeed);
  };

  struct OSMSCOUT_API PositionChangedMessage : public NavigationMessage
  {
    const bool     hasPositionLock;
    const GeoCoord currentPosition;
    const double   currentSpeed;

    PositionChangedMessage(const Timestamp& timestamp,
                           bool hasPositionLock,
                           const GeoCoord& currentPosition,
                           double currentSpeed);
  };

  class OSMSCOUT_API PositionAgent : public NavigationAgent
  {
  public:
    std::list<NavigationMessageRef> Process(const NavigationMessageRef& message) override;
  };

  struct OSMSCOUT_API StreetChangedMessage : public NavigationMessage
  {
    const std::string name;

    StreetChangedMessage(const Timestamp& timestamp,
                         const std::string& name);
  };

  class OSMSCOUT_API CurrentStreetAgent : public NavigationAgent
  {
  private:
    LocationDescriptionServiceRef locationDescriptionService;

    GeoCoord                      currentPosition;
    Timestamp                     lastCheckTime;
    std::string                   lastStreetName;

  private:
    std::string GetStreetName() const;

  public:
    CurrentStreetAgent(const LocationDescriptionServiceRef& locationDescriptionService);
    std::list<NavigationMessageRef> Process(const NavigationMessageRef& message) override;
  };
}

#endif
