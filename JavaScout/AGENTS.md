# AGENTS.md — JavaScout

## Project Identity

- **What**: JavaFX desktop application for offline OSM map rendering, routing, navigation, and location lookup. Java client for libosmscout.
- **Parent**: [libosmscout](http://libosmscout.sourceforge.net/) — C++20 library for offline OSM data
- **Version**: 1.0-SNAPSHOT
- **GroupId**: `net.sf.libosmscout`
- **ArtifactId**: `javascout`
- **Homepage**: http://libosmscout.sourceforge.net/
- **Support**: Matrix `#libosmscout.matrix.org`, mailing list on SourceForge

## Repository Map

| Directory | Purpose |
|-----------|---------|
| `JavaScout/` | **JavaFX desktop app** — shell, map renderer, search, routing, navigation, favorites, GPX tracks |
| `JavaScout/src/main/java/com/framstag/libosmscout/` | App source: controllers, overlays, renderer, config |
| `JavaScout/src/main/resources/com/framstag/libosmscout/` | FXML layout, CSS styles, SVG assets |
| `JavaScout/src/test/java/com/framstag/libosmscout/` | Unit tests (JUnit 5) |
| `../libosmscout-client-java` | **Java client library** — JNI bridge to libosmscout C++ |
| `../libosmscout-client-java/java/com/framstag/libosmscout/client` | Java API classes (OSMScoutClient, data types, callbacks) |
| `../libosmscout-client-java/src` | C++ JNI implementation (OSMScoutClient.cpp) |
| `../libosmscout-binding` | SWIG-based Java bindings (older, separate from client-java) |
| `../Java` | Standalone Java examples (LocationLookup, OpenDatabase, Renderer, Routing) |
| `../Android/OsmScoutLib` | Android library bindings (separate from JavaScout) |
| `../openspec/specs` | OpenSpec specifications for JavaScout features |
| `../openspec/changes/archive` | Archived change proposals for JavaScout features |

## Build Systems

### Maven (primary)

```bash
cd JavaScout
mvn package                          # build JAR
mvn javafx:run                       # run via JavaFX Maven plugin
mvn test -Dnative.lib.dir=/path/to/libs  # run tests with native lib
```

The native C++ library (`libosmscout_client_java`) must be built separately via Meson in the parent project.

### Running Tests

```bash
cd JavaScout
mvn test -Dnative.lib.dir=/path/to/build/lib
```

Tests requiring native library:
- `OSMScoutClientNavigationTest` — route calculation via JNI
- `OSMScoutClientNavigationLiveTest` — live navigation via JNI
- `OSMScoutClientImportGpxTest` — GPX import via JNI

Pure unit tests (no native lib needed):
- `ConfigTest`, `MapRendererStateTest`, `TrackPlayerTest`
- `DescriptionEntryTest`, `ObjectDescriptionTest`, `TrackPointTest`

## Dependencies

| Dependency | Version | Scope |
|------------|---------|-------|
| JavaFX controls | 21 | Compile |
| JavaFX FXML | 21 | Compile |
| libosmscout-client-java | 1.0-SNAPSHOT | Compile |
| JUnit Jupiter | 5.10.2 | Test |

Native runtime dependencies (via `libosmscout_client_java`):
- libosmscout (core), libosmscout-map, libosmscout-map-cairo, libosmscout-client
- libosmscout-gpx (optional, gated by `OSMSCOUT_BUILD_GPX`)
- Cairo, Pango, PangoCairo, JNI (JDK headers)

## Code Conventions

### Language & Standards
- **Java 17+** with JavaFX 21
- Package: `com.framstag.libosmscout` for app, `com.framstag.libosmscout.client` for client library
- C++ JNI bridge: C++20, namespace `osmscout`
- No DI framework — objects constructed manually
- Async operations via `javafx.concurrent.Task` on daemon threads
- UI updates marshalled via `Platform.runLater()`

### File Layout (app)
```
JavaScout/src/main/java/com/framstag/libosmscout/
  JavaScout.java              ← Entry point (CLI arg parsing)
  JavaScoutApp.java           ← JavaFX Application subclass
  MainController.java         ← FXML controller, orchestrates all features
  MapRenderer.java            ← Canvas rendering, debounce, location marker
  MapInteractionHandler.java  ← Pan, zoom, long-press gestures
  SearchOverlay.java          ← Location search + favorites tab
  RoutePanel.java             ← Route input, calculation, navigation status
  DescriptionOverlay.java     ← Object description popup
  FavLocationDialog.java      ← Favorites CRUD dialog
  FavoritePickerDialog.java   ← Favorites picker for route input
  TrackPlayer.java            ← GPX track playback as simulated GPS
  CurrentRoadInfo.java        ← Road info data class
  RouteInstruction.java       ← Parsed route instruction for card rendering
  Config.java                 ← Config file read/write
  UIScale.java                ← DPI-aware scaling
  OverlayLayout.java          ← Shared overlay positioning logic
```

### Style & Quality
- Java: standard Java conventions (camelCase methods, PascalCase classes)
- C++ JNI: follows libosmscout C++ conventions
- FXML layout in `main.fxml` with CSS styling in `style.css`
- Tests: JUnit 5 (Jupiter)
- No mocking framework observed

### Error Handling
- JNI methods return `null` or empty arrays on error
- Callbacks report errors via `onError(String message)`
- Native exceptions caught and logged in C++ layer
- UI errors logged to stderr, never crash the app

## Architecture Overview

```
                    +---------------------------+
                    |  JavaScout (JavaFX App)   |
                    |  JavaScout.java           |
                    |  JavaScoutApp.java        |
                    |  MainController.java      |
                    +-----------+---------------+
                                |
                    +-----------v---------------+
                    |  libosmscout-client-java  |
                    |  OSMScoutClient.java      |
                    |  (JNI bridge)             |
                    +-----------+---------------+
                                |
                    +-----------v---------------+
                    |  libosmscout_client_java  |
                    |  OSMScoutClient.cpp       |
                    |  (C++ JNI implementation) |
                    +-----------+---------------+
                                |
          +---------------------+---------------------+
          |                     |                     |
          v                     v                     v
  +-------+--------+   +-------+--------+   +-------+--------+
  | libosmscout     |   | libosmscout-map |   | libosmscout-   |
  | Core DB/types   |   | + Cairo render  |   | client         |
  | Routing         |   |                 |   | (DBThread,     |
  | Location        |   |                 |   |  MapService)   |
  | GPX import      |   |                 |   |                |
  +-----------------+   +-----------------+   +-----------------+
```

### Data Flow

```
User Input (mouse/keyboard)
    │
    v
MapInteractionHandler / SearchOverlay / RoutePanel
    │
    v
MainController (orchestrator)
    │
    ├── MapRenderer.requestRender(lat, lon, mag, angle)
    │       │  (200ms debounce)
    │       v
    │   OSMScoutClient.render() / renderWithRouteAndPois()
    │       │  (JNI → C++ → Cairo → int[] ARGB)
    │       v
    │   Canvas PixelWriter.setPixels()
    │       │
    │       v
    │   drawCurrentLocationMarker() (JavaFX overlay)
    │
    ├── OSMScoutClient.searchLocations(query, limit)
    │       │  (JNI → LocationService::SearchForLocationByString)
    │       v
    │   SearchOverlay result list
    │
    ├── OSMScoutClient.calculateRouteAsync(start, dest, profile, callback)
    │       │  (JNI → MultiDBRoutingService::CalculateRoute)
    │       v
    │   RoutePanel shows route + MapRenderer renders route overlay
    │
    ├── OSMScoutClient.startNavigation(routeHandle, vehicle, listener)
    │       │  (JNI → NavigationEngine)
    │       v
    │   NavigationController.processLocation(lat, lon, speed, accuracy, ts)
    │       │  (JNI → NavigationEngine::ProcessLocation)
    │       v
    │   NavigationListener callbacks → UI updates
    │
    └── OSMScoutClient.importGpxTrack(filePath)
            │  (JNI → libosmscout-gpx)
            v
        TrackPlayer replays as simulated GPS
```

## Key Data Types

| Type | Package | Description |
|------|---------|-------------|
| `OSMScoutClient` | `client` | Central JNI client, created via builder |
| `NavigationController` | `client` | Live navigation session handle |
| `RouteEntry` | `client` | Route calculation result with geometry + metadata |
| `LocationEntry` | `client` | Location search result |
| `ObjectDescription` / `DescriptionEntry` | `client` | Structured object info |
| `TrackPoint` | `client` | GPX track point |
| `FavoriteLocation` / `FavoriteLocationGroup` | `client` | Favorites with group hierarchy |
| `RouteInstruction` | `client` | Turn-by-turn instruction with next-next hint |
| `NavigationPosition` | `client` | Estimated vehicle position with state |
| `MainController` | app | FXML controller, orchestrates all features |
| `MapRenderer` | app | Canvas rendering engine with 200ms debounce |
| `MapInteractionHandler` | app | Mouse/touch input: pan, zoom, long-press |
| `Config` | app | OS-specific config file (`config.properties`) |
| `UIScale` | app | DPI-aware scaling (touch targets ≥10mm) |

## Conventions for AI Agents

### Navigation
- **App source**: `JavaScout/src/main/java/com/framstag/libosmscout/` — start here for UI changes
- **Client API**: `../libosmscout-client-java/java/com/framstag/libosmscout/client` — Java API contracts
- **JNI bridge**: `../libosmscout-client-java/src/OSMScoutClient.cpp` — C++ implementation
- **Specs**: `../openspec/specs` — feature specifications in Gherkin format
- **Tests**: `JavaScout/src/test/java/` — JUnit 5 tests
- **Styles**: `JavaScout/src/main/resources/com/framstag/libosmscout/style.css`
- **Layout**: `JavaScout/src/main/resources/com/framstag/libosmscout/main.fxml`

### Common Patterns
- No DI framework — objects constructed manually in `MainController`
- Async operations use `javafx.concurrent.Task` on daemon threads
- UI updates marshalled via `Platform.runLater()`
- JNI methods return `null` or empty arrays on error
- Synthetic POI types (`_route_start`, `_route_end`, `_favorite`, `_search_selected`, `_track`) registered via `OSMScoutClientBuilder.withCustomPoiType()`
- Map rotation angle: radians, counter-clockwise (math convention), 0 = north-up
- Bearing: degrees, clockwise (navigation convention)

### Pitfalls
- Native library (`libosmscout_client_java`) must be built separately via Meson
- Tests requiring native lib need `-Dnative.lib.dir=...`
- JNI callback methods invoked from native thread — must marshal to JavaFX thread
- Cairo renderer requires PNG icons, not SVG
- Config file location is OS-specific (Linux: `~/.config/javascout/`, macOS: `~/Library/Application Support/JavaScout/`, Windows: `%APPDATA%\JavaScout\`)
- Legacy SVG icon path in config is auto-migrated to PNG path
- Reroute cooldown (15s) prevents cascading recalculations
- Road info lookup throttled (2s + 50m movement threshold)
- Auto-zoom suspended on manual zoom, re-engages on speed threshold boundary
- Follow mode suspended on manual pan, re-engaged via button
- Track player rebuilds timeline on speed change from next unplayed point
