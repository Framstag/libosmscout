#include <LaneEvaluationCompare.h>
#include <LaneEvaluationCommon.h>

#include <cmath>
#include <filesystem>
#include <iostream>
#include <map>
#include <vector>

#ifdef HAVE_MAP_CAIRO
#include <osmscout/Pixel.h>
#include <osmscout/db/Database.h>
#include <osmscout/projection/MercatorProjection.h>
#include <osmscoutmap/MapService.h>
#include <osmscoutmap/MapPainter.h>
#include <osmscoutmapcairo/MapPainterCairo.h>
#endif

namespace osmscout {

struct SuggestionDiff
{
  size_t nodeIndex = 0;
  GeoCoord coord;
  std::string name;
  std::optional<NodeLaneInfo> lanes;
  std::optional<NodeSuggestedLaneInfo> oldSuggestion;
  std::optional<NodeSuggestedLaneInfo> newSuggestion;
  std::vector<RouteNodeInfo> routeNodes;
  size_t junctionLocalIndex = 0;
};

static bool SuggestedLanesEqual(const std::optional<NodeSuggestedLaneInfo> &a,
                                const std::optional<NodeSuggestedLaneInfo> &b)
{
  if (!a.has_value() && !b.has_value()) {
    return true;
  }
  if (!a.has_value() || !b.has_value()) {
    return false;
  }
  return a->from == b->from && a->to == b->to && a->turn == b->turn;
}

static std::string FormatSuggestion(const std::optional<NodeSuggestedLaneInfo> &s)
{
  if (!s.has_value()) {
    return "(none)";
  }
  return "from=" + std::to_string(s->from) +
         " to=" + std::to_string(s->to) +
         " turn=" + s->turn;
}

static std::vector<SuggestionDiff> CompareRouteInfos(const RouteInfo &oldRoute,
                                                      const RouteInfo &newRoute)
{
  std::vector<SuggestionDiff> diffs;

  size_t maxNodes = std::min(oldRoute.nodes.size(), newRoute.nodes.size());

  for (size_t i = 0; i < maxNodes; i++) {
    const auto &oldNode = oldRoute.nodes[i];
    const auto &newNode = newRoute.nodes[i];

    if (!SuggestedLanesEqual(oldNode.suggestedLanes, newNode.suggestedLanes)) {
      SuggestionDiff diff;
      diff.nodeIndex = i;
      diff.coord = newNode.coord;
      diff.name = newNode.name;
      diff.lanes = newNode.lanes;
      diff.oldSuggestion = oldNode.suggestedLanes;
      diff.newSuggestion = newNode.suggestedLanes;

      constexpr size_t kNodeWindowRadius = 5;
      size_t windowStart = (i >= kNodeWindowRadius) ? (i - kNodeWindowRadius) : 0;
      size_t windowEnd = std::min(i + kNodeWindowRadius + 1, maxNodes);
      diff.junctionLocalIndex = i - windowStart;
      for (size_t j = windowStart; j < windowEnd; j++) {
        diff.routeNodes.push_back(newRoute.nodes[j]);
      }

      diffs.push_back(std::move(diff));
    }
  }

  if (oldRoute.nodes.size() != newRoute.nodes.size()) {
    std::cout << "  WARNING: Node count differs (old: " << oldRoute.nodes.size()
              << ", new: " << newRoute.nodes.size() << ")" << std::endl;
  }

  return diffs;
}

#ifdef HAVE_MAP_CAIRO

static void RenderLegend(cairo_t *cairo,
                         const SuggestionDiff &diff,
                         const std::optional<NodeSuggestedLaneInfo> &suggestion,
                         const std::string &label)
{
  const auto &node = diff.routeNodes[diff.junctionLocalIndex];

  std::vector<std::string> lines;
  lines.push_back(label);
  lines.push_back("node: " + std::to_string(diff.nodeIndex) +
                  "  (" + diff.coord.GetDisplayText() + ")");
  if (!node.name.empty()) {
    lines.push_back("name: " + node.name);
  }
  if (!node.typeName.empty()) {
    lines.push_back("type: " + node.typeName);
  }
  if (!node.turn.empty()) {
    lines.push_back("turn: " + node.turn);
  }
  if (node.lanes.has_value()) {
    std::string lanesStr = "lanes: count=" + std::to_string(node.lanes->laneCount) +
                           " oneway=" + (node.lanes->oneway ? "true" : "false");
    if (!node.lanes->laneTurns.empty()) {
      lanesStr += " turns=[";
      for (size_t i = 0; i < node.lanes->laneTurns.size(); i++) {
        if (i > 0) lanesStr += ",";
        lanesStr += node.lanes->laneTurns[i];
      }
      lanesStr += "]";
    }
    lines.push_back(lanesStr);
  }
  lines.push_back("suggested: " + FormatSuggestion(suggestion));

  double fontSize = 16.0;
  double padding = 10.0;
  double lineHeight = fontSize + 4.0;
  double x = padding;
  double y = padding;

  cairo_set_font_size(cairo, fontSize);

  double maxWidth = 0;
  for (const auto &line : lines) {
    cairo_text_extents_t ext;
    cairo_text_extents(cairo, line.c_str(), &ext);
    maxWidth = std::max(maxWidth, ext.width);
  }

  double boxW = maxWidth + 2 * padding;
  double boxH = lines.size() * lineHeight + 2 * padding;

  cairo_set_source_rgba(cairo, 1.0, 1.0, 1.0, 0.85);
  cairo_rectangle(cairo, x, y, boxW, boxH);
  cairo_fill(cairo);

  cairo_set_source_rgba(cairo, 0.0, 0.0, 0.0, 0.5);
  cairo_set_line_width(cairo, 1.0);
  cairo_rectangle(cairo, x, y, boxW, boxH);
  cairo_stroke(cairo);

  cairo_set_source_rgba(cairo, 0.0, 0.0, 0.0, 0.9);
  for (size_t i = 0; i < lines.size(); i++) {
    cairo_move_to(cairo, x + padding, y + padding + fontSize + i * lineHeight);
    cairo_show_text(cairo, lines[i].c_str());
  }
}

static void RenderLaneOverlay(cairo_t *cairo,
                              const MercatorProjection &projection,
                              const SuggestionDiff &diff,
                              const std::optional<NodeSuggestedLaneInfo> &suggestion,
                              const std::string &label)
{
  if (diff.routeNodes.size() < 2) {
    return;
  }

  struct PixelNode {
    Vertex2D pixel;
    bool valid = false;
  };

  std::vector<PixelNode> pixelNodes;
  pixelNodes.reserve(diff.routeNodes.size());
  for (const auto &rn : diff.routeNodes) {
    PixelNode pn;
    pn.valid = projection.GeoToPixel(rn.coord, pn.pixel);
    pixelNodes.push_back(pn);
  }

  double laneSpacing = 8.0;

  for (size_t seg = 0; seg + 1 < pixelNodes.size(); seg++) {
    if (!pixelNodes[seg].valid || !pixelNodes[seg + 1].valid) {
      continue;
    }

    const auto &segNode = diff.routeNodes[seg];
    int segLaneCount = segNode.lanes.has_value() ? segNode.lanes->laneCount : 0;
    if (segLaneCount <= 0) {
      continue;
    }

    double dx = pixelNodes[seg + 1].pixel.GetX() - pixelNodes[seg].pixel.GetX();
    double dy = pixelNodes[seg + 1].pixel.GetY() - pixelNodes[seg].pixel.GetY();
    double len = std::sqrt(dx * dx + dy * dy);
    if (len < 1e-6) {
      continue;
    }

    double perpX = -dy / len;
    double perpY = dx / len;

    double totalWidth = (segLaneCount - 1) * laneSpacing;

    bool adjacentToJunction = seg == diff.junctionLocalIndex ||
                              (diff.junctionLocalIndex > 0 && seg == diff.junctionLocalIndex - 1);

    for (int lane = 0; lane < segLaneCount; lane++) {
      double offset = -totalWidth / 2.0 + lane * laneSpacing;

      double x0 = pixelNodes[seg].pixel.GetX() + perpX * offset;
      double y0 = pixelNodes[seg].pixel.GetY() + perpY * offset;
      double x1 = pixelNodes[seg + 1].pixel.GetX() + perpX * offset;
      double y1 = pixelNodes[seg + 1].pixel.GetY() + perpY * offset;

      bool isSuggested = adjacentToJunction && suggestion.has_value() &&
                         lane >= suggestion->from && lane <= suggestion->to;

      if (isSuggested) {
        cairo_set_source_rgba(cairo, 0.0, 0.8, 0.0, 0.8);
        cairo_set_line_width(cairo, 5.0);
        cairo_set_dash(cairo, nullptr, 0, 0.0);
      } else {
        cairo_set_source_rgba(cairo, 0.5, 0.5, 0.5, 0.5);
        cairo_set_line_width(cairo, 3.0);
        double dashes[] = {6.0, 6.0};
        cairo_set_dash(cairo, dashes, 2, 0.0);
      }

      cairo_move_to(cairo, x0, y0);
      cairo_line_to(cairo, x1, y1);
      cairo_stroke(cairo);
    }
  }

  cairo_set_dash(cairo, nullptr, 0, 0.0);

  for (size_t ni = 0; ni < pixelNodes.size(); ni++) {
    if (!pixelNodes[ni].valid) {
      continue;
    }
    bool isIncoming = ni < diff.junctionLocalIndex;
    bool isJunction = ni == diff.junctionLocalIndex;

    if (isJunction) {
      cairo_set_source_rgba(cairo, 1.0, 1.0, 0.0, 0.9);
    } else if (isIncoming) {
      cairo_set_source_rgba(cairo, 0.0, 0.8, 0.0, 0.8);
    } else {
      cairo_set_source_rgba(cairo, 0.8, 0.0, 0.0, 0.8);
    }
    double radius = isJunction ? 8.0 : 5.0;
    cairo_arc(cairo, pixelNodes[ni].pixel.GetX(), pixelNodes[ni].pixel.GetY(),
              radius, 0, 2 * M_PI);
    cairo_fill(cairo);
  }

  RenderLegend(cairo, diff, suggestion, label);
}

static bool RenderJunctionImage(const std::string &databaseDir,
                                const std::string &stylesheet,
                                const SuggestionDiff &diff,
                                const std::string &outputPath,
                                bool useOldSuggestion)
{
  DatabaseParameter databaseParameter;
  auto database = std::make_shared<Database>(databaseParameter);
  if (!database->Open(databaseDir)) {
    log.Error() << "Cannot open database for rendering: " << databaseDir;
    return false;
  }

  auto mapService = std::make_shared<MapService>(database);
  auto styleConfig = std::make_shared<StyleConfig>(database->GetTypeConfig());
  if (!styleConfig->Load(stylesheet)) {
    log.Error() << "Cannot load stylesheet: " << stylesheet;
    return false;
  }

  size_t width = 1920;
  size_t height = 1080;
  GeoCoord center=diff.coord;

  MercatorProjection projection;
  projection.Set(center,
                 Magnification(Magnification::magBlock),
                 192.0,
                 width, height);

  MapParameter drawParameter;
  drawParameter.SetFontName("/usr/share/fonts/TTF/LiberationSans-Regular.ttf");
  drawParameter.SetFontSize(3.0);
  drawParameter.SetRenderSeaLand(false);
  drawParameter.SetRenderUnknowns(false);
  drawParameter.SetRenderBackground(true);

  AreaSearchParameter searchParameter;
  std::list<TileRef> tiles;
  MapData data;
  data.styleConfig = styleConfig;

  mapService->LookupTiles(projection, tiles);
  mapService->LoadMissingTileData(searchParameter, *styleConfig, tiles);
  mapService->AddTileDataToMapData(tiles, data);

  cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24,
                                                        static_cast<int>(width),
                                                        static_cast<int>(height));
  if (surface == nullptr) {
    log.Error() << "Cannot create cairo surface";
    return false;
  }

  cairo_t *cairo = cairo_create(surface);
  if (cairo == nullptr) {
    cairo_surface_destroy(surface);
    log.Error() << "Cannot create cairo context";
    return false;
  }

  std::vector<MapData> dataVec;
  dataVec.push_back(std::move(data));

  MapPainterCairo painter;
  painter.DrawMap(projection, drawParameter, dataVec, cairo);

  const auto &suggestion = useOldSuggestion ? diff.oldSuggestion : diff.newSuggestion;
  std::string label = useOldSuggestion ? "OLD: " : "NEW: ";
  label += FormatSuggestion(suggestion);

  RenderLaneOverlay(cairo, projection, diff, suggestion, label);

  if (cairo_surface_write_to_png(surface, outputPath.c_str()) != CAIRO_STATUS_SUCCESS) {
    log.Error() << "Cannot write PNG: " << outputPath;
    cairo_destroy(cairo);
    cairo_surface_destroy(surface);
    return false;
  }

  cairo_destroy(cairo);
  cairo_surface_destroy(surface);
  return true;
}

#endif // HAVE_MAP_CAIRO

static int CompareFile(const std::string &oldFile,
                       const std::string &newFile,
                       const std::string &basename,
                       [[maybe_unused]] const CompareOptions &options,
                       int &totalDiffs)
{
  RouteInfo oldRoute;
  RouteInfo newRoute;

  try {
    oldRoute = ReadRouteJson(oldFile);
  } catch (const std::exception &e) {
    std::cerr << "Error reading old file " << oldFile << ": " << e.what() << std::endl;
    return 1;
  }

  try {
    newRoute = ReadRouteJson(newFile);
  } catch (const std::exception &e) {
    std::cerr << "Error reading new file " << newFile << ": " << e.what() << std::endl;
    return 1;
  }

  auto diffs = CompareRouteInfos(oldRoute, newRoute);

  if (diffs.empty()) {
    return 0;
  }

  std::cout << "Route: " << basename << std::endl;

  int changed = 0;
  int added = 0;
  int removed = 0;

  for (const auto &diff : diffs) {
    std::cout << "  Node " << diff.nodeIndex
              << " (" << diff.coord.GetDisplayText() << ")";
    if (!diff.name.empty()) {
      std::cout << " \"" << diff.name << "\"";
    }

    if (!diff.oldSuggestion.has_value()) {
      std::cout << ": suggested lanes ADDED" << std::endl;
      added++;
    } else if (!diff.newSuggestion.has_value()) {
      std::cout << ": suggested lanes REMOVED" << std::endl;
      removed++;
    } else {
      std::cout << ": suggested lanes CHANGED" << std::endl;
      changed++;
    }

    std::cout << "    old: " << FormatSuggestion(diff.oldSuggestion) << std::endl;
    std::cout << "    new: " << FormatSuggestion(diff.newSuggestion) << std::endl;

#ifdef HAVE_MAP_CAIRO
    if (!options.imagesDir.empty() && !options.databaseDir.empty() && !options.stylesheet.empty()) {
      std::filesystem::create_directories(options.imagesDir);

      std::string stem = basename.substr(0, basename.find_last_of('.'));
      std::string oldImg = options.imagesDir + "/" + stem + "_node" + std::to_string(diff.nodeIndex) + "_old.png";
      std::string newImg = options.imagesDir + "/" + stem + "_node" + std::to_string(diff.nodeIndex) + "_new.png";

      RenderJunctionImage(options.databaseDir, options.stylesheet, diff, oldImg, true);
      RenderJunctionImage(options.databaseDir, options.stylesheet, diff, newImg, false);
      std::cout << "    images: " << oldImg << ", " << newImg << std::endl;
    }
#endif
  }

  std::cout << "  Summary: " << diffs.size() << " nodes with suggestion changes ("
            << changed << " changed, " << added << " added, " << removed << " removed)"
            << std::endl;

  totalDiffs += static_cast<int>(diffs.size());
  return 0;
}

int CompareRoutes(const std::string &oldPath,
                  const std::string &newPath,
                  const CompareOptions &options)
{
#ifndef HAVE_MAP_CAIRO
  if (!options.imagesDir.empty()) {
    std::cerr << "WARNING: Image rendering requested but built without cairo support. Skipping images." << std::endl;
  }
#endif

  namespace fs = std::filesystem;

  int totalDiffs = 0;
  int totalRoutes = 0;
  int routesWithDiffs = 0;

  if (fs::is_regular_file(oldPath) && fs::is_regular_file(newPath)) {
    totalRoutes = 1;
    std::string basename = fs::path(oldPath).filename().string();
    int diffs = 0;
    int rc = CompareFile(oldPath, newPath, basename, options, diffs);
    if (diffs > 0) {
      routesWithDiffs++;
    }
    totalDiffs = diffs;
    if (rc != 0) {
      return rc;
    }
  } else if (fs::is_directory(oldPath) && fs::is_directory(newPath)) {
    for (const auto &entry : fs::directory_iterator(oldPath)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".json") {
        continue;
      }

      std::string basename = entry.path().filename().string();
      std::string newFile = newPath + "/" + basename;

      if (!fs::exists(newFile)) {
        std::cout << "MISSING in new: " << basename << std::endl;
        continue;
      }

      totalRoutes++;
      int diffsBefore = totalDiffs;
      CompareFile(entry.path().string(), newFile, basename, options, totalDiffs);
      if (totalDiffs > diffsBefore) {
        routesWithDiffs++;
      }
    }
  } else {
    std::cerr << "Both paths must be either files or directories." << std::endl;
    return 1;
  }

  std::cout << "\n=== Overall Summary ===" << std::endl;
  std::cout << "Routes compared: " << totalRoutes << std::endl;
  std::cout << "Routes with differences: " << routesWithDiffs << std::endl;
  std::cout << "Total junction differences: " << totalDiffs << std::endl;

  return 0;
}

} // namespace osmscout
