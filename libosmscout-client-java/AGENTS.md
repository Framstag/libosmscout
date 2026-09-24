# AGENTS.md — libosmscout-client-java

## Project Identity

- **What**: Java client library for libosmscout — JNI bridge exposing OSM map rendering, routing, navigation, location search, favorites, and GPX track import to Java
- **Parent**: [libosmscout](http://libosmscout.sourceforge.net/) — C++20 library for offline OSM data
- **Version**: 1.0-SNAPSHOT
- **GroupId**: `net.sf.libosmscout`
- **ArtifactId**: `libosmscout-client-java`
- **Package**: `com.framstag.libosmscout.client`
- **Homepage**: http://libosmscout.sourceforge.net/
- **Support**: Matrix `#libosmscout.matrix.org`, mailing list on SourceForge

## Repository Map

| Path | Purpose |
|------|---------|
| `java/com/framstag/libosmscout/client/` | **Java API** — 19 source files: client, builder, data types, callbacks, enums |
| `src/OSMScoutClient.cpp` | **C++ JNI implementation** — 4172 lines, single translation unit |
| `src/meson.build` | Meson build for native shared library |
| `java/meson.build` | Meson build for Java JAR + JNI header generation |
| `meson.build` | Root Meson build file (delegates to java/ and src/) |
| `CMakeLists.txt` | CMake build for the native library, the Java classes and the JAR |
| `include/osmscoutclientjava/ClientJavaImportExport.h` | DLL export/import macros for Windows/Unix |

## Build Systems

### CMake (root project)

The native library and Java JAR are built from the repository root when JNI and a JDK are found:

```bash
cmake -B build -DOSMSCOUT_BUILD_CLIENT_JAVA=ON
cmake --build build --target osmscout_client_java java_jar
```

`OSMSCOUT_BUILD_CLIENT_JAVA` defaults to ON when `JNI_FOUND` and `Java_JAVAC_EXECUTABLE`; CMake fails with a clear message when the option is on but JNI is missing. The JAR is written to `build/libosmscout-client-java/libosmscoutclientjava.jar`.

### Meson

```bash
meson setup build-meson -DOSMSCOUT_BUILD_CLIENT_JAVA=ON
meson compile -C build-meson
```

`buildJava` is set to `true` when Java (JNI) dependencies are found; the JAR is written to `build-meson/libosmscout-client-java/java/libosmscoutclientjava.jar`.

### What gets built

| Artifact | Description |
|----------|-------------|
| `libosmscout_client_java.so`/`.dylib`/`.dll` | Native C++ shared library |
| `libosmscoutclientjava.jar` | Java JAR with all client classes |

### Maven (Java JAR only, standalone)

```bash
cd libosmscout-client-java
mvn package
```

Native `.so`/`.dylib`/`.dll` must be built separately via CMake or Meson and placed on `java.library.path` at runtime.

## Dependencies

### Native (C++) — linked at build time

| Library | Role |
|---------|------|
| `libosmscout` | Core DB, types, routing, location, geometry |
| `libosmscout-map` | Abstract map rendering layer |
| `libosmscout-map-cairo` | Cairo renderer (BGRx → ARGB conversion) |
| `libosmscout-client` | DBThread, MapService, MapManager |
| `libosmscout-gpx` | GPX file import (optional, gated by `buildGpx`) |
| Cairo / Pango / PangoCairo | Rendering backend |
| JNI (JDK headers) | Java Native Interface |

### Java — compile scope

None. Pure JNI, no Java framework dependencies.

## Code Conventions

### Language & Standards
- **Java 17+** — no frameworks, pure JNI
- **C++20** — JNI implementation in `OSMScoutClient.cpp`
- Namespace: `osmscout` for C++ code
- Package: `com.framstag.libosmscout.client` for all Java classes
- No DI framework — objects constructed manually via `OSMScoutClientBuilder`

### JNI Method Naming
All native methods follow the standard JNI convention:
```
Java_com_framstag_libosmscout_client_<ClassName>_<methodName>
```

### Error Handling
- Native methods return `null` or empty arrays on error
- Callbacks report errors via `onError(String message)`
- C++ exceptions caught and logged, never propagated to Java as crashes
- `nativeHandle` (long) stores opaque pointer to C++ `ClientData` object
- Null/invalid `nativeHandle` checked at top of each JNI method

### Memory Management
- C++ `ClientData` allocated in `build()` JNI call, freed in `close()`
- `JavaNavigationController` allocated in `startNavigation()`, freed in `stop()`
- JNI global references for callback objects released on session end
- All intermediate C++ objects released before returning from JNI calls

## Architecture Overview

```
Java Application (e.g. JavaScout)
         │
         ▼
┌─────────────────────────────────┐
│  libosmscout-client-java (JAR)  │
│  OSMScoutClient.java            │
│  NavigationController.java      │
│  Data types / callbacks / enums │
└──────────┬──────────────────────┘
           │ JNI
           ▼
┌──────────────────────────────────┐
│  libosmscout_client_java (.so)   │
│  OSMScoutClient.cpp              │
│    ClientData (Settings,         │
│      MapManager, DBThread,       │
│      DatabaseRef, TypeConfig,    │
│      StyleConfig)                │
│    JavaNavigationController      │
│      (NavigationEngine,         │
│       Position/Speed/Data/       │
│       LaneAgent)                 │
└──────────┬───────────────────────┘
           │
     ┌─────┼─────┬─────┬─────┐
     ▼     ▼     ▼     ▼     ▼
  libosmscout  libosmscout-map  libosmscout-client  libosmscout-gpx  Cairo
```

### JNI Call Flow

```
Java call                    C++ JNI bridge              C++ libosmscout

openDatabase(path)    →     MapManager::OpenDatabase()
                            DBThread::LoadData()

render(w,h,lat,lon,  →     MercatorProjection
  angle,mag)               MapPainterCairo::DrawMap()
                            BGRx → ARGB int[]

searchLocations(      →     LocationService::
  query,limit)              SearchForLocationByString()

calculateRouteAsync(  →     MultiDBRoutingService
  start,dest,profile)       GetClosestRoutableNode()
                            CalculateRoute()
                            TransformRouteDataToPoints()

startNavigation(      →     JavaNavigationController
  routeHandle,              (background thread)
  listener)                 NavigationEngine
                            Position/Speed/Data/LaneAgent

processLocation(       →     NavigationEngine::
  lat,lon,speed,            ProcessLocation()
  accuracy,ts)         ←     DispatchMessage()
                             → NavigationListener callbacks

importGpxTrack(       →     gpx::ImportGpx()
  filePath)                 Return first track points

getDescription(       →     DescriptionService
  lat,lon)                  Query objects in bbox
                            Return best match
```

## Key Data Types

| Type | Language | Description |
|------|----------|-------------|
| `OSMScoutClient` | Java | Main client. 47 native methods: render, search, route, navigate, favorites, GPX, description, projection, style switching, basemap configuration. |
| `OSMScoutClientBuilder` | Java | Fluent builder. Icon dir, map dirs, DPI, units, stylesheets, custom POI types, basemap directory, basemap stylesheet. |
| `NavigationController` | Java | Live navigation session handle. `processLocation()`, `stop()`. |
| `NavigationListener` | Java | 11 default-method callbacks for navigation events. |
| `RouteEntry` | Java | Route result: geometry arrays, distance, duration, descriptions, routeHandle. |
| `RouteCallback` | Java | 4 callbacks: `onProgress`, `onSuccess`, `onError`, `onCancel`. |
| `RouteInstruction` | Java | Turn-by-turn: distanceTo (distance from the route start in the instruction list, remaining distance for the next instruction), turnType, streetName, description, nextNext*. |
| `LocationEntry` | Java | Search result: label, type, objectType, lat, lon, region, objectFileOffset. |
| `ObjectDescription` / `DescriptionEntry` | Java | Structured object info from `getDescription()`. |
| `TrackPoint` | Java | GPX track point: lat, lon, timestamp. |
| `FavoriteLocation` / `FavoriteLocationGroup` | Java | Favorites with group hierarchy and extensible attributes. |
| `ClientData` | C++ struct | Holds all native state: Settings, MapManager, DBThread, DatabaseRef, TypeConfigRef, StyleConfigRef. |
| `JavaNavigationController` | C++ class | Background thread running NavigationEngine, dispatching messages to Java. |

## Conventions for AI Agents

### Navigation
- **Java API**: `java/com/framstag/libosmscout/client/` — start here for API changes
- **JNI bridge**: `src/OSMScoutClient.cpp` — single file, 4172 lines
- **Build**: `src/meson.build` for native lib, `java/meson.build` for JAR + JNI headers
- **Export macros**: `include/osmscoutclientjava/ClientJavaImportExport.h`

### Common Patterns
- `nativeHandle` (long) stores opaque C++ pointer — checked for null at top of every JNI method
- `ClientData` accessed via `getClientData(env, obj)` helper
- JNI callbacks use cached method IDs from `NavigationListenerMethods` / `RouteCallbackMethods` structs
- Synthetic POI types (`_route_start`, `_route_end`, `_favorite`, `_search_selected`, `_track`) registered at build time via `withCustomPoiType()`
- Basemap configuration: `withBasemapLookupDirectory(dir)` and `withBasemapStyleSheet(name)` on the builder set the basemap database and the stylesheet it renders with (a plain stylesheet name, with or without `.oss`, resolved against the stylesheets directory; an unusable name leaves the basemap on the active map style). `setBasemapLookupDirectory(dir)` changes the directory on a running client and reloads the basemap asynchronously — an empty value unloads it, and the outcome is observable through the rendered map and `wasLastStyleLoadSuccessful()`.
- Route calculation supports object references from search results for precise node resolution
- Instruction distances: the remaining distance of the next route instruction is derived from the position's progress along its current route segment (the fraction of `routeNode -> routeNode+1` the snapped fix lies at). `RouteInstructionAgent` offers that progress to the instruction builder when the builder accepts it and keeps calling the coordinate-based form otherwise; the bridge falls back to the straight-line distance from the segment start when a position reports no progress, and never reports a negative distance. The arrival instruction carries the distance of the destination node from the route start, like every other instruction of the list.
- Navigation engine runs on dedicated background thread, messages dispatched to Java via `CallVoidMethod`
- GPX import gated by `buildGpx` — returns empty array when disabled
- Render pipeline: Cairo BGRx surface → manual conversion to ARGB `int[]` → Java

### Pitfalls
- **Two build systems** — the native library and JAR are built by CMake (root project, `OSMSCOUT_BUILD_CLIENT_JAVA`) or by Meson (`buildJava`). JavaScout consumes the JAR through Maven, so a freshly built JAR has to be installed into the local Maven repository (`mvn install:install-file -Dfile=<build>/libosmscout-client-java/libosmscoutclientjava.jar ...`); a stale JAR there makes the native and Java sides disagree.
- **Single translation unit** — all JNI code in one 4172-line `.cpp` file. Be careful with merge conflicts.
- **JNI callback thread safety** — callbacks invoked from native thread; Java consumer must marshal to UI thread
- **JNI global references** — must be released to avoid leaks; `JavaNavigationController` destructor handles cleanup
- **Cairo pixel format** — Cairo uses BGRx, manually converted to ARGB for Java. Alpha channel is always 0xFF.
- **Route handle** — `RouteEntry.routeHandle` is opaque; valid only until next `calculateRouteAsync()` call
- **NavigationController** — single-use; call `stop()` to release, create new via `startNavigation()` for next session
- **GPX build gate** — `importGpxTrack` returns empty array when `OSMSCOUT_BUILD_GPX=OFF`; no compile error
- **Favorite persistence** — CRUD operations modify in-memory state; `saveFavoriteLocations()` must be called explicitly
- **JNI header generation** — Meson's `javaModule.native_headers()` generates headers at build time; header paths must match Java package structure
