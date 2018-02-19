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

#include <functional>
#include <limits>

#include <osmscout/system/Math.h>

#include <osmscout/BoundingBoxDataFile.h>
#include <osmscout/TypeDistributionDataFile.h>
#include <osmscout/CoordDataFile.h>

#include <osmscout/util/File.h>

#include <osmscout/import/RawCoastline.h>
#include <osmscout/import/RawCoord.h>
#include <osmscout/import/RawNode.h>
#include <osmscout/import/RawWay.h>

#include <osmscout/private/Config.h>
#include <osmscout/ImportFeatures.h>

#if defined(HAVE_LIB_XML) || defined(OSMSCOUT_IMPORT_HAVE_XML_SUPPORT)
  #include <osmscout/import/PreprocessOSM.h>
#endif

#if defined(HAVE_LIB_PROTOBUF) || defined(OSMSCOUT_IMPORT_HAVE_PROTOBUF_SUPPORT)
  #include <osmscout/import/PreprocessPBF.h>
#endif

#include <osmscout/import/PreprocessPoly.h>

namespace osmscout {

  const char* Preprocess::RAWCOORDS_DAT="rawcoords.dat";
  const char* Preprocess::RAWNODES_DAT="rawnodes.dat";
  const char* Preprocess::RAWWAYS_DAT="rawways.dat";
  const char* Preprocess::RAWRELS_DAT="rawrels.dat";
  const char* Preprocess::RAWCOASTLINE_DAT="rawcoastline.dat";
  const char* Preprocess::RAWDATAPOLYGON_DAT="rawdatapolygon.dat";
  const char* Preprocess::RAWTURNRESTR_DAT="rawturnrestr.dat";

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

  Preprocess::Callback::Callback(const TypeConfigRef& typeConfig,
                                 const ImportParameter& parameter,
                                 Progress& progress)
  : typeConfig(typeConfig),
    parameter(parameter),
    progress(progress),
    blockWorkerQueue(parameter.GetProcessingQueueSize()),
    writeWorkerQueue(parameter.GetProcessingQueueSize()),
    writeWorkerThread(&Preprocess::Callback::WriteWorkerLoop,this),
    coordCount(0),
    nodeCount(0),
    wayCount(0),
    areaCount(0),
    relationCount(0),
    coastlineCount(0),
    datapolygonCount(0),
    turnRestrictionCount(0),
    multipolygonCount(0),
    lastNodeId(std::numeric_limits<OSMId>::min()),
    lastWayId(std::numeric_limits<OSMId>::min()),
    lastRelationId(std::numeric_limits<OSMId>::min()),
    nodeSortingError(false),
    waySortingError(false),
    relationSortingError(false)
  {
    minCoord.Set(90.0,180.0);
    maxCoord.Set(-90.0,-180.0);

    size_t blockWorkerCount=std::max((unsigned int)1,std::thread::hardware_concurrency());

    progress.Info("Using "+std::to_string(blockWorkerCount)+" block worker threads"+" with queue size of "+std::to_string(parameter.GetProcessingQueueSize()));

    for (size_t t=1; t<=blockWorkerCount; t++) {
      blockWorkerThreads.push_back(std::thread(&Preprocess::Callback::BlockWorkerLoop,this));
    }

    nodeStat.resize(typeConfig->GetTypeCount(),0);
    areaStat.resize(typeConfig->GetTypeCount(),0);
    wayStat.resize(typeConfig->GetTypeCount(),0);
  }

  Preprocess::Callback::~Callback()
  {
    // no code
  }

  bool Preprocess::Callback::Initialize()
  {
    readNodes=false;
    readWays=false;
    readRelations=false;

    try {
      rawCoordWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      RAWCOORDS_DAT));
      rawCoordWriter.Write(coordCount);

      nodeWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      RAWNODES_DAT));
      nodeWriter.Write(nodeCount);

      wayWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     RAWWAYS_DAT));
      wayWriter.Write(wayCount+areaCount);

      coastlineWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                           RAWCOASTLINE_DAT));
      coastlineWriter.Write(coastlineCount);

      datapolygonWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                             RAWDATAPOLYGON_DAT));
      datapolygonWriter.Write(datapolygonCount);

      turnRestrictionWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                                 RAWTURNRESTR_DAT));
      turnRestrictionWriter.Write(turnRestrictionCount);

      multipolygonWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                              RAWRELS_DAT));
      multipolygonWriter.Write(multipolygonCount);
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());

      rawCoordWriter.CloseFailsafe();
      nodeWriter.CloseFailsafe();
      wayWriter.CloseFailsafe();
      coastlineWriter.CloseFailsafe();
      datapolygonWriter.CloseFailsafe();
      turnRestrictionWriter.CloseFailsafe();
      multipolygonWriter.CloseFailsafe();

      return false;
    }

    return true;
  }

  void Preprocess::Callback::NodeSubTask(const RawNodeData& data,
                                         ProcessedData& processed)
  {
    RawCoord rawCoord;

    rawCoord.SetOSMId(data.id);
    rawCoord.SetCoord(data.coord);

    processed.rawCoords.push_back(std::move(rawCoord));

    TypeInfoRef type=typeConfig->GetNodeType(data.tags);

    if (!type->GetIgnore()) {
      RawNode node;

      node.SetId(data.id);
      node.SetType(type);
      node.SetCoord(data.coord);

      node.Parse(*parameter.GetErrorReporter(),
                 *typeConfig,
                 data.tags);

      processed.rawNodes.push_back(std::move(node));
    }
  }

  void Preprocess::Callback::WaySubTask(const RawWayData& data,
                                        ProcessedData& processed)
  {
    TypeInfoRef areaType;
    TypeInfoRef wayType;
    int         isArea=0; // 0==unknown, 1==true, -1==false
    bool        isCoastlineArea=false;
    RawWay      way;
    bool        isCoastline=false;
    bool        isDataPolygon=false;

    if (data.nodes.size()<2) {
      parameter.GetErrorReporter()->ReportWay(data.id,
                                              data.tags,
                                              "Less than two nodes");

      return;
    }

    way.SetId(data.id);

    auto naturalTag=data.tags.find(typeConfig->tagNatural);

    if (naturalTag!=data.tags.end() &&
        naturalTag->second=="coastline") {
      isCoastline=true;
    }

    if (isCoastline) {
      isCoastlineArea=data.nodes.size()>3 &&
                      (data.nodes.front()==data.nodes.back() ||
                       isArea==1);
    }

    auto dataPolygonTag=data.tags.find(typeConfig->tagDataPolygon);
    if (dataPolygonTag!=data.tags.end()) {
      isDataPolygon=true;
    }

    //
    // Way/Area object type detection
    //

    typeConfig->GetWayAreaType(data.tags,
                               wayType,
                               areaType);

    // Evaluate the isArea tag
    auto areaTag=data.tags.find(typeConfig->tagArea);

    if (areaTag==data.tags.end()) {
      isArea=0;
    }
    else if (areaTag->second=="0" ||
             areaTag->second=="no" ||
             areaTag->second=="false") {
      isArea=-1;
    }
    else {
      isArea=1;
    }

    // Evaluate the junction=roundabout tag
    auto junctionTag=data.tags.find(typeConfig->tagJunction);

    if (junctionTag!=data.tags.end() &&
        junctionTag->second=="roundabout") {
      isArea=-1;
    }

    if (isArea==0) {
      if (wayType->GetPinWay()) {
        isArea=-1;
      }
      else if (data.nodes.size()>3 &&
               data.nodes.front()==data.nodes.back()) {
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

        parameter.GetErrorReporter()->ReportWay(data.id,
                                                data.tags,
                                                "Should be way but is area");
      }

      if (areaType==typeConfig->typeInfoIgnore ||
          areaType->GetIgnore()) {
        way.SetType(typeConfig->typeInfoIgnore,
                    true);
      }
      else {
        way.SetType(areaType,true);
      }

      if (data.nodes.size()>3 &&
          data.nodes.front()==data.nodes.back()) {
        way.SetNodes(data.nodes.begin(),--data.nodes.end());
      }
      else {
        way.SetNodes(data.nodes.begin(),data.nodes.end());
      }

      break;
    case -1:
      if (wayType==typeConfig->typeInfoIgnore &&
          areaType!=typeConfig->typeInfoIgnore) {
        parameter.GetErrorReporter()->ReportWay(data.id,
                                                data.tags,
                                                "Should be area but is way");
      }

      if (wayType==typeConfig->typeInfoIgnore ||
          wayType->GetIgnore()) {
        way.SetType(typeConfig->typeInfoIgnore,false);
      }
      else {
        way.SetType(wayType,false);
      }

      way.SetNodes(data.nodes.begin(),data.nodes.end());

      break;
    default:
      assert(false);
    }

    way.Parse(*parameter.GetErrorReporter(),
              *typeConfig,
              data.tags);

    if (isCoastline) {
      RawCoastline coastline;

      coastline.SetId(way.GetId());
      coastline.SetType(isCoastlineArea);
      coastline.SetNodes(way.GetNodes());

      processed.rawCoastlines.push_back(std::move(coastline));
    }
    if (isDataPolygon){
      RawCoastline coastline;

      coastline.SetId(way.GetId());
      coastline.SetType(true);
      coastline.SetNodes(way.GetNodes());

      processed.rawDatapolygon.push_back(std::move(coastline));
    }

    processed.rawWays.push_back(std::move(way));
  }

  void Preprocess::Callback::TurnRestrictionSubTask(const std::vector<RawRelation::Member>& members,
                                                    TurnRestriction::Type type,
                                                    ProcessedData& processed)
  {
    OSMId from=0;
    OSMId via=0;
    OSMId to=0;

    for (const auto& member : members) {
      if (member.type==RawRelation::memberWay &&
        member.role=="from") {
        from=member.id;
      }
      else if (member.type==RawRelation::memberNode &&
        member.role=="via") {
        via=member.id;
      }
      else if (member.type==RawRelation::memberWay &&
        member.role=="to") {
        to=member.id;
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

      processed.turnRestriction.push_back(restriction);
    }
  }

  void Preprocess::Callback::MultipolygonSubTask(const TagMap& tags,
                                                 const std::vector<RawRelation::Member>& members,
                                                 OSMId id,
                                                 const TypeInfoRef& type,
                                                 ProcessedData& processed)
  {
    RawRelation relation;

    relation.SetId(id);

    if (type->GetIgnore()) {
      relation.SetType(typeConfig->typeInfoIgnore);
    }
    else {
      relation.SetType(type);
    }

    relation.members=members;

    relation.Parse(*parameter.GetErrorReporter(),
                   *typeConfig,
                   tags);

    processed.rawRelations.push_back(std::move(relation));
  }

  void Preprocess::Callback::RelationSubTask(const RawRelationData& data,
                                             ProcessedData& processed)
  {
    if (data.members.empty()) {
      progress.Warning("Relation "+
                       std::to_string(data.id)+
                       " does not have any members!");
      parameter.GetErrorReporter()->ReportRelation(data.id,
                                                   data.tags,
                                                   "Does not have any members");
      return;
    }

    TurnRestriction::Type turnRestrictionType;

    if (IsTurnRestriction(data.tags,
                          turnRestrictionType)) {
      TurnRestrictionSubTask(data.members,
                             turnRestrictionType,
                             processed);
    }

    TypeInfoRef multipolygonType;

    if (IsMultipolygon(data.tags,
                       multipolygonType)) {
      MultipolygonSubTask(data.tags,
                          data.members,
                          data.id,
                          multipolygonType,
                          processed);
    }
  }

  Preprocess::Callback::ProcessedDataRef Preprocess::Callback::BlockTask(RawBlockDataRef data)
  {
    ProcessedDataRef processed(new ProcessedData());

    processed->rawCoastlines.reserve(data->wayData.size());
    processed->rawCoords.reserve(data->nodeData.size());
    processed->rawNodes.reserve(data->nodeData.size());
    processed->rawWays.reserve(data->wayData.size());
    processed->rawRelations.reserve(data->relationData.size());
    processed->turnRestriction.reserve(data->relationData.size());

    //std::cout << "Poping block " << data->nodeData.size() << " " << data->wayData.size() << " " << data->relationData.size() << std::endl;

    for (const auto& entry : data->nodeData) {
      NodeSubTask(entry,
                  *processed);
    }

    for (const auto& entry : data->wayData) {
      WaySubTask(entry,
                 *processed);
    }

    for (const auto& entry : data->relationData) {
      RelationSubTask(entry,
                      *processed);
    }

    return processed;
  }

  void Preprocess::Callback::BlockWorkerLoop()
  {
    std::packaged_task<ProcessedDataRef()> task;

    while (blockWorkerQueue.PopTask(task)) {
      task();
    }
  }

  void Preprocess::Callback::WriteTask(std::shared_future<ProcessedDataRef>& p)
  {
    const ProcessedDataRef& processed=p.get();

    for (const auto& coastline : processed->rawCoastlines) {
      coastline.Write(coastlineWriter);
      coastlineCount++;
    }

    for (const auto& polygon : processed->rawDatapolygon){
      polygon.Write(datapolygonWriter);
      datapolygonCount++;
    }

    for (const auto& coord : processed->rawCoords) {
      coord.Write(rawCoordWriter);
      coordCount++;
    }

    for (const auto& node : processed->rawNodes) {
      node.Write(*typeConfig,
                 nodeWriter);

      TypeInfoRef type=node.GetType();

      nodeStat[type->GetIndex()]++;
      nodeCount++;
    }

    for (const auto& way : processed->rawWays) {
      if (way.IsArea()) {
        areaStat[way.GetType()->GetIndex()]++;
        areaCount++;
      }
      else {
        wayStat[way.GetType()->GetIndex()]++;
        wayCount++;
      }

      way.Write(*typeConfig,
                wayWriter);
    }

    for (const auto& relation : processed->rawRelations) {
      areaStat[relation.GetType()->GetIndex()]++;

      relation.Write(*typeConfig,
                     multipolygonWriter);

      multipolygonCount++;
    }

    for (const auto& turnRestriction : processed->turnRestriction) {
      turnRestriction.Write(turnRestrictionWriter);
      turnRestrictionCount++;
    }
  }

  void Preprocess::Callback::WriteWorkerLoop()
  {
    std::packaged_task<void()> task;

    while (writeWorkerQueue.PopTask(task)) {
      task();
    }
  }

  void Preprocess::Callback::ProcessBlock(RawBlockDataRef data)
  {
    //
    // Synchronous processing block, because of access to shared data
    //

    for (const auto& entry : data->nodeData) {
      if (entry.id<lastNodeId) {
        nodeSortingError=true;
      }

      lastNodeId=entry.id;

      minCoord.Set(std::min(minCoord.GetLat(),entry.coord.GetLat()),
                   std::min(minCoord.GetLon(),entry.coord.GetLon()));

      maxCoord.Set(std::max(maxCoord.GetLat(),entry.coord.GetLat()),
                   std::max(maxCoord.GetLon(),entry.coord.GetLon()));

      if (!readNodes) {
        progress.Info("Start reading nodes");
      }

      readNodes=true;
    }

    for (const auto& entry : data->wayData) {
      if (entry.id<lastWayId) {
        waySortingError=true;
      }

      lastWayId=entry.id;

      if (!readWays) {
        progress.Info("Start reading ways");
      }

      readWays=true;
    }

    for (const auto& entry : data->relationData) {
      if (entry.id<lastRelationId) {
        relationSortingError=true;
      }

      lastRelationId=entry.id;
      relationCount++;

      if (!readRelations) {
        progress.Info("Start reading relations");
      }

      readRelations=true;
    }

    //
    // Delegate rest of data processing to asynchronous Worker
    //

    //std::cout << "Pushing block " << data->nodeData.size() << " " << data->wayData.size() << " " << data->relationData.size() << std::endl;

    std::packaged_task<ProcessedDataRef()> blockTask(std::bind(&Preprocess::Callback::BlockTask,this,
                                                               data));
    // We use a shared_future because packaged_task does not work an all system with future, because future
    // is only moveable.
    std::shared_future<ProcessedDataRef>   processingResult(blockTask.get_future());


    blockWorkerQueue.PushTask(blockTask);

    //
    // Pass the (future of the) result of the processing back to the asynchronous writer
    //

    std::packaged_task<void()> writeTask(std::bind(&Preprocess::Callback::WriteTask,this,
                                                   processingResult));

    writeWorkerQueue.PushTask(writeTask);
  }


  bool Preprocess::Callback::DumpDistribution()
  {
    FileWriter writer;

    progress.SetAction("Writing 'distribution.dat'");

    try {
      writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                  TypeDistributionDataFile::DISTRIBUTION_DAT));

      for (const auto &type : typeConfig->GetTypes()) {
        writer.Write(nodeStat[type->GetIndex()]);
        writer.Write(wayStat[type->GetIndex()]);
        writer.Write(areaStat[type->GetIndex()]);
      }

      writer.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());

      writer.CloseFailsafe();

      return false;
    }

    return true;
  }

  bool Preprocess::Callback::DumpBoundingBox()
  {
    progress.SetAction("Generating '"+std::string(BoundingBoxDataFile::BOUNDINGBOX_DAT)+"'");

    FileWriter writer;

    try {
      writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                  BoundingBoxDataFile::BOUNDINGBOX_DAT));

      writer.WriteCoord(minCoord);
      writer.WriteCoord(maxCoord);

      writer.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());

      writer.CloseFailsafe();

      return false;
    }

    return true;
  }

  bool Preprocess::Callback::Cleanup(bool success)
  {
    progress.Info("Waiting for block processor...");
    blockWorkerQueue.Stop();
    for (auto& thread : blockWorkerThreads) {
      thread.join();
    }
    progress.Info("Waiting for block processor done.");

    progress.Info("Waiting for write processor...");
    writeWorkerQueue.Stop();
    writeWorkerThread.join();
    progress.Info("Waiting for write processor done.");

    rawCoordWriter.SetPos(0);
    rawCoordWriter.Write(coordCount);

    nodeWriter.SetPos(0);
    nodeWriter.Write(nodeCount);

    wayWriter.SetPos(0);
    wayWriter.Write(wayCount+areaCount);

    coastlineWriter.SetPos(0);
    coastlineWriter.Write(coastlineCount);

    datapolygonWriter.SetPos(0);
    datapolygonWriter.Write(datapolygonCount);

    turnRestrictionWriter.SetPos(0);
    turnRestrictionWriter.Write(turnRestrictionCount);

    multipolygonWriter.SetPos(0);
    multipolygonWriter.Write(multipolygonCount);

    try {
      rawCoordWriter.Close();
      nodeWriter.Close();
      wayWriter.Close();
      coastlineWriter.Close();
      datapolygonWriter.Close();
      turnRestrictionWriter.Close();
      multipolygonWriter.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());

      rawCoordWriter.CloseFailsafe();
      nodeWriter.CloseFailsafe();
      wayWriter.CloseFailsafe();
      coastlineWriter.CloseFailsafe();
      datapolygonWriter.CloseFailsafe();
      turnRestrictionWriter.CloseFailsafe();
      multipolygonWriter.CloseFailsafe();

      return false;
    }

    progress.SetAction("Dump statistics");

    if (success) {
      progress.Info(std::string("Coords:           ")+std::to_string(coordCount));
      progress.Info(std::string("Nodes:            ")+std::to_string(nodeCount));
      progress.Info(std::string("Ways/Areas/Sum:   ")+std::to_string(wayCount)+" "+
                    std::to_string(areaCount)+" "+
                    std::to_string(wayCount+areaCount));
      progress.Info(std::string("Relations:        ")+std::to_string(relationCount));
      progress.Info(std::string("Coastlines:       ")+std::to_string(coastlineCount));
      progress.Info(std::string("Turnrestrictions: ")+std::to_string(turnRestrictionCount));
      progress.Info(std::string("Multipolygons:    ")+std::to_string(multipolygonCount));

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
          progress.Warning("Type "+type->GetName()+ ": "+std::to_string(nodeStat[i])+" node(s), "+std::to_string(areaStat[i])+" area(s), "+std::to_string(wayStat[i])+" ways(s)");
        }
        else {
          progress.Info("Type "+type->GetName()+ ": "+std::to_string(nodeStat[i])+" node(s), "+std::to_string(areaStat[i])+" area(s), "+std::to_string(wayStat[i])+" ways(s)");
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

    description.AddProvidedFile(BoundingBoxDataFile::BOUNDINGBOX_DAT);

    description.AddProvidedTemporaryFile(TypeDistributionDataFile::DISTRIBUTION_DAT);
    description.AddProvidedTemporaryFile(RAWCOORDS_DAT);
    description.AddProvidedTemporaryFile(RAWNODES_DAT);
    description.AddProvidedTemporaryFile(RAWWAYS_DAT);
    description.AddProvidedTemporaryFile(RAWRELS_DAT);
    description.AddProvidedTemporaryFile(RAWCOASTLINE_DAT);
    description.AddProvidedTemporaryFile(RAWDATAPOLYGON_DAT);
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

#if defined(HAVE_LIB_XML) || defined(OSMSCOUT_IMPORT_HAVE_XML_SUPPORT)
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

#if defined(HAVE_LIB_PROTOBUF) || defined(OSMSCOUT_IMPORT_HAVE_PROTOBUF_SUPPORT)
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
        /*
      else if (filename.length()>=4 &&
               filename.substr(filename.length()-4)==".olt") {

        PreprocessOLT preprocess(callback);

        if (!preprocess.Import(typeConfig,
                               parameter,
                               progress,
                               filename)) {
          return false;
        }
      }*/
      else if (filename.length()>=5 &&
            filename.substr(filename.length()-5)==".poly") {

        PreprocessPoly preprocess(callback);

        if (!preprocess.Import(typeConfig,
                               parameter,
                               progress,
                               filename)) {
          return false;
        }
      }
      else {
        std::unique_ptr<Preprocessor> preprocessor=parameter.GetPreprocessor(filename,
                                                                            callback);

        if (preprocessor) {
          if (!preprocessor->Import(typeConfig,
                                    parameter,
                                    progress,
                                    filename)) {
            return false;
          }
        }
        else {
          progress.Error("Sorry, this file type is not yet supported!");
          return false;
        }
      }
    }

    if (!parameter.GetBoundingPolygonFile().empty()) {
      PreprocessPoly preprocess(callback);

      if (!preprocess.Import(typeConfig,
                             parameter,
                             progress,
                             parameter.GetBoundingPolygonFile())) {
        return false;
      }
    }

    return true;
  }

  bool Preprocess::Import(const TypeConfigRef& typeConfig,
                          const ImportParameter& parameter,
                          Progress& progress)
  {
    Callback callback(typeConfig,
                      parameter,
                      progress);

    if (!callback.Initialize()) {
      return false;
    }

    bool result=ProcessFiles(typeConfig,
                             parameter,
                             progress,
                             callback);

    if (!callback.Cleanup(result)) {
      return false;
    }

    return result;
  }
}

