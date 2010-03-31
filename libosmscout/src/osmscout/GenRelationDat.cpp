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

#include <osmscout/GenRelationDat.h>

#include <algorithm>
#include <cassert>

#include <osmscout/RawNode.h>
#include <osmscout/RawRelation.h>
#include <osmscout/RawWay.h>

#include <osmscout/Util.h>

#include <osmscout/DataFile.h>

#include <osmscout/Relation.h>

namespace osmscout {

  bool ResolveMember(const RawRelation::Member& member,
                     DataFile<RawNode>& nodeDataFile,
                     DataFile<RawWay>& wayDataFile,
                     std::vector<Point>& nodes,
                     TypeId& type)
  {
    if (member.type==RawRelation::memberNode) {
      RawNode node;

      if (!nodeDataFile.Get(member.id,node)) {
        return false;
      }

      Point point;

      point.id=node.id;
      point.lat=node.lat;
      point.lon=node.lon;

      nodes.push_back(point);

      type=node.type;

      return true;
    }
    else if (member.type==RawRelation::memberWay) {
      RawWay way;

      if (!wayDataFile.Get(member.id,way)) {
        return false;
      }

      type=way.type;

      std::vector<RawNode> ns;

      if (!nodeDataFile.Get(way.nodes,ns)) {
        return false;
      }

      for (size_t i=0; i<ns.size(); i++) {
        Point point;

        point.id=ns[i].id;
        point.lat=ns[i].lat;
        point.lon=ns[i].lon;

        nodes.push_back(point);
      }

      return true;
    }
    else {
      return false;
    }
  }

  /**
    Returns true, if area a is in area b
    */
  inline bool IsAreaInArea(const std::vector<bool>& includes, size_t count, size_t a, size_t b)
  {
    return includes[a*count+b];
  }

  /**
    Find a top level role.

    A top level role is a role that is not included by any other unused role ("top level tree
    element").
    */
  std::list<Relation::Role>::const_iterator FindTopLevel(const std::list<Relation::Role>& rings,
                                                         const std::vector<bool>& includes,
                                                         const std::vector<bool>& used,
                                                         size_t& topIndex)
  {
    size_t i=0;
    std::list<Relation::Role>::const_iterator r=rings.begin();
    while (r!=rings.end()) {
      bool found=false;

      if (!used[i]) {
        bool included=false;

        for (size_t x=0; x<rings.size();x++) {
          if (x!=i && !used[x]) {
            included=IsAreaInArea(includes,rings.size(),i,x);

            if (included) {
              break;
            }
          }
        }

        found=!included;

        if (found) {
          topIndex=i;
          return r;
        }
      }

      ++i;
      ++r;
    }

    return rings.end();
  }

  /**
    Find a sub role.

    A sub role is a role that is included by the given top role but is not included
    by any other role ("direct child tree element").
    */
  std::list<Relation::Role>::const_iterator FindSub(const std::list<Relation::Role>& rings,
                                                    size_t topIndex,
                                                    const std::vector<bool>& includes,
                                                    const std::vector<bool>& used,
                                                    size_t& subIndex)
  {
    size_t i=0;
    std::list<Relation::Role>::const_iterator r=rings.begin();
    while (r!=rings.end()) {
      if (!used[i] && IsAreaInArea(includes,rings.size(),i,topIndex)) {
        bool included=false;

        for (size_t x=0; x<rings.size();x++) {
          if (x!=i && !used[x]) {
            included=IsAreaInArea(includes,rings.size(),i,x);

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
      ++r;
    }

    return rings.end();
  }

  /**
    Recursivly consume all direct children and all direct children of that children)
    of the given role.
    */
  void ConsumeSubs(const std::list<Relation::Role>& rings,
                   std::list<Relation::Role>& groups,
                   const std::vector<bool> includes,
                   std::vector<bool>& used,
                   size_t topIndex, size_t id)
  {
    std::list<Relation::Role>::const_iterator sub;
    size_t                                    subIndex;

    sub=FindSub(rings,topIndex,includes,used,subIndex);
    while (sub!=rings.end()) {
      used[subIndex]=true;
      groups.push_back(*sub);
      groups.back().role=NumberToString(id);

      ConsumeSubs(rings,groups,includes,used,subIndex,id+1);

      sub=FindSub(rings,topIndex,includes,used,subIndex);
    }
  }

  /**
    Try to resolve a multipolygon relation.

    See http://wiki.openstreetmap.org/wiki/Relation:multipolygon/Algorithm
    */
  bool ResolveMultipolygon(Relation& relation,
                           Progress& progress)
  {
    size_t                    counter=0;
    std::list<Relation::Role> roles;
    std::list<Relation::Role> rings;
    std::list<Relation::Role> groups;
    std::list<Point>          points;

    // Make a local copy of the relation roles
    for (size_t i=0; i<relation.roles.size(); i++) {
      roles.push_back(relation.roles[i]);
    }

    //
    // Ring assignment
    //

    // Try to consume all roles
    while (roles.size()>0) {
      // Take the first unused roles and copy all its points
      for (size_t i=0; i<roles.front().nodes.size(); i++) {
        points.push_back(roles.front().nodes[i]);
      }
      roles.erase(roles.begin());

      // Now consume more roles that have the same start or end
      // until all joined points build a closed shape ("current way")
      while (roles.size()>0 &&
             points.front().id!=points.back().id) {
        bool found=false;

        // Find a role that continues the current way
        for (std::list<Relation::Role>::iterator role=roles.begin();
             role!=roles.end();
             ++role) {
          if (points.back().id==role->nodes.front().id) {
            for (size_t i=1; i<role->nodes.size(); i++) {
              points.push_back(role->nodes[i]);
            }
            roles.erase(role);

            found=true;
            break;
          }
          else if (points.back().id==role->nodes.back().id) {
            for (size_t i=1; i<role->nodes.size(); i++) {
              points.push_back(role->nodes[role->nodes.size()-i-1]);
            }
            roles.erase(role);
            found=true;
            break;
          }
        }

        // if we havn't found another way and we have not closed
        // the current way we have to give up
        if (!found) {
          progress.Warning("Multipolygon relation "+NumberToString(relation.id)+
                           ": Cannot find matching node for node id "+
                           NumberToString(points.back().id));
          return false;
        }
      }

      // All roles have been consumed and we still have not closed the current way
      if (points.front().id!=points.back().id) {
        progress.Warning("Multipolygon relation "+NumberToString(relation.id)+
                         ": No ways left to close current ring");
        return false;
      }

      // Add the found points the our new (internal) list of merges roles
      Relation::Role role;

      role.nodes.reserve(points.size());

      for (std::list<Point>::const_iterator point=points.begin();
           point!=points.end();
           ++point) {
        role.nodes.push_back(*point);
      }

      rings.push_back(role);
      points.clear();
      counter++;
    }

    //
    // Ring grouping
    //

    std::vector<bool> includes;
    std::vector<bool> used;

    includes.resize(rings.size()*rings.size(),false);
    used.resize(rings.size(),false);

    size_t i=0;
    for (std::list<Relation::Role>::const_iterator r1=rings.begin();
         r1!=rings.end();
         ++r1) {
      size_t j=0;
      for (std::list<Relation::Role>::const_iterator r2=rings.begin();
           r2!=rings.end();
           ++r2) {
        if (i==j) {
          includes[j*rings.size()+i]=false;
        }
        else {
          includes[j*rings.size()+i]=IsAreaInArea(r2->nodes,r1->nodes);
        }

        j++;
      }

      i++;
    }

    size_t id=0;
    while (groups.size()<rings.size()) {
      std::list<Relation::Role>::const_iterator top;
      size_t                                    topIndex;

      // Find a ring that is not yet used and that is not contained by another unused ring
      top=FindTopLevel(rings,includes,used,topIndex);

      if (top==rings.end()) {
        progress.Warning("Multipolygon relation "+NumberToString(relation.id)+
                         ": Error during ring grouping");
        return false;
      }

      used[topIndex]=true;
      groups.push_back(*top);
      groups.back().role=NumberToString(id);

      ConsumeSubs(rings,groups,includes,used,topIndex,id+1);
    }

    //
    // Multipolygon creation
    //

    //
    // Copy back data
    //

    relation.roles.clear();
    relation.roles.reserve(groups.size());

    for (std::list<Relation::Role>::const_iterator group=groups.begin();
         group!=groups.end();
         ++group) {
      relation.roles.push_back(*group);
    }

    return true;
  }

  bool GenerateRelationDat(const TypeConfig& typeConfig,
                           const ImportParameter& parameter,
                           Progress& progress)
  {
    DataFile<RawNode> nodeDataFile("rawnodes.dat","rawnode.idx",10);
    DataFile<RawWay>  wayDataFile("rawways.dat","rawway.idx",10);

    if (!nodeDataFile.Open(".")) {
      std::cerr << "Cannot open raw nodes data files!" << std::endl;
      return false;
    }

    if (!wayDataFile.Open(".")) {
      std::cerr << "Cannot open raw way data files!" << std::endl;
      return false;
    }

    //
    // Analysing distribution of nodes in the given interval size
    //

    progress.SetAction("Generate relations.dat");

    FileScanner scanner;
    FileWriter  writer;
    size_t      allRelationCount=0;
    size_t      selectedRelationCount=0;
    size_t      writtenRelationCount=0;

    if (!scanner.Open("rawrels.dat")) {
      progress.Error("Canot open 'rawrels.dat'");
      return false;
    }

    if (!writer.Open("relations.dat")) {
      progress.Error("Canot create 'relations.dat'");
      return false;
    }

    while (!scanner.HasError()) {
      RawRelation rawRel;

      rawRel.Read(scanner);

      if (!scanner.HasError()) {
        allRelationCount++;

        if (rawRel.type==typeIgnore) {
          continue;
        }

        if (rawRel.members.size()==0) {
          progress.Warning("Relation "+
                           NumberToString(rawRel.id)+
                           " does not have any members!");
        }

        Relation              rel;
        std::string           name;
        std::set<std::string> roles;
        bool                  error=false;

        selectedRelationCount++;

        rel.id=rawRel.id;
        rel.type=rawRel.type;

        for (size_t i=0; i<rawRel.tags.size(); i++) {
          if (rawRel.tags[i].key==tagType) {
            rel.relType=rawRel.tags[i].value;
          }
          else if (rawRel.tags[i].key==tagName) {
            name=rawRel.tags[i].value;
          }
        }

        for (size_t i=0; i<rawRel.members.size(); i++) {
          roles.insert(rawRel.members[i].role);
        }

        rel.roles.resize(rawRel.members.size());

        for (size_t m=0; m<rawRel.members.size(); m++) {
          TypeId type;

          rel.roles[m].role=rawRel.members[m].role;

          if (!ResolveMember(rawRel.members[m],nodeDataFile,
                             wayDataFile,
                             rel.roles[m].nodes,type)) {
            progress.Error("Cannot resolve relation member with id "+
                           NumberToString(rawRel.members[m].id)+
                           " for relation "+
                           NumberToString(rawRel.id)+" "+name);
            error=true;
            break;
          }

          rel.roles[m].type=type;
        }

        if (error) {
          continue;
        }

        if (rel.relType=="multipolygon" ||
            rel.relType=="boundary") {
          if (!ResolveMultipolygon(rel,progress)) {
            progress.Error("Cannot resolve multipolygon relation "+
                           NumberToString(rawRel.id)+" "+name);
            continue;
          }
        }

        //progress.Debug("Storing relation "+rel.relType+" "+NumberToString(rel.type)+" "+name);

        rel.id=rawRel.id;
        rel.tags=rawRel.tags;

        rel.Write(writer);

        writtenRelationCount++;
      }
    }

    progress.Info(NumberToString(allRelationCount)+" relations read"+
                  ", "+NumberToString(selectedRelationCount)+" relation selected"+
                  ", "+NumberToString(writtenRelationCount)+" relations written");

    return scanner.Close() && writer.Close() && wayDataFile.Close() && nodeDataFile.Close();
  }
}

