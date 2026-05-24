#ifndef LIBOSMSCOUT_LANE_EVALUATION_COMMON_H
#define LIBOSMSCOUT_LANE_EVALUATION_COMMON_H

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <osmscout/GeoCoord.h>
#include <osmscout/db/Database.h>
#include <osmscout/util/Distance.h>
#include <osmscout/routing/RouteDescription.h>
#include <osmscout/routing/RoutingService.h>

namespace osmscout {

struct NodeLaneInfo
{
  bool oneway = false;
  uint8_t laneCount = 0;
  std::vector<std::string> laneTurns;
};

struct NodeSuggestedLaneInfo
{
  uint8_t from = 0;
  uint8_t to = 0;
  std::string turn;
};

struct RouteNodeInfo
{
  size_t nodeIndex = 0;
  GeoCoord coord;
  Distance distance;
  std::string name;
  std::string ref;
  std::string typeName;
  std::string turn;
  std::optional<NodeLaneInfo> lanes;
  std::optional<NodeSuggestedLaneInfo> suggestedLanes;
};

struct RouteInfo
{
  int version = 1;
  GeoCoord start;
  GeoCoord target;
  std::string database;
  std::vector<RouteNodeInfo> nodes;
};

void GetCarSpeedTable(std::map<std::string, double> &speedTable);

std::string MoveToTurnCommand(RouteDescription::DirectionDescription::Move move);

bool ComputeRoute(const std::string &databasePath,
                  const GeoCoord &start,
                  const GeoCoord &target,
                  RouteDescription &description);

void WriteRouteJson(const RouteDescription &description,
                    const GeoCoord &start,
                    const GeoCoord &target,
                    const std::string &databasePath,
                    std::ostream &out);

RouteInfo ReadRouteJson(const std::string &inputPath);

std::string CoordToFilenameComponent(const GeoCoord &coord);

std::string RouteFilename(const GeoCoord &start, const GeoCoord &target);

} // namespace osmscout

#endif // LIBOSMSCOUT_LANE_EVALUATION_COMMON_H
