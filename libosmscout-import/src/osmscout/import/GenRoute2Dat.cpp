/*
  This source is part of the libosmscout library
  Copyright (C) 2020  Lukáš Karas

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

#include <osmscout/Route.h>

#include <osmscout/import/GenRoute2Dat.h>
#include <osmscout/import/Preprocess.h>
#include <osmscout/WayDataFile.h>
#include <osmscout/RouteDataFile.h>

namespace osmscout {

  void RouteDataGenerator2::GetDescription(const ImportParameter& /*parameter*/,
                                           ImportModuleDescription& description) const
  {
    description.SetName("RouteDataGenerator2");
    description.SetDescription("Generate route data");

    description.AddRequiredFile(Preprocess::RAWROUTE_DAT);
    description.AddRequiredFile(WayDataFile::WAYS_IDMAP);
    description.AddRequiredFile(WayDataFile::WAYS_DAT);

    description.AddProvidedFile(RouteDataFile::ROUTE_DAT);
  }

  bool RouteDataGenerator2::Import(const TypeConfigRef& typeConfig,
                                   const ImportParameter& parameter,
                                   Progress& progress)
  {
    progress.SetAction("Generate route.dat");

    std::multimap<OSMId,FileOffset> wayIdMap;

    FileScanner rawRouteScanner;
    FileWriter  routeWriter;
    FileScanner wayIdScanner;
    WayDataFile wayData(parameter.GetWayDataCacheSize());

    if (!wayData.Open(typeConfig, parameter.GetDestinationDirectory(), true)){
      progress.Error("Cannot open way data file");
      return false;
    }

    try {
      wayIdScanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                        WayDataFile::WAYS_IDMAP),
                        FileScanner::Sequential,
                        true);

      uint32_t wayIdCount;
      wayIdScanner.Read(wayIdCount);

      for (uint32_t w=1; w<wayIdCount; w++) {
        progress.SetProgress(w,wayIdCount);

        OSMId      id;
        uint8_t    typeByte;
        FileOffset fileOffset;

        wayIdScanner.Read(id);
        wayIdScanner.Read(typeByte);
        assert((OSMRefType)typeByte==osmRefWay);
        wayIdScanner.ReadFileOffset(fileOffset);

        wayIdMap.insert(std::make_pair(id, fileOffset));
      }

      wayIdScanner.Close();

      rawRouteScanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                           Preprocess::RAWROUTE_DAT),
                           FileScanner::Sequential,
                           true);

      uint32_t rawRouteCount=0;
      uint32_t routeCount=0;
      rawRouteScanner.Read(rawRouteCount);

      routeWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                       RouteDataFile::ROUTE_DAT));

      routeWriter.Write(routeCount);

      for (uint32_t r=1; r <= rawRouteCount; r++) {
        progress.SetProgress(r, rawRouteCount);

        RawRelation rawRoute;
        rawRoute.Read(*typeConfig, rawRouteScanner);

        Route route;
        route.SetFeatures(rawRoute.GetFeatureValueBuffer()); // setup type also

        // load all way members
        std::multimap<Id, WayRef> wayPointMap;
        for (const auto &member: rawRoute.members){
          if (member.type==RawRelation::memberWay){
            for (auto it=wayIdMap.lower_bound(member.id); it!=wayIdMap.upper_bound(member.id); ++it) {
              WayRef way;
              if (!wayData.GetByOffset(it->second, way)){
                progress.Error("Cannot read way");
                return false;
              }
              assert(way);
              assert(!way->nodes.empty());

              wayPointMap.insert(std::make_pair(way->GetFrontId(),way));
              wayPointMap.insert(std::make_pair(way->GetBackId(),way));
              route.bbox.Include(way->GetBoundingBox());
            }
          }
        }

        // build route segments
        while (!wayPointMap.empty()){
          // find some point where is just one way
          Id segmentFrontId;
          for (auto it:wayPointMap){
            segmentFrontId=it.first;
            if (wayPointMap.count(segmentFrontId) == 1) {
              break;
            }
          }

          // build segment
          auto startWayIt=wayPointMap.lower_bound(segmentFrontId);
          WayRef tailWay=startWayIt->second;
          Route::Segment segment;
          Id segmentBackId=segmentFrontId;
          wayPointMap.erase(startWayIt);
          while (tailWay) {
            if (segmentBackId == tailWay->GetFrontId()) {
              segmentBackId = tailWay->GetBackId();
              segment.members.push_back(Route::SegmentMember{Route::MemberDirection::forward, tailWay->GetFileOffset()});
            } else {
              segmentBackId = tailWay->GetFrontId();
              segment.members.push_back(Route::SegmentMember{Route::MemberDirection::backward, tailWay->GetFileOffset()});
            }
            WayRef newTail;
            for (auto it=wayPointMap.lower_bound(segmentBackId);
                 it != wayPointMap.upper_bound(segmentBackId);){
              if (it->second == tailWay) {
                it=wayPointMap.erase(it);
              } else if (!newTail) {
                newTail=it->second;
                it=wayPointMap.erase(it);
              } else {
                ++it;
              }
            }
            tailWay=newTail;
          }
          route.segments.push_back(std::move(segment));
        }

        if (!route.segments.empty() && route.bbox.IsValid()) {
          route.Write(*typeConfig,
                      routeWriter);
          routeCount++;
        }
      }

      routeWriter.SetPos(0);
      routeWriter.Write(routeCount); // write actual number of routes in file

      progress.Info(std::string("Process ") + std::to_string(rawRouteCount) + " routes");

      rawRouteScanner.Close();
      routeWriter.Close();
      wayData.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      rawRouteScanner.CloseFailsafe();
      routeWriter.CloseFailsafe();
      wayData.Close();
      return false;
    }

    return true;
  }

}