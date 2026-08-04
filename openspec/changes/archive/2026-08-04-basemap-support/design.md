## Context

JavaScout uses `OSMScoutClientBuilder` to create the C++ client. The builder already exposes `withBasemapLookupDirectory()` which passes the path to `DBThread` for overlay rendering. The `MapDownloadManager` handles HTTP downloads in Java (to avoid JVM crashes from native HTTP calls). The basemap is a special world-wide map on the provider's server at a well-known path, not listed in the standard JSON map listing.

See `proposal.md` for motivation and capability overview.

## Goals / Non-Goals

**Goals:**
- JavaScout can discover, download, and load the world basemap
- Basemap renders as an overlay underneath regional maps
- User can see basemap status and trigger download/update from UI
- All HTTP operations stay in Java (existing pattern, avoids JVM crash)

**Non-Goals:**
- No changes to the C++ core library or JNI bridge (already sufficient)
- No changes to the map provider's server or listing format
- No automatic basemap download on first launch (user-initiated only)
- No basemap creation tooling (BasemapImport already exists)

## Decisions

### Decision 1: Probe basemap via HTTP directory listing

**Chosen:** Fetch `{provider.uri}/basemap/` and parse the HTML directory listing to find tar.gz archives. The actual server URL is `https://osmscout.karry.cz/basemap/`.

**Alternatives considered:**
- **Extend server JSON listing** — Would require server changes, not under our control.
- **Hardcode basemap path in provider config** — Chosen approach. Simple, no server changes. The path `/basemap/` is a convention on the karry.cz server.

### Decision 2: Download tar.gz archive, extract to `{mapsDir}/basemap/`

**Chosen:** Download the tar.gz archive, then extract it into `{mapsDir}/basemap/`. The archive contains the actual map database files (same format as regional maps).

**Alternatives considered:**
- **Download individual files** — Server only provides archives, not individual files.
- **Use archive directly** — Map database needs extracted files. Must extract.
- **Separate config key** — Adds complexity, no benefit. The basemap is a single well-known entity.

### Decision 3: Basemap download uses Java HTTP + tar extraction

**Chosen:** Download the tar.gz archive using `java.net.http.HttpClient` (same pattern as regional maps), then extract using Java's `GZIPInputStream` + `TarArchiveInputStream` (Apache Commons Compress or manual tar parsing).

**Alternatives considered:**
- **Reuse MapDownloadManager directly** — Regional maps download individual files from a server directory. Basemap is a single archive. Different enough to warrant a separate method.
- **Native C++ download + extraction** — Would trigger the OpenJDK 17.0.2 JVM crash. Also adds native tar parsing complexity.

### Decision 4: Basemap loaded at startup via builder, reloaded via DBThread

**Chosen:** Pass basemap dir to `withBasemapLookupDirectory()` at build time. After download/update, call `client.openDatabase(basemapPath)` to trigger `OnDatabaseListChanged`.

**Alternatives considered:**
- **Recreate client** — Expensive, tears down all state. The existing `OnDatabaseListChanged` mechanism handles runtime path changes.
- **File watcher** — Overengineered for a one-time load.

## Risks / Trade-offs

- **Server path changes** → If the provider moves the basemap, probing fails silently. Mitigation: log the probe URL and failure for debugging; user sees "Basemap unavailable".
- **Archive extraction failure** → Corrupted download or unsupported archive format. Mitigation: verify archive integrity (gzip CRC) before extraction; clean up on failure.
- **Large archive memory usage** → Full basemap is 39MB compressed, extraction needs streaming. Mitigation: stream decompression, don't load entire archive into memory.
- **Basemap download size** → The world basemap could be large. Mitigation: show progress via existing `MapDownloadListener`; support cancellation.
- **JVM crash on native HTTP** → Already mitigated: all HTTP stays in Java. The basemap probe and download use `java.net.http.HttpClient`.
- **Basemap overlay conflicts** → If a regional map has different coastline data, rendering may show double borders at low zoom. Mitigation: this is expected — the basemap is a fallback for areas without regional maps.
