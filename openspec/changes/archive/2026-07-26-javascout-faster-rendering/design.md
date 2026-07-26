## Context

JavaScout's current `MapRenderer.java` uses a naive single-frame render pipeline:

1. View change triggers `requestRender()` → sets `pendingRequest`
2. Debounce thread polls every 200ms, picks up request
3. Spawns a new thread per render, calls JNI `client.render()` or `client.renderWithRouteAndPois()`
4. On success, `Platform.runLater()` blits `int[]` pixels to JavaFX Canvas via `PixelWriter.setPixels()`
5. Current-location marker drawn on top in same `runLater`

**Problems:**
- Every pan/zoom/rotate = full Cairo render (expensive: DB tile load + projection + draw)
- No off-screen buffer → visible flicker between frames
- No reuse of previously rendered content
- New thread per render, no cancellation of in-flight work
- 200ms fixed debounce adds latency even when render is fast

**Reference: OSMScout2 `PlaneMapRenderer`** solves this with:
- `canvasOverrun = 1.5` — renders 1.5× screen dimensions
- Double buffering: `currentImage` (back) ↔ `finishedImage` (front), swapped on completion
- `RenderMap()` checks if finishedImage covers current viewport → if yes, blits with offset/scale, no re-render
- Debounce timer: 10ms initial, 200ms for updated data
- Background `DBLoadJob` + `DBRenderJob` with tile state change notifications

**Reference: OSMScout2 `TiledMapRenderer`** adds:
- Tile-based rendering at fixed zoom levels
- LRU tile caches (online + offline)
- Tile merging: adjacent tile requests merged into larger render jobs
- Per-tile epoch tracking for cache invalidation

## Goals / Non-Goals

**Goals:**
- Eliminate visible flicker during map operations
- Skip re-render on small pans (reuse off-screen buffer)
- Reduce render frequency via debounce + cancellation
- Keep existing JNI methods unchanged (`render()`, `renderWithRouteAndPois()`, `projectToPixel()`)
- Maintain backward-compatible `MapRenderer` public API
- Support all existing overlays: route, favorites, search marker, track, current-location

**Non-Goals:**
- Tile-based rendering as primary mode (overrun buffer is simpler and sufficient for desktop)
- Online tile downloading (no web tile source for JavaScout)
- GPU-accelerated rendering (Cairo is CPU-based; GPU would require OpenGL backend)
- Multi-monitor DPI handling improvements
- Changing the JNI/C++ rendering layer

## Decisions

### Decision 1: Overrun buffer as primary, tile cache as secondary

**Choice:** Implement canvas overrun + double buffering first. Add tile cache as an optimization layer on top if needed.

**Rationale:**
- Overrun buffer gives the biggest win for least complexity: panning is the most frequent operation
- OSMScout2 `PlaneMapRenderer` proves this works well with `canvasOverrun=1.5`
- Tile cache adds complexity (tile grid math, LRU eviction, partial updates) but only helps when zooming/rotating
- Can add tile cache later without breaking the overrun buffer architecture

### Decision 2: `WritableImage` as off-screen buffer, not raw `int[]`

**Choice:** Use JavaFX `WritableImage` for the off-screen buffer instead of keeping raw `int[]` arrays.

**Rationale:**
- `WritableImage` can be drawn to Canvas via `GraphicsContext.drawImage()` with sub-region cropping
- Sub-region copy is a single GPU operation vs manual pixel array copy
- `PixelWriter` still used for initial pixel write from JNI result
- Simplifies the overrun sub-region blit logic

### Decision 3: Dedicated render thread with job queue, not thread-per-render

**Choice:** Single background render thread with a blocking job queue. New request cancels pending job.

**Rationale:**
- Avoids thread explosion on rapid view changes
- Enables clean cancellation: set a `volatile boolean cancel` flag, check it in the render loop
- Matches OSMScout2 pattern of `DBLoadJob` + `DBRenderJob` on a single thread
- Simpler synchronization than thread pool

### Decision 4: Epoch-based cache invalidation

**Choice:** Increment an epoch counter on zoom/rotate/style-change. Tag each cached tile/buffer with the epoch. On render, discard entries with stale epoch.

**Rationale:**
- OSMScout2 uses this pattern (`currentEpoch`, `finishedEpoch`)
- Simple O(1) invalidation — no need to walk the cache
- Works for both overrun buffer and tile cache

### Decision 5: Overrun factor of 2.5

**Choice:** Render at 2.5× screen dimensions (increased from original 1.5).

**Rationale:**
- 2.5× means the user can pan ~75% of screen width in any direction before hitting the margin
- Covers most pan gestures without triggering a full render
- Larger buffer uses more memory (~18MB at 1920×1080) but eliminates frequent full-render pauses
- The sub-region blit now always draws with background fill for areas outside the buffer, so even when the margin is exceeded, the map moves smoothly with gray edges

### Decision 6: Sub-region blit always executes, fills gaps with background

**Choice:** The sub-region blit always draws the overlapping part of the front buffer, even when the viewport extends beyond the rendered area. Areas outside the buffer are filled with a background color. The blit returns `false` when a full render is needed, triggering an async render to fill the gaps.

**Rationale:**
- Eliminates jump-back when viewport exceeds rendered area
- Map remains responsive during full renders
- Gray edges are preferable to waiting for renders
- Full render completes asynchronously and fills the gaps

### Decision 7: Pixel offset uses Mercator projection, not flat-earth

**Choice:** Compute sub-region pixel offset using the full Mercator projection (`geoToScreen()` with `atanh` and latitude-dependent scale), matching the marker positioning formula.

**Rationale:**
- Flat-earth approximation (`degPerPx = 360 / 2^mag / screenW`) diverges from the Mercator projection at higher latitudes
- Marker drifted relative to map content during pan
- Using the same projection for both map offset and marker position keeps them aligned

### Decision 8: No cancellation of in-flight renders

**Choice:** Removed the `cancelCurrent` mechanism. Every render the thread starts completes and writes its result to the buffer.

**Rationale:**
- `cancelCurrent` caused every render to be discarded when a new pan event arrived
- With rapid pan events, no render ever completed, resulting in no visual updates
- The `AtomicReference<RenderJob>` ensures the latest job is always processed next

### Decision 9: Tile cache NOT used for display composition

**Choice:** Tiles are stored after renders but never composed into a full image for display.

**Rationale:**
- Tiles are keyed by pixel coordinates `(zoomLevel, tileX, tileY)`, not geographic position
- After a pan, cached tiles from the previous position would show stale content
- The composite path caused snap-back to the original position

### Decision 10: Current-location marker drawn without front buffer redraw

**Choice:** `setCurrentLocation()` calls `drawMarker()` directly instead of `redrawCurrentLocationMarker()` (which redrew the front buffer).

**Rationale:**
- `redrawCurrentLocationMarker()` called `blitFrontBuffer()` which drew the center of the front buffer
- After a sub-region blit pan, the front buffer still contained the original image
- Redrawing the front buffer overwrote the panned view, causing snap-back
- `drawMarker()` only draws the marker on the existing canvas, preserving the panned view

### Decision 11: Zoom scales current buffer as placeholder

**Choice:** When zoom level changes, scale the current front buffer to the new magnification and display it immediately as a temporary placeholder. Trigger a full render for the high-quality version.

**Rationale:**
- Without scaling, zoom causes a visual pause until the full render completes
- Each magnification level doubles/ halves resolution: scale = 2^(newMag - oldMag)
- Zoom in: take center sub-region of buffer, scale up (pixelated but responsive)
- Zoom out: scale buffer down, center with gray borders
- Full render replaces placeholder when complete — no jump-back because the view state is already updated to the new mag

### Decision 12: Window resize unbinds canvas before setting size

**Choice:** When the window is resized, the Canvas dimensions are updated by listening to the parent panel's width/height properties. Before setting the canvas size, the width/height properties are unbound (the layout system may have bound them). The parent panel's minimum size is set to 0 to allow the BorderPane to shrink the center below the canvas size. A clip rectangle is bound to the panel dimensions to prevent canvas overflow.

**Rationale:**
- JavaFX Canvas has a fixed size — it does not automatically resize when the parent shrinks
- The StackPane computes its minimum size from the canvas, preventing the BorderPane from shrinking the center
- Setting min size to 0 allows the BorderPane to shrink the center, keeping the status bar visible
- The clip prevents the canvas from visually overlapping the status bar when the canvas is larger than the panel
- Unbinding before setting avoids "A bound value cannot be set" exceptions from the layout system

## Risks / Trade-offs

| Risk | Impact | Mitigation |
|------|--------|------------|
| Memory: 1.5× buffer at high DPI (e.g. 4K) = ~18MB for 1920×1080×1.5 | Acceptable on desktop | Use `WritableImage` which can be GC'd; cap at screen size |
| Overrun buffer stale after overlay change (route, favorites, track) | User sees outdated overlays | Trigger full re-render when overlay data changes; overlay changes are infrequent |
| Race condition: render completes after newer request arrives | Wrong frame displayed | Check epoch before swapping buffers; discard if epoch mismatch |
| JNI `render()` is synchronous and blocks the render thread | UI thread unaffected (render is on background thread) | Acceptable — Cairo render is CPU-bound anyway |
| Canvas resize during render | Wrong dimensions | Check canvas size at start of render; discard if changed |
| Current-location marker jitter during pan (no re-render) | Marker position drifts | Re-project marker in UI thread on each frame; no re-render needed |
