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

#include <osmscout/import/GenRelationDat.h>

#include <algorithm>
#include <cassert>

#include <osmscout/DataFile.h>

#include <osmscout/Relation.h>

#include <osmscout/util/Geometry.h>

#include <osmscout/import/RawNode.h>
#include <osmscout/import/RawRelation.h>
#include <osmscout/import/RawWay.h>

namespace osmscout {

  class GroupingState
  {
  private:
    size_t rings;
    bool   *used;
    bool   *includes;
    bool   *hasIncludes;

  public:
    GroupingState(size_t rings)
    {
      this->rings=rings;

      used=new bool[rings];
      for (size_t i=0; i<rings; i++) {
        used[i]=false;
      }

      includes=new bool[rings*rings];
      for (size_t i=0; i<rings*rings; i++) {
        includes[i]=false;
      }

      hasIncludes=new bool[rings];
      for (size_t i=0; i<rings; i++) {
        hasIncludes[i]=false;
      }
    }

    ~GroupingState()
    {
      delete [] hasIncludes;
      delete [] includes;
      delete [] used;
    }

    inline size_t GetRingCount() const
    {
      return rings;
    }

    inline void SetUsed(size_t used)
    {
      this->used[used]=true;
    }

    inline bool IsUsed(size_t used) const
    {
      return this->used[used];
    }

    inline void SetIncluded(size_t includer, size_t included)
    {
      hasIncludes[includer]=true;
      includes[included*rings+includer]=true;
    }

    inline bool HasIncludes(size_t includer) const
    {
      return hasIncludes[includer];
    }

    inline bool Includes(size_t included, size_t includer) const
    {
      return includes[included*rings+includer];
    }
  };

  struct MultipolygonPart
  {
    Relation::Role       role;
    std::vector<RawNode> nodes;
    std::list<RawWayRef> ways;

    bool IsArea() const
    {
      if (ways.size()==1) {
        return ways.front()->IsArea() ||
               ways.front()->GetNodes().front()==ways.front()->GetNodes().back();
      }
      else {
        return false;
      }
    }
  };


  static bool ResolveMember(const TypeConfig& typeConfig,
                            Id id,
                            TypeInfo type,
                            const std::string& name,
                            const RawRelation::Member& member,
                            DataFile<RawNode>& nodeDataFile,
                            DataFile<RawWay>& wayDataFile,
                            DataFile<RawRelation>& relationDataFile,
                            std::set<Id>& resolvedRelations,
                            std::vector<Relation::Role>& roles,
                            Progress& progress)
  {
    if (member.type==RawRelation::memberNode) {
      RawNodeRef node;

      if (!nodeDataFile.Get(member.id,node)) {
        progress.Error("Cannot resolve node member "+
                       NumberToString(member.id)+
                       " for relation "+
                       NumberToString(id)+" "+type.GetName()+ " "+name);
        return false;
      }

      Relation::Role role;

      role.role=member.role;
      role.ring=0;
      role.nodes.push_back(Point(node->GetId(),
                           node->GetLat(),
                           node->GetLon()));

      role.attributes.type=node->GetType();

      roles.push_back(role);
    }
    else if (member.type==RawRelation::memberWay) {
      RawWayRef way;

      if (!wayDataFile.Get(member.id,way)) {
        progress.Error("Cannot resolve way member "+
                       NumberToString(member.id)+
                       " for relation "+
                       NumberToString(id)+" "+name);
        return false;
      }

      std::vector<RawNodeRef> nodes;

      if (!nodeDataFile.Get(way->GetNodes(),nodes)) {
        progress.Error("Cannot resolve nodes of way member "+
                       NumberToString(member.id)+
                       " for relation "+
                       NumberToString(id)+" "+name);
        return false;
      }

      bool           reverseNodes=false;
      Relation::Role role;

      role.role=member.role;
      role.ring=0;
      role.attributes.type=way->GetType();

      std::vector<Tag> tags(way->GetTags());

      if (!role.attributes.SetTags(progress,
                                   typeConfig,
                                   way->GetId(),
                                   way->IsArea(),
                                   tags,
                                   reverseNodes)) {
        return false;
      }

      role.nodes.reserve(nodes.size());
      for (size_t i=0; i<nodes.size(); i++) {
        Point point;

        point.Set(nodes[i]->GetId(),
                  nodes[i]->GetLat(),
                  nodes[i]->GetLon());

        role.nodes.push_back(point);
      }

      roles.push_back(role);

      return true;
    }
    else if (member.type==RawRelation::memberRelation) {
      RawRelationRef relation;

      if (resolvedRelations.find(member.id)!=resolvedRelations.end()) {
        progress.Error("Found self referencing relation "+
                       NumberToString(member.id)+
                       " during resolving of members of relation "+
                       NumberToString(id)+" "+name);
        return false;
      }


      if (!relationDataFile.Get(member.id,relation)) {
        progress.Error("Cannot resolve relation member "+
                       NumberToString(member.id)+
                       " for relation "+
                       NumberToString(id)+" "+name);
        return false;
      }

      resolvedRelations.insert(member.id);

      for (size_t m=0; m<relation->members.size(); m++) {
        if (!ResolveMember(typeConfig,
                           id,
                           type,
                           name,
                           relation->members[m],
                           nodeDataFile,
                           wayDataFile,
                           relationDataFile,
                           resolvedRelations,
                           roles,
                           progress)) {
          break;
        }
      }

      return true;
    }

    return false;
  }

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
  static std::list<MultipolygonPart>::const_iterator FindTopLevel(const std::list<MultipolygonPart>& rings,
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
  static std::list<MultipolygonPart>::const_iterator FindSub(const std::list<MultipolygonPart>& rings,
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
    Recursivly consume all direct children and all direct children of that children)
    of the given role.
    */
  static void ConsumeSubs(const std::list<MultipolygonPart>& rings,
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

  static bool BuildRings(Progress& progress,
                         const Relation& relation,
                         std::list<MultipolygonPart>& parts)
  {
    std::list<MultipolygonPart> rings;

    /*
    // Make a local copy of the relation roles
    for (size_t i=0; i<relation.roles.size(); i++) {
      // We can have ways that are of type way and still build a closed shape.
      // We are sure that they build an area and fix the data here
      if (relation.roles[i].nodes.size()>2 &&
          relation.roles[i].nodes.front().GetId()==relation.roles[i].nodes.back().GetId()) {
        relation.roles[i].attributes.flags|=SegmentAttributes::isArea;
        relation.roles[i].nodes.pop_back();
      }

      roles.push_back(relation.roles[i]);
    }*/

    // Try to consume all roles
    while (!parts.empty()) {
      size_t           rolesSelected=1;
      bool             finished=false;
      MultipolygonPart leadingPart=parts.front();
      MultipolygonPart ring;

      parts.pop_front();

      ring.role.ring=0;
      ring.role.role=leadingPart.role.role;
      ring.ways=leadingPart.ways;
      ring.nodes=leadingPart.nodes;

      // TODO: All roles that make up a ring should have the same type

      if (leadingPart.IsArea()) {
        finished=true;
      }

      // Now consume more roles that have the same start or end
      // until all joined points build a closed shape ("current way")
      while (!parts.empty() &&
             !finished) {
        bool found=false;

        // Find a role that continues the current way
        for (std::list<MultipolygonPart>::iterator r=parts.begin();
             r!=parts.end();
             ++r) {
          if (ring.nodes.back().GetId()==r->nodes.front().GetId()) {
            for (size_t i=1; i<r->nodes.size(); i++) {
              ring.nodes.push_back(r->nodes[i]);
            }

            parts.erase(r);

            rolesSelected++;
            found=true;

            break;
          }
          else if (ring.nodes.back().GetId()==r->nodes.back().GetId()) {
            for (size_t i=1; i<r->nodes.size(); i++) {
              ring.nodes.push_back(r->nodes[r->nodes.size()-i-1]);
            }

            parts.erase(r);

            rolesSelected++;
            found=true;
            break;
          }
        }

        if (found) {
          if (ring.nodes.front().GetId()==ring.nodes.back().GetId()) {
            finished=true;
          }
        }
        else {
          // if we havn't found another way and we have not closed
          // the current way we have to give up
          progress.Error("Cannot resolve match node "+NumberToString(ring.nodes.back().GetId())+
              " for multipolygon relation "+NumberToString(relation.GetId())+" "+relation.GetName());
          return false;
        }
      }

      // All roles have been consumed and we still have not closed the current way
      if (!finished) {
        progress.Error("No ways left to close current ring for multipolygon relation "+NumberToString(relation.GetId())+
                       " "+relation.GetName());
        return false;
      }

      // During concatination we might define a closed ring with start==end, but everywhere else
      // in the code we store areas without repeating the start, so we remove the final node again
      if (ring.nodes.back().GetId()==ring.nodes.front().GetId()) {
        ring.nodes.pop_back();
      }

      if (!AreaIsSimple(ring.nodes)) {
        progress.Error("Resolved ring is not simple for multipolygon relation "+NumberToString(relation.GetId())+
                       " "+relation.GetName());

        return false;
      }

      rings.push_back(ring);
    }

    parts=rings;

    return true;
  }

  /**
    Try to resolve a multipolygon relation.

    See http://wiki.openstreetmap.org/wiki/Relation:multipolygon/Algorithm
    */
  static bool ResolveMultipolygon(Progress& progress,
                                  const Relation& relation,
                                  std::list<MultipolygonPart>& parts)
  {
    std::list<MultipolygonPart> groups;

    //
    // Ring assignment
    //

    if (!BuildRings(progress,
                    relation,
                    parts)) {
      return false;
    }

    //
    // Ring grouping
    //

    GroupingState state(parts.size());

    size_t i=0;
    for (std::list<MultipolygonPart>::const_iterator r1=parts.begin();
         r1!=parts.end();
         ++r1) {
      size_t j=0;

      for (std::list<MultipolygonPart>::const_iterator r2=parts.begin();
           r2!=parts.end();
           ++r2) {
        if (i!=j &&
            IsAreaInArea(r2->nodes,r1->nodes)) {
          state.SetIncluded(i,j);
        }

        j++;
      }

      i++;
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
        progress.Warning("Error during ring grouping for multipolygon relation "+NumberToString(relation.GetId())+
                         " "+relation.GetName());
        return false;
      }

      state.SetUsed(topIndex);
      groups.push_back(*top);

      if (state.HasIncludes(topIndex)) {
        ConsumeSubs(parts,groups,state,topIndex,1);
      }
    }

    if (groups.empty()) {
      progress.Warning("No groups for multipolygon relation "+NumberToString(relation.GetId())+
                       " "+relation.GetName());
      return false;
    }

    //
    // Copy back data
    //

    parts=groups;

    return true;
  }

  /**
    Merge relations roles where nodes build a connected way (start/end nodes match)
    and where all relevant relations values are the same (no visible difference in drawing)
    thus reducing the number of roles and increasing the number of points per role
    (which gives us the change to optimize better for low magnification).
    */
  /*
  static bool CompactRelation(Relation& relation,
                              const std::string& name,
                              Progress& progress)
  {
    size_t oldSize=relation.roles.size();

    if (oldSize<=1) {
      // Nothing to compact
      return true;
    }

    std::vector<Relation::Role>::iterator role=relation.roles.begin();
    while (role!=relation.roles.end()) {
      bool merged=false;

      std::vector<Relation::Role>::iterator cand=role; // candidate role

      ++cand;

      while (cand!=relation.roles.end()) {
        if (role->GetType()!=cand->GetType() ||
            role->GetFlags()!=cand->GetFlags() ||
            role->GetLayer()!=cand->GetLayer() ||
            role->role!=cand->role ||
            role->GetName()!=cand->GetName() ||
            role->GetRefName()!=cand->GetRefName()) {
          ++cand;
          continue;
        }

        if (role->nodes.front().GetId()==cand->nodes.front().GetId()) {
          role->nodes.reserve(role->nodes.size()+
                              cand->nodes.size()-1);
          for (size_t i=1; i<cand->nodes.size(); i++) {
            role->nodes.insert(role->nodes.begin(),cand->nodes[i]);
          }
          merged=true;
          relation.roles.erase(cand);
        }
        else if (role->nodes.front().GetId()==cand->nodes.back().GetId()) {
          role->nodes.reserve(role->nodes.size()+
                              cand->nodes.size()-1);
          for (size_t i=1; i<cand->nodes.size(); i++) {
            role->nodes.insert(role->nodes.begin(),cand->nodes[cand->nodes.size()-1-i]);
          }
          merged=true;
          relation.roles.erase(cand);
        }
        else if (role->nodes.back().GetId()==cand->nodes.front().GetId()) {
          role->nodes.reserve(role->nodes.size()+
                              cand->nodes.size()-1);
          for (size_t i=1; i<cand->nodes.size(); i++) {
            role->nodes.push_back(cand->nodes[i]);
          }
          merged=true;
          relation.roles.erase(cand);
        }
        else if (role->nodes.back().GetId()==cand->nodes.back().GetId()) {
          role->nodes.reserve(role->nodes.size()+
                              cand->nodes.size()-1);
          for (size_t i=1; i<cand->nodes.size(); i++) {
            role->nodes.push_back(cand->nodes[cand->nodes.size()-1-i]);
          }
          merged=true;
          relation.roles.erase(cand);
        }
        else {
          ++cand;
        }
      }

      if (!merged) {
        ++role;
      }
    }

    if (oldSize!=relation.roles.size()) {
      if (progress.OutputDebug()) {
        progress.Debug("Compacted number of roles of relation "+NumberToString(relation.GetId())+" "+name+
                       " from "+NumberToString(oldSize)+" to "+NumberToString(relation.roles.size()));
      }
    }

    return true;
  }*/

  static bool ResolveMultipolygonMembers(Progress& progress,
                                         const TypeConfig& typeConfig,
                                         std::set<Id>& wayAreaIndexBlacklist,
                                         DataFile<RawNode>& nodeDataFile,
                                         DataFile<RawWay>& wayDataFile,
                                         DataFile<RawRelation>& relDataFile,
                                         std::set<Id>& resolvedRelations,
                                         const Relation& relation,
                                         RawRelation& rawRelation,
                                         std::list<MultipolygonPart>& parts)
  {
    TypeId boundaryId;

    boundaryId=typeConfig.GetWayTypeId("boundary_administrative");

    if (boundaryId==typeIgnore) {
      boundaryId=typeConfig.GetAreaTypeId("boundary_administrative");
    }

    for (std::vector<RawRelation::Member>::const_iterator member=rawRelation.members.begin();
        member!=rawRelation.members.end();
        member++) {
      if (member->type==RawRelation::memberWay &&
          (member->role=="inner" ||
           member->role=="outer" ||
           member->role.empty())) {
        RawWayRef way;

        if (!wayDataFile.Get(member->id,way)) {
          progress.Error("Cannot resolve way member "+
                         NumberToString(member->id)+
                         " for relation "+
                         NumberToString(relation.GetId())+" "+
                         typeConfig.GetTypeInfo(relation.GetType()).GetName()+" "+
                         relation.GetName());
          return false;
        }

        std::vector<RawNodeRef> nodes;

        if (!nodeDataFile.Get(way->GetNodes(),nodes)) {
          progress.Error("Cannot resolve nodes of way member "+
                         NumberToString(member->id)+
                         " for relation "+
                         NumberToString(relation.GetId())+" "+
                         typeConfig.GetTypeInfo(relation.GetType()).GetName()+" "+
                         relation.GetName());
          return false;
        }

        MultipolygonPart part;

        part.role.ring=0;
        part.role.role=member->role;

        for (size_t i=0; i<nodes.size();i++) {
          part.nodes.push_back(*nodes[i]);
        }

        part.ways.push_back(way);

        parts.push_back(part);
      }
      if (member->type==RawRelation::memberRelation &&
          (member->role=="inner" ||
           member->role=="outer" ||
           member->role.empty())) {
        if (boundaryId!=typeIgnore &&
            relation.GetType()==boundaryId) {
          RawRelationRef childRelation;

          if (resolvedRelations.find(member->id)!=resolvedRelations.end()) {
            progress.Error("Found self referencing relation "+
                           NumberToString(member->id)+
                           " during resolving of members of relation "+
                           NumberToString(relation.GetId())+" "+relation.GetName());
            return false;
          }


          if (!relDataFile.Get(member->id,childRelation)) {
            progress.Error("Cannot resolve relation member "+
                           NumberToString(member->id)+
                           " for relation "+
                           NumberToString(relation.GetId())+" "+relation.GetName());
            return false;
          }

          resolvedRelations.insert(member->id);

          for (size_t m=0; m<childRelation->members.size(); m++) {
            if (!ResolveMultipolygonMembers(progress,
                                            typeConfig,
                                            wayAreaIndexBlacklist,
                                            nodeDataFile,
                                            wayDataFile,
                                            relDataFile,
                                            resolvedRelations,
                                            relation,
                                            *childRelation,
                                            parts)) {
              break;
            }
          }
        }
        else {
          progress.Warning("Unsupported relation reference in relation "+
                           NumberToString(relation.GetId())+" "+
                           typeConfig.GetTypeInfo(relation.GetType()).GetName()+" "+
                           relation.GetName());
        }
        /*
        if (!ResolveMember(typeConfig,
                           rawRelation.GetId(),
                           relation.GetName(),
                           *member,
                           nodeDataFile,
                           wayDataFile,
                           relDataFile,
                           resolvedRelations,
                           relation.roles,
                           progress)) {
          return false;
        }*/
      }
    }

    return true;
  }

  static bool HandleMultipolygonRelation(Progress& progress,
                                         const TypeConfig& typeConfig,
                                         std::set<Id>& wayAreaIndexBlacklist,
                                         DataFile<RawNode>& nodeDataFile,
                                         DataFile<RawWay>& wayDataFile,
                                         DataFile<RawRelation>& relDataFile,
                                         RawRelation& rawRelation,
                                         Relation& relation)
  {
    std::set<Id> resolvedRelations;
    bool         reverseNodes;
    TagId        tagName=typeConfig.GetTagId("name");

    // manually scan the tags of the raRelation just to get a name for the relation
    // to improve the quality of further error output.
    if (tagName!=tagIgnore) {
      for (std::vector<Tag>::const_iterator tag=rawRelation.tags.begin();
          tag!=rawRelation.tags.end();
          tag++) {
        if (tag->key==tagName) {
          relation.attributes.name=tag->value;
          break;
        }
      }
    }

    relation.SetId(rawRelation.GetId());
    relation.SetType(rawRelation.GetType());

    std::list<MultipolygonPart> parts;

    if (!ResolveMultipolygonMembers(progress,
                                    typeConfig,
                                    wayAreaIndexBlacklist,
                                    nodeDataFile,
                                    wayDataFile,
                                    relDataFile,
                                    resolvedRelations,
                                    relation,
                                    rawRelation,
                                    parts)) {
      return false;
    }

    // Reconstruct multiploygon relation by applying the multipolygon resolving
    // algorithm as destribed at
    // http://wiki.openstreetmap.org/wiki/Relation:multipolygon/Algorithm
    if (!ResolveMultipolygon(progress,
                             relation,
                             parts)) {
      return false;
    }

    // Resolve SegmentAttributes for each ring

    for (std::list<MultipolygonPart>::iterator ring=parts.begin();
        ring!=parts.end();
        ring++) {
      if (ring->role.ring==0 &&
          ring->IsArea() &&
          ring->ways.front()->GetType()!=typeIgnore) {
        std::vector<Tag> tags(ring->ways.front()->GetTags());

        ring->role.attributes.type=ring->ways.front()->GetType();

        if (!ring->role.attributes.SetTags(progress,
                                           typeConfig,
                                           ring->ways.front()->GetId(),
                                           true,
                                           tags,
                                           reverseNodes)) {
                return false;
        }
      }
    }

    // If a ring and the direct child ring have the same time, this is old school style for
    // the child ring being a clip region. We set the type of the child to typeIgnore then...

    for (std::list<MultipolygonPart>::iterator ring=parts.begin();
        ring!=parts.end();
        ring++) {
      if (ring->IsArea()) {
        std::list<MultipolygonPart>::iterator childRing=ring;

        childRing++;
        while (childRing!=parts.end() &&
               childRing->role.ring==ring->role.ring+1) {

          if (childRing->IsArea() &&
              ring->role.GetType()==childRing->role.GetType()) {
            childRing->role.attributes.type=typeIgnore;
            wayAreaIndexBlacklist.insert(childRing->ways.front()->GetId());
          }
          childRing++;
        }
      }
    }

    // If the relation itself does not have a type, try to autodetect the type from the outer rings

    if (relation.GetType()==typeIgnore) {
      for (std::list<MultipolygonPart>::iterator ring=parts.begin();
          ring!=parts.end();
          ring++) {
        if (ring->role.ring==0 &&
            ring->IsArea() &&
            ring->ways.front()->GetType()!=typeIgnore) {
          if (relation.GetType()==typeIgnore ||
              relation.GetType()==ring->ways.front()->GetType()) {
            if (progress.OutputDebug()) {
              progress.Debug("Autodetecting type of multipolygon relation "+NumberToString(relation.GetId())+" as "+NumberToString(relation.GetType()));
            }

            relation.SetType(ring->ways.front()->GetType());
            ring->role.attributes.type=typeIgnore;
            wayAreaIndexBlacklist.insert(ring->ways.front()->GetId());
          }
          else if (ring->ways.front()->GetType()!=typeIgnore) {
            progress.Warning("Multipolygon relation "+NumberToString(relation.GetId())+" has conflicting types for outer boundary ("+
                             typeConfig.GetTypeInfo(relation.GetType()).GetName()+ " vs. "+typeConfig.GetTypeInfo(ring->ways.front()->GetType()).GetName()+")");
          }
        }
      }
    }

    if (relation.GetType()==typeIgnore) {
      progress.Warning("Multipolygon relation "+NumberToString(relation.GetId())+" does not have a type, skipping");
      return false;
    }

    if (typeConfig.GetTypeInfo(relation.GetType()).GetIgnore()) {
      return false;
    }

    if (!relation.attributes.SetTags(progress,
                                     typeConfig,
                                     rawRelation.GetId(),
                                     true,
                                     rawRelation.tags,
                                     reverseNodes)) {
      return false;
    }

    // Blacklisting areas

    for (size_t m=0; m<relation.roles.size(); m++) {
      for (std::list<MultipolygonPart>::const_iterator ring=parts.begin();
          ring!=parts.end();
          ring++) {
        if (ring->ways.front()->GetType()!=typeIgnore &&
            ring->IsArea()) {
          wayAreaIndexBlacklist.insert(ring->ways.front()->GetId());
        }
      }
    }

    // (Re)create roles for relation

    relation.roles.reserve(parts.size());
    for (std::list<MultipolygonPart>::iterator ring=parts.begin();
        ring!=parts.end();
        ring++) {
      for (std::vector<RawNode>::const_iterator node=ring->nodes.begin();
          node!=ring->nodes.end();
          node++) {
        Point point;

        point.Set(node->GetId(),
                  node->GetLat(),
                  node->GetLon());

        ring->role.nodes.push_back(point);
      }

      assert(!ring->role.nodes.empty());

      relation.roles.push_back(ring->role);
    }

    assert(!relation.roles.empty());

    return true;
  }

  std::string RelationDataGenerator::GetDescription() const
  {
    return "Generate 'relations.dat'";
  }

  bool RelationDataGenerator::Import(const ImportParameter& parameter,
                                     Progress& progress,
                                     const TypeConfig& typeConfig)
  {
    std::set<Id>          wayAreaIndexBlacklist;

    DataFile<RawNode>     nodeDataFile("rawnodes.dat",
                                       "rawnode.idx",
                                       parameter.GetRawNodeDataCacheSize(),
                                       parameter.GetRawNodeIndexCacheSize());
    DataFile<RawWay>      wayDataFile("rawways.dat",
                                      "rawway.idx",
                                      parameter.GetRawWayDataCacheSize(),
                                      parameter.GetRawWayIndexCacheSize());

    DataFile<RawRelation> relDataFile("rawrels.dat",
                                      "rawrel.idx",
                                      parameter.GetRawWayDataCacheSize(),
                                      parameter.GetRawWayIndexCacheSize());

    if (!nodeDataFile.Open(parameter.GetDestinationDirectory(),
                           parameter.GetRawNodeIndexMemoryMaped(),
                           parameter.GetRawNodeDataMemoryMaped())) {
      std::cerr << "Cannot open raw nodes data files!" << std::endl;
      return false;
    }

    if (!wayDataFile.Open(parameter.GetDestinationDirectory(),
                          parameter.GetRawWayIndexMemoryMaped(),
                          parameter.GetRawWayDataMemoryMaped())) {
      std::cerr << "Cannot open raw way data files!" << std::endl;
      return false;
    }

    if (!relDataFile.Open(parameter.GetDestinationDirectory(),true,true)) {
      std::cerr << "Cannot open raw relation data files!" << std::endl;
      return false;
    }

    //
    // Analysing distribution of nodes in the given interval size
    //

    progress.SetAction("Generate relations.dat");

    FileScanner         scanner;
    FileWriter          writer;
    uint32_t            rawRelationCount=0;
    size_t              selectedRelationCount=0;
    uint32_t            writtenRelationCount=0;
    std::vector<size_t> wayTypeCount;
    std::vector<size_t> wayNodeTypeCount;
    std::vector<size_t> areaTypeCount;
    std::vector<size_t> areaNodeTypeCount;

    wayTypeCount.resize(typeConfig.GetMaxTypeId()+1,0);
    wayNodeTypeCount.resize(typeConfig.GetMaxTypeId()+1,0);
    areaTypeCount.resize(typeConfig.GetMaxTypeId()+1,0);
    areaNodeTypeCount.resize(typeConfig.GetMaxTypeId()+1,0);

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "rawrels.dat"))) {
      progress.Error("Cannot open 'rawrels.dat'");
      return false;
    }

    if (!scanner.Read(rawRelationCount)) {
      progress.Error("Cannot read nunber of raw relations from data file");
      return false;
    }

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "relations.dat"))) {
      progress.Error("Cannot create 'relations.dat'");
      return false;
    }

    writer.Write(writtenRelationCount);

    for (uint32_t r=1; r<=rawRelationCount; r++) {
      progress.SetProgress(r,rawRelationCount);

      RawRelation rawRel;

      if (!rawRel.Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(r)+" of "+
                       NumberToString(rawRelationCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");
        return false;
      }

      if (rawRel.members.empty()) {
        progress.Warning("Relation "+
                         NumberToString(rawRel.GetId())+
                         " does not have any members!");
        continue;
      }

      if (rawRel.GetType()!=typeIgnore &&
          typeConfig.GetTypeInfo(rawRel.GetType()).GetIgnore()) {
        continue;
      }

      bool isArea=false;

      selectedRelationCount++;

      // Check, if the type should be handled as multipolygon
      isArea=typeConfig.GetTypeInfo(rawRel.GetType()).GetMultipolygon();

      // Is it possibly a explicitly marked multipolygon?
      if (!isArea) {
        std::vector<Tag>::iterator tag=rawRel.tags.begin();
        while (tag!=rawRel.tags.end()) {
          if (tag->key==typeConfig.tagType) {
            if (tag->value=="multipolygon") {
              isArea=true;
            }

            tag=rawRel.tags.erase(tag);
          }
          else {
            tag++;
          }
        }
      }

      Relation rel;

      if (isArea) {
        if (!HandleMultipolygonRelation(progress,
                                        typeConfig,
                                        wayAreaIndexBlacklist,
                                        nodeDataFile,
                                        wayDataFile,
                                        relDataFile,
                                        rawRel,
                                        rel)) {
          continue;
        }
      }
      else {
        continue;
      }

      if (progress.OutputDebug()) {
        progress.Debug("Storing relation "+NumberToString(rel.GetId())+" "+NumberToString(rel.GetType())+" "+rel.GetName());
      }

      if (rel.IsArea()) {
        areaTypeCount[rel.GetType()]++;
        for (size_t i=0; i<rel.roles.size(); i++) {
          if (rel.roles[i].ring==0) {
            areaNodeTypeCount[rel.GetType()]+=rel.roles[i].nodes.size();
          }
        }
      }
      else {
        wayTypeCount[rel.GetType()]++;

        for (size_t i=0; i<rel.roles.size(); i++) {
          wayNodeTypeCount[rel.GetType()]+=rel.roles[i].nodes.size();
        }
      }

      rel.Write(writer);
      writtenRelationCount++;
    }

    progress.Info(NumberToString(rawRelationCount)+" relations read"+
                  ", "+NumberToString(selectedRelationCount)+" relation selected"+
                  ", "+NumberToString(writtenRelationCount)+" relations written");

    writer.SetPos(0);
    writer.Write(writtenRelationCount);

    if (!(scanner.Close() &&
          writer.Close() &&
          wayDataFile.Close() &&
          nodeDataFile.Close())) {
      return false;
    }

    progress.SetAction("Generate wayblack.dat");

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "wayblack.dat"))) {
      progress.Error("Cannot create 'wayblack.dat'");
      return false;
    }

    for (std::set<Id>::const_iterator id=wayAreaIndexBlacklist.begin();
         id!=wayAreaIndexBlacklist.end();
         ++id) {
      writer.WriteNumber(*id);
    }

    progress.Info(NumberToString(wayAreaIndexBlacklist.size())+" ways written to blacklist");

    progress.Info("Dump statistics");

    for (size_t i=0; i<typeConfig.GetMaxTypeId(); i++) {
      std::string buffer=typeConfig.GetTypeInfo(i).GetName()+": "+
                         NumberToString(wayTypeCount[i])+" "+NumberToString(wayNodeTypeCount[i])+" "+
                         NumberToString(areaTypeCount[i])+" "+NumberToString(areaNodeTypeCount[i]);

      progress.Debug(buffer);
    }

    return writer.Close();
  }
}
