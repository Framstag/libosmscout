/*
  This source is part of the libosmscout library
  Copyright (C) 2009  Tim Teulings

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

#include <osmscout/import/Preprocess.h>

#include <limits>

#include <osmscout/system/Math.h>

#include <osmscout/util/File.h>
#include <osmscout/util/String.h>

#include <osmscout/import/RawCoastline.h>
#include <osmscout/import/RawNode.h>
#include <osmscout/import/RawWay.h>

#include <osmscout/private/Config.h>

#if defined(HAVE_LIB_XML)
  #include <osmscout/import/PreprocessOSM.h>
#endif

#if defined(HAVE_LIB_PROTOBUF)
  #include <osmscout/import/PreprocessPBF.h>
#endif

#include <iostream>
namespace osmscout {

  static uint32_t coordPageSize=64;

  bool Preprocess::StoreCurrentPage()
  {
    if (!coordWriter.SetPos(currentPageOffset)) {
      return false;
    }

    for (size_t i=0; i<coordPageSize; i++) {

      if (!isSet[i]) {
        uint32_t latValue=0xffffffff;
        uint32_t lonValue=0xffffffff;

        coordWriter.Write(latValue);
        coordWriter.Write(lonValue);
      }
      else {
        coordWriter.WriteCoord(lats[i],lons[i]);
      }
    }

    currentPageId=std::numeric_limits<PageId>::max();

    return !coordWriter.HasError();
  }

  bool Preprocess::StoreCoord(OSMId id,
                              double lat,
                              double lon)
  {
    PageId relatedId=id-std::numeric_limits<Id>::min();
    PageId pageId=relatedId/coordPageSize;
    FileOffset coordPageOffset=relatedId%coordPageSize;

    if (currentPageId!=std::numeric_limits<PageId>::max()) {
      if (currentPageId==pageId) {
        lats[coordPageOffset]=lat;
        lons[coordPageOffset]=lon;
        isSet[coordPageOffset]=true;

        return true;
      }
      else {
        StoreCurrentPage();
      }
    }

    CoordPageOffsetMap::const_iterator pageOffsetEntry=coordIndex.find(pageId);

    if (pageOffsetEntry==coordIndex.end()) {
      isSet.assign(coordPageSize,false);

      lats[coordPageOffset]=lat;
      lons[coordPageOffset]=lon;
      isSet[coordPageOffset]=true;

      FileOffset pageOffset=coordPageCount*coordPageSize*2*sizeof(uint32_t);

      currentPageId=pageId;
      currentPageOffset=pageOffset;

      pageOffsetEntry=coordIndex.insert(std::make_pair(pageId,pageOffset)).first;

      coordPageCount++;

      return true;
    }

    if (!coordWriter.SetPos(pageOffsetEntry->second+coordPageOffset*2*sizeof(uint32_t))) {
      return false;
    }

    return coordWriter.WriteCoord(lat,lon);
  }

  bool Preprocess::IsTurnRestriction(const TypeConfig& typeConfig,
                                     const std::map<TagId,std::string>& tags,
                                     TurnRestriction::Type& type) const
  {
    bool isRestriction=false;
    bool isTurnRestriction=false;

    type=TurnRestriction::Allow;

    for (std::map<TagId,std::string>::const_iterator tag=tags.begin();
        tag!=tags.end();
        tag++) {
      if (tag->first==typeConfig.tagType) {
        if (tag->second=="restriction") {
          isRestriction=true;
        }
      }
      else if (tag->first==typeConfig.tagRestriction) {
        if (tag->second=="only_left_turn" ||
            tag->second=="only_right_turn" ||
            tag->second=="only_straight_on") {
          isTurnRestriction=true;
          type=TurnRestriction::Allow;
        }
        else if (tag->second=="no_left_turn" ||
                 tag->second=="no_right_turn" ||
                 tag->second=="no_straight_on" ||
                 tag->second=="no_u_turn") {
          isTurnRestriction=true;
          type=TurnRestriction::Forbit;
        }
      }

      // finished collection data
      if (isRestriction &&
          isTurnRestriction) {
        break;
      }
    }

    return isRestriction &&
           isTurnRestriction;
  }

  void Preprocess::ProcessTurnRestriction(const std::vector<RawRelation::Member>& members,
                                          TurnRestriction::Type type)
  {
    Id from=0;
    Id via=0;
    Id to=0;

    for (std::vector<RawRelation::Member>::const_iterator member=members.begin();
         member!=members.end();
         ++member) {
      if (member->type==RawRelation::memberWay &&
          member->role=="from") {
        from=member->id;
      }
      else if (member->type==RawRelation::memberNode &&
               member->role=="via") {
        via=member->id;
      }
      else if (member->type==RawRelation::memberWay &&
               member->role=="to") {
        to=member->id;
      }

      // finished collection data
      if (from!=0 &&
          via!=0 &&
          to!=0) {
        break;
      }
    }

    if (from!=0 &&
        via!=0 &&
        to!=0) {
      TurnRestriction restriction(type,
                                  from,
                                  via,
                                  to);

      restriction.Write(turnRestrictionWriter);
      turnRestrictionCount++;
    }
  }

  bool Preprocess::IsMultipolygon(const TypeConfig& typeConfig,
                                  const std::map<TagId,std::string>& tags,
                                  TypeId& type)
  {
    typeConfig.GetRelationTypeId(tags,type);

    if (type!=typeIgnore &&
        typeConfig.GetTypeInfo(type).GetIgnore()) {
      return false;
    }

    bool isArea=type!=typeIgnore &&
                typeConfig.GetTypeInfo(type).GetMultipolygon();

    if (!isArea) {
      std::map<TagId,std::string>::const_iterator typeTag=tags.find(typeConfig.tagType);

      isArea=typeTag!=tags.end() && typeTag->second=="multipolygon";
    }

    return isArea;
  }

  void Preprocess::ProcessMultipolygon(const TypeConfig& typeConfig,
                                       const std::map<TagId,std::string>& tags,
                                       const std::vector<RawRelation::Member>& members,
                                       OSMId id,
                                       TypeId type)
  {
    RawRelation relation;

    relation.SetId(id);
    relation.SetType(type);
    typeConfig.ResolveTags(tags,
                           relation.tags);

    relation.members=members;

    relation.Write(multipolygonWriter);

    multipolygonCount++;
  }


  std::string Preprocess::GetDescription() const
  {
    return "Preprocess";
  }

  bool Preprocess::Import(const ImportParameter& parameter,
                          Progress& progress,
                          const TypeConfig& typeConfig)
  {
    if (parameter.GetMapfile().length()>=4 &&
        parameter.GetMapfile().substr(parameter.GetMapfile().length()-4)==".osm")  {

#if defined(HAVE_LIB_XML)
      PreprocessOSM preprocess;

      return preprocess.Import(parameter,
                               progress,
                               typeConfig);
#else
      progress.Error("Support for the OSM file format is not enabled!");
#endif
    }

    if (parameter.GetMapfile().length()>=4 &&
             parameter.GetMapfile().substr(parameter.GetMapfile().length()-4)==".pbf") {

#if defined(HAVE_LIB_PROTOBUF)
      PreprocessPBF preprocess;

      return preprocess.Import(parameter,
                               progress,
                               typeConfig);
#else
      progress.Error("Support for the PBF file format is not enabled!");
      return false;
#endif
    }

    progress.Error("Sorry, this file type is not yet supported!");
    return false;
  }

  bool Preprocess::Initialize(const ImportParameter& parameter,
                              Progress& progress)
  {
    // This is something I do not like
    this->progress=&progress;

    coordPageCount=0;
    currentPageId=std::numeric_limits<PageId>::max();

    nodeCount=0;
    wayCount=0;
    areaCount=0;
    relationCount=0;
    coastlineCount=0;
    turnRestrictionCount=0;
    multipolygonCount=0;

    lastNodeId=std::numeric_limits<OSMId>::min();
    lastWayId=std::numeric_limits<OSMId>::min();
    lastRelationId=std::numeric_limits<OSMId>::min();

    nodeSortingError=false;
    waySortingError=false;
    relationSortingError=false;

    nodeWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                    "rawnodes.dat"));
    nodeWriter.Write(nodeCount);

    wayWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   "rawways.dat"));
    wayWriter.Write(wayCount+areaCount);

    coastlineWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                        "rawcoastline.dat"));
    coastlineWriter.Write(coastlineCount);

    turnRestrictionWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                        "rawturnrestr.dat"));
    turnRestrictionWriter.Write(turnRestrictionCount);

    multipolygonWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                        "rawrels.dat"));
    multipolygonWriter.Write(multipolygonCount);

    coordWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "coord.dat"));

    FileOffset offset=0;

    coordWriter.Write(coordPageSize);
    coordWriter.Write(offset);
    coordWriter.FlushCurrentBlockWithZeros(coordPageSize*2*sizeof(uint32_t));

    coordPageCount++;

    lats.resize(coordPageSize);
    lons.resize(coordPageSize);
    isSet.resize(coordPageSize);

    return !nodeWriter.HasError() &&
           !wayWriter.HasError() &&
           !coastlineWriter.HasError() &&
           !turnRestrictionWriter.HasError() &&
           !multipolygonWriter.HasError() &&
           !coordWriter.HasError();
  }

  void Preprocess::ProcessNode(const TypeConfig& typeConfig,
                               const OSMId& id,
                               const double& lon,
                               const double& lat,
                               const std::map<TagId,std::string>& tagMap)
  {
    RawNode    node;
    TypeId     type=typeIgnore;
    FileOffset nodeOffset;

    if (id<lastNodeId) {
      nodeSortingError=true;
    }

    typeConfig.GetNodeTypeId(tagMap,type);

    if (type!=typeIgnore) {
      typeConfig.ResolveTags(tagMap,tags);

      nodeWriter.GetPos(nodeOffset);

      node.SetId(id);
      node.SetType(type);
      node.SetCoords(lon,lat);
      node.SetTags(tags);

      node.Write(nodeWriter);

      nodeCount++;
    }
    else {
      nodeOffset=0;
    }

    StoreCoord(id,
               lat,
               lon);

    lastNodeId=id;
  }

  void Preprocess::ProcessWay(const TypeConfig& typeConfig,
                              const OSMId& id,
                              std::vector<OSMId>& nodes,
                              const std::map<TagId,std::string>& tagMap)
  {
    TypeId                                      areaType=typeIgnore;
    TypeId                                      wayType=typeIgnore;
    int                                         isArea=0; // 0==unknown, 1==true, -1==false
    bool                                        isCoastlineArea=false;
    std::map<TagId,std::string>::const_iterator areaTag;
    std::map<TagId,std::string>::const_iterator naturalTag;
    RawWay                                      way;
    bool                                        isCoastline=false;

    if (nodes.size()<2) {
      progress->Warning("Way "+
                        NumberToString(id)+
                        " has less than two nodes!");
      return;
    }

    if (id<lastWayId) {
      waySortingError=true;
    }

    way.SetId(id);

    areaTag=tagMap.find(typeConfig.tagArea);

    if (areaTag==tagMap.end()) {
      isArea=0;
    }
    else if (areaTag->second=="no" ||
             areaTag->second=="false" ||
             areaTag->second=="0") {
      isArea=-1;
    }
    else {
      isArea=1;
    }

    naturalTag=tagMap.find(typeConfig.tagNatural);

    if (naturalTag!=tagMap.end() &&
        naturalTag->second=="coastline") {
      isCoastline=true;
    }

    if (isCoastline) {
      isCoastlineArea=nodes.size()>3 &&
                      (nodes.front()==nodes.back() ||
                       isArea==1);
    }

    typeConfig.GetWayAreaTypeId(tagMap,wayType,areaType);
    typeConfig.ResolveTags(tagMap,tags);

    if (isArea==1 &&
        areaType==typeIgnore) {
      isArea=0;
    }
    else if (isArea==-1 &&
             wayType==typeIgnore) {
      isArea=0;
    }

    if (isArea==0) {
      if (wayType!=typeIgnore &&
          areaType==typeIgnore) {
        isArea=-1;
      }
      else if (wayType==typeIgnore &&
               areaType!=typeIgnore) {
        isArea=1;
      }
      else if (wayType!=typeIgnore &&
               areaType!=typeIgnore) {
        if (nodes.size()>3 &&
            nodes.front()==nodes.back()) {
          if (typeConfig.GetTypeInfo(wayType).GetPinWay()) {
            isArea=-1;
          }
          else {
            isArea=1;
          }
        }
        else {
          isArea=-1;
        }
      }
    }

    switch (isArea) {
    case 1:
      way.SetType(areaType,true);

      if (nodes.size()>3 &&
          nodes.front()==nodes.back()) {
        nodes.pop_back();
      }

      areaCount++;
      break;
    case -1:
      way.SetType(wayType,false);
      wayCount++;
      break;
    default:
      if (nodes.size()>3 &&
          nodes.front()==nodes.back()) {
        way.SetType(typeIgnore,
                    true);

        nodes.pop_back();

        areaCount++;
      }
      else {
        way.SetType(typeIgnore,
                    false);
        wayCount++;
      }
    }

    way.SetNodes(nodes);
    way.SetTags(tags);

    way.Write(wayWriter);

    lastWayId=id;

    if (isCoastline) {
      RawCoastline coastline;

      coastline.SetId(way.GetId());
      coastline.SetType(isCoastlineArea);
      coastline.SetNodes(way.GetNodes());

      coastline.Write(coastlineWriter);

      coastlineCount++;
    }
  }

  void Preprocess::ProcessRelation(const TypeConfig& typeConfig,
                                   const OSMId& id,
                                   const std::vector<RawRelation::Member>& members,
                                   const std::map<TagId,std::string>& tagMap)
  {
    if (id<lastRelationId) {
      relationSortingError=true;
    }

    if (members.empty()) {
      progress->Warning("Relation "+
                        NumberToString(id)+
                        " does not have any members!");
      return;
    }

    TurnRestriction::Type turnRestrictionType;

    if (IsTurnRestriction(typeConfig,
                          tagMap,
                          turnRestrictionType)) {
      ProcessTurnRestriction(members,
                             turnRestrictionType);
    }

    TypeId multipolygonType;

    if (IsMultipolygon(typeConfig,
                       tagMap,
                       multipolygonType)) {
      ProcessMultipolygon(typeConfig,
                          tagMap,
                          members,
                          id,
                          multipolygonType);
    }

    /*
    RawRelation relation;
    TypeId      type;

    relation.SetId(id);
    relation.members=members;

    typeConfig.GetRelationTypeId(tagMap,type);
    typeConfig.ResolveTags(tagMap,relation.tags);

    relation.SetType(type);

    relation.Write(relationWriter);*/

    relationCount++;
    lastRelationId=id;
  }

  bool Preprocess::Cleanup(Progress& progress)
  {
    //Since I do not like take a pointer to a reference
    // I at least try to assure, that we do not misuse it.
    this->progress=NULL;

    if (currentPageId!=0) {
      StoreCurrentPage();
    }

    nodeWriter.SetPos(0);
    nodeWriter.Write(nodeCount);

    wayWriter.SetPos(0);
    wayWriter.Write(wayCount+areaCount);

    coastlineWriter.SetPos(0);
    coastlineWriter.Write(coastlineCount);

    turnRestrictionWriter.SetPos(0);
    turnRestrictionWriter.Write(turnRestrictionCount);

    multipolygonWriter.SetPos(0);
    multipolygonWriter.Write(multipolygonCount);

    coordWriter.SetPos(0);

    coordWriter.Write(coordPageSize);

    FileOffset coordIndexOffset=coordPageCount*coordPageSize*2*sizeof(uint32_t);

    coordWriter.Write(coordIndexOffset);

    coordWriter.SetPos(coordIndexOffset);
    coordWriter.Write((uint32_t)coordIndex.size());

    for (CoordPageOffsetMap::const_iterator entry=coordIndex.begin();
         entry!=coordIndex.end();
         ++entry) {
      coordWriter.Write(entry->first);
      coordWriter.Write(entry->second);
    }

    nodeWriter.Close();
    wayWriter.Close();
    coastlineWriter.Close();
    coordWriter.Close();
    turnRestrictionWriter.Close();
    multipolygonWriter.Close();

    progress.Info(std::string("Nodes:            ")+NumberToString(nodeCount));
    progress.Info(std::string("Ways/Areas/Sum:   ")+NumberToString(wayCount)+" "+
                  NumberToString(areaCount)+" "+
                  NumberToString(wayCount+areaCount));
    progress.Info(std::string("Relations:        ")+NumberToString(relationCount));
    progress.Info(std::string("Coastlines:       ")+NumberToString(coastlineCount));
    progress.Info(std::string("Turnrestrictions: ")+NumberToString(turnRestrictionCount));
    progress.Info(std::string("Multipolygons:    ")+NumberToString(multipolygonCount));
    progress.Info(std::string("Coord pages:      ")+NumberToString(coordIndex.size()));

    if (nodeSortingError) {
      progress.Error("Nodes are not sorted by increasing id");
    }

    if (waySortingError) {
      progress.Error("Ways are not sorted by increasing id");
    }

    if (relationSortingError) {
      progress.Error("Relations are not sorted by increasing id");
    }

    if (nodeSortingError || waySortingError || relationSortingError) {
      return false;
    }

    return true;
  }
}

