# Tasks

## 1. Shared database metadata helper

- [x] 1.1 Move `Import/src/DbJsonWriter.{h,cpp}` and `Import/src/JsonWriter.{h,cpp}` into the import
      library as `libosmscout-import/include/osmscoutimport/DbJson.h`,
      `libosmscout-import/src/osmscoutimport/DbJson.cpp`,
      `libosmscout-import/include/osmscoutimport/JsonWriter.h` and
      `libosmscout-import/src/osmscoutimport/JsonWriter.cpp`, keeping the written layout and the
      `DbJsonData`, `DbJsonFileEntry`, `BuildDbJsonInventory`, `WriteDbJson` and `JsonWriter`
      behaviour unchanged; update `Import/src/Import.cpp` to include the library headers and delete
      the tool's copies (spec: basemap-generation — Basemap content)
- [x] 1.2 Update the library build descriptions (`libosmscout-import/CMakeLists.txt`,
      `libosmscout-import/meson.build`) for the moved sources and headers, and remove the moved
      sources from `Import/CMakeLists.txt` and `Import/meson.build`
      (spec: basemap-generation — Basemap content)
- [x] 1.3 Add `ReadDbJson(const std::string& directory, DbJsonData& data)` beside the writer, using
      `nlohmann_json`: read the schema, type config version, generated timestamp, source url and md5,
      the import block, the output bounding box, every `output.files` entry with its size and CRC-32,
      and the type count; report failure for a missing file, unparsable content, and a missing or
      unsupported schema value (spec: basemap-generation — Basemap content)
- [x] 1.4 Add unit tests in `Tests/src/DbJsonWriterTest.cpp`: write-then-read round trip preserves
      every field including the file inventory and CRC-32 values; reading a missing file fails;
      reading malformed content fails; reading a file with an unsupported schema fails; reading a
      file written by the current writer keeps the inventory intact
      (spec: basemap-generation — Basemap content)
- [x] 1.5 Update `Tests/CMakeLists.txt` and `Tests/meson.build` so `DbJsonWriterTest` and
      `JsonWriterTest` include the library headers and link the import library instead of compiling
      the moved sources (spec: basemap-generation — Basemap content)
- [x] 1.6 Verify the build compiles without warnings and the existing tests still pass
      (`cmake --build build`, `ctest -R "DbJson|JSON"`)

## 2. Dependency propagation (nlohmann_json)

- [x] 2.1 Declare the requirement where the import library is described:
      `find_package(nlohmann_json REQUIRED)` plus
      `target_link_libraries(OSMScoutImport nlohmann_json::nlohmann_json)` in
      `libosmscout-import/CMakeLists.txt`, and `dependency('nlohmann_json', required: true)` added to
      the library's dependencies in `libosmscout-import/meson.build`; leave the optional lookup in
      `cmake/features.cmake` untouched for MCPServer
      (spec: mapgen-container — Image contents)
- [x] 2.2 Add the dependency to `conanfile.py` (with the import-library options) and to
      `vcpkg_full.json`, `vcpkg_medium.json`, `vcpkg_minimum.json`
      (spec: mapgen-container — Image contents)
- [x] 2.3 Add the dependency package to the build stage of `scripts/mapgen/Dockerfile` and to the
      `ci/docker/*` images that build the import library
      (spec: mapgen-container — Image contents)
- [x] 2.4 Add the dependency package to the workflows that build the import library: Ubuntu (CMake
      and Meson, both compilers), macOS, MSYS, sanitizer, SonarQube, and rely on the vcpkg profiles
      for VS2025 (spec: mapgen-container — Image contents)
- [x] 2.5 Verify a CMake and a Meson configure/build with the import library enabled, and verify that
      a configure with the import library disabled still succeeds without the package present
      (`-DOSMSCOUT_BUILD_IMPORT=OFF`)

## 3. Basemap import tool updates the metadata

- [x] 3.1 In `BasemapImport/src/BasemapImport.cpp`, after a successful `water.idx` write, update an
      existing `db.json` in the destination directory with an inventory entry for `water.idx`
      (size and CRC-32, using the shared helper); when no `db.json` exists, log a note and continue,
      so the documented manual procedure still works
      (spec: basemap-generation — Basemap content)
- [x] 3.2 Add a unit test covering: an existing metadata file gains the water index entry with a
      matching checksum; the operation leaves all other metadata fields untouched; a directory
      without metadata is left alone and reported
      (spec: basemap-generation — Basemap content)
- [x] 3.3 Confirm the `BasemapImport` target is still built with
      `-DOSMSCOUT_BUILD_TOOL_IMPORT=ON` and update `BasemapImport/CMakeLists.txt` and
      `BasemapImport/meson.build` only if the shared helper needs it
      (spec: mapgen-container — Image contents)

## 4. Basemap configuration

- [x] 4.1 Add `basemap.json` validation to `scripts/mapgen/mapgen.sh`: required file, supported
      schema version, required extract path, refresh and history values, adoption interval not
      shorter than the refresh interval, and rejection of unknown `importOptions` keys
      (spec: basemap-generation — Basemap configuration file, Declared inputs)
- [x] 4.2 Validate `importOptions` values: numeric ranges, `waterIndexMinMag <= waterIndexMaxMag`,
      `minIndexLevel <= maxIndexLevel` and the tool's index limit, and reject an unknown key.
      The map is limited to the options listed in the design (`waterIndexMinMag`,
      `waterIndexMaxMag`, `lowZoomOptMaxMag`, `areaNodeGridMag`, `maxAdminLevel`, `langOrder`,
      `altLangOrder`, `strictAreas`, `minIndexLevel`, `maxIndexLevel`, `maxWaterDistance`)
      (spec: basemap-generation — Declared inputs)
- [x] 4.3 Include the basemap configuration in `--check-config` and report a missing or invalid
      file as a failure of the check, without downloading or importing anything
      (spec: mapgen-container — Configuration check covers the basemap configuration)
- [x] 4.4 Add `scripts/mapgen/basemap.example.json` documenting every option, and mention the new
      file in the mount description of `scripts/mapgen/Dockerfile` and the seed instructions of
      `scripts/mapgen/docker-compose.yml`
      (spec: mapgen-container — Volume mounts)

## 5. Basemap step: input checks and cadences

- [x] 5.1 Create `scripts/mapgen/mapgen-basemap.sh` with the configuration reading, logging and
      helpers it needs, callable with the same environment conventions as `mapgen.sh`
      (spec: basemap-generation — Basemap configuration file)
- [x] 5.2 Implement extract hashing plus check-state reading and writing
      (`private/admin/basemap_check.json`: extract hash, coastline hash, coastline `Last-Modified`,
      last checked, last adopted), and the refresh gate that skips all check work inside the
      interval (spec: basemap-generation — Change detection and refresh cadence)
- [x] 5.3 Implement the coastline fetch: conditional request with the stored `Last-Modified`, reuse
      of the work-area archive on "not modified", download on change, verification against a
      configured `sha256` when present, and extraction with `unzip`
      (spec: basemap-generation — Coastline acquisition and adoption cadence)
- [x] 5.4 Implement the adoption gate: a newer remote copy is only adopted once
      `coastlinesRefresh` has elapsed since the last adoption; report newer-but-not-adopted
      availability in the pass output
      (spec: basemap-generation — Coastline acquisition and adoption cadence)
- [x] 5.5 Implement the decision reporting: no input change means no production, with the check
      recorded and no generation record written
      (spec: basemap-generation — Records of the basemap step)

## 6. Basemap production and placement

- [x] 6.1 Implement the database production: run `Import` with the bundled basemap type file, the
      configured import options, `--source-url` naming the mounted export and `--source-md5` carrying
      its hash, and cache the output in the work area keyed by that hash so a coastline-only change
      re-runs only the water index step
      (spec: basemap-generation — Basemap content)
- [x] 6.2 Run `BasemapImport` with the configured index levels and maximum water distance against
      the unpacked shapefile, so the placed water index is the coastline-derived one
      (spec: basemap-generation — Basemap content)
- [x] 6.3 Verify the produced directory against the file set a client requires of a database
      directory, and fail loudly naming any missing file before anything is placed
      (spec: basemap-generation — Basemap content)
- [x] 6.4 Determine the database format version from the produced metadata and place the result into
      `public/basemap/v<version>/` using the staging directory and directory replacement the regional
      pipeline already uses, so no partial file set is ever served
      (spec: basemap-generation — Version-keyed slot placement)
- [x] 6.5 Restore or complete a slot left incomplete by an interrupted replacement on the next pass,
      mirroring the existing staging recovery
      (spec: basemap-generation — Version-keyed slot placement)

## 7. Manifest, retention, records

- [x] 7.1 Write `public/basemap/index.json` (schema, versions with their change times) atomically
      after a successful placement, and only then
      (spec: basemap-generation — Availability manifest)
- [x] 7.2 Prune basemap slot directories beyond the configured retention after a successful
      placement, keeping all versions when retention asks for no pruning
      (spec: basemap-generation — Retention)
- [x] 7.3 Write the generation record for each placement (`private/admin/basemap_v<version>_generation.json`)
      naming the placed version, the time, and the pruned versions
      (spec: basemap-generation — Records of the basemap step)

## 8. Pass integration

- [x] 8.1 Invoke `mapgen-basemap.sh` from `scripts/mapgen/mapgen.sh` after the regional loop, inside
      the same pass and lock, and fold its status into the pass status so a basemap failure is
      reported without undoing or blocking the regional work
      (spec: basemap-generation — Failure isolation)
- [x] 8.2 Keep the previously served basemap intact on any failure path, and add a shell-level test
      or a documented manual check for the failing-basemap case reusing
      `scripts/mapgen/recovery-check-test.sh` conventions
      (spec: basemap-generation — Failure isolation)
- [x] 8.3 Confirm the entry point still passes arguments such as `--check-config` through and does
      not start a scheduled mode when arguments are given
      (spec: mapgen-container — Entry point semantics)

## 9. Container image

- [x] 9.1 Build and copy the basemap import tool in `scripts/mapgen/Dockerfile` (build target and a
      copy into the runtime image), alongside the existing import tool
      (spec: mapgen-container — Image contents)
- [x] 9.2 Add the unpack utility to the runtime stage, add the default coastline source as build
      arguments next to the existing pinned-scheduler pattern, and point a documented variable at the
      bundled basemap type file (`MAPGEN_BASEMAP_TYPEFILE`)
      (spec: mapgen-container — Image contents)
- [x] 9.3 Verify the image: both tools present, the basemap type file present, the unpack utility
      present, the non-root identity unchanged, and the bundled type files load
      (spec: mapgen-container — The bundled type configuration is complete and loads)
- [x] 9.4 Extend the smoke tests in `.github/workflows/mapgen_image.yml`: a basemap configuration
      check that passes, and one that fails for a missing basemap configuration; update the existing
      config fixtures so the current checks keep passing
      (spec: mapgen-container — Configuration check covers the basemap configuration)

## 10. Serving configuration

- [x] 10.1 Add locations in `scripts/mapgen/nginx-serve.conf` for `basemap/index.json` (JSON content
      type, short cache) and for the basemap slot tree (long cache), leaving directory listings
      disabled, and verify with `curl` against a local compose run that the manifest and one slot file
      are served and that a directory path is still refused
      (spec: mapgen-container — Image contents)

## 11. Client discovery

- [x] 11.1 Replace the directory-listing probe in
      `libosmscout-client-java/java/com/framstag/libosmscout/client/BasemapManager.java` with a read
      of the availability manifest, reporting the offered versions and the newest change time, and
      reporting the basemap as unavailable without an error for a missing, empty, or unparsable
      manifest (spec: basemap-discovery — Probe basemap existence)
- [x] 11.2 Choose the newest offered version the library supports, using the same database format
      version value the regional listing uses; expose that value once instead of adding a second
      literal (spec: basemap-discovery — Choose a readable version)
- [x] 11.3 Report update availability by comparing the locally kept metadata against the manifest's
      version and change time (spec: basemap-discovery — Report basemap version)
- [x] 11.4 Update `BasemapManagerTest.java` for the manifest contract: version selection, no readable
      version, unparsable manifest, update available, and update not available
      (spec: basemap-discovery — Choose a readable version)

## 12. Client download

- [x] 12.1 Download the basemap as a database: the chosen version's metadata, then every data file it
      names, into `{mapsDir}/basemap/`, keeping the metadata locally
      (spec: basemap-download — Download basemap archive)
- [x] 12.2 Verify each downloaded file against the checksum in the metadata before registration, and
      discard the installation and report the error on a mismatch, leaving any previous installation
      usable (spec: basemap-download — Verify downloaded basemap files)
- [x] 12.3 Replace the archive download and extraction with the atomic update flow driven by the
      locally kept metadata (spec: basemap-download — Update basemap)
- [x] 12.4 Remove the variant selection from
      `JavaScout/src/main/java/com/framstag/libosmscout/MapDownloadController.java` and offer the
      single available basemap directly, keeping the download, update, and cancel controls
      (spec: basemap-download — Select basemap variant removal)
- [x] 12.5 Update the download-related tests, including the failure and cancel paths, so no test
      depends on archive extraction (spec: basemap-download — Download basemap archive, Cancel
      basemap download)

## 13. Documentation

- [x] 13.1 Update `Documentation/MapRepository.md`: the configuration files section (three files),
      the server layout section (basemap slots and manifest), the integrity model (checksums for the
      basemap), the client update contract (basemap metadata comparison), and the operations section
      (retention and refresh cadences)
      (spec: basemap-generation — Availability manifest, Retention)
- [x] 13.2 Add a pointer from `webpage/content/tutorials/BasemapImporting.md` to the pipeline, so the
      manual procedure is documented as the manual case
      (spec: basemap-generation — Basemap content)
- [x] 13.3 Update `AGENTS.md` and `Documentation/MapRepository.md` references if the new script and
      configuration change the described structure of `scripts/mapgen/`
      (spec: mapgen-container — Image contents)

## 14. Verification

- [x] 14.1 Verify both build systems configure and build: CMake (`cmake --build build`) and Meson
      (`meson compile -C build`) after the metadata helper move and the tool changes
- [x] 14.2 Verify the existing test suite still passes (`ctest -j 2 --output-on-failure` with
      `QT_QPA_PLATFORM=offscreen` where needed) and report any test that had to change
- [x] 14.3 Run an end-to-end pass against a small extract and a small shapefile: confirm the slot
      contains the required file set, the water index is the coastline-derived one, the manifest
      names the version, a second pass with unchanged inputs does nothing, and a pass with a changed
      extract places a new slot
- [x] 14.4 Verify the client against the pass output: discovery, download, checksum verification, and
      loading of the basemap as an overlay
