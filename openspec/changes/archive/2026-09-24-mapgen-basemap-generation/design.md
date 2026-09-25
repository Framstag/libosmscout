# Design

## Context

See `proposal.md` for motivation. Current state that shapes the approach:

- `scripts/mapgen/mapgen.sh` performs one pass per invocation: it validates `imports.json` +
  `names.json`, gates each import by a refresh interval, downloads a source and verifies it against
  a published `.md5`, runs `Import --typefile map.ost`, then stages the output into
  `public/<region-path>/v<typeConfigVersion>/` by per-file copy into a staging directory and a
  directory rename. Check state and generation records live in `private/admin/`.
- The image (`scripts/mapgen/Dockerfile`) builds target `Import` only, copies `stylesheets/*.ost`
  (so `basemap.ost` is already present, and it includes no modules), then runs as uid/gid 1000 with
  a read-only root filesystem. `BasemapImport` is already a target of the build system whenever
  `OSMSCOUT_BUILD_TOOL_IMPORT=ON` (`CMakeLists.txt:463-467`, `meson.build:632`) but is not built
  into the image.
- `db.json` is written by the Import tool only (`Import/src/DbJsonWriter.{h,cpp}`), is not part of
  the library, and has no reader. Per-file inventory entries carry CRC-32 (zlib-compatible),
  explicitly not POSIX `cksum`.
- The client side already has basemap capabilities. `BasemapManager.java` probes an HTML directory
  listing and downloads+extracts a `*.tar.gz`; `MapDownloadManager.java` downloads regional maps
  file by file from a fixed file list (`MapDirectory::MandatoryFiles()` +
  `OptionalFiles()`), writing a local `metadata.json` and registering the directory. That file list
  includes routing and index files that a basemap import may or may not produce.

## Goals / Non-Goals

**Goals:**

- One pass produces both regional databases and the world basemap, sharing the lock, the work area,
  and the failure reporting.
- The basemap is served exactly like a regional database (metadata + per-file download), so client
  code paths and integrity handling are the same kind of thing in both cases.
- Basemap regeneration frequency is governed by input content and by two independent intervals, so
  a source that changes daily does not force a daily basemap.
- A basemap placed once stays served until it is replaced atomically; a failing basemap step never
  damages the served repository.

**Non-Goals:**

- A second basemap variant (minimal/full). The `basemap-download` variant requirement is removed.
- Changing how regional imports work, or changing their water index source.
- Making the shapefile-derived coastline available to regional databases.
- Skipping `Import`'s own water-index generation (tracked as an open question, not implemented).

## Decisions

### D1: The basemap gets its own required configuration file

`/config/basemap.json`, schema-versioned, validated by the same code path that validates
`imports.json`. Its absence is a configuration failure of the pass.

Alternatives:
- *Extend `imports.json` with a `basemap` block* — rejected: `validate_config` derives validity from
  an exact match between manifest ids and `names.json` leaves; a basemap that is not a region would
  have to be exempted, weakening a rule that currently protects the region index from typos.
- *Optional file, basemap skipped when absent* — rejected by the operator: a deployment that forgets
  the file would silently serve no basemap, which is harder to notice than a configuration error.

Files: `scripts/mapgen/mapgen.sh` (validation, `--check-config`), new
`scripts/mapgen/basemap.example.json`, `scripts/mapgen/docker-compose.yml` (seed instructions),
`Documentation/MapRepository.md` §1.

### D2: Import tuning is an explicit whitelist, not passthrough arguments

`basemap.json` carries `importOptions` with named keys mapped to `Import` options
(`waterIndexMinMag`, `waterIndexMaxMag`, `lowZoomOptMaxMag`, `areaNodeGridMag`, `maxAdminLevel`,
`langOrder`, `altLangOrder`, `strictAreas`) and to `BasemapImport` options (`minIndexLevel`,
`maxIndexLevel`, `maxWaterDistance`). The pass rejects unknown keys, non-numeric values, and
inconsistent combinations (`waterIndexMinMag > waterIndexMaxMag`, `minIndexLevel > maxIndexLevel`,
values above the tool's limit).

Alternatives:
- *Fixed values in the image* — rejected by the operator, and it would make the image the only place
  to express audience-dependent settings such as `langOrder`.
- *A passthrough array of raw tool arguments* — rejected: `--destinationDirectory` and `--typefile`
  are decided by the pass, and a raw array would allow them to be overridden or duplicated. A
  whitelist keeps the pass in control of the contract while leaving the tuning open.

Files: `scripts/mapgen/mapgen-basemap.sh`, `scripts/mapgen/mapgen.sh`.

### D3: The planet export is mounted; the coastline is fetched

The planet export is named as a path inside the read-only configuration area. The coastline source
is a URL with an optional checksum, defaulting to the value compiled into the image.

Alternatives:
- *Both mounted* — rejected by the operator: the coastline dataset is published and updated by a
  third party, and mounting it makes the image's default meaningless and adds an operator chore.
- *Both downloaded* — rejected: a pre-filtered planet export is produced by the operator's own
  filtering step; there is no service that publishes it, and the pass would need credentials or a
  bespoke source configuration for it.

Files: `scripts/mapgen/mapgen-basemap.sh`, `scripts/mapgen/Dockerfile` (default URL as build
arguments, mirroring the existing `SUPERCRONIC_VERSION` pattern).

### D4: Content-based change detection with two cadences

The pass computes an md5 of the planet export and of the coastline archive, records them in
`private/admin/basemap_check.json` together with `lastCheckedAt` and `lastAdoptedAt`, and produces a
new basemap only when one of the two values differs from the one the served basemap was built from.
Two intervals in `basemap.json`: `refresh` (how often the inputs may be checked at all) and
`coastlinesRefresh` (how often a newer coastline copy may be adopted). Validation requires
`coastlinesRefresh >= refresh`.

Alternatives:
- *A single refresh interval* — rejected by the operator: with the upstream dataset changing daily,
  a single interval either checks rarely (stale extract detection) or adopts daily (needless
  basemap regeneration).
- *Timestamp comparison of the mounted extract* — rejected: timestamps change on copy, and the
  existing pipeline already treats content as the identity of an import (the published `.md5`).
- *An explicit operator-supplied version string* — rejected earlier in the exploration: it puts the
  burden of noticing an input change on the operator.

Hashing a multi-GB extract costs one full read per check; the check happens only when `refresh` has
elapsed, and the same md5 value is handed to `Import --source-md5`, so the file is read once for
both purposes. md5 here is content identity, not security, matching the existing justification
comment in `scripts/mapgen/mapgen.sh` (sonar `shell:S4790`).

Files: `scripts/mapgen/mapgen-basemap.sh`, `scripts/mapgen/mapgen.sh` (state-file helpers are
reusable).

### D5: Coastline verification is conditional-GET plus archive CRC, with an optional operator checksum

The upstream site publishes no checksums, updates roughly daily, and asks consumers to use
`If-Modified-Since` rather than re-download (verified on `osmdata.openstreetmap.de`, both the
dataset page and its downloading page). The pass therefore stores the archive and its
`Last-Modified` in the work area, sends a conditional request, and reuses the stored copy on a
"not modified" response. `unzip` validates each entry's CRC-32 during extraction, so a truncated or
corrupt download fails. A `sha256` in `basemap.json` is enforced when present, for operators who
mirror the archive.

Alternatives:
- *Pin a checksum in the image* — rejected: it goes stale within a day of upstream regeneration and
  would turn every pass into a verification failure.
- *Mirror the archive ourselves and pin it* — rejected: adds hosting and a refresh chore outside the
  image, for a dataset that is not security-sensitive.

Files: `scripts/mapgen/mapgen-basemap.sh`, `scripts/mapgen/Dockerfile` (add `unzip` to the runtime
stage).

### D6: `BasemapImport` owns `water.idx`, and it updates the metadata

`Import --typefile basemap.ost` runs first and produces the database plus `db.json`.
`BasemapImport --coastlines <unpacked .shp>` then writes `water.idx` into the same directory, and
appends that file to `db.json`'s inventory (size + CRC-32) so the client downloads and verifies it
like any other file. The Import output is cached in the work area keyed by the extract md5, so a
pass triggered by a coastline change only re-runs `BasemapImport` and the placement.

Alternatives:
- *Rely on `Import`'s own water index (`GenWaterIndex` over the extract's coastline ways)* —
  rejected by the operator; the upstream coastline dataset repairs OSM coastline errors and is the
  documented path in `webpage/content/tutorials/BasemapImporting.md`.
- *Have the pass add the water index entry with shell tooling* — rejected: the inventory uses
  zlib-compatible CRC-32, and POSIX `cksum` computes a different CRC (the existing pipeline warns
  about exactly this). `Import/src/DbJsonWriter.cpp` already computes it correctly.
- *Leave `water.idx` out of the inventory* — rejected: the client's fixed file list requires the
  file, and an unverified file would bypass the integrity model.

To let a second tool touch the metadata, the writer moves from the Import tool into the import
library and gains a reader: `Import/src/DbJsonWriter.{h,cpp}` and `Import/src/JsonWriter.{h,cpp}` →
`libosmscout-import/include/osmscoutimport/DbJson.h` + `src/osmscoutimport/DbJson.cpp` and
`libosmscout-import/include/osmscoutimport/JsonWriter.h` + `src/osmscoutimport/JsonWriter.cpp`, with
`ReadDbJson()` added beside `BuildDbJsonInventory()`/`WriteDbJson()`. The writer keeps emitting the
same layout, so nothing downstream of `db.json` changes; the reader is written with
`nlohmann_json`, which becomes a required dependency **of the import library** (see D12).
`BasemapImport` updates an existing `db.json` when it finds one and logs a note when it does not, so
the documented manual procedure (run `BasemapImport` into an otherwise empty directory) keeps
working.

Alternatives considered for the reader:
- *A hand-written tolerant reader in the library* — rejected by the operator: another parser to
  maintain in the core import library for a machine-generated file, when a maintained one exists.
- *Let `Import` own the coastline-derived water index, removing the need for a reader* — rejected by
  the operator, although it avoids the new dependency entirely; it moves the shapefile generator into
  the library and changes the responsibility split between the two tools.

Files: `Import/src/Import.cpp`, `Import/src/DbJsonWriter.*`, `Import/src/JsonWriter.*` (move),
`Import/CMakeLists.txt`, `Import/meson.build`, `libosmscout-import/CMakeLists.txt`,
`libosmscout-import/meson.build`, `BasemapImport/src/BasemapImport.cpp`,
`BasemapImport/CMakeLists.txt`, `BasemapImport/meson.build`, `Tests/src/DbJsonWriterTest.cpp`,
`Tests/src/JsonWriterTest.cpp`, `Tests/CMakeLists.txt`, `Tests/meson.build`.

### D7: The basemap is served as a version-keyed slot, like a regional database

`public/basemap/v<typeConfigVersion>/` holds `db.json` plus every data file, including `water.idx`.
Placement mirrors the regional staging: copy the intended files into `private/staging/basemap/`,
then replace the slot directory. Retention prunes whole slot directories, counted by the
`history` value in `basemap.json` (`0` keeps everything, as in the regional pipeline).

Alternatives:
- *A `tar.gz` per slot* — rejected by the operator: it would keep two delivery mechanisms
  (extract-and-unpack for the basemap, file-by-file for regions) and would leave the client with a
  second integrity model.
- *A flat directory with the version only in the manifest* — rejected by the operator: version-keyed
  slots are how regional databases already work, and they make retention and rollback symmetrical.

Files: `scripts/mapgen/mapgen-basemap.sh`, `scripts/mapgen/mapgen.sh` (staging/record helpers).

### D8: Availability is a generated manifest

The pass writes `public/basemap/index.json`, atomically, after a successful placement:
`{"schema":1,"versions":[{"typeConfigVersion":N,"changedAt":"<iso8601>"}]}`. The per-slot `db.json`
remains the authority for files, checksums, and generation time.

Alternatives:
- *nginx `autoindex on` plus a client date-parsing fix* — rejected: nginx's listing date format
  (`23-Feb-2026 00:16`) is not matched by the client's parser, which then sorts by file name
  ascending and reports the *oldest* archive as newest; relying on directory listings also means
  discovery breaks whenever the serving configuration changes.
- *`autoindex on` only* — rejected for the same reason; a manifest is explicit and testable.

Files: `scripts/mapgen/mapgen-basemap.sh`, `scripts/mapgen/nginx-serve.conf`.

### D9: The client downloads the basemap like a regional database

`BasemapManager` probes `{provider}/basemap/index.json`, chooses the newest version not above the
database format version the library supports, downloads `db.json` and then each file it names into
`{mapsDir}/basemap/`, keeps the metadata locally as the update-check base, verifies each file
against the checksum in the metadata, and registers the directory. Archive download and extraction
are removed.

Alternatives:
- *Keep the archive, add a manifest beside it* — rejected: the file list a client requires is fixed,
  so a database directory is sufficient, and one delivery path is less code than two.
- *Keep the HTML-listing fallback* — rejected: with a manifest there is nothing to fall back to, and
  a silent fallback to a format known to mis-sort dates is worse than a clear "unavailable".

Files: `libosmscout-client-java/java/com/framstag/libosmscout/client/BasemapManager.java`,
`MapDownloadManager.java`, `JavaScout/src/main/java/com/framstag/libosmscout/MapDownloadController.java`,
`libosmscout-client-java/src/test/.../BasemapManagerTest.java`.

### D10: The basemap step is a script of its own, inside the same pass and lock

`scripts/mapgen/mapgen-basemap.sh` holds the basemap logic; `mapgen.sh` calls it after the regional
loop, inside the same `flock` (`mapgen-pass.sh`), and folds its exit status into the pass status. A
basemap failure does not prevent regional imports from having been performed.

Alternatives:
- *Inline into `mapgen.sh`* — rejected: `mapgen.sh` is already 481 lines of one concern; the basemap
  adds inputs, cadence, placement, manifest, and retention.
- *A second container or a second pass* — rejected: it would need its own lock and its own trigger,
  and would reintroduce the ordering problem the single pass solves (the basemap manifest must be
  written after its slot, and the pass must not run two writers concurrently).

Files: `scripts/mapgen/mapgen.sh`, `scripts/mapgen/mapgen-pass.sh`, `scripts/mapgen/mapgen-entrypoint.sh`.

### D11: Serving configuration is explicit per path

`nginx-serve.conf` gains a location for `basemap/index.json` (`application/json`, short cache) and
one for `/basemap/v<N>/` (long cache), with no directory listings anywhere. The existing
`location /` behaviour stays as the catch-all 404.

Alternatives:
- *Rely on the catch-all* — rejected: `index.json` would be served without a content type and both
  the manifest and the slot files would fall under a `try_files $uri =404` path with no cache
  policy, which is how the discovery contract would quietly rot.

Files: `scripts/mapgen/nginx-serve.conf`.

### D12: `nlohmann_json` becomes a dependency of the import library, import-gated

`libosmscout-import` requires `nlohmann_json` (CMake `find_package(... REQUIRED)` in
`libosmscout-import/CMakeLists.txt`, `dependency('nlohmann_json', required: true)` in
`libosmscout-import/meson.build`). Because the requirement sits in the import library's own build
description, it only applies where that library is built: iOS, Android NDK and JavaScout configure
with `OSMSCOUT_BUILD_IMPORT=OFF` and are unaffected.

Alternatives:
- *Hand-written reader* — rejected by the operator (D6).
- *`Import` owns the water index* — rejected by the operator (D6).
- *Require `nlohmann_json` at the top level for every build* — rejected: it would extend the
  dependency to builds that never touch the import library, for no benefit.

The dependency has to be added to every configuration that builds the import library:
`scripts/mapgen/Dockerfile` (build stage), `conanfile.py`, the three `vcpkg_*.json` profiles, the
`ci/docker/*` images that build it, and the workflows that build it (Ubuntu, macOS, MSYS, sanitizer,
SonarQube, VS2025). `cmake/features.cmake` keeps its optional lookup, because MCPServer continues to
build only when the package is present.

Files: `libosmscout-import/CMakeLists.txt`, `libosmscout-import/meson.build`, `conanfile.py`,
`vcpkg_full.json`, `vcpkg_medium.json`, `vcpkg_minimum.json`, `ci/docker/*/Dockerfile`,
`.github/workflows/*.yml`, `scripts/mapgen/Dockerfile`.

## Implementation notes

Decisions taken while implementing, recorded here because the design left them open:

- **The manifest is written after pruning, not before.** The design says the manifest is written
after a placement; what it must not do is name a version that is already gone. Pruning therefore
runs before the manifest is rewritten, and the shell contract test asserts that the manifest names
exactly the versions that are served.
- **The metadata append is a library function.** `AddDbJsonInventoryEntry()` in the shared metadata
helper adds one file to the inventory of an existing metadata file, replacing an entry that is
already there. `BasemapImport` calls it after writing the water index; the behaviour (checksum
matches the file, other fields untouched, no metadata means nothing to do) is unit tested, and the
real tool was verified against a generated shapefile.
- **The adopted coastline copy and the fetched copy are separate files.** A conditional request
compares against the newest *fetched* archive, while production uses the *adopted* one, so a pass
that finds newer coastline data before the adoption interval elapses keeps producing from the
adopted copy - and a later pass is still told "unchanged" without a transfer.
- **"Not modified" includes an empty transfer.** A source without last-modified information (a
local file, for instance) answers a conditional request with no body and no 304, so an empty or
absent download is treated as "unchanged" as well.
- **The configured options are mapped to tools explicitly.** The import tool receives only the
options it knows, the water index tool only its own. The first end-to-end run with the real tools
caught this: sending the water index options to the import tool makes it refuse to run at all. The
shell contract test's stub tools now reject unknown options for the same reason.

- **The work area keeps one basemap import output.** The import output cached by extract hash is
what makes a coastline-only change cheap, but stale caches of earlier extracts are removed after a
successful placement, so the work area does not grow with the number of planet exports; the shell
contract test asserts that exactly one cache survives.

## Sequence diagrams

Pass, basemap portion (after the regional loop, same lock):

```
mapgen.sh          mapgen-basemap.sh        work area        repository
   |                      |                     |                 |
   |-- invoke ----------->|                     |                 |
   |                      |-- read basemap.json |                 |
   |                      |-- validate ---------+                 |
   |                      |                     |                 |
   |                      |-- refresh elapsed? -+-- no --> exit 0  |
   |                      |                     |                 |
   |                      |-- md5(extract) ----->|                 |
   |                      |-- conditional GET -->| (zip+Last-Mod) |
   |                      |   304 -> reuse       |                 |
   |                      |   200 -> verify, store                 |
   |                      |                     |                 |
   |                      |-- adoption due? ----+-- no --> record, exit
   |                      |                     |                 |
   |                      |-- Import (cached by extract md5) ----->| (work area)
   |                      |-- unzip zip ------->|                 |
   |                      |-- BasemapImport ---->| water.idx + db.json update
   |                      |-- stage copy ------>|                 |
   |                      |-- atomic slot replace ----------------->| public/basemap/v<N>/
   |                      |-- write index.json (tmp+mv) ----------->| public/basemap/index.json
   |                      |-- prune slots (history) --------------->| remove v<old>
   |                      |-- records --------->|                 |
   |<-- status -----------|                     |                 |
```

Client download:

```
MapDownloadController   BasemapManager        provider            local maps dir
       |                     |                    |                    |
       |-- fetchAvailable -->|                    |                    |
       |                     |-- GET index.json ->|                    |
       |                     |<-- versions[] -----|                    |
       |                     |-- pick newest <= FILE_FORMAT_VERSION    |
       |<-- archive list ----|                    |                    |
       |-- download -------->|                    |                    |
       |                     |-- GET v<N>/db.json>|                    |
       |                     |-- keep metadata ---+------------------->| basemap/db.json
       |                     |-- GET each file -->|                    |
       |                     |-- verify checksum -+------------------->| basemap/<file>
       |                     |-- register ------->| native register    |
```

## Risks / Trade-offs

- **[A basemap slot must contain every file `MapDirectory::MandatoryFiles()` names, but a
  `basemap.ost` import may skip files that regions always produce (routing, intersections, route
  nodes)]** → Mitigation: the pass verifies the produced directory against the required file set
  before placement and fails loudly, naming the missing file. A task must first confirm empirically
  what `Import` produces for this type file; if files are genuinely absent, the option is to make
  `BasemapImport` create the missing inventory entries only for files that exist, and to reduce the
  client's required set for basemaps — the second is a client contract change and needs its own
  decision.
- **[`Import`'s own water-index generation is wasted work because `BasemapImport` overwrites
  `water.idx`]** → Mitigation: measure first; the cached-Import-output rule means it is paid only
  when the extract changes. A future option to skip that step is an open question.
- **[The database format version the client compares against is a literal `"27"` in
  `MapDownloadManager.java`]** → Mitigation: the basemap probe must use the same value the regional
  listing uses; a task exposes the library's version through one place instead of adding a second
  literal.
- **[Hashing a multi-GB extract on every check]** → Mitigation: the check is gated by `refresh`
  (default days), and the same md5 serves as `--source-md5`; the work-area cache avoids re-import.
- **[`unzip`'s CRC-32 is integrity, not authenticity]** → Mitigation: documented; the optional
  operator `sha256` is the supported route for deployments that need verification of origin.
- **[Basemap staleness by design: the coastline source changes daily, adoption defaults to a much
  longer interval]** → Mitigation: the pass reports available-but-not-adopted newer data; the
  interval is configuration, so a deployment that wants fresh coastlines can shorten it and pay with
  more regenerations.
- **[Rewriting `db.json` after `Import` produced it]** → Mitigation: reuse the existing inventory
  code (same CRC), write atomically as `WriteDbJson` already does, and cover it with a unit test.
- **[Moving the db.json writer into the library touches several build files]** → Mitigation:
  the move is mechanical; both build systems are updated in the same task, and the existing
  `Tests/src/DbJsonWriterTest.cpp` guards the behaviour.
- **[`nlohmann_json` becomes a hard dependency of a core library]** → Mitigation: the requirement is
  declared in the import library itself, so it reaches exactly the configurations that build that
  library (D12 lists them); iOS, Android and JavaScout builds are unaffected because they configure
  the import library off. Every affected configuration is updated in the same group of tasks, and a
  build of each is the acceptance check.

## Migration Plan

1. Move + extend the db.json helper, then teach `BasemapImport` to update metadata; unit-test.
2. Add `basemap.json` validation and the basemap step (with the file-set verification), still
   without client changes; verify with a small extract that the served slot is complete.
3. Extend the image (tool, `unzip`, default coastline URL) and the serve configuration; smoke tests
   cover the config check and the tool's presence.
4. Switch the client: manifest-based discovery, file-by-file download, local metadata, checksum
   verification; remove archive extraction.
5. Update `Documentation/MapRepository.md` and the tutorial pointer.

Rollback: steps 1-3 are additive (no basemap served unless `basemap.json` exists, and the manifest
is a new path), so a deployment can revert the image to a previous tag while the client keeps
working against a repository that has no basemap. Step 4 is the point of no return for the client;
it must ship after the generating side has produced a basemap, or the client reports the basemap as
unavailable rather than failing.

## Open Questions

- Whether `Import` should gain a way to skip its water-index generation for basemap runs (saves one
  pass over coastline ways); no spec depends on the answer.
- Whether the upstream coastline source's newer availability should surface anywhere besides the
  pass log (for example an operator-visible note in the manifest); the manifest contract is
  currently additive-only, so this can wait.
- Whether a second basemap variant will ever be produced; the removed requirement documents the
  migration if it returns.
