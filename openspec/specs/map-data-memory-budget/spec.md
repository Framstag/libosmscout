# map-data-memory-budget Specification

## Purpose
Bounds the memory that the cached map data of all open databases occupies, distributes that bound by
which databases actually supply the visible map, releases the caches of databases that stay out of
view, and makes the bound configurable and observable.

## Requirements

### Requirement: Cached map data is accounted in comparable units

The system SHALL account the content of a map data cache in a unit that reflects the amount of memory
the cached objects occupy, so that content of the same unit value occupies a comparable amount of
memory regardless of the kind of objects (points, lines, polygons or index cells) it holds. The
system SHALL report the accounted size of a cache and of all caches together.

#### Scenario: Polygon-heavy content weighs more than point-heavy content

- **WHEN** two caches hold the same number of cached objects, one of polygon objects and one of point objects
- **THEN** the accounted size of the polygon cache SHALL be larger than the accounted size of the point cache

#### Scenario: Reported usage is available for a cache

- **WHEN** a caller asks for the current accounted size of a cache
- **THEN** the system SHALL return a memory figure derived from the accounting of the cached content
- **AND** reading the figure SHALL NOT evict, load or modify any cached content

### Requirement: One budget bounds the caches of all open databases

The system SHALL accept a total memory budget covering the map data caches of all open databases
together. While a budget is configured, the total accounted size of all those caches SHALL NOT exceed
the budget after the system has had the opportunity to react to a change of the cached content. The
budget SHALL be optional: while no budget is configured the system SHALL keep the existing behaviour
of sizing each database's caches independently.

#### Scenario: Total usage stays within the budget

- **WHEN** a budget is configured and the visible map spans several databases whose combined content exceeds the budget
- **THEN** the total accounted size of the map data caches SHALL be at or below the budget

#### Scenario: No budget configured keeps per-database sizing

- **WHEN** no budget is configured and a client sets a cache size for a single database
- **THEN** that database's cache SHALL be bounded by the client's size
- **AND** no other database's cache SHALL be affected by it

### Requirement: The budget is distributed by relevance to the view

The system SHALL treat a database as relevant to the current view when its geographic extent
intersects the area the view covers. Relevant databases SHALL share the budget; a database that is not
relevant SHALL be reduced to a configurable floor instead of retaining a share.

#### Scenario: Only relevant databases hold a share

- **WHEN** several databases are open and the view covers only some of them
- **THEN** the databases the view covers SHALL hold shares of the budget
- **AND** each database the view does not cover SHALL be reduced to the floor

#### Scenario: The floor is kept when everything is out of view

- **WHEN** the view covers no database
- **THEN** every open database SHALL be reduced to the floor
- **AND** the total accounted size SHALL remain at or below the budget

### Requirement: Changes of the relevant set are hysteretic

The system SHALL NOT redistribute the budget on every render. It SHALL apply a distribution only after
the set of relevant databases has been stable for a configurable settling period, and a database that
became relevant again within that period SHALL keep its share.

#### Scenario: A short excursion out of view keeps the share

- **WHEN** a database stops being relevant and becomes relevant again within the settling period
- **THEN** its share SHALL NOT have been reduced in between

#### Scenario: A stable change of the relevant set is applied

- **WHEN** the set of relevant databases is unchanged for the settling period
- **THEN** the system SHALL apply the distribution for that set
- **AND** the total accounted size SHALL be at or below the budget afterwards

### Requirement: Databases that stay out of view release their caches

The system SHALL release the cached content of a database that has not been relevant for longer than a
configurable idle period, in addition to reducing it to the floor. A released database SHALL reload
its data when it becomes relevant again.

#### Scenario: Long idleness releases the content

- **WHEN** a database has not been relevant for longer than the idle period
- **THEN** its accounted size SHALL be zero

#### Scenario: A released database works again

- **WHEN** a database whose caches were released becomes relevant again
- **THEN** loading and rendering SHALL succeed
- **AND** its accounted size SHALL rise above zero again

### Requirement: Memory bounding does not change the rendered map

The system SHALL produce the same map content with and without a budget. Bounding memory SHALL only
affect how much data is kept resident and how often data is re-read, never which objects a view
contains.

#### Scenario: The same objects are rendered with a small and a large budget

- **WHEN** the same view is rendered with a budget far below the data it needs and with a budget far above it
- **THEN** both renders SHALL contain the same objects of every kind

#### Scenario: Bounding does not fail a render

- **WHEN** a budget is configured that is much smaller than the data of a single view
- **THEN** rendering SHALL complete successfully

### Requirement: The budget is configurable and observable by clients

A client SHALL be able to set the total budget in memory units and to read the current total usage.
The shipped clients SHALL configure a default budget when the application does not set one, while
tools and tests that do not configure a budget SHALL keep their current behaviour.

#### Scenario: A client sets a budget and reads the usage

- **WHEN** a client sets a budget and renders
- **THEN** reading the total usage SHALL return a value at or below the configured budget

#### Scenario: Shipped clients bound memory without configuration

- **WHEN** an application using a shipped client renders a view spanning several databases without setting a budget
- **THEN** the total accounted size of the map data caches SHALL be bounded by a default

#### Scenario: Tools that configure nothing are unaffected

- **WHEN** a tool or test that does not configure a budget renders
- **THEN** its cache sizes SHALL be the ones it configured
