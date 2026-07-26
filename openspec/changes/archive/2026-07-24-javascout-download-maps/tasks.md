## 1. Core C++ — HttpClient Interface

- [x] 1.1 Define `HttpClient` abstract interface in `include/osmscoutclient/HttpClient.h` with `Fetch()` and `Download()` methods
- [x] 1.2 Add `ProgressCallback` type alias for download progress reporting
- [x] 1.3 Add `HttpClient` source to `libosmscout-client/CMakeLists.txt` and `meson.build`

## 2. Core C++ — AvailableMapEntry

- [x] 2.1 Define `AvailableMapEntry` class in `include/osmscoutclient/AvailableMapEntry.h` with fields: name, path, description, size, version, creation timestamp, server directory, isDirectory flag
- [x] 2.2 Implement JSON parser for server response format (array of map/dir entries)
- [x] 2.3 Add `AvailableMapEntry` source to build files

## 3. Core C++ — MapDownloadService

- [x] 3.1 Define `MapDownloadService` class in `include/osmscoutclient/MapDownloadService.h` extending `AsyncWorker`
- [x] 3.2 Implement `FetchMapList(provider, httpClient)` — fetches JSON from provider list URI, parses into `AvailableMapEntry` tree
- [x] 3.3 Implement `DownloadMap(entry, targetDir, httpClient)` — downloads all map files sequentially, writes metadata.json, renames `.download` → final
- [x] 3.4 Implement `CancelDownload()` — sets breaker flag, cleans up partial `.download` files
- [x] 3.5 Implement `GetDownloads()` — returns list of active download jobs with progress
- [x] 3.6 Add `MapDownloadService` source to build files

## 4. Core C++ — MapManager Extension

- [x] 4.1 Add `AddLookupDirectory(const std::filesystem::path &dir)` method to `MapManager`
- [x] 4.2 Change `databaseLookupDirs` from `const` to mutex-protected member
- [x] 4.3 Implement `AddLookupDirectory()` — append dir, trigger `LookupDatabases()`
- [x] 4.4 Update `GetLookupDirectories()` to return copy under mutex

## 5. Core C++ — Settings Extension

- [x] 5.1 Add `GetMapsDirectory()` / `SetMapsDirectory()` to `Settings`
- [x] 5.2 Add `GetMapProviderId()` / `SetMapProviderId()` to `Settings`
- [x] 5.3 Add `mapsDirectoryChanged` and `mapProviderIdChanged` signals to `Settings`

## 6. Qt HttpClient Adapter

- [ ] 6.1 Implement `QtHttpClient` in `libosmscout-client-qt` wrapping `QNetworkAccessManager`
- [ ] 6.2 `Fetch()` — use `QNetworkAccessManager::get()` with blocking `QEventLoop` or callback
- [ ] 6.3 `Download()` — stream response to file, emit progress via callback
- [ ] 6.4 Add `QtHttpClient` source to `libosmscout-client-qt` build files

## 7. Java JNI — Java-side HTTP (JniHttpClient removed)

OpenJDK 17.0.2 crashes in G1 read barriers when `java.net.http.HttpClient` methods are called from JNI/native code. The JNI layer therefore no longer performs HTTP. All network I/O for map download happens in Java.

- [x] 7.1 Remove `JniHttpClient`/`JniEnvAttacher` from `libosmscout-client-java`
- [x] 7.2 Java `MapDownloadManager.fetchAvailableMaps()` performs HTTP in Java and calls `nativeParseMapList()` for JSON parsing
- [x] 7.3 Java `MapDownloadManager.downloadMap()` performs file downloads in Java (`HttpClient.send()` called from Java thread)
- [x] 7.4 JNI only handles non-HTTP work: `nativeGetMapFileNames()`, `nativePrepareMapDirectory()`, `nativeRegisterMapDirectory()`

## 8. Java JNI — Java Classes

- [x] 8.1 Create `MapProvider.java` — mirrors C++ `osmscout::MapProvider` (name, uri, listUri)
- [x] 8.2 Create `AvailableMapEntry.java` — mirrors C++ `AvailableMapEntry` (name, path, size, version, isDirectory, children)
- [x] 8.3 Create `MapDownloadListener.java` — interface with `onProgress(name, bytes, total)`, `onComplete(name)`, `onError(name, message)`
- [x] 8.4 Create `MapDownloadManager.java` — Java API wrapping native methods
- [x] 8.5 Add new Java files to `libosmscout-client-java/java/meson.build`

## 9. Java JNI — OSMScoutClient Extension

- [x] 9.1 Add `getMapDownloadManager()` method to `OSMScoutClient.java`
- [x] 9.2 Add `withMapsDirectory(String)` method to `OSMScoutClientBuilder.java`
- [x] 9.3 Implement JNI native methods in `OSMScoutClient.cpp`:
  - `nativeParseMapList(json, provider)` → `List<AvailableMapEntry>`
  - `nativeGetMapFileNames()` → `String[]`
  - `nativePrepareMapDirectory(entry, targetDir)` → boolean
  - `nativeRegisterMapDirectory(targetDir)` → boolean
  - `nativeCancelDownload(handle)`
  - `nativeGetInstalledMaps()` → `List<String>`
  - `nativeDeleteMap(path)`

## 10. JavaScout — Config Extension

- [x] 10.1 Add `getMapProvider()` / `setMapProvider(String)` to `Config.java`
- [x] 10.2 Default maps directory: `getConfigDir().resolve("maps")` if not configured

## 11. JavaScout — MapDownloadManager (Java-side)

- [x] 11.1 Create `MapDownloadManager` Java class wrapping `OSMScoutClient` native download methods
- [x] 11.2 Implement `fetchAvailableMaps()` — performs HTTP in Java, calls `nativeParseMapList()` for JSON parsing
- [x] 11.3 Implement `downloadMap(entry, dir, listener)` — starts background thread, downloads all files in Java, then calls native prepare/register
- [x] 11.4 Implement `cancelDownload(handle)` — interrupts worker thread and closes active HTTP stream for immediate stop
- [x] 11.5 Implement `getInstalledMaps()` — returns list of installed `MapDirectory` objects
- [x] 11.6 Implement `deleteMap(path)` — deletes map directory from disk and `MapManager`
- [x] 11.7 Add `AvailableMapEntry.findEntryByName()` recursive helper

## 12. JavaScout — Map Download UI

- [x] 12.1 Create `MapDownloadDialog.fxml` — tree view for available maps, download queue table, installed maps list
- [x] 12.2 Create `MapDownloadController.java` — handles tree selection, download start/cancel, installed map deletion
- [x] 12.3 Wire dialog into `MainController` menu (e.g., "File → Download Maps")
- [x] 12.4 Add `--map-provider` CLI argument to `JavaScout.java`
- [x] 12.5 Expose JavaFX `StringProperty` getters on `DownloadEntry` so `PropertyValueFactory` observes live updates
- [x] 12.6 Throttle UI progress updates to ~4 Hz to avoid flooding JavaFX with 8 KB-chunk events
- [x] 12.7 Show "Starting" status immediately when a download is queued

## 13. Build System

- [x] 13.1 Update `libosmscout-client/CMakeLists.txt` — add new source/header files
- [x] 13.2 Update `libosmscout-client/meson.build` — add new source files
- [ ] 13.3 Update `libosmscout-client-qt/CMakeLists.txt` — add `QtHttpClient` source
- [ ] 13.4 Update `libosmscout-client-qt/meson.build` — add `QtHttpClient` source
- [x] 13.5 Update `libosmscout-client-java/java/meson.build` — add new Java files
- [x] 13.6 Update `libosmscout-client-java/src/meson.build` — remove `JniHttpClient.cpp` (HTTP handled in Java)

## 14. JavaScout UI Polish

- [x] 14.1 Move download dialog trigger from app menu bar to on-map context menu
- [x] 14.2 Collapse tree sections by default
- [x] 14.3 Show progress/status via observable `StringProperty` so `PropertyValueFactory` observes live updates
- [x] 14.4 Clamp in-progress percentage to 99%; only `onComplete()` sets 100%
- [x] 14.5 Report cancellation as `Cancelled` instead of `Error: Download cancelled`
- [x] 14.6 Show Installed maps by map name derived from full path, with full path kept for deletion
- [x] 14.7 Throttle `Platform.runLater` progress updates to ~4 Hz

## 15. Startup / Discovery Fixes

- [x] 15.1 Trigger `MapManager::LookupDatabases()` during `OSMScoutClient.cpp` build and wait for completion
- [x] 15.2 Keep `knownPaths` empty so the download parent directory is not opened as a database
- [x] 15.3 Pass both CLI maps directory and default download directory to `withMapLookupDirectories()`
- [x] 15.4 Remove `client.openDatabase()` call from `MainController.initClientAndRenderer()`
- [x] 15.5 Return full directory path from `nativeGetInstalledMaps()`; derive display name in Java controller
- [x] 15.6 Add scan logging to `MapManager::LookupDatabases()` (candidates, valid, invalid, per-directory summary)

## 16. Build / Delivery

- [x] 16.1 `JavaScout/build.sh` installs fresh `libosmscoutclientjava.jar` to `~/.m2/repository` before `mvn package`
- [x] 16.2 `JavaScout/README.md` documents Meson-first build flow
- [x] 16.3 `JavaScout/javascout.sh` error message points to `./build.sh`

## 17. Testing

- [x] 17.1 Unit test `AvailableMapEntry` JSON parsing with sample server response
- [x] 17.2 Unit test `MapDownloadService` static helpers (`MapFiles`, `PrepareMapDirectory`, `RegisterMapDirectory`)
- [x] 17.3 Unit test `MapManager::AddLookupDirectory()` thread safety
- [x] 17.4 Unit test `Settings` maps directory and provider ID persistence
- [x] 17.5 Console smoke test: download Albania to completion with progress events
- [x] 17.6 Console smoke test: cancel a running North Rhine-Westphalia download
- [ ] 17.7 Integration test: `MapDownloadService` + `QtHttpClient` against test server (needs Qt)
- [ ] 17.8 Java unit test: `MapDownloadManager` with mock native layer
