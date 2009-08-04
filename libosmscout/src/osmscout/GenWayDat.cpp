/*
  TravelJinni - Openstreetmap offline viewer
  Copyright (C) 2009  Tim Teulings

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <osmscout/GenWayDat.h>

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>

#include <osmscout/Way.h>
#include <osmscout/RawRelation.h>
#include <osmscout/RawWay.h>

bool GenerateWayDat(const TypeConfig& typeConfig)
{
  //
  // Analysing distribution of nodes in the given interval size
  //

  std::cout << "Generate ways.dat..." << std::endl;

  std::ifstream                               in;
  std::ofstream                               out;
  TypeId                                      restrictionPosId;
  TypeId                                      restrictionNegId;
  std::map<Id,std::vector<Way::Restriction> > restrictions;

  restrictionPosId=typeConfig.GetRelationTypeId(tagRestriction,"only_straight_on");
  assert(restrictionPosId!=typeIgnore);

  restrictionNegId=typeConfig.GetRelationTypeId(tagRestriction,"no_straight_on");
  assert(restrictionNegId!=typeIgnore);

  std::cout << "Scanning for restriction relations..." << std::endl;

  in.open("rawrels.dat",std::ios::in|std::ios::binary);

  if (!in) {
    return false;
  }

  while (in) {
    RawRelation relation;

    relation.Read(in);

    if (in) {
      if (relation.type==restrictionPosId || relation.type==restrictionNegId) {
        Id               from;
        Way::Restriction restriction;

        restriction.members.resize(1,0);

        if (relation.type==restrictionPosId) {
          restriction.type=Way::rstrAllowTurn;
        }
        else if (relation.type==restrictionNegId) {
          restriction.type=Way::rstrForbitTurn;
        }
        else {
          continue;
        }

        for (size_t i=0; i<relation.members.size(); i++) {
          if (relation.members[i].type==RawRelation::memberWay &&
              relation.members[i].role=="from") {
            from=relation.members[i].id;
          }
          else if (relation.members[i].type==RawRelation::memberWay &&
                   relation.members[i].role=="to") {
            restriction.members[0]=relation.members[i].id;
          }
          else if (relation.members[i].type==RawRelation::memberNode &&
                   relation.members[i].role=="via") {
            restriction.members.push_back(relation.members[i].id);
          }
        }

        if (from!=0 &&
            restriction.members[1]!=0 &&
            restriction.members.size()>1) {
          restrictions[from].push_back(restriction);
        }
      }
    }
  }

  in.close();

  in.open("rawways.dat",std::ios::in|std::ios::binary);

  if (!in) {
    return false;
  }

  out.open("ways.dat",std::ios::out|std::ios::trunc|std::ios::binary);

  if (!out) {
    return false;
  }

  while (in) {
    RawWay rawWay;
    Way    way;

    rawWay.Read(in);

    if (rawWay.type!=typeIgnore) {
      int8_t layer=0;
      bool   isBridge=false;
      bool   isTunnel=false;
      bool   isBuilding=false;
      bool   isOneway=false;
      bool   reverseNodes=false;

      way.id=rawWay.id;
      way.type=rawWay.type;

      std::vector<Tag>::iterator tag=rawWay.tags.begin();
      while (tag!=rawWay.tags.end()) {
        if (tag->key==tagName) {
          way.name=tag->value;
          tag=rawWay.tags.erase(tag);
        }
        else if (tag->key==tagRef) {
          way.ref=tag->value;
          tag=rawWay.tags.erase(tag);
        }
        else if (tag->key==tagLayer) {
          if (sscanf(tag->value.c_str(),"%hhd",&layer)!=1) {
            std::cerr << "Layer tag value '" << tag->value << "' for way " << rawWay.id << " is not numeric!" << std::endl;
          }
          tag=rawWay.tags.erase(tag);
        }
        else if (tag->key==tagBridge) {
          isBridge=(tag->value=="yes" || tag->value=="true" || tag->value=="1") &&
                  !(tag->value=="no" || tag->value=="false" || tag->value=="0");
          tag=rawWay.tags.erase(tag);
        }
        else if (tag->key==tagTunnel) {
          isTunnel=(tag->value=="yes" || tag->value=="true" || tag->value=="1") &&
                  !(tag->value=="no" || tag->value=="false" || tag->value=="0");
          tag=rawWay.tags.erase(tag);
        }
        else if (tag->key==tagBuilding) {
          isBuilding=(tag->value=="yes" || tag->value=="true" || tag->value=="1") &&
                     !(tag->value=="no" || tag->value=="false" || tag->value=="0");

          tag=rawWay.tags.erase(tag);
        }
        else if (tag->key==tagOneway) {
          if (tag->value=="-1") {
            isOneway=true;
            reverseNodes=true;
          }
          else {
            isOneway=(tag->value=="yes" || tag->value=="true" || tag->value=="1") &&
                    !(tag->value=="no" || tag->value=="false" || tag->value=="0");
          }

          tag=rawWay.tags.erase(tag);
        }
        else {
          ++tag;
        }
      }

      if (reverseNodes) {
        std::reverse(way.nodes.begin(),way.nodes.end());
      }

      if (rawWay.IsArea()) {
        way.flags|=Way::isArea;
      }

      if (!way.name.empty()) {
        way.flags|=Way::hasName;
      }

      if (!way.ref.empty()) {
        way.flags|=Way::hasRef;
      }

      if (layer!=0) {
        way.flags|=Way::hasLayer;
        way.layer=layer;
      }

      if (isBridge) {
        way.flags|=Way::isBridge;
      }

      if (isTunnel) {
        way.flags|=Way::isTunnel;
      }

      if (isBuilding) {
        way.flags|=Way::isBuilding;
      }

      if (isOneway) {
        way.flags|=Way::isOneway;
      }

      way.tags=rawWay.tags;
      if (way.tags.size()>0) {
        way.flags|=Way::hasTags;
      }

      way.nodes.resize(rawWay.nodes.size());
      for (size_t i=0; i<rawWay.nodes.size(); i++) {
        way.nodes[i].id=rawWay.nodes[i];
      }

      std::map<Id,std::vector<Way::Restriction> >::iterator iter=restrictions.find(way.id);

      if (iter!=restrictions.end()) {
        way.flags|=Way::hasRestrictions;

        way.restrictions=iter->second;
      }

      way.Write(out);
    }
  }

  in.close();
  out.close();

  // Cleaning up...

  restrictions.clear();

  return true;
}

