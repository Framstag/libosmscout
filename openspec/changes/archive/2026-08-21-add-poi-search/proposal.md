## Why

JavaScout can search for locations by name, but cannot find points of interest (POIs) by category. Users who want to find nearby hotels, restaurants, or grocery stores must know the exact name of the place. A category-based POI search closes this gap: pick a category, define the search radius, and get a list of matching POIs around the current map center.

## What Changes

- Add a POI search entry point to the JavaScout main menu.
- Add a POI search dialog/overlay where the user can:
  - Choose a POI category from a fixed list (Hotels, Restaurants, Grocery store).
  - Define the size of the search area with a stepped slider.
- Add a POI search engine capability that finds POIs of the selected category within the chosen search area around the current map center.
- Hardcode the mapping from POI category to concrete OSM types in the first iteration (no user-configurable mapping yet).
- Display search results in a list similar to the existing location search result list.
- Show the details dialog for a POI when the user long-clicks a search result.

## Capabilities

### New Capabilities
- `poi-search-api`: Client API for type-based POI search — search POIs of given types within a radius around a coordinate and return structured results.
- `poi-search-ui`: JavaScout UI for POI search — menu entry, category selection, search area slider, result list, and long-click details dialog.

### Modified Capabilities
- `javascout-main-menu`: The main menu SHALL gain a POI search entry that opens the POI search UI.

## Impact

- `JavaScout/src/main/java/com/framstag/libosmscout/` — `MainController` (menu wiring, POI search orchestration), `SearchOverlay` or new POI search overlay (category picker, slider, result list), `DescriptionOverlay` (reused for details dialog), new POI category mapping class.
- `JavaScout/src/main/resources/com/framstag/libosmscout/` — `main.fxml` / `style.css` for new UI elements.
- `libosmscout-client-java/java/com/framstag/libosmscout/client/` — `OSMScoutClient` (new POI search method), new POI result data type.
- `libosmscout-client-java/src/OSMScoutClient.cpp` — JNI implementation of the POI search method.
- `libosmscout/src/osmscout/poi/POIService.*` — existing `GetPOIsInRadius` used as the search engine.
- `openspec/specs/` — new `poi-search-api` and `poi-search-ui` specs; delta for `javascout-main-menu`.
