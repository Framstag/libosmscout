## What Changes

Add POI markers to JavaScout for:
1. All loaded favorite locations
2. The currently selected search result (zero or one)

Markers are rendered through the existing Cairo map rendering pipeline by injecting synthetic nodes into `MapData::poiNodes`, exactly like the existing route start/end marker attempt. The synthetic types are registered as custom POI types in the `DBThread` type configuration so they can be styled via the standard `.oss` stylesheet files without modifying the imported `.osmscout` database.

## Capabilities

### New Capabilities

- `favorite-markers`: Render all loaded favorites as markers on the map. Visibility is controlled by the stylesheet (zoom-level filtering) — the application only needs to provide the coordinates.
- `search-selection-marker`: Render a marker for the currently selected search result. The marker is previewed while a result is highlighted in the search list and persists at the navigated-to result after the overlay closes. It is cleared when a new search query is entered.
- `custom-poi-types`: Register synthetic POI types at runtime (`_route_start`, `_route_end`, `_favorite`, `_search_selected`) so the style engine can render them.

### Modified Capabilities

- `route-visualization`: The existing `_route_start` and `_route_end` markers currently never render because the types are not registered and the stylesheet does not style them. This change fixes that as part of registering custom POI types.

## Impact

- **libosmscout-client-java/java/**: Extend `OSMScoutClientBuilder` with custom POI type registration. Extend `OSMScoutClient` rendering API to accept favorite coordinates and a selected search coordinate.
- **libosmscout-client-java/src/**: Update C++ `OSMScoutClient.cpp` to pass custom POI types to `DBThread` and to create `poiNodes` for favorites and search selection during render.
- **JavaScout/**: Update `MapRenderer` to hold marker state, update `MainController` to feed favorites/search selection into the renderer, and add a selection callback to `SearchOverlay`.
- **stylesheets/include/route.oss** and relevant stylesheets: Add styling rules for `_route_start`, `_route_end`, `_favorite`, and `_search_selected` types so markers actually appear. Existing stylesheets already `GROUP _route` and the `.ost` files already define `_route`; the synthetic node types only need style rules, not new database type definitions.
