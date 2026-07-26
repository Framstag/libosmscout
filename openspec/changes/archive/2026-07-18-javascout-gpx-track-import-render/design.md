# Design: JavaScout GPX Track Import and Render

## Context

JavaScout renders the map through a Cairo-based JNI bridge. Route geometry, favorite markers, and search-selection markers are already passed from Java into the native renderer as parallel coordinate arrays and rendered as synthetic ways/nodes with custom types (`_route`, `_favorite`, `_search_selected`). The libosmscout-gpx library exists and provides `osmscout::gpx::ImportGpx` to read GPX files into `GpxFile` / `Track` / `TrackSegment` / `TrackPoint` structures. The `PositionSimulator` Qt demo already shows how to replay a GPX track, but JavaScout has no equivalent path.

## Goals / Non-Goals

**Goals:**
- Import a GPX track from the filesystem into JavaScout.
- Render the imported track as a visible polyline on the Cairo map.
- Reuse existing custom-POI and route-overlay rendering patterns.

**Non-Goals:**
- Real-time replay or position simulation (future step).
- Turn-by-turn navigation from the track (future step).
- Editing, exporting, or persisting imported tracks.
- Multi-track GPX files are out of scope for the UI; the first track is used.

## Decisions

### 1. JNI import method returns a flat array of track points

Java `OSMScoutClient` gains `TrackPoint[] importGpxTrack(String filePath)`. The C++ side calls `osmscout::gpx::ImportGpx`, walks `gpxFile.tracks[0].segments[*].points`, and converts each `osmscout::gpx::TrackPoint` into a Java `TrackPoint` object containing `lat`, `lon`, and optional `timestamp`.

**Rationale:** Keeps the JNI surface small. Java owns the points and can feed them to the renderer as needed. The first track of the file is used; multi-track handling can be added later without breaking the API.

**Alternative considered:** Streaming points directly into C++ renderer state. Rejected because it couples import to rendering and makes testing/inspection harder on the Java side.

### 2. Track rendering uses the same path as route overlay

The renderer state in `MapRenderer` stores `trackLats`/`trackLons`. These are passed to a new JNI render method (or an extended existing method) that creates a synthetic `osmscout::Way` with type `_track` and adds it to `MapData::poiWays`, just like the existing `_route` way.

**Rationale:** Minimal changes to the native render pipeline; proven pattern from `route-visualization` spec. The track simply coexists with route/favorite/search markers.

### 3. `_track` is a synthetic POI type registered at runtime

`MainController` already registers `_favorite`, `_route_start`, `_route_end`, and `_search_selected` via `OSMScoutClientBuilder.withCustomPoiType`. `_track` is added to that list.

**Rationale:** The custom-POI mechanism was built exactly for this use case. No database re-import is required.


### 4. Top-left menu replaces standalone favorites button and hosts import

A new hamburger-style menu button is placed in the top-left corner of the map panel, replacing the existing favorites star button. The menu contains:
- **Favorites** — opens the existing `FavLocationDialog`
- **Import GPX Track…** — opens a JavaFX `FileChooser` filtered to `.gpx`

After file selection, `OSMScoutClient.importGpxTrack` is called and the returned points are forwarded to `MapRenderer.setTrackPoints`, which triggers a re-render.

**Rationale:** Groups infrequently used map actions in one discoverable location, frees overlay button real estate, and matches mobile/desktop map app conventions.

### 5. Build dependency on libosmscout-gpx

`libosmscout-client-java` and `JavaScout` must link against `osmscout_gpx` and include libxml2. CMake already has `OSMSCOUT_BUILD_GPX` option. The JNI C++ source will include `<osmscoutgpx/Import.h>` and `<osmscoutgpx/GpxFile.h>`.

**Rationale:** Required to call `ImportGpx`. Meson equivalent will need the same dependency.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| libxml2 not available on some build configs (e.g. minimal CI) | Guard with the existing `OSMSCOUT_BUILD_GPX` CMake option; if GPX is off, the import feature is disabled at compile time with a clear `#ifdef`. |
| Large GPX files cause JNI array allocation overhead | Accept for this first step; future work can chunk or filter points using `FilterNearPoints`. |
| Multi-segment tracks create disjoint polylines | Render all segments of the first track as a single merged `_track` way; gaps will be connected visually. Acceptable for a first step. |
| `_track` type must be styled or it is invisible | Add fallback default style; UI should warn if no style is found only after implementation verification. |

## Migration Plan

No production migration. Local build rebuild is sufficient. After the change, developers must ensure `OSMSCOUT_BUILD_GPX=ON` (default) when building JavaScout.

## Open Questions

- Should the imported track replace any previously imported track, or accumulate? Decision: replace for now (single active track).
- Should the map viewport pan/zoom to fit the track bounds? Desirable but optional; can be added in a follow-up.
- Which JavaFX control hosts the import button? Decided: a new top-left hamburger menu button replaces the standalone favorites button. Favorites management and GPX track import move into this menu.

## Sequence: Import and render a GPX track

```
User
 │ opens top-left menu and selects "Import GPX Track…"
 ▼
MainController ──▶ FileChooser.showOpenDialog()
 │
 │ path selected
 ▼
MainController ──▶ OSMScoutClient.importGpxTrack(path)
 │
 ▼
JNI (C++) ──▶ osmscout::gpx::ImportGpx(path, gpxFile)
 │
 ▼
JNI (C++) ──▶ allocate TrackPoint[] from gpxFile.tracks[0]
 │
 ▼
MainController ──▶ MapRenderer.setTrackPoints(points)
 │
 ▼
MapRenderer ──▶ requestRenderPreserveRoute(current center)
 │
 ▼
JNI render ──▶ build Way(type=_track) from trackLats/trackLons
 │
 ▼
Cairo MapPainter ──▶ draws track polyline
```
