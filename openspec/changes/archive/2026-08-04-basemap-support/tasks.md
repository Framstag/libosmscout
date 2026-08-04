## 1. Basemap Discovery

- [x] 1.1 Add `BasemapManager` class to `libosmscout-client-java` that probes `{provider.uri}/basemap/` via HTTP and parses HTML directory listing for tar.gz archives
- [x] 1.2 Implement archive name/date parsing from Apache-style directory listing
- [x] 1.3 Add `BasemapManager.getAvailableBasemaps()` returning list of available archives with name, size, date
- [x] 1.4 Add `BasemapManager.getInstalledBasemapInfo()` reading version from extracted basemap metadata
- [x] 1.5 Add `BasemapManager.isUpdateAvailable()` comparing server vs installed version
- [x] 1.6 Write unit tests for directory listing parsing and version comparison

## 2. Basemap Download

- [x] 2.1 Add `BasemapManager.downloadBasemap()` that downloads tar.gz via `java.net.http.HttpClient` with progress reporting
- [x] 2.2 Add tar.gz extraction using `GZIPInputStream` + manual tar parsing streaming to `{mapsDir}/basemap/`
- [x] 2.3 Implement atomic swap: download + extract to temp dir, then rename on success
- [x] 2.4 Implement cancellation support for in-progress download
- [x] 2.5 Implement cleanup of partial files on failure or cancellation
- [x] 2.6 Add `BasemapManager.deleteBasemap()` for removing installed basemap
- [x] 2.7 Write unit tests for download, extraction, cancellation, and cleanup

## 3. Basemap Loading

- [x] 3.1 In `MainController.initClientAndRenderer()`, check for basemap directory and call `builder.withBasemapLookupDirectory()` if present
- [x] 3.2 After basemap download/update, trigger `client.openDatabase(basemapPath)` to reload via `OnDatabaseListChanged`
- [x] 3.3 Verify basemap renders underneath regional maps (no C++ changes needed — existing DBThread overlay behavior)
- [x] 3.4 Test: start JavaScout with basemap installed, verify borders/country names visible at low zoom

## 4. Basemap UI

- [x] 4.1 Add basemap section to `MapDownloadController` with "Download Basemap" / "Update Basemap" button
- [x] 4.2 Show basemap variant selection when multiple archives exist (full vs minimal)
- [x] 4.3 Show basemap download progress in the downloads table
- [x] 4.4 Show "World Basemap" entry in installed maps list with size and version
- [x] 4.5 Add basemap status indicator to main view status bar
- [x] 4.6 Wire basemap delete button in installed maps list

## 5. Build & Verify

- [x] 5.1 Build JavaScout with `mvn package` — verify no compilation errors
- [x] 5.2 Run existing unit tests — verify no regressions
- [x] 5.3 Run `mvn javafx:run` — verify app starts and Map Download dialog opens
- [x] 5.4 Manual test: probe basemap, download, verify rendering
