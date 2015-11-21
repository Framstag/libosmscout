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

namespace osmscout {

  /**
    Returns true, if area a is in area b
   */
  inline bool IsAreaInArea(const std::vector<bool>& includes,
                           size_t count,
                           size_t a,
                           size_t b)
  {
    return includes[a*count+b];
  }

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
         r++) {
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
         r!=rings.end(); r++) {
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
      groups.back().role.ring=id;

      ConsumeSubs(rings,groups,state,subIndex,id+1);

      sub=FindSub(rings,topIndex,state,subIndex);
    }
  }

  bool RelAreaDataGenerator::BuildRings(const TypeConfig& typeConfig,
                                        const ImportParameter& parameter,
                                        Progress& progress,
                                        Id id,
                                        const std::string& name,
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
        partsByEnd[part.role.ids.front()].push_back(&part);
        partsByEnd[part.role.ids.back()].push_back(&part);
      }
    }

    for (const auto& entry : partsByEnd) {
      if (entry.second.size()<2) {
        progress.Error("Node "+NumberToString(entry.first)+
                       " of way "+NumberToString(entry.second.front()->ways.front()->GetId())+
                       " cannot be joined with any other way of the relation "+
                       NumberToString(id)+" "+name);
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
        ring.role.ring=Area::outerRingId;
        ring.ways=part->ways;

        ringParts.push_back(part);
        nodeCount=part->role.nodes.size();
        backId=part->role.ids.back();

        while (true) {
          std::map<Id, std::list<MultipolygonPart*> >::iterator match=partsByEnd.find(backId);

          if (match!=partsByEnd.end()) {
            std::list<MultipolygonPart*>::iterator otherPart;

            // Search for matching part that has the same endpoint (and is not the part itself)
            otherPart=match->second.begin();
            while (otherPart!=match->second.end() &&
                   usedParts.find(*otherPart)!=usedParts.end()) {
              otherPart++;
            }

            if (otherPart!=match->second.end()) {
              if (backId==(*otherPart)->role.ids.front()) {
                backId=(*otherPart)->role.ids.back();
              }
              else {
                backId=(*otherPart)->role.ids.front();
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

        ring.role.ids.reserve(nodeCount);
        ring.role.nodes.reserve(nodeCount);

        for (std::list<MultipolygonPart*>::const_iterator p=ringParts.begin();
            p!=ringParts.end();
            p++) {
          MultipolygonPart* part=*p;

          if (p==ringParts.begin()) {
            for (size_t i=0; i<part->role.nodes.size(); i++) {
              ring.role.ids.push_back(part->role.ids[i]);
              ring.role.nodes.push_back(part->role.nodes[i]);
            }
          }
          else if (ring.role.ids.back()==part->role.ids.front()) {
            for (size_t i=1; i<part->role.nodes.size(); i++) {
              ring.role.ids.push_back(part->role.ids[i]);
              ring.role.nodes.push_back(part->role.nodes[i]);
            }
          }
          else {
            for (size_t i=1; i<part->role.nodes.size(); i++) {
              size_t idx=part->role.nodes.size()-1-i;

              ring.role.ids.push_back(part->role.ids[idx]);
              ring.role.nodes.push_back(part->role.nodes[idx]);
            }
          }
        }

        // During concatenation we might define a closed ring with start==end, but everywhere else
        // in the code we store areas without repeating the start, so we remove the final node again
        if (ring.role.ids.back()==ring.role.ids.front()) {
          ring.role.ids.pop_back();
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
      groups.back().role.ring=Area::outerRingId;

      if (state.HasIncludes(topIndex)) {
        ConsumeSubs(parts,groups,state,topIndex,Area::outerRingId+1);
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
                                                const CoordDataFile::CoordResultMap& coordMap,
                                                const IdRawWayMap& wayMap,
                                                const std::string& name,
                                                const RawRelation& rawRelation,
                                                std::list<MultipolygonPart>& parts)
  {
    for (std::vector<RawRelation::Member>::const_iterator member=rawRelation.members.begin();
         member!=rawRelation.members.end();
         member++) {
      if (member->type==RawRelation::memberRelation) {
        progress.Warning("Unsupported relation reference in relation "+
                         NumberToString(rawRelation.GetId())+" "+
                         rawRelation.GetType()->GetName()+" "+
                         name);
      }
      else if (member->type==RawRelation::memberWay &&
               (member->role=="inner" ||
                member->role=="outer" ||
                member->role.empty())) {
        IdRawWayMap::const_iterator wayEntry=wayMap.find(member->id);

        if (wayEntry==wayMap.end()) {
          progress.Error("Cannot resolve way member "+
                         NumberToString(member->id)+
                         " for relation "+
                         NumberToString(rawRelation.GetId())+" "+
                         rawRelation.GetType()->GetName()+" "+
                         name);

          return false;
        }

        RawWayRef way(wayEntry->second);

        MultipolygonPart part;

        part.role.SetType(typeConfig.typeInfoIgnore);
        part.role.ring=Area::masterRingId;
        part.role.ids.reserve(way->GetNodeCount());
        part.role.nodes.reserve(way->GetNodeCount());

        for (const auto& osmId : way->GetNodes()) {
          CoordDataFile::CoordResultMap::const_iterator coordEntry=coordMap.find(osmId);

          if (coordEntry==coordMap.end()) {
            progress.Error("Cannot resolve node member "+
                           NumberToString(osmId)+
                           " for relation "+
                           NumberToString(rawRelation.GetId())+" "+
                           rawRelation.GetType()->GetName()+" "+
                           name);

            return false;
          }

          part.role.ids.push_back(coordEntry->second.point.GetId());
          part.role.nodes.push_back(coordEntry->second.point.GetCoords());
        }

        part.ways.push_back(way);

        parts.push_back(part);
      }
    }

    return true;
  }

  bool RelAreaDataGenerator::ComposeBoundaryMembers(const TypeConfig& typeConfig,
                                                    Progress& progress,
                                                    const CoordDataFile::CoordResultMap& coordMap,
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

        part.role.ring=Area::masterRingId;
        part.role.SetType(typeConfig.typeInfoIgnore);
        part.role.ids.reserve(way->GetNodeCount());
        part.role.nodes.reserve(way->GetNodeCount());

        for (const auto& osmId : way->GetNodes()) {
          CoordDataFile::CoordResultMap::const_iterator coordEntry=coordMap.find(osmId);

          if (coordEntry==coordMap.end()) {
            progress.Error("Cannot resolve node member "+
                           NumberToString(osmId)+
                           " for relation "+
                           NumberToString(rawRelation.GetId())+" "+
                           rawRelation.GetType()->GetName()+" "+
                           name);

            return false;
          }

          part.role.ids.push_back(coordEntry->second.point.GetId());
          part.role.nodes.push_back(coordEntry->second.point.GetCoords());
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

    CoordDataFile::CoordResultMap  coordMap;
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

    for (const auto& member : rawRelation.members) {
      if (member.type==RawRelation::memberWay &&
          (member.role=="inner" ||
           member.role=="outer" ||
           member.role.empty())) {
        wayIds.insert(member.id);
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
                             parts)) {
      return false;
    }

    /*
    if (rawRelation.tags.size()>0) {
      size_t outerRings=0;

      for (std::list<MultipolygonPart>::iterator ring=parts.begin();
           ring!=parts.end();
           ring++) {
        if (ring->role.ring==0) {
          outerRings++;
        }
      }

      if (outerRings==1) {
        for (std::list<MultipolygonPart>::iterator ring=parts.begin();
             ring!=parts.end();
             ring++) {
          if (ring->role.ring==0 &&
              ring->ways.size()==1 &&
              ring->ways.front()->GetTags().size()>0) {
            std::cout << "!!! Relation " << rawRelation.GetId() << " has tags at relation and outer ring!" << std::endl;
          }
        }
      }
    }*/

    // Resolve SegmentAttributes for each ring

    for (auto& ring : parts) {
      if (ring.IsArea() &&
          ring.ways.front()->GetType()!=typeConfig.typeInfoIgnore) {
        ring.role.SetFeatures(ring.ways.front()->GetFeatureValueBuffer());
      }
    }

    // If a ring and the direct child ring have the same type, this is old school style for
    // the child ring being a clip region. We set the type of the child to typeIgnore then...

    for (std::list<MultipolygonPart>::iterator ring=parts.begin();
        ring!=parts.end();
        ++ring) {
      if (ring->IsArea()) {
        std::list<MultipolygonPart>::iterator childRing=ring;

        childRing++;
        while (childRing!=parts.end() &&
               childRing->role.ring==ring->role.ring+1) {

          if (childRing->IsArea() &&
              ring->role.GetType()==childRing->role.GetType()) {
            childRing->role.SetType(typeConfig.typeInfoIgnore);
          }

          childRing++;
        }
      }
    }

    // Ring which have a type that is not a valid ring type get their type set to ignore.
    for (auto& ring : parts) {
      if (ring.role.GetType()!=typeConfig.typeInfoIgnore &&
          !ring.role.GetType()->CanBeArea()) {
        progress.Warning("Multipolygon relation "+NumberToString(rawRelation.GetId())+
                         " has ring of type "+
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
    masterRing.ring=Area::masterRingId;

    if (masterRing.GetType()==typeConfig.typeInfoIgnore) {
      for (auto& ring : parts) {
        if (ring.role.ring==Area::outerRingId &&
            ring.IsArea() &&
            ring.role.GetType()!=typeConfig.typeInfoIgnore) {
          if (masterRing.GetType()==typeConfig.typeInfoIgnore) {
            if (progress.OutputDebug()) {
              progress.Debug("Autodetecting type of multipolygon relation "+
                             NumberToString(rawRelation.GetId())+" as "+
                             ring.role.GetType()->GetName());
            }

            masterRing.SetFeatures(ring.role.GetFeatureValueBuffer());
            ring.role.SetType(typeConfig.typeInfoIgnore);
          }
          else if (masterRing.GetType()!=ring.role.GetType()) {
            progress.Warning("Multipolygon relation "+NumberToString(rawRelation.GetId())+
                             " has conflicting types for outer boundary ("+
                             masterRing.GetType()->GetName()+
                             " vs. "+ring.ways.front()->GetType()->GetName()+")");
          }
        }
      }
    }

    if (masterRing.GetType()==typeConfig.typeInfoIgnore) {
      progress.Warning("Multipolygon relation "+NumberToString(rawRelation.GetId())+" does not have a type, skipping");
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

    relation.rings.push_back(masterRing);

    relation.rings.reserve(parts.size());
    for (const auto& ring : parts) {
      assert(!ring.role.nodes.empty());

      relation.rings.push_back(ring.role);
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

  std::string RelAreaDataGenerator::GetDescription() const
  {
    return "Generate 'relarea.tmp'";
  }

  bool RelAreaDataGenerator::Import(const TypeConfigRef& typeConfig,
                                    const ImportParameter& parameter,
                                    Progress& progress)
  {
    IdSet                      wayAreaIndexBlacklist;

    CoordDataFile              coordDataFile("coord.dat");

    RawWayIndexedDataFile      wayDataFile(parameter.GetRawWayIndexCacheSize());

    RawRelationIndexedDataFile relDataFile(parameter.GetRawWayIndexCacheSize());
    FeatureRef                 featureName(typeConfig->GetFeature(RefFeature::NAME));

    if (!coordDataFile.Open(parameter.GetDestinationDirectory(),
                            parameter.GetCoordDataMemoryMaped())) {
      std::cerr << "Cannot open coord data files!" << std::endl;
      return false;
    }

    if (!wayDataFile.Open(typeConfig,
                          parameter.GetDestinationDirectory(),
                          FileScanner::FastRandom,
                          parameter.GetRawWayIndexMemoryMaped(),
                          FileScanner::FastRandom,
                          parameter.GetRawWayDataMemoryMaped())) {
      std::cerr << "Cannot open raw way data files!" << std::endl;
      return false;
    }

    if (!relDataFile.Open(typeConfig,
                          parameter.GetDestinationDirectory(),
                          FileScanner::FastRandom,
                          true,
                          FileScanner::FastRandom,
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

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "rawrels.dat"),
                      FileScanner::Sequential,
                      true)) {
      progress.Error("Cannot open '"+scanner.GetFilename()+"'");
      return false;
    }

    if (!scanner.Read(rawRelationCount)) {
      progress.Error("Cannot read number of raw relations from data file");
      return false;
    }

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "relarea.tmp"))) {
      progress.Error("Cannot create '"+writer.GetFilename()+"'");
      return false;
    }

    writer.Write(writtenRelationCount);

    for (uint32_t r=1; r<=rawRelationCount; r++) {
      progress.SetProgress(r,rawRelationCount);

      RawRelation rawRel;

      if (!rawRel.Read(*typeConfig,
                       scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(r)+" of "+
                       NumberToString(rawRelationCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");
        return false;
      }

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

      for (const auto& ring : rel.rings) {
        if (ring.ring!=Area::masterRingId) {
          if (ring.nodes.size()<3) {
            valid=false;

            break;
          }

          if (!IsValidToWrite(ring.nodes)) {
            dense=false;
          }
        }
      }

      if (!valid) {
        progress.Warning("Relation "+
                         NumberToString(rawRel.GetId())+" "+
                         rel.GetType()->GetName()+" "+
                         name+" has ring with less than three nodes, skipping");
        continue;
      }

      if (!dense) {
        progress.Warning("Relation "+
                         NumberToString(rawRel.GetId())+" "+
                         rel.GetType()->GetName()+" "+
                         name+" has ring(s) which nodes are not dense enough to be written, skipping");
        continue;
      }

      areaTypeCount[rel.GetType()->GetIndex()]++;
      for (const auto& ring: rel.rings) {
        if (ring.ring==Area::outerRingId) {
          areaNodeTypeCount[rel.GetType()->GetIndex()]+=ring.nodes.size();
        }
      }

      if (!writer.Write((uint8_t)osmRefRelation) ||
          !writer.Write(rawRel.GetId()) ||
          !rel.WriteImport(*typeConfig,
                           writer)) {
        return false;
      }

      writtenRelationCount++;
    }

    progress.Info(NumberToString(rawRelationCount)+" relations read"+
                  ", "+NumberToString(writtenRelationCount)+" relations written");

    writer.SetPos(0);
    writer.Write(writtenRelationCount);

    if (!(scanner.Close() &&
          writer.Close() &&
          wayDataFile.Close() &&
          coordDataFile.Close())) {
      return false;
    }

    progress.SetAction("Generate wayareablack.dat");

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                        "wayareablack.dat"))) {
      progress.Error("Cannot create 'wayblack.dat'");
      return false;
    }

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

    return writer.Close();
  }
}
