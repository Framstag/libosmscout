## 1. Render Thread & Job Queue

- [x] 1.1 Replace thread-per-render with single dedicated background render thread
- [x] 1.2 Implement job submission via `AtomicReference<RenderJob>` (replaces `LinkedBlockingQueue`)
- [x] 1.3 Remove `cancelCurrent` mechanism — every render completes and writes its result
- [x] 1.4 New request overwrites pending job atomically (no queue drain race)
- [x] 1.5 Add `Platform.runLater` callback on render completion for buffer swap + blit

## 2. Double Buffering

- [x] 2.1 Add `WritableImage backBuffer` and `WritableImage frontBuffer` fields
- [x] 2.2 Implement atomic buffer swap under lock (`ReentrantLock`)
- [x] 2.3 Write JNI pixel data to back buffer via `PixelWriter.setPixels()`
- [x] 2.4 Blit front buffer to Canvas via `GraphicsContext.drawImage()` in UI thread
- [x] 2.5 Handle null front buffer (first render or after clear) — draw background color

## 3. Canvas Overrun

- [x] 3.1 Add `canvasOverrun` field (default 2.5) and config setter
- [x] 3.2 Compute overrun dimensions: `(int)(w * overrun)`, `(int)(h * overrun)`
- [x] 3.3 On pan: compute pixel offset using Mercator projection (`geoToScreen()`)
- [x] 3.4 Always draw sub-region (clipped to buffer bounds), fill gaps with background color
- [x] 3.5 If viewport fits within rendered area: return true (no full render needed)
- [x] 3.6 If viewport exceeds rendered area: return false (triggers async full render, gray edges shown until complete)
- [x] 3.7 Invalidate overrun buffer on zoom, rotation, or overlay change
- [x] 3.8 Zoom scales current buffer as placeholder: scale front buffer by 2^(newMag-oldMag), display immediately, trigger full render
- [x] 3.9 Zoom in: take center sub-region and scale up (pixelated but responsive)
- [x] 3.10 Zoom out: scale buffer down and center with gray borders

## 4. Adaptive Debounce

- [x] 4.1 Replace 200ms fixed poll loop with event-driven timer
- [x] 4.2 Set 50ms debounce for pan events (mouse drag)
- [x] 4.3 Set 200ms debounce for zoom/rotate events
- [x] 4.4 Cancel pending timer on new request of same type
- [x] 4.5 Fire render immediately if no render has occurred yet (initial load)

## 5. Epoch-Based Invalidation

- [x] 5.1 Add `volatile long epoch` counter, incremented on zoom/rotate/style/overlay change
- [x] 5.2 Tag each render job with current epoch at submission time
- [x] 5.3 On render completion: compare job epoch with current epoch
- [x] 5.4 Discard completed render if epoch mismatch (newer request already submitted)
- [x] 5.5 Tag front buffer with epoch for overrun reuse checks

## 6. Tile Cache (Secondary)

- [x] 6.1 Implement `TileCache` class with `LinkedHashMap` LRU eviction
- [x] 6.2 Key tiles by `(zoomLevel, tileX, tileY)` tuple
- [x] 6.3 Store tiles after each full render completes
- [x] 6.4 Tile cache NOT used for display composition (tiles keyed by pixel coords, not geographic position)
- [x] 6.5 Tiles stored for potential future use (e.g., offline cache)
- [x] 6.6 Invalidate all tiles on epoch change (zoom/rotate/style)

## 7. Manual Testing

### 7.3 Overlay rendering with overrun buffer
- [x] 7.3.1 Open app, wait for initial render
- [x] 7.3.2 Calculate a route → route polyline and start/end markers appear
- [x] 7.3.3 Pan the map → route overlay stays visible and correctly positioned
- [x] 7.3.4 Zoom in/out → route overlay re-renders at new zoom level
- [x] 7.3.5 Add a favorite location → favorite marker appears
- [x] 7.3.6 Pan the map → favorite marker stays at correct geographic position
- [x] 7.3.7 Search for a location → search result marker appears
- [x] 7.3.8 Import a GPX track → track polyline appears
- [x] 7.3.9 Pan/zoom with track visible → track stays correctly positioned

### 7.4 Current-location marker during sub-region blit
- [x] 7.4.1 Enable GPS/navigation → current-location marker appears
- [x] 7.4.2 Pan the map via mouse drag → marker re-projects to correct screen position
- [x] 7.4.3 Marker does not drift or jitter relative to map content during pan
- [x] 7.4.4 Heading arrow points in correct direction after pan

### 7.5 Rapid interaction sequences
- [x] 7.5.1 Rapidly drag mouse across full screen width → map follows smoothly, no flicker
- [x] 7.5.2 Rapidly scroll zoom in/out → zoom settles at final level, no flicker
- [x] 7.5.3 Pan immediately after zoom → works without glitch
- [x] 7.5.4 Zoom immediately after pan → works without glitch
- [x] 7.5.5 Rapid alternating pan/zoom for 10 seconds → no crash, no frozen map

### 7.6 Window resize
- [x] 7.6.1 Drag window corner to resize larger → map re-renders at new size
- [x] 7.6.2 Drag window corner to resize smaller → map re-renders at new size
- [x] 7.6.3 Maximize window → map fills new area
- [x] 7.6.4 Restore window → map re-renders at original size
- [x] 7.6.5 Pan and zoom after resize → works normally

### 7.7 Shutdown
- [x] 7.7.1 Close app window → app exits cleanly (no hanging process)
- [x] 7.7.2 Check console output → no exceptions, no thread leak messages
- [x] 7.7.3 Open and close app 3 times in succession → works each time

## 8. Open Items

### 8.1 Coordinate buffer reallocation (data import)
- [ ] Re-import map data to fix 0% bounding box hit rate for nodes.dat
- [ ] Or patch import pipeline to generate bounding boxes

### 8.2 Code cleanup
- [x] Remove unused `redrawCurrentLocationMarker()` method
- [x] Extract duplicated `atanh()` to shared utility (`ProjectionUtils`)
- [x] Extract duplicated `geoToScreen()` to shared utility (`ProjectionUtils`)
- [x] Fix `forceFullRender` — was only used for debounce timing but sub-region blit ran first and succeeded (view hadn't moved), preventing overlay renders. Now skips blit when `forceFullRender` is true.

### 8.3 Performance
- [x] Replace busy-wait loops with wait/notify or ScheduledExecutorService
- [x] Fix race condition between `pendingRender` and `debounceState`
- [x] Add error recovery for JNI render failures

### 8.4 Logging
- [x] Add `Log.java` utility wrapping `java.util.logging.Logger`
- [x] Replace all `System.err/out.println` calls with `Log.info()`/`Log.error()`

### 8.5 Testing
- [x] Add unit tests for Mercator projection (`geoToScreen()`) — `ProjectionUtilsTest`
- [x] Add unit tests for sub-region blit offset calculation — `MapRendererBlitOffsetTest`

### 8.6 Startup
- [x] Add canvas size listener to trigger initial render when layout completes (canvas size was 0 at first `requestRender()` call)
