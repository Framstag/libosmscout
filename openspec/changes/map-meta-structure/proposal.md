## Why

Maps are generated from OSM extracts and must be regenerated regularly, copied into a standard server directory layout, and offered to clients that can check for updates. Today nothing records what was generated, from what source, when, and by which tool — regeneration, placement, and client update checks all have to be done ad hoc without a contract.

## What Changes

- Add a machine-readable manifest of all supported imports (id, download source, refresh frequency, retention depth), the base for the regeneration process.
- Add a machine-readable region index that associates every import id with localized display names and a hierarchical structure, used by client UI and as layout hint for the server directory structure.
- Add a machine-readable metadata file per generated database, written at the end of the import run, containing creation timestamp, type-config version, source identification, output file list with integrity data, and basic statistics.
- Add a machine-readable generation record per placed database, written by the regeneration process, documenting placement, pruning, and run outcomes.
- Add a regeneration script that downloads sources, verifies them, runs the import, places the resulting databases at the correct position in the server directory, and prunes old versions according to retention settings.
- Extend the import tool to emit the per-database metadata file and to accept source identification parameters.
- Add a container image that bundles the regeneration script and the current import tool and can mount a database repository to update.

## Capabilities

### New Capabilities

- `imports-manifest`: Definition and validation of the imports manifest (supported imports, download sources, refresh frequency, retention depth, global defaults and per-id overrides).
- `region-index`: Definition of the region index (hierarchical structure, localized names, leaf id contract with the imports manifest) and its client display contract.
- `map-metadata`: Per-database metadata emitted by the import tool at the end of a successful import run (creation time, type-config version, source facts, output files with sizes and integrity checksums, bounding box, statistics), including the new import tool parameters.
- `regen-script`: Behavior of the regeneration process (check cycle gating by refresh frequency, source verification, change detection, import invocation, placement, pruning, script-owned state records, error handling).
- `mapgen-container`: Container image contract (contents, entry point semantics, volume mounts, runtime user, read-only root filesystem).
- `client-update-check`: Client-side update detection semantics (probe only the client's own type-config version, comparison of creation timestamps, no probing of newer versions).

### Modified Capabilities

- None.

## Impact

- `Import/src/Import.cpp` — new command-line parameters (source identification), metadata emission step after successful import.
- `libosmscout-import/` — metadata emission support (new or extended API for import parameter source facts; no change to existing behavior).
- `libosmscout/include/osmscout/TypeConfig.h` — `FILE_FORMAT_VERSION` is the type-config version recorded in metadata (read-only reference, no change).
- New: regeneration script (bash), container image definition, server directory layout definition, client update-check documentation — all living in this change's deliverables unless a home is chosen in design.
- `AGENTS.md` — documentation fix: the claim that `libosmscout/io/` contains MD5/CRC support is incorrect; no checksum facility exists in the library today.
- No change to the type definition format, the generated database format, or the public type resolution API.
