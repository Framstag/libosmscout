# locationlookup-ranking-display

## ADDED Requirements

### Requirement: Rank column with components

Each displayed result SHALL show its rank and the three components `typeRank × distanceRank × matchRank`, matching the ranking formula used by OSMScout2 (`locationRank`) and JavaScout (`LocationSearchRanker`). Rank values range over the product of the three components; coordinate results do not apply to this demo.

#### Scenario: Rank components are visible

Given a database with searchable content
When the user runs LocationLookup for a search string
Then every displayed result includes a rank value and the three component values that produced it

### Requirement: Rank-sorted output

Results SHALL be displayed sorted by rank, highest first.

#### Scenario: Results ordered by rank

Given a search with results of different ranks
When the user runs LocationLookup
Then results appear in descending order of their displayed rank

### Requirement: Match rank

The match component SHALL be 1.0 for an exact label match with the search string, 0.75 for a prefix match, and 0.5 otherwise.

#### Scenario: Exact match ranks above prefix match

Given a search string that is the exact label of one result and a prefix of another result's label
When the user runs LocationLookup
Then the exact-match result shows match component 1.0 and ranks above the prefix-match result showing 0.75

### Requirement: Type rank

The type component SHALL follow the OSMScout2/JavaScout type table: `boundary_country` 1.0, `boundary_state` 0.93, `boundary_administrative`/`place_town` 0.9, `highway_residential`/`address` 0.8, `railway_station`/`railway_tram_stop`/`railway_subway_entrance`/`highway_bus_stop` 0.7, all other types 0.5.

#### Scenario: Type ranks follow the table

Given results of types `boundary_country`, `place_town`, and `amenity_cafe` for the same search string at the same location
When the user runs LocationLookup
Then the displayed type components are 1.0, 0.9, and 0.5 respectively

### Requirement: Distance rank

With a search center given via `--lat` and `--lon`, the distance component SHALL be `1 / log((distanceMeters / 1000) + e)`. Without a search center, the distance component SHALL be 1.0 and the output SHALL state that the distance rank is neutral.

#### Scenario: Distance component computed from center

Given a database with two results at different distances from the search center
When the user runs LocationLookup with `--lat` and `--lon` for the center
Then each result's distance component matches `1 / log((d / 1000) + e)` for its distance `d` in meters

#### Scenario: Neutral distance without center

Given a database with searchable content
When the user runs LocationLookup without `--lat` and `--lon` and without `--adminRegion`
Then every result shows distance component 1.0 and the output notes the distance rank is neutral

#### Scenario: Distance column with center

Given a database with searchable content and a search center given via `--lat` and `--lon`
When the user runs LocationLookup
Then each result with coordinates shows its distance from the search center in the `dist` column

#### Scenario: Center derived from admin region

Given a database with searchable content and `--adminRegion` given but no `--lat`/`--lon`
When the user runs LocationLookup
Then the header reports a search center derived from the admin region and each result with coordinates shows its distance from that center in the `dist` column

#### Scenario: Distance column without center or admin region

Given a database with searchable content
When the user runs LocationLookup without `--lat`/`--lon` and without `--adminRegion`
Then each result row shows `-` in the `dist` column

### Requirement: Weight overrides

`--weights T D M` SHALL scale the three rank components by the given factors and re-sort accordingly.

#### Scenario: Weights change order

Given a search where the best-matching result is far away and a worse-matching result is close
When the user runs LocationLookup with `--weights "1 0 1"` (distance rank disabled)
Then the displayed order and rank values reflect the overridden weights, with distance no longer affecting the rank
