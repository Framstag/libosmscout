# Spec Delta

## ADDED Requirements

### Requirement: POI search covers every loaded database

`searchPOIs` SHALL query every loaded non-basemap database whose area can hold the requested radius,
independently of how many results an earlier database already contributed. A database that alone
satisfied the requested result limit SHALL NOT prevent the remaining databases from being searched.
The requested limit SHALL apply to the merged result, not to an individual database.

#### Scenario: A hit that only a later database holds is found

- **GIVEN** two loaded databases whose areas both contain the search center
- **AND** only the database searched second contains a POI of the requested category within the radius
- **WHEN** `searchPOIs` is called for that category and center
- **THEN** the result SHALL contain that POI

#### Scenario: A database filling the limit does not hide nearer results

- **GIVEN** a loaded database that returns at least `limit` results on its own
- **AND** a second loaded database containing a POI nearer to the search center than the farthest
  entry the first database contributed
- **WHEN** `searchPOIs` is called with that `limit`
- **THEN** the result SHALL contain the nearer POI
- **AND** the result SHALL NOT contain more than `limit` entries

#### Scenario: The limit applies to the merged list

- **GIVEN** several loaded databases together containing more POIs of the category than `limit`
- **WHEN** `searchPOIs` is called with that `limit`
- **THEN** the result SHALL NOT contain more than `limit` entries

### Requirement: A POI present in several databases is returned once

A POI contained in more than one loaded database SHALL appear at most once in the result. The
surviving entry SHALL be the copy from the database whose area contains the search center; among
several such databases it SHALL be the copy of the database ordered first by the client.

#### Scenario: Duplicated POI collapses to one entry

- **GIVEN** two overlapping loaded databases that both contain the search center and both contain the
  same POI
- **WHEN** `searchPOIs` is called for the POI's category and that center
- **THEN** the result SHALL contain exactly one entry for that POI

#### Scenario: The copy from the database containing the search center survives

- **GIVEN** two overlapping loaded databases that both contain the same POI
- **AND** only one of them contains the search center
- **WHEN** `searchPOIs` is called for the POI's category and that center
- **THEN** the surviving entry SHALL be the copy of the database that contains the search center
- **AND** the resulting entry SHALL be the entry the same search returns with only that database
  loaded

### Requirement: POI search result order is deterministic

Results SHALL be ordered nearest first. Entries at the same distance SHALL be ordered by a criterion
that does not depend on the order in which the databases were discovered on disk, so that repeating
the same search over the same set of databases yields the same list.

#### Scenario: Repeated searches yield the same list

- **GIVEN** a set of loaded databases whose areas overlap around the search center
- **WHEN** the same `searchPOIs` call is issued twice
- **THEN** both calls SHALL return the same entries in the same order

#### Scenario: Same-distance entries have a stable order

- **GIVEN** two loaded databases each contributing a POI at the same distance from the search center
- **AND** the labels of the two POIs differ
- **WHEN** `searchPOIs` is called for their category and that center
- **THEN** the entry whose label sorts first SHALL be listed first
- **AND** the result order SHALL be the same for databases discovered in either order
