# design

## ctx

`Demos/src/LocationLookup.cpp` is a CLI demo that searches structured location data via `LocationService::SearchForLocationByString` (plus `SearchForLocationByForm` for default-admin-region resolution), prints a verbose per-entry dump with per-field match qualities and admin hierarchy, and supports `--limit`, `--transliterate`, `--adminRegion`, `--repeat`. OSMScout2 (`LocationSearch.qml`) and JavaScout (`LocationSearchRanker.java`) rank results as `typeRank × distanceRank × matchRank` on the UI side; JavaScout's JNI bridge additionally merges free-text hits from the MARISA text index (`TextSearchIndex::Search`) with structured results. This change makes LocationLookup reproduce that combined pipeline so search strings can be tested and ranking tuned from the command line.

## Goals / Non-Goals

**Goals:**
- Merged structured + fulltext search with source markers and truncation reporting
- Rank-sorted output with explicit rank components, matching the OSMScout2/JavaScout formula
- Tunable rank weights and search center via CLI flags
- Preserve existing flags (`--limit`, `--transliterate`, `--adminRegion`, `--repeat`)
- Fulltext support optional (MARISA-gated), matching the `LookupText` precedent

**Non-Goals:**
- No core `libosmscout` changes — `LocationService`, `TextSearchIndex`, match qualities all exist and are reused
- No changes to JavaScout, OSMScout2, or the JNI bridge
- No interactive/REPL mode — one run per search string
- No GUI, no map rendering

## Sequence

```
User:  LocationLookup DB [flags] QUERY
         │
         ▼
     ┌──────────────────────────────────────────────┐
     │ parse args: source selection, center,        │
     │ weights, limit, transliterate, adminRegion   │
     └──────────────┬───────────────────────────────┘
                    │
        ┌───────────┴───────────┐
        │                       │
        ▼                       ▼
 SearchForLocation    TextSearchIndex::Search
 ByString (structured)  (fulltext, transliterate)
        │                       │
        │   entries with        │   hits → ObjectFileRef
        │   per-field quality   │
        ▼                       ▼
     ┌──────────────────────────────┐
     │ merge (mirror JNI):          │
     │  - dedup fulltext hits       │
     │    against structured        │
     │    by object offset          │
     │  - structured first,         │
     │    fulltext fills limit      │
     │  - count truncated           │
     └──────────────┬───────────────┘
                    ▼
        resolve coords per result (node/way/area)
                    ▼
        rank = (typeRank·T) × (distanceRank·D) × (matchRank·M)
                    ▼
        sort rank desc → print compact table
        + header (pattern, center, weights, source mode)
        + truncated line + timing (structured/fulltext/total)
```

## Decisions

### 1. Where the tool lives

**Chosen:** Extend `Demos/src/LocationLookup.cpp` in place.
**Alternatives:** (a) new demo binary `SearchLab`; (b) extend `Demos/src/LookupText.cpp`.

Rationale: LocationLookup already has the best bones — per-field match qualities, admin hierarchy resolution, object loading, `--repeat` perf mode, transliterate. LookupText is a minimal 2013 interactive loop that would need a rewrite (its identity is raw `TextSearchIndex` access, and the interactive loop is out of scope). A new binary would duplicate LocationLookup's helpers. No docs or tests depend on LocationLookup's output format (verified by repo grep), so changing output is safe.

Risk: existing users of the demo see a different output format. Low impact — it is a demo, and the change is the point.

### 2. Fulltext integration

**Chosen:** Load `TextSearchIndex` directly in the demo, compiled only when MARISA is available (same gating as `LookupText` in both build systems).
**Alternatives:** (a) reuse `libosmscout-client-qt` `SearchModule`; (b) add fulltext search to core `LocationService`.

Rationale: the demo is core-only today; client-qt pulls Qt and its `SearchModule` is built around `DBThread`/`LocationEntry` objects not present in core. Extending `LocationService` would be a core API change for a demo feature — out of scope and against the non-goal. Direct index use follows the existing `LookupText` precedent and keeps the tool's dependencies minimal.

Risk: without MARISA the fulltext features are unavailable; the CLI must fail with an explanatory error (spec: `locationlookup-merged-search`). Mitigated by the `LookupText`-style build gating plus runtime check.

### 3. Ranking formula source

**Chosen:** Port the OSMScout2/JavaScout `locationRank` formula (`typeRank × distanceRank × matchRank`, including the type table) to C++ constants in the demo.
**Alternatives:** (a) display `LocationService` result order without a rank column; (b) move ranking into core `LocationService`.

Rationale: the point of the tool is to test and tune the ranking that JavaScout/OSMScout2 apply on the UI side — `LocationService` orders by match quality but does not apply the type/distance/match product. Porting the formula makes the demo a faithful reproduction. Core change (b) would couple UI ranking policy into the library — rejected.

Risk: formula drift between C++ demo and the Java/JS implementations. Mitigated: weights are overridable via `--weights`, so tuning experiments do not depend on recompiling, and the constants are documented as mirroring `LocationSearch.qml`/`LocationSearchRanker.java`.

### 4. Merge semantics

**Chosen:** Mirror the JNI bridge exactly: structured results first, fulltext fills the remaining limit, fulltext hits duplicating a structured object (same offset) are dropped, and truncated fulltext hits are counted and reported.
**Alternatives:** (a) merge all results and sort the combined list purely by rank.

Rationale: the tool must expose the behavior JavaScout users actually see. The JNI pre-truncation (structured fills the limit before fulltext is considered) is a real result-quality constraint worth surfacing; the truncated-count line makes the loss visible and measurable.

Risk: none — display-only behavior; the rank-sorted view still lets the user evaluate what JavaScout's UI would show within the truncated list.

### 5. Search center

**Chosen:** `--lat`/`--lon` flags; without them, the center is derived from the admin region given via `--adminRegion` (centroid of the region's object). With neither, distance rank is neutral (D = 1.0) and the header says so.
**Alternatives:** (a) require a center always; (b) derive the center from the default admin region's centroid only.

Rationale: explicit flags stay the primary interface (matches LocationLookup's optional-flag style and keeps runs simple). Auto-deriving from `--adminRegion` gives distance values in the common "search within region" workflow without extra flags; the header reports the auto-derived center so the reference point is never hidden.

Risk: without any center the distance column is meaningless; mitigated by the explicit neutral note, D = 1.0, and `-` in the `dist` column.

## Risks

- **MARISA optional**: fulltext features vanish on core-only builds. Mitigated: gated registration in both build systems, explanatory CLI error.
- **Output change**: existing users of the demo see a new format. Low impact, no dependents verified.
- **Formula drift**: C++ rank may diverge from Java/JS. Mitigated: `--weights` for experiments, documented constants.
- **Dedup edge cases**: entries whose coordinates resolve from different object kinds (node/way/area) may report slightly different distances than JavaScout's `LocationEntry.lat/lon`. Accepted — the demo resolves coordinates from the same object refs the JNI uses; minor variance is display-only.
