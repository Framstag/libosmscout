## What Changes

Replace JavaScout's naive single-frame render pipeline with a buffered, overrun-based rendering system. Currently every view change (pan, zoom, rotate) triggers a full Cairo render via JNI — expensive and causes visible flicker. The new pipeline adopts techniques proven in OSMScout2's `PlaneMapRenderer` and `TiledMapRenderer`.

**Key changes:**
- Off-screen buffer + double buffering (swap on render complete)
- Canvas overrun: render 2.5× visible area, reuse on small pans
- Tile cache: store rendered tiles in LRU cache for potential future use (not used for display composition)
- Combined approach: overrun buffer for interactive panning, full re-render for zoom/rotate

## Capabilities

### New Capabilities

- `off-screen-buffer`: Maintain a persistent off-screen image buffer. Render into it asynchronously. On completion, swap to front buffer and blit to JavaFX Canvas. Eliminates flicker and allows UI thread to always display a complete frame.

- `canvas-overrun`: Render a larger area (1.5× screen dimensions) into the off-screen buffer. On pan, compute offset between old and new view center. If new viewport fits within the rendered area, copy the relevant sub-region — no re-render needed. Only trigger full re-render on zoom, rotation, or pan beyond the overrun margin.

- `tile-cache`: Split the map view into fixed-size tiles at each zoom level. Cache rendered tiles in an LRU map after each full render. Tiles are NOT used for display composition (keyed by pixel coordinates, not geographic position — would return stale content after pan). Stored for potential future use (e.g., offline cache).

- `async-render-pipeline`: Move all Cairo rendering (JNI calls) to a background thread. Use a debounced request queue. Every render completes and writes its result (no cancellation — prevents starvation where no render ever finishes). Notify UI thread on completion via `Platform.runLater`.

### Modified Capabilities

- `map-rendering`: The existing `MapRenderer.java` render pipeline is replaced. The `render()` and `renderWithRouteAndPois()` JNI methods remain the same, but call patterns change. The `MapRenderer` API surface stays backward-compatible.

## Impact

**Affected code:**
- `JavaScout/src/main/java/com/framstag/libosmscout/MapRenderer.java` — major rewrite
- `JavaScout/src/main/java/com/framstag/libosmscout/MainController.java` — minor: add canvas resize listeners, panel min size, clip for window resize handling
- `JavaScout/src/main/java/com/framstag/libosmscout/MapInteractionHandler.java` — minor: pan/zoom events use Mercator projection for mouse drag
- `JavaScout/src/main/java/com/framstag/libosmscout/TileCache.java` — new file: LRU tile cache
- `libosmscout-client-java/` — JNI layer unchanged (existing `render()` and `renderWithRouteAndPois()` sufficient)

**Dependencies:** None new. All techniques use existing JavaFX APIs (`Canvas`, `PixelWriter`, `WritableImage`) and existing JNI methods.

**Performance targets:**
- Pan within overrun area: 0 ms (no re-render, just blit)
- Pan beyond overrun: re-render, but 1.5× area means fewer re-renders overall
- Zoom/rotate: re-render, tile cache reduces per-frame cost
- No visible flicker during any operation
