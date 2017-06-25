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

#include <osmscout/import/GenWayWayDat.h>

#include <algorithm>

#include <osmscout/DataFile.h>
#include <osmscout/TypeDistributionDataFile.h>

#include <osmscout/system/Assert.h>

#include <osmscout/util/Geometry.h>
#include <osmscout/util/StopClock.h>
#include <osmscout/util/String.h>

#include <osmscout/import/RawNode.h>
#include <osmscout/import/RawRelation.h>
#include <osmscout/import/RawWay.h>
#include <osmscout/import/Preprocess.h>

namespace osmscout {

  const char* WayWayDataGenerator::WAYWAY_TMP="wayway.tmp";
  const char* WayWayDataGenerator::TURNRESTR_DAT="turnrestr.dat";

  static inline bool WayByNodeCountSorter(const RawWayRef& a,
                                          const RawWayRef& b)
  {
    return a->GetNodeCount()>b->GetNodeCount();
  }

  void WayWayDataGenerator::GetDescription(const ImportParameter& /*parameter*/,
                                           ImportModuleDescription& description) const
  {
    description.SetName("WayWayDataGenerator");
    description.SetDescription("Merge ways into bigger ways");

    description.AddRequiredFile(TypeDistributionDataFile::DISTRIBUTION_DAT);
    description.AddRequiredFile(Preprocess::RAWWAYS_DAT);
    description.AddRequiredFile(Preprocess::RAWTURNRESTR_DAT);

    description.AddProvidedTemporaryFile(WAYWAY_TMP);
    description.AddProvidedTemporaryFile(TURNRESTR_DAT);
  }

  bool WayWayDataGenerator::ReadTurnRestrictions(const ImportParameter& parameter,
                                                 Progress& progress,
                                                 RestrictionData& restrictions)
  {
    FileScanner scanner;
    uint32_t    restrictionCount=0;

    try {
      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   Preprocess::RAWTURNRESTR_DAT),
                   FileScanner::Sequential,
                   true);

      scanner.Read(restrictionCount);

      for (uint32_t r=1; r<=restrictionCount; r++) {
        progress.SetProgress(r,restrictionCount);

        TurnRestrictionRef restriction=std::make_shared<TurnRestriction>();

        restriction->Read(scanner);

        restrictions.restrictions.insert(std::make_pair(restriction->GetFrom(),restriction));
        restrictions.restrictions.insert(std::make_pair(restriction->GetTo(),restriction));
      }

      progress.Info(std::string("Read ")+NumberToString(restrictionCount)+" turn restrictions");

      scanner.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      return false;
    }

    return true;
  }

  bool WayWayDataGenerator::WriteTurnRestrictions(const ImportParameter& parameter,
                                                  Progress& progress,
                                                  const RestrictionData& restrictions)
  {
    std::set<TurnRestrictionRef> restrictionsSet;

    for (const auto& restriction : restrictions.restrictions) {
      restrictionsSet.insert(restriction.second);
    }

    FileWriter writer;

    try {
      writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                  TURNRESTR_DAT));

      writer.Write((uint32_t)restrictionsSet.size());

      for (const auto &restriction : restrictionsSet) {
        restriction->Write(writer);
      }

      writer.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());

      writer.CloseFailsafe();

      return false;
    }

    progress.Info(std::string("Wrote back ")+NumberToString(restrictionsSet.size())+" restrictions");

    return true;
  }

  bool WayWayDataGenerator::GetWays(const ImportParameter& parameter,
                                     Progress& progress,
                                     const TypeConfig& typeConfig,
                                     TypeInfoSet& types,
                                     FileScanner& scanner,
                                     std::vector<std::list<RawWayRef> >& ways)
  {
    uint32_t    wayCount=0;
    size_t      collectedWaysCount=0;
    size_t      collectedWayNodesCount=0;
    size_t      typesWithWays=0;
    TypeInfoSet currentTypes(types);

    scanner.GotoBegin();

    scanner.Read(wayCount);

    for (uint32_t w=1; w<=wayCount; w++) {
      RawWayRef way=std::make_shared<RawWay>();

      progress.SetProgress(w,wayCount);

      way->Read(typeConfig,
                scanner);

      if (way->IsArea()) {
        continue;
      }

      if (!currentTypes.IsSet(way->GetType())) {
        continue;
      }

      if (ways[way->GetType()->GetIndex()].empty()) {
        typesWithWays++;
      }

      ways[way->GetType()->GetIndex()].push_back(way);

      collectedWaysCount++;
      collectedWayNodesCount+=way->GetNodes().size();

      while ((collectedWaysCount>parameter.GetRawWayBlockSize() ||
              collectedWayNodesCount>parameter.GetRawCoordBlockSize()) &&
             typesWithWays>1) {
        TypeInfoRef victimType;

        // Find the type with the smallest amount of ways loaded
        for (auto &type : currentTypes) {
          if (!ways[type->GetIndex()].empty() &&
              (!victimType ||
               ways[type->GetIndex()].size()<ways[victimType->GetIndex()].size())) {
            victimType=type;
          }
        }

        // If there is more then one type of way, we always must find a "victim" type.
        assert(victimType);

        collectedWaysCount-=ways[victimType->GetIndex()].size();
        for (auto const &way:ways[victimType->GetIndex()]){
          collectedWayNodesCount-=way->GetNodes().size();
        }
        ways[victimType->GetIndex()].clear();

        typesWithWays--;
        currentTypes.Remove(victimType);
      }
    }

    // If we are done, remove all successfully collected types from our list of "not yet collected" types.
    types.Remove(currentTypes);

    progress.SetAction("Collected "+NumberToString(collectedWaysCount)+" ways for "+NumberToString(currentTypes.Size())+" types");

    return true;
  }

  void WayWayDataGenerator::UpdateRestrictions(RestrictionData& restrictions,
                                               OSMId oldId,
                                               OSMId newId)
  {
    std::list<TurnRestrictionRef> oldRestrictions;

    auto hits=restrictions.restrictions.equal_range(oldId);

    for (auto& hit=hits.first; hit!=hits.second; ++hit) {
      oldRestrictions.push_back(hit->second);
    }

    restrictions.restrictions.erase(hits.first,hits.second);

    for (auto &restriction : oldRestrictions) {
      if (restriction->GetFrom()==oldId) {
        restriction->SetFrom(newId);
        restrictions.restrictions.insert(std::make_pair(restriction->GetFrom(),restriction));
      }

      if (restriction->GetTo()==oldId) {
        restriction->SetTo(newId);
        restrictions.restrictions.insert(std::make_pair(restriction->GetTo(),restriction));
      }
    }
  }

  bool WayWayDataGenerator::IsRestricted(const RestrictionData& restrictions,
                                         OSMId wayId,
                                         OSMId nodeId) const
  {
    // We have an index entry for turn restriction, where the given way id is
    // "from" or "to" so we can just check for "via" == nodeId

    auto hits=restrictions.restrictions.equal_range(wayId);

    for (auto hit=hits.first; hit!=hits.second; ++hit) {
      if (hit->second->GetVia()==nodeId) {
        return true;
      }
    }

    return false;
  }

  bool WayWayDataGenerator::MergeWays(Progress& progress,
                                      std::list<RawWayRef>& ways,
                                      RestrictionData& restrictions)
  {
    WaysByNodeMap waysByNode;

    // Sort by decreasing node count to assure that we merge longest ways first
    ways.sort(WayByNodeCountSorter);

    // Index by first node id (if way is not circular)
    for (WayListPtr w=ways.begin();
        w!=ways.end();
        ++w) {
      RawWayRef way(*w);
      OSMId     firstNodeId=way->GetFirstNodeId();
      OSMId     lastNodeId=way->GetLastNodeId();

      if (firstNodeId!=lastNodeId) {
        waysByNode[firstNodeId].push_back(w);
      }
    }

    // Variables for monitoring progress
    SilentProgress silentProgress;
    size_t         currentWay;
    size_t         wayCount=ways.size();

    currentWay=1;
    for (auto &way : ways) {
      OSMId lastNodeId=way->GetLastNodeId();

      progress.SetProgress(currentWay,wayCount);

      currentWay++;

      WaysByNodeMap::iterator lastNodeCandidate=waysByNode.find(lastNodeId);

      // Way is circular (see above) and/or already closed
      if (lastNodeCandidate==waysByNode.end()) {
        continue;
      }

      // If we are a oneway that could be merged with more than
      // one alternative, we skip since we cannot be sure
      // that the merge is correct
      if (way->IsOneway() &&
          lastNodeCandidate->second.size()>2) {
        continue;
      }

      // If restrictions apply to the join point, we cannot merge
      // because the restriction might be broken later (the restriction direction
      // is undefined afterwards)
      bool restricted=false;
      {
#pragma omp critical
        restricted=IsRestricted(restrictions,
                                way->GetId(),
                                lastNodeId);
      }

      if (restricted) {
        continue;
      }

      while (lastNodeCandidate!=waysByNode.end()) {
        bool hasMerged=false;

        for (WayListPtrList::iterator c=lastNodeCandidate->second.begin();
             c!=lastNodeCandidate->second.end();
             ++c) {
          RawWayRef candidate(*(*c));

          // Can happen if we would close a way (something like A => B => A)
          if (candidate->GetId()==way->GetId()) {
            continue;
          }

          // If restrictions apply to the join point, we cannot merge
          // because the restriction might be broken later (the restriction direction
          // is undefined afterwards)
          bool restricted=false;
          {
#pragma omp critical
            restricted=IsRestricted(restrictions,
                                    candidate->GetId(),
                                    lastNodeId);
          }

          if (restricted) {
            continue;
          }

          //Attributes do not match => No candidate
          if (way->GetFeatureValueBuffer()!=candidate->GetFeatureValueBuffer()) {
            continue;
          }

          /*
          if (way->GetNodeCount()+candidate->GetNodeCount()>300) {
            // Do not merge ways that are too big in result.
            //  If we already had the data, we would get the resulting bounding box
            // for a better check (different way could be very different in the desitiy
            // of its nodes and the covered area)
            continue;
          }
           */

          // This is a match
          hasMerged=true;

          {
#pragma omp critical
            UpdateRestrictions(restrictions,
                               candidate->GetId(),
                               way->GetId());
          }
          //
          // Append candidate nodes
          //

          std::vector<OSMId> nodes(way->GetNodes());

          nodes.reserve(nodes.size()+candidate->GetNodeCount()-1);

          for (size_t i=1; i<candidate->GetNodeCount(); i++) {
            nodes.push_back(candidate->GetNodeId(i));
          }

          way->SetNodes(nodes);

          //
          // Cleanup
          //

          WaysByNodeMap::iterator otherEntry;

          // Erase the matched way from the list of ways to process
          ways.erase(*c);

          // Erase the matched way from the map of ways (entry via the matched node)
          lastNodeCandidate->second.erase(c);

          // If the resulting entry is empty, delete it, too, for performance reasons
          if (lastNodeCandidate->second.empty()) {
            waysByNode.erase(lastNodeCandidate);
          }

          // We found a merge
          break;
        }

        // If we have merged a way search for the new candidates
        if (hasMerged) {
          lastNodeId=way->GetLastNodeId();

          lastNodeCandidate=waysByNode.find(lastNodeId);
        }
        else {
          lastNodeCandidate=waysByNode.end();
        }
      }
    }

    return true;
  }

  bool WayWayDataGenerator::SplitLongWays(Progress& progress,
                     std::list<RawWayRef>& ways,
                     CoordDataFile::ResultMap& coordsMap)
  {
    std::list<RawWayRef> newWays;

    size_t currentWay=1;
    size_t wayCount=ways.size();

    for (auto way: ways) {
      //*wayIt;
      double length=0.0;
      bool split = way->GetNodeCount() > 300;
      if ((!split) && way->GetNodeCount() >= 2){
        // check real length (in km)
        auto osmIdIt = way->GetNodes().begin();
        auto endIt = way->GetNodes().end();
        auto prev = coordsMap.find(*osmIdIt);
        osmIdIt ++;
        while (osmIdIt != endIt && (!split)){
          auto current = coordsMap.find(*osmIdIt);
          if (prev == coordsMap.end() || current == coordsMap.end()) {
            split = true;
          }
          else {
            length += GetSphericalDistance(prev->second.GetCoord(), current->second.GetCoord());
          }
          prev = current;
          osmIdIt ++;
        }
        split = length > 30.0;
      }

      progress.SetProgress(currentWay,wayCount);
      currentWay++;

      if (!split) {
        newWays.push_back(way);
        continue;
      }

      std::string msg = "Splitting long way " + NumberToString(way->GetId()) +
        " with " + NumberToString(way->GetNodeCount()) + " nodes";
      if (length > 0.0) {
        msg += " and real length " + std::to_string(length) + " km";
      }
      progress.Debug(msg);

      double segmentLength=0.0;
      size_t segmentNodeCnt=1;
      RawWayRef segment = std::make_shared<RawWay>();

      auto osmIdIt = way->GetNodes().begin();
      auto endIt = way->GetNodes().end();
      auto segmentStart = osmIdIt;
      auto segmentEnd = osmIdIt;

      auto prev = coordsMap.find(*osmIdIt);
      // jump to first valid node
      while (prev==coordsMap.end() && osmIdIt != endIt) {
        progress.Error("Cannot resolve node with id "+
                       NumberToString(*osmIdIt)+
                       " for way "+
                       NumberToString(way->GetId())+
                       ", skipping");
        osmIdIt ++;
        segmentStart = osmIdIt;
        if (osmIdIt != endIt){
          prev = coordsMap.find(*osmIdIt);
        }
      }

      osmIdIt ++;
      segmentEnd=osmIdIt;
      while (osmIdIt!=endIt) {

        if (segment->GetId()==0) {
          segment->SetId(way->GetId());
          segment->SetType(way->GetType(), way->IsArea());
          segment->SetFeatureValueBuffer(way->GetFeatureValueBuffer());
        }

        auto current = coordsMap.find(*osmIdIt);

        if (current!=coordsMap.end()) {
          segmentLength += GetSphericalDistance(prev->second.GetCoord(), current->second.GetCoord());
          segmentNodeCnt ++;
        }

        if (segmentNodeCnt >= 300 || segmentLength > 30.0 || current==coordsMap.end()) {
          auto currentSegmentStart = segmentStart;
          segmentStart=osmIdIt;
          prev = current;
          osmIdIt ++;
          segmentEnd=osmIdIt;

          segment->SetNodes(currentSegmentStart, osmIdIt);
          newWays.push_back(segment);
          //std::cout << "  - New segment " << segment->GetId() <<
          //  " with " << segment->GetNodeCount() << " nodes and real length " << segmentLength << " km" << std::endl;

          // skip invalid nodes
          while (prev==coordsMap.end() && osmIdIt != endIt){
            progress.Error("Cannot resolve node with id "+
                           NumberToString(*osmIdIt)+
                           " for way "+
                           NumberToString(way->GetId())+
                           ", splitting");
            osmIdIt ++;
            segmentStart = osmIdIt;
            if (osmIdIt != endIt){
              prev = coordsMap.find(*osmIdIt);
            }
          }
          // reset segment
          segmentLength=0.0;
          segmentNodeCnt=1;
          segment = std::make_shared<RawWay>();

        }
        else {
          prev = current;
          osmIdIt ++;
          segmentEnd=osmIdIt;
        }
      }
      if (segment->GetId() != 0 && segmentNodeCnt >= 2) {
        segment->SetNodes(segmentStart, segmentEnd);
        newWays.push_back(segment);
        //std::cout << "  - New segment (last) " << segment->GetId() <<
        //    " with " << segment->GetNodeCount() << " nodes and real length " << segmentLength << " km" << std::endl;
      }
    }

    ways.clear();

    for (auto way: newWays) {
      ways.push_back(way);
    }

    return true;
  }

  void WayWayDataGenerator::WriteWay(Progress& progress,
                                     const TypeConfig& typeConfig,
                                     FileWriter& writer,
                                     uint32_t& writtenWayCount,
                                     const CoordDataFile::ResultMap& coordsMap,
                                     const RawWay& rawWay)
  {
    Way   way;
    OSMId wayId=rawWay.GetId();

    way.SetFeatures(rawWay.GetFeatureValueBuffer());

    way.nodes.resize(rawWay.GetNodeCount());

    for (size_t n=0; n<rawWay.GetNodeCount(); n++) {
      auto coord=coordsMap.find(rawWay.GetNodeId(n));

      if (coord==coordsMap.end()) {
        progress.Error("Cannot resolve node with id "+
                       NumberToString(rawWay.GetNodeId(n))+
                       " for Way "+
                       NumberToString(wayId)+
                       ", skipping");
        return;
      }

      way.nodes[n].Set(coord->second.GetSerial(),
                       coord->second.GetCoord());
    }

    if (!IsValidToWrite(way.nodes)) {
      progress.Error("Way coordinates are not dense enough to be written for Way "+
                     NumberToString(wayId)+", skipping");
      return;
    }

    writer.Write((uint8_t)osmRefWay);
    writer.Write(wayId);
    way.Write(typeConfig,
              writer);

    writtenWayCount++;
  }

  bool WayWayDataGenerator::HandleLowMemoryFallback(Progress& progress,
                                                    const TypeConfig& typeConfig,
                                                    FileScanner& scanner,
                                                    const TypeInfoSet& types,
                                                    FileWriter& writer,
                                                    uint32_t& writtenWayCount,
                                                    const CoordDataFile& coordDataFile)
  {
    uint32_t areaCount=0;
    size_t   collectedAreasCount=0;

    scanner.GotoBegin();

    scanner.Read(areaCount);

    for (uint32_t w=1; w<=areaCount; w++) {
      RawWayRef way=std::make_shared<RawWay>();

      progress.SetProgress(w,areaCount);

      way->Read(typeConfig,
                scanner);

      if (way->IsArea()) {
        continue;
      }

      if (way->GetType()->GetIgnore()) {
        continue;
      }

      if (!types.IsSet(way->GetType())) {
        continue;
      }

      if (way->GetNodeCount()<2) {
        continue;
      }

      collectedAreasCount++;

      std::set<OSMId>          nodeIds;
      CoordDataFile::ResultMap coordsMap;

      for (size_t n=0; n<way->GetNodeCount(); n++) {
        nodeIds.insert(way->GetNodeId(n));
      }

      if (!coordDataFile.Get(nodeIds,
                             coordsMap)) {
        progress.Error("Cannot read nodes!");
        return false;
      }

      nodeIds.clear();

      WriteWay(progress,
               typeConfig,
               writer,
               writtenWayCount,
               coordsMap,
               *way);
    }

    progress.SetAction("Collected "+NumberToString(collectedAreasCount)+" areas for "+NumberToString(types.Size())+" types");

    return true;
  }

  bool WayWayDataGenerator::Import(const TypeConfigRef& typeConfig,
                                   const ImportParameter& parameter,
                                   Progress& progress)
  {
    progress.SetAction("Generate wayway.tmp");

    TypeInfoSet                             wayTypes;
    TypeInfoSet                             slowFallbackTypes;

    RestrictionData                         restrictions;

    TypeDistributionDataFile                typeDistributionDataFile;

    FileScanner                             scanner;
    FileWriter                              wayWriter;
    uint32_t                                rawWayCount=0;

    uint32_t                                writtenWayCount=0;
    uint32_t                                mergeCount=0;

    progress.SetAction("Reading type distribution");


    if (!typeDistributionDataFile.Load(*typeConfig,
                                       parameter.GetDestinationDirectory())) {
      progress.Error("Cannot load data file '"+typeDistributionDataFile.GetFilename()+"'");
      return false;
    }

    for (const auto &type : typeConfig->GetTypes()) {
      if (!type->GetIgnore()) {
        if (typeDistributionDataFile.GetWayCount(*type)>=parameter.GetRawWayBlockSize()) {
          slowFallbackTypes.Set(type);
        }
        else {
          wayTypes.Set(type);
        }
      }
    }

    //
    // handling of restriction relations
    //

    progress.SetAction("Reading turn restrictions");

    if (!ReadTurnRestrictions(parameter,
                              progress,
                              restrictions)) {
      return false;
    }

    CoordDataFile coordDataFile;

    if (!coordDataFile.Open(parameter.GetDestinationDirectory(),
                            parameter.GetCoordDataMemoryMaped())) {
      log.Error() << "Cannot open coord data file!";
      return false;
    }

    try {
      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   Preprocess::RAWWAYS_DAT),
                   FileScanner::Sequential,
                   parameter.GetRawWayDataMemoryMaped());

      scanner.Read(rawWayCount);

      wayWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     WAYWAY_TMP));

      wayWriter.Write(writtenWayCount);

      /* ------ */

      while (!wayTypes.Empty()) {
        std::vector<std::list<RawWayRef> > waysByType(typeConfig->GetTypeCount());

        //
        // Load type data
        //

        progress.SetAction("Collecting way data by type");

        if (!GetWays(parameter,
                     progress,
                     *typeConfig,
                     wayTypes,
                     scanner,
                     waysByType)) {
          return false;
        }

        // TODO: only print it, if there is something to merge at all
        progress.SetAction("Merging ways");

#pragma omp parallel for
        for (int64_t typeIdx = 0; typeIdx<(int64_t)typeConfig->GetTypeCount(); typeIdx++) {
          size_t originalWayCount=waysByType[typeIdx].size();

          if (originalWayCount>0) {
            MergeWays(progress,
                      waysByType[typeIdx],
                      restrictions);

#pragma omp critical
            if (waysByType[typeIdx].size()<originalWayCount) {
              progress.Info("Reduced ways of '"+typeConfig->GetTypeInfo(typeIdx)->GetName()+"' from "+
                            NumberToString(originalWayCount)+" to "+NumberToString(waysByType[typeIdx].size())+ " way(s)");
              mergeCount+=originalWayCount-waysByType[typeIdx].size();
            }
          }
        }

        progress.SetAction("Collecting node ids");

        std::set<OSMId>          nodeIds;
        CoordDataFile::ResultMap coordsMap;

        for (size_t type=0; type<waysByType.size(); type++) {
          for (const auto &rawWay : waysByType[type]) {
            for (size_t n=0; n<rawWay->GetNodeCount(); n++) {
              nodeIds.insert(rawWay->GetNodeId(n));
            }
          }
        }

        progress.SetAction("Loading "+NumberToString(nodeIds.size())+" nodes");

        if (!coordDataFile.Get(nodeIds,
                               coordsMap)) {
          progress.Error("Cannot read nodes");

          return false;
        }

        nodeIds.clear();

        // split too long ways again to shorter segments
        progress.SetAction("Splitting too long ways");
#pragma omp parallel for
        for (int64_t typeIdx = 0; typeIdx<(int64_t)typeConfig->GetTypeCount(); typeIdx++) {
          size_t originalWayCount=waysByType[typeIdx].size();

          if (originalWayCount>0) {
            SplitLongWays(progress, waysByType[typeIdx], coordsMap);

#pragma omp critical
            if (waysByType[typeIdx].size()>originalWayCount) {
              progress.Info("Splitted long ways of '"+typeConfig->GetTypeInfo(typeIdx)->GetName()+"' from "+
                            NumberToString(originalWayCount)+" to "+NumberToString(waysByType[typeIdx].size())+ " way(s)");
              mergeCount+=originalWayCount-waysByType[typeIdx].size();
            }
          }
        }

        progress.SetAction("Writing ways");

        for (size_t type=0; type<waysByType.size(); type++) {
          for (const auto &rawWay : waysByType[type]) {
            WriteWay(progress,
                     *typeConfig,
                     wayWriter,
                     writtenWayCount,
                     coordsMap,
                     *rawWay);
          }

          waysByType[type].clear();
        }
      }

      /* -------*/

      if (!slowFallbackTypes.Empty()) {
        progress.Info("Handling low memory fall back (no merging) for the following types");

        for (auto type : slowFallbackTypes) {
          progress.Info("* "+type->GetName());
        }

        HandleLowMemoryFallback(progress,
                                *typeConfig,
                                scanner,
                                slowFallbackTypes,
                                wayWriter,
                                writtenWayCount,
                                coordDataFile);
      }

      /* -------*/

      scanner.Close();

      wayWriter.SetPos(0);
      wayWriter.Write(writtenWayCount);
      wayWriter.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      scanner.CloseFailsafe();
      return false;
    }

    // Cleaning up...

    if (!coordDataFile.Close()) {
      return false;
    }

    progress.SetAction("Storing back updated turn restrictions");

    WriteTurnRestrictions(parameter,
                          progress,
                          restrictions);

    progress.Info(NumberToString(rawWayCount) + " raw way(s) read, "+
                  NumberToString(writtenWayCount) + " way(s) written, "+
                  NumberToString(mergeCount) + " merges");

    return true;
  }
}

