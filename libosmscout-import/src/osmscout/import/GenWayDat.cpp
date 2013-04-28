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

#include <osmscout/import/GenWayDat.h>

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

  static inline bool WayByNodeCountSorter(const RawWayRef& a,
                                          const RawWayRef& b)
  {
    return a->GetNodeCount()>b->GetNodeCount();
  }

  void WayDataGenerator::GetWayTypes(const TypeConfig& typeConfig,
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

      if (type->CanBeWay()) {
        types.insert(type->GetId());
      }

      if (type->CanBeArea()) {
        types.insert(type->GetId());
      }
    }
  }

  void WayDataGenerator::SetNodeUsed(NodeUseMap& nodeUseMap,
                                     Id id)
  {
    Id offset=id/16;

    NodeUseMap::iterator entry=nodeUseMap.find(offset);

    if (entry==nodeUseMap.end()) {
      entry=nodeUseMap.insert(std::make_pair(offset,0)).first;
    }

    Id index=(id%16)*2;

    uint32_t data=entry->second;

    if (data & (1<<(index+1))) {
      // do nothing
    }
    else if (data & (1<<index)) {
      entry->second|=(1 << (index+1));
    }
    else {
      entry->second|=(1 << index);
    }
  }

  bool WayDataGenerator::IsNodeUsedAtLeastTwice(const NodeUseMap& nodeUseMap,
                                                Id id) const
  {
    Id offset=id/16;

    NodeUseMap::const_iterator entry=nodeUseMap.find(offset);

    if (entry==nodeUseMap.end()) {
      return false;
    }

    Id index=(id%16)*2+1;

    bool result=entry->second & (1 << index);

    return result;
  }

  std::string WayDataGenerator::GetDescription() const
  {
    return "Generate 'ways.tmp'";
  }

  bool WayDataGenerator::ReadWayBlacklist(const ImportParameter& parameter,
                                          Progress& progress,
                                          BlacklistSet& wayBlacklist)
  {
    FileScanner scanner;

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "wayblack.dat"),
                      FileScanner::Sequential,
                      true)) {
      progress.Error("Cannot open 'wayblack.dat'");
      return false;
    }

    while (!scanner.IsEOF()) {
      Id id;

      scanner.ReadNumber(id);

      if (scanner.HasError()) {
        return false;
      }

      wayBlacklist.insert(id);
    }

    return scanner.Close();
  }

  bool WayDataGenerator::ReadTurnRestrictions(const ImportParameter& parameter,
                                              Progress& progress,
                                              std::multimap<Id,TurnRestrictionRef>& restrictions)
  {
    FileScanner scanner;
    uint32_t    restrictionCount=0;

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "rawturnrestr.dat"),
                      FileScanner::Sequential,
                      true)) {
      progress.Error("Cannot open 'rawturnrestr.dat'");
      return false;
    }

    if (!scanner.Read(restrictionCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    for (uint32_t r=1; r<=restrictionCount; r++) {
      progress.SetProgress(r,restrictionCount);

      TurnRestrictionRef restriction=new TurnRestriction();

      if (!restriction->Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(r)+" of "+
                       NumberToString(restrictionCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");
        return false;
      }

      restrictions.insert(std::make_pair(restriction->GetFrom(),restriction));
      restrictions.insert(std::make_pair(restriction->GetTo(),restriction));
    }

    if (!scanner.Close()) {
      progress.Error("Cannot close file 'rawturnrestr.dat'");
      return false;
    }

    progress.Info(std::string("Read ")+NumberToString(restrictionCount)+" turn restrictions");

    return true;
  }

  bool WayDataGenerator::WriteTurnRestrictions(const ImportParameter& parameter,
                                               Progress& progress,
                                               std::multimap<Id,TurnRestrictionRef>& restrictions)
  {
    std::set<TurnRestrictionRef> restrictionsSet;

    for (std::multimap<Id,TurnRestrictionRef>::const_iterator restriction=restrictions.begin();
        restriction!=restrictions.end();
        ++restriction) {
      restrictionsSet.insert(restriction->second);
    }

    FileWriter writer;

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "turnrestr.dat"))) {
      progress.Error("Cannot create 'turnrestr.dat'");
      return false;
    }

    writer.Write((uint32_t)restrictionsSet.size());

    for (std::set<TurnRestrictionRef>::const_iterator r=restrictionsSet.begin();
        r!=restrictionsSet.end();
        ++r) {
      TurnRestrictionRef restriction=*r;

      restriction->Write(writer);
    }

    if (!writer.Close()) {
      progress.Error("Cannot close file 'turnrestr.dat'");
      return false;
    }

    progress.Info(std::string("Wrote back ")+NumberToString(restrictionsSet.size())+" restrictions");

    return true;
  }

  bool WayDataGenerator::GetWays(const ImportParameter& parameter,
                                 Progress& progress,
                                 const TypeConfig& typeConfig,
                                 std::set<TypeId>& types,
                                 const BlacklistSet& blacklist,
                                 FileScanner& scanner,
                                 std::vector<std::list<RawWayRef> >& ways,
                                 std::vector<std::list<RawWayRef> >& areas,
                                 NodeUseMap& nodeUseMap,
                                bool buildNodeUseMap)
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

      if (buildNodeUseMap) {
        OSMSCOUT_HASHSET<Id> nodeSet;

        for (size_t i=0; i<way->GetNodeCount(); i++) {
          Id nodeId=way->GetNodeId(i);

          if (nodeSet.find(nodeId)==nodeSet.end()) {
            SetNodeUsed(nodeUseMap,nodeId);
          }

          nodeSet.insert(nodeId);
        }
      }

      if (currentTypes.find(way->GetType())==currentTypes.end()) {
        continue;
      }

      if (way->IsArea()) {
        if (way->GetNodeCount()<3) {
          continue;
        }
      }
      else {
        if (way->GetNodeCount()<2) {
          continue;
        }
      }

      if (blacklist.find(way->GetId())!=blacklist.end()) {
        continue;
      }

      if (ways[way->GetType()].empty() &&
          areas[way->GetType()].empty()) {
        typesWithWays++;
      }

      if (way->IsArea()) {
        areas[way->GetType()].push_back(way);
      }
      else {
        ways[way->GetType()].push_back(way);
      }

      collectedWaysCount++;

      while (collectedWaysCount>parameter.GetRawWayBlockSize() &&
             typesWithWays>1) {
        size_t victimType=ways.size();

        // Find the type with the smalest amount of ways loaded
        for (size_t i=0; i<ways.size(); i++) {
          if ((!ways[i].empty() ||
               !areas[i].empty()) &&
              (victimType>=ways.size() ||
               (ways[i].size()+areas[i].size()<ways[victimType].size()+areas[victimType].size()))) {
            victimType=i;
          }
        }

        if (victimType<ways.size()) {
          collectedWaysCount-=ways[victimType].size();
          ways[victimType].clear();

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

  void WayDataGenerator::UpdateRestrictions(std::multimap<Id,TurnRestrictionRef>& restrictions,
                                            Id oldId,
                                            Id newId)
  {
    std::list<TurnRestrictionRef> oldRestrictions;

    std::pair<std::multimap<Id,TurnRestrictionRef>::iterator,
              std::multimap<Id,TurnRestrictionRef>::iterator> hits=restrictions.equal_range(oldId);
    for (std::multimap<Id,TurnRestrictionRef>::iterator hit=hits.first;
        hit!=hits.second;
        ++hit) {
      oldRestrictions.push_back(hit->second);
    }

    restrictions.erase(hits.first,hits.second);

    for (std::list<TurnRestrictionRef>::iterator r=oldRestrictions.begin();
        r!=oldRestrictions.end();
        ++r) {
      TurnRestrictionRef restriction(*r);

      if (restriction->GetFrom()==oldId) {
        restriction->SetFrom(newId);
        restrictions.insert(std::make_pair(restriction->GetFrom(),restriction));
      }
      else if (restriction->GetTo()==oldId) {
        restriction->SetTo(newId);
        restrictions.insert(std::make_pair(restriction->GetTo(),restriction));
      }
    }
  }

  bool WayDataGenerator::IsRestricted(const std::multimap<Id,TurnRestrictionRef>& restrictions,
                                      Id wayId,
                                      Id nodeId) const
  {
    // We have an index entry for turn restriction, where the given way id is
    // "from" or "to" so we can jst check for "via" == nodeId

    std::pair<std::multimap<Id,TurnRestrictionRef>::const_iterator,
              std::multimap<Id,TurnRestrictionRef>::const_iterator> hits=restrictions.equal_range(wayId);
    for (std::multimap<Id,TurnRestrictionRef>::const_iterator hit=hits.first;
        hit!=hits.second;
        ++hit) {
      if (hit->second->GetVia()==nodeId) {
        return true;
      }
    }

    return false;
  }

  bool WayDataGenerator::MergeWays(Progress& progress,
                                   const TypeConfig& typeConfig,
                                   std::list<RawWayRef>& ways,
                                   BlacklistSet& wayBlacklist,
                                   std::multimap<Id,TurnRestrictionRef>& restrictions)
  {
    WaysByNodeMap  waysByNode;
    SilentProgress silentProgress;
    size_t         currentWay;
    size_t         wayCount=ways.size();

    ways.sort(WayByNodeCountSorter);

    for (WayListPtr w=ways.begin();
        w!=ways.end();
        ++w) {
      RawWayRef way(*w);
      Id        firstNodeId=way->GetFirstNodeId();
      Id        lastNodeId=way->GetLastNodeId();

      if (firstNodeId!=lastNodeId) {
        waysByNode[firstNodeId].push_back(w);
        waysByNode[lastNodeId].push_back(w);
      }
    }

    currentWay=1;
    for (WayListPtr w=ways.begin();
         w!=ways.end();
         ++w) {
      RawWayRef way(*w);
      Id        lastNodeId=way->GetLastNodeId();

      progress.SetProgress(currentWay,wayCount);

      currentWay++;

      WaysByNodeMap::iterator lastNodeCandidate=waysByNode.find(lastNodeId);

      // Way is circular
      if (lastNodeCandidate==waysByNode.end()) {
        continue;
      }

      // Nothing to merge at all, since there is only us
      if (lastNodeCandidate->second.size()==1) {
        continue;
      }

      SegmentAttributes origAttributes;
      std::vector<Tag>  origTags(way->GetTags());
      bool              origReverseNodes;

      origAttributes.type=way->GetType();
      if (!origAttributes.SetTags(silentProgress,
                                  typeConfig,
                                  way->GetId(),
                                  way->IsArea(),
                                  origTags,
                                  origReverseNodes)) {
        continue;
      }

      //
      // Remove lastNodeId entry from the WaysByNode map
      //
      lastNodeCandidate->second.remove(w);

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

          assert(candidate->GetId()!=way->GetId());

          SegmentAttributes candidateAttributes;
          std::vector<Tag>  candidateTags(candidate->GetTags());
          bool              candidateReverseNodes;

          candidateAttributes.type=candidate->GetType();
          if (!candidateAttributes.SetTags(silentProgress,
                                           typeConfig,
                                           candidate->GetId(),
                                           candidate->IsArea(),
                                           candidateTags,
                                           candidateReverseNodes)) {
            continue;
          }

          // Wrong direction
          if (origReverseNodes!=candidateReverseNodes) {
            continue;
          }

          //Attributes do not match
          if (origAttributes!=candidateAttributes) {
            continue;
          }

          // Merging ways in the wrong way
          if (lastNodeId==candidate->GetLastNodeId() &&
              origAttributes.IsOneway()) {
            continue;
          }

          if (IsRestricted(restrictions,
                           way->GetId(),
                           lastNodeId)) {
            continue;
          }

          // This is a match
          hasMerged=true;

          wayBlacklist.insert(candidate->GetId());

          UpdateRestrictions(restrictions,
                             candidate->GetId(),
                             way->GetId());

          std::vector<Id> nodes(way->GetNodes());

          nodes.reserve(nodes.size()+candidate->GetNodeCount()-1);

          if (lastNodeId==candidate->GetFirstNodeId()) {
            // Append candidate nodes
            for (size_t i=1; i<candidate->GetNodeCount(); i++) {
              nodes.push_back(candidate->GetNodeId(i));
            }
          }
          else {
            // Prepend candidate nodes
             for (size_t i=1; i<candidate->GetNodeCount(); i++) {
               nodes.push_back(candidate->GetNodeId(candidate->GetNodeCount()-1-i));
             }
          }

          way->SetNodes(nodes);

          WaysByNodeMap::iterator otherEntry;

          // Erase the matched way from the map of ways (entry via the not matched node))
          if (lastNodeId==candidate->GetFirstNodeId()) {
            otherEntry=waysByNode.find(candidate->GetLastNodeId());
          }
          else {
            otherEntry=waysByNode.find(candidate->GetFirstNodeId());
          }

          assert(otherEntry!=waysByNode.end());

          // Erase the matched way from the list of ways to process
          ways.erase(*c);

          otherEntry->second.remove(*c);

          // Erase the matched way from the map of ways (entry via the matched node))
          lastNodeCandidate->second.erase(c);

          // If the resulting entry is empty, delete it, too
          if (lastNodeCandidate->second.empty()) {
            waysByNode.erase(lastNodeCandidate);
          }

          // If the resulting entry is empty, delete it, too
          if (otherEntry->second.empty()) {
            waysByNode.erase(otherEntry);
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

      lastNodeId=way->GetLastNodeId();

      // Add the (possibly new) lastNodeId to the wayByNode map (again))
      // if the way is not now closed
      if (way->GetFirstNodeId()!=lastNodeId) {
        waysByNode[lastNodeId].push_back(w);
      }
    }

    return true;
  }

  bool WayDataGenerator::WriteWay(const ImportParameter& parameter,
                                  Progress& progress,
                                  const TypeConfig& typeConfig,
                                  FileWriter& writer,
                                  uint32_t& writtenWayCount,
                                  const CoordDataFile::CoordResultMap& coordsMap,
                                  const NodeUseMap& nodeUseMap,
                                  const RawWay& rawWay)
  {
    std::vector<Tag> tags(rawWay.GetTags());
    Way              way;
    bool             reverseNodes=false;
    Id               wayId=rawWay.GetId();

    way.SetType(rawWay.GetType());

    if (!way.SetTags(progress,
                     typeConfig,
                     wayId,
                     rawWay.IsArea(),
                     tags,
                     reverseNodes)) {
      return true;
    }

    way.ids.resize(rawWay.GetNodeCount());
    way.nodes.resize(rawWay.GetNodeCount());

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

      if (IsNodeUsedAtLeastTwice(nodeUseMap,rawWay.GetNodeId(n))) {
        way.ids[n]=coord->second.GetId();
      }
      else {
        way.ids[n]=0;
      }

      way.nodes[n].Set(coord->second.GetLat(),
                       coord->second.GetLon());
    }

    if (!success) {
      return true;
    }

    if (reverseNodes) {
      std::reverse(way.ids.begin(),way.ids.end());
      std::reverse(way.nodes.begin(),way.nodes.end());
    }

    // startIsJoint/endIsJoint
    if (way.IsArea()) {
      if (parameter.GetStrictAreas() &&
          !AreaIsSimple(way.nodes)) {
        progress.Error("Area "+NumberToString(wayId)+" of type '"+typeConfig.GetTypeInfo(way.GetType()).GetName()+"' is not simple");
        return true;
      }
    }

    if (!writer.Write(wayId)) {
      return false;
    }

    if (!way.Write(writer)) {
      return false;
    }

    writtenWayCount++;

    return true;
  }

  bool WayDataGenerator::Import(const ImportParameter& parameter,
                                Progress& progress,
                                const TypeConfig& typeConfig)
  {
    progress.SetAction("Generate ways.tmp");

    std::set<TypeId>                     wayTypes;

    BlacklistSet                         wayBlacklist; //! Map of ways, that should be handled

    // List of restrictions for a way
    std::multimap<Id,TurnRestrictionRef> restrictions; //! Map of restrictions

    NodeUseMap                           nodeUseMap;   //! Bitmap of nodes that are at leats used twice

    FileScanner                          scanner;
    FileWriter                           wayWriter;
    uint32_t                             rawWayCount=0;

    uint32_t                             writtenWayCount=0;
    uint32_t                             mergeCount=0;

#if defined(OSMSCOUT_HASHMAP_HAS_RESERVE)
    nodeUseMap.reserve(2000000);
#endif

    GetWayTypes(typeConfig,
                wayTypes);

    //
    // load blacklist of wayId as a result from multipolygon relation parsing
    //

    progress.SetAction("Reading way blacklist");

    if (!ReadWayBlacklist(parameter,
                          progress,
                          wayBlacklist)) {
      return false;
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
                                     "ways.tmp"))) {
      progress.Error("Cannot create 'ways.tmp'");
      return false;
    }

    wayWriter.Write(writtenWayCount);

    /* ------ */

    size_t iteration=1;
    while (!wayTypes.empty()) {
      std::vector<std::list<RawWayRef> > waysByType(typeConfig.GetTypes().size());
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
                   waysByType,
                   areasByType,
                   nodeUseMap,
                   iteration==1)) {
        return false;
      }

      // TODO: only print it, if there is something to merge at all
      progress.SetAction("Merging ways");

      for (size_t type=0; type<waysByType.size(); type++) {
        size_t originalWayCount=waysByType[type].size();

        if (originalWayCount>0) {
          MergeWays(progress,
                    typeConfig,
                    waysByType[type],
                    wayBlacklist,
                    restrictions);

          if (waysByType[type].size()<originalWayCount) {
            progress.Info("Reduced ways of '"+typeConfig.GetTypeInfo(type).GetName()+"' from "+
                          NumberToString(originalWayCount)+" to "+NumberToString(waysByType[type].size())+ " way(s)");
            mergeCount+=originalWayCount-waysByType[type].size();
          }
        }
      }

      progress.SetAction("Collecting node ids");

      std::set<Id>                  nodeIds;
      CoordDataFile::CoordResultMap coordsMap;

      for (size_t type=0; type<waysByType.size(); type++) {
        for (std::list<RawWayRef>::const_iterator w=waysByType[type].begin();
             w!=waysByType[type].end();
             ++w) {
          RawWayRef way(*w);

          for (size_t n=0; n<way->GetNodeCount(); n++) {
            nodeIds.insert(way->GetNodeId(n));
          }
        }
      }

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

      for (size_t type=0; type<waysByType.size(); type++) {
        for (std::list<RawWayRef>::const_iterator w=waysByType[type].begin();
             w!=waysByType[type].end();
             ++w) {
          RawWayRef rawWay(*w);

          WriteWay(parameter,
                   progress,
                   typeConfig,
                   wayWriter,
                   writtenWayCount,
                   coordsMap,
                   nodeUseMap,
                   *rawWay);
        }

        waysByType[type].clear();
      }

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
                   nodeUseMap,
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

    nodeUseMap.clear();
    wayBlacklist.clear();

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

