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

#include <osmscout/import/GenPTRouteDat.h>

#include <list>
#include <map>

#include <osmscout/TypeFeatures.h>
#include <osmscout/FeatureReader.h>

#include <osmscout/PTRouteDataFile.h>

#include <osmscout/import/Preprocess.h>

#include <osmscout/util/File.h>

namespace osmscout {

  bool PTRouteDataGenerator::WriteRoutes(const TypeConfig& typeConfig,
                                         const ImportParameter& parameter,
                                         Progress& progress,
                                         const std::list<PTRouteRef>& routes)
  {
    FileWriter writer;
    uint32_t   routesWrittenCount=routes.size();

    progress.SetAction("Writing "+std::string(PTRouteDataFile::PTROUTES_DAT));

    try {
      writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                  PTRouteDataFile::PTROUTES_DAT));

      writer.Write(routesWrittenCount);

      for (auto route : routes) {
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

    description.AddProvidedFile(PTRouteDataFile::PTROUTES_DAT);
  }

  bool PTRouteDataGenerator::Import(const TypeConfigRef& typeConfig,
                                    const ImportParameter& parameter,
                                    Progress& progress)
  {
    FileScanner           routeMasterScanner;
    FileScanner           routeScanner;
    FileWriter            routeWriter;
    std::list<PTRouteRef> routes;

    std::map<Id,PTRouteRef> idRouteMasterMap;

    try {
      progress.SetAction("Scanning route masters");

      uint32_t                   routeMasterCount   =0;
      uint32_t                   routeCount         =0;
      NameFeatureValueReader     nameReader(*typeConfig);
      RefFeatureValueReader      refReader(*typeConfig);
      OperatorFeatureValueReader operatorReader(*typeConfig);
      NetworkFeatureValueReader  networkReader(*typeConfig);
      auto                       defaultName        =NameFeatureValue("");
      auto                       defaultRef         =RefFeatureValue("");
      auto                       defaultOperatorName=OperatorFeatureValue("");
      auto                       defaultNetworkName =NetworkFeatureValue("");

      routeMasterScanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                              Preprocess::RAWROUTEMASTER_DAT),
                              FileScanner::Sequential,
                              true);

      routeMasterScanner.Read(routeMasterCount);

      for (uint32_t m=1; m<=routeMasterCount; m++) {
        progress.SetProgress(m,routeMasterCount);

        RawRelation rawRel;

        rawRel.Read(*typeConfig,
                    routeMasterScanner);

        std::string name=nameReader.GetValue(rawRel.GetFeatureValueBuffer(),defaultName).GetName();
        std::string ref=refReader.GetValue(rawRel.GetFeatureValueBuffer(),defaultRef).GetRef();
        std::string operatorName=operatorReader.GetValue(rawRel.GetFeatureValueBuffer(),defaultOperatorName).GetOperator();
        std::string networkName=networkReader.GetValue(rawRel.GetFeatureValueBuffer(),defaultNetworkName).GetNetwork();

        PTRouteRef route=std::make_shared<PTRoute>();

        // ID?
        route->SetType(rawRel.GetType());
        route->SetName(name);
        route->SetRef(ref);
        route->SetOperator(operatorName);
        route->SetNetwork(networkName);

        routes.push_back(route);

        for (const auto& member : rawRel.members) {
          if (member.type==RawRelation::memberRelation) {
            idRouteMasterMap[member.id]=route;
          }
        }
      }

      routeMasterScanner.Close();

      progress.SetAction("Scanning routes");

      routeScanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                        Preprocess::RAWROUTE_DAT),
                        FileScanner::Sequential,
                        true);

      routeScanner.Read(routeCount);

      for (uint32_t r=1; r<=routeCount; r++) {
        progress.SetProgress(r,routeCount);

        RawRelation rawRel;

        rawRel.Read(*typeConfig,
                    routeScanner);

        std::string name=nameReader.GetValue(rawRel.GetFeatureValueBuffer(),defaultName).GetName();
        std::string ref=refReader.GetValue(rawRel.GetFeatureValueBuffer(),defaultRef).GetRef();
        std::string operatorName=operatorReader.GetValue(rawRel.GetFeatureValueBuffer(),defaultOperatorName).GetOperator();
        std::string networkName=networkReader.GetValue(rawRel.GetFeatureValueBuffer(),defaultNetworkName).GetNetwork();

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

        route->variants.push_back(variant);
      }

      routeScanner.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());

      routeScanner.CloseFailsafe();
      routeMasterScanner.CloseFailsafe();

      return false;
    }

    if (!WriteRoutes(*typeConfig,parameter,progress,routes)) {
      return false;
    }

    return true;
  }
}
