# Basemap Support for JavaScout

## What Changes

JavaScout currently has no way to discover, download, or use the world basemap (borders, country names, coastlines) that exists on the map provider's FTP server. The basemap is a special world-wide map not listed in the regular map JSON listing.

The C++/JNI layer already supports a basemap lookup directory (`OSMScoutClientBuilder.withBasemapLookupDirectory()`, `DBThread` basemap parameter), but JavaScout never sets it. The `BasemapImport` tool and basemap stylesheets (`basemap.ost`, `basemap.oss`) already exist.

This change adds basemap support to JavaScout: discovery, download, automatic loading, and visual feedback.

## Capabilities

### New Capabilities

- `basemap-discovery`: Detect basemap availability on the configured map provider. The basemap lives at a well-known path on the server (e.g., `world/basemap`) but is not returned by the standard map listing endpoint. JavaScout probes for it separately.

- `basemap-download`: Download the basemap via the existing `MapDownloadManager` infrastructure. The basemap is treated as a special map entry with a fixed name "World Basemap" and a known server directory.

- `basemap-loading`: On startup, if a basemap directory exists, pass it to `OSMScoutClientBuilder.withBasemapLookupDirectory()` so the C++ layer loads it as an overlay. The basemap renders underneath regional maps, providing context (borders, country names) when zoomed out or when no regional map is loaded.

- `basemap-ui`: Show basemap status in the UI — indicate whether a basemap is installed, offer download/update in the Map Download dialog, and show a "Basemap" entry in the installed maps list.

### Modified Capabilities

- `map-download`: The Map Download dialog gains a "Basemap" section or indicator. The installed maps list includes the basemap alongside regional maps.

## Impact

- **JavaScout/**: `MainController.java` — add `withBasemapLookupDirectory()` call in `initClientAndRenderer()`. `MapDownloadController.java` — add basemap discovery and download UI. `Config.java` — persist basemap directory path.

- **libosmscout-client-java/**: `OSMScoutClientBuilder.java` — already has `withBasemapLookupDirectory()`, no changes needed. `MapDownloadManager.java` — may need a method to probe/download basemap separately from the regular map listing.

- **libosmscout-client-java/src/**: `OSMScoutClient.cpp` — already passes basemap dir to `DBThread`, no changes needed.

- **No changes** to core libosmscout, libosmscout-import, or stylesheets — existing infrastructure is sufficient.
