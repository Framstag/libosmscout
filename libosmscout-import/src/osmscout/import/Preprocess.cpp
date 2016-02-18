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

#include <osmscout/CoordDataFile.h>

#include <osmscout/util/Logger.h>
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

namespace osmscout {

  static uint32_t coordPageSize=64;

  const char* Preprocess::BOUNDING_DAT="bounding.dat";
  const char* Preprocess::DISTRIBUTION_DAT="distribution.dat";
  const char* Preprocess::RAWNODES_DAT="rawnodes.dat";
  const char* Preprocess::RAWWAYS_DAT="rawways.dat";
  const char* Preprocess::RAWRELS_DAT="rawrels.dat";
  const char* Preprocess::RAWCOASTLINE_DAT="rawcoastline.dat";
  const char* Preprocess::RAWTURNRESTR_DAT="rawturnrestr.dat";

  bool Preprocess::Callback::StoreCurrentPage()
  {
    coordWriter.SetPos(currentPageOffset);

    for (size_t i=0; i<coordPageSize; i++) {
      if (!isSet[i]) {
        coordWriter.WriteInvalidCoord();
      }
      else {
        coordWriter.WriteCoord(coords[i]);
      }
    }

    currentPageId=std::numeric_limits<PageId>::max();

    return !coordWriter.HasError();
  }

  bool Preprocess::Callback::StoreCoord(OSMId id,
                                        const GeoCoord& coord)
  {
    PageId     relatedId=id-std::numeric_limits<Id>::min();
    PageId     pageId=relatedId/coordPageSize;
    FileOffset coordPageIndex=relatedId%coordPageSize;

    if (currentPageId!=std::numeric_limits<PageId>::max()) {
      if (currentPageId==pageId) {
        coords[coordPageIndex]=coord;
        isSet[coordPageIndex]=true;

        return true;
      }
      else {
        StoreCurrentPage();
      }
    }

    CoordPageOffsetMap::const_iterator pageOffsetEntry=coordIndex.find(pageId);

    // Do we write a coord to a page, we have not yet written and
    // thus must begin a new page?
    if (pageOffsetEntry==coordIndex.end()) {
      isSet.assign(coordPageSize,false);

      coords[coordPageIndex]=coord;
      isSet[coordPageIndex]=true;

      FileOffset pageOffset=coordPageCount*coordPageSize*coordByteSize;

      currentPageId=pageId;
      currentPageOffset=pageOffset;

      pageOffsetEntry=coordIndex.insert(std::make_pair(pageId,pageOffset)).first;

      coordPageCount++;

      return true;
    }

    // We have to update a coord in a page we have already written
    coordWriter.SetPos(pageOffsetEntry->second+coordPageIndex*coordByteSize);

    return coordWriter.WriteCoord(coord);
  }

  bool Preprocess::Callback::IsTurnRestriction(const TagMap& tags,
                                               TurnRestriction::Type& type) const
  {
    auto typeValue=tags.find(typeConfig->tagType);

    if (typeValue==tags.end()) {
      return false;
    }

    if (typeValue->second!="restriction") {
      return false;
    }

    auto restrictionValue=tags.find(typeConfig->tagRestriction);

    if (restrictionValue==tags.end()) {
      return false;
    }

    type=TurnRestriction::Allow;

    if (restrictionValue->second=="only_left_turn" ||
        restrictionValue->second=="only_right_turn" ||
        restrictionValue->second=="only_straight_on") {
      type=TurnRestriction::Allow;

      return true;
    }
    else if (restrictionValue->second=="no_left_turn" ||
             restrictionValue->second=="no_right_turn" ||
             restrictionValue->second=="no_straight_on" ||
             restrictionValue->second=="no_u_turn") {
      type=TurnRestriction::Forbit;

      return true;
    }

    return false;
  }

  void Preprocess::Callback::ProcessTurnRestriction(const std::vector<RawRelation::Member>& members,
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

  bool Preprocess::Callback::IsMultipolygon(const TagMap& tags,
                                            TypeInfoRef& type)
  {
    type=typeConfig->GetRelationType(tags);

    if (type!=typeConfig->typeInfoIgnore &&
        type->GetIgnore()) {
      return false;
    }

    bool isArea=type!=typeConfig->typeInfoIgnore &&
                type->GetMultipolygon();

    if (!isArea) {
      auto typeTag=tags.find(typeConfig->tagType);

      isArea=typeTag!=tags.end() && typeTag->second=="multipolygon";
    }

    return isArea;
  }

  void Preprocess::Callback::ProcessMultipolygon(const TagMap& tags,
                                                 const std::vector<RawRelation::Member>& members,
                                                 OSMId id,
                                                 const TypeInfoRef& type)
  {
    RawRelation relation;

    areaStat[type->GetIndex()]++;

    relation.SetId(id);

    if (type->GetIgnore()) {
      relation.SetType(typeConfig->typeInfoIgnore);
    }
    else {
      relation.SetType(type);
    }

    relation.members=members;

    relation.Parse(progress,
                   *typeConfig,
                   tags);
    relation.Write(*typeConfig,
                   multipolygonWriter);

    multipolygonCount++;
  }

  Preprocess::Callback::Callback(const TypeConfigRef& typeConfig,
                                 const ImportParameter& parameter,
                                 Progress& progress)
  : typeConfig(typeConfig),
    parameter(parameter),
    progress(progress),
    coordCount(0),
    nodeCount(0),
    wayCount(0),
    areaCount(0),
    relationCount(0),
    coastlineCount(0),
    turnRestrictionCount(0),
    multipolygonCount(0),
    lastNodeId(std::numeric_limits<OSMId>::min()),
    lastWayId(std::numeric_limits<OSMId>::min()),
    lastRelationId(std::numeric_limits<OSMId>::min()),
    nodeSortingError(false),
    waySortingError(false),
    relationSortingError(false),
    coordPageCount(0),
    currentPageId(std::numeric_limits<PageId>::max()),
    currentPageOffset(0)
  {
    minCoord.Set(90.0,180.0);
    maxCoord.Set(-90.0,-180.0);

    nodeStat.resize(typeConfig->GetTypeCount(),0);
    areaStat.resize(typeConfig->GetTypeCount(),0);
    wayStat.resize(typeConfig->GetTypeCount(),0);
  }


  bool Preprocess::Callback::Initialize()
  {
    try {
      nodeWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      RAWNODES_DAT));
      nodeWriter.Write(nodeCount);

      wayWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     RAWWAYS_DAT));
      wayWriter.Write(wayCount+areaCount);

      coastlineWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                           RAWCOASTLINE_DAT));
      coastlineWriter.Write(coastlineCount);

      turnRestrictionWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                                 RAWTURNRESTR_DAT));
      turnRestrictionWriter.Write(turnRestrictionCount);

      multipolygonWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                              RAWRELS_DAT));
      multipolygonWriter.Write(multipolygonCount);

      coordWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                       CoordDataFile::COORD_DAT));

      FileOffset offset=0;

      coordWriter.Write(coordPageSize);
      coordWriter.Write(offset);
      coordWriter.FlushCurrentBlockWithZeros(coordPageSize*coordByteSize);

      coordPageCount++;

      coords.resize(coordPageSize);
      isSet.resize(coordPageSize);
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();

      nodeWriter.CloseFailsafe();
      wayWriter.CloseFailsafe();
      coastlineWriter.CloseFailsafe();
      turnRestrictionWriter.CloseFailsafe();
      multipolygonWriter.CloseFailsafe();
      coordWriter.CloseFailsafe();

      return false;
    }

    return true;
  }

  void Preprocess::Callback::ProcessNode(const OSMId& id,
                                         const double& lon,
                                         const double& lat,
                                         const TagMap& tagMap)
  {
    RawNode      node;
    ObjectOSMRef object(id,
                        osmRefNode);

    if (id<lastNodeId) {
      nodeSortingError=true;
    }

    minCoord.Set(std::min(minCoord.GetLat(),lat),
                 std::min(minCoord.GetLon(),lon));

    maxCoord.Set(std::max(maxCoord.GetLat(),lat),
                 std::max(maxCoord.GetLon(),lon));

    coordCount++;

    StoreCoord(id,
               GeoCoord(lat,
                        lon));

    TypeInfoRef type=typeConfig->GetNodeType(tagMap);

    nodeStat[type->GetIndex()]++;

    if (!type->GetIgnore()) {
      node.SetId(id);
      node.SetType(type);
      node.SetCoords(lon,lat);

      node.Parse(progress,
                 *typeConfig,
                 tagMap);

      node.Write(*typeConfig,
                 nodeWriter);

      nodeCount++;
    }

    lastNodeId=id;
  }

  void Preprocess::Callback::ProcessWay(const OSMId& id,
                                        std::vector<OSMId>& nodes,
                                        const TagMap& tagMap)
  {
    TypeInfoRef areaType;
    TypeInfoRef wayType;
    int         isArea=0; // 0==unknown, 1==true, -1==false
    bool        isCoastlineArea=false;
    RawWay      way;
    bool        isCoastline=false;

    if (nodes.size()<2) {
      progress.Warning("Way "+
                       NumberToString(id)+
                       " has less than two nodes!");
      return;
    }

    if (id<lastWayId) {
      waySortingError=true;
    }

    way.SetId(id);

    auto naturalTag=tagMap.find(typeConfig->tagNatural);

    if (naturalTag!=tagMap.end() &&
        naturalTag->second=="coastline") {
      isCoastline=true;
    }

    if (isCoastline) {
      isCoastlineArea=nodes.size()>3 &&
                      (nodes.front()==nodes.back() ||
                       isArea==1);
    }

    //
    // Way/Area object type detection
    //

    typeConfig->GetWayAreaType(tagMap,
                               wayType,
                               areaType);

    // Evaluate the isArea tag
    auto areaTag=tagMap.find(typeConfig->tagArea);

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

    // Evaluate the junction=roundabout tag
    auto junctionTag=tagMap.find(typeConfig->tagJunction);

    if (junctionTag!=tagMap.end() &&
        junctionTag->second=="roundabout") {
      isArea=-1;
    }

    if (isArea==0) {
      if (wayType->GetPinWay()) {
        isArea=-1;
      }
      else if (nodes.size()>3 &&
         nodes.front()==nodes.back()) {
        isArea=1;
      }
      else {
        isArea=-1;
      }
    }

    switch (isArea) {
    case 1:
      if (areaType==typeConfig->typeInfoIgnore &&
          wayType!=typeConfig->typeInfoIgnore) {
        progress.Warning("Way "+
                         NumberToString(id)+
                         " of type '" + wayType->GetName()+"' should be way but is area => ignoring type");
      }

      if (areaType==typeConfig->typeInfoIgnore ||
          areaType->GetIgnore()) {
        way.SetType(typeConfig->typeInfoIgnore,
                    true);
      }
      else {
        way.SetType(areaType,true);
      }

      if (nodes.size()>3 &&
          nodes.front()==nodes.back()) {
        nodes.pop_back();
      }

      areaStat[areaType->GetIndex()]++;

      areaCount++;
      break;
    case -1:
      if (wayType==typeConfig->typeInfoIgnore &&
          areaType!=typeConfig->typeInfoIgnore) {
        progress.Warning("Way "+
                         NumberToString(id)+
                         " of type '" + areaType->GetName()+"' should be area but is way => ignoring type!");
      }

      if (wayType==typeConfig->typeInfoIgnore ||
          wayType->GetIgnore()) {
        way.SetType(typeConfig->typeInfoIgnore,false);
      }
      else {
        way.SetType(wayType,false);
      }

      wayStat[wayType->GetIndex()]++;

      wayCount++;
      break;
    default:
      assert(false);
    }

    way.SetNodes(nodes);

    way.Parse(progress,
              *typeConfig,
              tagMap);

    way.Write(*typeConfig,
              wayWriter);

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

  void Preprocess::Callback::ProcessRelation(const OSMId& id,
                                             const std::vector<RawRelation::Member>& members,
                                             const TagMap& tagMap)
  {
    if (id<lastRelationId) {
      relationSortingError=true;
    }

    if (members.empty()) {
      progress.Warning("Relation "+
                       NumberToString(id)+
                       " does not have any members!");
      return;
    }

    TurnRestriction::Type turnRestrictionType;

    if (IsTurnRestriction(tagMap,
                          turnRestrictionType)) {
      ProcessTurnRestriction(members,
                             turnRestrictionType);
    }

    TypeInfoRef multipolygonType;

    if (IsMultipolygon(tagMap,
                       multipolygonType)) {
      ProcessMultipolygon(tagMap,
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

  bool Preprocess::Callback::DumpDistribution()
  {
    FileWriter writer;

    progress.SetAction("Writing 'distribution.dat'");

    try {
      writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                  DISTRIBUTION_DAT));

      for (const auto &type : typeConfig->GetTypes()) {
        writer.Write(nodeStat[type->GetIndex()]);
        writer.Write(wayStat[type->GetIndex()]);
        writer.Write(areaStat[type->GetIndex()]);
      }

      writer.Close();
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();

      writer.CloseFailsafe();

      return false;
    }

    return true;
  }

  bool Preprocess::Callback::DumpBoundingBox()
  {
    progress.SetAction("Generating bounding.dat");

    FileWriter writer;

    try {
      writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                  BOUNDING_DAT));

      if (!writer.WriteCoord(minCoord) ||
          !writer.WriteCoord(maxCoord)) {
        progress.Error("Cannot write to '"+writer.GetFilename()+"'");
        return false;
      }

      writer.Close();
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();

      writer.CloseFailsafe();

      return false;
    }

    return true;
  }

  bool Preprocess::Callback::Cleanup(bool success)
  {
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

    for (const auto& entry :coordIndex) {
      coordWriter.Write(entry.first);
      coordWriter.Write(entry.second);
    }

    try {
      nodeWriter.Close();
      wayWriter.Close();
      coastlineWriter.Close();
      coordWriter.Close();
      turnRestrictionWriter.Close();
      multipolygonWriter.Close();
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();

      nodeWriter.CloseFailsafe();
      wayWriter.CloseFailsafe();
      coastlineWriter.CloseFailsafe();
      coordWriter.CloseFailsafe();
      turnRestrictionWriter.CloseFailsafe();
      multipolygonWriter.CloseFailsafe();

      return false;
    }

    if (success) {
      progress.Info(std::string("Coords:           ")+NumberToString(coordCount));
      progress.Info(std::string("Coord pages:      ")+NumberToString(coordIndex.size()));
      progress.Info(std::string("Nodes:            ")+NumberToString(nodeCount));
      progress.Info(std::string("Ways/Areas/Sum:   ")+NumberToString(wayCount)+" "+
                    NumberToString(areaCount)+" "+
                    NumberToString(wayCount+areaCount));
      progress.Info(std::string("Relations:        ")+NumberToString(relationCount));
      progress.Info(std::string("Coastlines:       ")+NumberToString(coastlineCount));
      progress.Info(std::string("Turnrestrictions: ")+NumberToString(turnRestrictionCount));
      progress.Info(std::string("Multipolygons:    ")+NumberToString(multipolygonCount));

      for (const auto &type : typeConfig->GetTypes()) {
        size_t      i=type->GetIndex();
        bool        isEmpty=(type->CanBeNode() && nodeStat[i]==0) ||
                            (type->CanBeArea() && areaStat[i]==0) ||
                            (type->CanBeWay() && wayStat[i]==0);
        bool        isImportant=!type->GetIgnore() &&
                                !type->GetName().empty() &&
                                type->GetName()[0]!='_';

        if (isEmpty &&
            isImportant) {
          progress.Warning("Type "+type->GetName()+ ": "+NumberToString(nodeStat[i])+" node(s), "+NumberToString(areaStat[i])+" area(s), "+NumberToString(wayStat[i])+" ways(s)");
        }
        else {
          progress.Info("Type "+type->GetName()+ ": "+NumberToString(nodeStat[i])+" node(s), "+NumberToString(areaStat[i])+" area(s), "+NumberToString(wayStat[i])+" ways(s)");
        }
      }
    }

    //std::cout << "Bounding box: " << "[" << minCoord.GetLat() << "," << minCoord.GetLon() << " x " << maxCoord.GetLat() << "," << maxCoord.GetLon() << "]" << std::endl;

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

    if (success) {
      if (!DumpDistribution()) {
        return false;
      }

      if (!DumpBoundingBox()) {
        return false;
      }
    }

    return true;
  }

  void Preprocess::GetDescription(const ImportParameter& /*parameter*/,
                                  ImportModuleDescription& description) const
  {
    description.SetName("Preprocess");
    description.SetDescription("Initial parsing of import file(s)");

    description.AddProvidedFile(BOUNDING_DAT);

    description.AddProvidedDebuggingFile(CoordDataFile::COORD_DAT);

    description.AddProvidedTemporaryFile(DISTRIBUTION_DAT);
    description.AddProvidedTemporaryFile(RAWNODES_DAT);
    description.AddProvidedTemporaryFile(RAWWAYS_DAT);
    description.AddProvidedTemporaryFile(RAWRELS_DAT);
    description.AddProvidedTemporaryFile(RAWCOASTLINE_DAT);
    description.AddProvidedTemporaryFile(RAWTURNRESTR_DAT);
  }

  bool Preprocess::ProcessFiles(const TypeConfigRef& typeConfig,
                                const ImportParameter& parameter,
                                Progress& progress,
                                Callback& callback)
  {
    for (const auto& filename : parameter.GetMapfiles()) {
      if (filename.length()>=4 &&
          filename.substr(filename.length()-4)==".osm")  {

#if defined(HAVE_LIB_XML)
        PreprocessOSM preprocess(callback);

        if (!preprocess.Import(typeConfig,
                               parameter,
                               progress,
                               filename)) {
          return false;
        }
#else
        progress.Error("Support for the OSM file format is not enabled!");
        return false;
#endif
      }
      else if (filename.length()>=4 &&
            filename.substr(filename.length()-4)==".pbf") {

#if defined(HAVE_LIB_PROTOBUF)
        PreprocessPBF preprocess(callback);

        if (!preprocess.Import(typeConfig,
                               parameter,
                               progress,
                               filename)) {
          return false;
        }
#else
        progress.Error("Support for the PBF file format is not enabled!");
        return false;
#endif
      }
      else {
        progress.Error("Sorry, this file type is not yet supported!");
        return false;
      }
    }

    return true;
  }

  bool Preprocess::Import(const TypeConfigRef& typeConfig,
                          const ImportParameter& parameter,
                          Progress& progress)
  {
    bool     result=false;
    Callback callback(typeConfig,
                      parameter,
                      progress);

    if (!callback.Initialize()) {
      return false;
    }

    result=ProcessFiles(typeConfig,
                        parameter,
                        progress,
                        callback);

    if (!callback.Cleanup(result)) {
      return false;
    }

    return result;
  }
}

