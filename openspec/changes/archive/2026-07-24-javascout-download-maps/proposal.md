# JavaScout Map Download

## What Changes

Add map download capability to JavaScout and the underlying client libraries. Users can browse available maps from configured map providers, download them to a local directory, and use them for rendering and routing — all from within the JavaScout JavaFX application.

The default download directory is `maps/` inside the JavaScout config directory (`~/.config/javascout/maps/` on Linux), configurable via the existing `Config` properties file.

The feature builds on the existing `libosmscout-client` (C++ core) types (`MapProvider`, `MapDirectory`, `MapManager`), adding the missing non-Qt download orchestration and Java JNI bindings so JavaScout can use them without Qt dependencies.

## Capabilities

### New Capabilities

- `map-provider-registry`: Use the existing `MapProvider` type in `libosmscout-client` core. JavaScout hard-codes the default `karry.cz` provider for now and stores a provider name in `Config`.

- `map-list-fetch`: Fetch available map tree from a provider's JSON API. JavaScout performs the HTTP request in Java (`java.net.http.HttpClient`) and passes the JSON to the native `MapDownloadService::ParseMapList()` via JNI (`nativeParseMapList()`), returning a tree of `AvailableMapEntry`.

- `map-download-engine`: Download map files (multiple files per map) from a server to a local directory. The Java `MapDownloadManager` performs HTTP in Java and uses JNI only for directory preparation (`nativePrepareMapDirectory()`), metadata writing, and registration (`nativeRegisterMapDirectory()`). The C++ `MapDownloadService` provides helper functions for these steps but does not perform HTTP itself for the Java path.

- `map-install-management`: After download, register the new map directory with `MapManager` so it appears in `DBThread`'s database list and becomes available for rendering and routing. Already partially handled by `MapManager::LookupDatabases()` — ensure the download path is in the lookup directories.

- `javascout-map-download-ui`: JavaFX UI in JavaScout with:
  - Map browser dialog (tree view of available maps grouped by region), sections collapsed by default
  - Download queue with progress bars per map, status text, and cancel button
  - Installed maps management (list by map name, delete by full directory path)
  - Context-menu entry on the map to open the dialog (not the app menu bar)

- `javascout-client-java-map-download`: Java JNI bindings in `libosmscout-client-java`:
  - `MapProvider` Java class (mirrors C++ `osmscout::MapProvider`)
  - `AvailableMapEntry` Java class (directory or leaf map entry)
  - `MapDownloadListener` Java interface for progress/completion/error events
  - `MapDownloadManager` Java class with methods: `fetchAvailableMaps()`, `downloadMap()`, `cancelDownload()`, `getInstalledMaps()`, `deleteMap()`
  - JNI bridge in `OSMScoutClient.cpp` for the C++ helpers (`nativeParseMapList`, `nativeGetMapFileNames`, `nativePrepareMapDirectory`, `nativeRegisterMapDirectory`, `nativeGetInstalledMaps`, `nativeDeleteMap`)

### Modified Capabilities

- `libosmscout-client-settings`: Extend `Settings` to persist the maps download directory path and selected map provider ID. Add `mapsDirectory` and `mapProviderId` keys.

- `libosmscout-client-map-manager`: Extend `MapManager` to support adding lookup directories at runtime (not just at construction). Add `AddLookupDirectory()` method. The C++ client startup (`OSMScoutClient.cpp` build) now triggers `MapManager::LookupDatabases()` and waits for it so previously downloaded maps are discovered before the client is returned.

- `javascout-config`: Extend `Config.java` with `getMapProvider()` / `setMapProvider()`; `getMapsDirectory()` already returns `<configDir>/maps` as fallback.

## Impact

### New Files

**libosmscout-client (C++ core):**
- `include/osmscoutclient/HttpClient.h` — abstract interface for HTTP operations
- `include/osmscoutclient/MapDownloadService.h` — map list fetcher and download engine (uses `HttpClient`)
- `src/osmscoutclient/MapDownloadService.cpp`
- `include/osmscoutclient/AvailableMapEntry.h` — tree entry for available maps
- `src/osmscoutclient/AvailableMapEntry.cpp`

**libosmscout-client-qt:**
- `include/osmscoutclientqt/QtHttpClient.h` — `HttpClient` impl using `QNetworkAccessManager`
- `src/osmscoutclientqt/QtHttpClient.cpp`

**libosmscout-client-java (Java JNI):**
- `java/com/framstag/libosmscout/client/MapProvider.java`
- `java/com/framstag/libosmscout/client/AvailableMapEntry.java`
- `java/com/framstag/libosmscout/client/MapDownloadManager.java`
- `java/com/framstag/libosmscout/client/MapDownloadListener.java`
- `src/JniHttpClient.cpp` — **removed**; HTTP is performed in Java to avoid an OpenJDK 17.0.2 G1 read-barrier crash

**JavaScout (JavaFX app):**
- `src/main/java/com/framstag/libosmscout/MapDownloadController.java`
- `src/main/resources/com/framstag/libosmscout/MapDownloadDialog.fxml`
- `src/main/resources/com/framstag/libosmscout/main.fxml` — context-menu entry for download dialog

### Modified Files

**libosmscout-client:**
- `include/osmscoutclient/MapManager.h` — add `AddLookupDirectory()`
- `src/osmscoutclient/MapManager.cpp` — implement `AddLookupDirectory()`, add scan logging
- `include/osmscoutclient/Settings.h` — add maps directory + provider ID getters/setters
- `src/osmscoutclient/Settings.cpp` — implement new settings
- `include/osmscoutclient/HttpClient.h` — abstract `HttpClient` interface (kept for other consumers; not used by Java path)
- `include/osmscoutclient/MapDownloadService.h` — helper API for directory preparation/registration
- `src/osmscoutclient/MapDownloadService.cpp`
- `include/osmscoutclient/AvailableMapEntry.h` — tree entry for available maps
- `src/osmscoutclient/AvailableMapEntry.cpp`
- `CMakeLists.txt` — add new source/header files; **no libcurl link**

**libosmscout-client-java:**
- `src/OSMScoutClient.cpp` — add JNI methods for map download; trigger initial `MapManager::LookupDatabases()` during build; keep `knownPaths` empty so the download parent directory is not opened as a database
- `java/com/framstag/libosmscout/client/OSMScoutClient.java` — add `getMapDownloadManager()` method
- `java/com/framstag/libosmscout/client/OSMScoutClientBuilder.java` — add `withMapsDirectory()` method; `withMapLookupDirectories()` now accepts multiple lookup directories
- `java/meson.build` — add new Java source files
- `src/meson.build` — add new JNI source; **no libcurl link**, `JniHttpClient.cpp` removed

**JavaScout:**
- `src/main/java/com/framstag/libosmscout/JavaScout.java` — add `--map-provider` CLI arg; CLI maps directory argument is used for map lookup only, not for download storage
- `src/main/java/com/framstag/libosmscout/JavaScoutApp.java` — initialize map download manager
- `src/main/java/com/framstag/libosmscout/MainController.java` — add context-menu action for map download dialog; pass both the configured maps directory and the default download directory to `OSMScoutClientBuilder`; do not call `client.openDatabase()` at startup because `MapManager` discovers the actual map subdirectories
- `src/main/java/com/framstag/libosmscout/Config.java` — add map provider getter/setter
- `src/main/java/com/framstag/libosmscout/MapDownloadController.java` — handle tree selection, download start/cancel, installed map list refresh/deletion; display map name derived from full path; keep full directory path for deletion
- `JavaScout/build.sh` — installs the freshly built `libosmscoutclientjava.jar` into the local Maven repository before packaging JavaScout
- `JavaScout/README.md` — document Meson-first build flow

**Build system:**
- No changes to root `CMakeLists.txt` or `meson.build` — no new C++ dependencies
- `libosmscout-client-java/src/meson.build` — link `libosmscoutclientjava` against `libosmscoutclient` (already done)

### Dependencies

- **No new C++ library dependencies.** HTTP is abstracted behind `HttpClient` interface, but the Java download path does not use the C++ `HttpClient` implementation.
- **JavaScout**: Uses Java standard library `java.net.http.HttpClient` (Java 11+, already required). All HTTP for map list fetching and file downloads runs on Java threads.
- **OSMScout2**: Continues to use its existing Qt `QNetworkAccessManager` download path (unchanged).
- **Existing**: nlohmann/json (already in `libosmscout-client` for JSON parsing), zlib, libpng, etc.

### Findings

- **OpenJDK 17.0.2 G1 read-barrier crash**: Calling `java.net.http.HttpClient.send()` / `sendAsync()` from JNI/native code reproducibly crashes the JVM at `G1BarrierSet::AccessBarrier::oop_access_barrier`. This crash happened inside `MapDownloadService::DownloadMapSync()` → `JniHttpClient::Download()`. The only reliable fix was to move all HTTP execution to Java threads and restrict the JNI layer to filesystem/metadata/registration work.
- **Stale `libosmscoutclientjava.jar`**: JavaScout depends on the Meson-built client jar. An outdated jar in `~/.m2/repository` produced `UnsatisfiedLinkError` at runtime. `JavaScout/build.sh` now installs the freshly built jar into the local Maven repository before `mvn package`.
- **Map directory discovery at startup**: `MapManager::LookupDatabases()` is not called automatically during `MapManager` construction; it must be triggered explicitly. The JNI client build now calls it and waits for completion so downloaded maps are available immediately.
- **Raw download parent must not be opened as a database**: Passing `~/.config/javascout/maps/` directly to `DBThread` causes errors such as "Cannot open file .../maps/types.dat". Only the discovered map subdirectories should be opened. `OSMScoutClient.cpp` therefore keeps `knownPaths` empty and lets `MapManager` discover the actual database directories.

### Risks

- **No new C++ dependency**: The abstract `HttpClient` interface keeps the core library dependency-free, although the Java path does not use a C++ HTTP implementation.
- **Thread safety**: Java downloads run on Java worker threads; JNI callbacks for progress are posted to JavaFX via `Platform.runLater()`. Progress updates are throttled to ~4 Hz to avoid flooding the UI thread.
- **Conflict with Qt download path**: The Qt `MapDownloader`/`FileDownloader` chain in `libosmscout-client-qt` remains unchanged. The Java path is independent and uses Java-side HTTP.
- **Teardown race**: A pre-existing `std::bad_alloc` can occur in `MapManager::LookupDatabases()` during `client.close()` when it races with other threads. This is unrelated to the download crash and remains a known issue.
- **Installed-list deletion must use full path**: `nativeGetInstalledMaps()` returns `MapDirectory::GetDirStr()` (full path). The Java controller derives the display name from the path and keeps the path as the deletion key. Returning display names caused `deleteMap()` to receive a basename and fail silently.
- **Empty map labels**: `MapDirectory` metadata may have an empty `name` field. The Java controller derives the display name from the full path returned by `nativeGetInstalledMaps()`. `MapManager::LookupDatabases()` logs candidate/valid/invalid counts for diagnosis.
