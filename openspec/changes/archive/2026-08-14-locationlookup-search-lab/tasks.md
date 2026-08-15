# tasks

Testing note: LocationLookup is a standalone demo binary with no unit-test framework in `Demos/` and no database fixtures available to `Tests/`. New logic is therefore verified by build checks, existing test suite runs, and manual smoke runs against a real database (tasks 5.x). No task below adds unit tests for demo-internal code; if a helper proves worth testing, extracting it into core would be a separate change.

## 1. Fulltext search integration (spec: locationlookup-merged-search)

- [x] 1.1 Add `#include <osmscout/db/TextSearchIndex.h>` and load the text index from the database directory; on load failure distinguish "index files missing" from other errors (Requirement: Missing text index handling)
- [x] 1.2 Implement fulltext search with `TextSearchIndex::Search` using the same flags as the JNI bridge (POIs, locations, regions, other; transliterate) (Requirement: Merged structured and fulltext search by default)
- [x] 1.3 Add `--structured-only` and `--fulltext-only` flags (mutually exclusive; error on both) (Requirement: Source selection flags)
- [x] 1.4 Implement merge mirroring the JNI bridge: dedup fulltext hits against structured results by object offset, structured first, fulltext fills remaining limit (Requirement: Merged structured and fulltext search by default)
- [x] 1.5 Count truncated fulltext hits and print a truncated-reporting line (Requirement: Result limit applies to merged list)
- [x] 1.6 Guard fulltext code with `OSMSCOUT_HAVE_LIB_MARISA`; `--fulltext-only` without MARISA prints an explanatory error and exits non-zero (Requirement: Fulltext availability depends on build)

## 2. Ranking display (spec: locationlookup-ranking-display)

- [x] 2.1 Port the `locationRank` formula as named constants: type table (boundary_country 1.0 … default 0.5), match rules (exact 1.0, prefix 0.75, else 0.5), distance rule `1/log((d/1000)+e)` (Requirements: Type rank, Match rank, Distance rank)
- [x] 2.2 Resolve a coordinate per result from its object ref (node/way/area) via the database, and compute distance from the search center (Requirement: Distance rank)
- [x] 2.3 Add `--lat`/`--lon` center flags; without them distance component = 1.0 and header notes "distance rank neutral" (Requirement: Distance rank)
- [x] 2.4 Add `--weights T D M` multiplying the three components (Requirement: Weight overrides)
- [x] 2.5 Sort results by rank descending and print the rank column with components `T·D·M` (Requirements: Rank column with components, Rank-sorted output)

## 3. CLI surface and output (spec: locationlookup-cli)

- [x] 3.1 Replace the verbose per-entry dump with the compact table; keep per-field match qualities (`=`/`~`/`-`), admin region hierarchy, object type, coordinates (Requirement: Compact output table)
- [x] 3.2 Keep `--limit`, `--transliterate`, `--adminRegion`, `--repeat` behavior unchanged (Requirement: Existing flags preserved)
- [x] 3.3 Add timing report split into structured time, fulltext time, total; honor `--repeat` (Requirements: Timing report, Repeat mode still measures performance)
- [x] 3.4 Update `--help` text and the header output (pattern, center, weights, source mode) (Requirement: New flags)

## 4. Build integration

- [x] 4.1 Add the MARISA-gated demo entry to `Demos/CMakeLists.txt` following the `LookupText` pattern (Requirement: Fulltext availability depends on build)
- [x] 4.2 Add the MARISA-gated executable to `Demos/meson.build` following the `LookupText` pattern (Requirement: Fulltext availability depends on build)
- [x] 4.3 Verify the build works both with and without MARISA (Requirement: Fulltext availability depends on build)

## 5. Verification

- [x] 5.1 Build compiles without errors on both build systems, with and without MARISA (change-level apply rule)
- [x] 5.2 Existing test suite still passes (`ctest` / `meson test`) (change-level apply rule)
- [x] 5.3 Smoke test against a real database: run sample queries (e.g. "berlin hauptbahnhof", a plain street name, a POI name) in default, `--structured-only`, and `--fulltext-only` modes; verify source markers, qualities, hierarchy, rank columns, truncated line, timing
- [x] 5.4 Compare output for the same queries against JavaScout's result list ordering (manual, documenting any divergence in rank or order)
- [x] 5.5 Run `--repeat` mode and confirm the timing report covers repeated runs
- [x] 5.6 Run `--weights` experiments (e.g. distance disabled) and confirm order/rank values change as expected
