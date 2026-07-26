## Context

JavaScout is a JavaFX demo app in `JavaScout/`. It renders maps via `libosmscout-client-java` JNI bridge (C++ → Java). Current map interaction is basic: drag-to-pan, scroll-to-zoom, keyboard navigation. No object inspection exists.

The change adds long-press → object lookup → structured description → overlay dialog. Spans three layers:

| Layer | Role |
|-------|------|
| `libosmscout` core | `DescriptionService` — takes resolved `Node`/`Way`/`Area`, returns `ObjectDescription` (list of `DescriptionEntry` with section/subsection/label/value) |
| `libosmscout-client-java` JNI | New native method `getDescription(lat, lon)` — resolves closest reasonable object, calls `DescriptionService`, marshals result to Java |
| `JavaScout/` JavaFX | Long-press detection in `MapInteractionHandler`, `DescriptionOverlay` dialog, wiring in `MainController` |

### Constraints

- No new external dependencies — `DescriptionService` already in core libosmscout
- JNI marshalling must be minimal and type-safe
- Dialog follows existing `SearchOverlay` pattern (StackPane overlay, fade animation, fullscreen on small screens)

## Goals / Non-Goals

**Goals:**

- Mouse long press on map shows description overlay for the most reasonable visible object
- Description content dynamically adapts to `ObjectDescription` structure (sections, subsections, label/value pairs)
- Dialog is fullscreen on small screens (<600px width), centered overlay on desktop
- Long descriptions are scrollable
- Long-press timeout is configurable via `Config` property
- JNI bridge exposes `ObjectDescription` as Java objects

**Non-Goals:**

- Touch long press (out of scope for this change)
- Object highlighting or selection state on the map
- Editing or modifying object data
- Localization of section/label keys (keys are English and can be used as-is or as i18n keys)
- Routing or navigation from the description dialog

## Decisions

### D1: Object selection heuristic

**Decision**: Query objects in a small bounding box (~50m radius) around the click coordinate using `Database` spatial indexes (`LoadNodesInRadius`, `LoadWaysInRadius`, `LoadAreasInRadius`). Rank candidates by: (1) has `DescriptionService` data, (2) way/node within 5m of click (very close), (3) small area (<10,000 sq m) contains the click point, (4) type rank (area > way > node), (5) proximity. Areas larger than 10,000 sq m (administrative boundaries, landuse) do not get the "contains" bonus — they would otherwise always win over nearby ways/nodes.

**Rationale**: Pure closest-by-distance often picks uninteresting objects (e.g., a tiny node on a building wall). The heuristic matches user intent — they long-pressed something they see and want info about. The "very close" check ensures clicking on a road gets the road, not a containing landuse area. The size threshold prevents huge boundaries from beating nearby objects.

**Alternatives considered**:
- *Closest by Haversine only* — too naive, often picks road surface nodes or unnamed ways
- *Let user tap to select* — requires object picking hit-test, more complex, changes interaction model
- *Show all objects in area* — overwhelming on dense maps
- *Prefer nodes over areas* — caused trees/traffic lights to beat buildings the user clicked on

Zoom-level visibility is not checked because the JNI layer lacks access to `StyleConfig`/`MapService`. The "very close" (5m) heuristic handles the common case (clicking a road vs containing landuse). If zoom filtering becomes necessary, add a `mag` parameter to `getDescription()` and filter candidates by type visibility per zoom level.

### D2: JNI data marshalling

**Decision**: Add a single native method `ObjectDescription getDescription(double lat, double lon)` to `OSMScoutClient`. The JNI side creates Java `ObjectDescription` and `DescriptionEntry` objects, populates them from the C++ structs, and returns the result.

**Rationale**: One round-trip per long press. The `ObjectDescription` structure is simple (flat list of entries) so marshalling is straightforward. No need for callback-based async since `DBThread::RunSynchronousJob` blocks the calling thread briefly.

**Alternatives considered**:
- *Return JSON string* — avoids JNI object creation but requires JSON parsing on Java side, adds dependency
- *Multiple native calls* (get objects, then get description) — two round-trips, more complex error handling

### D3: Long-press detection in MapInteractionHandler

**Decision**: Add a `javafx.animation.PauseTransition` timer in `MapInteractionHandler`. `MOUSE_PRESSED` starts the timer. `MOUSE_RELEASED` or `MOUSE_DRAGGED` cancels it. On timer fire, invoke a callback with `(lat, lon)`. The timeout value is read from `Config` (default 500ms).

**Rationale**: `PauseTransition` runs on the JavaFX UI thread, no threading issues. Timer approach is simple and matches the existing debounce pattern in `SearchOverlay`. Configurable via `Config` so users can adjust.

**Alternatives considered**:
- *`ScheduledService`* — overkill for a simple timer
- *`Thread.sleep` in a background thread* — requires Platform.runLater, more complex

### D4: DescriptionOverlay dialog

**Decision**: New `DescriptionOverlay extends StackPane` following the `SearchOverlay` pattern. Contains a `VBox` inside a `ScrollPane` for the description content. Sections rendered as bold headers, subsections as indented sub-headers, label/value pairs as rows. Close button in title bar. Click outside or Escape to close. Fullscreen when window width < 600px.

**Rationale**: Reuses established patterns (`SearchOverlay` structure, `SMALL_SCREEN_THRESHOLD`). `ScrollPane` handles long content. Dynamic rendering from `ObjectDescription` entries means no hardcoded layout per section type.

### D5: DescriptionService lifecycle

**Decision**: Create a single `DescriptionService` instance in the JNI layer's `ClientData` struct, initialized when the database opens. Reuse it for all description lookups.

**Rationale**: `DescriptionService` is stateless (no mutable state between calls). Creating one per lookup is wasteful. Storing in `ClientData` follows the same pattern as other services.

## Sequence

```
User                    MapInteractionHandler        MainController         OSMScoutClient(JNI)     Database / DescriptionService
 |                              |                         |                        |                        |
 |-- mouse press --------------->|                        |                        |                        |
 |                              |-- start timer (500ms)   |                        |                        |
 |                              |                         |                        |                        |
 |-- mouse release / drag ----->|                        |                        |                        |
 |                              |-- cancel timer          |                        |                        |
 |                              |                         |                        |                        |
 |                              | [timer fires]           |                        |                        |
 |                              |-- onLongPress(lat,lon)->|                        |                        |
 |                              |                         |-- getDescription() ---->|                        |
 |                              |                         |   (JNI call)           |                        |
 |                              |                         |                        |-- RunSynchronousJob --->|
 |                              |                         |                        |   (DB read lock)       |
 |                              |                         |                        |                        |-- query bbox
 |                              |                         |                        |                        |-- rank candidates
 |                              |                         |                        |                        |-- DescriptionService
 |                              |                         |                        |<-- ObjectDescription ---|
 |                              |                         |<-- ObjectDescription --|                        |
 |                              |                         |                        |                        |
 |                              |                         |-- show overlay ------->|                        |
 |                              |                         |   (DescriptionOverlay) |                        |
 |<-- rendered dialog ----------|                        |                        |                        |
```

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| JNI call blocks UI thread during DB lookup | `getDescription()` runs `DBThread::RunSynchronousJob` which acquires a read lock — fast (milliseconds). If latency becomes an issue, switch to async pattern with callback |
| `DescriptionService` returns empty description for some objects | Heuristic already filters for objects with data. Dialog shows "No description available" for empty results |
| Large `ObjectDescription` with many entries | `ScrollPane` handles overflow. Dialog has max-height constraint |
| Config property `longPressTimeoutMs` not persisted | Use existing `Config` class which reads/writes `config.properties` |
| JNI method signature changes break binary compat | JavaScout is a demo app, not a library with ABI guarantees. Rebuild on changes |
