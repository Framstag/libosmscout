# Proposal

## Why

The map repository pipeline regenerates regional databases, but not the world basemap that
clients already know how to discover, download, and render as an overlay. Because the basemap
is produced outside the pipeline, the served repository cannot offer one, and the discovery,
download, and UI capabilities that were built for it have nothing to point at.

## What Changes

- The regeneration pass gains a basemap step: it produces a world basemap database and serves it
  from the same repository, under its own version-keyed slot directories, alongside the regional
  databases.
- The basemap step is configured by its own required configuration file. The existing imports
  manifest and region index keep their current shape and meaning; the basemap is not a region
  and is not listed as one.
- Basemap inputs: a pre-filtered planet export is supplied through the configuration; the world
  coastline data is fetched from a fixed upstream source, checked cheaply for changes, and only
  adopted into a new basemap when a separate, slower adoption interval has elapsed.
- The served basemap carries the same metadata shape as a regional database, including a
  database format version and a per-file checksum set, and is delivered to clients file by file
  like a regional database. **BREAKING** for the client-side basemap distribution contract: the
  archive-based delivery is replaced by the normal database layout.
- Basemap discovery changes from reading a server-provided directory listing to reading a
  generated manifest that names the available database format versions.
- The serving image exposes the basemap slot tree and that manifest with suitable caching, and no
  longer depends on directory listing behaviour for basemap discovery.

## Capabilities

### New Capabilities

- `basemap-generation`: how the regeneration pass produces, places, versions, retains, and
  prunes the world basemap in the repository, and what its configuration file controls.

### Modified Capabilities

- `mapgen-container`: the image contents change (the basemap import tool, the basemap type
  definitions and their location in the image, the configuration it expects, the extra runtime
  utility needed to unpack the coastline data), the volume and configuration mounts gain the
  basemap configuration file, and the bundled type configuration requirement extends to the
  basemap type file.
- `basemap-discovery`: discovery no longer parses a server directory listing; it reads the
  generated manifest, and reports the database format versions the server offers so the system
  can choose one it can read.
- `basemap-download`: the basemap is downloaded as a normal database (metadata file plus the
  file set of a database directory) instead of a single archive that is extracted, with the
  downloaded metadata kept locally as the comparison base for later update checks.

## Impact

- `scripts/mapgen/mapgen.sh` — pass orchestration, configuration validation, check and
  generation records, staging, placement, retention.
- `scripts/mapgen/mapgen-basemap.sh` — new: the basemap step (input checks, coastline fetch and
  verification, import, water index generation, slot placement, manifest writing).
- `scripts/mapgen/Dockerfile` — build and copy the basemap import tool, bundle the basemap type
  file under a documented variable, add the unpack utility to the runtime stage, carry the
  default coastline source.
- `scripts/mapgen/nginx-serve.conf` — locations for the basemap manifest and slot tree.
- `scripts/mapgen/docker-compose.yml`, `scripts/mapgen/imports.example.json` siblings — new
  `basemap.example.json`; the compose file's configuration seed instructions mention the file.
- `scripts/mapgen/mapgen-entrypoint.sh` — configuration checks include the basemap
  configuration.
- `libosmscout-import/CMakeLists.txt`, `libosmscout-import/meson.build` — the import library gains a
  required JSON dependency for the database metadata reader it now shares, and the Import tool's
  metadata writer and JSON writer move into the library (`Import/src/DbJsonWriter.*`,
  `Import/src/JsonWriter.*`).
- Build configurations that build the import library — `conanfile.py`, `vcpkg_full.json`,
  `vcpkg_medium.json`, `vcpkg_minimum.json`, the `ci/docker/*` images and the Ubuntu, macOS, MSYS,
  sanitizer, SonarQube and VS2025 workflows gain that dependency. Configurations that turn the
  import library off (iOS, Android, JavaScout) are unaffected.
- `Tests/CMakeLists.txt`, `Tests/meson.build`, `Tests/src/DbJsonWriterTest.cpp`,
  `Tests/src/JsonWriterTest.cpp` — the tests of the moved metadata writer build against the library
  instead of compiling its sources locally.
- `.github/workflows/mapgen_image.yml` — smoke tests cover the basemap configuration check and
  the presence of the new tool and type file; the config fixtures gain a basemap configuration.
- `libosmscout-client-java/java/com/framstag/libosmscout/client/BasemapManager.java` — discovery
  via the manifest, download through the normal database file set, local metadata as the update
  comparison base; archive extraction removed.
- `libosmscout-client-java/java/com/framstag/libosmscout/client/MapDownloadManager.java` —
  reuse for the basemap file set where the current code is basemap-specific.
- `JavaScout/src/main/java/com/framstag/libosmscout/MapDownloadController.java` — basemap
  availability, installed state, and update indication driven by the new metadata.
- `libosmscout-client-java` tests and `JavaScout` tests — basemap manager tests follow the new
  discovery and download contract.
- `Documentation/MapRepository.md` — configuration files, server layout, integrity model, client
  update contract, and operations sections gain the basemap.
- `webpage/content/tutorials/BasemapImporting.md` — the manual procedure remains valid; a
  pointer to the pipeline is added.
