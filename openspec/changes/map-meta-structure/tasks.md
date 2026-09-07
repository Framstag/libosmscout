## 1. Import tool: metadata emitter

- [ ] 1.1 Add `--source-url` and `--source-md5` command line parameters to the Import tool (`Import/src/Import.cpp`), keep them optional, and verify `--help` lists them and a test import with and without them succeeds; verify the project builds cleanly with CMake and Meson (spec: map-metadata/Source identification)
- [ ] 1.2 Implement the minimal write-only JSON writer (new translation unit under `Import/`) and verify Catch2 unit tests pass covering escaping of quotes, backslashes, control characters, and non-ASCII strings (spec: map-metadata/Relative references)
- [ ] 1.3 Implement the emit step running after a successful import in the Import tool: destination directory file listing, byte sizes, zlib crc32 per file, bounding box read back from `boundingbox.dat`, tool identity/version, steps, duration; verify a generated `db.json` on a test extract with correct content (spec: map-metadata/Output file inventory, Bounding box and statistics)
- [ ] 1.4 Write `db.json` atomically (temporary file + rename) and only on success; verify no `db.json` is written when an import run fails and that no partially written file is ever observable (spec: map-metadata/Metadata emission)
- [ ] 1.5 Verify inventory consistency: compare db.json file list, sizes, and crc32 against the destination directory contents, run with and without `--eco`, and verify the inventory matches disk in both cases (spec: map-metadata/Output file inventory)
- [ ] 1.6 Verify relative-reference rule: copy a database with its db.json to a different directory and verify the metadata stays valid without modification (spec: map-metadata/Relative references)

## 2. Regeneration script and configs

- [ ] 2.1 Implement imports manifest validation (schema version, duplicate ids, malformed entries) and region index validation (leaf ids consistent with the manifest in both directions) as a self-check mode of the script; verify sample valid and invalid configs produce the expected accept/reject results (spec: imports-manifest/Imports manifest structure, Unique import ids, Manifest validation; region-index/Leaf id consistency)
- [ ] 2.2 Implement the refresh gate: per-id last-check time persisted in `admin/<id>_check.json`, due check only when elapsed time exceeds the refresh frequency (global default, per-id override); verify with manipulated check timestamps that not-due imports cause no network activity (spec: regen-script/Check cycle gating, Check-cycle state; imports-manifest/Refresh frequency)
- [ ] 2.3 Implement source change detection: compare the published source hash with the `source.md5` of the newest stored db.json and skip unchanged imports; verify a rerun with an unchanged source performs no download and no import (spec: regen-script/Source change detection)
- [ ] 2.4 Implement download and source verification: fetch the published hash sidecar, verify the downloaded source, refuse to import mismatched data; verify with a tampered download that no import runs and the failure is recorded (spec: regen-script/Source verification)
- [ ] 2.5 Implement import invocation with `--source-url` and `--source-md5` passed through; verify the values land verbatim in the produced db.json (spec: regen-script/Import invocation)
- [ ] 2.6 Implement placement: stage `staging/<id>/v<version>.new` inside the repository volume and rename over the live slot; verify the slot path follows the region index tree and version from db.json, and verify a reader polling during replacement always sees the complete old or complete new database (spec: regen-script/Placement, Atomic replacement; region-index/Server layout hint)
- [ ] 2.7 Implement pruning: after each successful placement remove the oldest type-config versions until the retention depth (global default, per-id override) is met; verify version directories are reduced to the expected count (spec: regen-script/Pruning; imports-manifest/Retention depth)
- [ ] 2.8 Implement run records: write `admin/<id>_v<version>_generation.json` per placement with placement time, server position, retention, pruned versions, and outcome; verify record content against a manual run (spec: regen-script/Run records)
- [ ] 2.9 Implement failure handling: one import failing in a set verifies that the successful database is placed, the failure is recorded, no partial state remains, and the script exits non-zero (spec: regen-script/Failure handling)
- [ ] 2.10 Provide example configs (imports manifest F1, region index F2) and an example webserver config denying `/admin/` and `/staging/`; verify the deny paths return non-2xx over HTTP and names.json plus version slots are reachable (spec: client-update-check/Client-accessible data; region-index/Client display)

## 3. Container image

- [ ] 3.1 Write the two-stage Dockerfile (build stage: minimized cmake Release build of core, import, and the Import tool; runtime stage: dependencies, script, non-root user); verify `docker build` succeeds and the runtime image contains the Import tool (spec: mapgen-container/Image contents)
- [ ] 3.2 Set the entry point to a single pass over the imports manifest with refresh gating; verify the container exits after the pass and runs with a read-only root filesystem as non-root (spec: mapgen-container/Entry point semantics, Least privilege)
- [ ] 3.3 Verify the volume contract: downloads and intermediate data confined to a transient work mount, configuration read-only, and placement/pruning happening in the mounted repository (spec: mapgen-container/Volume mounts)
- [ ] 3.4 Add a CI job building and smoke-testing the image; verify the job is green (spec: mapgen-container/Image contents)

## 4. Client update check contract

- [ ] 4.1 Build a decision-matrix test harness against a fixture repository: metadata with newer timestamp, equal/older timestamp, fresh client, and missing metadata for the client's version; verify the four outcomes from the spec (update available, up to date, available for installation, unavailable) (spec: client-update-check/Update comparison, Missing data for own version, Fresh install offer)
- [ ] 4.2 Verify the harness never probes a newer type-config version directory than the client's own, including when only a newer version exists on the server (spec: client-update-check/Own-version probe only)
- [ ] 4.3 Verify download verification against db.json checksums: a corrupted fixture file is rejected and not used (spec: client-update-check/Downloaded file verification)

## 5. Integration verification

- [ ] 5.1 Build the full project without errors (CMake and Meson) (spec: map-metadata/Metadata emission)
- [ ] 5.2 Run the existing test suite (`ctest` on the CMake build) and confirm no regressions (spec: map-metadata/Relative references)
- [ ] 5.3 End-to-end run on a fixed extract: import with the new tool, run the script once, verify placement position in the repository, retention, and run records match the region index and manifest (spec: regen-script/Placement, Pruning; region-index/Server layout hint)
- [ ] 5.4 Run the script a second time on the unchanged source; verify a no-op (no download, no import, no new placement) (spec: regen-script/Source change detection)
- [ ] 5.5 Check all new/modified code against the conventions in guidelines/CodeStyles.md, `.uncrustify`, and `.clang-tidy` (spec: map-metadata/Metadata emission)
- [ ] 5.6 Correct AGENTS.md: the claim that `libosmscout/io/` contains MD5/CRC support is wrong; also check TODO.md and update it if the change reveals further pre-existing issues (spec: map-metadata/Relative references)
