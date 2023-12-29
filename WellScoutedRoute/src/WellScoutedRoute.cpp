/*
  WellscoutedRoute - a demo program for libosmscout
  Copyright (C) 2023  Tim Teulings

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

#include <chrono>
#include <iostream>
#include <iomanip>
#include <fstream>

#include <osmscout/db/Database.h>

#include <osmscout/routing/RoutePostprocessor.h>
#include <osmscout/routing/DBFileOffset.h>
#include <osmscout/routing/RoutingService.h>
#include <osmscout/routing/RouteDescriptionPostprocessor.h>

#include <osmscout/cli/CmdLineParsing.h>
#include <osmscout/util/Geometry.h>

#include <osmscoutgpx/GpxFile.h>
#include <osmscoutgpx/Export.h>

/**
 * Call me with:
 *   maps/arnsberg-regbez 51.5725728 7.4649565 20
 */
struct Arguments
{
  bool                              help=false;
  std::string                       router=osmscout::RoutingService::DEFAULT_FILENAME_BASE;
  osmscout::Vehicle                 vehicle=osmscout::Vehicle::vehicleBicycle;
  std::string                       databaseDirectory;
  osmscout::GeoCoord                start;
  osmscout::Distance                distance;
  bool                              debug=false;
  bool                              dataDebug=false;
  bool                              routeDebug=false;
  std::string                       routeJson;
};

struct Path
{
  osmscout::Distance      length;
  osmscout::ObjectFileRef object;
  size_t                  targetNodeIndex;
  uint8_t                 flags;
  double                  costs;
};

struct Node
{
  osmscout::Point     point;
  std::vector<Path>   paths;
  double              costs;
  size_t              previousNodeIndex;
};

struct Route
{
  size_t              nodeIndex;
  std::vector<size_t> path;
  double              costs;
  osmscout::Distance  length;
};

/*
struct Step
{
  double              costs;
  size_t              nodeIndex;
  size_t              previousNodeIndex;
  osmscout::Distance  length;
  osmscout::Distance  currentDirectDistance;
  osmscout::Distance  maximumDirectDistance;
  std::vector<size_t> path;

  bool operator<(const Step& other)
  {
    return this->costs/this->currentDirectDistance.AsMeter()<other.costs/this->currentDirectDistance.AsMeter();
  }
};*/

static void ExportGpx(const std::vector<Node>& graph,
                      const std::vector<size_t>& path,
                      const std::string& prefix,
                      size_t dumpIndex)
{
  osmscout::gpx::GpxFile gpxFile;
  osmscout::gpx::Route   route;

  route.name="Route";
  for (auto nodeIndex : path) {
    route.points.emplace_back(graph[nodeIndex].point.GetCoord());
  }

  gpxFile.name="Route";
  gpxFile.routes.push_back(route);

  osmscout::gpx::ExportGpx(gpxFile,"Route_"+prefix+"_"+std::to_string(dumpIndex)+".gpx");
}

static void ExportGpx(const std::vector<Node>& graph,
                      const std::vector<size_t>& path,
                      size_t currentNodeIndex,
                      const std::string& prefix,
                      size_t dumpIndex)
{
  osmscout::gpx::GpxFile gpxFile;
  osmscout::gpx::Route   route;

  route.name="Route";
  for (auto nodeIndex : path) {
    route.points.emplace_back(graph[nodeIndex].point.GetCoord());
  }
  route.points.emplace_back(graph[currentNodeIndex].point.GetCoord());

  gpxFile.name="Route";
  gpxFile.routes.push_back(route);

  osmscout::gpx::ExportGpx(gpxFile,"Route_"+prefix+"_"+std::to_string(dumpIndex)+".gpx");
}

static void DumpPath(const std::vector<Node>& graph,
                     const std::vector<size_t>& path) {
  std::cout << path[0] << "(" << graph[path[0]].point.GetId() << ")";

  for (size_t i = 1; i < path.size(); ++i) {
    const Node &previousNode = graph[path[i - 1]];

    size_t pathIdx = std::numeric_limits<size_t>::max();
    for (size_t idx = 0; idx < previousNode.paths.size(); idx++) {
      if (previousNode.paths[idx].targetNodeIndex == path[i]) {
        pathIdx = idx;
        break;
      }
    }

    std::cout << "-[" << previousNode.paths[pathIdx].object.GetName() << "]-";

    std::cout << path[i] << "(" << graph[path[i]].point.GetId() << ")";
  }

  std::cout << std::endl;
}

static void DumpPath(const std::vector<Node>& graph,
                     const std::vector<size_t>& path,
                     size_t currentNodeIndex)
{
  std::cout << path[0] << "(" << graph[path[0]].point.GetId() << ")";

  for (size_t i=1; i<path.size(); ++i) {
    const Node& previousNode=graph[path[i-1]];

    size_t pathIdx=std::numeric_limits<size_t>::max();
    for (size_t idx=0; idx<previousNode.paths.size(); idx++) {
      if (previousNode.paths[idx].targetNodeIndex==path[i]) {
        pathIdx=idx;
        break;
      }
    }

    std::cout << "-[" << previousNode.paths[pathIdx].object.GetName() << "]-";

    std::cout << path[i] << "(" << graph[path[i]].point.GetId() << ")";
  }

  const Node& previousNode=graph[path.back()];
  size_t pathIdx=std::numeric_limits<size_t>::max();
  for (size_t idx=0; idx<previousNode.paths.size(); idx++) {
    if (previousNode.paths[idx].targetNodeIndex==currentNodeIndex) {
      pathIdx=idx;
      break;
    }
  }

  std::cout << "-[" << previousNode.paths[pathIdx].object.GetName() << "]-";

  std::cout << currentNodeIndex << "(" << graph[currentNodeIndex].point.GetId() << ")" << std::endl;
}

static double CostOfPath(const Path& path)
{
  double costs=path.length.AsMeter();

  if (path.flags & osmscout::RouteNode::usableByCar) {
    costs+=5.0*path.length.AsMeter();
  }

  if (path.flags & osmscout::RouteNode::usableByFoot) {
    costs+=2.0*path.length.AsMeter();
  }

  return costs;
}

static void InitializeCmdLineParser(osmscout::CmdLineParser& argParser,
                                    Arguments& args)
{
  std::vector<std::string>  helpArgs{"h","help"};

  argParser.AddOption(osmscout::CmdLineFlag([&args](const bool& value) {
                        args.help=value;
                      }),
                      helpArgs,
                      "Return argument help",
                      true);

  argParser.AddOption(osmscout::CmdLineStringOption([&args](const std::string& value) {
                        args.routeJson=value;
                      }),
                      "routeJson",
                      "Dump resulting route as JSON to file",
                      false);

  argParser.AddOption(osmscout::CmdLineFlag([&args](const bool& value) {
                        args.debug=value;
                      }),
                      "debug",
                      "Enable debug output",
                      false);

  argParser.AddOption(osmscout::CmdLineFlag([&args](const bool& value) {
                        args.dataDebug=value;
                      }),
                      "dataDebug",
                      "Dump data nodes to sdt::cout",
                      false);

  argParser.AddOption(osmscout::CmdLineFlag([&args](const bool& value) {
                        args.routeDebug=value;
                      }),
                      "routeDebug",
                      "Dump route description data to std::cout",
                      false);

  argParser.AddOption(osmscout::CmdLineAlternativeFlag([&args](const std::string& value) {
                        if (value=="foot") {
                          args.vehicle=osmscout::Vehicle::vehicleFoot;
                        }
                        else if (value=="bicycle") {
                          args.vehicle=osmscout::Vehicle::vehicleBicycle;
                        }
                        else if (value=="car") {
                          args.vehicle=osmscout::Vehicle::vehicleCar;
                        }
                      }),
                      {"foot","bicycle","car"},
                      "Vehicle type to use for routing");

  argParser.AddOption(osmscout::CmdLineStringOption([&args](const std::string& value) {
                        args.router=value;
                      }),
                      "router",
                      "Router filename base");

  argParser.AddPositional(osmscout::CmdLineStringOption([&args](const std::string& value) {
                            args.databaseDirectory=value;
                          }),
                          "DATABASE",
                          "Directory of the first db to use");

  argParser.AddPositional(osmscout::CmdLineGeoCoordOption([&args](const osmscout::GeoCoord& value) {
                            args.start=value;
                          }),
                          "START",
                          "start coordinate");

  argParser.AddPositional(osmscout::CmdLineDoubleOption([&args](double value) {
                            args.distance=osmscout::Distance::Of<osmscout::Kilometer>(value);
                          }),
                          "DISTANCE",
                          "distance for roundtrip");
}

static bool HandleCmdLine(osmscout::CmdLineParser& argParser,
                          const Arguments& args)
{
  if (auto cmdLineParseResult=argParser.Parse();
     cmdLineParseResult.HasError()) {
    osmscout::log.Error() << "ERROR: " << cmdLineParseResult.GetErrorDescription();
    osmscout::log.Error() << argParser.GetHelp();
    return false;
  }

  if (args.help) {
    osmscout::log.Warn() << argParser.GetHelp();
    return false;
  }

  return true;
}

static std::vector<osmscout::RouteNode> LoadRouteNodes(osmscout::ConsoleProgress& progress,
                                                       const Arguments& args,
                                                       const osmscout::Distance& maxDistance)
{
  osmscout::FileScanner            routeReader;
  std::vector<osmscout::RouteNode> nodeList;

  try {
    routeReader.Open(args.databaseDirectory+"/"+"router.dat",osmscout::FileScanner::Sequential,true);

    [[maybe_unused]]
    osmscout::FileOffset indexOffset=routeReader.ReadFileOffset();
    uint32_t nodeCount=routeReader.ReadUInt32();
    [[maybe_unused]]
    uint32_t routeNodeTileMag=routeReader.ReadUInt32();

    progress.Info("Loading "+std::to_string(nodeCount)+" route nodes");

    uint32_t relevantCount=0;
    uint32_t irrelevantCount=0;

    nodeList.reserve(nodeCount);

    for (uint32_t i=1; i<=nodeCount; ++i) {
      progress.SetProgress(i,nodeCount,"Reading route nodes");
      osmscout::RouteNode node;

      node.Read(routeReader);

      size_t validPaths=0;
      for (const auto& path : node.paths) {
        if (path.IsUsable(args.vehicle) || path.IsRestricted(args.vehicle)) {
          ++validPaths;
        }
      }

      osmscout::Distance directDistance=osmscout::GetSphericalDistance(args.start,node.GetCoord());

      if (validPaths>0 && directDistance<=maxDistance) {
        ++relevantCount;

        nodeList.push_back(node);
      }
      else {
        ++irrelevantCount;
      }
    }

    osmscout::log.Info() << "Relevant: " << std::to_string(relevantCount) << ", irrelevant: " << std::to_string(irrelevantCount);

    routeReader.Close();
  }
  catch (osmscout::IOException& e) {
    osmscout::log.Error() << "Error: " << e.GetErrorMsg();
    routeReader.CloseFailsafe();

    throw e;
  }

  return nodeList;
}

static std::vector<Node> BuildGraph(osmscout::ConsoleProgress& progress,
                                    const std::vector<osmscout::RouteNode>& nodeList,
                                    osmscout::Vehicle vehicle)
{
  std::unordered_map<osmscout::Id,size_t> idToIndexMap;

  for (size_t i=0; i<nodeList.size(); ++i) {
    idToIndexMap[nodeList[i].GetId()]=i;
  }

  progress.SetAction("Building graph array");

  std::vector<Node> graph;

  graph.reserve(nodeList.size());

  for (const auto& routeNode : nodeList) {
    Node node;

    node.point=routeNode.GetPoint();

    for (const auto& routePath : routeNode.paths) {
      if (!routePath.IsUsable(vehicle) || routePath.IsRestricted(vehicle)) {
        continue;
      }

      Path path;

      path.length=routePath.distance;
      path.targetNodeIndex=idToIndexMap[routePath.id];
      path.flags=routePath.flags;
      path.object=routeNode.objects[routePath.objectIndex].object;

      node.paths.push_back(path);
    }

    graph.push_back(node);
  }

  return graph;
}

static void DumpStatistics(osmscout::ConsoleProgress& progress,
                           const std::vector<Node>& graph)
{
  size_t graphNodeCount=0;
  size_t graphEdgeCount=0;

  for (const auto& node : graph) {
    ++graphNodeCount;
    graphEdgeCount+=node.paths.size();
  }

  progress.Info("The graph contains "+std::to_string(graphNodeCount)+" nodes and "+std::to_string(graphEdgeCount)+" edges and has O("+std::to_string(graphNodeCount*graphEdgeCount)+")");
}

static size_t GetStartRouteNodeIndex(osmscout::ConsoleProgress& progress,
                                     const std::vector<Node>& graph,
                                     const osmscout::GeoCoord& start)
{
  size_t startNodeIndex=std::numeric_limits<size_t>::max();
  osmscout::Distance distance=osmscout::GetSphericalDistance(start,graph[0].point.GetCoord());

  for (size_t i=1; i<graph.size(); ++i) {
    osmscout::Distance currentDistance=osmscout::GetSphericalDistance(start,graph[i].point.GetCoord());

    if (currentDistance<distance) {
      startNodeIndex=i;
      distance=currentDistance;
    }
  }

  progress.Info("Closest route node is: "+std::to_string(startNodeIndex)+" at "+graph[startNodeIndex].point.GetCoord().GetDisplayText());

  return startNodeIndex;
}

static void CalculateGraphCosts(osmscout::ConsoleProgress& progress,
                                std::vector<Node>& graph)
{
  for (auto& node : graph) {
    for (auto& path : node.paths) {
      path.costs=CostOfPath(path);
    }
  }
}

static void BellmanFord(osmscout::ConsoleProgress& progress,
                        std::vector<Node>& graph,
                        size_t startNodeIndex,
                        const std::unordered_set<size_t>& restrictedNodeIndexes)
{
  for (auto& node : graph) {
    node.costs=std::numeric_limits<double>::max();
    node.previousNodeIndex=std::numeric_limits<size_t>::max();
  }

  graph[startNodeIndex].costs=0;

  for (size_t iter=1; iter<graph.size(); ++iter) {
    //progress.SetProgress(iter,graph.size(),"Calculating weight");
    bool somethingChanged=false;
    for (size_t nodeIndex=0; nodeIndex<graph.size(); ++nodeIndex) {
      const Node& fromNode=graph[nodeIndex];
      for (const auto& path : fromNode.paths) {
        if (restrictedNodeIndexes.find(path.targetNodeIndex)!=restrictedNodeIndexes.end()) {
          continue;
        }

        Node& toNode=graph[path.targetNodeIndex];
        if (fromNode.costs<std::numeric_limits<double>::max() &&
            fromNode.costs+path.costs<toNode.costs) {
          toNode.costs=fromNode.costs+path.costs;
          toNode.previousNodeIndex=nodeIndex;
          somethingChanged=true;
        }
      }
    }

    if (!somethingChanged) {
      break;
    }
  }
}

static bool CheckForNegativeCycles(osmscout::ConsoleProgress& progress,
                                   std::vector<Node>& graph,
                                   const std::unordered_set<size_t>& restrictedNodeIndexes)
{
  bool somethingChanged = false;
  for (size_t nodeIndex = 0; nodeIndex < graph.size(); ++nodeIndex) {
    Node &fromNode = graph[nodeIndex];
    for (auto &path: fromNode.paths) {
      Node &toNode = graph[path.targetNodeIndex];

      if (restrictedNodeIndexes.find(path.targetNodeIndex)!=restrictedNodeIndexes.end()) {
        continue;
      }

      if (fromNode.costs + path.costs < toNode.costs) {
        toNode.costs = fromNode.costs + path.costs;
        somethingChanged = true;
      }
    }

    if (somethingChanged) {
      progress.Error("A negative cycle exists");
      return false;
    }

  }

  return true;
}

static Route CalculateRoute(const std::vector<Node>& graph,size_t nodeIndex) {
  const Node &node = graph[nodeIndex];
  size_t previousNodeIndex = nodeIndex;

  assert(node.previousNodeIndex != std::numeric_limits<size_t>::max());

  Route route;

  route.nodeIndex = nodeIndex;
  route.costs = node.costs;
  route.length = osmscout::Distance::Zero();

  size_t pathLength = 1;
  size_t currentNodeIndex = node.previousNodeIndex;

  while (currentNodeIndex != std::numeric_limits<size_t>::max()) {
    const Node &currentNode = graph[currentNodeIndex];
    ++pathLength;
    currentNodeIndex = currentNode.previousNodeIndex;
  }

  route.path.reserve(pathLength);
  route.path.push_back(nodeIndex);

  currentNodeIndex = node.previousNodeIndex;
  while (currentNodeIndex != std::numeric_limits<size_t>::max()) {
    const Node &currentNode = graph[currentNodeIndex];

    for (const auto &path: graph[previousNodeIndex].paths) {
      if (path.targetNodeIndex == currentNodeIndex) {
        route.length += path.length;
        break;
      }
    }

    route.path.push_back(currentNodeIndex);
    previousNodeIndex = currentNodeIndex;
    currentNodeIndex = currentNode.previousNodeIndex;
  }

  std::reverse(route.path.begin(), route.path.end());

  return route;
}

int main(int argc, char* argv[])
{
  osmscout::CmdLineParser   argParser("Routing",
                                      argc,argv);
  Arguments                 args;

  InitializeCmdLineParser(argParser, args);
  if (!HandleCmdLine(argParser,args)) {
    return 1;
  }

  osmscout::log.Debug(args.debug);
  osmscout::log.Info(true);
  osmscout::log.Warn(true);
  osmscout::log.Error(true);

  osmscout::DatabaseParameter databaseParameter;
  osmscout::DatabaseRef       database=std::make_shared<osmscout::Database>(databaseParameter);

  if (!database->Open(args.databaseDirectory)) {
    osmscout::log.Error() << "Cannot open db";

    return 1;
  }

  size_t dumpCount=1;
  size_t dumpLimit=50;

  osmscout::log.Info() << "We have a route length of " << args.distance.AsString();


  //  Circle extend = 2 * PI * r
  //  r = extends / (2*PI)
  osmscout::log.Info() << "This could be a circle with radius " << args.distance/(2*M_PI) << " and diameter " << args.distance/M_PI;

  osmscout::Distance maxDistance=args.distance/M_PI*1.1;
  osmscout::Distance maxLength=args.distance*1.10;

  osmscout::log.Info() << "We tolerate a maximum distance up to " << maxDistance;
  osmscout::log.Info() << "We tolerate a maximum length up to " << maxLength;

  try {
    osmscout::ConsoleProgress progress;

    progress.SetAction("Loading routing graph");

    std::vector<osmscout::RouteNode> nodeList=LoadRouteNodes(progress,args,maxDistance);

    progress.SetAction("Building node index");

    std::vector<Node> graph=BuildGraph(progress,nodeList,args.vehicle);

    progress.SetAction("Calculate statistics");

    DumpStatistics(progress,graph);

    progress.SetAction("Finding closest route node");

    size_t startNodeIndex=GetStartRouteNodeIndex(progress,
                                                 graph,
                                                 args.start);
    // TODO: Check fro numeric_limits::max()

    // The following steps implement
    // https://en.wikipedia.org/wiki/Bellman%E2%80%93Ford_algorithm

    progress.SetAction("Calculate cost of each path");

    CalculateGraphCosts(progress,
                        graph);

    std::unordered_set<size_t> restrictedNodeIndexes;

    progress.SetAction("Bellman-Ford");

    BellmanFord(progress,graph,startNodeIndex,restrictedNodeIndexes);

    if (!CheckForNegativeCycles(progress,graph,restrictedNodeIndexes)) {
      return 1;
    }

    progress.SetAction("Build routes");

    std::vector<Route> routes;

    // TODO: Calculate the actual count to safe memory
    routes.reserve(graph.size());

    for (size_t nodeIndex = 0; nodeIndex < graph.size(); ++nodeIndex) {
      if (graph[nodeIndex].previousNodeIndex==std::numeric_limits<size_t>::max()) {
        continue;
      }

      routes.push_back(CalculateRoute(graph,nodeIndex));
    }

    progress.Info("we have found "+std::to_string(routes.size())+" routes");

    progress.SetAction("Find best route");

    std::sort(routes.begin(),routes.end(), [](const Route & a, const Route & b) {
      return a.costs<b.costs;
    });

    size_t backStartIndex=0;
    while (backStartIndex < routes.size() &&
           routes[backStartIndex].length < maxDistance) {
      ++backStartIndex;
    }

    auto awayRoute=routes.begin();
    while (awayRoute != routes.end() &&
           awayRoute->length < maxDistance) {
      ++awayRoute;
    }

    progress.SetAction("Collect routes back");

    // TODO: Calculate number of possible back routes and active progress

    size_t backRouteCount=1;
    while (awayRoute != routes.end()) {
      progress.SetProgress(backRouteCount, routes.size() - backStartIndex);
      ++backRouteCount;

      //DumpPath(graph, awayRoute->path);

      std::unordered_set<size_t> restrictedNodeIndexes;

      for (auto& nodeIndex : awayRoute->path) {
        if (nodeIndex != startNodeIndex) {
          restrictedNodeIndexes.insert(nodeIndex);
        }
      }

      BellmanFord(progress, graph, awayRoute->nodeIndex,restrictedNodeIndexes);

      if (!CheckForNegativeCycles(progress,graph,restrictedNodeIndexes)) {
        return 1;
      }

      if (graph[startNodeIndex].previousNodeIndex==std::numeric_limits<size_t>::max()) {
        ++awayRoute;
        continue;
      }

      Route backRoute=CalculateRoute(graph, startNodeIndex);

      if (awayRoute->length+backRoute.length<maxLength) {
        ++awayRoute;
        continue;
      }

      Route roundtripRoute;

      roundtripRoute.costs=awayRoute->costs+backRoute.costs;
      roundtripRoute.length=awayRoute->length+backRoute.length;

      auto awayStart=awayRoute->path.begin();
      auto awayEnd=awayRoute->path.end();

      --awayEnd;

      std::copy(awayStart,awayEnd,std::back_inserter(roundtripRoute.path));
      std::copy(backRoute.path.begin(),backRoute.path.end(),std::back_inserter(roundtripRoute.path));

      progress.Info("Route "+ std::to_string(roundtripRoute.costs) + " " + roundtripRoute.length.AsString());

      //DumpPath(graph, roundtripRoute.path);
      if (dumpCount<=dumpLimit) {
        //osmscout::log.Info() << "Exporting gpx route " << roundtripRoute.path.size();
        ExportGpx(graph,roundtripRoute.path,"Solution",dumpCount);
        ++dumpCount;
      }

      ++awayRoute;
    }

    // TODO: After collecting all possible routes (or stopping after a while) do the following:
    // - Cluster them regarding target regions
    // - Find the "best" route within each cluster
    // - Find the "best" route over all clusters
    // - Possibly we have to stop after a number of iterations, and let the user decide regarding the cluster
    //   And then optimize just these sub-routes?

    return 0;
  }
  catch (osmscout::IOException& e) {
    osmscout::log.Error() << "Error: " << e.GetErrorMsg();
  }
}