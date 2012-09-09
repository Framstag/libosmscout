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
#include <cassert>

#include <osmscout/DataFile.h>

#include <osmscout/util/Geometry.h>
#include <osmscout/util/StopClock.h>
#include <osmscout/util/String.h>

#include <osmscout/import/RawNode.h>
#include <osmscout/import/RawRelation.h>
#include <osmscout/import/RawWay.h>

namespace osmscout {

  std::string WayDataGenerator::GetDescription() const
  {
    return "Generate 'ways.dat'";
  }

  bool WayDataGenerator::ReadWayBlacklist(const ImportParameter& parameter,
                                          Progress& progress,
                                          BlacklistSet& wayBlacklist)
  {
    FileScanner scanner;

    progress.SetAction("Loading way blacklist");

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "wayblack.dat"),
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

  bool WayDataGenerator::ReadWayEndpoints(const ImportParameter& parameter,
                                          Progress& progress,
                                          const TypeConfig& typeConfig,
                                          EndPointWayMap& endPointWayMap)
  {
    FileScanner scanner;
    uint32_t    rawWayCount=0;

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "rawways.dat"),
                                      parameter.GetRawWayDataMemoryMaped())) {
      progress.Error("Cannot open 'rawways.dat'");
      return false;
    }

    if (!scanner.Read(rawWayCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    for (uint32_t w=1; w<=rawWayCount; w++) {
      progress.SetProgress(w,rawWayCount);

      RawWay rawWay;

      if (!rawWay.Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(w)+" of "+
                       NumberToString(rawWayCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");
        return false;
      }

      if (rawWay.GetType()==typeIgnore) {
        continue;
      }

      if (typeConfig.GetTypeInfo(rawWay.GetType()).GetIgnore()) {
        continue;
      }

      if (rawWay.IsArea()) {
        continue;
      }

      endPointWayMap[rawWay.GetNodes().front()].push_back(rawWay.GetId());
      endPointWayMap[rawWay.GetNodes().back()].push_back(rawWay.GetId());
    }

    if (!scanner.Close()) {
      progress.Error("Cannot close file 'rawways.dat'");
      return false;
    }

    return true;
  }

  bool WayDataGenerator::ReadAreasIncludingEndpoints(const ImportParameter& parameter,
                                                     Progress& progress,
                                                     const TypeConfig& typeConfig,
                                                     const EndPointWayMap& endPointWayMap,
                                                     EndPointAreaSet& endPointAreaSet)
  {
    FileScanner scanner;
    uint32_t    rawWayCount=0;

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "rawways.dat"),
                                      parameter.GetRawWayDataMemoryMaped())) {
      progress.Error("Cannot open 'rawways.dat'");
      return false;
    }

    if (!scanner.Read(rawWayCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    for (uint32_t w=1; w<=rawWayCount; w++) {
      progress.SetProgress(w,rawWayCount);

      RawWay rawWay;

      if (!rawWay.Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(w)+" of "+
                       NumberToString(rawWayCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");
        return false;
      }

      if (rawWay.GetType()==typeIgnore) {
        continue;
      }

      if (typeConfig.GetTypeInfo(rawWay.GetType()).GetIgnore()) {
        continue;
      }

      if (!rawWay.IsArea()) {
        continue;
      }

      for (size_t i=0; i<rawWay.GetNodeCount(); i++) {
        EndPointWayMap::const_iterator nodeUse;

        nodeUse=endPointWayMap.find(rawWay.GetNodeId(i));

        if (nodeUse!=endPointWayMap.end()) {
          endPointAreaSet.insert(rawWay.GetNodeId(i));
        }
      }
    }

    if (!scanner.Close()) {
      progress.Error("Cannot close file 'rawways.dat'");
      return false;
    }

    return true;
  }

  void WayDataGenerator::GetWayMergeCandidates(const RawWay& way,
                                               const EndPointWayMap& endPointWayMap,
                                               const BlacklistSet& wayBlacklist,
                                               std::set<Id>& candidates)
  {
    EndPointWayMap::const_iterator endPoints;

    endPoints=endPointWayMap.find(way.GetNodes().front());

    if (endPoints!=endPointWayMap.end()) {
      for (std::list<Id>::const_iterator id=endPoints->second.begin();
          id!=endPoints->second.end();
          id++) {
        if (*id>way.GetId() &&
            wayBlacklist.find(*id)==wayBlacklist.end()) {
          candidates.insert(*id);
        }
      }
    }

    endPoints=endPointWayMap.find(way.GetNodes().back());

    if (endPoints!=endPointWayMap.end()) {
      for (std::list<Id>::const_iterator id=endPoints->second.begin();
          id!=endPoints->second.end();
          id++) {
        if (*id>way.GetId() &&
            wayBlacklist.find(*id)==wayBlacklist.end()) {
          candidates.insert(*id);
        }
      }
    }
  }

  bool WayDataGenerator::LoadWays(Progress& progress,
                                  FileScanner& scanner,
                                  NumericIndex<Id>& rawWayIndex,
                                  const std::set<Id>& ids,
                                  std::map<Id,RawWayRef>& ways)
  {
    std::vector<FileOffset> offsets;

    if (!rawWayIndex.GetOffsets(ids,offsets)) {
      return false;
    }

    FileOffset oldPos;

    if (!scanner.GetPos(oldPos)){
      progress.Error("Error while getting current file position");
      return false;
    }

    for (std::vector<FileOffset>::const_iterator offset=offsets.begin();
         offset!=offsets.end();
         offset++) {
      if (!scanner.SetPos(*offset)) {
        progress.Error("Error while moving to way at offset " + NumberToString(*offset));
        return false;
      }

      RawWayRef way=new RawWay();

      way->Read(scanner);

      if (scanner.HasError()) {
        progress.Error("Error while loading way at offset " + NumberToString(*offset));
        return false;
      }

      ways[way->GetId()]=way;
    }

    if (!scanner.SetPos(oldPos)) {
      progress.Error("Error while resetting current file position");
      return false;
    }

    return true;
  }

  bool WayDataGenerator::CompareWays(const RawWay& a,
                                     const RawWay& b) const
  {
    if (a.GetType()!=b.GetType()) {
      return false;
    }

    if (a.GetTags().size()!=b.GetTags().size()) {
      return false;
    }

    for (size_t t=0; t<b.GetTags().size(); t++) {
      if (a.GetTags()[t].key!=b.GetTags()[t].key) {
        return false;
      }

      if (a.GetTags()[t].value!=b.GetTags()[t].value) {
        return false;
      }
    }

    return true;
  }

  void WayDataGenerator::UpdateRestrictions(std::multimap<Id,TurnRestrictionRef>& restrictions,
                                            Id oldId,
                                            Id newId)
  {
    return;

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

  bool WayDataGenerator::JoinWays(Progress& progress,
                                  const TypeConfig& typeConfig,
                                  FileScanner& scanner,
                                  std::vector<RawWayRef>& rawWays,
                                  size_t blockCount,
                                  EndPointWayMap& endPointWayMap,
                                  NumericIndex<Id>& rawWayIndex,
                                  BlacklistSet& wayBlacklist,
                                  std::multimap<Id,TurnRestrictionRef>& restrictions,
                                  size_t& mergeCount)
  {
    std::vector<bool>      hasBeenMerged(blockCount, true);
    std::map<Id,RawWayRef> mergedWays;
    bool                   somethingHasMerged=true;
    SilentProgress         silentProgress;

    while (somethingHasMerged) {
      somethingHasMerged=false;

      // Collect all candidate ids for all ways, that still have potential merges
      // to be verified

      std::set<Id> allCandidates;

      for (size_t b=0; b<blockCount; b++) {
        if (hasBeenMerged[b] &&
            !rawWays[b]->IsArea()) {
          GetWayMergeCandidates(rawWays[b],
                                endPointWayMap,
                                wayBlacklist,
                                allCandidates);
        }
      }

      if (allCandidates.empty()) {
        continue;
      }

      // Now load all candidate ways by id as calculated above

      std::map<Id,RawWayRef> ways;

      progress.Info("Loading " + NumberToString(allCandidates.size())+ " candidate(s)");
      if (!LoadWays(progress,
                    scanner,
                    rawWayIndex,
                    allCandidates,
                    ways)) {
        return false;
      }

      allCandidates.clear();

      progress.Info("Merging");
      for (size_t b=0; b<blockCount; b++) {
        if (hasBeenMerged[b] &&
            !rawWays[b]->IsArea()) {
          RawWayRef rawWay=rawWays[b];

          hasBeenMerged[b]=false;

          // The way has already been merged (within this block)
          if (wayBlacklist.find(rawWay->GetId())!=wayBlacklist.end()) {
            continue;
          }

          SegmentAttributes origAttributes;
          std::vector<Tag>  origTags(rawWay->GetTags());
          bool              origReverseNodes;

          origAttributes.type=rawWay->GetType();
          if (!origAttributes.SetTags(silentProgress,
                                      typeConfig,
                                      rawWay->GetId(),
                                      rawWay->IsArea(),
                                      origTags,
                                      origReverseNodes)) {
            continue;
          }

          std::set<Id> candidates;

          GetWayMergeCandidates(rawWay,
                                endPointWayMap,
                                wayBlacklist,
                                candidates);

          for (std::set<Id>::const_iterator id=candidates.begin();
              id!=candidates.end();
              ++id) {
            std::map<Id,RawWayRef>::const_iterator wayEntry;

            wayEntry=ways.find(*id);

            if (wayEntry==ways.end()) {
              progress.Error("Error while loading merge candidate "+
                             NumberToString(*id) +
                             " (Internal error?)");
              continue;
            }

            RawWayRef candidate=wayEntry->second;

            if (candidate->IsArea()) {
              continue;
            }

            // We do not merge against ways, that are already on the blacklist
            // because of previous merges
            if(wayBlacklist.find(*id)!=wayBlacklist.end()) {
              continue;
            }

            // Assure that we do not work on an old version from disk,
            // but take the local version if we already had done merges on them
            std::map<Id,RawWayRef>::const_iterator allreadyMergedCandidate=mergedWays.find(candidate->GetId());

            if (allreadyMergedCandidate!=mergedWays.end()) {
              candidate=allreadyMergedCandidate->second;
            }

            SegmentAttributes matchAttributes;
            std::vector<Tag>  matchTags(candidate->GetTags());
            bool              matchReverseNodes;

            matchAttributes.type=candidate->GetType();
            if (!matchAttributes.SetTags(silentProgress,
                                         typeConfig,
                                         candidate->GetId(),
                                         candidate->IsArea(),
                                         matchTags,
                                         matchReverseNodes)) {
              continue;
            }

            if (origAttributes!=matchAttributes) {
              continue;
            }

            if (origReverseNodes!=matchReverseNodes) {
              continue;
            }

            std::vector<Id> origNodes(rawWay->GetNodes());

            if (origReverseNodes) {
              std::reverse(origNodes.begin(),origNodes.end());
              origReverseNodes=false;
            }

            if (matchReverseNodes) {
              std::vector<Id> matchNodes(candidate->GetNodes());

              std::reverse(matchNodes.begin(),matchNodes.end());
              candidate->SetNodes(matchNodes);
              matchReverseNodes=false;
            }

            if (!origAttributes.IsOneway() &&
                origNodes.front()==candidate->GetNodes().front()) {
              origNodes.reserve(origNodes.size()+candidate->GetNodeCount()-1);

              for (size_t i=1; i<candidate->GetNodeCount(); i++) {
                origNodes.insert(origNodes.begin(),candidate->GetNodeId(i));
              }

              endPointWayMap[origNodes.front()].push_back(rawWay->GetId());
            }
            else if (origNodes.front()==candidate->GetNodes().back()) {
              origNodes.reserve(origNodes.size()+candidate->GetNodeCount()-1);

              for (size_t i=1; i<candidate->GetNodeCount(); i++) {
                origNodes.insert(origNodes.begin(),candidate->GetNodeId(candidate->GetNodeCount()-1-i));
              }

              endPointWayMap[origNodes.front()].push_back(rawWay->GetId());
            }
            else if (origNodes.back()==candidate->GetNodes().front()) {
              origNodes.reserve(origNodes.size()+candidate->GetNodeCount()-1);

              for (size_t i=1; i<candidate->GetNodeCount(); i++) {
                origNodes.push_back(candidate->GetNodeId(i));
              }

              endPointWayMap[origNodes.back()].push_back(rawWay->GetId());
            }
            else if (!origAttributes.IsOneway() &&
                     origNodes.back()==candidate->GetNodes().back()) {
              origNodes.reserve(origNodes.size()+candidate->GetNodeCount()-1);

              for (size_t i=1; i<candidate->GetNodeCount(); i++) {
                origNodes.push_back(candidate->GetNodeId(candidate->GetNodeCount()-1-i));
              }

              endPointWayMap[origNodes.back()].push_back(rawWay->GetId());
            }
            else {
              continue;
            }

            hasBeenMerged[b]=true;
            somethingHasMerged=true;

            UpdateRestrictions(restrictions,
                               candidate->GetId(),
                               rawWay->GetId());

            wayBlacklist.insert(candidate->GetId());

            rawWay->SetNodes(origNodes);

            mergedWays[rawWay->GetId()]=rawWay;

            mergeCount++;

            break;
          }
        }
      }
    }

    return true;
  }

  bool WayDataGenerator::Import(const ImportParameter& parameter,
                                Progress& progress,
                                const TypeConfig& typeConfig)
  {
    progress.SetAction("Generate ways.dat");

    FileScanner                          scanner;
    FileWriter                           writer;
    uint32_t                             rawWayCount=0;

    // List of restrictions for a way
    std::multimap<Id,TurnRestrictionRef> restrictions;

    uint32_t                             writtenWayCount=0;

    EndPointWayMap                       endPointWayMap;
    EndPointAreaSet                      endPointAreaSet;

    BlacklistSet                         wayBlacklist;

#if defined(OSMSCOUT_HASHMAP_HAS_RESERVE)
    endPointWayMap.reserve(2000000);
    endPointAreaSet.reserve(100000);
#endif

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

    //
    // Building a map of way endpoint ids and list of ways having this point as endpoint
    //

    progress.SetAction("Collecting way endpoints");

    if (!ReadWayEndpoints(parameter,
                          progress,
                          typeConfig,
                          endPointWayMap)) {
      return false;
    }

    //
    // Now enriching this map with areas that include this endpoints as node
    //

    progress.SetAction("Enriching way endpoints");

    if (!ReadAreasIncludingEndpoints(parameter,
                                     progress,
                                     typeConfig,
                                     endPointWayMap,
                                     endPointAreaSet)) {
      return false;
    }

    progress.Info(NumberToString(endPointWayMap.size())+ " endpoints collected");

    DataFile<RawNode> nodeDataFile("rawnodes.dat",
                                   "rawnode.idx",
                                   parameter.GetRawNodeDataCacheSize(),
                                   parameter.GetRawNodeIndexCacheSize());
    NumericIndex<Id>  rawWayIndex("rawway.idx",parameter.GetRawWayIndexCacheSize());

    if (!nodeDataFile.Open(parameter.GetDestinationDirectory(),
                           parameter.GetRawNodeIndexMemoryMaped(),
                           parameter.GetRawNodeDataMemoryMaped())) {
      std::cerr << "Cannot open raw node data file!" << std::endl;
      return false;
    }

    if (!rawWayIndex.Open(parameter.GetDestinationDirectory(),
                           parameter.GetRawWayIndexMemoryMaped())) {
      std::cerr << "Cannot open raw way index file!" << std::endl;
      return false;
    }

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "rawways.dat"),
                                      parameter.GetRawWayDataMemoryMaped())) {
      progress.Error("Cannot open 'rawways.dat'");
      return false;
    }

    if (!scanner.Read(rawWayCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "ways.dat"))) {
      progress.Error("Cannot create 'ways.dat'");
      return false;
    }

    writer.Write(writtenWayCount);

    uint32_t               currentWay=1;
    size_t                 mergeCount=0;
    std::vector<RawWayRef> block(parameter.GetRawWayBlockSize());

    while (currentWay<rawWayCount) {
      size_t blockCount=0;

      progress.SetAction("Loading up to " + NumberToString(block.size()) + " ways");
      while (blockCount<block.size() && currentWay<rawWayCount) {
        progress.SetProgress(currentWay,rawWayCount);

        if (block[blockCount].Invalid()) {
          block[blockCount]=new RawWay();
        }

        if (!block[blockCount]->Read(scanner)) {
          progress.Error(std::string("Error while reading data entry ")+
                         NumberToString(currentWay)+" of "+
                         NumberToString(rawWayCount)+
                         " in file '"+
                         scanner.GetFilename()+"'");
          return false;
        }

        currentWay++;

        if (wayBlacklist.find(block[blockCount]->GetId())!=wayBlacklist.end()) {
          continue;
        }

        if (block[blockCount]->GetType()==typeIgnore) {
          continue;
        }

        if (typeConfig.GetTypeInfo(block[blockCount]->GetType()).GetIgnore()) {
          continue;
        }

        if (block[blockCount]->GetNodeCount()<2) {
          progress.Error(std::string("Way ")+
                         NumberToString(block[blockCount]->GetId())+" has only "+
                         NumberToString(block[blockCount]->GetNodeCount())+
                         " node(s) but requires at least 2 nodes");
          continue;
        }


        blockCount++;
      }

      progress.SetAction("Merging ways");
      // Join with potential joined ways
      if (!JoinWays(progress,
                    typeConfig,
                    scanner,
                    block,
                    blockCount,
                    endPointWayMap,
                    rawWayIndex,
                    wayBlacklist,
                    restrictions,
                    mergeCount)) {
        return false;
      }

      progress.SetAction("Loading nodes");

      std::set<Id>                    nodeIds;
      std::vector<RawNodeRef>         nodes;
      OSMSCOUT_HASHMAP<Id,RawNodeRef> nodesMap;

      for (size_t w=0; w<blockCount; w++) {
        for (size_t n=0; n<block[w]->GetNodeCount(); n++) {
          nodeIds.insert(block[w]->GetNodeId(n));
        }
      }

      if (!nodeDataFile.Get(nodeIds,nodes)) {
        std::cerr << "Cannot read nodes!" << std::endl;
        continue;
      }

      nodeIds.clear();

#if defined(OSMSCOUT_HASHMAP_HAS_RESERVE)
      nodesMap.reserve(nodes.size());
#endif

      for (std::vector<RawNodeRef>::const_iterator node=nodes.begin();
          node!=nodes.end();
          node++) {
        nodesMap[(*node)->GetId()]=*node;
      }

      nodes.clear();

      progress.SetAction("Writing ways");

      for (size_t w=0; w<blockCount; w++) {
        // Way has been joined, no need to write it
        BlacklistSet::iterator blacklistEntry=wayBlacklist.find(block[w]->GetId());

        if (blacklistEntry!=wayBlacklist.end()) {
          wayBlacklist.erase(blacklistEntry);

          continue;
        }

        std::vector<Tag> tags(block[w]->GetTags());
        Way              way;
        bool             reverseNodes=false;

        way.SetId(block[w]->GetId());
        way.SetType(block[w]->GetType());

        if (!way.SetTags(progress,
                         typeConfig,
                         block[w]->IsArea(),
                         tags,
                         reverseNodes)) {
          continue;
        }

        way.nodes.resize(block[w]->GetNodeCount());

        bool success=true;
        for (size_t n=0; n<block[w]->GetNodeCount(); n++) {
          OSMSCOUT_HASHMAP<Id,RawNodeRef>::const_iterator node=nodesMap.find(block[w]->GetNodeId(n));

          if (node==nodesMap.end()) {
            progress.Error("Cannot resolve node with id "+
                           NumberToString(block[w]->GetNodeId(n))+
                           " for Way "+
                           NumberToString(way.GetId()));
            success=false;
            break;
          }

          way.nodes[n].Set(node->second->GetId(),
                           node->second->GetLat(),
                           node->second->GetLon());
        }

        if (!success) {
          continue;
        }

        if (reverseNodes) {
          std::reverse(way.nodes.begin(),way.nodes.end());
        }

        // startIsJoint/endIsJoint

        if (way.IsArea()) {
          if (!AreaIsSimple(way.nodes)) {
            progress.Error("Area "+NumberToString(way.GetId())+" of type '"+typeConfig.GetTypeInfo(way.GetType()).GetName()+"' is not simple");

            continue;
          }
        }
        else {
          EndPointWayMap::const_iterator  wayJoint;
          EndPointAreaSet::const_iterator areaJoint;
          std::list<Id>::iterator         jointWayId;
          size_t                          startNodeJointCount=0;
          size_t                          endNodeJointCount=0;

          wayJoint=endPointWayMap.find(way.nodes.front().GetId());

          if (wayJoint!=endPointWayMap.end()) {
            for (std::list<Id>::const_iterator jointWayId=wayJoint->second.begin();
                jointWayId!=wayJoint->second.end();
                ++jointWayId) {
              if (wayBlacklist.find(*jointWayId)==wayBlacklist.end()) {
                startNodeJointCount++;
              }
            }
          }

          if (startNodeJointCount==1) {
            startNodeJointCount=0;
          }

          if (startNodeJointCount==0) {
            areaJoint=endPointAreaSet.find(way.nodes.front().GetId());
          }

          way.SetStartIsJoint(startNodeJointCount>0 || areaJoint!=endPointAreaSet.end());

          wayJoint=endPointWayMap.find(way.nodes.back().GetId());

          if (wayJoint!=endPointWayMap.end()) {
            for (std::list<Id>::const_iterator jointWayId=wayJoint->second.begin();
                jointWayId!=wayJoint->second.end();
                ++jointWayId) {
              if (wayBlacklist.find(*jointWayId)==wayBlacklist.end()) {
                endNodeJointCount++;
              }
            }
          }

          if (endNodeJointCount==1) {
            endNodeJointCount=0;
          }

          if (endNodeJointCount==0) {
            areaJoint=endPointAreaSet.find(way.nodes.back().GetId());
          }

          way.SetEndIsJoint(endNodeJointCount>0 || areaJoint!=endPointAreaSet.end());
        }

        way.Write(writer);
        writtenWayCount++;
      }
    }

    if (!scanner.Close()) {
      progress.Error("Cannot close file 'rawways.dat'");
      return false;
    }

    writer.SetPos(0);
    writer.Write(writtenWayCount);


    if (!writer.Close()) {
      return false;
    }

    // Cleaning up...

    endPointWayMap.clear();
    endPointAreaSet.clear();
    wayBlacklist.clear();

    if (!rawWayIndex.Close()) {
      return false;
    }

    if (!nodeDataFile.Close()) {
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

