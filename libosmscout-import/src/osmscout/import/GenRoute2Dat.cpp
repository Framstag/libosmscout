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

namespace osmscout {

  const char* const RouteDataGenerator2::ROUTE_DAT="route.dat";

  void RouteDataGenerator2::GetDescription(const ImportParameter& /*parameter*/,
                                           ImportModuleDescription& description) const
  {
    description.SetName("RouteDataGenerator2");
    description.SetDescription("Generate route data");

    description.AddRequiredFile(Preprocess::RAWROUTE_DAT);
    description.AddRequiredFile(WayDataFile::WAYS_IDMAP);

    description.AddProvidedFile(ROUTE_DAT);
  }

  bool RouteDataGenerator2::Import(const TypeConfigRef& typeConfig,
                                   const ImportParameter& parameter,
                                   Progress& progress)
  {
    progress.SetAction("Generate route.dat");

    std::multimap<ObjectOSMRef,ObjectFileRef> wayIdMap;

    FileScanner scanner;
    FileWriter  routeWriter;
    FileScanner wayIdScanner;

    uint32_t    routeCount=0;

    try {
      wayIdScanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                        WayDataFile::WAYS_IDMAP),
                        FileScanner::Sequential,
                        true);

      uint32_t wayIdCount;
      wayIdScanner.Read(wayIdCount);

      for (uint32_t w=1; w<wayIdCount; w++) {
        progress.SetProgress(w,wayIdCount);

        Id         id;
        uint8_t    typeByte;
        FileOffset fileOffset;

        wayIdScanner.Read(id);
        wayIdScanner.Read(typeByte);
        wayIdScanner.ReadFileOffset(fileOffset);

        ObjectOSMRef  osmRef(id,(OSMRefType)typeByte);
        ObjectFileRef fileRef(fileOffset,refWay);

        if (idMap.find(osmRef)!=idMap.end()) {
          idMap[osmRef]=fileRef;
        }
      }

      wayIdScanner.Close();

      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   Preprocess::RAWROUTE_DAT),
                   FileScanner::Sequential,
                   true);

      scanner.Read(routeCount);

      routeWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                       ROUTE_DAT));

      routeWriter.Write(routeCount);

      for (uint32_t r=1; r <= routeCount; r++) {
        progress.SetProgress(r, routeCount);

        RawRelation rawRoute;
        rawRoute.Read(*typeConfig, scanner);

        Route route;

        std::vector<WayRef> ways;
        for (const auto &member: rawRoute.members){
          if (member.type==RawRelation::memberWay){

          }
        }

        route.Write(*typeConfig,
                    routeWriter);
      }

      progress.Info(std::string("Process ") + std::to_string(routeCount) + " routes");

      scanner.Close();
      routeWriter.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      scanner.CloseFailsafe();
      routeWriter.CloseFailsafe();
      return false;
    }

    return true;
  }

}