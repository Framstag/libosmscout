/*
  This source is part of the libosmscout library
  Copyright (C) 2010  Tim Teulings

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

#include <osmscout/import/GenRelAreaDat.h>

#include <algorithm>

#include <osmscout/TypeFeatures.h>

#include <osmscout/system/Assert.h>

#include <osmscout/util/Geometry.h>
#include <osmscout/import/Preprocess.h>
#include <osmscout/import/GenRawNodeIndex.h>
#include <osmscout/import/GenRawWayIndex.h>
#include <osmscout/import/GenRawRelIndex.h>

namespace osmscout {

  const char*           RelAreaDataGenerator::RELAREA_TMP="relarea.tmp";
  const char*           RelAreaDataGenerator::WAYAREABLACK_DAT="wayareablack.dat";
  static const uint64_t MAX_WAYS=1500;
  static const uint64_t MAX_COORDS=150000;

  /**
    Find a top level role.

    A top level role is a role that is not included by any other unused role ("top level tree
    element").
   */
  std::list<RelAreaDataGenerator::MultipolygonPart>::const_iterator RelAreaDataGenerator::FindTopLevel(const std::list<MultipolygonPart>& rings,
                                                                                                       const GroupingState& state,
                                                                                                       size_t& topIndex)
  {
    size_t i=0;
    for (std::list<MultipolygonPart>::const_iterator r=rings.begin();
         r!=rings.end();
         ++r) {
      if (!state.IsUsed(i)) {
        bool included=false;

        for (size_t x=0; x<state.GetRingCount(); x++) {
          if (!state.IsUsed(x) &&
              state.Includes(i,x)) {
            included=true;
            break;
          }
        }

        if (!included) {
          topIndex=i;
          return r;
        }
      }

      ++i;
    }

    return rings.end();
  }

  /**
    Find a sub role.

    A sub role is a role that is included by the given top role but is not included
    by any other role ("direct child tree element").
   */
  std::list<RelAreaDataGenerator::MultipolygonPart>::const_iterator RelAreaDataGenerator::FindSub(const std::list<MultipolygonPart>& rings,
                                                                                                  size_t topIndex,
                                                                                                  const GroupingState& state,
                                                                                                  size_t& subIndex)
  {
    size_t i=0;
    for (std::list<MultipolygonPart>::const_iterator r=rings.begin();
         r!=rings.end();
         ++r) {
      if (!state.IsUsed(i) &&
          state.Includes(i,topIndex)) {
        bool included=false;

        for (size_t x=0; x<state.GetRingCount(); x++) {
          if (x!=i && !state.IsUsed(x)) {
            included=state.Includes(i,x);

            if (included) {
              break;
            }
          }
        }

        if (!included) {
          subIndex=i;
          return r;
        }
      }

      ++i;
    }

    return rings.end();
  }

  /**
    Recursively consume all direct children and all direct children of that children)
    of the given role.
   */
  void RelAreaDataGenerator::ConsumeSubs(const std::list<MultipolygonPart>& rings,
                                         std::list<MultipolygonPart>& groups,
                                         GroupingState& state,
                                         size_t topIndex,
                                         size_t id)
  {
    std::list<MultipolygonPart>::const_iterator sub;
    size_t                                      subIndex;

    sub=FindSub(rings,topIndex,state,subIndex);
    while (sub!=rings.end()) {
      state.SetUsed(subIndex);
      groups.push_back(*sub);
      groups.back().role.SetRing(id);

      ConsumeSubs(rings,groups,state,subIndex,id+1);

      sub=FindSub(rings,topIndex,state,subIndex);
    }
  }

  bool RelAreaDataGenerator::BuildRings(const TypeConfig& typeConfig,
                                        const ImportParameter& parameter,
                                        Progress& progress,
                                        Id id,
                                        const std::string& name,
                                        const TypeInfoRef& type,
                                        std::list<MultipolygonPart>& parts)
  {
    std::list<MultipolygonPart>                 rings;
    bool                                        allArea=true;

    std::map<Id, std::list<MultipolygonPart*> > partsByEnd;
    std::set<MultipolygonPart*>                 usedParts;

    // First check, if relation only consists of closed areas
    // In this case nothing is to do
    for (const auto& part : parts) {
      if (!part.IsArea()) {
        allArea=false;
      }
    }

    // All parts already are areas, so no further rings to build
    if (allArea) {
      return true;
    }

    // Now build up temporary structures to merge all ways to closed areas
    for (auto& part : parts) {
      if (part.IsArea()) {
        rings.push_back(part);
      }
      else {
        partsByEnd[part.role.GetFrontId()].push_back(&part);
        partsByEnd[part.role.GetBackId()].push_back(&part);
      }
    }

    for (const auto& entry : partsByEnd) {
      if (entry.second.size()<2) {
        progress.Error("Node "+NumberToString(entry.first)+
                       " of way "+NumberToString(entry.second.front()->ways.front()->GetId())+
                       " cannot be joined with any other way of the relation "+
                       NumberToString(id)+" "+name);

        parameter.GetErrorReporter()->ReportRelation(id,
                                                     type,
                                                     "Incomplete or broken relation - cannot join path "+
                                                     NumberToString(entry.second.front()->ways.front()->GetId()));
        return false;
      }

      if (entry.second.size()%2!=0) {
        progress.Error("Node "+NumberToString(entry.first)+
                       " of way "+NumberToString(entry.second.front()->ways.front()->GetId())+
                       " can be joined with uneven number of ways of the relation "+
                       NumberToString(id)+" "+name);
        return false;
      }
    }

    for (auto& entry : partsByEnd) {
      while (!entry.second.empty()) {

        MultipolygonPart* part=entry.second.front();

        entry.second.pop_front();

        if (usedParts.find(part)!=usedParts.end()) {
          continue;
        }

        usedParts.insert(part);

        MultipolygonPart             ring;
        std::list<MultipolygonPart*> ringParts;
        size_t                       nodeCount;
        Id                           backId;

        ring.role.SetType(typeConfig.typeInfoIgnore);
        ring.ways=part->ways;

        ringParts.push_back(part);
        nodeCount=part->role.nodes.size();
        backId=part->role.GetBackId();

        while (true) {
          std::map<Id, std::list<MultipolygonPart*> >::iterator match=partsByEnd.find(backId);

          if (match!=partsByEnd.end()) {
            std::list<MultipolygonPart*>::iterator otherPart;

            // Search for matching part that has the same endpoint (and is not the part itself)
            otherPart=match->second.begin();
            while (otherPart!=match->second.end() &&
                   usedParts.find(*otherPart)!=usedParts.end()) {
              ++otherPart;
            }

            if (otherPart!=match->second.end()) {
              if (backId==(*otherPart)->role.GetFrontId()) {
                backId=(*otherPart)->role.GetBackId();
              }
              else {
                backId=(*otherPart)->role.GetFrontId();
              }

              ring.ways.push_back((*otherPart)->ways.front());

              ringParts.push_back(*otherPart);
              nodeCount+=(*otherPart)->role.nodes.size()-1;

              usedParts.insert(*otherPart);
              match->second.erase(otherPart);

              continue;
            }
          }

          // We have found no match
          break;
        }

        ring.role.nodes.reserve(nodeCount);

        for (std::list<MultipolygonPart*>::const_iterator p=ringParts.begin();
            p!=ringParts.end();
            p++) {
          MultipolygonPart* part=*p;

          if (p==ringParts.begin()) {
            for (size_t i=0; i<part->role.nodes.size(); i++) {
              ring.role.nodes.push_back(part->role.nodes[i]);
            }
          }
          else if (ring.role.GetBackId()==part->role.GetFrontId()) {
            for (size_t i=1; i<part->role.nodes.size(); i++) {
              ring.role.nodes.push_back(part->role.nodes[i]);
            }
          }
          else {
            for (size_t i=1; i<part->role.nodes.size(); i++) {
              size_t idx=part->role.nodes.size()-1-i;

              ring.role.nodes.push_back(part->role.nodes[idx]);
            }
          }
        }

        // During concatenation we might define a closed ring with start==end, but everywhere else
        // in the code we store areas without repeating the start, so we remove the final node again
        if (ring.role.GetBackId()==ring.role.GetFrontId()) {
          ring.role.nodes.pop_back();
        }

        if (parameter.GetStrictAreas() &&
            !AreaIsSimple(ring.role.nodes)) {
          progress.Error("Resolved ring including way "+NumberToString(ring.ways.front()->GetId())+
                         " is not simple for multipolygon relation "+NumberToString(id)+" "+
                         name);

          return false;
        }

        rings.push_back(ring);
      }
    }

    parts=rings;

    return true;
  }

  /**
    Try to resolve a multipolygon relation.

    See http://wiki.openstreetmap.org/wiki/Relation:multipolygon/Algorithm
   */
  bool RelAreaDataGenerator::ResolveMultipolygon(const TypeConfig& typeConfig,
                                                 const ImportParameter& parameter,
                                                 Progress& progress,
                                                 Id id,
                                                 const std::string& name,
                                                 const TypeInfoRef& type,
                                                 std::list<MultipolygonPart>& parts)
  {
    std::list<MultipolygonPart> groups;

    //
    // Ring assignment
    //

    if (!BuildRings(typeConfig,
                    parameter,
                    progress,
                    id,
                    name,
                    type,
                    parts)) {
      return false;
    }

    //
    // Ring grouping
    //

    GroupingState state(parts.size());

    size_t ix=0;
    for (const auto& r1 : parts) {
      size_t jx=0;
      for (const auto& r2 : parts) {
        if (ix!=jx) {
          if (IsAreaSubOfArea(r2.role.nodes,
                              r1.role.nodes)) {
            state.SetIncluded(ix,jx);
          }
        }

        jx++;
      }

      ix++;
    }

    //
    // Multipolygon creation
    //

    while (groups.size()<state.GetRingCount()) {
      std::list<MultipolygonPart>::const_iterator top;
      size_t                                      topIndex=0;

      // Find a ring that is not yet used and that is not contained by another unused ring
      top=FindTopLevel(parts,state,topIndex);

      if (top==parts.end()) {
        progress.Warning("Error during ring grouping for multipolygon relation "+
                         NumberToString(id)+" "+
                         name);
        return false;
      }

      state.SetUsed(topIndex);

      groups.push_back(*top);

      groups.back().role.MarkAsOuterRing();

      if (state.HasIncludes(topIndex)) {
        ConsumeSubs(parts,groups,state,topIndex,Area::outerRingId+1/*groups.back().role.GetRing()+1*/);
      }
    }

    if (groups.empty()) {
      progress.Warning("No groups for multipolygon relation "+NumberToString(id)+" "+
                       name);
      return false;
    }

    //
    // Copy back data
    //

    parts=groups;

    return true;
  }

  bool RelAreaDataGenerator::ComposeAreaMembers(const TypeConfig& typeConfig,
                                                Progress& progress,
                                                const CoordDataFile::ResultMap& coordMap,
                                                const IdRawWayMap& wayMap,
                                                const std::string& name,
                                                const RawRelation& rawRelation,
                                                std::list<MultipolygonPart>& parts)
  {
    for (const auto& member : rawRelation.members) {
      if (member.type==RawRelation::memberRelation) {
        progress.Warning("Unsupported relation reference in relation "+
                         NumberToString(rawRelation.GetId())+" "+
                         rawRelation.GetType()->GetName()+" "+
                         name);
      }
      else if (member.type==RawRelation::memberWay &&
               (member.role=="inner" ||
                member.role=="outer" ||
                member.role.empty())) {
        IdRawWayMap::const_iterator wayEntry=wayMap.find(member.id);

        if (wayEntry==wayMap.end()) {
          progress.Error("Cannot resolve way member "+
                         NumberToString(member.id)+
                         " for relation "+
                         NumberToString(rawRelation.GetId())+" "+
                         rawRelation.GetType()->GetName()+" "+
                         name);

          return false;
        }

        RawWayRef way(wayEntry->second);

        MultipolygonPart part;

        part.role.SetType(typeConfig.typeInfoIgnore);
        part.role.MarkAsMasterRing();
        part.role.nodes.resize(way->GetNodeCount());

        for (size_t n=0; n<way->GetNodeCount(); n++) {
          OSMId                                    osmId=way->GetNodeId(n);
          CoordDataFile::ResultMap::const_iterator coordEntry=coordMap.find(osmId);

          if (coordEntry==coordMap.end()) {
            progress.Error("Cannot resolve node member "+
                           NumberToString(osmId)+
                           " for relation "+
                           NumberToString(rawRelation.GetId())+" "+
                           rawRelation.GetType()->GetName()+" "+
                           name);

            return false;
          }

          part.role.nodes[n].Set(coordEntry->second.GetSerial(),
                                 coordEntry->second.GetCoord());
          }

        part.ways.push_back(way);

        parts.push_back(part);
      }
    }

    return true;
  }

  bool RelAreaDataGenerator::ComposeBoundaryMembers(const TypeConfig& typeConfig,
                                                    Progress& progress,
                                                    const CoordDataFile::ResultMap& coordMap,
                                                    const IdRawWayMap& wayMap,
                                                    const std::map<OSMId,RawRelationRef>& relationMap,
                                                    const Area& relation,
                                                    const std::string& name,
                                                    const RawRelation& rawRelation,
                                                    IdSet& resolvedRelations,
                                                    std::list<MultipolygonPart>& parts)
  {
    for (const auto& member : rawRelation.members) {
      if (member.type==RawRelation::memberRelation) {
        if (member.role=="inner" ||
            member.role=="outer") {
          std::map<OSMId,RawRelationRef>::const_iterator relationEntry=relationMap.find(member.id);

          if (relationEntry==relationMap.end()) {
            progress.Error("Cannot resolve relation member "+
                           NumberToString(member.id)+
                           " for relation "+
                           NumberToString(rawRelation.GetId())+" "+
                           rawRelation.GetType()->GetName()+" "+
                           name);

            return false;
          }

          RawRelationRef childRelation(relationEntry->second);

          resolvedRelations.insert(member.id);

          if (!ComposeBoundaryMembers(typeConfig,
                                      progress,
                                      coordMap,
                                      wayMap,
                                      relationMap,
                                      relation,
                                      name,
                                      *childRelation,
                                      resolvedRelations,
                                      parts)) {
            break;
          }
        }
        else {
          progress.Warning("Ignored boundary relation role '"+member.role+"' in relation "+
                           NumberToString(rawRelation.GetId())+" "+
                           rawRelation.GetType()->GetName()+" "+
                           name);
        }
      }
      else if (member.type==RawRelation::memberWay &&
               (member.role=="inner" ||
                member.role=="outer" ||
                member.role.empty())) {
        IdRawWayMap::const_iterator wayEntry=wayMap.find(member.id);

        if (wayEntry==wayMap.end()) {
          progress.Error("Cannot resolve way member "+
                         NumberToString(member.id)+
                         " for relation "+
                         NumberToString(rawRelation.GetId())+" "+
                         rawRelation.GetType()->GetName()+" "+
                         name);

          return false;
        }

        RawWayRef way(wayEntry->second);

        MultipolygonPart part;

        part.role.SetType(typeConfig.typeInfoIgnore);
        part.role.MarkAsMasterRing();
        part.role.nodes.resize(way->GetNodeCount());

        for (size_t n=0; n<way->GetNodeCount(); n++) {
          OSMId                                    osmId=way->GetNodeId(n);
          CoordDataFile::ResultMap::const_iterator coordEntry=coordMap.find(osmId);

          if (coordEntry==coordMap.end()) {
            progress.Error("Cannot resolve node member "+
                           NumberToString(osmId)+
                           " for relation "+
                           NumberToString(rawRelation.GetId())+" "+
                           rawRelation.GetType()->GetName()+" "+
                           name);

            return false;
          }

          part.role.nodes[n].Set(coordEntry->second.GetSerial(),
                                 coordEntry->second.GetCoord());
        }

        part.ways.push_back(way);

        parts.push_back(part);
      }
    }

    return true;
  }

  bool RelAreaDataGenerator::ResolveMultipolygonMembers(Progress& progress,
                                                        const TypeConfig& typeConfig,
                                                        CoordDataFile& coordDataFile,
                                                        RawWayIndexedDataFile& wayDataFile,
                                                        RawRelationIndexedDataFile& relDataFile,
                                                        IdSet& resolvedRelations,
                                                        const Area& relation,
                                                        const std::string& name,
                                                        const RawRelation& rawRelation,
                                                        std::list<MultipolygonPart>& parts)
  {
    TypeInfoSet                    boundaryTypes(typeConfig);
    TypeInfoRef                    boundaryType;
    std::set<OSMId>                nodeIds;
    std::set<OSMId>                wayIds;
    std::set<OSMId>                pendingRelationIds;
    std::set<OSMId>                visitedRelationIds;

    CoordDataFile::ResultMap       coordMap;
    IdRawWayMap                    wayMap;
    std::map<OSMId,RawRelationRef> relationMap;

    boundaryType=typeConfig.GetTypeInfo("boundary_country");
    assert(boundaryType);
    boundaryTypes.Set(boundaryType);

    boundaryType=typeConfig.GetTypeInfo("boundary_state");
    assert(boundaryType);
    boundaryTypes.Set(boundaryType);

    boundaryType=typeConfig.GetTypeInfo("boundary_county");
    assert(boundaryType);
    boundaryTypes.Set(boundaryType);

    boundaryType=typeConfig.GetTypeInfo("boundary_administrative");
    assert(boundaryType);
    boundaryTypes.Set(boundaryType);

    visitedRelationIds.insert(rawRelation.GetId());

    // Initial collection of all relation and way ids of the top level relation

    bool hasMaxWayError = false;
    
    for (const auto& member : rawRelation.members) {
      if (member.type==RawRelation::memberWay &&
          (member.role=="inner" ||
           member.role=="outer" ||
           member.role.empty())) {
        wayIds.insert(member.id);

        if (wayIds.size()>MAX_WAYS) {
          hasMaxWayError = true;
          continue;
        }
      }
      else if (member.type==RawRelation::memberRelation &&
               (member.role=="inner" ||
                member.role=="outer" ||
                member.role.empty())) {
        if (boundaryTypes.IsSet(rawRelation.GetType())) {
          if (visitedRelationIds.find(member.id)!=visitedRelationIds.end()) {
            progress.Warning("Relation "+
                             NumberToString(member.id)+
                             " is referenced multiple times within relation "+
                             NumberToString(rawRelation.GetId())+" "+name);
            continue;
          }

          if (resolvedRelations.find(member.id)!=resolvedRelations.end()) {
            progress.Error("Found self referencing relation "+
                           NumberToString(member.id)+
                           " during resolving of members of relation "+
                           NumberToString(rawRelation.GetId())+" "+name);
            return false;
          }

          pendingRelationIds.insert(member.id);
        }
        else {
          progress.Warning("Unsupported relation reference in relation "+
                           NumberToString(rawRelation.GetId())+" "+
                           rawRelation.GetType()->GetName()+" "+
                           name);
        }
      }
    }

    // Load child relations recursively and collect more way ids at the same time

    while (!pendingRelationIds.empty()) {
      std::vector<RawRelationRef> childRelations;

      childRelations.reserve(pendingRelationIds.size());

      if (!relDataFile.Get(pendingRelationIds,
                           childRelations)) {
        progress.Error("Cannot resolve child relations of relation "+
                       NumberToString(rawRelation.GetId())+" "+name);
        return false;
      }

      pendingRelationIds.clear();

      for (const auto& childRelation : childRelations) {
        visitedRelationIds.insert(childRelation->GetId());
        relationMap[childRelation->GetId()]=childRelation;

        for (const auto& member : childRelation->members) {
          if (member.type==RawRelation::memberWay &&
              (member.role=="inner" ||
               member.role=="outer" ||
               member.role.empty())) {
            wayIds.insert(member.id);

            if (wayIds.size()>MAX_WAYS) {
              hasMaxWayError = true;
              continue;
            }
          }
          else if (member.type==RawRelation::memberRelation &&
                   (member.role=="inner" ||
                    member.role=="outer" ||
                    member.role.empty())) {
            if (boundaryTypes.IsSet(rawRelation.GetType())) {
              if (visitedRelationIds.find(member.id)!=visitedRelationIds.end()) {
                progress.Warning("Relation "+
                                 NumberToString(member.id)+
                                 " is referenced multiple times within relation "+
                                 NumberToString(rawRelation.GetId())+" "+name);
                continue;;
              }

              if (resolvedRelations.find(member.id)!=resolvedRelations.end()) {
                progress.Error("Found self referencing relation "+
                               NumberToString(member.id)+
                               " during resolving of members of relation "+
                               NumberToString(rawRelation.GetId())+" "+
                               rawRelation.GetType()->GetName()+" "+
                               name);
                return false;
              }

              pendingRelationIds.insert(member.id);
            }
            else {
              progress.Warning("Unsupported relation reference in relation "+
                               NumberToString(rawRelation.GetId())+" "+
                               rawRelation.GetType()->GetName()+" "+
                               name);
            }
          }
        }
      }
    }

    if (hasMaxWayError) {
      progress.Error("Relation "+
                      NumberToString(rawRelation.GetId())+" "+name+
                      " references too many ways (" +
                      NumberToString(wayIds.size())+")");
      return false;
    }
    
    // Now load all ways and collect all coordinate ids

    std::vector<RawWayRef> ways;

    ways.reserve(wayIds.size());

    if (!wayDataFile.Get(wayIds,
                         ways)) {
      progress.Error("Cannot resolve child ways of relation "+
                     NumberToString(rawRelation.GetId())+" "+
                     rawRelation.GetType()->GetName()+" "+
                     name);
      return false;
    }

    wayMap.reserve(ways.size());

    for (const auto& way : ways) {
      for (const auto& osmId : way->GetNodes()) {
        nodeIds.insert(osmId);
      }

      wayMap[way->GetId()]=way;
    }

    wayIds.clear();
    ways.clear();

    // Now load all node coordinates

    if (nodeIds.size()>MAX_COORDS) {
      progress.Error("Relation "+
                     NumberToString(rawRelation.GetId())+" "+name+
                     " references too many nodes (" +
                     NumberToString(nodeIds.size())+")");
      return false;
    }

    if (!coordDataFile.Get(nodeIds,
                           coordMap)) {
      progress.Error("Cannot resolve child nodes of relation "+
                     NumberToString(rawRelation.GetId())+" "+
                     rawRelation.GetType()->GetName()+" "+
                     name);
      return false;
    }

    nodeIds.clear();

    // Now build together everything

    if (boundaryTypes.IsSet(rawRelation.GetType())) {
      return ComposeBoundaryMembers(typeConfig,
                                    progress,
                                    coordMap,
                                    wayMap,
                                    relationMap,
                                    relation,
                                    name,
                                    rawRelation,
                                    resolvedRelations,
                                    parts);
    }
    else {
      return ComposeAreaMembers(typeConfig,
                                progress,
                                coordMap,
                                wayMap,
                                name,
                                rawRelation,
                                parts);
    }
  }

  /**
   * Tries to detect the type of the relation based on the types of the outer rings. In cases where
   * outer rings have different types. Takes the type with occurs most. In the case where there is only
   * one outer ring, return it as copyPart to allow calling code to copy attributes from it.
   *
   * @param parameter
   *    Parameter object
   * @param typeConfig
   *    Type configuration
   * @param rawRelation
   *    The raw relation to analyse (only passed for type and id)
   * @param parts
   *    The list of parts for the raw relation
   * @param copyPart
   *    Optional reference of the part tat defines the outer ring
   * @return
   */
  TypeInfoRef RelAreaDataGenerator::AutodetectRelationType(const ImportParameter& parameter,
                                                           const TypeConfig& typeConfig,
                                                           const RawRelation& rawRelation,
                                                           std::list<MultipolygonPart>& parts,
                                                           std::list<MultipolygonPart>::iterator& copyPart) const
  {
    std::vector<size_t>                                typeCount(typeConfig.GetTypeCount(),0);
    std::vector<std::list<MultipolygonPart>::iterator> partRef(typeConfig.GetTypeCount(),parts.end());

    TypeInfoRef masterType=typeConfig.typeInfoIgnore;

    // Count the occurence of outer types and store the last outer ring found for each type
    for (std::list<MultipolygonPart>::iterator ring=parts.begin(); ring!=parts.end(); ++ring) {
      if (ring->role.IsOuterRing() &&
          ring->IsArea() &&
          ring->role.GetType()!=typeConfig.typeInfoIgnore) {
        typeCount[ring->role.GetType()->GetIndex()]++;
        partRef[ring->role.GetType()->GetIndex()]=ring;
      }
    }

    size_t maxCount=0;
    size_t maxCountType=0;
    size_t countTypes=0;

    for (size_t i=0; i<typeCount.size(); i++) {
      if (typeCount[i]>maxCount) {
        maxCount=typeCount[i];
        maxCountType=i;
        countTypes=1;
      }
      else if (typeCount[i]==maxCount) {
        countTypes++;
      }
    }

    masterType=typeConfig.GetTypes()[maxCountType];

    if (countTypes>1 &&
        masterType!=typeConfig.typeInfoIgnore) {
      parameter.GetErrorReporter()->ReportRelation(rawRelation.GetId(),
                                                   rawRelation.GetType(),
                                                   "Conflicting types for outer ring (choosen type "+
                                                   masterType->GetName()+")");
    }

    if (countTypes==1) {
      copyPart=partRef[masterType->GetIndex()];
    }
    else {
      copyPart=parts.end();
    }

    return masterType;
  }

  bool RelAreaDataGenerator::HandleMultipolygonRelation(const ImportParameter& parameter,
                                                        Progress& progress,
                                                        const TypeConfig& typeConfig,
                                                        IdSet& wayAreaIndexBlacklist,
                                                        CoordDataFile& coordDataFile,
                                                        RawWayIndexedDataFile& wayDataFile,
                                                        RawRelationIndexedDataFile& relDataFile,
                                                        RawRelation& rawRelation,
                                                        const std::string& name,
                                                        Area& relation)
  {
    IdSet resolvedRelations;

    std::list<MultipolygonPart> parts;

    if (!ResolveMultipolygonMembers(progress,
                                    typeConfig,
                                    coordDataFile,
                                    wayDataFile,
                                    relDataFile,
                                    resolvedRelations,
                                    relation,
                                    name,
                                    rawRelation,
                                    parts)) {
      return false;
    }

    // Reconstruct multipolygon relation by applying the multipolygon resolving
    // algorithm as described at
    // http://wiki.openstreetmap.org/wiki/Relation:multipolygon/Algorithm
    if (!ResolveMultipolygon(typeConfig,
                             parameter,
                             progress,
                             rawRelation.GetId(),
                             name,
                             rawRelation.GetType(),
                             parts)) {
      return false;
    }

    // Resolve SegmentAttributes for each ring

    for (auto& ring : parts) {
      if (ring.IsArea() &&
          ring.ways.front()->GetType()!=typeConfig.typeInfoIgnore) {
        ring.role.SetFeatures(ring.ways.front()->GetFeatureValueBuffer());
      }
    }

    // Ring which have a type that is not a valid ring type get their type set to ignore.
    for (auto& ring : parts) {
      if (ring.role.GetType()!=typeConfig.typeInfoIgnore &&
          !ring.role.GetType()->CanBeArea()) {
        parameter.GetErrorReporter()->ReportRelation(rawRelation.GetId(),
                                                     rawRelation.GetType(),
                                                     "Has ring of type "+
                                                     ring.role.GetType()->GetName()+
                                                     " which is not an area type");

        ring.role.SetType(typeConfig.typeInfoIgnore);
      }
    }

    //
    // If the relation itself does not have a type, try to autodetect the type from the outer rings
    //

    Area::Ring masterRing;

    masterRing.SetFeatures(rawRelation.GetFeatureValueBuffer());
    masterRing.MarkAsMasterRing();

    if (masterRing.GetType()==typeConfig.typeInfoIgnore) {
      std::list<MultipolygonPart>::iterator copyPart=parts.end();

      TypeInfoRef masterType=AutodetectRelationType(parameter,
                                                    typeConfig,
                                                    rawRelation,
                                                    parts,
                                                    copyPart);

      if (progress.OutputDebug() && masterType!=typeConfig.typeInfoIgnore) {
        progress.Debug("Autodetecting type of multipolygon relation "+
                       NumberToString(rawRelation.GetId())+" as "+
                       masterType->GetName());
      }

      if (copyPart!=parts.end())  {
        masterRing.SetFeatures(copyPart->role.GetFeatureValueBuffer());
        //copyPart->role.SetType(typeConfig.typeInfoIgnore);
      }
      else {
        masterRing.SetType(masterType);
      }
    }

    if (masterRing.GetType()==typeConfig.typeInfoIgnore) {
      parameter.GetErrorReporter()->ReportRelation(rawRelation.GetId(),
                                                   rawRelation.GetType(),
                                                   "No type");
      return false;
    }

    if (masterRing.GetType()->GetIgnore()) {
      return false;
    }

    // Blacklisting areas

    for (const auto& ring : parts) {
      if (ring.IsArea()) {
        // TODO: We currently blacklist all areas, we only should blacklist all
        // areas that have a type. Because areas without a type are implicitly blacklisted anyway later on.
        // However because we change the type of area rings to typeIgnore above we need some bookkeeping for this
        // to work here.
        // On the other hand do not fill the blacklist until you are sure that the relation will not be rejected.
        wayAreaIndexBlacklist.insert(ring.ways.front()->GetId());
      }
    }

    // (Re)create roles for relation

    bool optimizeAwayMaster=false;

    if (masterRing.HasAnyFeaturesSet()) {
      // Check: Master ring has values and there is one explicit outer ring
      size_t outerRingCount=0;

      for (const auto& ring : parts) {
        if (ring.role.IsOuterRing()) {
          outerRingCount++;

          if (outerRingCount>1) {
            break;
          }
        }
      }

      if (outerRingCount==1) {
        optimizeAwayMaster=true;

        for (auto& ring : parts) {
          if (ring.role.IsOuterRing()) {
            ring.role.SetFeatures(masterRing.GetFeatureValueBuffer());
            break;
          }
        }
      }
    }
    else {
      // Check: The master ring does not have any values and has the same type as all outer rings
      bool outerRingsClean=true;

      for (const auto& ring : parts) {
        if (ring.role.IsOuterRing() &&
            ring.role.GetType()!=masterRing.GetType()) {
          outerRingsClean=false;
          break;
        }
      }

      if (outerRingsClean) {
        optimizeAwayMaster=true;
      }
    }

    if (!optimizeAwayMaster) {
      relation.rings.push_back(masterRing);
    }

    relation.rings.reserve(parts.size()+relation.rings.size());

    for (const auto& ring : parts) {
      assert(!ring.role.nodes.empty());

      relation.rings.push_back(ring.role);
    }

    if (!optimizeAwayMaster) {
      for (auto& ring : relation.rings) {
        if (ring.IsOuterRing() &&
            ring.GetType()==masterRing.GetType()) {
          ring.SetType(typeConfig.typeInfoIgnore);
        }
      }
    }

    // If a ring and the direct child ring have the same type, this is old school style for
    // the child ring being a clip region. We set the type of the child to typeIgnore then...
    for (size_t r=0; r<relation.rings.size(); r++) {
      size_t s=r+1;

      while (s<relation.rings.size() &&
             relation.rings[s].GetRing()==relation.rings[r].GetRing()+1 &&
             relation.rings[s].GetType()==relation.rings[r].GetType()) {
        relation.rings[s].SetType(typeConfig.typeInfoIgnore);
        s++;
      }
    }

    assert(!relation.rings.empty());

    return true;
  }

  std::string RelAreaDataGenerator::ResolveRelationName(const FeatureRef& featureName,
                                                        const RawRelation& rawRelation) const
  {
    for (size_t i=0; i<rawRelation.GetFeatureCount(); i++) {
      if (rawRelation.HasFeature(i) &&
          rawRelation.GetFeature(i).GetFeature()==featureName &&
          rawRelation.GetFeature(i).GetFeature()->HasValue()) {
        NameFeatureValue* value=dynamic_cast<NameFeatureValue*>(rawRelation.GetFeatureValue(i));

        if (value!=NULL) {
          return value->GetName();
        }
      }
    }

    return "";
  }

  void RelAreaDataGenerator::GetDescription(const ImportParameter& /*parameter*/,
                                                 ImportModuleDescription& description) const
  {
    description.SetName("RelAreaDataGenerator");
    description.SetDescription("Resolves raw relations to areas");

    description.AddRequiredFile(CoordDataFile::COORD_DAT);
    description.AddRequiredFile(Preprocess::RAWWAYS_DAT);
    description.AddRequiredFile(Preprocess::RAWRELS_DAT);
    description.AddRequiredFile(RawWayIndexGenerator::RAWWAY_IDX);
    description.AddRequiredFile(RawRelationIndexGenerator::RAWREL_IDX);

    description.AddProvidedTemporaryFile(RELAREA_TMP);
    description.AddProvidedTemporaryFile(WAYAREABLACK_DAT);
  }

  bool RelAreaDataGenerator::Import(const TypeConfigRef& typeConfig,
                                    const ImportParameter& parameter,
                                    Progress& progress)
  {
    IdSet                      wayAreaIndexBlacklist;

    CoordDataFile              coordDataFile;

    RawWayIndexedDataFile      wayDataFile(parameter.GetRawWayIndexCacheSize(),/*dataCache*/0);

    RawRelationIndexedDataFile relDataFile(parameter.GetRawWayIndexCacheSize(),/*dataCache*/0);
    FeatureRef                 featureName(typeConfig->GetFeature(RefFeature::NAME));

    if (!coordDataFile.Open(parameter.GetDestinationDirectory(),
                            parameter.GetCoordDataMemoryMaped())) {
      std::cerr << "Cannot open coord data files!" << std::endl;
      return false;
    }

    if (!wayDataFile.Open(typeConfig,
                          parameter.GetDestinationDirectory(),
                          parameter.GetRawWayIndexMemoryMaped(),
                          parameter.GetRawWayDataMemoryMaped())) {
      std::cerr << "Cannot open raw way data files!" << std::endl;
      return false;
    }

    if (!relDataFile.Open(typeConfig,
                          parameter.GetDestinationDirectory(),
                          true,
                          true)) {
      std::cerr << "Cannot open raw relation data files!" << std::endl;
      return false;
    }

    //
    // Analysing distribution of nodes in the given interval size
    //

    progress.SetAction("Generate relarea.tmp");

    FileScanner         scanner;
    FileWriter          writer;
    uint32_t            rawRelationCount=0;
    uint32_t            writtenRelationCount=0;
    std::vector<size_t> wayTypeCount(typeConfig->GetTypeCount(),0);
    std::vector<size_t> wayNodeTypeCount(typeConfig->GetTypeCount(),0);
    std::vector<size_t> areaTypeCount(typeConfig->GetTypeCount(),0);
    std::vector<size_t> areaNodeTypeCount(typeConfig->GetTypeCount(),0);

    try {
      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   Preprocess::RAWRELS_DAT),
                   FileScanner::Sequential,
                   true);

      scanner.Read(rawRelationCount);

      writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                  RELAREA_TMP));

      writer.Write(writtenRelationCount);

      for (uint32_t r=1; r<=rawRelationCount; r++) {
        progress.SetProgress(r,rawRelationCount);

        RawRelation rawRel;

        rawRel.Read(*typeConfig,
                    scanner);

        // Normally we now also skip an object because of its missing type, but
        // in case of relations things are a little bit more difficult,
        // type might be placed at the outer ring and not on the relation
        // itself, we thus still need to parse the complete relation for
        // type analysis before we can skip it.

        std::string name=ResolveRelationName(featureName,
                                             rawRel);
        Area        rel;

        if (!HandleMultipolygonRelation(parameter,
                                        progress,
                                        *typeConfig,
                                        wayAreaIndexBlacklist,
                                        coordDataFile,
                                        wayDataFile,
                                        relDataFile,
                                        rawRel,
                                        name,
                                        rel)) {
          continue;
        }

        bool valid=true;
        bool dense=true;
        bool big=false;

        for (const auto& ring : rel.rings) {
          if (!ring.IsMasterRing()) {
            if (ring.nodes.size()<3) {
              valid=false;
              break;
            }

            if (!IsValidToWrite(ring.nodes)) {
              dense=false;
              break;
            }

            if (ring.nodes.size()>FileWriter::MAX_NODES) {
              big=true;
              break;
            }
          }
        }

        if (!valid) {
          progress.Warning("Relation "+
                           NumberToString(rawRel.GetId())+" "+
                           rel.GetType()->GetName()+" "+
                           name+" has ring with less than three nodes, skipping");
          parameter.GetErrorReporter()->ReportRelation(rawRel.GetId(),
                                                       rel.GetType(),
                                                       "Ring with less than three nodes (no area)");
          continue;
        }

        if (!dense) {
          progress.Warning("Relation "+
                           NumberToString(rawRel.GetId())+" "+
                           rel.GetType()->GetName()+" "+
                           name+" has ring(s) which nodes are not dense enough to be written, skipping");
          continue;
        }

        if (big) {
          progress.Warning("Relation "+
                           NumberToString(rawRel.GetId())+" "+
                           rel.GetType()->GetName()+" "+
                           name+" has ring(s) with too many nodes, skipping");
          continue;
        }

        areaTypeCount[rel.GetType()->GetIndex()]++;
        for (const auto& ring: rel.rings) {
          if (ring.IsOuterRing()) {
            areaNodeTypeCount[rel.GetType()->GetIndex()]+=ring.nodes.size();
          }
        }

        writer.Write((uint8_t)osmRefRelation);
        writer.Write(rawRel.GetId());

        rel.WriteImport(*typeConfig,
                        writer);

        writtenRelationCount++;
      }

      progress.Info(NumberToString(rawRelationCount)+" relations read"+
                    ", "+NumberToString(writtenRelationCount)+" relations written");

      writer.SetPos(0);
      writer.Write(writtenRelationCount);

      writer.Close();

      if (!(wayDataFile.Close() &&
            coordDataFile.Close())) {
        return false;
      }

      scanner.Close();

      progress.SetAction("Generate wayareablack.dat");

      writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                  WAYAREABLACK_DAT));

      for (const auto& id : wayAreaIndexBlacklist) {
        writer.WriteNumber(id);
      }

      progress.Info(NumberToString(wayAreaIndexBlacklist.size())+" ways written to blacklist");

      progress.Info("Dump statistics");

      for (const auto &type : typeConfig->GetTypes()) {
        size_t idx=type->GetIndex();

        std::string buffer=type->GetName()+": "+
                NumberToString(wayTypeCount[idx])+" "+NumberToString(wayNodeTypeCount[idx])+" "+
                NumberToString(areaTypeCount[idx])+" "+NumberToString(areaNodeTypeCount[idx]);

        progress.Debug(buffer);
      }

      writer.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());

      scanner.CloseFailsafe();
      writer.CloseFailsafe();

      return false;
    }

    return true;
  }
}
