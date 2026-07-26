## Context

JavaScout renders the map by calling the JNI `OSMScoutClient.renderWithRoute()` method, which runs `MapPainterCairo` on a pixel buffer and returns ARGB data. The C++ render path already has a hook for overlay data: it builds an `osmscout::Way` for the route and tries to add `_route_start` / `_route_end` nodes to `MapData::poiNodes`. Those nodes never appear because the types are neither registered in the type config nor styled in the stylesheet.

The Qt client solves the same problem with `OSMScoutQtBuilder::AddCustomPoiType()`. The underlying `DBThread` registers those types in every database's `TypeConfig` and builds an empty `StyleConfig` that also knows them. The Java client currently passes an empty `customPoiTypes` vector to `DBThread`.

## Goals / Non-Goals

**Goals:**
- Register synthetic POI types `_route_start`, `_route_end`, `_favorite`, `_search_selected` at runtime.
- Render all loaded favorite locations as map markers.
- Render a marker for the currently selected search result (0 or 1).
- Use the existing `poiNodes` rendering path; no JavaFX overlay.
- Keep markers zoom-level aware via stylesheet rules.

**Non-Goals:**
- No labels on markers.
- No per-group icons or colors.
- No clustering.
- No markers for non-selected search results.
- No changes to imported `.osmscout` databases.

## Decisions

### 1. Register custom POI types in the Java builder

**Decision**: Add `withCustomPoiType(String)` to `OSMScoutClientBuilder`. The default builder used by JavaScout registers `_route_start`, `_route_end`, `_favorite`, and `_search_selected`.

**Rationale**: The C++ `DBThread::registerCustomPoiTypes()` already exists and does exactly what we need. We only need to expose it through the Java builder and JNI build path.

### 2. Extend `renderWithRoute` rather than add a separate render method

**Decision**: Change the native signature from `renderWithRoute(width, height, lat, lon, mag, routeLats, routeLons)` to include optional favorite lat/lon arrays and a single selected-search lat/lon pair. Add a Java overload so existing callers still compile.

**Rationale**: One render call means the markers, route, and base map are always in sync. Avoids flicker from separate layers. Keeps the debounce loop simple.

### 3. Use `Double.NaN` to mean "no selected search result"

**Decision**: On the Java side, pass `Double.NaN` for latitude when no search result is selected. The JNI side checks `!isnan(lat)` before creating a node.

**Rationale**: Java has no optional primitive doubles; `NaN` is the cleanest sentinel and `std::isnan()` is available in C++.

### 4. Favorite markers = all loaded favorites

**Decision**: JavaScout reads `client.getFavoriteGroups()` and flattens every favorite into parallel lat/lon arrays passed to the renderer on every render.

**Rationale**: Simple. The rendering engine will filter by visibility. Favorites are usually few enough that rebuilding the arrays is cheap.

### 5. Stylesheet rules in existing included files

**Decision**: Add style rules for `_route_start`, `_route_end`, `_favorite`, and `_search_selected` to `stylesheets/include/route.oss` (or another shared include). All stylesheets that include it will pick them up. The types are not added to `map.ost` because they are synthetic runtime types.

**Rationale**: `.oss` files support includes via `MODULE`. The `_route` way style is already in `include/route.oss`, so node marker styles belong there too. This keeps changes minimal and consistent across all stylesheets that include the file.

## Risks / Trade-offs

- **No labels**: Users cannot tell which favorite is which on the map. Acceptable for first version; labels can be added later via `NameFeature` on the synthetic nodes.
- **Zoom-level visibility**: If the stylesheet does not filter markers by `MAG`, they may crowd the map at low zoom. We will add `[MAG city-]` or tighter rules.
- **Performance**: Favorite count is usually small (< 1000). Passing arrays across JNI on every render adds a small copy. Acceptable.
- **Type registration timing**: Custom types are registered in `DBThread` constructor and again when databases change. If a database is opened after the style is loaded, it may briefly miss the types; `DBThread::OnDatabaseListChanged` already re-registers, so this is handled.
