# JavaScout TODO

## Open Issues

### 1. Coordinate buffer reallocation (data import)
`Bounding box hit rate for file nodes.dat is only 0%` — data import problem, not code bug.
Causes `*** Buffer reallocation: 1048576` (1M Vertex2D elements, ~16MB) from scanning all nodes.
- Root cause: imported map data has no bounding box metadata for nodes.dat
- Fix: re-import map data with correct settings, or patch import pipeline to generate bounding boxes
- Impact: memory waste, slower startup, potential OOM on large datasets

### 2. `redrawCurrentLocationMarker()` is dead code
~~Defined in `MapRenderer.java` but never called. `setCurrentLocation()` calls `drawMarker()` directly.~~
- ~~Clean up: remove unused method~~
- [x] Removed

### 3. `atanh()` duplicated in two classes
~~Both `MapRenderer.java` and `MapInteractionHandler.java` define their own `private static double atanh(double x)`.~~
- ~~Extract to shared utility class (e.g., `MathUtils` or `ProjectionUtils`)~~
- [x] Extracted to `ProjectionUtils.java`

### 4. `geoToScreen()` Mercator projection duplicated
~~Same projection math in `MapRenderer.java` and `MapInteractionHandler.java`.~~
- ~~Extract to shared utility class~~
- [x] Extracted to `ProjectionUtils.java` with `geoToScreen()`, `screenToGeo()`, `dragDeltaToNewCenter()`, `zoomAtCursor()`

### 5. Debounce thread busy-waits
~~`startDebounceLoop()` uses `Thread.sleep(10)` when idle.~~
- ~~Replace with `wait/notify` or `ScheduledExecutorService` for efficiency~~
- [x] Replaced with `debounceLock.wait()` — thread sleeps until notified by `submitDebounced()`

### 6. Render thread busy-waits
~~`startRenderThread()` uses `Thread.sleep(10)` when no job pending.~~
- ~~Replace with `wait/notify` or `LinkedBlockingQueue` take() pattern~~
- [x] Replaced with `renderLock.wait()` — thread sleeps until notified by `enqueueRenderJob()`

### 7. `pendingRender` / `debounceState` race condition
~~`submitDebounced()` sets `debounceState` and `pendingRender` in separate operations without synchronization.~~
- ~~Debounce thread could read state before pending render is set~~
- [x] Fixed: set `pendingRender` BEFORE `debounceState` (volatile happens-before guarantees visibility)
- [x] Also added `debounceLock.notify()` to wake debounce thread immediately instead of polling

### 8. `forceFullRender` parameter bug
~~`submitDebounced()` accepts `forceFullRender` but always tries sub-region blit first regardless. When overlay data changed (favorites/search/track) but view hadn't moved, blit succeeded and returned early — no render triggered, overlay never appeared.~~
- [x] Fixed: skip sub-region blit when `forceFullRender` is true

### 9. No unit tests for Mercator projection
~~`geoToScreen()` has no unit tests. Critical path for marker positioning, sub-region offset, and mouse drag.~~
- ~~Add parameterized tests for known lat/lon → pixel mappings~~
- [x] Added `ProjectionUtilsTest` (28 tests: atanh, computeScale, geoToScreen, screenToGeo, round-trip, dragDelta, zoomAtCursor, edge cases)
- [x] Added `MapRendererBlitOffsetTest` (14 tests: viewport bounds, overrun fit/miss, clipping, destination coords)

### 10. No error recovery for JNI render failures
~~When `client.render()` or `client.renderWithRouteAndPois()` throws, error is printed to stderr but no retry or fallback.~~
- ~~Consider retry with smaller render area or degraded quality~~
- [x] Added single retry with 100ms delay before giving up

### 11. Marker drawn on canvas, not in buffer
`drawMarker()` draws directly to canvas `GraphicsContext`. Marker is lost when buffer is blitted.
- Current approach: `drawMarker()` called after every blit in `executeRender()` and `trySubRegionBlit()`
- Works but fragile — any blit path that forgets to call `drawMarker()` loses the marker
- Consider compositing marker onto front buffer instead

### 12. Tile cache stored but unused for display
`TileCache` stores tiles after each render but they're never used for display composition.
- Spec says "Tiles stored for potential future use (e.g., offline cache)"
- Could be used for faster re-renders after pan (composite from cached tiles instead of full JNI render)

### 13. `System.err.println` for all error logging
~~No proper logging framework. All errors go to stderr.~~
- ~~Consider adding SLF4J or java.util.logging~~
- [x] Added `Log.java` utility wrapping `java.util.logging.Logger`
- [x] Replaced all 48 `System.err/out.println` calls across 8 files with `Log.info()`, `Log.error()`

### 14. Canvas size may be 0 at startup
~~`enqueueRenderJob()` checks `w <= 0 || h <= 0` and returns early.~~
- ~~Initial render may be delayed until canvas has a size~~
- ~~Ensure initial render is triggered after canvas is laid out~~
- [x] Added listener on canvas width/height in constructor. When canvas gets first non-zero size, fires `enqueueRenderJob()` directly (bypasses debounce). Uses `initialRenderPending` flag to fire only once.
