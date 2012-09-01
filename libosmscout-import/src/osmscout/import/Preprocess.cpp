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

#include <osmscout/import/RawCoastline.h>
#include <osmscout/import/RawNode.h>
#include <osmscout/import/RawWay.h>

#include <osmscout/util/File.h>
#include <osmscout/util/String.h>

#include <osmscout/private/Config.h>

#if defined(HAVE_LIB_XML)
  #include <osmscout/import/PreprocessOSM.h>
#endif

#if defined(HAVE_LIB_PROTOBUF)
  #include <osmscout/import/PreprocessPBF.h>
#endif

namespace osmscout {

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

  bool Preprocess::Initialize(const ImportParameter& parameter)
  {
    nodeCount=0;
    wayCount=0;
    areaCount=0;
    relationCount=0;
    coastlineCount=0;

    lastNodeId=0;
    lastWayId=0;
    lastRelationId=0;

    nodeSortingError=false;
    waySortingError=false;
    relationSortingError=false;

    nodeWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                    "rawnodes.dat"));
    nodeWriter.Write(nodeCount);

    wayWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   "rawways.dat"));
    wayWriter.Write(wayCount+areaCount);

    relationWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                        "rawrels.dat"));
    relationWriter.Write(relationCount);

    coastlineWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                        "rawcoastline.dat"));
    coastlineWriter.Write(coastlineCount);

    return !nodeWriter.HasError() &&
           !wayWriter.HasError() &&
           !relationWriter.HasError() &&
           !coastlineWriter.HasError();
  }

  void Preprocess::ProcessNode(const TypeConfig& typeConfig,
                               const Id& id,
                               const double& lon,
                               const double& lat,
                               const std::map<TagId,std::string>& tagMap)
  {
    RawNode node;
    TypeId  type=typeIgnore;

    if (id<lastNodeId) {
      nodeSortingError=true;
    }

    typeConfig.GetNodeTypeId(tagMap,type);
    typeConfig.ResolveTags(tagMap,tags);

    node.SetId(id);
    node.SetType(type);
    node.SetCoordinates(lon,lat);
    node.SetTags(tags);

    node.Write(nodeWriter);

    nodeCount++;
    lastNodeId=id;
  }

  void Preprocess::ProcessWay(const TypeConfig& typeConfig,
                              const Id& id,
                              std::vector<Id>& nodes,
                              const std::map<TagId,std::string>& tagMap)
  {
    TypeId                                      areaType=typeIgnore;
    TypeId                                      wayType=typeIgnore;
    int                                         isArea=0; // 0==unknown, 1==true, -1==false
    std::map<TagId,std::string>::const_iterator areaTag;
    std::map<TagId,std::string>::const_iterator naturalTag;
    RawWay                                      way;
    bool                                        isCoastline=false;

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
      else if (areaType!=typeIgnore &&
          nodes.size()>2 &&
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

    if (isArea==1) {
      way.SetType(areaType,true);

      if (nodes.size()>2 &&
          nodes.front()==nodes.back()) {
        nodes.pop_back();
      }

      areaCount++;
    }
    else if (isArea==-1) {
      way.SetType(wayType,false);
      wayCount++;
    }
    else {
      if (nodes.size()>1 &&
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
      coastline.SetType(way.IsArea());
      coastline.SetNodes(way.GetNodes());

      coastline.Write(coastlineWriter);

      coastlineCount++;
    }
  }

  void Preprocess::ProcessRelation(const TypeConfig& typeConfig,
                                   const Id& id,
                                   const std::vector<RawRelation::Member>& members,
                                   const std::map<TagId,std::string>& tagMap)
  {
    RawRelation relation;
    TypeId      type;

    if (id<lastRelationId) {
      relationSortingError=true;
    }

    relation.SetId(id);
    relation.members=members;

    typeConfig.GetRelationTypeId(tagMap,type);
    typeConfig.ResolveTags(tagMap,relation.tags);

    relation.SetType(type);

    relation.Write(relationWriter);

    relationCount++;
    lastRelationId=id;
  }

  bool Preprocess::Cleanup(const ImportParameter& parameter,
                           Progress& progress)
  {
    nodeWriter.SetPos(0);
    nodeWriter.Write(nodeCount);

    wayWriter.SetPos(0);
    wayWriter.Write(wayCount+areaCount);

    relationWriter.SetPos(0);
    relationWriter.Write(relationCount);

    coastlineWriter.SetPos(0);
    coastlineWriter.Write(coastlineCount);

    nodeWriter.Close();
    wayWriter.Close();
    relationWriter.Close();
    coastlineWriter.Close();

    progress.Info(std::string("Nodes:          ")+NumberToString(nodeCount));
    progress.Info(std::string("Ways/Areas/Sum: ")+NumberToString(wayCount)+" "+
                  NumberToString(areaCount)+" "+
                  NumberToString(wayCount+areaCount));
    progress.Info(std::string("Relations:      ")+NumberToString(relationCount));
    progress.Info(std::string("Coastlines:     ")+NumberToString(coastlineCount));

    if (!parameter.GetRenumberIds()) {
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
    }

    return true;
  }
}

