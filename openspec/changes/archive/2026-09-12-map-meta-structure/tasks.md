## 1. Import tool: metadata emitter

- [x] 1.1 Add `--source-url` and `--source-md5` command line parameters to the Import tool (`Import/src/Import.cpp`), keep them optional, and verify `--help` lists them and a test import with and without them succeeds; verify the project builds cleanly with CMake and Meson (spec: map-metadata/Source identification)
- [x] 1.2 Implement the minimal write-only JSON writer (new translation unit under `Import/`) and verify Catch2 unit tests pass covering escaping of quotes, backslashes, control characters, and non-ASCII strings (spec: map-metadata/Relative references)
- [x] 1.3 Implement the emit step running after a successful import in the Import tool: destination directory file listing, byte sizes, zlib crc32 per file, bounding box read back from `boundingbox.dat`, tool identity/version, steps, duration; verify a generated `db.json` on a test extract with correct content (spec: map-metadata/Output file inventory, Bounding box and statistics)
- [x] 1.4 Write `db.json` atomically (temporary file + rename) and only on success; verify no `db.json` is written when an import run fails and that no partially written file is ever observable (spec: map-metadata/Metadata emission)
- [x] 1.5 Verify inventory consistency: compare db.json file list, sizes, and crc32 against the destination directory contents, run with and without `--eco`, and verify the inventory matches disk in both cases (spec: map-metadata/Output file inventory)
- [x] 1.6 Verify relative-reference rule: copy a database with its db.json to a different directory and verify the metadata stays valid without modification (spec: map-metadata/Relative references)

## 2. Regeneration script and configs

- [x] 2.1 Implement imports manifest validation (schema version, duplicate ids, malformed entries) and region index validation (leaf ids consistent with the manifest in both directions) as a self-check mode of the script; verify sample valid and invalid configs produce the expected accept/reject results (spec: imports-manifest/Imports manifest structure, Unique import ids, Manifest validation; region-index/Leaf id consistency)
- [x] 2.2 Implement the refresh gate: per-id last-check time persisted in `admin/<id>_check.json`, due check only when elapsed time exceeds the refresh frequency (global default, per-id override); verify with manipulated check timestamps that not-due imports cause no network activity (spec: regen-script/Check cycle gating, Check-cycle state; imports-manifest/Refresh frequency)
- [x] 2.3 Implement source change detection: compare the published source hash with the `source.md5` of the newest stored db.json and skip unchanged imports; verify a rerun with an unchanged source performs no download and no import (spec: regen-script/Source change detection)
- [x] 2.4 Implement download and source verification: fetch the published hash sidecar, verify the downloaded source, refuse to import mismatched data; verify with a tampered download that no import runs and the failure is recorded (spec: regen-script/Source verification)
- [x] 2.5 Implement import invocation with `--source-url` and `--source-md5` passed through; verify the values land verbatim in the produced db.json (spec: regen-script/Import invocation)
- [x] 2.6 Implement placement: stage `staging/<id>/v<version>.new` inside the repository volume and rename over the live slot; verify the slot path follows the region index tree and version from db.json, and verify a reader polling during replacement always sees the complete old or complete new database (spec: regen-script/Placement, Atomic replacement; region-index/Server layout hint)
- [x] 2.7 Implement pruning: after each successful placement remove the oldest type-config versions until the retention depth (global default, per-id override) is met; verify version directories are reduced to the expected count (spec: regen-script/Pruning; imports-manifest/Retention depth)
- [x] 2.8 Implement run records: write `admin/<id>_v<version>_generation.json` per placement with placement time, server position, retention, pruned versions, and outcome; verify record content against a manual run (spec: regen-script/Run records)
- [x] 2.9 Implement failure handling: one import failing in a set verifies that the successful database is placed, the failure is recorded, no partial state remains, and the script exits non-zero (spec: regen-script/Failure handling)
- [x] 2.10 Provide example configs (imports manifest F1, region index F2) and an example webserver config denying `/admin/` and `/staging/`; verify the deny paths return non-2xx over HTTP and names.json plus version slots are reachable (spec: client-update-check/Client-accessible data; region-index/Client display)

## 3. Container image

- [x] 3.1 Write the two-stage Dockerfile (build stage: minimized cmake Release build of core, import, and the Import tool; runtime stage: dependencies, script, non-root user); verify `docker build` succeeds and the runtime image contains the Import tool (spec: mapgen-container/Image contents)
- [x] 3.2 Set the entry point to a single pass over the imports manifest with refresh gating; verify the container exits after the pass and runs with a read-only root filesystem as non-root (spec: mapgen-container/Entry point semantics, Least privilege)
- [x] 3.3 Verify the volume contract: downloads and intermediate data confined to a transient work mount, configuration read-only, and placement/pruning happening in the mounted repository (spec: mapgen-container/Volume mounts)
- [x] 3.4 Add a CI job building and smoke-testing the image; verify the job is green (spec: mapgen-container/Image contents)
- [x] 3.5 Fix container build, verified locally: add root `.dockerignore` (exclude maps/, build dirs, debug/, Dockerfile itself to keep COPY cache valid), replace the broken ldd/cp library-copy step with ldd dependency verification, add missing runtime libraries (libxml2, libprotobuf32, liblzma5, libmarisa0, libtbb12, libgomp1, libzstd1, zlib1g), and use `useradd -o` for the uid 1000 collision with the base image's ubuntu user (spec: mapgen-container/Image contents, Least privilege)
- [x] 3.6 Verify the built image: `docker build` green (~200MB), non-root uid 1000, `--check-config` with the public/private layout and a refresh-gated single pass both succeed inside the container (spec: mapgen-container/Entry point semantics)

## 4. Client update check contract

- [x] 4.1 Build a decision-matrix test harness against a fixture repository: metadata with newer timestamp, equal/older timestamp, fresh client, and missing metadata for the client's version; verify the four outcomes from the spec (update available, up to date, available for installation, unavailable) (spec: client-update-check/Update comparison, Missing data for own version, Fresh install offer)
- [x] 4.2 Verify the harness never probes a newer type-config version directory than the client's own, including when only a newer version exists on the server (spec: client-update-check/Own-version probe only)
- [x] 4.3 Verify download verification against db.json checksums: a corrupted fixture file is rejected and not used (spec: client-update-check/Downloaded file verification)

## 5. Integration verification

- [x] 5.1 Build the full project without errors (CMake and Meson) (spec: map-metadata/Metadata emission)
- [x] 5.2 Run the existing test suite (`ctest` on the CMake build) and confirm no regressions (spec: map-metadata/Relative references)
- [x] 5.3 End-to-end run on a fixed extract: import with the new tool, run the script once, verify placement position in the repository, retention, and run records match the region index and manifest (spec: regen-script/Placement, Pruning; region-index/Server layout hint)
- [x] 5.4 Run the script a second time on the unchanged source; verify a no-op (no download, no import, no new placement) (spec: regen-script/Source change detection)
- [x] 5.5 Check all new/modified code against the conventions in guidelines/CodeStyles.md, `.uncrustify`, and `.clang-tidy` (spec: map-metadata/Metadata emission)
- [x] 5.6 Correct AGENTS.md: the claim that `libosmscout/io/` contains MD5/CRC support is wrong; also check TODO.md and update it if the change reveals further pre-existing issues (spec: map-metadata/Relative references)

## 6. Repository layout: public/private separation

- [x] 6.1 Split the repository volume at the top level into `public/` (served root: names.json + version slots) and `private/` (script-owned: admin/ records + staging/), keeping staging on the same filesystem as the public slots for rename atomicity; verify placement lands under `public/<path>/v<version>` and records/staging under `private/` (spec: regen-script/Placement, Atomic replacement; mapgen-container/Volume mounts)
- [x] 6.2 Rely on structural separation instead of webserver deny rules: served root is exactly `public/`; update the example nginx config, so no `/admin/` or `/staging/` deny blocks are needed (spec: client-update-check/Client-accessible data)
- [x] 6.3 Read the region index from the served root (`MAPGEN_NAMES_FILE`, default `<public>/names.json`) as single source of truth; `imports.json` stays in the read-only config mount (spec: mapgen-container/Volume mounts)
- [x] 6.4 Document the rsync publish model (`public/` only, no excludes needed, optional chmod 700 on the private part) and update the container CI smoke test to the new layout (spec: regen-script/Run records)
- [x] 6.5 Verify end-to-end on the NRW sub-region configuration: five slots placed under `public/`, records under `private/admin/`, staging empty, refresh gate no-op on rerun, crc32 inventory intact (spec: regen-script/Placement, Atomic replacement)

## 7. Web server image and orchestration

- [x] 7.1 Add a second nginx-based image (`scripts/mapgen/Dockerfile.serve`, `scripts/mapgen/nginx-serve.conf`) that serves exactly `<repo>/public/` read-only over plain HTTP without authentication; verify names.json and db.json return 200 with `application/json`, database files 200, and any private/staging/directory-listing path 404 (spec: web-server/Served surface, Anonymous read-only access, Static content web server, Image contents)
- [x] 7.2 Add `scripts/mapgen/docker-compose.yml` orchestrating mapgen (writer, writable repository) and serve (reader, read-only) on one shared repository volume; document first-run seeding and volume ownership; verify compose config parses, both containers start, and `--check-config` passes through compose (spec: web-server/Compose orchestration, Server sees regenerated data)

## 8. Client guide

- [x] 8.1 Write `Documentation/MapClientGuide.md` covering discovery (names.json), the own-version db.json probe with the four-outcome decision matrix, download with per-file size + CRC-32 verification, and worked curl examples (spec: client-update-check/Update comparison, Missing data for own version, Fresh install offer, Downloaded file verification; map-metadata/Output file inventory)
