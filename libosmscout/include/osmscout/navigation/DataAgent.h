#ifndef OSMSCOUT_NAVIGATION_DATA_AGENT_H
#define OSMSCOUT_NAVIGATION_DATA_AGENT_H

/*
 This source is part of the libosmscout library
 Copyright (C) 2018  Lukas Karas

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

#include <list>

namespace osmscout {

  struct OSMSCOUT_API RoutableObjects
  {
    std::map<FileOffset,WayRef>  ways;
    std::map<FileOffset,AreaRef> areas;
  };

  using RoutableObjectsRef=std::shared_ptr<RoutableObjects>;

  /**
   * Message to pass to the NavigationEngine with routable objects around current possition
   */
  struct OSMSCOUT_API RoutableObjectsUpdateMessage CLASS_FINAL : public NavigationMessage
  {
    std::map<DatabaseId,RoutableObjectsRef> data;

    RoutableObjectsUpdateMessage(const Timestamp& timestamp);
  };

  using RoutableObjectsUpdateMessageRef=std::shared_ptr<RoutableObjectsUpdateMessage>;

  template <typename DataLoader>
  class OSMSCOUT_API DataAgent CLASS_FINAL : public NavigationAgent
  {
  private:
    std::map<std::string,DatabaseId> databaseMapping;
    Vehicle vehicle{Vehicle::vehicleCar};
    DataLoader &dataLoader;

  public:
    explicit DataAgent(DataLoader &mapServiceProvider):
      dataLoader(mapServiceProvider)
    {

    }

    std::list<NavigationMessageRef> Process(const NavigationMessageRef& message) override
    {
      auto result=std::list<NavigationMessageRef>();

      if (dynamic_cast<RouteUpdateMessage*>(message.get())!=nullptr) {
        auto routeUpdateMessage = dynamic_cast<RouteUpdateMessage *>(message.get());
        databaseMapping.clear();
        // build reverse mapping
        for (auto &e:routeUpdateMessage->routeDescription->GetDatabaseMapping()){
          databaseMapping[e.second]=e.first;
        }
        vehicle=routeUpdateMessage->vehicle;
      } else if (dynamic_cast<GPSUpdateMessage*>(message.get())!=nullptr) {
        auto gpsUpdateMessage=dynamic_cast<GPSUpdateMessage*>(message.get());
        GeoBox box=GeoBox::BoxByCenterAndRadius(gpsUpdateMessage->currentPosition,Distance::Of<Meter>(200));

        auto msg=std::make_shared<RoutableObjectsUpdateMessage>(gpsUpdateMessage->timestamp);

        dataLoader.loadRoutableObjects(box,
            vehicle,
            databaseMapping,
            msg->data);

        result.push_back(msg);
      }
      return result;
    }
  };

}
#endif
