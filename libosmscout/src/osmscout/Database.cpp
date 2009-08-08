/*
  TravelJinni - Openstreetmap offline viewer
  Copyright (C) 2009  Tim Teulings

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <osmscout/Database.h>

#include <cassert>
#include <cmath>
#include <iostream>

// For mesurement of time
#include <sys/time.h>

#include <osmscout/RoutingProfile.h>
#include <osmscout/TypeConfigLoader.h>

struct NodeCacheValueSizer : public Database::NodeCache::ValueSizer
{
  size_t GetSize(const std::vector<Node>& value) const
  {
    size_t memory=0;

    for (size_t i=0; i<value.size(); i++) {
      memory+=sizeof(Node)+value[i].tags.size()*sizeof(Tag);
    }

    return memory;
  }
};

struct WayCacheValueSizer : public Database::WayCache::ValueSizer
{
  size_t GetSize(const std::vector<Way>& value) const
  {
    size_t memory=0;

    for (size_t i=0; i<value.size(); i++) {
      memory+=sizeof(Way)+value[i].nodes.size()*sizeof(Point)+value[i].tags.size()*sizeof(Tag);
    }

    return memory;
  }
};

struct NodeUseCacheValueSizer : public Database::NodeUseCache::ValueSizer
{
  size_t GetSize(const std::vector<Database::NodeUse>& value) const
  {
    size_t memory=0;

    for (size_t i=0; i<value.size(); i++) {
      memory+=sizeof(Database::NodeUse);
    }

    return memory;
  }
};

Database::Database()
 : isOpen(false),
   nodeCache(2000),
   wayCache(3000),
   nodeUseCache(10), // Seems like the cache is more expensive than direct loading!?
   typeConfig(NULL)
{
  // no code
}

Database::~Database()
{
  // no code
}

bool Database::Open(const std::string& path)
{
  assert(!path.empty());

  this->path=path;

  std::string typeConfigFileName=path+"/"+"map.ost.xml";

  typeConfig=new TypeConfig();

  if (!LoadTypeConfig(typeConfigFileName.c_str(),*typeConfig)) {
    delete typeConfig;
    typeConfig=NULL;
    std::cerr << "Cannot load map.ost.xml!" << std::endl;
    return false;
  }

  std::cout << "Loading node index..." << std::endl;
  if (!nodeIndex.LoadNodeIndex(path)) {
    std::cerr << "Cannot load NodeIndex!" << std::endl;
    return false;
  }
  std::cout << "Loading node index done." << std::endl;

  std::cout << "Loading way index..." << std::endl;
  if (!wayIndex.LoadWayIndex(path)) {
    std::cerr << "Cannot load WayIndex!" << std::endl;
    return false;
  }
  std::cout << "Loading way index done." << std::endl;

  std::cout << "Loading area node index..." << std::endl;
  if (!areaNodeIndex.LoadAreaNodeIndex(path)) {
    std::cerr << "Cannot load AreaNodeIndex!" << std::endl;
    return false;
  }
  std::cout << "Loading area node index done." << std::endl;

  std::cout << "Loading area way index..." << std::endl;
  if (!areaWayIndex.LoadAreaWayIndex(path)) {
    std::cerr << "Cannot load AreaWayIndex!" << std::endl;
    return false;
  }
  std::cout << "Loading area way index done." << std::endl;

  std::cout << "Loading city street index..." << std::endl;
  if (!cityStreetIndex.LoadCityStreetIndex(path)) {
    std::cerr << "Cannot load CityStreetIndex!" << std::endl;
    return false;
  }
  std::cout << "Loading city street index done." << std::endl;

  std::cout << "Loading node use index..." << std::endl;
  if (!nodeUseIndex.LoadNodeUseIndex(path)) {
    std::cerr << "Cannot load NodeUseIndex!" << std::endl;
    return false;
  }
  std::cout << "Loading node use index done." << std::endl;

  isOpen=true;

  return true;
}

bool Database::IsOpen() const
{
  return isOpen;
}


void Database::Close()
{
  nodeCache.Flush();
  wayCache.Flush();
  nodeUseCache.Flush();

  isOpen=false;
}

TypeConfig* Database::GetTypeConfig() const
{
  return typeConfig;
}

size_t Database::GetMaximumPriority(const StyleConfig& styleConfig,
                                    double minlon, double minlat,
                                    double maxlon, double maxlat,
                                    double magnification,
                                    size_t maxNodes) const
{
  size_t           effectiveNodes;
  std::set<size_t> priorities;
  size_t           maxPriority=0;

  double realArea=(maxlon-minlon)*(maxlat-minlat);
  double tileArea=(GetTileY(maxlat)-GetTileY(minlat)+1)*
                  (GetTileX(maxlon)-GetTileX(minlon)+1)*
                  (GetTileWidth()*GetTileHeight());


  std::cout << "Real region: " << maxlon-minlon << "x" << maxlat-minlat << std::endl;
  std::cout << "Real area: " << realArea << std::endl;
  std::cout << "Number of tiles: " << (GetTileY(maxlat)-GetTileY(minlat)+1)*(GetTileX(maxlon)-GetTileX(minlon)+1) << std::endl;
  std::cout << "One tile area: " << GetTileWidth()*GetTileHeight() << std::endl;
  std::cout << "Tile area: " << tileArea << std::endl;
  std::cout << "Max nodes: " << maxNodes << std::endl;
  effectiveNodes=maxNodes*(tileArea/realArea);
  std::cout << "Effective max nodes: " << effectiveNodes << std::endl;

  size_t           iterations=0;
  size_t           nodes=0;
  std::set<TypeId> drawTypes;

  styleConfig.GetPriorities(priorities);

  styleConfig.GetNodeTypesWithMag(magnification,drawTypes);

  // Number of node nodes is only dependend on the magnification level

  for (std::set<TypeId>::const_iterator drawType=drawTypes.begin();
       drawType!=drawTypes.end();
       ++drawType) {
    nodes+=areaNodeIndex.GetNodes(*drawType,
                                 GetTileX(minlon),GetTileY(minlat),
                                 GetTileX(maxlon),GetTileY(maxlat));
  }

  // Number of way and area nodes is dependend on the priority

  for (std::set<size_t>::const_iterator priority=priorities.begin();
       priority!=priorities.end();
       ++priority) {
    iterations++;
    drawTypes.clear();

    styleConfig.GetAreaTypesWithPrio(maxPriority,drawTypes);
    styleConfig.GetWayTypesWithPrio(maxPriority,drawTypes);

    size_t newNodes=0;

    for (std::set<TypeId>::const_iterator drawType=drawTypes.begin();
         drawType!=drawTypes.end();
         ++drawType) {
      newNodes+=areaWayIndex.GetNodes(*drawType,
                                      GetTileX(minlon),GetTileY(minlat),
                                      GetTileX(maxlon),GetTileY(maxlat));
    }

    if (iterations>1 && nodes+newNodes>=effectiveNodes) {
      break;
    }

    maxPriority=*priority;
    nodes+=newNodes;
  }

  return maxPriority;
}

bool Database::GetWays(const StyleConfig& styleConfig,
                       double lonMin, double latMin,
                       double lonMax, double latMax,
                       double magnification,
                       size_t maxPriority,
                       std::list<Way>& ways) const
{
  std::set<Page>           pages;
  std::list<WayIndexEntry> wayIndexEntries;
  size_t                   wayAllCount=0;
  size_t                   waySelectedCount=0;
  size_t                   maxNodesCount=0;
  size_t                   cacheCount=0;
  size_t                   diskCount=0;
  std::string              file=path+"/"+"ways.dat";

  areaWayIndex.GetPages(styleConfig,
                        lonMin,latMin,lonMax,latMax,
                        magnification,
                        maxPriority,
                        pages);

  std::cout << "Getting way index entries..." << std::endl;

  wayIndex.GetWayPagesIndexEntries(pages,wayIndexEntries);

  std::cout << "Found " << wayIndexEntries.size() << " index entries" << std::endl;

  //
  // Loading relevant ways
  //

  std::cout << "Scanning ways..." << std::endl;

  if (!wayReader.IsOpen()) {
    if (!wayReader.Open(file)){
      std::cerr << "Error while opening ways.dat file!" << std::endl;
    }
  }

  for (std::list<WayIndexEntry>::iterator indexEntry=wayIndexEntries.begin();
       indexEntry!=wayIndexEntries.end();
       ++indexEntry) {
    Cache<size_t,std::vector<Way> >::CacheRef cacheRef;

    if (!wayCache.GetEntry(indexEntry->interval,cacheRef)) {
      if (!wayReader.ReadPageToBuffer(indexEntry->offset,indexEntry->size)) {
        std::cerr << "Error while reading page from ways.dat file!" << std::endl;
      }
      diskCount++;

      Cache<size_t, std::vector<Way> >::CacheEntry cacheEntry(indexEntry->interval);

      cacheRef=wayCache.SetEntry(cacheEntry);
      cacheRef->value.resize(indexEntry->count);

      for (size_t i=0; i<indexEntry->count; i++) {
        cacheRef->value[i].Read(wayReader);

        if (wayReader.HasError()) {
          std::cerr << "Error while reading data from ways.dat page!" << std::endl;
          wayReader.Close();
          break;
        }
      }
    }
    else {
      cacheCount++;
    }

    // Filter ways based on priority, type and covered area
    for (std::vector<Way>::const_iterator way=cacheRef->value.begin();
         way!=cacheRef->value.end();
         ++way) {
      if ((!way->IsArea() && styleConfig.IsWayVisible(way->type,maxPriority)) ||
          (way->IsArea() && styleConfig.IsAreaVisible(way->type,maxPriority))) {
        maxNodesCount=std::max(maxNodesCount,way->nodes.size());

        double wLonMin=way->nodes[0].lon;
        double wLonMax=way->nodes[0].lon;
        double wLatMin=way->nodes[0].lat;
        double wLatMax=way->nodes[0].lat;
        double match=false;

        for (size_t i=0; i<way->nodes.size(); i++) {
          wLonMin=std::min(wLonMin,way->nodes[i].lon);
          wLatMin=std::min(wLatMin,way->nodes[i].lat);
          wLonMax=std::max(wLonMax,way->nodes[i].lon);
          wLatMax=std::max(wLatMax,way->nodes[i].lat);

          if (way->nodes[i].lon>=lonMin && way->nodes[i].lon<=lonMax &&
              way->nodes[i].lat>=latMin && way->nodes[i].lat<=latMax) {
            match=true;
            break;
          }
        }

        if (way->IsArea()) {
          // Check that to be drawn area is completely in area
          if (!match &&
              lonMin>=wLonMin && latMin>=wLatMin &&
              lonMax<=wLonMax && latMax<=wLatMax) {
            match=true;
          }
        }

        if (match) {
          ways.push_back(*way);
          waySelectedCount++;
        }
      }
      wayAllCount++;
    }
  }

  std::cout << "Ways scanned: " << wayAllCount << std::endl;
  std::cout << "Ways selected: " << waySelectedCount << std::endl;
  std::cout << "Maximum nodes per way: " << maxNodesCount << std::endl;
  std::cout << "Way cache: " << cacheCount << " disk: " << diskCount << " cache size: " << wayCache.GetSize() << std::endl;

  return true;
}

bool Database::GetNodes(const StyleConfig& styleConfig,
                        double lonMin, double latMin,
                        double lonMax, double latMax,
                        double magnification,
                        size_t maxPriority,
                        std::list<Node>& nodes) const
{
  std::set<Page>            pages;
  std::list<NodeIndexEntry> nodeIndexEntries;
  size_t                    nodeAllCount=0;
  size_t                    nodeSelectedCount=0;
  size_t                    cacheCount=0;
  size_t                    diskCount=0;
  std::string               file=path+"/"+"nodes.dat";

  // Nodes

  areaNodeIndex.GetPages(styleConfig,
                         lonMin,latMin,lonMax,latMax,
                         magnification,
                         maxPriority,
                         pages);

  std::cout << "Getting node index entries..." << std::endl;


  nodeIndex.GetNodePagesIndexEntries(pages,nodeIndexEntries);

  std::cout << "Found " << nodeIndexEntries.size() << " index entries" << std::endl;

  //
  // Loading relevant nodes
  //


  if (!nodeReader.IsOpen()) {
    if (!nodeReader.Open(file)) {
      std::cerr << "Error while opening nodes.dat file!" << std::endl;
      return false;
    }
  }

  for (std::list<NodeIndexEntry>::iterator indexEntry=nodeIndexEntries.begin();
       indexEntry!=nodeIndexEntries.end();
       ++indexEntry) {
    Cache<size_t,std::vector<Node> >::CacheRef cacheRef;

    if (!nodeCache.GetEntry(indexEntry->interval,cacheRef)) {
      diskCount++;
      if (!nodeReader.ReadPageToBuffer(indexEntry->offset,indexEntry->size)) {
        std::cerr << "Error while reading page from nodes.dat file!" << std::endl;
        nodeReader.Close();
        return false;
      }

      Cache<size_t, std::vector<Node> >::CacheEntry cacheEntry(indexEntry->interval);

      cacheRef=nodeCache.SetEntry(cacheEntry);
      //cacheRef->value.resize(indexEntry->nodeCount);
      cacheRef->value.reserve(indexEntry->nodeCount);

      for (size_t i=0; i<indexEntry->nodeCount; i++) {
        Node node;

        node.Read(nodeReader);
        //cacheRef->value[i].Read(nodeStream);

        if (nodeReader.HasError()) {
          std::cerr << "Error while reading data from nodes.dat page!" << std::endl;
          nodeReader.Close();
          break;
        }

        cacheRef->value.push_back(node);
      }
    }
    else {
      cacheCount++;
    }

    for (std::vector<Node>::const_iterator node=cacheRef->value.begin();
         node!=cacheRef->value.end();
         ++node) {
      if (styleConfig.IsNodeVisible(node->type,magnification)) {
        if (node->lon>=lonMin && node->lon<=lonMax &&
            node->lat>=latMin && node->lat<=latMax) {
          nodes.push_back(*node);
          nodeSelectedCount++;
        }
      }
      nodeAllCount++;
    }
  }

  std::cout << "Nodes scanned: " << nodeAllCount << std::endl;
  std::cout << "Nodes selected: " << nodeSelectedCount << std::endl;
  std::cout << "Node cache: " << cacheCount << " disk: " << diskCount << " cache size: " << nodeCache.GetSize() << std::endl;

  return true;
}

bool Database::GetObjects(const StyleConfig& styleConfig,
                          double lonMin, double latMin,
                          double lonMax, double latMax,
                          double magnification,
                          size_t maxNodes,
                          std::list<Node>& nodes,
                          std::list<Way>& ways) const
{
  std::cout << "Getting objects from index..." << std::endl;

  size_t maxPriority;

  std::cout << "Analysing distribution for maximum priority..." << std::endl;

  maxPriority=GetMaximumPriority(styleConfig,
                                 lonMin,latMin,lonMax,latMax,
                                 magnification,
                                 maxNodes);

  std::cout << "Maximum priority is: " << maxPriority << std::endl;

  if (!GetWays(styleConfig,
               lonMin,latMin,lonMax,latMax,
               magnification,
               maxPriority,
               ways)) {
    return false;
  }

  if (!GetNodes(styleConfig,
                lonMin,latMin,lonMax,latMax,
                magnification,
                maxPriority,
                nodes)) {
    return false;
  }

  return true;
}

bool Database::GetNode(const Id& id, Node& node) const
{
  std::set<Id>              ids;
  std::list<NodeIndexEntry> indexEntries;
  std::string               file=path+"/"+"nodes.dat";

  ids.insert(id);

  nodeIndex.GetNodeIndexEntries(ids,indexEntries);

  if (indexEntries.size()==0) {
    return false;
  }

  if (!nodeReader.IsOpen()) {
    if (!nodeReader.Open(file)) {
      std::cerr << "Error while opening nodes.dat file!" << std::endl;
      return false;
    }
  }

  Cache<size_t,std::vector<Node> >::CacheRef cacheRef;

  const NodeIndexEntry& indexEntry=indexEntries.front();

  if (!nodeCache.GetEntry(indexEntry.interval,cacheRef)) {
    if (!nodeReader.ReadPageToBuffer(indexEntry.offset,indexEntry.size)) {
      std::cerr << "Error while reading page from nodes.dat file!" << std::endl;
      nodeReader.Close();
      return false;
    }

    Cache<size_t, std::vector<Node> >::CacheEntry cacheEntry(indexEntry.interval);

    cacheRef=nodeCache.SetEntry(cacheEntry);
    cacheRef->value.reserve(indexEntry.nodeCount);

    for (size_t i=1; i<=indexEntry.nodeCount; i++) {
      Node node;

      node.Read(nodeReader);

      if (nodeReader.HasError()) {
        std::cerr << "Error while reading data from nodes.dat page!" << std::endl;
        nodeReader.Close();
        return false;
      }

      cacheRef->value.push_back(node);
    }
  }

  for (std::vector<Node>::const_iterator n=cacheRef->value.begin();
       n!=cacheRef->value.end();
       ++n) {
    if (n->id==id) {
      node=*n;
      return true;
    }
  }

  return false;
}

bool Database::GetWay(const Id& id, Way& way) const
{
  std::set<Id>   ids;
  std::list<Way> ways;

  ids.insert(id);

  if (GetWays(ids,ways)) {
    if (ways.size()>0) {
      way=*ways.begin();
      return true;
    }
  }

  return false;
}

bool Database::GetWays(const std::set<Id>& ids, std::list<Way>& ways) const
{
  std::list<WayIndexEntry> indexEntries;
  std::string              file=path+"/"+"ways.dat";

  wayIndex.GetWayIndexEntries(ids,indexEntries);

  if (indexEntries.size()==0) {
    std::cout << "GetWays(): Ids not found in index" << std::endl;
    return false;
  }

  if (!wayReader.IsOpen()) {
    if (!wayReader.Open(file)) {
      std::cerr << "Error while opening ways.dat file!" << std::endl;
      return false;
    }
  }

  Cache<size_t,std::vector<Way> >::CacheRef cacheRef;

  for (std::list<WayIndexEntry>::const_iterator indexEntry=indexEntries.begin();
       indexEntry!=indexEntries.end();
       ++indexEntry) {
    if (!wayCache.GetEntry(indexEntry->interval,cacheRef)) {
      wayReader.ReadPageToBuffer(indexEntry->offset,indexEntry->size);

      Cache<size_t, std::vector<Way> >::CacheEntry cacheEntry(indexEntry->interval);

      cacheRef=wayCache.SetEntry(cacheEntry);
      cacheRef->value.resize(indexEntry->count);

      for (size_t i=0; i<indexEntry->count; i++) {
        cacheRef->value[i].Read(wayReader);

        if (wayReader.HasError()) {
          std::cerr << "Error while reading from ways.dat file!" << std::endl;
          wayReader.Close();
          return false;
        }
      }
    }

    for (std::vector<Way>::const_iterator w=cacheRef->value.begin();
         w!=cacheRef->value.end();
         ++w) {
      if (ids.find(w->id)!=ids.end()) {
        ways.push_back(*w);
      }
    }
  }

  return true;
}

bool Database::GetMatchingCities(const std::string& name,
                                 std::list<City>& cities,
                                 size_t limit, bool& limitReached) const
{
  return cityStreetIndex.GetMatchingCities(name,cities,limit,limitReached);
}

bool Database::GetMatchingStreets(Id urbanId, const std::string& name,
                                  std::list<Street>& streets,
                                  size_t limit, bool& limitReached) const
{
  return cityStreetIndex.GetMatchingStreets(urbanId,name,streets,limit,limitReached);
}

bool Database::GetJoints(Id id,
                         std::list<Way>& ways) const
{
  std::set<Id> ids;

  ids.insert(id);

  return GetJoints(ids,ways);
}

bool Database::GetJoints(const std::set<Id>& ids,
                         std::list<Way>& ways) const
{
  std::list<NodeUseIndexEntry> indexEntries;
  std::string                  file=path+"/"+"nodeuse.idx";
  std::set<Id>                 wayIds;
  std::set<Id>                 areaIds;

  ways.clear();

  nodeUseIndex.GetNodeIndexEntries(ids,indexEntries);

  if (indexEntries.size()==0) {
    std::cout << "GetJoints(): Ids not found in index" << std::endl;
    return false;
  }

  if (!nodeUseReader.IsOpen()) {
    if (!nodeUseReader.Open(file)) {
      std::cerr << "Cannot open nodeuse.idx file!" << std::endl;
      return false;
    }
  }

  Cache<size_t,std::vector<NodeUse> >::CacheRef cacheRef;

  for (std::list<NodeUseIndexEntry>::const_iterator indexEntry=indexEntries.begin();
       indexEntry!=indexEntries.end();
       ++indexEntry) {
    if (!nodeUseCache.GetEntry(indexEntry->interval,cacheRef)) {
      if (!nodeUseReader.ReadPageToBuffer(indexEntry->offset,indexEntry->size)) {
        std::cerr << "Cannot read nodeuse.idx page from file!" << std::endl;
        nodeUseReader.Close();
        return false;
      }

      Cache<size_t, std::vector<NodeUse> >::CacheEntry cacheEntry(indexEntry->interval);

      cacheEntry.value.reserve(indexEntry->count);
      cacheRef=nodeUseCache.SetEntry(cacheEntry);

      for (size_t i=0; i<indexEntry->count; i++) {
        NodeUse nodeUse;
        size_t  count;

        nodeUseReader.Read(nodeUse.id);
        nodeUseReader.Read(count);

        if (nodeUseReader.HasError()) {
          std::cerr << "Error while reading from nodeuse.idx file!" << std::endl;
          nodeUseReader.Close();
          return false;
        }

        nodeUse.references.resize(count);

        for (size_t j=0; j<count; j++) {
          nodeUseReader.Read(nodeUse.references[j]);
        }

        cacheRef->value.push_back(nodeUse);
      }
    }

    for (std::vector<NodeUse>::const_iterator w=cacheRef->value.begin();
         w!=cacheRef->value.end();
         ++w) {
      if (ids.find(w->id)!=ids.end()) {
        for (size_t i=0; i<w->references.size(); i++) {
          wayIds.insert(w->references[i]);
        }
      }
    }
  }

  GetWays(wayIds,ways);

  return true;

}

struct RNode
{
  Id        id;
  double    lon;
  double    lat;
  double    currentCost;
  double    estimateCost;
  double    overallCost;
  Reference ref;
  Id        prev;

  RNode()
   : id(0)
  {
  }

  RNode(Id id,
        double lon, double lat,
        const Reference& reference,
        Id prev)
   : id(id),
     lon(lon),
     lat(lat),
     currentCost(0),
     estimateCost(0),
     overallCost(0),
     ref(reference),
     prev(prev)
  {
    // no code
  }

  bool operator==(const RNode& node)
  {
    return id==node.id;
  }

  bool operator<(const RNode& node) const
  {
    return id<node.id;
  }
};

struct RNodeCostCompare
{
  bool operator()(const RNode& a, const RNode& b) const
  {
//    if (a.overallCost==b.overallCost) {
      //return a.currentCost<b.currentCost;
/*    }
    else {*/
      return a.overallCost<b.overallCost;
    //}
  }
};

double GetCost(double aLon, double aLat,
               double bLon, double bLat)
{
  return 6371.01*2*asin(sqrt(pow(sin((bLon-aLon)*M_PI/360),2)+
                             cos(aLon*M_PI/180)*cos(bLon*M_PI/180)*
                             pow(sin((bLat-aLat)*M_PI/360),2)));
}


bool CanBeTurnedInto(const Way& way, Id via, Id to)
{
  if (way.restrictions.size()==0) {
    return true;
  }

  for (std::vector<Way::Restriction>::const_iterator iter=way.restrictions.begin();
       iter!=way.restrictions.end();
       ++iter) {
    if (iter->type==Way::rstrAllowTurn) {
      // If our "to" is restriction "to" and our via is in the list of restriction "vias"
      // we can turn, else not.
      // If our !"to" is not the "to" of our restriction we also cannot turn.
      if (iter->members[0]==to) {
        for (size_t i=1; i<iter->members.size(); i++) {
          if (iter->members[i]==via) {
            return true;
          }
        }

        return false;
      }
      else {
        return false;
      }
    }
    else if (iter->type==Way::rstrForbitTurn) {
      // If our "to" is the restriction "to" and our "via" is in the list of the restriction "vias"
      // we cannot turn.
      if (iter->members[0]==to) {
        for (size_t i=1; i<iter->members.size(); i++) {
          if (iter->members[i]==via) {
            return false;
          }
        }
      }
    }
  }

  return true;
}

struct Follower
{
  std::list<Way>  ways;
};

typedef std::set<RNode,RNodeCostCompare> OpenList;
typedef OpenList::iterator RNodeRef;

struct RouteStep
{
  Id wayId;
  Id nodeId;
};

bool Database::CalculateRoute(Id startWayId, Id startNodeId,
                              Id targetWayId, Id targetNodeId,
                              RouteData& route)
{
  TypeId              type;
  std::map<Id,Way>    wayCache; // Does not gain much performance since we are caching in database, too
  std::map<Id,Follower> candidatesCache;
  Way                 currentWay;
  double              startLon,startLat;
  Way                 targetWay;
  double              targetLon,targetLat;
  OpenList            openList;
  std::map<Id,RNodeRef> openMap;
  std::map<Id,RNode>  closeMap;
  std::set<Id>        loaded;
  std::vector<size_t> costs;
  RoutingProfile      profile;

  std::cout << "=========== Routing start =============" << std::endl;

  timeval start;
  timeval stop;

  gettimeofday(&start,NULL);

  route.Clear();

  profile.SetTurnCostFactor(1/60/2); // 30 seconds

  type=typeConfig->GetWayTypeId(tagHighway,"motorway");
  assert(type!=typeIgnore);
  profile.SetTypeCostFactor(type,1/110.0);

  type=typeConfig->GetWayTypeId(tagHighway,"motorway_link");
  assert(type!=typeIgnore);
  profile.SetTypeCostFactor(type,1/90.0);

  type=typeConfig->GetWayTypeId(tagHighway,"trunk");
  assert(type!=typeIgnore);
  profile.SetTypeCostFactor(type,1/90.0);

  type=typeConfig->GetWayTypeId(tagHighway,"trunk_link");
  assert(type!=typeIgnore);
  profile.SetTypeCostFactor(type,1/70.0);

  type=typeConfig->GetWayTypeId(tagHighway,"primary");
  assert(type!=typeIgnore);
  profile.SetTypeCostFactor(type,1/70.0);

  type=typeConfig->GetWayTypeId(tagHighway,"primary_link");
  assert(type!=typeIgnore);
  profile.SetTypeCostFactor(type,1/60.0);

  type=typeConfig->GetWayTypeId(tagHighway,"secondary");
  assert(type!=typeIgnore);
  profile.SetTypeCostFactor(type,1/60.0);

  type=typeConfig->GetWayTypeId(tagHighway,"secondary_link");
  assert(type!=typeIgnore);
  profile.SetTypeCostFactor(type,1/50.0);

  type=typeConfig->GetWayTypeId(tagHighway,"tertiary");
  assert(type!=typeIgnore);
  profile.SetTypeCostFactor(type,1/55.0);

  type=typeConfig->GetWayTypeId(tagHighway,"unclassified");
  assert(type!=typeIgnore);
  profile.SetTypeCostFactor(type,1/50.0);

  type=typeConfig->GetWayTypeId(tagHighway,"road");
  assert(type!=typeIgnore);
  profile.SetTypeCostFactor(type,1/50.0);

  type=typeConfig->GetWayTypeId(tagHighway,"residential");
  assert(type!=typeIgnore);
  profile.SetTypeCostFactor(type,1/40.0);

  type=typeConfig->GetWayTypeId(tagHighway,"living_street");
  assert(type!=typeIgnore);
  profile.SetTypeCostFactor(type,1/10.0);

  type=typeConfig->GetWayTypeId(tagHighway,"service");
  assert(type!=typeIgnore);
  profile.SetTypeCostFactor(type,1/30.0);

  GetWay(startWayId,currentWay);
  GetWay(targetWayId,targetWay);

  size_t index=0;
  while (index<currentWay.nodes.size()) {
    if (currentWay.nodes[index].id==startNodeId) {
      startLon=currentWay.nodes[index].lon;
      startLat=currentWay.nodes[index].lat;
      break;
    }

    index++;
  }

  assert(index<currentWay.nodes.size());

  index=0;
  while (index<targetWay.nodes.size()) {
    if (targetWay.nodes[index].id==targetNodeId) {
      targetLon=targetWay.nodes[index].lon;
      targetLat=targetWay.nodes[index].lat;
      break;
    }

    index++;
  }

  assert(index<targetWay.nodes.size());

  RNode node=RNode(startNodeId,
                   startLon,
                   startLat,
                   Reference(startWayId,refWay),
                   0);

  node.currentCost=0.0;
  node.estimateCost=GetCost(startLon,
                            startLat,
                            targetLon,
                            targetLat);
  node.overallCost=openList.begin()->currentCost+openList.begin()->estimateCost;

  openList.insert(node);

  openMap[openList.begin()->id]=openList.begin();

  currentWay.id=0;

  std::vector<RNode> follower;

  follower.reserve(1000);
  std::map<Id,Follower>::const_iterator cacheEntry=candidatesCache.end();

  do {
    bool cachedFollower=false;
    //
    // Take entry from open list with lowest cost
    //

    RNode current=*openList.begin();
    openList.erase(openList.begin());
    openMap.erase(current.id);

    /*
    std::cout << "Cost " << current.currentCost << " " << current.overallCost;
    std::cout << " node " << current.id << " of " << (current.ref.type==refWay ? "way" : "area") << " " << current.ref.id;
    std::cout << " " << openList.size() << " " << closeMap.size() << std::endl;
    */

    //
    // Place all followers on list
    //

    follower.clear();

    // Get joint nodes in same way/area
    if (currentWay.id!=current.ref.id) {
      std::map<Id,Way>::const_iterator cacheEntry=wayCache.find(current.ref.id);
      if (cacheEntry!=wayCache.end()) {
        currentWay=cacheEntry->second;
        cachedFollower=false;
      }
      else {
        // TODO: Create cache entry first and then fill it directly...
        GetWay(current.ref.id,currentWay);
        wayCache[current.ref.id]=currentWay;
        cachedFollower=false;
      }
    }
    else {
      cachedFollower=true;
    }

    if (!profile.CanUse(currentWay.type)) {
      continue;
    }

    if (currentWay.IsArea()) {
      for (size_t i=0; i<currentWay.nodes.size(); ++i) {
        if (currentWay.nodes[i].id!=current.id) {

          std::map<Id,RNode>::iterator closeEntry=closeMap.find(currentWay.nodes[i].id);

          if (closeEntry!=closeMap.end()) {
            continue;
          }

          follower.push_back(RNode(currentWay.nodes[i].id,
                                   currentWay.nodes[i].lon,
                                   currentWay.nodes[i].lat,
                                   Reference(currentWay.id,refWay),
                                   current.id));
        }
      }
    }
    else {
      for (size_t i=0; i<currentWay.nodes.size(); ++i) {
        if (currentWay.nodes[i].id==current.id) {
          if (i>0 && !currentWay.IsOneway()) {
            std::map<Id,RNode>::iterator closeEntry=closeMap.find(currentWay.nodes[i-1].id);

            if (closeEntry==closeMap.end()) {
              follower.push_back(RNode(currentWay.nodes[i-1].id,
                                       currentWay.nodes[i-1].lon,
                                       currentWay.nodes[i-1].lat,
                                       Reference(currentWay.id,refWay),
                                       current.id));
            }
          }

          if (i<currentWay.nodes.size()-1) {
            std::map<Id,RNode>::iterator closeEntry=closeMap.find(currentWay.nodes[i+1].id);

            if (closeEntry==closeMap.end()) {
              follower.push_back(RNode(currentWay.nodes[i+1].id,
                                       currentWay.nodes[i+1].lon,
                                       currentWay.nodes[i+1].lat,
                                       Reference(currentWay.id,refWay),
                                       current.id));
            }
          }

          break;
        }
      }
    }

    // Get joint ways and areas

    if (!cachedFollower) {
      cacheEntry=candidatesCache.find(current.ref.id);

      if (cacheEntry==candidatesCache.end()) {
        std::pair<std::map<Id,Follower >::iterator,bool> result;

        result=candidatesCache.insert(std::pair<Id,Follower>(current.ref.id,Follower()));

        GetJoints(current.ref.id,result.first->second.ways);

        cacheEntry=result.first;
      }
    }

    // Get joint nodes in joint way/area

    for (std::list<Way>::const_iterator iter=cacheEntry->second.ways.begin();
         iter!=cacheEntry->second.ways.end();
         ++iter) {

      if (!profile.CanUse(iter->type)) {
        continue;
      }

      if (iter->IsArea()) {
        for (size_t i=0; i<iter->nodes.size(); i++) {
          if (iter->nodes[i].id!=current.id) {
            std::map<Id,RNode>::iterator closeEntry=closeMap.find(iter->nodes[i].id);

            if (closeEntry!=closeMap.end()) {
              continue;
            }

            follower.push_back(RNode(iter->nodes[i].id,
                                     iter->nodes[i].lon,
                                     iter->nodes[i].lat,
                                     Reference(iter->id,refArea),
                                     current.id));
          }
        }
      }
      else {
        for (size_t i=0; i<iter->nodes.size(); ++i) {
          if (iter->nodes[i].id==current.id  && CanBeTurnedInto(currentWay,iter->nodes[i].id,iter->id)) {

            if (i>0 && !iter->IsOneway()) {
              std::map<Id,RNode>::iterator closeEntry=closeMap.find(iter->nodes[i-1].id);

              if (closeEntry==closeMap.end()) {
                follower.push_back(RNode(iter->nodes[i-1].id,
                                         iter->nodes[i-1].lon,
                                         iter->nodes[i-1].lat,
                                         Reference(iter->id,refWay),
                                         current.id));
              }
            }

            if (i<iter->nodes.size()-1) {
              std::map<Id,RNode>::iterator closeEntry=closeMap.find(iter->nodes[i+1].id);

              if (closeEntry==closeMap.end()) {
                follower.push_back(RNode(iter->nodes[i+1].id,
                                         iter->nodes[i+1].lon,
                                         iter->nodes[i+1].lat,
                                         Reference(iter->id,refWay),
                                         current.id));
              }
            }

            break;
          }
        }
      }
    }

    for (std::vector<RNode>::iterator iter=follower.begin();
         iter!=follower.end();
         ++iter) {
      double currentCost=current.currentCost+
                         profile.GetCostFactor(currentWay.type)*GetCost(current.lon,
                                                                        current.lat,
                                                                        iter->lon,
                                                                        iter->lat);

      if (currentWay.id!=iter->id) {
        currentCost+=profile.GetTurnCostFactor();
      }

      std::map<Id,RNodeRef>::iterator openEntry=openMap.find(iter->id);

      if (openEntry!=openMap.end() &&
          openEntry->second->currentCost<=currentCost) {
        continue;
      }

      double estimateCost=profile.GetMinCostFactor()*GetCost(iter->lon,iter->lat,targetLon,targetLat);
      double overallCost=currentCost+estimateCost;

      if (openEntry!=openMap.end()) {
        iter->prev=current.id;
        iter->currentCost=currentCost;
        iter->estimateCost=estimateCost;
        iter->overallCost=overallCost;

        openList.erase(openEntry->second);
        openList.insert(*iter);
      }
      else {
        iter->currentCost=currentCost;
        iter->estimateCost=estimateCost;
        iter->overallCost=overallCost;

        openList.insert(*iter);
        openMap[openList.begin()->id]=openList.begin();
      }
    }

    //
    // Added current node to close map
    //

    closeMap[current.id]=current;

    //
    // Check if finished
    //

    if (current.id==targetNodeId) {
      /*
      std::cout << "Final Path:" << std::endl;
      std::cout << "-----------" << std::endl;
      std::cout << current.currentCost << "Km" << std::endl;
      std::cout << wayCache.size() << " ways cached" << std::endl;
      std::cout << std::endl;*/

      std::list<RouteStep> steps;

      while (current.prev!=0) {
        RouteStep step;

        step.wayId=current.ref.id;
        step.nodeId=current.id;

        steps.push_back(step);
        current=closeMap.find(current.prev)->second;
      }

      route.AddEntry(startWayId,startNodeId);
      for (std::list<RouteStep>::const_reverse_iterator step=steps.rbegin();
           step!=steps.rend();
           ++step) {
        route.AddEntry(step->wayId,step->nodeId);

        /*
        std::cout << "node " << step->nodeId << "( way " << step->wayId << ")";

        Way way;

        GetWay(step->wayId,way);

        for (size_t i=0; i<way.tags.size(); i++) {
          if (way.tags[i].key==tagName) {
            std::cout << " " << way.tags[i].value;
          }
          else if (way.tags[i].key==tagRef) {
            std::cout << " " << way.tags[i].value;
          }
        }

        std::cout << std::endl;*/
      }

      gettimeofday(&stop,NULL);

      timersub(&stop,&start,&stop);

      std::cout << "Time:" << stop.tv_sec << "." << start.tv_usec << std::endl;

      std::cout << "=========== Routing end ==============" << std::endl;
      return true;
    }
  } while (openList.size()>0);

  std::cout << "No route found!" << std::endl;
  std::cout << "=========== Routing end ==============" << std::endl;

  return false;
}

bool Database::TransformRouteDataToRouteDescription(const RouteData& data,
                                                    RouteDescription& description)
{
  Way                                              way,newWay;
  Id                                               node=0,newNode=0;
  std::list<RouteData::RouteEntry>::const_iterator iter;
  std::string                                      name,refName;
  double                                           distance=0.0;

  description.Clear();

  if (data.Entries().size()==0) {
    return true;
  }

  iter=data.Entries().begin();

  if (!GetWay(iter->GetWayId(),way)) {
    return false;
  }

  for (size_t i=0; i<way.nodes.size(); i++) {
    if (way.nodes[i].id==iter->GetNodeId()) {
      node=i;
      break;
    }
  }

  description.AddStep(0.0,RouteDescription::start,way.GetName(),way.GetRefName());
  description.AddStep(0.0,RouteDescription::drive,way.GetName(),way.GetRefName());

  iter++;

  while (iter!=data.Entries().end()) {
    if (!GetWay(iter->GetWayId(),newWay)) {
      return false;
    }

    for (size_t i=0; i<newWay.nodes.size(); i++) {
      if (newWay.nodes[i].id==iter->GetNodeId()) {
        distance+=GetCost(way.nodes[node].lon,way.nodes[node].lat,
                          newWay.nodes[i].lon,newWay.nodes[i].lat);
        newNode=i;
      }
    }

//    distance+=GetCosts(way

    bool match=false;

    while (iter!=data.Entries().end() &&
           newWay.GetName().empty() &&
           newWay.GetRefName().empty()) {
      ++iter;

      double lastLon=newWay.nodes[newNode].lon;
      double lastLat=newWay.nodes[newNode].lat;

      if (!GetWay(iter->GetWayId(),newWay)) {
        return false;
      }

      for (size_t i=0; i<newWay.nodes.size(); i++) {
        if (newWay.nodes[i].id==iter->GetNodeId()) {
          distance+=GetCost(lastLon,lastLat,
                            newWay.nodes[i].lon,newWay.nodes[i].lat);
          newNode=i;
          break;
        }
      }
    }

    if (!way.GetName().empty() &&
        way.GetName()==newWay.GetName()) {
      match=true;
    }
    else if (way.GetName().empty() &&
             !way.GetRefName().empty()
             && way.GetRefName()==newWay.GetRefName()) {
      match=true;
    }

    if (!match) {
      description.AddStep(distance,RouteDescription::switchRoad,newWay.GetName(),newWay.GetRefName());
      description.AddStep(distance,RouteDescription::drive,newWay.GetName(),newWay.GetRefName());
    }

    way=newWay;
    node=newNode;

    ++iter;
  }

  description.AddStep(distance,RouteDescription::reachTarget,newWay.GetName(),newWay.GetRefName());

  return true;
}

bool Database::TransformRouteDataToWay(const RouteData& data,
                                       Way& way)
{
  way.id=0;
  way.type=typeRoute;
  way.flags=0;
  way.layer=5;
  way.tags.clear();
  way.nodes.clear();
  way.nodes.reserve(data.Entries().size());

  if (data.Entries().size()==0) {
    return true;
  }

  for (std::list<RouteData::RouteEntry>::const_iterator iter=data.Entries().begin();
       iter!=data.Entries().end();
       ++iter) {
    Way w;

    if (!GetWay(iter->GetWayId(),w)) {
      return false;
    }

    for (size_t i=0; i<w.nodes.size(); i++) {
      if (w.nodes[i].id==iter->GetNodeId()) {
        way.nodes.push_back(w.nodes[i]);
        break;
      }
    }
  }

  return true;
}
void Database::DumpStatistics()
{
  nodeCache.DumpStatistics("Node cache",NodeCacheValueSizer());
  wayCache.DumpStatistics("Way cache",WayCacheValueSizer());
  nodeUseCache.DumpStatistics("Node use cache",NodeUseCacheValueSizer());

  nodeIndex.DumpStatistics();
  wayIndex.DumpStatistics();

  areaNodeIndex.DumpStatistics();
  areaWayIndex.DumpStatistics();

  cityStreetIndex.DumpStatistics();

  nodeUseIndex.DumpStatistics();
}

