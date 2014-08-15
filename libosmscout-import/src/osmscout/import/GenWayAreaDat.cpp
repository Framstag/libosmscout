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

#include <osmscout/import/GenWayAreaDat.h>

#include <algorithm>

#include <osmscout/DataFile.h>

#include <osmscout/system/Assert.h>

#include <osmscout/util/Geometry.h>
#include <osmscout/util/StopClock.h>
#include <osmscout/util/String.h>

#include <osmscout/import/RawNode.h>
#include <osmscout/import/RawRelation.h>
#include <osmscout/import/RawWay.h>

namespace osmscout {

  std::string WayAreaDataGenerator::GetDescription() const
  {
    return "Generate 'wayarea.tmp'";
  }

  bool WayAreaDataGenerator::ReadWayBlacklist(const ImportParameter& parameter,
                                              Progress& progress,
                                              BlacklistSet& wayBlacklist)
  {
    FileScanner scanner;

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "wayareablack.dat"),
                      FileScanner::Sequential,
                      true)) {
      progress.Error("Cannot open '" + scanner.GetFilename() + "'");
      return false;
    }

    while (!scanner.IsEOF()) {
      OSMId id;

      scanner.ReadNumber(id);

      if (scanner.HasError()) {
        return false;
      }

      wayBlacklist.insert(id);
    }

    return scanner.Close();
  }

  bool WayAreaDataGenerator::GetAreas(const ImportParameter& parameter,
                                      Progress& progress,
                                      const TypeConfig& typeConfig,
                                      std::set<TypeId>& types,
                                      std::set<TypeId>& slowFallbackTypes,
                                      const BlacklistSet& blacklist,
                                      FileScanner& scanner,
                                      std::vector<std::list<RawWayRef> >& areas)
  {
    uint32_t         wayCount=0;
    size_t           collectedWaysCount=0;
    size_t           typesWithWays=0;
    std::set<TypeId> currentTypes(types);

    if (!scanner.GotoBegin()) {
      progress.Error("Error while positioning at start of file");
      return false;
    }

    if (!scanner.Read(wayCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    for (uint32_t w=1; w<=wayCount; w++) {
      RawWayRef way=new RawWay();

      progress.SetProgress(w,wayCount);

      if (!way->Read(typeConfig,
                     scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
            NumberToString(w)+" of "+
            NumberToString(wayCount)+
            " in file '"+
            scanner.GetFilename()+"'");
        return false;
      }

      if (!way->IsArea()) {
        continue;
      }

      if (currentTypes.find(way->GetTypeId())==currentTypes.end()) {
        continue;
      }

      if (way->GetNodeCount()<3) {
        continue;
      }

      if (blacklist.find(way->GetId())!=blacklist.end()) {
        continue;
      }

      if (areas[way->GetTypeId()].empty()) {
        typesWithWays++;
      }

      areas[way->GetTypeId()].push_back(way);

      collectedWaysCount++;

      if (collectedWaysCount>parameter.GetRawWayBlockSize()) {
        for (size_t i=0; i<areas.size(); i++) {
          if (!areas[i].empty() &&
              areas[i].size()>parameter.GetRawWayBlockSize()) {
            progress.Warning("Too many objects for type "+
                             typeConfig.GetTypeInfo(i)->GetName()+
                             " ("+NumberToString(areas[i].size())+
                             "), mark it for low memory fall back");

            collectedWaysCount-=areas[i].size();
            areas[i].clear();

            typesWithWays--;
            types.erase(i);
            currentTypes.erase(i);
            slowFallbackTypes.insert(i);
          }
        }
      }

      while (collectedWaysCount>parameter.GetRawWayBlockSize() &&
          typesWithWays>1) {
        size_t victimType=areas.size();

        // Find the type with the smallest amount of ways loaded
        for (size_t i=0; i<areas.size(); i++) {
          if (!areas[i].empty() &&
              (victimType>=areas.size() ||
               (areas[i].size()<areas[victimType].size()))) {
            victimType=i;
          }
        }

        if (victimType<areas.size()) {
          collectedWaysCount-=areas[victimType].size();
          areas[victimType].clear();

          typesWithWays--;
          currentTypes.erase(victimType);
        }
      }

      if (typesWithWays==0) {
        break;
      }
    }

    for (std::set<TypeId>::const_iterator type=currentTypes.begin();
         type!=currentTypes.end();
         ++type) {
      types.erase(*type);
    }

    progress.SetAction("Collected "+NumberToString(collectedWaysCount)+" ways for "+NumberToString(currentTypes.size())+" types");

    return true;
  }

  bool WayAreaDataGenerator::WriteArea(const ImportParameter& parameter,
                                       Progress& progress,
                                       const TypeConfig& typeConfig,
                                       FileWriter& writer,
                                       uint32_t& writtenWayCount,
                                       const CoordDataFile::CoordResultMap& coordsMap,
                                       const RawWay& rawWay)
  {
    Area       area;
    OSMId      wayId=rawWay.GetId();
    Area::Ring ring;

    ring.attributes.SetFeatures(typeConfig,
                                rawWay.GetFeatureValueBuffer());

    ring.SetType(rawWay.GetTypeId());
    ring.ring=Area::outerRingId;
    ring.ids.resize(rawWay.GetNodeCount());
    ring.nodes.resize(rawWay.GetNodeCount());

    bool success=true;
    for (size_t n=0; n<rawWay.GetNodeCount(); n++) {
      CoordDataFile::CoordResultMap::const_iterator coord=coordsMap.find(rawWay.GetNodeId(n));

      if (coord==coordsMap.end()) {
        progress.Error("Cannot resolve node with id "+
                       NumberToString(rawWay.GetNodeId(n))+
                       " for Way "+
                       NumberToString(wayId));
        success=false;
        break;
      }

      ring.ids[n]=coord->second.point.GetId();

      ring.nodes[n]=coord->second.point.GetCoords();
    }

    if (!success) {
      return true;
    }

    if (parameter.GetStrictAreas() &&
        !AreaIsSimple(ring.nodes)) {
      progress.Error("Area "+NumberToString(wayId)+" of type '"+typeConfig.GetTypeInfo(area.GetType())->GetName()+"' is not simple");
      return true;
    }

    area.rings.push_back(ring);

    if (!writer.Write(wayId)) {
      return false;
    }

    if (!area.Write(typeConfig,
                    writer)) {
      return false;
    }

    writtenWayCount++;

    return true;
  }

  bool WayAreaDataGenerator::HandleLowMemoryFallback(const ImportParameter& parameter,
                                                     Progress& progress,
                                                     const TypeConfig& typeConfig,
                                                     FileScanner& scanner,
                                                     std::set<TypeId>& types,
                                                     const BlacklistSet& blacklist,
                                                     FileWriter& writer,
                                                     uint32_t& writtenWayCount,
                                                     const CoordDataFile& coordDataFile)
  {
    uint32_t wayCount=0;
    size_t   collectedWaysCount=0;

    if (!scanner.GotoBegin()) {
      progress.Error("Error while positioning at start of file");
      return false;
    }

    if (!scanner.Read(wayCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    for (uint32_t w=1; w<=wayCount; w++) {
      RawWayRef way=new RawWay();

      progress.SetProgress(w,wayCount);

      if (!way->Read(typeConfig,
                     scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
            NumberToString(w)+" of "+
            NumberToString(wayCount)+
            " in file '"+
            scanner.GetFilename()+"'");
        return false;
      }

      if (!way->IsArea()) {
        continue;
      }

      if (types.find(way->GetTypeId())==types.end()) {
        continue;
      }

      if (way->GetNodeCount()<3) {
        continue;
      }

      if (blacklist.find(way->GetId())!=blacklist.end()) {
        continue;
      }

      collectedWaysCount++;

      std::set<OSMId>               nodeIds;
      CoordDataFile::CoordResultMap coordsMap;

      for (size_t n=0; n<way->GetNodeCount(); n++) {
        nodeIds.insert(way->GetNodeId(n));
      }

      if (!coordDataFile.Get(nodeIds,coordsMap)) {
        std::cerr << "Cannot read nodes!" << std::endl;
        return false;
      }

      nodeIds.clear();

      if (!WriteArea(parameter,
                    progress,
                    typeConfig,
                    writer,
                    writtenWayCount,
                    coordsMap,
                    way)) {
        return false;
      }
    }

    progress.SetAction("Collected "+NumberToString(collectedWaysCount)+" areas for "+NumberToString(types.size())+" types");

    return true;
  }

  bool WayAreaDataGenerator::Import(const TypeConfigRef& typeConfig,
                                    const ImportParameter& parameter,
                                    Progress& progress)
  {
    progress.SetAction("Generate wayarea.tmp");

    std::set<TypeId> areaTypes;
    std::set<TypeId> slowFallbackTypes;

    BlacklistSet     wayBlacklist; //! Map of ways, that should not be handled

    CoordDataFile    coordDataFile("coord.dat");
    FileScanner      scanner;
    FileWriter       areaWriter;
    uint32_t         rawWayCount=0;

    uint32_t         writtenWayCount=0;

    typeConfig->GetAreaTypes(areaTypes);

    //
    // load blacklist of wayId as a result from multipolygon relation parsing
    //

    progress.SetAction("Reading way area blacklist");

    if (!ReadWayBlacklist(parameter,
                          progress,
                          wayBlacklist)) {
      return false;
    }

    if (!coordDataFile.Open(parameter.GetDestinationDirectory(),
                            parameter.GetCoordDataMemoryMaped())) {
      std::cerr << "Cannot open coord data file!" << std::endl;
      return false;
    }

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "rawways.dat"),
                      FileScanner::Sequential,
                      parameter.GetRawWayDataMemoryMaped())) {
      progress.Error("Cannot open '" + scanner.GetFilename()+ "'");
      return false;
    }

    if (!scanner.Read(rawWayCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    if (!areaWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "wayarea.tmp"))) {
      progress.Error("Cannot create '" + areaWriter.GetFilename() + "'");
      return false;
    }

    areaWriter.Write(writtenWayCount);

    /* ------ */

    size_t iteration=1;
    while (!areaTypes.empty()) {
      std::vector<std::list<RawWayRef> > areasByType(typeConfig->GetTypes().size());

      //
      // Load type data
      //

      progress.SetAction("Collecting way data by type");

      if (!GetAreas(parameter,
                    progress,
                    typeConfig,
                    areaTypes,
                    slowFallbackTypes,
                    wayBlacklist,
                    scanner,
                    areasByType)) {
        return false;
      }

      progress.SetAction("Collecting node ids");

      std::set<OSMId>               nodeIds;
      CoordDataFile::CoordResultMap coordsMap;

      for (size_t type=0; type<areasByType.size(); type++) {
        for (std::list<RawWayRef>::const_iterator w=areasByType[type].begin();
             w!=areasByType[type].end();
             ++w) {
          RawWayRef areas(*w);

          for (size_t n=0; n<areas->GetNodeCount(); n++) {
            nodeIds.insert(areas->GetNodeId(n));
          }
        }
      }

      if (!nodeIds.empty()) {
        progress.SetAction("Loading "+NumberToString(nodeIds.size())+" nodes");
        if (!coordDataFile.Get(nodeIds,coordsMap)) {
          std::cerr << "Cannot read nodes!" << std::endl;
          return false;
        }

        nodeIds.clear();
      }

      progress.SetAction("Writing ways");

      for (size_t type=0; type<areasByType.size(); type++) {
        for (std::list<RawWayRef>::const_iterator w=areasByType[type].begin();
             w!=areasByType[type].end();
             ++w) {
          RawWayRef rawWay(*w);

          WriteArea(parameter,
                    progress,
                    typeConfig,
                    areaWriter,
                    writtenWayCount,
                    coordsMap,
                    *rawWay);
        }

        areasByType[type].clear();
      }

      iteration++;
    }

    if (!slowFallbackTypes.empty()) {
      progress.SetAction("Handling low memory fall back");

      HandleLowMemoryFallback(parameter,
                              progress,
                              typeConfig,
                              scanner,
                              slowFallbackTypes,
                              wayBlacklist,
                              areaWriter,
                              writtenWayCount,
                              coordDataFile);
    }

    /* -------*/

    if (!scanner.Close()) {
      progress.Error("Cannot close file '" + scanner.GetFilename() + "'");
      return false;
    }

    areaWriter.SetPos(0);
    areaWriter.Write(writtenWayCount);


    if (!areaWriter.Close()) {
      return false;
    }

    // Cleaning up...

    wayBlacklist.clear();

    if (!coordDataFile.Close()) {
      return false;
    }

    progress.Info(NumberToString(rawWayCount) + " raw way(s) read, "+
                  NumberToString(writtenWayCount) + " areas(s) written");

    return true;
  }
}

