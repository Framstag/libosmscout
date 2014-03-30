/*
  This source is part of the libosmscout library
  Copyright (C) 2013  Tim Teulings

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

#include <osmscout/AttributeAccess.h>

namespace osmscout {

  void AttributeAccess::Parse(Progress& /*progress*/,
                              const TypeConfig& typeConfig,
                              TypeId type,
                              Id /*id*/,
                              std::vector<Tag>& tags)
  {
    access=0;

    TypeInfo typeInfo=typeConfig.GetTypeInfo(type);

    if (typeInfo.CanRouteFoot()) {
      access|=(footForward|footBackward);
    }

    if (typeInfo.CanRouteBicycle()) {
      access|=(bicycleForward|bicycleBackward);
    }

    if (typeInfo.CanRouteCar()) {
      access|=(carForward|carBackward);
    }

    std::vector<Tag>::iterator tag;

    // Flag access

    tag=tags.begin();
    while (tag!=tags.end()) {
      if (tag->key==typeConfig.tagAccess) {
        access=0;

        if (!(tag->value=="no")) {
          access=(footForward|footBackward|bicycleForward|bicycleBackward|carForward|carBackward);
        }

        tag=tags.erase(tag);
      }
      else {
        ++tag;
      }
    }

    // Flag access:forward/access:backward

    tag=tags.begin();
    while (tag!=tags.end()) {
      if (tag->key==typeConfig.tagAccessForward) {
        access&=~(footForward|bicycleForward|carForward);

        if (!(tag->value=="no")) {
          access|=(footForward|bicycleForward|carForward);
        }

        tag=tags.erase(tag);
      }
      else if (tag->key==typeConfig.tagAccessBackward) {
        access&=~(footBackward|bicycleBackward|carBackward);

        if (!(tag->value=="no")) {
          access|=(footBackward|bicycleBackward|carBackward);
        }

        tag=tags.erase(tag);
      }
      else {
        ++tag;
      }
    }

    // Flags access:foot, access:bicycle, access:motor_vehicle, access:motorcar

    tag=tags.begin();
    while (tag!=tags.end()) {
      if (tag->key==typeConfig.tagAccessFoot) {
        access&=~(footForward|footBackward);
        if (!(tag->value=="no")) {
          access|=(footForward|footBackward);
        }

        tag=tags.erase(tag);
      }
      else if (tag->key==typeConfig.tagAccessBicycle) {
        access&=~(bicycleForward|bicycleBackward);

        if (!(tag->value=="no")) {
          if (!access & onewayBackward) {
            access|=(bicycleForward);
          }
          if (!access & onewayForward) {
            access|=(bicycleBackward);
          }
        }

        tag=tags.erase(tag);
      }
      else if (tag->key==typeConfig.tagAccessMotorVehicle ||
               tag->key==typeConfig.tagAccessMotorcar) {
        access&=~(carForward|carBackward);

        if (!(tag->value=="no")) {
          if (!access & onewayBackward) {
            access|=(carForward);
          }
          if (!access & onewayForward) {
            access|=(carBackward);
          }
        }

        tag=tags.erase(tag);
      }
      else {
        ++tag;
      }
    }

    // Flags access:foot::forward/access:foot::backward,
    //       access:bicycle::forward/access:bicycle::backward,
    //       access:motor_vehicle::forward/access:motor_vehicle::backward,
    //       access:motorcar::forward/access:motorcar::backward

    tag=tags.begin();
    while (tag!=tags.end()) {
      if (tag->key==typeConfig.tagAccessFootForward) {
        access&=~(footForward);

        if (!(tag->value=="no")) {
          access|=(footForward);
        }

        tag=tags.erase(tag);
      }
      else if (tag->key==typeConfig.tagAccessFootBackward) {
        access&=~(footBackward);

        if (!(tag->value=="no")) {
          access|=(footBackward);
        }

        tag=tags.erase(tag);
      }
      else if (tag->key==typeConfig.tagAccessBicycleForward) {
        access&=~(bicycleForward);

        if (!(tag->value=="no")) {
          access|=(bicycleForward);
        }

        tag=tags.erase(tag);
      }
      else if (tag->key==typeConfig.tagAccessBicycleBackward) {
        access&=~(bicycleBackward);

        if (!(tag->value=="no")) {
          access|=(bicycleBackward);
        }

        tag=tags.erase(tag);
      }
      else if (tag->key==typeConfig.tagAccessMotorVehicleForward ||
               tag->key==typeConfig.tagAccessMotorcarForward) {
        access&=~(carForward);

        if (!(tag->value=="no")) {
          access|=(carForward);
        }

        tag=tags.erase(tag);
      }
      else if (tag->key==typeConfig.tagAccessMotorVehicleBackward ||
               tag->key==typeConfig.tagAccessMotorcarBackward) {
        access&=~(carBackward);

        if (!(tag->value=="no")) {
          access|=(carBackward);
        }

        tag=tags.erase(tag);
      }
      else {
        ++tag;
      }
    }

    tag=tags.begin();
    while (tag!=tags.end()) {
      if (tag->key==typeConfig.tagOneway) {
        if (tag->value=="-1") {
          access&=~(bicycleForward|carForward|onewayForward);
          access|=onewayBackward;
        }
        else if (!(tag->value=="no" || tag->value=="false" || tag->value=="0")) {
          access&=~(bicycleBackward|carBackward|onewayBackward);
          access|=onewayForward;
        }

        tag=tags.erase(tag);
      }
      else if (tag->key==typeConfig.tagJunction) {
        if (tag->value=="roundabout") {
          access&=~(bicycleBackward|carBackward|onewayBackward);
          access|=(bicycleForward|carForward|onewayForward);
        }

        tag=tags.erase(tag);
      }
      else {
        ++tag;
      }
    }
  }

  bool AttributeAccess::Read(FileScanner& scanner)
  {
    return scanner.Read(access);
  }

  bool AttributeAccess::Write(FileWriter& writer) const
  {
    return writer.Write(access);
  }

  bool AttributeAccess::operator==(const AttributeAccess& other) const
  {
    return access==other.access;
  }

  bool AttributeAccess::operator!=(const AttributeAccess& other) const
  {
    return access!=other.access;
  }
}
