# Spec Delta

## Purpose
Scope location searches to the current map region via a default admin region, improving relevance and latency, matching OSMScout2's `SetDefaultAdminRegion` behavior.

## ADDED Requirements

### Requirement: A scoped search stays inside the extent of the resolved region

A search with a default admin region SHALL admit only hits whose position lies inside the geographic extent
of that region, whichever loaded database produced the hit, including hits from the free-text index. The
extent SHALL be derived from the object that represents the region and SHALL be a superset of the region.
For a region represented only by a position, the extent SHALL be a documented box around that position.

#### Scenario: A hit inside the extent is admitted

- **GIVEN** a scoped search whose resolved region has an extent
- **WHEN** a database produces a hit whose position lies inside that extent
- **THEN** the hit SHALL be part of the result set

#### Scenario: A hit outside the extent is rejected

- **GIVEN** the same scoped search
- **WHEN** another database's text index produces a hit whose position lies outside the extent
- **THEN** that hit SHALL NOT be part of the result set

#### Scenario: The extent never drops a hit inside its own region

- **GIVEN** a region and its derived extent
- **WHEN** a hit is placed inside the region
- **THEN** its position SHALL lie inside the extent

#### Scenario: A region represented only by a position gets a box around it

- **GIVEN** a resolved region whose object is a single position rather than an area or a way
- **WHEN** its extent is derived
- **THEN** the extent SHALL be the documented box around that position
- **AND** it SHALL admit a position inside the box and reject one well outside it

### Requirement: An unavailable extent leaves the search unscoped

When no extent can be established for the scope — no region was resolved, the region's object cannot be
loaded, or the derived box is invalid — the filter SHALL admit every position, so the search degrades to
the unscoped behaviour instead of returning nothing.

#### Scenario: An unestablished extent admits every position

- **GIVEN** a scoped search whose region provided no usable extent
- **WHEN** hits from any database are considered
- **THEN** each hit SHALL be admitted regardless of its position

### Requirement: An unusable position is never inside a set extent

A position that is not finite SHALL NOT be reported as inside a set extent, because it cannot be placed in
the scope and is not a usable result either.

#### Scenario: A non-finite position is rejected

- **GIVEN** a set extent
- **WHEN** a hit position has a non-finite coordinate
- **THEN** the position SHALL NOT be inside the extent

### Requirement: A search result reports whether it lies inside the active scope

Every search result SHALL report whether its position lies inside the extent of the active default admin
region. A search without a scope SHALL report every result as inside it. A report SHALL be a value a
consumer can order by, so it can place an out-of-scope hit below an in-scope one.

#### Scenario: A scoped search reports the verdict per result

- **GIVEN** a scoped search whose region has an extent
- **WHEN** the result set is returned
- **THEN** a result inside the extent SHALL report that it lies inside the scope
- **AND** a result outside it SHALL report that it does not

#### Scenario: A search without a scope reports every result as inside

- **GIVEN** a search with no default admin region
- **WHEN** the result set is returned
- **THEN** every result SHALL report that it lies inside the scope
