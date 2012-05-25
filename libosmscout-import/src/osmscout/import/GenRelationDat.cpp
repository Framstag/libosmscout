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


  static bool ResolveMember(const TypeConfig& typeConfig,
                            Id id,
                            const std::string& name,
                            const RawRelation::Member& member,
                            DataFile<RawNode>& nodeDataFile,
                            DataFile<RawWay>& wayDataFile,
                            DataFile<RawRelation>& relationDataFile,
                            std::set<Id>& resolvedRelations,
                            const std::string& roleName,
                            std::vector<Relation::Role>& roles,
                            Progress& progress)
  {
    if (member.type==RawRelation::memberNode) {
      RawNodeRef node;

      if (!nodeDataFile.Get(member.id,node)) {
        progress.Error("Cannot resolve relation member of type node with id "+
                       NumberToString(member.id)+
                       " for relation "+
                       NumberToString(id)+" "+name);
        return false;
      }

      Relation::Role role;

      role.role=roleName;
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
        progress.Error("Cannot resolve relation member of type way with id "+
                       NumberToString(member.id)+
                       " for relation "+
                       NumberToString(id)+" "+name);
        return false;
      }

      std::vector<RawNodeRef> nodes;

      if (!nodeDataFile.Get(way->GetNodes(),nodes)) {
        progress.Error("Cannot resolve nodes of relation member of type way with id "+
                       NumberToString(member.id)+
                       " for relation "+
                       NumberToString(id)+" "+name);
        return false;
      }

      bool           reverseNodes=false;
      Relation::Role role;

      role.role=roleName;
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
        progress.Error("Cannot resolve relation member of type relation with id "+
                       NumberToString(member.id)+
                       " for relation "+
                       NumberToString(id)+" "+name);
        return false;
      }

      resolvedRelations.insert(member.id);

      for (size_t m=0; m<relation->members.size(); m++) {
        if (!ResolveMember(typeConfig,
                           id,
                           name,
                           relation->members[m],
                           nodeDataFile,
                           wayDataFile,
                           relationDataFile,
                           resolvedRelations,
                           roleName,
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
  static std::list<Relation::Role>::const_iterator FindTopLevel(const std::list<Relation::Role>& rings,
                                                                const GroupingState& state,
                                                                size_t& topIndex)
  {
    size_t i=0;
    for (std::list<Relation::Role>::const_iterator r=rings.begin();
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
  static std::list<Relation::Role>::const_iterator FindSub(const std::list<Relation::Role>& rings,
                                                           size_t topIndex,
                                                           const GroupingState& state,
                                                           size_t& subIndex)
  {
    size_t i=0;
    for (std::list<Relation::Role>::const_iterator r=rings.begin();
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
  static void ConsumeSubs(const std::list<Relation::Role>& rings,
                          std::list<Relation::Role>& groups,
                          GroupingState& state,
                          size_t topIndex,
                          size_t id)
  {
    std::list<Relation::Role>::const_iterator sub;
    size_t                                    subIndex;

    sub=FindSub(rings,topIndex,state,subIndex);
    while (sub!=rings.end()) {
      state.SetUsed(subIndex);
      groups.push_back(*sub);
      groups.back().ring=id;

      ConsumeSubs(rings,groups,state,subIndex,id+1);

      sub=FindSub(rings,topIndex,state,subIndex);
    }
  }

  static bool AssignRings(Relation& relation,
                          std::list<Relation::Role>& rings,
                          Progress& progress)
  {
    std::list<Relation::Role> roles;

    // Make a local copy of the relation roles
    for (size_t i=0; i<relation.roles.size(); i++) {
      // We have some marker nodes like "admin_centre", which we want to skip
      if (relation.roles[i].nodes.size()==1) {
        continue;
      }

      // We can have ways that are of type way and still build a closed shape.
      // We are sure that they build an area and fix the data here
      if (relation.roles[i].nodes.size()>2 &&
          relation.roles[i].nodes.front().GetId()==relation.roles[i].nodes.back().GetId()) {
        relation.roles[i].attributes.flags|=SegmentAttributes::isArea;
        relation.roles[i].nodes.pop_back();
      }

      roles.push_back(relation.roles[i]);
    }

    // Try to consume all roles
    while (!roles.empty()) {
      size_t           rolesSelected=1;
      bool             finished=false;
      Relation::Role   leadingRole=roles.front();
      Relation::Role   role;
      std::list<Point> points;

      roles.pop_front();

      role.attributes=leadingRole.attributes;
      role.attributes.flags|=SegmentAttributes::isArea;
      role.ring=0;

      // Take the first unused roles and copy all its points
      for (size_t i=0; i<leadingRole.nodes.size(); i++) {
        points.push_back(leadingRole.nodes[i]);
      }

      // TODO: All roles that make up a ring should have the same type

      if (leadingRole.IsArea()) {
        finished=true;
      }

      // Now consume more roles that have the same start or end
      // until all joined points build a closed shape ("current way")
      while (!roles.empty() &&
             !finished) {
        bool found=false;

        // Find a role that continues the current way
        for (std::list<Relation::Role>::iterator r=roles.begin();
             r!=roles.end();
             ++r) {
          if (points.back().GetId()==r->nodes.front().GetId()) {
            for (size_t i=1; i<r->nodes.size(); i++) {
              points.push_back(r->nodes[i]);
            }

            if (role.attributes.type==typeIgnore &&
                r->GetType()!=typeIgnore) {
              role.attributes.type=r->GetType();
            }

            if (role.GetName().empty() && !r->GetName().empty()) {
              role.attributes.name=r->GetName();
            }

            if (role.GetRefName().empty() && !r->GetRefName().empty()) {
              role.attributes.ref=r->GetRefName();
            }

            roles.erase(r);

            rolesSelected++;
            found=true;

            break;
          }
          else if (points.back().GetId()==r->nodes.back().GetId()) {
            for (size_t i=1; i<r->nodes.size(); i++) {
              points.push_back(r->nodes[r->nodes.size()-i-1]);
            }

            if (role.attributes.type==typeIgnore &&
                r->GetType()!=typeIgnore) {
              role.attributes.type=r->GetType();
            }

            if (role.GetName().empty() && !r->GetName().empty()) {
              role.attributes.name=r->GetName();
            }

            if (role.GetRefName().empty() && !r->GetRefName().empty()) {
              role.attributes.ref=r->GetRefName();
            }

            roles.erase(r);

            rolesSelected++;
            found=true;
            break;
          }
        }

        if (found) {
          if (points.front().GetId()==points.back().GetId()) {
            finished=true;
          }
        }
        else {
          // if we havn't found another way and we have not closed
          // the current way we have to give up
          progress.Error("Multipolygon relation "+NumberToString(relation.GetId())+
                         ": Cannot find matching node for node id "+
                         NumberToString(points.back().GetId()));
          return false;
        }
      }

      // All roles have been consumed and we still have not closed the current way
      if (!finished) {
        progress.Error("Multipolygon relation "+NumberToString(relation.GetId())+
                       ": No ways left to close current ring");
        return false;
      }

      role.nodes.reserve(points.size());

      for (std::list<Point>::const_iterator point=points.begin();
           point!=points.end();
           ++point) {
        role.nodes.push_back(*point);
      }

      // During concatination we might define a closed ring with start==end, but everywhere else
      // in the code we store areas without repeating the start, so we remove the final node again
      if (role.nodes.back().GetId()==role.nodes.front().GetId()) {
        role.nodes.pop_back();
      }

      rings.push_back(role);
    }

    return true;
  }

  /**
    Try to resolve a multipolygon relation.

    See http://wiki.openstreetmap.org/wiki/Relation:multipolygon/Algorithm
    */
  static bool ResolveMultipolygon(Progress& progress,
                                  Relation& relation)
  {
    std::list<Relation::Role> rings;
    std::list<Relation::Role> groups;

    //
    // Ring assignment
    //

    if (!AssignRings(relation,
                     rings,
                     progress)) {
      return false;
    }

    //
    // Ring grouping
    //

    GroupingState state(rings.size());

    size_t i=0;
    for (std::list<Relation::Role>::const_iterator r1=rings.begin();
         r1!=rings.end();
         ++r1) {
      size_t j=0;

      for (std::list<Relation::Role>::const_iterator r2=rings.begin();
           r2!=rings.end();
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
      std::list<Relation::Role>::const_iterator top;
      size_t                                    topIndex;

      // Find a ring that is not yet used and that is not contained by another unused ring
      top=FindTopLevel(rings,state,topIndex);

      if (top==rings.end()) {
        progress.Warning("Multipolygon relation "+NumberToString(relation.GetId())+
                         ": Error during ring grouping");
        return false;
      }

      state.SetUsed(topIndex);
      groups.push_back(*top);

      //
      // The outer ring(s) inherit attribute values from the relation
      //

      // Outer ring always has the type of the relation
      groups.back().attributes.type=relation.GetType();
      groups.back().ring=0;

      if (state.HasIncludes(topIndex)) {
        ConsumeSubs(rings,groups,state,topIndex,1);
      }
    }

    if (groups.empty()) {
      progress.Warning("Multipolygon relation "+NumberToString(relation.GetId())+
                       ": No groups");
      return false;
    }

    //
    // Copy back data
    //

    relation.roles.assign(groups.begin(),groups.end());

    return true;
  }

  /**
    Merge relations roles where nodes build a connected way (start/end nodes match)
    and where all relevant relations values are the same (no visible difference in drawing)
    thus reducing the number of roles and increasing the number of points per role
    (which gives us the change to optimize better for low magnification).
    */
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

      Relation              rel;
      bool                  error=false;
      bool                  isArea=false;
      bool                  reverseNodes;

      selectedRelationCount++;

      rel.SetId(rawRel.GetId());
      rel.SetType(rawRel.GetType());

      // Check, if the type should be handled as multipolygon
      isArea=typeConfig.GetTypeInfo(rel.GetType()).GetMultipolygon();

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

      if (!rel.attributes.SetTags(progress,
                                  typeConfig,
                                  rel.GetId(),
                                  isArea,
                                  rawRel.tags,
                                  reverseNodes)) {
        continue;
      }

      rel.roles.reserve(rawRel.members.size());

      std::set<Id> resolvedRelations;

      for (size_t m=0; m<rawRel.members.size(); m++) {
        if (!ResolveMember(typeConfig,
                           rawRel.GetId(),
                           rel.GetName(),
                           rawRel.members[m],
                           nodeDataFile,
                           wayDataFile,
                           relDataFile,
                           resolvedRelations,
                           rawRel.members[m].role,
                           rel.roles,
                           progress)) {
          error=true;
          break;
        }
      }

      resolvedRelations.clear();

      if (error) {
        continue;
      }

      // Resolve type of multipolygon relations if the relation does
      // not have a type
      if (rel.GetType()==typeIgnore) {
        if (rel.IsArea()) {
          bool   correct=true;
          TypeId typeId=typeIgnore;

          for (size_t m=0; m<rel.roles.size(); m++) {
            if (rel.roles[m].role=="outer" &&
                rel.roles[m].GetType()!=typeIgnore &&
                typeConfig.GetTypeInfo(rel.roles[m].GetType()).CanBeArea()) {
              if (typeId==typeIgnore) {
                typeId=rel.roles[m].GetType();
                if (progress.OutputDebug()) {
                  progress.Debug("Autodetecting type of multipolygon relation "+NumberToString(rel.GetId())+" as "+NumberToString(typeId));
                }
              }
              else if (typeId!=typeIgnore &&
                       typeId!=rel.roles[m].GetType()) {
                if (progress.OutputDebug()) {
                  progress.Warning("Multipolygon relation "+NumberToString(rel.GetId())+" has conflicting types for outer boundary ("+
                                   typeConfig.GetTypeInfo(typeId).GetName()+ " vs. "+typeConfig.GetTypeInfo(rel.roles[m].GetType()).GetName()+")");
                }
                correct=false;
              }
            }
          }

          if (correct) {
            rel.SetType(typeId);
          }
        }
      }

      if (rel.GetType()==typeIgnore) {
        //std::cout << "Cannot identify type of relation " << rel.GetId() << std::endl;
        continue;
      }

      if (typeConfig.GetTypeInfo(rel.GetType()).GetIgnore()) {
        continue;
      }

      if (!rel.IsArea()) {
        continue;
      }

      // Blacklist all ways that build the multipolygon relation
      if (rel.IsArea()) {
        for (size_t m=0; m<rawRel.members.size(); m++) {
          if (rawRel.members[m].type==RawRelation::memberWay) {
            wayAreaIndexBlacklist.insert(rawRel.members[m].id);
          }
        }
      }
      // Blacklist all relation members that have the same type as relation itself
      // from the areaWayIndex to assure that a way will not be returned twice,
      // once as part of the relation and once as way itself
      else if (typeConfig.GetTypeInfo(rel.GetType()).GetConsumeChildren()) {
        for (size_t m=0; m<rel.roles.size(); m++) {
          if (rel.GetType()==rel.roles[m].GetType()) {
            wayAreaIndexBlacklist.insert(rawRel.members[m].id);
          }
        }
      }

      // Reconstruct multiploygon relation by applying the multipolygon resolving
      // algorithm as destribed at
      // http://wiki.openstreetmap.org/wiki/Relation:multipolygon/Algorithm
      if (rel.IsArea()) {
        if (!ResolveMultipolygon(progress,rel)) {
          progress.Error("Cannot resolve multipolygon relation "+
                         NumberToString(rawRel.GetId())+" "+rel.GetName());
          continue;
        }
      }
      else {
        if (!CompactRelation(rel,rel.GetName(),progress)) {
          progress.Error("Relation "+NumberToString(rel.GetId())+
                         " cannot be compacted");
          continue;
        }
      }

      // Postprocessing of relation

      if (rel.IsArea()) {
        for (size_t m=0; m<rel.roles.size(); m++) {

          // Outer boundary inherits attributes from relation
          if (rel.roles[m].ring==0) {
            if (rel.roles[m].GetType()==typeIgnore) {
              rel.roles[m].attributes.type=rel.GetType();
            }

            if (!rel.GetName().empty() && rel.roles[m].attributes.GetName().empty()) {
              rel.roles[m].attributes.name=rel.GetName();
            }
            else if (rel.GetName().empty() && !rel.roles[m].attributes.GetName().empty()) {
              rel.GetName()=rel.roles[m].attributes.name;
            }

            if (!rel.GetRefName().empty() && rel.roles[m].GetRefName().empty()) {
              rel.roles[m].attributes.ref=rel.GetRefName();
            }
            else if (rel.GetRefName().empty() && !rel.roles[m].GetRefName().empty()) {
              rel.GetRefName()=rel.roles[m].attributes.ref;
            }
          }
        }

      }
      else {
        for (size_t m=0; m<rel.roles.size(); m++) {
          if (rel.roles[m].GetType()==typeIgnore) {
            // If a relation member does not have a type, it has the type of the relation
            rel.roles[m].attributes.type=rel.GetType();
          }

          if (!rel.GetName().empty() && rel.roles[m].attributes.GetName().empty()) {
            rel.roles[m].attributes.name=rel.GetName();
          }
          else if (rel.GetName().empty() && !rel.roles[m].attributes.GetName().empty()) {
            rel.GetName()=rel.roles[m].attributes.name;
          }

          if (!rel.GetRefName().empty() && rel.roles[m].GetRefName().empty()) {
            rel.roles[m].attributes.ref=rel.GetRefName();
          }
          else if (rel.GetRefName().empty() && !rel.roles[m].GetRefName().empty()) {
            rel.GetRefName()=rel.roles[m].attributes.ref;
          }
        }
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
