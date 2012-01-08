#ifndef OSMSCOUT_ROUTER_H
#define OSMSCOUT_ROUTER_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2012  Tim Teulings

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

#include <list>
#include <set>

#include <osmscout/TypeConfig.h>

// Datafiles
#include <osmscout/WayDataFile.h>

// Fileoffset by Id index
#include <osmscout/WayIndex.h>

// Reverse index
#include <osmscout/NodeUseIndex.h>

#include <osmscout/Route.h>

#include <osmscout/util/Cache.h>

namespace osmscout {

  /**
    Database instance initialisation parameter to influence the behaviour of the database
    instance.

    The following groups attributes are currently available:
    * cache sizes.
    */
  class OSMSCOUT_API RouterParameter
  {
  private:
    unsigned long wayIndexCacheSize;
    unsigned long wayCacheSize;

    bool          debugPerformance;

  public:
    RouterParameter();

    void SetWayIndexCacheSize(unsigned long wayIndexCacheSize);
    void SetWayCacheSize(unsigned long wayCacheSize);

    void SetDebugPerformance(bool debug);

    unsigned long GetWayIndexCacheSize() const;
    unsigned long GetWayCacheSize() const;

    bool IsDebugPerformance() const;
  };

  class OSMSCOUT_API Router
  {
  public: // Fix this
    struct NodeUse
    {
      Id              id;
      std::vector<Id> references;
    };

    typedef Cache<size_t,std::vector<NodeUse> > NodeUseCache;
    typedef const Way*                          WayPtr;

  private:
    bool                  isOpen;          //! true, if opened
    bool                  debugPerformance;

    std::string           path;             //! Path to the directory containing all files

    mutable NodeUseCache  nodeUseCache;     //! Cache for node use data, seems like the cache is more expensive than direct loading!?
    NodeUseIndex          nodeUseIndex;

    WayDataFile           wayDataFile;      //! Cached access to the 'ways.dat' file

    mutable FileScanner   nodeUseScanner;   //! File stream to the nodeuse.idx file

    TypeConfig            *typeConfig;      //! Type config for the currently opened map

  private:
    bool GetWays(std::map<Id,Way>& cache,
                 const std::set<Id>& ids,
                 std::vector<WayPtr>& refs);

    bool GetWay(std::map<Id,Way>& cache,
                Id id,
                WayPtr& ref);

    bool GetWay(const Id& id,
                WayRef& way) const;

    bool GetJoints(Id id,
                   std::set<Id>& wayIds) const;
    bool GetJoints(const std::set<Id>& ids,
                   std::set<Id>& wayIds) const;

  public:
    Router(const RouterParameter& parameter);
    virtual ~Router();

    bool Open(const std::string& path);
    bool IsOpen() const;
    void Close();

    void FlushCache();

    bool CalculateRoute(Id startWayId, Id startNodeId,
                        Id targetWayId, Id targetNodeId,
                        RouteData& route);

    bool TransformRouteDataToRouteDescription(const RouteData& data,
                                              RouteDescription& description);
    bool TransformRouteDataToWay(const RouteData& data,
                                 Way& way);

    void DumpStatistics();
  };
}

#endif
