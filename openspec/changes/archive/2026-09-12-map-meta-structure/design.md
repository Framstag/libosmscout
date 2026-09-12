## Context

See proposal.md - Why for motivation, specs/ for the required behavior. Current state: the Import tool writes a database into a destination directory and logs progress, but emits no machine-readable output; nothing records provenance, and no scriptable placement/update contract exists. Relevant verified facts:

- `Preprocess.cpp:874-878` already writes `boundingbox.dat` (a `GeoBox` of the imported data) — the bounding box is available at end of run.
- `libosmscout-import/CMakeLists.txt:164-165` links `ZLIB::ZLIB` — zlib is already in the import build chain.
- `nlohmann/json` exists in the repo but only inside MCPServer; it is not part of the import tool chain and appears in none of its build configs (CMake/vcpkg/conan — and the tool is also built by Meson, `meson.build:510`).
- No MD5/CRC utility exists in libosmscout (AGENTS.md's claim about `io/` is incorrect; confirmed absent in working tree and git history).
- `FILE_FORMAT_VERSION` (TypeConfig.h:1044, currently 27) is the type-config version.

## Goals / Non-Goals

**Goals:**
- Contract-driven regeneration: scriptable check → verify → import → place → prune pipeline.
- Provenance per generated database, emitted by the import tool itself at end of run.
- Client update detection with zero server-side version enumeration; clients probe only their own type-config version.
- Containerized pipeline with pinned import tool; scheduler stays external.
- No new external dependencies anywhere (JSON writing, checksums, and parsing all rely on what exists).

**Non-Goals:**
- Changes to the database format, type definition format, or public libosmscout APIs.
- The actual webserver serving the repository (nginx vs MCPServer is an operator choice; this change defines the layout contract and example deny rules only).
- Windows-native execution of the regeneration script (the container is Linux; Windows hosts run the container).
- Client implementation (this change defines the client check contract and data access rules, not the client app itself).

## Decisions

### Decision 1: File format — JSON everywhere

All four data files (imports manifest, region index, database metadata, run records) are JSON.

- Chosen: JSON. Scripts consume it with `jq` or POSIX-adjacent tooling; python3 stdlib parses it; C++ already has nlohmann precedent (MCPServer) if a future client-side parser is needed; the import tool only ever *writes* its file (see Decision 4).
- Alternative A: YAML — comments and hand-editing friendlier, but parsing from bash requires extra tooling and indentation errors fail silently in pipelines.
- Alternative B: TOML/INI — weak nesting for the tree and per-file inventories; TOML write support in scripts is immature.

Rationale: JSON is the lingua franca for machine-consumed output; all three consumers (script, client, import tool) cover it with zero new tooling.

### Decision 2: Split ownership via separate files

Three per-database artifacts, not one:

- `db.json` — owned by the import tool, written once at end of a successful run, copied verbatim to the server, never mutated afterwards.
- `<id>_v<version>_generation.json` — owned by the regeneration script, written per placement at the server side, documented as a run record (placement time, pruned versions, outcomes).
- `<id>_check.json` — owned by the script, per-id check-cycle state (last check time, last seen source hash).

- Chosen: split. Writers never mutate each other's output; provenance stays trustworthy; client needs exactly one file (db.json) plus the region index.
- Alternative A: one metadata file that the script rewrites at placement — corrupts import provenance and makes the client depend on script-written sections.
- Alternative B: no run records — loses auditability and the refresh gate's persistence.

Rationale: ownership boundaries map 1:1 to file boundaries; script state derives from the server tree anyway (newest db.json hash = previous source), so run records stay documentary and never become a functional prerequisite.

### Decision 3: Integrity — md5 for the source, self-implemented CRC-32 for the outputs

Two layers, each fit for its purpose:

- Source (the downloaded extract): the download service publishes an md5 sidecar; the script verifies the download against it and passes the verified hash to the import tool as `--source-md5`. The md5 is embedded, never recomputed inside libosmscout (no hash code needed for it).
- Output database files: a self-implemented table-based CRC-32 (`libosmscout/include/osmscout/io/Crc32.h`, `libosmscout/src/osmscout/io/Crc32.cpp`, public API `osmscout::Crc32` and `osmscout::ComputeFileCrc32`; IEEE 802.3 polynomial 0x04C11DB7, reflected 0xEDB88320) computed by the emit step. The implementation is bit-identical to zlib's `crc32()` — verified against the standard check value (0xCBF43926 for "123456789") and cross-checked against zlib on real import output — so clients can verify with the library itself or any standard CRC-32 implementation. Checksums + byte sizes land in db.json; the client verifies downloads, the script verifies copies.

- Chosen: md5-in (published sidecar + pass-through) and self-implemented CRC-32 out. CRC-32 detects torn copies, disk rot, and buggy transfers — the actual threat model; adversarial integrity is covered by TLS on the download channel and md5 on the source. A table-based CRC-32 is ~40 lines, has no dependency, and is trivially testable against the standard check value.
- Alternative A: zlib `crc32()` (zlib is already linked in the import chain) — zero new code, but adds a dependency on a C library for a 40-line algorithm and pulls zlib into the Import tool's link line in Meson.
- Alternative B: add an MD5 implementation to libosmscout and checksum everything with it — matches the sidecar format but adds ~200 lines of new crypto code to a library that never had it; no consumer gains anything over crc32 for output files.
- Alternative C: no output checksums — a truncated db file would only be caught by the client at runtime, not by verification.

Note: POSIX `cksum` uses a different CRC parametrization than the IEEE 802.3 CRC-32 — it must not be used to verify the emitted crc32 values. Script-side verification uses python3 `zlib.crc32` or is skipped in favor of client-side verification.

### Decision 4: Metadata emitter — minimal in-house JSON writer in the Import tool

The emit step lives in the Import tool (`Import/`), not in libosmscout-import: it reads everything through existing parameter getters, so no library API change is needed. It writes JSON with a small, write-only emitter (one new translation unit) rather than pulling nlohmann into the tool's chain.

- Chosen: write-only emitter, escaping unit-tested. The schema is fixed and small; a writer is ~100 lines; no dependency appears in any of the four build systems (CMake, Meson, vcpkg, conan) that would otherwise need the nlohmann entry.
- Alternative A: add nlohmann (header-only) to the Import tool chain — parse machinery we never use, plus build-config churn in every build system the tool participates in.
- Alternative B: emit in libosmscout-import — forces new public surface in the library for a tool concern.

Rationale: the tool needs a one-way, narrow JSON writer; a dependency with a full parser for that is the wrong trade. The escaping tests pin the only real risk (path/URL strings with quotes, backslashes, control chars).

### Decision 5: Server layout — per-id version slots, admin/ excluded at the webserver

```
/repository/                 served root (webserver)
  names.json                   region index (F2)
  berlin/
    v27/                       one slot per type-config version
      db.json                  database metadata (F3)
      map.lib, *.dat, ...      database files
    v26/  ...                  older slots, kept per retention
  admin/                       NOT served (deny rule)
    berlin_v27_generation.json
    berlin_check.json
  staging/                     NOT served (deny rule)
    berlin/v27.new/            staging for atomic replacement
```

- Chosen: the leaf path in the region index gives the placement position; the type-config version from db.json keys the slot dir. No `current` symlink: the version path IS the stable path — consumers pick by their own version. `admin/` and `staging/` are excluded at the webserver via `location ^~` deny rules in the example config.
- Alternative A: a `current` symlink plus version dirs — one more moving part and a torn-flip window for readers that follow it.
- Alternative B: serve a clean subtree (`tree/`) and keep `staging/`/`admin/` outside — pollutes every URL with a prefix; deny rules on the flat root are simpler.

Rationale: version-keyed paths make client probing a single GET (see Decision 6) and keep the client-accessible surface small and explicit.

### Decision 6: Client update check — own-version probe only

The client requests `<leaf>/v<itsTypeConfigVersion>/db.json`. 404 → no data for its version, stop. 200 → compare `generatedAt` against its local baseline (its own last-seen db.json or absence = fresh install). Never probes v+1.

- Chosen: probe-own-version. Requires zero server-side enumeration/version listing feature; the decision matrix is already specified in specs/client-update-check.
- Alternative A: server returns an index of available versions — a new server feature and more surface for the client to mis-parse.
- Alternative B: no probe at all, client trusts a global index — breaks the "one db per slot" freshness model.

### Decision 7: Atomic replacement — stage inside the repository volume, rename

The script imports into a work area, places the result as `<leaf>/v<version>.new`, then renames it over the live slot (`mv` on the same filesystem), then prunes.

- Chosen: staging inside the repository volume (under `staging/`). `rename(2)` is atomic on the same filesystem, so readers see the complete old or complete new database, never a mixture. If staging lived on the work volume, the cross-filesystem `mv` would fail with EXDEV or silently fall back to copy (non-atomic).
- Alternative A: `cp` the files directly into the live slot — torn reads for concurrent consumers, exactly what the "Atomic replacement" requirement forbids.
- Alternative B: staging on the work volume and accepting a copy-across-filesystems — non-atomic and slower.

### Decision 8: Change gating — two levels, check state persisted

- Refresh level: per-id `lastCheckedAt` compared against the refresh frequency (integer days, global default + per-id override). Not due → skip with no network activity. `lastCheckedAt` is updated every check, regardless of outcome, so the gate is stable across runs.
- Source level: published source hash vs newest db.json `source.md5`. Equal → skip import; differ → download, verify, import.
- Chosen: explicit per-id check-state file in `admin/`.
- Alternative A: derive last check from db timestamps — fails for runs where nothing changed (no new db.json, so the gate would re-check every run).
- Alternative B: no refresh gate, let an external scheduler be the only throttle — makes per-id cadences (the refresh field) meaningless.

Unit choice is integer days: `now - lastCheckedAt >= refresh*86400` is trivial in bash, and jq can compare epochs directly. ISO 8601 durations (`P7D`) were rejected: parsing them in bash/jq costs more than the readability gains.

### Decision 9: Container — two-stage build, dynamic linking, single-pass entrypoint

```
Stage 1 (build): ubuntu:noble, cmake Release, build only core + import
                 + Import tool (unneeded OSMSCOUT_BUILD_* features off)
Stage 2 (runtime): debian-slim, Import binary + ldd-copied .so deps,
                 jq/curl/ca-certificates, script, non-root user,
                 read-only rootfs
ENTRYPOINT: one pass over the imports manifest, honoring refresh, exit 0
```

Mounts: work area (`/work`, transient — pbf, import temp), repository (`/repository`), config read-only (`/config` — imports manifest + region index).

- Chosen: dynamic linking + slim runtime (~150 MB) and external scheduler (host cron / systemd timer / k8s CronJob restarts the container). Refresh gating inside stays meaningful regardless of restart frequency (Decision 8).
- Alternative A: static-linked binary + distroless — smallest image, but static C++ linking of protobuf/libxml2 is finicky and buys little here.
- Alternative B: cron inside the container — couples scheduling with the image and duplicates the refresh gate's job.

## Risks / Trade-offs

- **[JSON escaping bugs in emitter]** → Dedicated unit tests with hostile strings (quotes, backslash, control chars, unicode paths) before any integration test. See tasks.
- **[crc32 vs POSIX cksum confusion]** → Documented in the script header and design; verification uses python3 `zlib.crc32` or client-side only. `cksum` is never used.
- **[EXDEV / non-atomic fallback]** → Staging lives inside the repository volume; a test asserts rename stays on one filesystem.
- **[Webserver misconfig exposes admin/ or staging/]** → Example server config ships with deny rules; a verification task probes those paths over HTTP.
- **[eco mode deletes intermediate files — inventory drift]** → The file inventory is taken from the actual destination-directory listing at emit time (truth = disk), not an assumed file set; tasks test inventory with and without `--eco`.
- **[Import tool version drift vs image tag]** → Image tag == libosmscout version; db.json records `import.version`; ops can diff tree metadata vs image tag.
- **[Rollback mid-rollout]** → Old Import binary still runs (new params optional, emitter additive). Script treats a missing db.json as "source changed" → one import regenerates it. Self-healing.

## Migration Plan

1. Implement the emitter + new CLI parameters in the Import tool; unit tests for the JSON writer; build in both CMake and Meson.
2. Write the regeneration script with example configs (F1/F2) and the example webserver deny config; unit/self-check mode for config validation.
3. Dockerfile + CI smoke build of the image.
4. Deploy: run one manual import with the new tool on a test extract; verify db.json content, then run the script against the test repository; verify placement/prune against retention.
5. Cut over scheduled runs to the container.
6. Rollback: revert to the old image / binary; existing databases remain valid; no db.json emitted until the new tool runs again (which recreates it on the next import).

## Open Questions

- Webserver choice (nginx vs MCPServer) — operator decision; this change only defines the layout contract and example deny rules. Does not affect specs, approach, or tasks.
- Whether `imports.json` (F1) itself is served — defaults to no (client never needs it); if an ops dashboard later wants it, the deny rule just gets removed. Does not affect specs, approach, or tasks.
