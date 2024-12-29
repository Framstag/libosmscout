/*
  This source is part of the libosmscout library
  Copyright (C) 2020  Tim Teulings

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

#include <osmscoutimport/GenPTRouteDat.h>

#include <list>
#include <map>

#include <osmscout/FeatureReader.h>

#include <osmscout/db/NodeDataFile.h>
#include <osmscout/db/WayDataFile.h>
#include <osmscout/db/PTRouteDataFile.h>

#include <osmscout/feature/ColorFeature.h>
#include <osmscout/feature/FromToFeature.h>
#include <osmscout/feature/NameFeature.h>
#include <osmscout/feature/NetworkFeature.h>
#include <osmscout/feature/OperatorFeature.h>
#include <osmscout/feature/RefFeature.h>

#include <osmscout/io/File.h>

#include <osmscout/util/Color.h>

#include <osmscoutimport/Preprocess.h>

namespace osmscout {

  bool PTRouteDataGenerator::WriteRoutes(const TypeConfig& typeConfig,
                                         const ImportParameter& parameter,
                                         Progress& progress,
                                         const std::list<PTRouteRef>& routes)
  {
    FileWriter writer;
    uint32_t   routesWrittenCount=static_cast<uint32_t>(routes.size());

    progress.SetAction("Writing '{}'",PTRouteDataFile::PTROUTES_DAT);

    try {
      writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                  PTRouteDataFile::PTROUTES_DAT));

      writer.Write(routesWrittenCount);

      for (const auto& route : routes) {
        route->Write(typeConfig,writer);
      }

      writer.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());

      writer.CloseFailsafe();

      return false;
    }

    progress.Info(std::string("Wrote "+std::to_string(routesWrittenCount)+" routes"));

    return true;
  }

  void PTRouteDataGenerator::GetDescription(const ImportParameter& /*parameter*/,
                                            ImportModuleDescription& description) const
  {
    description.SetName("PTRouteDataGenerator");
    description.SetDescription("Generate public transport route data");

    description.AddRequiredFile(Preprocess::RAWROUTEMASTER_DAT);
    description.AddRequiredFile(Preprocess::RAWROUTE_DAT);
    description.AddRequiredFile(NodeDataFile::NODES_IDMAP);
    description.AddRequiredFile(WayDataFile::WAYS_IDMAP);

    description.AddProvidedFile(PTRouteDataFile::PTROUTES_DAT);
  }

  bool PTRouteDataGenerator::Import(const TypeConfigRef& typeConfig,
                                    const ImportParameter& parameter,
                                    Progress& progress)
  {
    FileScanner                routeMasterScanner;
    FileScanner                routeScanner;
    FileScanner                nodeIdScanner;
    FileScanner                wayIdScanner;
    FileWriter                 routeWriter;
    std::list<PTRouteRef>      routes;

    std::map<Id,PTRouteRef>    idRouteMasterMap;
    std::map<ObjectOSMRef,ObjectFileRef> idMap;

    try {
      progress.SetAction("Scanning route masters");

      NameFeatureValueReader     nameReader(*typeConfig);
      RefFeatureValueReader      refReader(*typeConfig);
      OperatorFeatureValueReader operatorReader(*typeConfig);
      NetworkFeatureValueReader  networkReader(*typeConfig);
      FromToFeatureValueReader   fromToReader(*typeConfig);
      ColorFeatureValueReader    colorReader(*typeConfig);
      auto                       defaultName        =NameFeatureValue("");
      auto                       defaultRef         =RefFeatureValue("");
      auto                       defaultOperatorName=OperatorFeatureValue("");
      auto                       defaultNetworkName =NetworkFeatureValue("");
      auto                       defaultFromToName =FromToFeatureValue("","");
      auto                       defaultColor=ColorFeatureValue(Color::LUCENT_WHITE);

      routeMasterScanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                              Preprocess::RAWROUTEMASTER_DAT),
                              FileScanner::Sequential,
                              true);

      uint32_t routeMasterCount=routeMasterScanner.ReadUInt32();

      for (uint32_t m=1; m<=routeMasterCount; m++) {
        progress.SetProgress(m,routeMasterCount);

        RawRelation rawRel;

        rawRel.Read(*typeConfig,
                    routeMasterScanner);

        std::string name=nameReader.GetValue(rawRel.GetFeatureValueBuffer(),defaultName).GetName();
        std::string ref=refReader.GetValue(rawRel.GetFeatureValueBuffer(),defaultRef).GetRef();
        std::string operatorName=operatorReader.GetValue(rawRel.GetFeatureValueBuffer(),defaultOperatorName).GetOperator();
        std::string networkName=networkReader.GetValue(rawRel.GetFeatureValueBuffer(),defaultNetworkName).GetNetwork();
        Color       color=colorReader.GetValue(rawRel.GetFeatureValueBuffer(),defaultColor).GetColor();

        PTRouteRef route=std::make_shared<PTRoute>();

        // ID?
        route->SetType(rawRel.GetType());
        route->SetName(name);
        route->SetRef(ref);
        route->SetOperator(operatorName);
        route->SetNetwork(networkName);
        route->SetColor(color);

        routes.push_back(route);

        for (const auto& member : rawRel.members) {
          if (member.type==RawRelation::memberRelation) {
            idRouteMasterMap[member.id]=route;
          }
        }
      }

      routeMasterScanner.Close();

      progress.SetAction("Scanning routes for object references");

      routeScanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                        Preprocess::RAWROUTE_DAT),
                        FileScanner::Sequential,
                        true);

      uint32_t routeCount=routeScanner.ReadUInt32();

      for (uint32_t r=1; r<=routeCount; r++) {
        progress.SetProgress(r,routeCount);

        RawRelation rawRel;

        rawRel.Read(*typeConfig,
                    routeScanner);

        for (const auto& member : rawRel.members) {
          idMap.emplace(member.GetObjectOSMRef(),ObjectFileRef());
        }
      }

      progress.SetAction("Resolving node ids");


      nodeIdScanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                         NodeDataFile::NODES_IDMAP),
                         FileScanner::Sequential,
                         true);

      uint32_t nodeIdCount=nodeIdScanner.ReadUInt32();

      for (uint32_t n=1; n<nodeIdCount; n++) {
        progress.SetProgress(n,nodeIdCount);

        Id         id=nodeIdScanner.ReadUInt64();
        uint8_t    typeByte=nodeIdScanner.ReadUInt8();
        FileOffset fileOffset=nodeIdScanner.ReadFileOffset();

        ObjectOSMRef  osmRef(id,(OSMRefType)typeByte);

        if (idMap.find(osmRef)!=idMap.end()) {
          ObjectFileRef fileRef(fileOffset, refNode);
          idMap[osmRef]=fileRef;
        }
      }

      nodeIdScanner.Close();

      progress.SetAction("Resolving way ids");


      wayIdScanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                         WayDataFile::WAYS_IDMAP),
                         FileScanner::Sequential,
                         true);

      uint32_t wayIdCount=wayIdScanner.ReadUInt32();

      for (uint32_t w=1; w<wayIdCount; w++) {
        progress.SetProgress(w,wayIdCount);

        Id         id=wayIdScanner.ReadUInt64();
        uint8_t    typeByte=wayIdScanner.ReadUInt8();
        FileOffset fileOffset=wayIdScanner.ReadFileOffset();

        ObjectOSMRef  osmRef(id,(OSMRefType)typeByte);

        if (idMap.find(osmRef)!=idMap.end()) {
          ObjectFileRef fileRef(fileOffset, refWay);
          idMap[osmRef]=fileRef;
        }
      }

      wayIdScanner.Close();

      progress.SetAction("Scanning routes");

      routeScanner.GotoBegin();

      routeCount=routeScanner.ReadUInt32();

      for (uint32_t r=1; r<=routeCount; r++) {
        progress.SetProgress(r,routeCount);

        RawRelation rawRel;

        rawRel.Read(*typeConfig,
                    routeScanner);

        std::string name=nameReader.GetValue(rawRel.GetFeatureValueBuffer(),defaultName).GetName();
        std::string ref=refReader.GetValue(rawRel.GetFeatureValueBuffer(),defaultRef).GetRef();
        std::string operatorName=operatorReader.GetValue(rawRel.GetFeatureValueBuffer(),defaultOperatorName).GetOperator();
        std::string networkName=networkReader.GetValue(rawRel.GetFeatureValueBuffer(),defaultNetworkName).GetNetwork();
        auto        fromToValue=fromToReader.GetValue(rawRel.GetFeatureValueBuffer(),defaultFromToName);
        Color       color=colorReader.GetValue(rawRel.GetFeatureValueBuffer(),defaultColor).GetColor();

        auto       routeMasterIter=idRouteMasterMap.find(rawRel.GetId());
        PTRouteRef route;

        if (routeMasterIter==idRouteMasterMap.end()) {
          progress.Error("Cannot find route master for route "+std::to_string(rawRel.GetId())+" "+name);
          continue;
        }

        route=routeMasterIter->second;

        PTRoute::Variant variant;

        variant.SetName(name);
        variant.SetRef(ref);
        variant.SetOperator(operatorName);
        variant.SetNetwork(networkName);
        variant.SetFrom(fromToValue.GetFrom());
        variant.SetTo(fromToValue.GetTo());
        variant.SetColor(color);

        for (const auto& member : rawRel.members) {
          if (member.role=="stop") {
            PTRoute::Stop stop;

            auto objectFileRefIter=idMap.find(member.GetObjectOSMRef());

            if (objectFileRefIter!=idMap.end() &&
                objectFileRefIter->second.Valid()) {
              stop.SetStop(objectFileRefIter->second);
            }

            stop.SetType(PTRoute::StopType::normal);

            variant.stops.push_back(stop);
          }
          else if (member.role=="stop_entry_only") {
            PTRoute::Stop stop;

            auto objectFileRefIter=idMap.find(member.GetObjectOSMRef());

            if (objectFileRefIter!=idMap.end()) {
              stop.SetStop(objectFileRefIter->second);
            }

            stop.SetType(PTRoute::StopType::entryOnly);

            variant.stops.push_back(stop);
          }
          else if (member.role=="stop_exit_only") {
            PTRoute::Stop stop;

            auto objectFileRefIter=idMap.find(member.GetObjectOSMRef());

            if (objectFileRefIter!=idMap.end()) {
              stop.SetStop(objectFileRefIter->second);
            }

            stop.SetType(PTRoute::StopType::exitOnly);

            variant.stops.push_back(stop);
          }
          else if (member.role=="platform") {
            PTRoute::Platform platform;

            auto objectFileRefIter=idMap.find(member.GetObjectOSMRef());

            if (objectFileRefIter!=idMap.end()) {
              platform.SetPlatform(objectFileRefIter->second);
            }

            platform.SetType(PTRoute::PlatformType::normal);

            variant.platforms.push_back(platform);
          }
          else if (member.role=="platform_entry_only") {
            PTRoute::Platform platform;

            auto objectFileRefIter=idMap.find(member.GetObjectOSMRef());

            if (objectFileRefIter!=idMap.end()) {
              platform.SetPlatform(objectFileRefIter->second);
            }

            platform.SetType(PTRoute::PlatformType::entryOnly);

            variant.platforms.push_back(platform);
          }
          else if (member.role=="platform_exit_only") {
            PTRoute::Platform platform;

            auto objectFileRefIter=idMap.find(member.GetObjectOSMRef());

            if (objectFileRefIter!=idMap.end()) {
              platform.SetPlatform(objectFileRefIter->second);
            }

            platform.SetType(PTRoute::PlatformType::exitOnly);

            variant.platforms.push_back(platform);
          }
        }

        route->variants.push_back(variant);
      }

      routeScanner.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());

      nodeIdScanner.CloseFailsafe();
      routeScanner.CloseFailsafe();
      routeMasterScanner.CloseFailsafe();

      return false;
    }

    return WriteRoutes(*typeConfig,
                       parameter,
                       progress,
                       routes);
  }
}
