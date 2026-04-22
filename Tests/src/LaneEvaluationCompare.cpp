#include <LaneEvaluationCompare.h>
#include <LaneEvaluationCommon.h>

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
#include <cairo/cairo.h>
#endif

namespace osmscout {

struct SuggestionDiff
{
  size_t nodeIndex = 0;
  double lat = 0.0;
  double lon = 0.0;
  std::string name;
  std::optional<NodeLaneInfo> lanes;
  std::optional<NodeSuggestedLaneInfo> oldSuggestion;
  std::optional<NodeSuggestedLaneInfo> newSuggestion;
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
      diff.lat = newNode.lat;
      diff.lon = newNode.lon;
      diff.name = newNode.name;
      diff.lanes = newNode.lanes;
      diff.oldSuggestion = oldNode.suggestedLanes;
      diff.newSuggestion = newNode.suggestedLanes;
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

static void RenderLaneOverlay(cairo_t *cairo,
                              const MercatorProjection &projection,
                              double junctionLat,
                              double junctionLon,
                              const std::optional<NodeLaneInfo> &lanes,
                              const std::optional<NodeSuggestedLaneInfo> &suggestion,
                              const std::string &label)
{
  Vertex2D centerPixel;
  GeoCoord junctionCoord(junctionLat, junctionLon);
  if (!projection.GeoToPixel(junctionCoord, centerPixel)) {
    return;
  }
  double centerX = centerPixel.GetX();
  double centerY = centerPixel.GetY();

  int laneCount = lanes.has_value() ? lanes->laneCount : 0;
  if (laneCount <= 0) {
    return;
  }

  double laneWidth = 8.0;
  double totalWidth = laneCount * laneWidth;
  double startX = centerX - totalWidth / 2.0;
  double laneHeight = 60.0;
  double topY = centerY - laneHeight / 2.0;

  for (int i = 0; i < laneCount; i++) {
    double x = startX + i * laneWidth;

    bool suggested = suggestion.has_value() &&
                     i >= suggestion->from && i <= suggestion->to;

    if (suggested) {
      cairo_set_source_rgba(cairo, 0.0, 0.8, 0.0, 0.6);
    } else {
      cairo_set_source_rgba(cairo, 0.5, 0.5, 0.5, 0.4);
    }
    cairo_rectangle(cairo, x, topY, laneWidth - 1, laneHeight);
    cairo_fill(cairo);

    cairo_set_source_rgba(cairo, 0.0, 0.0, 0.0, 0.7);
    cairo_set_line_width(cairo, 0.5);
    cairo_rectangle(cairo, x, topY, laneWidth - 1, laneHeight);
    cairo_stroke(cairo);
  }

  cairo_set_source_rgb(cairo, 0.0, 0.0, 0.0);
  cairo_set_font_size(cairo, 12.0);
  cairo_move_to(cairo, centerX - 40, topY - 8);
  cairo_show_text(cairo, label.c_str());
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

  size_t width = 600;
  size_t height = 400;
  GeoCoord center(diff.lat, diff.lon);

  MercatorProjection projection;
  projection.Set(center,
                 Magnification(Magnification::magBlock),
                 96.0,
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

  RenderLaneOverlay(cairo, projection, diff.lat, diff.lon,
                    diff.lanes, suggestion, label);

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
                       const CompareOptions &options,
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
              << " (" << diff.lat << ", " << diff.lon << ")";
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
