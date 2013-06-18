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

  void WayAreaDataGenerator::GetWayTypes(const TypeConfig& typeConfig,
                                         std::set<TypeId>& types) const
  {
    for (std::vector<TypeInfo>::const_iterator type=typeConfig.GetTypes().begin();
        type!=typeConfig.GetTypes().end();
        type++) {
      if (type->GetId()==typeIgnore) {
        continue;
      }

      if (type->GetIgnore()) {
        continue;
      }

      if (type->CanBeArea()) {
        types.insert(type->GetId());
      }
    }
  }

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
      progress.Error("Cannot open 'wayareablack.dat'");
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

  bool WayAreaDataGenerator::GetWays(const ImportParameter& parameter,
                                     Progress& progress,
                                     const TypeConfig& typeConfig,
                                     std::set<TypeId>& types,
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

      if (!way->Read(scanner)) {
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

      if (currentTypes.find(way->GetType())==currentTypes.end()) {
        continue;
      }

      if (way->GetNodeCount()<3) {
        continue;
      }

      if (blacklist.find(way->GetId())!=blacklist.end()) {
        continue;
      }

      if (areas[way->GetType()].empty()) {
        typesWithWays++;
      }

      areas[way->GetType()].push_back(way);

      collectedWaysCount++;

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
    }

    for (std::set<TypeId>::const_iterator type=currentTypes.begin();
         type!=currentTypes.end();
         ++type) {
      types.erase(*type);
    }

    progress.SetAction("Collected "+NumberToString(collectedWaysCount)+" ways for "+NumberToString(currentTypes.size())+" types");

    return true;
  }

  bool WayAreaDataGenerator::WriteWay(const ImportParameter& parameter,
                                      Progress& progress,
                                      const TypeConfig& typeConfig,
                                      FileWriter& writer,
                                      uint32_t& writtenWayCount,
                                      const CoordDataFile::CoordResultMap& coordsMap,
                                      const RawWay& rawWay)
  {
    std::vector<Tag> tags(rawWay.GetTags());
    Area             area;
    OSMId            wayId=rawWay.GetId();


    Area::Ring ring;

    if (!ring.attributes.SetTags(progress,
                                 typeConfig,
                                 tags)) {
      return true;
    }

    ring.SetType(rawWay.GetType());
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

      ring.ids[n]=coord->second.GetId();

      ring.nodes[n].Set(coord->second.GetLat(),
                        coord->second.GetLon());
    }

    if (!success) {
      return true;
    }

    if (parameter.GetStrictAreas() &&
        !AreaIsSimple(ring.nodes)) {
      progress.Error("Area "+NumberToString(wayId)+" of type '"+typeConfig.GetTypeInfo(area.GetType()).GetName()+"' is not simple");
      return true;
    }

    area.rings.push_back(ring);

    if (!writer.Write(wayId)) {
      return false;
    }

    if (!area.Write(writer)) {
      return false;
    }

    writtenWayCount++;

    return true;
  }

  bool WayAreaDataGenerator::Import(const ImportParameter& parameter,
                                    Progress& progress,
                                    const TypeConfig& typeConfig)
  {
    progress.SetAction("Generate wayarea.tmp");

    std::set<TypeId>                        wayTypes;

    BlacklistSet                            wayBlacklist; //! Map of ways, that should be handled

    FileScanner                             scanner;
    FileWriter                              wayWriter;
    uint32_t                                rawWayCount=0;

    uint32_t                                writtenWayCount=0;

    GetWayTypes(typeConfig,
                wayTypes);

    //
    // load blacklist of wayId as a result from multipolygon relation parsing
    //

    progress.SetAction("Reading way area blacklist");

    if (!ReadWayBlacklist(parameter,
                          progress,
                          wayBlacklist)) {
      return false;
    }

    CoordDataFile     coordDataFile("coord.dat");

    if (!coordDataFile.Open(parameter.GetDestinationDirectory(),
                            parameter.GetCoordDataMemoryMaped())) {
      std::cerr << "Cannot open coord data file!" << std::endl;
      return false;
    }

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "rawways.dat"),
                      FileScanner::Sequential,
                      parameter.GetRawWayDataMemoryMaped())) {
      progress.Error("Cannot open 'rawways.dat'");
      return false;
    }

    if (!scanner.Read(rawWayCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    if (!wayWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "wayarea.tmp"))) {
      progress.Error("Cannot create 'wayarea.tmp'");
      return false;
    }

    wayWriter.Write(writtenWayCount);

    /* ------ */

    size_t iteration=1;
    while (!wayTypes.empty()) {
      std::vector<std::list<RawWayRef> > areasByType(typeConfig.GetTypes().size());

      //
      // Load type data
      //

      progress.SetAction("Collecting way data by type");

      if (!GetWays(parameter,
                   progress,
                   typeConfig,
                   wayTypes,
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

      progress.SetAction("Loading "+NumberToString(nodeIds.size())+" nodes");
      if (!coordDataFile.Get(nodeIds,coordsMap)) {
        std::cerr << "Cannot read nodes!" << std::endl;
        return false;
      }

      nodeIds.clear();

      progress.SetAction("Writing ways");

      for (size_t type=0; type<areasByType.size(); type++) {
        for (std::list<RawWayRef>::const_iterator w=areasByType[type].begin();
             w!=areasByType[type].end();
             ++w) {
          RawWayRef rawWay(*w);

          WriteWay(parameter,
                   progress,
                   typeConfig,
                   wayWriter,
                   writtenWayCount,
                   coordsMap,
                   *rawWay);
        }

        areasByType[type].clear();
      }

      iteration++;
    }

    /* -------*/

    if (!scanner.Close()) {
      progress.Error("Cannot close file 'rawways.dat'");
      return false;
    }

    wayWriter.SetPos(0);
    wayWriter.Write(writtenWayCount);


    if (!wayWriter.Close()) {
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

