# locationlookup-search-lab

Extend the `LocationLookup` demo into a search-result inspection tool: merged structured + fulltext search with source selection, ranking display with tunable weights, and a compact rank-sorted output. Purpose: manually test search strings and optimize result ordering against the ranking behavior that JavaScout and OSMScout2 apply.

## What Changes

`LocationLookup` today searches structured location data only (`LocationService`), prints a verbose per-entry dump, and has no ranking information. OSMScout2 and JavaScout combine structured search with free-text search over the text index (MARISA) and then rank results by `typeRank × distanceRank × matchRank` on the UI side. This change turns the demo into a faithful, inspectable reproduction of that combined pipeline so result quality can be tested and tuned from the command line.

## Capabilities

### New Capabilities

- `locationlookup-merged-search`: LocationLookup performs both structured location search and fulltext text-index search, merges them into one result list (deduplicated, limited), marks each result with its source, and reports how many fulltext hits were truncated by the limit. Source selection switches allow running either half alone.
- `locationlookup-ranking-display`: Results are displayed rank-sorted with an explicit rank column showing `typeRank × distanceRank × matchRank` and its components, matching the OSMScout2/JavaScout ranking. Rank weights and the search center are configurable.
- `locationlookup-cli`: The demo's command-line surface and output format. New flags for source selection, search center, and weight overrides; the verbose dump is replaced by a compact table that keeps per-field match qualities, admin region hierarchy, and timing.

## Impact

- `Demos/src/LocationLookup.cpp` — main rework: fulltext search, merge, ranking, new flags, new output format.
- `Demos/CMakeLists.txt` — new MARISA-gated section so fulltext features are optional, following the `LookupText` pattern.
- `Demos/meson.build` — matching MARISA-gated executable entry.
- **No core `libosmscout` changes** — `LocationService`, `TextSearchIndex` already exist and are reused.
- **No changes to** JavaScout, OSMScout2, libosmscout-client-java, or libosmscout-client-qt.
