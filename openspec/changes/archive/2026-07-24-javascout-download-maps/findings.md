# JavaScout Map Download — Findings

This document records the key technical findings discovered while implementing map download support for JavaScout.

## 1. OpenJDK 17.0.2 crashes when `java.net.http.HttpClient` is called from JNI

### Symptom

`SIGSEGV` inside `libjvm.so` at `G1BarrierSet::AccessBarrier::oop_access_barrier`, with the crashing thread named `map-download-...`.

### Reproduction

- Create `java.net.http.HttpClient` in Java.
- Pass it into JNI/native code.
- Call `HttpClient.send()` or `sendAsync()` from the native side.
- The crash reproduces reliably on OpenJDK 17.0.2 with the G1 collector.

### Attempted mitigations

- Creating the `HttpClient` on a Java thread and passing it to native: still crashed.
- Using `AttachCurrentThread` / `JniEnvAttacher`: did not help.
- The crash is in GC read barriers, so GC-specific flags (e.g. `-XX:-UseG1GC`) were considered but rejected because the code must work under all GC options.

### Resolution

Move **all** HTTP execution to Java threads. The JNI layer only performs non-HTTP work:

| JNI method | Responsibility |
|------------|----------------|
| `nativeParseMapList` | Parse provider JSON into Java `AvailableMapEntry` tree |
| `nativeGetMapFileNames` | Return ordered list of map database file names |
| `nativePrepareMapDirectory` | Create target dir, clean partial files, write `metadata.json` |
| `nativeRegisterMapDirectory` | Add target dir to `MapManager` lookup dirs and rescan |
| `nativeGetInstalledMaps` | Return display names of discovered installed maps |
| `nativeDeleteMap` | Delete a map directory and rescan |

`JniHttpClient.cpp` and `JniHttpClient.h` were removed entirely.

---

## 2. `MapManager::LookupDatabases()` is not triggered automatically

### Symptom

After restarting JavaScout, downloaded maps in `~/.config/javascout/maps/` were not listed in the Installed Maps tab.

### Root cause

`MapManager` stores lookup directories at construction but does not run a scan until something calls `LookupDatabases()`. The Qt client triggers this from `InstalledMapsModel` and `MapDownloader`; the JNI client did not.

### Resolution

`OSMScoutClient.cpp` build now calls `mapManager->LookupDatabases()` after `DBThread` initialization and waits for the future to complete before returning the Java `OSMScoutClient`.

```cpp
clientData->dbThread->Initialize();
auto lookupFuture = clientData->mapManager->LookupDatabases();
lookupFuture.StdFuture().wait();
```

This guarantees that `databaseListChanged` has fired and `DBThread` has opened all discovered map databases before the UI appears.

---

## 3. Passing the download parent directory to `DBThread` fails

### Symptom

Console showed:

```
File '/home/tim/.config/javascout/maps/types.dat' - Cannot open file for reading
Cannot open db '/home/tim/.config/javascout/maps'
```

### Root cause

`MainController` called `client.openDatabase(databaseDirectory)`, which passed the raw lookup directory (including `~/.config/javascout/maps/`) directly to `DBThread`. `DBThread` tried to open the parent directory as a single database, but only the map subdirectories contain valid databases.

### Resolution

- `OSMScoutClient.cpp` initializes `clientData->knownPaths` to an empty vector. `openDatabase()` should only track explicitly opened map directories, not the parent download folder.
- `MainController` no longer calls `client.openDatabase(databaseDirectory)` at startup. Instead, it passes both the CLI/config maps directory and the default download directory to `OSMScoutClientBuilder.withMapLookupDirectories()`. `MapManager` discovers the actual database subdirectories recursively and emits them to `DBThread`.

---

## 4. Stale `libosmscoutclientjava.jar` causes runtime `UnsatisfiedLinkError`

### Symptom

```
UnsatisfiedLinkError: 'boolean com.framstag.libosmscout.client.MapDownloadManager.nativeDownloadMap(...)'
```

### Root cause

JavaScout's `pom.xml` depends on `libosmscout-client-java` via Maven. The jar in `~/.m2/repository` was older than the freshly compiled native library, so native method signatures no longer matched.

### Resolution

`JavaScout/build.sh` auto-locates the Meson-built `libosmscoutclientjava.jar` and installs it into the local Maven repository before running `mvn package`. This ensures the shaded fat jar always contains the matching client classes.

---

## 5. JavaFX `PropertyValueFactory` needs observable properties

### Symptom

Download progress/status in the JavaFX table never updated; the row stayed at the initial value.

### Root cause

`PropertyValueFactory` only observes changes when the model object exposes JavaFX property methods (`xxxProperty()`). Plain getters return a static snapshot.

### Resolution

`MapDownloadController.DownloadEntry` was updated to use `SimpleStringProperty` for `mapName`, `targetDir`, `progress`, and `status`, with matching `xxxProperty()` accessors.

---

## 6. Cancellation must unblock blocking I/O

### Symptom

Pressing Cancel did nothing; the download continued because `Thread.interrupt()` cannot interrupt a blocking `InputStream.read()`.

### Resolution

`MapDownloadManager` stores the active HTTP `InputStream` in `ActiveDownload`. `cancelDownload()`:

1. Sets the `cancelled` flag.
2. Interrupts the worker thread.
3. Closes the active stream, which forces `read()` to return with an exception.

The worker distinguishes cancellation from failure and reports `"Download cancelled"`, which the UI renders as `Cancelled`.

---

## 7. Progress reaches 99% while status says Complete

### Symptom

UI showed `99%` / `Complete` at the end of a download.

### Root cause

Integer truncation of `bytesDownloaded * 100 / totalBytes` can yield 99 just before the final `onComplete()` callback. The progress callback also ran after every 8 KB chunk, so the last update could arrive before the completion callback.

### Resolution

- Clamp in-progress percentage to a maximum of 99%.
- Only the `onComplete()` callback sets progress to 100% and status to `Complete`.

---

## 8. Installed-list deletion requires the full directory path

### Symptom

Selecting an installed map and pressing Delete did nothing.

### Root cause

`nativeGetInstalledMaps()` was changed to return display names (derived from `MapDirectory::GetName()`). The Java controller stored the display name in `installedPaths` and passed it to `deleteMap()`. `deleteMap()` needs the full filesystem path, so it received a basename and failed silently.

### Resolution

- `nativeGetInstalledMaps()` returns `MapDirectory::GetDirStr()` (full path).
- The Java controller derives the display name from the path filename and keeps the full path as the deletion key.
- `onDeleteMap()` checks the `deleteMap()` result, removes the list item immediately on success, and shows an error dialog on failure.

## 9. `MapDirectory` metadata may have an empty name

### Symptom

Installed Maps list showed a row with an empty label.

### Root cause

`MapDirectory::GetName()` returns the `name` field parsed from `metadata.json`. If that field is missing or empty, the display derived from it was empty.

### Resolution

The Java controller now derives the display name from the full path returned by `nativeGetInstalledMaps()` (`Paths.get(dir).getFileName()`), so an empty metadata `name` no longer produces an empty UI label. `MapManager::LookupDatabases()` also logs candidate, valid, and invalid counts per directory to help diagnose metadata issues.

---

## 10. Pre-existing teardown race in `MapManager`

### Symptom

During `client.close()` a warning appears:

```
Error iterating directory /tmp/.../maps/Albania: std::bad_alloc
```

### Status

This is a pre-existing race between `MapManager::LookupDatabases()` and client teardown. It was observed before the download feature was added and is not caused by the Java-side HTTP path. It remains a known issue for future investigation.
