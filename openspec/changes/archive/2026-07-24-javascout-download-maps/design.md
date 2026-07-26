## Context

JavaScout (JavaFX) needs map download capability. Currently only OSMScout2 (Qt) has it, via `libosmscout-client-qt`'s `MapDownloader`/`FileDownloader`/`AvailableMapsModel` — all Qt-dependent. JavaScout uses `libosmscout-client-java` (JNI) which wraps `libosmscout-client` (C++ core) and has no Qt dependency.

The C++ core (`libosmscout-client`) already has:
- `MapProvider` — provider metadata (URI, list URI)
- `MapDirectory` — local map metadata (name, path, version, validity)
- `MapManager` — scans lookup dirs, maintains `databaseDirectories` list
- `Settings` — persists config, loads providers from JSON
- `DBThread` — manages open DB instances, picks up `MapManager` changes

Missing: HTTP fetching (list available maps, download files), download orchestration, Java bindings for all of the above.

## Goals / Non-Goals

**Goals:**
- Abstract `HttpClient` interface in `libosmscout-client` for consumers that want a C++ HTTP abstraction
- `MapDownloadService` in `libosmscout-client` — helpers for map directory preparation, metadata writing, and registration (the Java path does not use it for HTTP)
- `MapManager::AddLookupDirectory()` — runtime registration of download target dir
- `Settings` extensions — persist download directory and selected provider
- JNI bridge in `libosmscout-client-java` — expose non-HTTP download helpers to Java
- Java-side HTTP in `libosmscout-client-java` — `MapDownloadManager` uses `java.net.http.HttpClient` on Java threads
- JavaScout UI — browse, download, manage maps
- Default download dir: `~/.config/javascout/maps/` (Linux), configurable
- Startup discovery of downloaded maps — `MapManager::LookupDatabases()` triggered during client build

**Non-Goals:**
- Replace Qt `MapDownloader`/`FileDownloader` in `libosmscout-client-qt` — stays unchanged
- Add HTTP to C++ core via new library dependency — zero new C++ deps
- Resume interrupted downloads (partial file support) — deferred, use `.download` temp suffix
- Map update detection / auto-update — user manually re-downloads
- Multiple map providers simultaneously in UI — first provider only, extendable
- Calling `java.net.http.HttpClient` from JNI/native code — explicitly avoided due to JVM crash

## Decisions

### Decision 1: Abstract `HttpClient` interface over concrete HTTP library

**Chosen:** Pure virtual `HttpClient` interface in `libosmscout-client`.

```cpp
class HttpClient {
public:
  virtual ~HttpClient() = default;

  /// Fetch URL body as string. Returns empty on error.
  virtual std::string Fetch(const std::string &url) = 0;

  /// Download URL to file. Reports progress via callback.
  /// Returns true on success.
  virtual bool Download(const std::string &url,
                        const std::filesystem::path &dest,
                        ProgressCallback progress) = 0;
};
```

**Alternatives considered:**
- **libcurl**: Battle-tested, portable, but new dependency for core library. Meson wrap exists but adds build complexity.
- **Platform APIs** (WinHTTP, CFNetwork, libsoup): More code, platform #ifdefs, each has different semantics.
- **cpp-httplib**: Header-only but pulls in OpenSSL. Single-threaded design.

**Rationale:** Zero new C++ deps. Each consumer uses its existing HTTP stack. JavaScout uses Java stdlib (Java 11+). OSMScout2 uses Qt. Test code uses stub/mock.

### Decision 2: Java-side HTTP, JNI for metadata/registration only

**Chosen:** `MapDownloadManager` performs all HTTP in Java on a background thread. The JNI layer only handles directory preparation, metadata writing, and map registration.

```
Java MapDownloadManager
  → java.net.http.HttpClient (created on Java thread)
    → send()/read InputStream
    → write .download files
    → rename to final files
  → JNI: nativePrepareMapDirectory(entry, targetDir)
  → JNI: nativeRegisterMapDirectory(targetDir)
```

**Rationale:** During testing, OpenJDK 17.0.2 crashed inside G1 read barriers whenever `java.net.http.HttpClient` methods were invoked from JNI/native code. Moving HTTP entirely to Java threads avoids the JVM bug while still reusing the core C++ logic for filesystem/metadata work.

**Risk:** Cancellation must interrupt both the Java worker thread and any blocking HTTP `InputStream::read`. Solved by storing the active stream and closing it from `cancelDownload()`.

### Decision 3: `MapDownloadService` is a helper, not the Java download orchestrator

**Chosen:** `MapDownloadService` keeps its `AsyncWorker`-based API for C++ consumers but is not used for HTTP on the Java path. Instead, it exposes static helpers:
- `MapFiles()` — ordered list of map database file names (mandatory + optional)
- `PrepareMapDirectory(entry, targetDir)` — create directory, clean partial files, write `metadata.json`
- `RegisterMapDirectory(targetDir, mapManager)` — add the directory to `MapManager` lookup dirs and trigger a rescan

**Rationale:** The Java path must avoid calling `java.net.http.HttpClient` from JNI. `MapDownloadManager` therefore runs HTTP in Java and calls the static helpers via JNI. The helpers still reuse the existing `MapDirectory` / `AvailableMapEntry` logic and metadata format.

### Decision 4: `MapManager::AddLookupDirectory()` with mutex

**Chosen:** Add `AddLookupDirectory()` that appends to a new `mutable` vector protected by mutex, then triggers `LookupDatabases()`.

```cpp
class MapManager {
  std::vector<std::filesystem::path> databaseLookupDirs;
  std::mutex lookupMutex;
public:
  void AddLookupDirectory(const std::filesystem::path &dir);
};
```

**Rationale:** Download target dir is unknown at construction time. Must be added after download completes. Mutex protects concurrent access from `DBThread` and download thread.

### Decision 5: Download file layout

**Chosen:** Each map downloads into its own subdirectory under the configured maps dir. Files are written with `.download` suffix during transfer, renamed on completion. Metadata JSON written alongside.

```
<maps-dir>/
  europe/
    germany/
      germany.osmscout/
        metadata.json
        types.dat
        areas.dat
        ...
    czech-republic/
      ...
```

**Rationale:** Matches existing `MapDirectory` expectations. The `.download` suffix prevents `MapManager::LookupDatabases()` from picking up incomplete downloads. Metadata JSON is written last as the signal of completion.

### Decision 6: JavaScout UI architecture

**Chosen:** Separate `MapDownloadController` + FXML dialog, not embedded in `MainController`.

```
MapDownloadDialog (FXML)
  → MapDownloadController (JavaFX Controller)
    → MapDownloadManager (Java, HTTP + JNI bridge)
      → OSMScoutClient native methods
        → MapDownloadService helpers (C++)
```

**Rationale:** Keeps `MainController` manageable. Dialog is self-contained. `MapDownloadManager` is a Java wrapper that can be unit-tested independently.

### Decision 7: Startup map discovery

**Chosen:** `OSMScoutClient.cpp` build triggers `MapManager::LookupDatabases()` and waits for it before returning the Java `OSMScoutClient`. The configured lookup directories include both the CLI/config maps directory and the default download directory (`~/.config/javascout/maps/`). `OSMScoutClient.openDatabase()` is no longer called at startup; `MapManager` discovers the actual database subdirectories and `DBThread` opens them via the `databaseListChanged` signal.

**Rationale:** `MapManager` does not scan automatically on construction, so downloaded maps were invisible after restart. Explicitly triggering and waiting for the scan ensures the Installed Maps list is populated immediately. Keeping `knownPaths` empty prevents `openDatabase()` from trying to open the raw parent download directory as a database.

## Sequence: Map Download Flow

```
User                  MapDownloadDialog    MapDownloadController    MapDownloadManager    JNI/OSMScoutClient    MapDownloadService    java.net.http.HttpClient
 |                           |                       |                     |                     |                     |                         |
 |-- select map, Download -->|                       |                     |                     |                     |                         |
 |                            |-- downloadMap() ---->|                     |                     |                     |                         |
 |                            |                       |-- start Java thread |                     |                     |                         |
 |                            |                       |-- onProgress(0,total)|                    |                     |                         |
 |                            |<-- "Starting" status |                     |                     |                     |                         |
 |                            |                       |-- nativePrepareMapDirectory() ---------->|                     |                         |
 |                            |                       |                     |                     |-- PrepareMapDirectory()|                         |
 |                            |                       |<-- ok --------------|<-- return ---------|                         |                         |
 |                            |                       |-- for each file:    |                     |                     |                         |
 |                            |                       |   HttpClient.send() |                     |                     |                         |
 |                            |                       |   read InputStream  |                     |                     |                         |
 |                            |                       |   write .download   |                     |                     |                         |
 |                            |                       |   onProgress() ---->|                     |                     |                         |
 |                            |<-- progress update --|                     |                     |                     |                         |
 |                            |                       |   rename → final    |                     |                     |                         |
 |                            |                       |-- nativeRegisterMapDirectory() -------->|                     |                         |
 |                            |                       |                     |                     |-- RegisterMapDirectory()|                         |
 |                            |                       |                     |                     |   AddLookupDirectory()  |                         |
 |                            |                       |                     |                     |   LookupDatabases()     |                         |
 |                            |                       |<-- ok --------------|<-- return ---------|                         |                         |
 |                            |                       |-- onComplete() ---->|                     |                     |                         |
 |                            |<-- "Complete" / 100%|                     |                     |                     |                         |
 |-- sees map in renderer --->|                       |                     |                     |                     |                         |
```

## Sequence: Map List Fetch Flow

```
User                  MapDownloadDialog    MapDownloadController    MapDownloadManager    JNI/OSMScoutClient    MapDownloadService    java.net.http.HttpClient
 |                           |                       |                     |                     |                     |                         |
 |-- open dialog ----------->|                       |                     |                     |                     |                         |
 |                            |-- fetchMaps() ------> |                     |                     |                     |                         |
 |                            |                       |-- HttpClient.send() |                     |                     |                         |
 |                            |                       |                     |                     |                     |                         |-- HTTP GET /latest.php
 |                            |                       |                     |                     |                     |                         |-- JSON body
 |                            |                       |-- nativeParseMapList(json) ------------->|                     |                         |
 |                            |                       |                     |                     |-- AvailableMapEntry::FromJsonArray()          |                         |
 |                            |                       |<-- entries[] -------|<-- return ---------|                     |                         |
 |                            |<-- populate tree -----|                     |                     |                     |                         |
 |-- sees available maps ---->|                       |                     |                     |                     |                         |
```

## Sequence: Startup Map Discovery Flow

```
JavaScoutApp            MainController          OSMScoutClientBuilder    OSMScoutClient.cpp       MapManager           DBThread
   |                         |                          |                        |                     |                      |
   |-- launch --------------->|                          |                        |                     |                      |
   |                         |-- withMapLookupDirectories(dbDir, downloadDir) ---->|                        |                     |                      |
   |                         |-- builder.build() ------>|                        |                     |                      |
   |                         |                          |-- create MapManager(dbDir, downloadDir)     |                      |
   |                         |                          |-- create DBThread() ->|                     |                      |
   |                         |                          |-- dbThread->Initialize()                   |                      |
   |                         |                          |-- mapManager->LookupDatabases()            |                      |
   |                         |                          |   (wait for scan)     |-- recursive scan --|                      |
   |                         |                          |                        |   emit databaseListChanged             |
   |                         |                          |                        |                     |-- open discovered DBs  |
   |                         |<-- client ---------------|<-- return ----------------------------------|                      |
   |                         |-- show dialog, Installed tab reads getInstalledMaps()                   |                      |
   |<-- maps listed ---------|                          |                        |                     |                      |
```

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| OpenJDK 17.0.2 crash when `java.net.http.HttpClient` is called from JNI | All HTTP moved to Java threads; JNI only handles filesystem/metadata/registration. |
| Java `HttpClient` response on wrong thread | `MapDownloadListener` callbacks are posted to JavaFX via `Platform.runLater()`. Progress throttled to ~4 Hz. |
| Partial download on crash | `.download` suffix prevents pickup. On restart, stale `.download` files cleaned during `LookupDatabases()`. |
| Large map download blocks UI | File copy runs on a Java background thread. Cancellation interrupts the thread and closes the active HTTP `InputStream`. |
| `MapManager::AddLookupDirectory()` races with `DBThread` | Mutex-guarded. `LookupDatabases()` called after unlock. |
| Downloaded maps invisible after restart | `OSMScoutClient.cpp` build triggers `MapManager::LookupDatabases()` and waits for completion. |
| Raw download parent opened as a database | `knownPaths` starts empty; `MapManager` discovers actual subdirectories and emits them to `DBThread`. |
| Installed-list deletion must use full path | `nativeGetInstalledMaps()` returns `MapDirectory::GetDirStr()`. Controller derives display name and keeps path as deletion key. |
| Empty map labels in Installed list | Java controller derives display name from the full path returned by `nativeGetInstalledMaps()`. |
| Qt `MapDownloader` diverges from new helpers | Both use same `MapDirectory`/`MapManager` types. Qt path is unchanged. |

## Open Questions

1. **Should the Java path use `HttpClient.send()` or `sendAsync()`?** Currently `send()` is used for simplicity; `sendAsync()` would give cancellable futures without closing the stream manually. Deferred.

2. **How to handle HTTP redirects?** Java's `HttpClient` follows redirects by default (`Redirect.NORMAL`). No extra code needed.

3. **Should `MapDownloadService` support multiple simultaneous downloads on the Java path?** Java `MapDownloadManager` starts one thread per `downloadMap()` call; concurrency is unbounded. A thread pool could be added later.

4. **How to report download errors to JavaScout UI?** `MapDownloadListener.onError(name, message)`. Cancellation is reported as `"Download cancelled"` and shown as `Cancelled` rather than `Error: ...`.

5. **Should we validate downloaded map integrity?** Not implemented. File sizes from the server are used only for progress; no checksum validation.
