## Why

JavaScout demo UI currently breaks on window resize, uses fixed-size overlays that do not adapt to display DPI or touch targets, and has inconsistent dialog sizing. Route result is also not reliably drawn on the map. These issues make the demo feel unfinished on both desktop and touch devices. Clean them up so the JavaFX shell behaves like a responsive map application.

## What Changes

- Make window resize fully functional: map canvas resizes and redraws to new dimensions, keeping the same geographic center.
- Make overlay controls (search and route buttons) adapt to display DPI and use touch-friendly spacing/sizing based on "thumb on display" size.
- Reduce excess spacing between the two overlay buttons.
- Unify dialog sizing: route panel and search panel use the same responsive sizing strategy.
- Keep route search location dialog pre-filled from the current data source (start or end point of route) when opening from route input.
- Ensure the calculated route is rendered on the map via the existing `renderWithRoute` JNI path.

## Capabilities

### New Capabilities
- `javascout-responsive-shell`: Window resize handling, DPI-aware overlay sizing, and responsive layout rules for the JavaScout JavaFX shell.
- `javascout-route-dialog-data`: Route location search dialog pre-fills from the active route data source (start/destination).

### Modified Capabilities
- `javafx-ui-shell`: Requirements updated to cover responsive resize, DPI-aware overlay button sizing, and consistent dialog dimensions.
- `route-input`: Requirements updated so the search overlay opened for route location selection starts with the current start/destination value.
- `route-visualization`: Requirements updated to guarantee the route is rendered after successful calculation and re-rendered on view changes.

## Impact

- JavaScout JavaFX demo in `JavaScout/`.
- FXML layout `JavaScout/src/main/resources/com/framstag/libosmscout/main.fxml` and stylesheet `style.css`.
- Java controllers/overlays: `MainController.java`, `JavaScoutApp.java`, `MapRenderer.java`, `MapInteractionHandler.java`, `SearchOverlay.java`, `RoutePanel.java`.
- JNI render path in `libosmscout-client-java/src/OSMScoutClient.cpp` is already implemented; Java side must ensure route arrays are passed correctly.
- No public API changes to `libosmscout-client-java`.
