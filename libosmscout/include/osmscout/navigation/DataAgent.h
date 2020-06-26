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

  struct OSMSCOUT_API RoutableDBObjects
  {
    TypeConfigRef typeConfig;
    std::map<FileOffset,WayRef>  ways;
    std::map<FileOffset,AreaRef> areas;
  };

  struct OSMSCOUT_API RoutableObjects
  {
    std::map<DatabaseId, RoutableDBObjects> dbMap;
    GeoBox bbox;

    TypeConfigRef GetTypeConfig(const DatabaseId &dbId) const;

    WayRef GetWay(const DatabaseId &dbId, const ObjectFileRef &objRef) const;

    AreaRef GetArea(const DatabaseId &dbId, const ObjectFileRef &areaRef) const;
  };

  using RoutableObjectsRef=std::shared_ptr<RoutableObjects>;

  struct OSMSCOUT_API RoutableObjectsRequestMessage CLASS_FINAL : public NavigationMessage
  {
    GeoBox bbox;

    RoutableObjectsRequestMessage(const Timestamp& timestamp, const GeoBox &bbox);
  };

  /**
   * Message to pass to the NavigationEngine with routable objects around current possition
   */
  struct OSMSCOUT_API RoutableObjectsMessage CLASS_FINAL : public NavigationMessage
  {
    RoutableObjectsRef data;

    RoutableObjectsMessage(const Timestamp& timestamp, const RoutableObjectsRef &data);
  };

  using RoutableObjectsMessageRef=std::shared_ptr<RoutableObjectsMessage>;

  template <typename DataLoader>
  class DataAgent CLASS_FINAL : public NavigationAgent
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

      if (auto* routeUpdateMessage = dynamic_cast<RouteUpdateMessage *>(message.get());
          routeUpdateMessage != nullptr) {

        databaseMapping.clear();
        // build reverse mapping
        for (auto &e:routeUpdateMessage->routeDescription->GetDatabaseMapping()){
          databaseMapping[e.second]=e.first;
        }
        vehicle=routeUpdateMessage->vehicle;
      } else if (auto* requestMessage = dynamic_cast<RoutableObjectsRequestMessage*>(message.get());
                 requestMessage != nullptr) {

        if (databaseMapping.empty()){
          return result; // no route yet
        }

        if (GetSphericalDistance(requestMessage->bbox.GetMinCoord(),
                                 requestMessage->bbox.GetMaxCoord()) > Kilometers(2)){
          log.Warn() << "Requested routable data from huge region: " << requestMessage->bbox.GetDisplayText();
        }

        auto msg=std::make_shared<RoutableObjectsMessage>(requestMessage->timestamp, std::make_shared<RoutableObjects>());

        dataLoader.loadRoutableObjects(requestMessage->bbox,
            vehicle,
            databaseMapping,
            msg->data);

        // TODO: load all route objects too

        result.push_back(msg);
      }
      return result;
    }
  };

}
#endif
