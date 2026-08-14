# locationlookup-cli

## ADDED Requirements

### Requirement: New flags

The CLI SHALL add `--structured-only`, `--fulltext-only`, `--lat`, `--lon`, and `--weights T D M` as documented in the help text.

#### Scenario: New flags listed in help

Given a built LocationLookup binary
When the user runs LocationLookup with `--help`
Then the help text documents `--structured-only`, `--fulltext-only`, `--lat`, `--lon`, and `--weights`

### Requirement: Existing flags preserved

The existing flags `--limit`, `--transliterate`, `--adminRegion`, and `--repeat` SHALL keep working as before.

#### Scenario: Existing flags accepted

Given a built LocationLookup binary
When the user runs LocationLookup with `--limit 5 --transliterate --adminRegion <name>`
Then the run succeeds and the output header reflects the given limit and transliteration setting

### Requirement: Compact output table

The verbose per-entry dump SHALL be replaced by a compact table. The table SHALL still show per-field match qualities (`=` exact, `~` candidate, `-` none for admin region, location, address, POI, postal area), the admin region hierarchy, object type, and coordinates per result.

#### Scenario: Match qualities and hierarchy visible

Given a search string matching a location with an address
When the user runs LocationLookup
Then the output shows per-field match quality markers, the admin region hierarchy, the object type, and coordinates for the matching result

### Requirement: Timing report

The output SHALL report search timing, split into structured search time and fulltext search time.

#### Scenario: Timing shown after results

Given a built LocationLookup binary
When the user runs LocationLookup for a search string
Then the output ends with a line reporting structured search time, fulltext search time, and total time

### Requirement: Repeat mode still measures performance

With `--repeat N`, the merged search (both sources, per the active source selection) SHALL run N times and the timing report SHALL cover the repeated runs, preserving the demo's performance-testing role.

#### Scenario: Repeat run completes

Given a built LocationLookup binary
When the user runs LocationLookup with `--repeat 3` for a search string
Then the run completes and reports timing for the repeated search
