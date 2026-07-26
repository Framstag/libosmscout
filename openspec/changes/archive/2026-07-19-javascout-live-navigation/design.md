# JavaScout live navigation design

## Context

JavaScout is a JavaFX demo built on `libosmscout-client-java` JNI. It already supports database rendering, search, favorites, route calculation, and GPX track import. The libosmscout core provides a `NavigationEngine` with agents (`PositionAgent`, `RouteStateAgent`, `BearingAgent`, `ArrivalEstimateAgent`, `SpeedAgent`, `LaneAgent`, `VoiceInstructionAgent`) used by the Qt client (`NavigationModule`). JavaScout currently has no bridge to this engine and only renders static route geometry.

## Goals / Non-Goals

**Goals:**
- Expose libosmscout `NavigationEngine` to Java through `libosmscout-client-java`.
- Allow JavaScout to replay an imported GPX track as simulated GPS fixes.
- Report snapped position, off-route, reroute request, target reached, arrival, speed limits, lanes, and voice samples to Java listeners.
- Provide map auto-center / follow-heading mode and current-location marker rendering.

**Non-Goals:**
- Real device GPS provider integration (serial, NMEA, Android location) is out of scope; only the Java API contract and a simulated track driver are required.
- Voice playback implementation in JavaScout is not required; only sample identifiers are forwarded.
- Reroute calculation itself is out of scope; only the request callback is emitted so the app can call `calculateRouteAsync` again.

## Decisions

### 1. Keep RouteDescriptionRef in C++ and bind a NavigationController per route

**Decision:** When `calculateRouteWithObjectsAsync` generates the `RouteDescription`, store it in `ClientData` keyed by a new opaque navigation handle. Java creates a `NavigationController` for that route via `client.startNavigation(routeHandle, listener)`.

**Rationale:** The `NavigationEngine` needs the full `RouteDescriptionRef`, not just point geometry. Re-using the same C++ object avoids re-running the postprocessor or serializing the description across JNI. Qt `NavigationModule` does the same by holding `routeDescription`.

**Alternatives considered:**
- Serialize `RouteDescription` to Java and back: heavy, lossy, many C++ types are not JNI-friendly.
- Recompute description from `RouteEntry` points: loses turn-by-turn descriptions and routing metadata.

### 2. Run the NavigationEngine on a dedicated C++ background thread with a Java callback global ref

**Decision:** `NavigationController` JNI keeps a global ref to the Java listener and a dedicated C++ thread that pumps `GPSUpdateMessage` + `TimeTickMessage` through the engine at the requested interval.

**Rationale:** Engine work (route snapping, data loading via `DataAgent`) can block on database IO. Offloading from the JavaFX thread matches the existing route-calculation pattern.

**Alternatives considered:**
- Run engine on Java thread calling synchronously into JNI: simpler but would block JavaFX during `DataAgent` loads.
- Run each message in `RunSynchronousJob` on `DBThread`: could work but harder to sequence ticks and cancellation.

### 3. Send GPS updates from Java, not from C++

**Decision:** Java owns the clock for simulated playback. It calls `navigationController.processLocation(lat, lon, speed, accuracy, timestamp)` whenever it wants; C++ immediately processes the message and returns nothing. C++ schedules `TimeTickMessage` itself on the background thread every second.

**Rationale:** Track playback logic (speed multiplier, pause/resume) belongs in Java. Real GPS mode will also originate in Java. C++ should not own wall-clock timing for fixes because it cannot pause/resume a simulation cleanly.

### 4. Map follow mode lives in JavaFX controller, not renderer

**Decision:** `NavigationController` reports position estimates to a JavaFX component (`NavigationView` or `MainController`). That component decides whether to call `renderer.requestRenderPreserveRoute(lat, lon, mag)` with the estimated position. Manual pan disables follow until re-enabled.

**Rationale:** Follow policy is UI interaction logic. Keeping it out of `MapRenderer` lets the renderer stay a pure rendering widget.

### 5. Current-location marker rendered as a JavaFX overlay on top of the Canvas

**Decision:** The native renderer returns only the base map image. After blitting the pixels to the JavaFX Canvas, JavaScout paints the current-location marker directly on the Canvas graphics context. JavaScout requests the screen pixel coordinate from a native `projectToPixel()` helper that uses the same `MercatorProjection` as the map renderer. The marker is a semi-transparent coloured dot with a white border and an optional bearing arrow.

**Rationale:** This gives the Java layer full control over the marker style (arrow, GPS-quality colour coding, halo, animation) without touching OSS or the Cairo painter. It also guarantees the marker is visually on top of every map element.

**Alternatives considered:**
- Use a synthetic `_current_location` NODE styled in OSS: limited to simple circles/icons; arrow and dynamic colours require complex OSS workarounds.
- Draw marker in native Cairo after `DrawMap()`: works, but every visual tweak requires a C++ rebuild and re-negotiating JNI data.

## Risks / Trade-offs

- **Risk:** `RouteDescriptionRef` lifetime and thread safety. The route calculation thread may still own it while navigation starts.  
  **Mitigation:** Store it in `ClientData` under a mutex and only hand it to the navigation thread after `calculateRouteWithObjectsAsync` completes.

- **Risk:** `NavigationEngine` requires a `DataAgent` that loads routable objects from the database. Current `OSMScoutClient` hides `DBThread` details.  
  **Mitigation:** Implement `DataAgent<NavigationClient>` in C++ that delegates to `ClientData->dbThread->RunSynchronousJob` like the Qt `NavigationModule` does.

- **Risk:** Map rotation for heading-follow is not supported by `MapRenderer` or native `MercatorProjection`.  
  **Mitigation:** First milestone keeps map upright and only re-centers; heading-follow is a later enhancement using rotated Canvas or adjusted projection. Marked as non-goal for initial implementation.

- **Risk:** JNI callback method lookup and thread attachment overhead per message.  
  **Mitigation:** Cache `jmethodID`s in `NavigationListenerMethods` struct and attach the C++ thread to the JVM once.

- **Risk:** Adding `_current_location` type to styles breaks other renderers if not defined there.  
  **Mitigation:** Add the type defensively (if absent the renderer simply does not draw it), and update `standard.oss` / `cycle.oss` / `winter.oss` only if they share the same `map.ost`.

## Migration Plan

- No database or API migration required.
- JavaScout users will see new playback controls when a track is imported.
- Existing `RouteEntry` API remains unchanged; navigation is opt-in via new `client.startNavigation(...)`.

## Open Questions

1. Should navigation be limited to one active session at a time? (Recommended: yes, like Qt.)
2. Which OSS file defines `_current_location`? Need to check whether all JavaScout styles share one `map.ost`.
3. Does JavaScout need `VoiceInstructionAgent` in the first milestone, or only agents required for position/speed/lane?
4. Is there a JavaFX `Timeline` / `AnimationTimer` preference for track playback?
