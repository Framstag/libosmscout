#include <LaneEvaluationCommon.h>

#include <cassert>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <osmscout/routing/SimpleRoutingService.h>
#include <osmscout/routing/RoutePostprocessor.h>
#include <osmscout/routing/RouteDescriptionPostprocessor.h>
#include <osmscout/util/LaneTurn.h>

#include <json/json.hpp>

namespace osmscout {

void GetCarSpeedTable(std::map<std::string, double> &speedTable)
{
  speedTable["highway_motorway"] = 110.0;
  speedTable["highway_motorway_trunk"] = 100.0;
  speedTable["highway_motorway_primary"] = 70.0;
  speedTable["highway_motorway_link"] = 60.0;
  speedTable["highway_motorway_junction"] = 60.0;
  speedTable["highway_trunk"] = 100.0;
  speedTable["highway_trunk_link"] = 60.0;
  speedTable["highway_primary"] = 70.0;
  speedTable["highway_primary_link"] = 60.0;
  speedTable["highway_secondary"] = 60.0;
  speedTable["highway_secondary_link"] = 50.0;
  speedTable["highway_tertiary"] = 55.0;
  speedTable["highway_tertiary_link"] = 55.0;
  speedTable["highway_unclassified"] = 50.0;
  speedTable["highway_road"] = 50.0;
  speedTable["highway_residential"] = 20.0;
  speedTable["highway_roundabout"] = 40.0;
  speedTable["highway_living_street"] = 10.0;
  speedTable["highway_service"] = 30.0;
}

std::string MoveToTurnCommand(RouteDescription::DirectionDescription::Move move)
{
  switch (move) {
  case RouteDescription::DirectionDescription::sharpLeft:
    return "Turn sharp left";
  case RouteDescription::DirectionDescription::left:
    return "Turn left";
  case RouteDescription::DirectionDescription::slightlyLeft:
    return "Turn slightly left";
  case RouteDescription::DirectionDescription::straightOn:
    return "Straight on";
  case RouteDescription::DirectionDescription::slightlyRight:
    return "Turn slightly right";
  case RouteDescription::DirectionDescription::right:
    return "Turn right";
  case RouteDescription::DirectionDescription::sharpRight:
    return "Turn sharp right";
  }
  assert(false);
  return "???";
}

bool ComputeRoute(const std::string &databasePath,
                  const GeoCoord &start,
                  const GeoCoord &target,
                  RouteDescription &description)
{
  DatabaseParameter databaseParameter;
  auto database = std::make_shared<Database>(databaseParameter);

  if (!database->Open(databasePath)) {
    log.Error() << "Cannot open database: " << databasePath;
    return false;
  }

  auto routingProfile = std::make_shared<FastestPathRoutingProfile>(database->GetTypeConfig());
  RouterParameter routerParameter;

  routingProfile->SetPenaltySameType(Meters(40));
  routingProfile->SetPenaltyDifferentType(Meters(250));
  routingProfile->SetMaxPenalty(std::chrono::seconds(10));

  auto router = std::make_shared<SimpleRoutingService>(database,
                                                       routerParameter,
                                                       RoutingService::DEFAULT_FILENAME_BASE);
  if (!router->Open()) {
    log.Error() << "Cannot open routing database";
    return false;
  }

  TypeConfigRef typeConfig = database->GetTypeConfig();
  std::map<std::string, double> carSpeedTable;
  GetCarSpeedTable(carSpeedTable);
  routingProfile->ParametrizeForCar(*typeConfig, carSpeedTable, 160.0);

  RoutingParameter parameter;

  auto startResult = router->GetClosestRoutableNode(start, *routingProfile, Kilometers(1));
  if (!startResult.IsValid() || startResult.GetRoutePosition().GetObjectFileRef().GetType() == refNode) {
    log.Error() << "Cannot find routable node near start: " << start.GetDisplayText();
    return false;
  }

  auto targetResult = router->GetClosestRoutableNode(target, *routingProfile, Kilometers(1));
  if (!targetResult.IsValid() || targetResult.GetRoutePosition().GetObjectFileRef().GetType() == refNode) {
    log.Error() << "Cannot find routable node near target: " << target.GetDisplayText();
    return false;
  }

  auto result = router->CalculateRoute(*routingProfile,
                                       startResult.GetRoutePosition(),
                                       targetResult.GetRoutePosition(),
                                       std::nullopt,
                                       parameter);
  if (!result.Success()) {
    log.Error() << "Route calculation failed";
    return false;
  }

  auto routeDescriptionResult = router->TransformRouteDataToRouteDescription(result.GetRoute());
  if (!routeDescriptionResult.Success()) {
    log.Error() << "Route description generation failed";
    return false;
  }

  std::list<RoutePostprocessor::PostprocessorRef> postprocessors{
    std::make_shared<RoutePostprocessor::DistanceAndTimePostprocessor>(),
    std::make_shared<RoutePostprocessor::StartPostprocessor>("Start"),
    std::make_shared<RoutePostprocessor::TargetPostprocessor>("Target"),
    std::make_shared<RoutePostprocessor::WayNamePostprocessor>(),
    std::make_shared<RoutePostprocessor::WayTypePostprocessor>(),
    std::make_shared<RoutePostprocessor::CrossingWaysPostprocessor>(),
    std::make_shared<RoutePostprocessor::DirectionPostprocessor>(),
    std::make_shared<RoutePostprocessor::LanesPostprocessor>(),
    std::make_shared<RoutePostprocessor::SuggestedLanesPostprocessor>(),
    std::make_shared<RoutePostprocessor::MotorwayJunctionPostprocessor>(),
    std::make_shared<RoutePostprocessor::DestinationPostprocessor>(),
    std::make_shared<RoutePostprocessor::MaxSpeedPostprocessor>(),
    std::make_shared<RoutePostprocessor::InstructionPostprocessor>(),
    std::make_shared<RoutePostprocessor::POIsPostprocessor>()
  };

  std::set<std::string, std::less<>> motorwayTypeNames{
    "highway_motorway", "highway_motorway_trunk", "highway_trunk", "highway_motorway_primary"};
  std::set<std::string, std::less<>> motorwayLinkTypeNames{
    "highway_motorway_link", "highway_trunk_link"};
  std::set<std::string, std::less<>> junctionTypeNames{
    "highway_motorway_junction"};

  std::vector<RoutingProfileRef> profiles{routingProfile};
  std::vector<DatabaseRef> databases{database};

  RoutePostprocessor postprocessor;
  if (!postprocessor.PostprocessRouteDescription(*routeDescriptionResult.GetDescription(),
                                                  profiles,
                                                  databases,
                                                  postprocessors,
                                                  motorwayTypeNames,
                                                  motorwayLinkTypeNames,
                                                  junctionTypeNames)) {
    log.Error() << "Route postprocessing failed";
    return false;
  }

  description = std::move(*routeDescriptionResult.GetDescription());

  router->Close();
  return true;
}

static std::string JsonEscape(const std::string &str)
{
  std::string buffer;
  buffer.reserve(str.size());
  for (char c : str) {
    switch (c) {
    case '"':  buffer.append("\\\""); break;
    case '\\': buffer.append("\\\\"); break;
    case '\n': buffer.append("\\n"); break;
    case '\r': buffer.append("\\r"); break;
    case '\t': buffer.append("\\t"); break;
    default:   buffer.push_back(c); break;
    }
  }
  return buffer;
}

void WriteRouteJson(const RouteDescription &description,
                    const GeoCoord &start,
                    const GeoCoord &target,
                    const std::string &databasePath,
                    std::ostream &out)
{
  out.precision(8);
  out.imbue(std::locale("C"));

  out << "{\n";
  out << "  \"version\": 1,\n";
  out << "  \"start\": { \"lat\": " << start.GetLat() << ", \"lon\": " << start.GetLon() << " },\n";
  out << "  \"target\": { \"lat\": " << target.GetLat() << ", \"lon\": " << target.GetLon() << " },\n";
  out << "  \"database\": \"" << JsonEscape(databasePath) << "\",\n";
  out << "  \"nodes\": [\n";

  size_t nodeIndex = 0;
  bool first = true;
  for (const auto &node : description.Nodes()) {
    if (!first) {
      out << ",\n";
    }
    first = false;

    out << "    {\n";
    out << "      \"nodeIndex\": " << nodeIndex << ",\n";
    out << "      \"lat\": " << node.GetLocation().GetLat() << ",\n";
    out << "      \"lon\": " << node.GetLocation().GetLon() << ",\n";
    out << "      \"distanceKm\": " << node.GetDistance().As<Kilometer>() << "";

    for (const auto &desc : node.GetDescriptions()) {
      if (auto nameDesc = dynamic_cast<RouteDescription::NameDescription *>(desc.get());
          nameDesc != nullptr) {
        out << ",\n";
        out << "      \"name\": \"" << JsonEscape(nameDesc->GetName()) << "\",\n";
        out << "      \"ref\": \"" << JsonEscape(nameDesc->GetRef()) << "\"";
      } else if (auto typeNameDesc = dynamic_cast<RouteDescription::TypeNameDescription *>(desc.get());
                 typeNameDesc != nullptr) {
        out << ",\n";
        out << "      \"type\": \"" << JsonEscape(typeNameDesc->GetName()) << "\"";
      } else if (auto directionDesc = dynamic_cast<RouteDescription::DirectionDescription *>(desc.get());
                 directionDesc != nullptr) {
        out << ",\n";
        out << "      \"turn\": \"" << JsonEscape(MoveToTurnCommand(directionDesc->GetTurn())) << "\"";
      }
    }

    auto laneDesc = node.GetDescription<RouteDescription::LaneDescription>();
    if (laneDesc) {
      out << ",\n";
      out << "      \"lanes\": {\n";
      out << "        \"oneway\": " << (laneDesc->IsOneway() ? "true" : "false") << ",\n";
      out << "        \"laneCount\": " << static_cast<int>(laneDesc->GetLaneCount()) << ",\n";
      out << "        \"laneTurns\": [";
      bool firstTurn = true;
      for (const auto &turn : laneDesc->GetLaneTurns()) {
        if (!firstTurn) {
          out << ", ";
        }
        firstTurn = false;
        out << "\"" << LaneTurnString(turn) << "\"";
      }
      out << "]\n";
      out << "      }";
    }

    auto suggestedDesc = node.GetDescription<RouteDescription::SuggestedLaneDescription>();
    if (suggestedDesc) {
      out << ",\n";
      out << "      \"suggestedLanes\": {\n";
      out << "        \"from\": " << static_cast<int>(suggestedDesc->GetFrom()) << ",\n";
      out << "        \"to\": " << static_cast<int>(suggestedDesc->GetTo()) << ",\n";
      out << "        \"turn\": \"" << LaneTurnString(suggestedDesc->GetTurn()) << "\"\n";
      out << "      }";
    }

    out << "\n";
    out << "    }";

    nodeIndex++;
  }

  out << "\n";
  out << "  ]\n";
  out << "}\n";
}

RouteInfo ReadRouteJson(const std::string &inputPath)
{
  std::ifstream file(inputPath);
  if (!file.is_open()) {
    throw std::runtime_error("Cannot open file: " + inputPath);
  }

  nlohmann::json j;
  file >> j;

  RouteInfo info;
  info.version = j.value("version", 1);
  info.start = GeoCoord(j["start"]["lat"].get<double>(),
                         j["start"]["lon"].get<double>());
  info.target = GeoCoord(j["target"]["lat"].get<double>(),
                          j["target"]["lon"].get<double>());
  info.database = j.value("database", "");

  for (const auto &nodeJson : j["nodes"]) {
    RouteNodeInfo nodeInfo;
    nodeInfo.nodeIndex = nodeJson["nodeIndex"].get<size_t>();
    nodeInfo.coord.Set(nodeJson["lat"].get<double>(), nodeJson["lon"].get<double>());
    nodeInfo.distance = Kilometers(nodeJson.value("distanceKm", 0.0));
    nodeInfo.name = nodeJson.value("name", "");
    nodeInfo.ref = nodeJson.value("ref", "");
    nodeInfo.typeName = nodeJson.value("type", "");
    nodeInfo.turn = nodeJson.value("turn", "");

    if (nodeJson.contains("lanes")) {
      NodeLaneInfo lanes;
      const auto &lanesJson = nodeJson["lanes"];
      lanes.oneway = lanesJson.value("oneway", false);
      lanes.laneCount = lanesJson.value("laneCount", 0);
      for (const auto &t : lanesJson["laneTurns"]) {
        lanes.laneTurns.push_back(t.get<std::string>());
      }
      nodeInfo.lanes = lanes;
    }

    if (nodeJson.contains("suggestedLanes")) {
      NodeSuggestedLaneInfo suggested;
      const auto &sugJson = nodeJson["suggestedLanes"];
      suggested.from = sugJson["from"].get<uint8_t>();
      suggested.to = sugJson["to"].get<uint8_t>();
      suggested.turn = sugJson["turn"].get<std::string>();
      nodeInfo.suggestedLanes = suggested;
    }

    info.nodes.push_back(std::move(nodeInfo));
  }

  return info;
}

std::string CoordToFilenameComponent(const GeoCoord &coord)
{
  std::ostringstream ss;
  ss.imbue(std::locale("C"));
  ss << std::fixed << std::setprecision(6);

  double lat = coord.GetLat();
  double lon = coord.GetLon();

  if (lat < 0) {
    ss << "n";
    lat = -lat;
  }
  ss << static_cast<int>(lat) << "p" << static_cast<int>((lat - static_cast<int>(lat)) * 1000000);

  ss << "_";

  if (lon < 0) {
    ss << "n";
    lon = -lon;
  }
  ss << static_cast<int>(lon) << "p" << static_cast<int>((lon - static_cast<int>(lon)) * 1000000);

  return ss.str();
}

std::string RouteFilename(const GeoCoord &start, const GeoCoord &target)
{
  return "route_" + CoordToFilenameComponent(start) + "__" + CoordToFilenameComponent(target) + ".json";
}

} // namespace osmscout
