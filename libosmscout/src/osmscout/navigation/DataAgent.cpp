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

#include <osmscout/navigation/DataAgent.h>

namespace osmscout {

  TypeConfigRef RoutableObjects::GetTypeConfig(const DatabaseId &dbId) const
  {
    auto objMap=dbMap.find(dbId);
    if (objMap==dbMap.end()){
      return {};
    }
    return objMap->second.typeConfig;
  }

  WayRef RoutableObjects::GetWay(const DatabaseId &dbId, const ObjectFileRef &objRef) const
  {
    if (!objRef.Valid()){
      return {};
    }
    if (objRef.GetType()!=RefType::refWay){
      return {};
    }
    auto objMap=dbMap.find(dbId);
    if (objMap==dbMap.end()){
      return {};
    }
    auto objEntry=objMap->second.ways.find(objRef.GetFileOffset());
    if (objEntry==objMap->second.ways.end()){
      return {};
    }
    return objEntry->second;
  }

  AreaRef RoutableObjects::GetArea(const DatabaseId &dbId, const ObjectFileRef &objRef) const
  {
    if (!objRef.Valid()){
      return {};
    }
    if (objRef.GetType()!=RefType::refWay){
      return {};
    }
    auto objMap=dbMap.find(dbId);
    if (objMap==dbMap.end()){
      return {};
    }
    auto objEntry=objMap->second.areas.find(objRef.GetFileOffset());
    if (objEntry==objMap->second.areas.end()){
      return {};
    }
    return objEntry->second;
  }

  RoutableObjectsRequestMessage::RoutableObjectsRequestMessage(const Timestamp& timestamp, const GeoBox &bbox):
      NavigationMessage(timestamp), bbox(bbox)
  {}

  RoutableObjectsMessage::RoutableObjectsMessage(const Timestamp& timestamp, const RoutableObjectsRef &data):
      NavigationMessage(timestamp), data(data)
  {}

  NoRoutableObjectsMessage::NoRoutableObjectsMessage(const Timestamp& timestamp):
      NavigationMessage(timestamp)
  {}
}
