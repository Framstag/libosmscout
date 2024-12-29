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

#include <osmscout/routing/RoutingService.h>
#include <osmscout/routing/RouteDescriptionPostprocessor.h>

#include <osmscout/cli/CmdLineParsing.h>
#include <osmscout/util/Geometry.h>
#include <osmscout/util/Progress.h>

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
  osmscout::Vehicle                 vehicle=osmscout::Vehicle::vehicleFoot;
  std::string                       databaseDirectory;
  osmscout::GeoCoord                start;
  osmscout::Distance                routeLength;
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
  osmscout::TypeInfoRef   type;
  uint8_t                 grade;

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
  double              averageCosts;
};

struct Step;

using StepRef = std::shared_ptr<Step>;

struct Step
{
  Route              route;
  StepRef            previousStep;
};

std::unordered_set<size_t> GetPathAsRestrictions(StepRef step)
{
  std::unordered_set<size_t> restrictions;

  while (step) {
    for (const auto& index: step->route.path) {
      restrictions.insert(index);
    }

    step=step->previousStep;
  }

  return restrictions;
}

struct StepTask
{
  size_t                     id;
  double                     costs;
  double                     averageCosts;
  double                     circliness;
  double                     traction;
  size_t                     stepCount;
  osmscout::Distance         currentDistance;
  osmscout::Distance         remainingDistance;
  StepRef                    previousStep;
  size_t                     startNodeIndex;
  std::unordered_set<size_t> nodeRestrictions;
  osmscout::Distance         requestedLength;
};

static void ExportGpxInitialStep(std::vector<Node>& graph,
                                 const osmscout::Point& start,
                                 const std::vector<StepRef>& steps)
{
  osmscout::gpx::GpxFile gpxFile;

  gpxFile.name="Route";

  osmscout::gpx::Waypoint startPoint(start.GetCoord());
  startPoint.name="Start";
  gpxFile.waypoints.push_back(startPoint);

  for (const auto& step : steps) {
    osmscout::gpx::Waypoint endPoint(graph[step->route.path.back()].point.GetCoord());
    endPoint.name=std::to_string(step->route.averageCosts);
    gpxFile.waypoints.push_back(endPoint);

    osmscout::gpx::Track track;
    track.name="Roundtrip";
    track.displayColor=osmscout::Color::DARK_RED;

    StepRef            currentStep=step;
    std::list<StepRef> path;

    while (currentStep) {
      path.push_front(currentStep);
      currentStep=currentStep->previousStep;
    }

    for (const StepRef& pathStep : path) {
      osmscout::gpx::TrackSegment segment;

      for (auto nodeIndex : pathStep->route.path) {
        segment.points.emplace_back(graph[nodeIndex].point.GetCoord());
      }

      track.segments.push_back(segment);
    }

    gpxFile.tracks.push_back(track);
  }

  osmscout::gpx::ExportGpx(gpxFile,"Initial-Step.gpx");
}

static void ExportGpx(std::vector<Node>& graph,
                      const StepRef& lastStep,
                      const std::string& prefix,
                      size_t dumpIndex)
{
  osmscout::gpx::GpxFile gpxFile;
  StepRef                currentStep=lastStep;
  std::list<StepRef>     path;
  std::string            name=prefix+"_"+std::to_string(lastStep->route.averageCosts)+"_"+std::to_string(dumpIndex);


  while (currentStep) {
    path.push_front(currentStep);
    currentStep=currentStep->previousStep;
  }

  gpxFile.name=name;

  osmscout::gpx::Waypoint startPoint(graph[path.front()->route.path.front()].point.GetCoord());
  startPoint.name="Start";
  gpxFile.waypoints.push_back(startPoint);

  size_t runningIndex=1;
  for (const StepRef& step : path) {
    /*
    osmscout::gpx::Waypoint stepPoint(graph[step->route.path.back()].point.GetCoord());
    stepPoint.name=std::to_string(runningIndex)+" - "+std::to_string(graph[step->route.path.back()].point.GetId());
    stepPoint.symbol="Square_green";
    gpxFile.waypoints.push_back(stepPoint);
    runningIndex++;*/

    for (auto nodeIndex : step->route.path) {
      osmscout::gpx::Waypoint stepPoint(graph[nodeIndex].point.GetCoord());
      stepPoint.name=std::to_string(runningIndex)+" - "+std::to_string(nodeIndex)+" - "+ std::to_string(graph[nodeIndex].point.GetId());
      stepPoint.symbol="Square_green";
      stepPoint.description=std::to_string(step->route.nodeIndex);
      gpxFile.waypoints.push_back(stepPoint);
      runningIndex++;
    }
  }

  osmscout::gpx::Track track;
  track.name="Roundtrip";
  track.displayColor=osmscout::Color::DARK_RED;

  for (const StepRef& step : path) {
    osmscout::gpx::TrackSegment segment;

    for (auto nodeIndex : step->route.path) {
      segment.points.emplace_back(graph[nodeIndex].point.GetCoord());
    }

    track.segments.push_back(segment);
  }

  gpxFile.tracks.push_back(track);

  osmscout::gpx::ExportGpx(gpxFile,name+".gpx");
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
  double costs=10.0;

  if (path.flags & osmscout::RouteNode::usableByBicycle) {
    costs=costs*2;
  }

  if (path.flags & osmscout::RouteNode::usableByCar) {
    costs=costs*4;
  }

  switch (path.grade) {
    case 1:
      costs=costs*0.8;
      break;
    case 2:
      costs=costs*0.9;
      break;
    case 3:
      costs=costs*1;
      break;
    case 4:
      costs=costs*2;
      break;
    case 5:
      costs=costs*5;
    default:
      break;
  }

  if (path.type->GetName()=="highway_track") {
    costs = costs * 0.8;
  }

  if (path.type->GetName()=="highway_path") {
    costs = costs * 1.2;
  }

  return costs*path.length.AsMeter();
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
                            args.routeLength=osmscout::Distance::Of<osmscout::Kilometer>(value);
                          }),
                          "LENGTH",
                          "length of the round trip");
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

static std::vector<osmscout::RouteNode> LoadRouteNodes(osmscout::Progress& progress,
                                                       const Arguments& args,
                                                       const osmscout::Distance& maxTurnPointDistance)
{
  osmscout::FileScanner            routeReader;
  std::vector<osmscout::RouteNode> nodeList;

  try {
    routeReader.Open(args.databaseDirectory+"/"+"router.dat",osmscout::FileScanner::Sequential,true);

    [[maybe_unused]] osmscout::FileOffset indexOffset=routeReader.ReadFileOffset();
    uint32_t nodeCount=routeReader.ReadUInt32();
    [[maybe_unused]] uint32_t routeNodeTileMag=routeReader.ReadUInt32();

    progress.Info("Loading {} route nodes",nodeCount);

    uint32_t relevantCount=0;
    uint32_t irrelevantCount=0;

    nodeList.reserve(nodeCount);

    for (uint32_t i=1; i<=nodeCount; ++i) {
      progress.SetProgress(i,nodeCount,"Reading route nodes");
      osmscout::RouteNode node;

      node.Read(routeReader);

      size_t validPaths=0;
      for (const auto& path : node.paths) {
        if (path.IsUsable(args.vehicle)) {
          ++validPaths;
        }
      }

      osmscout::Distance directDistance=osmscout::GetSphericalDistance(args.start,node.GetCoord());

      if (validPaths>0 && directDistance<=maxTurnPointDistance) {
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

static std::vector<Node> BuildGraph(osmscout::Progress& progress,
                                    const std::vector<osmscout::RouteNode>& nodeList,
                                    osmscout::Vehicle vehicle,
                                    const std::vector<osmscout::ObjectVariantData>& variantData)
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
      path.type=variantData[routeNode.objects[routePath.objectIndex].objectVariantIndex].type;
      path.grade=variantData[routeNode.objects[routePath.objectIndex].objectVariantIndex].grade;

      node.paths.push_back(path);
    }

    graph.push_back(node);
  }

  return graph;
}

static void DumpStatistics(osmscout::Progress& progress,
                           const std::vector<Node>& graph)
{
  size_t graphNodeCount=0;
  size_t graphEdgeCount=0;

  for (const auto& node : graph) {
    ++graphNodeCount;
    graphEdgeCount+=node.paths.size();
  }

  progress.Info("The graph contains {} nodes and {} edges and has O({})",
                graphNodeCount,
                graphEdgeCount,
                graphNodeCount*graphEdgeCount);
}

static size_t GetStartRouteNodeIndex(osmscout::Progress& progress,
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

static void CalculateGraphCosts(osmscout::Progress& /*progress*/,
                                std::vector<Node>& graph)
{
  for (auto& node : graph) {
    for (auto& path : node.paths) {
      path.costs=CostOfPath(path);
    }
  }
}

static void BellmanFord(osmscout::Progress& /*progress*/,
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
        if (restrictedNodeIndexes.contains(path.targetNodeIndex)) {
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

static bool CheckForNegativeCycles(osmscout::Progress& progress,
                                   std::vector<Node>& graph,
                                   const std::unordered_set<size_t>& restrictedNodeIndexes)
{
  bool somethingChanged = false;
  for (size_t nodeIndex = 0; nodeIndex < graph.size(); ++nodeIndex) {
    Node &fromNode = graph[nodeIndex];
    for (auto &path: fromNode.paths) {
      Node &toNode = graph[path.targetNodeIndex];

      if (restrictedNodeIndexes.contains(path.targetNodeIndex)) {
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

  route.averageCosts=route.costs/route.length.AsMeter();

  std::reverse(route.path.begin(), route.path.end());

  return route;
}

std::vector<StepRef> CalculateStepTask(std::vector<Node>& graph,
                                       const StepTask& task)
{
  std::vector<StepRef> steps;
  osmscout::ConsoleProgress progress;

  BellmanFord(progress,graph,task.startNodeIndex,task.nodeRestrictions);

  if (!CheckForNegativeCycles(progress,graph,task.nodeRestrictions)) {
    osmscout::log.Error() << "We have negative cycles";
    return steps;
  }

  for (size_t nodeIndex = 0; nodeIndex < graph.size(); ++nodeIndex) {
    size_t currentNodeIndex=nodeIndex;
    size_t previousNodeIndex=graph[currentNodeIndex].previousNodeIndex;
    osmscout::Distance lastStepDistance;

    if (previousNodeIndex==std::numeric_limits<size_t>::max()) {
      continue;
    }

    for (const auto &path: graph[previousNodeIndex].paths) {
      if (path.targetNodeIndex==currentNodeIndex) {
        lastStepDistance=path.length;
        break;
      }
    }

    osmscout::Distance routeLength;

    while (previousNodeIndex!=std::numeric_limits<size_t>::max() &&
           previousNodeIndex!=task.startNodeIndex) {
      for (const auto &path: graph[previousNodeIndex].paths) {
        if (path.targetNodeIndex == currentNodeIndex) {
          routeLength += path.length;
          break;
        }
      }

      currentNodeIndex = previousNodeIndex;
      previousNodeIndex = graph[currentNodeIndex].previousNodeIndex;
    }

    if (previousNodeIndex!=task.startNodeIndex) {
      // No route...
      continue;
    }

    // Every node where we directly stepped over the "requestedLength" distance is a resulting
    // Step
    if ((routeLength-lastStepDistance)<task.requestedLength &&
        routeLength>task.requestedLength) {
      StepRef step=std::make_shared<Step>();

      step->route=CalculateRoute(graph,nodeIndex);
      steps.push_back(step);
    }
  }

  return steps;
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

  /*
  double radius=500;
  double extend=2 * M_PI *radius;

  double testMaxDistance=radius*2;
  double routeLength=3*extend/4;

  double alpha=routeLength/(testMaxDistance/2);
  double d=testMaxDistance * sin(alpha / 2);

  std::cout << alpha << " " << d << std::endl;

  exit(0);*/

  osmscout::DatabaseParameter databaseParameter;
  osmscout::DatabaseRef       database=std::make_shared<osmscout::Database>(databaseParameter);

  if (!database->Open(args.databaseDirectory)) {
    osmscout::log.Error() << "Cannot open db";

    return 1;
  }

  osmscout::ObjectVariantDataFile objectVariantDataFile;

  if (!objectVariantDataFile.Load(*(database->GetTypeConfig()),
                                    osmscout::AppendFileToDir(database->GetPath(),
                                                              osmscout::RoutingService::GetData2Filename(osmscout::RoutingService::DEFAULT_FILENAME_BASE)))) {
    return 1;
  }

  double routeLengthGraceFactor=1.10;

  osmscout::Distance routeLengthWithGrace=(args.routeLength * routeLengthGraceFactor);
  // The furthest possible point away from the start
  osmscout::Distance maxTurnPointDistance= routeLengthWithGrace / 2.0;
  // We assume that we route away from our initial start and later on back to the start
  // That turn-point should be at least maxDistance away...
  osmscout::Distance maxDistance=maxTurnPointDistance;
  // The length of our final round trip route must be in the range [args.distance,maxDistance]
  osmscout::Distance maxLength=routeLengthWithGrace;

  osmscout::log.Info() << "We have a given round trip length of " << args.routeLength.AsString();
  osmscout::log.Info() << "With a grace factor of " << routeLengthGraceFactor << " we have a maximum round trip length of " << routeLengthWithGrace.AsString();
  osmscout::log.Info() << "In the degenerated case the furthest point away thus would be " << maxTurnPointDistance.AsString() << " away";


  //  Circle extend = 2 * PI * r
  //  r = extends / (2*PI)
  osmscout::log.Info() << "In the optimal case this also could be a circle with radius " << routeLengthWithGrace/(2*M_PI) << " and diameter " << routeLengthWithGrace/M_PI;


  osmscout::log.Info() << "We tolerate a maximum distance up to " << maxDistance;
  osmscout::log.Info() << "We tolerate a maximum length up to " << maxLength;

  try {
    osmscout::ConsoleProgress progress;

    progress.SetAction("Loading routing graph up to max turn point distance");

    std::vector<osmscout::RouteNode> nodeList=LoadRouteNodes(progress,args,maxDistance);

    progress.SetAction("Building node index");

    std::vector<Node> graph=BuildGraph(progress,
                                       nodeList,
                                       args.vehicle,
                                       objectVariantDataFile.GetData());

    progress.SetAction("Calculate statistics");

    DumpStatistics(progress,graph);

    progress.SetAction("Finding closest route node");

    size_t startNodeIndex=GetStartRouteNodeIndex(progress,
                                                 graph,
                                                 args.start);

    // Running id for tasks
    size_t nextId=0;

    // TODO: Check for numeric_limits::max()

    // The following steps implement
    // https://en.wikipedia.org/wiki/Bellman%E2%80%93Ford_algorithm

    progress.SetAction("Calculate cost of each path");

    CalculateGraphCosts(progress,
                        graph);

    std::unordered_set<size_t> globalRestrictions;

    globalRestrictions.insert(3163);

    // The number of intermediate routing steps
    osmscout::Distance stepDistance=routeLengthWithGrace / 4.0;

    osmscout::log.Info() << "In the first step we route for  " << stepDistance.AsString();

    StepTask initialTask;

    initialTask.id=nextId++;
    initialTask.stepCount=1;
    initialTask.costs=0.0;
    initialTask.averageCosts=0.0;
    initialTask.circliness=0.0;
    initialTask.traction=0.0;
    initialTask.currentDistance=osmscout::Distance::Zero();
    initialTask.remainingDistance=routeLengthWithGrace;
    initialTask.startNodeIndex=startNodeIndex;
    initialTask.requestedLength=stepDistance;
    initialTask.nodeRestrictions.insert(globalRestrictions.begin(),globalRestrictions.end());

    std::vector<StepTask> taskList;

    taskList.push_back(initialTask);

    size_t exportedRouteIndex=1;

    while (!taskList.empty()) {
      osmscout::log.Info() << "Task List size: " << taskList.size();

      StepTask task=taskList.back();
      taskList.pop_back();

      osmscout::log.Info() << "Calculating task: " << task.id << "/" << task.stepCount << " " << task.currentDistance << " " << task.remainingDistance << " " << task.averageCosts << " " << task.circliness << " " << task.traction;

      std::vector<StepRef> steps=CalculateStepTask(graph,task);

      if (task.stepCount==1) {
        ExportGpxInitialStep(graph,
                             graph[startNodeIndex].point,
                             steps);
      }

      size_t droppedTooFarCount=0;
      size_t droppedTooFarRestCount=0;
      size_t droppedNotBackToStart=0;
      size_t newTasksCount=0;

      for (const auto& step : steps) {
        osmscout::Distance remainingDistance;

        step->previousStep=task.previousStep;

        if (step->route.length < task.remainingDistance) {
          remainingDistance = task.remainingDistance - step->route.length;
        } else {
          remainingDistance = osmscout::Distance::Zero();
        }

        // TODO: Throw away routes, that are too far away
        osmscout::Distance distanceFromStart=osmscout::GetSphericalDistance(graph[startNodeIndex].point.GetCoord(),
                                                                            graph[step->route.path.back()].point.GetCoord());

        //osmscout::log.Info() << "? " << step->route.averageCosts << " " << step->route.length << " " << distanceFromStart.AsString();

        if (distanceFromStart>maxDistance) {
          osmscout::log.Debug() << "- Drop task result (too far): " << task.stepCount << " " << task.currentDistance << " " << task.remainingDistance << " " << task.costs << " " << task.averageCosts;
          droppedTooFarCount++;
          continue;
        }

        if (step->route.length+distanceFromStart>maxDistance) {
          osmscout::log.Debug() << "- Drop task result (too far for rest): " << task.stepCount << " " << task.currentDistance << " " << task.remainingDistance << " " << task.costs << " " << task.averageCosts;
          droppedTooFarRestCount++;
          continue;
        }

        if (remainingDistance.AsMeter() == 0.0) {
          if (step->route.path.back()!=startNodeIndex) {
            osmscout::log.Debug() << "- Drop task result (not back to start): " << task.stepCount << " " << task.currentDistance << " " << task.remainingDistance << " " << task.costs << " " << task.averageCosts;
            droppedNotBackToStart++;

            //ExportGpx(graph, step, "Route", exportedRouteIndex);
            //exportedRouteIndex++;

            continue;
          }

          osmscout::log.Info() << "+ Use Route: " << exportedRouteIndex << " " << step->route.averageCosts << " " << step->route.path.front() << "-"<< step->route.path.back() << " " << step->route.length;

          ExportGpx(graph, step, "Route", exportedRouteIndex);
          exportedRouteIndex++;

          continue;
        }

        StepTask newTask;

        newTask.id=nextId++;
        newTask.stepCount = task.stepCount + 1;
        newTask.costs=task.costs+step->route.costs;
        newTask.currentDistance=task.currentDistance+step->route.length;
        newTask.averageCosts=newTask.costs/newTask.currentDistance.AsMeter();

        double alpha=newTask.currentDistance.AsMeter()/(maxDistance.AsMeter()/2);
        double d=maxDistance.AsMeter() * sin(alpha / 2);

        //osmscout::log.Info() << "Circliness: " << d << " " << distanceFromStart.AsMeter();

        newTask.circliness=std::abs(d-distanceFromStart.AsMeter());
        newTask.traction=task.traction+newTask.circliness*newTask.circliness;
        newTask.remainingDistance = remainingDistance;
        newTask.startNodeIndex = step->route.path.back();
        newTask.requestedLength = stepDistance;
        newTask.previousStep = step;
        newTask.nodeRestrictions=GetPathAsRestrictions(step);
        newTask.nodeRestrictions.insert(globalRestrictions.begin(),globalRestrictions.end());

        // We are close to the end of the path, it is now allow to route back to the start
        if (remainingDistance<stepDistance) {
          newTask.nodeRestrictions.erase(startNodeIndex);
        }

        /*
        if (newTask.circliness<100.0) {
          ExportGpx(graph,step,"Route", exportedRouteIndex);
          exportedRouteIndex++;
        }*/

        taskList.push_back(newTask);

        newTasksCount++;
      }

      progress.Info(std::to_string(steps.size()) + " new routes, " + std::to_string(droppedTooFarCount) + " too far, " +std::to_string(droppedTooFarRestCount)+" too far for back " + std::to_string(droppedNotBackToStart) + " not back to start, " + std::to_string(newTasksCount) + " new tasks");

      // Best cost/length factor first
      std::sort(taskList.begin(),taskList.end(), [](const StepTask& a, const StepTask& b) {
        // Highest stepCount first
        if (a.stepCount!=b.stepCount) {
          return a.stepCount<b.stepCount;
        }

        // lowest Traction first
        //return a.traction>b.traction;
        return a.circliness>b.circliness;
        //return a.averageCosts>b.averageCosts;
      });

      /*
      osmscout::log.Info() << "TaskList:";
      for (const auto& task : taskList) {
        osmscout::log.Info()
        << "* "
        << task.id << "/" << task.stepCount
        << " " << task.currentDistance
        << " " << task.remainingDistance
        << " " << task.costs
        << " " << task.averageCosts
        << " " << task.circliness
        << " " << task.traction;
      }*/
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