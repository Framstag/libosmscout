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
                     std::vector<Point> nodes)
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

      return true;
    }
    else if (member.type==RawRelation::memberWay) {
      RawWay way;

      if (!wayDataFile.Get(member.id,way)) {
        return false;
      }

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

        if (rawRel.type!=typeIgnore) {
          Relation              rel;
          std::set<std::string> roles;
          size_t                rp;
          bool                  error=false;

          selectedRelationCount++;

          for (size_t i=0; i<rawRel.members.size(); i++) {
            roles.insert(rawRel.members[i].role);
          }

          rel.roles.resize(roles.size());

          rp=0;
          for (std::set<std::string>::const_iterator r=roles.begin();
               r!=roles.end();
               ++r) {
            rel.roles[rp].role=*r;

            for (size_t m=0; m<rawRel.members.size(); m++) {
              if (rawRel.members[m].role==*r) {
                if (!ResolveMember(rawRel.members[m],nodeDataFile,
                                   wayDataFile,
                                   rel.roles[rp].nodes)) {
                  progress.Error("Cannot resolve relation member with id "+
                                 NumberToString(rawRel.members[m].id)+
                                 " for relation "+
                                 NumberToString(rawRel.id));
                  error=true;
                  break;
                }
              }
            }

            if (error) {
              break;
            }

            rp++;
          }

          if (!error) {
            rel.id=rawRel.id;
            rel.tags=rawRel.tags;

            rel.Write(writer);

            writtenRelationCount++;
          }
        }
      }
    }

    progress.Info(NumberToString(allRelationCount)+" relations read"+
                  ", "+NumberToString(selectedRelationCount)+" relation selected"+
                  ", "+NumberToString(writtenRelationCount)+" relations written");

    return scanner.Close() && writer.Close() && wayDataFile.Close() && nodeDataFile.Close();
  }
}

