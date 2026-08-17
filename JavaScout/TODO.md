# JavaScout TODO

## Open Issues

### 1. Coordinate buffer reallocation (data import)
`Bounding box hit rate for file nodes.dat is only 0%` — data import problem, not code bug.
Causes `*** Buffer reallocation: 1048576` (1M Vertex2D elements, ~16MB) from scanning all nodes.
- Root cause: imported map data has no bounding box metadata for nodes.dat
- Fix: re-import map data with correct settings, or patch import pipeline to generate bounding boxes
- Impact: memory waste, slower startup, potential OOM on large datasets
- Status: still open — external to JavaScout, lives in the import pipeline

### 2. Marker drawn on canvas, not in buffer
`drawMarker()` draws directly to canvas `GraphicsContext`. Marker is lost when buffer is blitted.
- Current approach: marker redrawn after every blit (`executeRender()` blit path, `trySubRegionBlit()`) and via `Platform.runLater(this::drawMarker)` from `setCurrentLocation()`/`clearCurrentLocation()`
- Works but fragile — any blit path that forgets to redraw the marker loses it
- Consider compositing marker onto front buffer instead

### 3. Tile cache stored but unused for display
`TileCache` stores tiles after each render (`tileCache.storeTiles()` in `executeRender()`) but they're never read back for display composition.
- Spec says "Tiles stored for potential future use (e.g., offline cache)"
- Could be used for faster re-renders after pan (composite from cached tiles instead of full JNI render)

### 4. `MapDownloadController` bypasses `Log.java`
`MapDownloadController.java` (basemap download, added 2026-08-04) uses 13 raw `System.err.println` calls instead of the `Log` utility — regression of the earlier `Log.java` migration (items 2–10, 13, 14 of the original list, all implemented).
- All output goes to stderr, no log levels, no timestamps
- Fix: replace with `Log.info()` / `Log.error()` like the rest of the codebase
- Also: broad `catch (Exception e)` blocks swallow errors with only a stderr print — consider structured error reporting to the UI
