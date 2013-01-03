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

#include <osmscout/DebugDatabase.h>

#include <algorithm>
#include <cassert>
#include <iostream>

#include <osmscout/TypeConfigLoader.h>

#include <osmscout/util/StopClock.h>

#include <osmscout/private/Math.h>

namespace osmscout {

  DebugDatabaseParameter::DebugDatabaseParameter()
  : nodeIndexCacheSize(1000),
    nodeCacheSize(1000),
    wayIndexCacheSize(2000),
    wayCacheSize(8000),
    relationIndexCacheSize(1000),
    relationCacheSize(1000)
  {
    // no code
  }

  void DebugDatabaseParameter::SetNodeIndexCacheSize(unsigned long nodeIndexCacheSize)
  {
    this->nodeIndexCacheSize=nodeIndexCacheSize;
  }

  void DebugDatabaseParameter::SetNodeCacheSize(unsigned long nodeCacheSize)
  {
    this->nodeCacheSize=nodeCacheSize;
  }

  void DebugDatabaseParameter::SetWayIndexCacheSize(unsigned long wayIndexCacheSize)
  {
    this->wayIndexCacheSize=wayIndexCacheSize;
  }

  void DebugDatabaseParameter::SetWayCacheSize(unsigned long wayCacheSize)
  {
    this->wayCacheSize=wayCacheSize;
  }

  void DebugDatabaseParameter::SetRelationIndexCacheSize(unsigned long relationIndexCacheSize)
  {
    this->relationIndexCacheSize=relationIndexCacheSize;
  }

  void DebugDatabaseParameter::SetRelationCacheSize(unsigned long relationCacheSize)
  {
    this->relationCacheSize=relationCacheSize;
  }

  unsigned long DebugDatabaseParameter::GetNodeIndexCacheSize() const
  {
    return nodeIndexCacheSize;
  }

  unsigned long DebugDatabaseParameter::GetNodeCacheSize() const
  {
    return nodeCacheSize;
  }

  unsigned long DebugDatabaseParameter::GetWayIndexCacheSize() const
  {
    return wayIndexCacheSize;
  }

  unsigned long DebugDatabaseParameter::GetWayCacheSize() const
  {
    return wayCacheSize;
  }

  unsigned long DebugDatabaseParameter::GetRelationIndexCacheSize() const
  {
    return relationIndexCacheSize;
  }

  unsigned long DebugDatabaseParameter::GetRelationCacheSize() const
  {
    return relationCacheSize;
  }

  DebugDatabase::DebugDatabase(const DebugDatabaseParameter& parameter)
   : isOpen(false),
     nodeDataFile("nodes.dat",
                  "node.idx",
                  parameter.GetNodeCacheSize(),
                  parameter.GetNodeIndexCacheSize()),
     relationDataFile("relations.dat",
                      "relation.idx",
                      parameter.GetRelationCacheSize(),
                      parameter.GetRelationIndexCacheSize()),
     wayDataFile("ways.dat",
                 "way.idx",
                  parameter.GetWayCacheSize(),
                  parameter.GetWayIndexCacheSize()),
     typeConfig(NULL)
  {
    // no code
  }

  DebugDatabase::~DebugDatabase()
  {
    delete typeConfig;
  }

  bool DebugDatabase::Open(const std::string& path)
  {
    assert(!path.empty());

    this->path=path;

    typeConfig=new TypeConfig();

    if (!LoadTypeData(path,*typeConfig)) {
      std::cerr << "Cannot load 'types.dat'!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

    if (!nodeDataFile.Open(path,FileScanner::LowMemRandom,true,FileScanner::LowMemRandom,true)) {
      std::cerr << "Cannot open 'nodes.dat'!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

    if (!wayDataFile.Open(path,FileScanner::LowMemRandom,true,FileScanner::LowMemRandom,true)) {
      std::cerr << "Cannot open 'ways.dat'!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

    if (!relationDataFile.Open(path,FileScanner::LowMemRandom,true,FileScanner::LowMemRandom,true)) {
      std::cerr << "Cannot open 'relations.dat'!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

    isOpen=true;

    return true;
  }

  bool DebugDatabase::IsOpen() const
  {
    return isOpen;
  }


  void DebugDatabase::Close()
  {
    nodeDataFile.Close();
    wayDataFile.Close();

    isOpen=false;
  }

  TypeConfig* DebugDatabase::GetTypeConfig() const
  {
    return typeConfig;
  }

  bool DebugDatabase::GetNode(const Id& id,
                              NodeRef& node) const
  {
    if (!IsOpen()) {
      return false;
    }

    std::vector<Id>      ids;
    std::vector<NodeRef> nodes;

    ids.push_back(id);

    if (GetNodes(ids,nodes)) {
      if (!nodes.empty()) {
        node=*nodes.begin();
        return true;
      }
    }

    return false;
  }

  bool DebugDatabase::GetNodes(const std::vector<Id>& ids,
                               std::vector<NodeRef>& nodes) const
  {
    if (!IsOpen()) {
      return false;
    }

    return nodeDataFile.Get(ids,nodes);
  }

  bool DebugDatabase::GetWay(const Id& id,
                             WayRef& way) const
  {
    if (!IsOpen()) {
      return false;
    }

    std::vector<Id>     ids;
    std::vector<WayRef> ways;

    ids.push_back(id);

    if (GetWays(ids,ways)) {
      if (!ways.empty()) {
        way=*ways.begin();
        return true;
      }
    }

    return false;
  }

  bool DebugDatabase::GetWays(const std::vector<Id>& ids,
                              std::vector<WayRef>& ways) const
  {
    if (!IsOpen()) {
      return false;
    }

    return wayDataFile.Get(ids,ways);
  }

  bool DebugDatabase::GetWays(const std::set<Id>& ids,
                              std::vector<WayRef>& ways) const
  {
    if (!IsOpen()) {
      return false;
    }

    return wayDataFile.Get(ids,ways);
  }

  bool DebugDatabase::GetRelation(const Id& id,
                                  RelationRef& relation) const
  {
    if (!IsOpen()) {
      return false;
    }

    std::vector<Id>          ids;
    std::vector<RelationRef> relations;

    ids.push_back(id);

    if (GetRelations(ids,relations)) {
      if (!relations.empty()) {
        relation=*relations.begin();
        return true;
      }
    }

    return false;
  }

  bool DebugDatabase::GetRelations(const std::vector<Id>& ids,
                                   std::vector<RelationRef>& relations) const
  {
    if (!IsOpen()) {
      return false;
    }

    return relationDataFile.Get(ids,relations);
  }
}
